#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "orchard.h"
#include "orchard-app.h"
#include "orchard-events.h"
#include "orchard-math.h"
#include "touch.h"
#include "orchard-ui.h"
#include "charger.h"
#include "paging.h"
#include "led.h"
#include "genes.h"
#include "storage.h"
#include "radio.h"
#include "TransceiverReg.h"
#include "userconfig.h"
#include "accel.h"
#include "mic.h"
#include "time.h"
#include "cmd-forward.h"
#include "address.h"
#include "baton.h"

#include "shellcfg.h"

/*

  key layout to code
   4       3
 5   6   1   2
   7       0

   0x80 = down
   0x40 = right
   0x20 = left
   0x10 = up

   0x02 = home
   0x04 = select
   0x01 = TBD (down)
   0x08 = TBD (up)
 */

#define SEXTEST 0
#define SEX_TURNAROUND_TIME 4000 // 4 seconds for sex -- mostly to give sender time to "shake it"

extern uint8_t sex_running;  // from app-default.c
extern uint8_t sex_done;

orchard_app_end();

static const OrchardApp *orchard_app_list;
static virtual_timer_t run_launcher_timer;
static bool run_launcher_timer_engaged;
#define RUN_LAUNCHER_TIMEOUT MS2ST(500)

orchard_app_instance instance;  // the one and in fact only instance of any orchard app

event_source_t orchard_app_terminated;
event_source_t orchard_app_terminate;
event_source_t timer_expired;
event_source_t ui_completed;

static uint16_t  captouch_collected_state = 0;

#define COLLECT_INTERVAL 50  // time to collect events for multi-touch gesture
#define TRACK_INTERVAL 1  // trackpad debounce in ms

static virtual_timer_t chargecheck_timer;
static event_source_t chargecheck_timeout;
#define CHARGECHECK_INTERVAL 1000 // time between checking state of charger and battery
// note if this goes below 1s, we'll have to change the uptime counter which currently does whole seconds only

// 3.3V (3300mV) is threshold for OLED failure; 50mV for margin
#define BRIGHT_THRESH  3550     // threshold to limit brightness
#define SAFETY_THRESH  3470     // threshold to go into safety mode
#define SHIPMODE_THRESH  3330   // threshold to go into ship mode

static virtual_timer_t ping_timer;
static event_source_t ping_timeout;
#define PING_MIN_INTERVAL  5000 // base time between pings
#define PING_RAND_INTERVAL 2000 // randomization zone for pings
static char *friends[MAX_FRIENDS]; // array of pointers to friends' names; first byte is priority metric
#define FRIENDS_INIT_CREDIT  4  // defines how long a friend record stays around before expiration
// max level of credit a friend can have; defines how long a record can stay around
// once a friend goes away. Roughly equal to
// 2 * (PING_MIN_INTERVAL + PING_RAND_INTERVAL / 2 * MAX_CREDIT) milliseconds
#define FRIENDS_MAX_CREDIT   12
#define FRIENDS_SORT_HYSTERESIS 4 

static uint8_t cleanup_state = 0;
mutex_t friend_mutex;

static uint8_t ui_override = 0;

uint32_t uptime = 0;

bool timekeeper = false;

void friend_cleanup(void);

#define MAIN_MENU_MASK  0x02
#define MAIN_MENU_VALUE 0x02

//#define TEST_WATCHDOG  // uncomment to force a reset when watchdog is enabled
// note -- watchdog is in hal_lld.c inside the K22x ports directory

void wdogPing(void) {
#ifdef TEST_WATCHDOG
  // do nothing if we're testing the watchdog
#else  
  WDOG->REFRESH = 0xA602;
  WDOG->REFRESH = 0xB480;
#endif
}

static void handle_radio_page(eventid_t id) {
  (void) id;
  uint8_t oldfx;

  ui_override = 1;
  oldfx = effectsGetPattern();
  effectsSetPattern(effectsNameLookup("strobe"));
  radioPagePopup();
  effectsSetPattern(oldfx);
  ui_override = 0;
  analogUpdateMic();
}

static void handle_ping_timeout(eventid_t id) {
  (void) id;
  const struct genes *family;
  family = (const struct genes *) storageGetData(GENE_BLOCK);

  if( !anonymous ) {
    radioAcquire(radioDriver);
    radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_ping,
	      strlen(family->name) + 1, family->name);
    radioRelease(radioDriver);
  }
    
  // cleanup every other ping we send, to make sure friends that are
  // nearby build up credit over time to max credits

  // if the system is unstable, tweak this parameter to reduce the
  // clean-up rate
  if( cleanup_state ) {
    friend_cleanup();
    chEvtBroadcast(&radio_app);
  }
  cleanup_state = !cleanup_state;
}

char *friend_lookup(char *name) {
  int i;

  osalMutexLock(&friend_mutex);
  for( i = 0; i < MAX_FRIENDS; i++ ) {
    if( friends[i] != NULL ) {
      if(0 == strncmp(&(friends[i][1]), name, GENE_NAMELENGTH)) {
	osalMutexUnlock(&friend_mutex);
	return friends[i];
      }
    }
  }
  osalMutexUnlock(&friend_mutex);
  return NULL;
}

uint8_t friendCount(void) {
  int i;
  uint8_t count = 0;
  
  osalMutexLock(&friend_mutex);
  for( i = 0; i < MAX_FRIENDS; i++ ) {
    if( friends[i] != NULL )
      count++;
  }
  osalMutexUnlock(&friend_mutex);
  
  return count;
}

char *friend_add(char *name) {
  char *record;
  uint32_t i;

  record = friend_lookup(name);
  if( record != NULL )
    return record;  // friend already exists, don't add it again

  osalMutexLock(&friend_mutex);
  for( i = 0; i < MAX_FRIENDS; i++ ) {
    if( friends[i] == NULL ) {
      friends[i] = (char *) chHeapAlloc(NULL, GENE_NAMELENGTH + 2); // space for NULL + metric byte
      if( friends[i] != NULL ) {
	friends[i][0] = FRIENDS_INIT_CREDIT;
	strncpy(&(friends[i][1]), name, GENE_NAMELENGTH);
	osalMutexUnlock(&friend_mutex);
	return friends[i];
      }
    }
  }
  osalMutexUnlock(&friend_mutex);

  // if we got here, we couldn't add the friend because we ran out of space
  return NULL;
}

// generate a one-time list of random names and populate the friend list
// for testing only
void cmd_friendlocal(BaseSequentialStream *chp, int argc, char *argv[]) {
  char tempName[GENE_NAMELENGTH];
  uint32_t i;
  uint32_t total;
  
  if( argc != 1 ) {
    chprintf(chp, "usage: friendlocal <friendcount>\n\r");
  }
  total = strtoul(argv[0], NULL, 0);

  for(i = 0; i < total; i++ ) {
    generateName(tempName);
    friend_add(tempName);
  }
}
//orchard_command("friendlocal", cmd_friendlocal);

// clear the friend table, e.g. when we change channels or go silent
void friendClear(void) {
  uint32_t i;
  uint32_t cleared = 1;

  osalMutexLock(&friend_mutex);
  do {
    cleared = 1;
    for( i = 0; i < MAX_FRIENDS; i++ ) {
      if( friends[i] == NULL )
	continue;

      friends[i][0]--;
      if( friends[i][0] == 0 ) {
	chHeapFree(friends[i]);
	friends[i] = NULL;
      } else {
	cleared = 0;
      }
    }
  } while( cleared == 0 );
  osalMutexUnlock(&friend_mutex);
}

// to be called periodically to decrement credits and de-alloc friends we haven't seen in a while
void friend_cleanup(void) {
  uint32_t i;

  osalMutexLock(&friend_mutex);
  for( i = 0; i < MAX_FRIENDS; i++ ) {
    if( friends[i] == NULL )
      continue;

    friends[i][0]--;
    if( friends[i][0] == 0 ) {
      chHeapFree(friends[i]);
      friends[i] = NULL;
    }
  }
  osalMutexUnlock(&friend_mutex);
}

int friend_comp(const void *a, const void *b) {
  char **mya;
  char **myb;
  
  mya = (char **)a;
  myb = (char **)b;

  if( *mya == NULL && *myb == NULL )
    return 0;

  if( *mya == NULL )
    return 1;

  if( *myb == NULL )
    return -1;

  if( ((*mya)[0] != (*myb)[0]) &&
      (((*mya)[0] < (FRIENDS_MAX_CREDIT - FRIENDS_SORT_HYSTERESIS)) ||
       ((*myb)[0] < (FRIENDS_MAX_CREDIT - FRIENDS_SORT_HYSTERESIS))) ) {
    return (*mya)[0] > (*myb)[0] ? -1 : 1;
  } else {
    // sort alphabetically from here
    return strncmp(&((*mya)[1]), &((*myb)[1]), GENE_NAMELENGTH + 1);
  }
}

void friendsSort(void) {
  osalMutexLock(&friend_mutex);
  qsort(friends, MAX_FRIENDS, sizeof(char *), friend_comp);
  osalMutexUnlock(&friend_mutex);
}

void friendsLock(void) {
  osalMutexLock(&friend_mutex);
}

void friendsUnlock(void) {
  osalMutexUnlock(&friend_mutex);
}

const char **friendsGet(void) {
  return (const char **) friends;   // you shouldn't modify this record outside of here, hence const
}

static void radio_ping_received(uint8_t prot, uint8_t src, uint8_t dst,
                                   uint8_t length, const void *data) {
  (void) prot;
  (void) src;
  (void) dst;
  (void) length;
  char *friend;

  if(anonymous) // ignore pings in anonymous mode
    return;
  
  friend = friend_lookup((char *) data);
  if( friend == NULL )
    friend = friend_add((char *) data);

  if( friend == NULL ) // we simply can't take any more friends, drop new requests until old ones expire
    return;

  if(friend[0] < FRIENDS_MAX_CREDIT)
    friend[0]++;

  chEvtBroadcast(&radio_app);
}

static void meiosis(genome *gamete, const genome *haploidM, const genome *haploidP) {
  uint32_t xover = rand();
  
  // create a gamete by picking chromosomes randomly from one parent or the other
  if( xover & 1 ) {
    gamete->cd_period = haploidM->cd_period;
    gamete->cd_rate = haploidM->cd_rate;
    gamete->cd_dir = haploidM->cd_dir;
  } else {
    gamete->cd_period = haploidP->cd_period;
    gamete->cd_rate = haploidP->cd_rate;
    gamete->cd_dir = haploidP->cd_dir;
  }
  xover >>= 1;
      
  if( xover & 1 )
    gamete->sat = haploidM->sat;
  else
    gamete->sat = haploidP->sat;
  xover >>= 1;
      
  if( xover & 1 ) {
    gamete->hue_ratedir = haploidM->hue_ratedir;
    gamete->hue_base = haploidM->hue_base;
    gamete->hue_bound = haploidM->hue_bound;
  }	else {
    gamete->hue_ratedir = haploidP->hue_ratedir;
    gamete->hue_base = haploidP->hue_base;
    gamete->hue_bound = haploidP->hue_bound;
  }
  xover >>= 1;

  if( xover & 1 )
    gamete->lin = haploidM->lin;
  else
    gamete->lin = haploidP->lin;
  xover >>= 1;
      
  if( xover & 1 )
    gamete->strobe = haploidM->strobe;
  else
    gamete->strobe = haploidP->strobe;
  xover >>= 1;
  
  if( xover & 1 )
    gamete->accel = haploidM->accel;
  else
    gamete->accel = haploidP->accel;
  xover >>= 1;
      
  if( xover & 1 )
    gamete->nonlin = haploidM->nonlin;
  else
    gamete->nonlin = haploidP->nonlin;
  xover >>= 1;
      
  if( xover & 1 )
    strncpy( gamete->name, haploidM->name, GENE_NAMELENGTH );
  else
    strncpy( gamete->name, haploidP->name, GENE_NAMELENGTH );
}

static uint8_t mfunc(uint8_t gene, uint8_t bits, uint32_t r) {
  return gray_decode(gray_encode(gene) ^ (bits << ((r >> 8) & 0x7)) );
}

static void mutate(genome *gamete, uint8_t mutation_rate) {
  uint32_t r;
  uint8_t bits;
  char genName[GENE_NAMELENGTH];

  // amplify mutation rate
  if( mutation_rate < 128 )
    bits = 1;
  else if( mutation_rate < 245 )
    bits = 3;
  else
    bits = 7;  // radioactive levels of mutation
  
  r = rand();
  if( (r & 0xFF) < mutation_rate ) {
    gamete->cd_period = mfunc(gamete->cd_period, bits, r) % 6;
  }
  
  r = rand();
  if( (r & 0xFF) < mutation_rate ) {
    gamete->cd_rate = mfunc(gamete->cd_rate, bits, r);
  }

  r = rand();
  if( (r & 0xFF) < mutation_rate ) {
    gamete->cd_dir = mfunc(gamete->cd_dir, bits, r);
  }

  r = rand();
  if( (r & 0xFF) < mutation_rate ) {
    gamete->sat = mfunc(gamete->sat, bits, r);
  }

  r = rand();
  if( (r & 0xFF) < mutation_rate ) {
    gamete->hue_ratedir = mfunc(gamete->hue_ratedir, bits, r);
  }

  r = rand();
  if( (r & 0xFF) < mutation_rate ) {
    gamete->hue_base = mfunc(gamete->hue_base, bits, r);
  }

  r = rand();
  if( (r & 0xFF) < mutation_rate ) {
    gamete->hue_bound = mfunc(gamete->hue_bound, bits, r);
  }

  r = rand();
  if( (r & 0xFF) < mutation_rate ) {
    gamete->lin = mfunc(gamete->lin, bits, r);
  }

  r = rand();
  if( (r & 0xFF) < mutation_rate ) {
    gamete->strobe = mfunc(gamete->strobe, bits, r);
  }

  r = rand();
  if( (r & 0xFF) < mutation_rate ) {
    gamete->accel = mfunc(gamete->accel, bits, r);
  }

  r = rand();
  if( (r & 0xFF) < mutation_rate ) {
    gamete->nonlin = mfunc(gamete->nonlin, bits, r);
  }

  r = rand();
  if( (r & 0xFF) < mutation_rate ) {
    generateName(genName);
    strncpy(gamete->name, genName, GENE_NAMELENGTH);
  }
}

static void handle_radio_sex_ack(uint8_t prot, uint8_t src, uint8_t dst,
                                   uint8_t length, const void *data) {
  (void) prot;
  (void) src;
  (void) dst;
  (void) length;
  
  genome *sperm;
  genome egg;
  uint8_t mutation_rate;
  struct genes *newfam;
  const struct genes *oldfam;
  int i;
  uint8_t curfam = 0;
  char *target;
  
  if( !sex_running )  // check to avoid people from sending acks without reqs
    return;
  
  oldfam = (const struct genes *) storageGetData(GENE_BLOCK);
  target = (char *)(data + sizeof(genome));
  if(strncmp(target, oldfam->name, GENE_NAMELENGTH) != 0)
    return; // I was expecting sex, someone acked, but actually I was hearing my neighbors doing it

  configIncSexResponses(); // record # times we've had sex
  newfam =  (struct genes *) chHeapAlloc(NULL, sizeof(struct genes));
  osalDbgAssert( newfam != NULL, "couldn't allocate space for the new family\n\r" );
  
  sperm = (genome *)data;
  mutation_rate = getMutationRate();
  
  newfam->signature = GENE_SIGNATURE;
  newfam->version = GENE_VERSION;
  strncpy(newfam->name, oldfam->name, GENE_NAMELENGTH);
#ifdef WHOLE_FAMILY
  (void) curfam;
  for( i = 0; i < GENE_FAMILYSIZE; i++ ) {
    meiosis(&egg, &(oldfam->haploidM[i]), &(oldfam->haploidP[i]));

    mutate(&egg, mutation_rate);
    mutate(sperm, mutation_rate);
    memcpy(&(newfam->haploidM[i]), &egg, sizeof(genome));
    memcpy(&(newfam->haploidP[i]), sperm, sizeof(genome));
  }
#else
  // ASSUME: current effect is in fact an Lg-series effect...
  // you know what happens when you assume? :P  
  //curfam = effectsCurName()[2] - '0';
  curfam = 0; // cube master only has one pattern, hardcoding to 0.
  meiosis(&egg, &(oldfam->haploidM[curfam]), &(oldfam->haploidP[curfam]));
  for( i = 0; i < GENE_FAMILYSIZE; i++ ) {
    if( i != curfam ) {
      // preserve other family members
      memcpy(&(newfam->haploidM[i]), &(oldfam->haploidM[i]), sizeof(genome));
      memcpy(&(newfam->haploidP[i]), &(oldfam->haploidP[i]), sizeof(genome));
    } else {
      // just make one new baby
      mutate(&egg, mutation_rate);
      mutate(sperm, mutation_rate);
      memcpy(&(newfam->haploidM[i]), &egg, sizeof(genome));
      memcpy(&(newfam->haploidP[i]), sperm, sizeof(genome));
    }
  }
#endif
  
  storagePatchData(GENE_BLOCK, (uint32_t *) newfam, GENE_OFFSET, sizeof(struct genes));
  chHeapFree(newfam);
  sex_done = 1;
}

#if SEXTEST
void handle_radio_sex_req(uint8_t prot, uint8_t src, uint8_t dst,
                                   uint8_t length, const void *data) {
#else
static void handle_radio_sex_req(uint8_t prot, uint8_t src, uint8_t dst,
                                   uint8_t length, const void *data) {
#endif
  (void) prot;
  (void) src;
  (void) dst;
  (void) length;
  const struct genes *family;
  uint8_t family_member = 0;
  genome  gamete;
  const userconfig *config;
  uint8_t consent = 0;
  char *who;
  char  response[sizeof(genome) + GENE_NAMELENGTH + 1];

  family = (const struct genes *) storageGetData(GENE_BLOCK);

  if( strncmp((char *)data, family->name, GENE_NAMELENGTH) == 0 ) {
    who = &(((char *)data)[strlen(family->name)+1]);
    config = getConfig();
    if( config->cfg_autosex == 0 ) {
      // UI prompt and escape with return if denied
      ui_override = 1;
      consent = getConsent(who);
      chThdSleepMilliseconds(300); // clear event queues
      ui_override = 0;
      analogUpdateMic(); // need this to restart the oscope if it's running
      if( !consent )
	return;
    } else {
      chThdSleepMilliseconds(SEX_TURNAROUND_TIME); // wait a little bit before responding to sex query
    }
    configIncSexResponses(); // record # times we've had sex
    // sex with me!    
    //family_member = effectsCurName()[2] - '0';
    family_member = 0; //there's only one effect in the cube master badge, hardcoding to 0.

    // silly biologists, they should have called it create_gamete
    meiosis(&gamete, &(family->haploidM[family_member]),
      &(family->haploidP[family_member]));
#if SEXTEST
    handle_radio_sex_ack(radio_prot_sex_ack, 255, 255, sizeof(genome), &gamete);
#else
    memcpy(response, &gamete, sizeof(genome));
    strncpy(&(response[sizeof(genome)]), who, GENE_NAMELENGTH);
    radioAcquire(radioDriver);
    sexmode = 1;
    radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_sex_ack,
  sizeof(response), response);
    sexmode = 0;
    radioRelease(radioDriver);
#endif
  }
}

static void handle_chargecheck_timeout(eventid_t id) {
  (void)id;
  struct accel_data accel; // for entropy call
  int16_t voltage;
  const struct userconfig *config;
  config = getConfig();

  wdogPing(); // ping the watchdog
  
  voltage = ggVoltage();
  if( (((uptime / 60) % 5) == 0) && ((uptime % 60) == 0) ) { // update the uptime, batt state once every 5 mins
    chprintf(stream, "Uptime: %dh %dm %ds\n\r", uptime / 3600, (uptime / 60) % 60, uptime % 60);
    chprintf(stream, "Volts: %dmV Soc: %d%% Stat: %s Fault: %s\n\r", voltage, ggStateofCharge(), chgStat(), chgFault());
    chprintf(stream, "last_time: %d\n\r", getNetworkTimeMs() );
  }
  
  uptime += CHARGECHECK_INTERVAL / 1000; // keep an uptime count in seconds
  
  // whenever this system task runs, add some entropy to the random number pool...
  accelPoll(&accel);
  addEntropy(accel.x ^ accel.y ^ accel.z);

  // flush config data if it's changed
  configLazyFlush();

  // check if battery is too low, and shut down if it is
  // but only if we're not plugged in to a charger
  if( (voltage < SHIPMODE_THRESH) && (isCharging() == 0) ) {  
    chargerShipMode();  // requires plugging in to re-active battery
  }

  if( voltage < BRIGHT_THRESH ) { // limit brightness when battery is weak
    if( getShift() < 3 )
      setShift(3);
  }

  if( voltage < SAFETY_THRESH ) {  // drop to saftey pattern to notify user of battery almost dead
    if( effectsGetPattern() != effectsNameLookup("safetyPattern") )
      effectsSetPattern(effectsNameLookup("safetyPattern"));

    // limit brightness to guarantee ~2 hours runtime in safety mode
    if( getShift() < 4 )
      setShift(4);
  }

  if (timekeeper && (uptime % config->cfg_timesync_interval) == 0) { // Run every timesync interval
    chprintf(stream, "Broadcasting time: %d\n\r", getNetworkTimeMs());
    broadcastTime();
  }

  // "pump" is now gone when switching interrupt mode from crcOK to packet ready
  uint8_t dummy;
  radioAcquire(radioDriver);
  dummy = radioRead(radioDriver, RADIO_IrqFlags2); // this "pump" is necessary to get the Rx interrupt to fire
  (void) dummy;
  radioRelease(radioDriver);
  /// chprintf(stream, "radio flags: %x\r\n", dummy);  /// TODO: FIGURE OUT WHY THIS IS NECESSARY????
}

static int captouch_to_key(uint8_t code) {
  if (code == 0x80)
    return keyBottom;
  if (code == 0x40)
    return keyRight;
  if (code == 0x20)
    return keyLeft;
  if (code == 0x10)
    return keyTop;
  if (code == MAIN_MENU_MASK)
    return keyHome;
  if( code == 0x04)
    return keySelect;
  
  // these are provisional
  if (code == 0x01)
    return keyBottomR;
  if (code == 0x08)
    return keyTopR;
  return code;
}

static void run_launcher(void *arg) {

  (void)arg;

  chSysLockFromISR();
  /* Launcher is the first app in the list */
  instance.next_app = orchard_app_list;
  instance.thr->flags |= CH_FLAG_TERMINATE;
  chEvtBroadcastI(&orchard_app_terminate);
  run_launcher_timer_engaged = false;
  chSysUnlockFromISR();
}

extern int start_test;
static void poke_run_launcher_timer(eventid_t id) {

  (void)id;

  uint8_t val = captouchRead();
  if (run_launcher_timer_engaged) {
    /* Timer is engaged, but both buttons are still held.  Do nothing.*/
    if ((val & MAIN_MENU_MASK) == MAIN_MENU_VALUE)
      return;

    /* One or more buttons was released.  Cancel the timer.*/
    chVTReset(&run_launcher_timer);
    run_launcher_timer_engaged = false;
  }
  else {
    /* Timer not engaged, and the magic sequene isn't held.  Do nothing.*/
    if ((val & MAIN_MENU_MASK) != MAIN_MENU_VALUE)
      return;

    /* Start the sequence to go to the main menu when we're done.*/
    run_launcher_timer_engaged = true;
    if( start_test == 0 ) { // fast timeout if we're not in test mode
      chVTSet(&run_launcher_timer, RUN_LAUNCHER_TIMEOUT, run_launcher, NULL);
    } else {
      chVTSet(&run_launcher_timer, 3000, run_launcher, NULL); // hold for three seconds in this case
    }
  }
}

static void ui_complete_cleanup(eventid_t id) {
  (void)id;
  OrchardAppEvent evt;
  
  // unhook the UI patch so key & dial events pass into the app
  instance.ui = NULL;

  evt.type = uiEvent;
  evt.ui.code = uiComplete;
  evt.ui.flags = uiOK;
  if( !ui_override )
    instance.app->event(instance.context, &evt);  
}

static void run_chargecheck(void *arg) {
  (void)arg;

  chSysLockFromISR();
  chEvtBroadcastI(&chargecheck_timeout);
  chVTSetI(&chargecheck_timer, MS2ST(CHARGECHECK_INTERVAL), run_chargecheck, NULL);
  chSysUnlockFromISR();
}

static void run_ping(void *arg) {
  (void)arg;

  chSysLockFromISR();
  chEvtBroadcastI(&ping_timeout);
  chVTSetI(&ping_timer, MS2ST(PING_MIN_INTERVAL + rand() % PING_RAND_INTERVAL), run_ping, NULL);
  chSysUnlockFromISR();
}

static void adc_temp_event(eventid_t id) {
  (void) id;
  OrchardAppEvent evt;

  evt.type = adcEvent;
  evt.adc.code = adcCodeTemp;
  if( !ui_override )
    instance.app->event(instance.context, &evt);
}

static void radio_app_event(eventid_t id) {
  (void)id;
  OrchardAppEvent evt;

  evt.type = radioEvent;
  if( !ui_override )
    instance.app->event(instance.context, &evt);
}

void keyHandler(eventid_t id) {
  (void)id;
  uint8_t val = touch_state;
  uint32_t i;
  OrchardAppEvent evt;

  if( ui_override )
    return;
  
  if (!instance.app->event) {
    return;
  }

  /* No key changed */
  if (instance.keymask == val) {
    return;
  }

  // don't send main menu requests on to the app
  if( (val & MAIN_MENU_MASK) == MAIN_MENU_VALUE ) { 
    captouch_collected_state = 0;
    return;
  }
    
  for (i = 0; i < 8; i++) {
    uint8_t code = captouch_to_key(1 << i);

    if ((val & (1 << i)) && !(instance.keymask & (1 << i))) {
      evt.type = keyEvent;
      evt.key.code = code;
      evt.key.flags = keyDown;
      if( instance.ui == NULL )
	instance.app->event(instance.context, &evt);
      else
	instance.ui->event(instance.context, &evt);
    }
    if (!(val & (1 << i)) && (instance.keymask & (1 << i))) {
      evt.type = keyEvent;
      evt.key.code = code;
      evt.key.flags = keyUp;
      if( instance.ui == NULL )
	instance.app->event(instance.context, &evt);
      else
	instance.ui->event(instance.context, &evt);
    }
  }
  
  instance.keymask = val;

  // reset the collective state to 0
  captouch_collected_state = 0;
}

static void terminate(eventid_t id) {

  (void)id;
  OrchardAppEvent evt;

  if (!instance.app->event)
    return;

  evt.type = appEvent;
  evt.app.event = appTerminate;
  instance.app->event(instance.context, &evt);
  chThdTerminate(instance.thr);
}

static void timer_event(eventid_t id) {

  (void)id;
  OrchardAppEvent evt;

  if (!instance.app->event)
    return;

  evt.type = timerEvent;
  evt.timer.usecs = instance.timer_usecs;
  if( !ui_override )
    instance.app->event(instance.context, &evt);

  if (instance.timer_repeating)
    orchardAppTimer(instance.context, instance.timer_usecs, true);
}

static void timer_do_send_message(void *arg) {

  (void)arg;
  chSysLockFromISR();
  chEvtBroadcastI(&timer_expired);
  chSysUnlockFromISR();
}

const OrchardApp *orchardAppByName(const char *name) {
  const OrchardApp *current;

  current = orchard_app_start();
  while(current->name) {
    if( !strncmp(name, current->name, 16) ) {
      return current;
    }
    current++;
  }
  return NULL;
}

void orchardAppRun(const OrchardApp *app) {
  instance.next_app = app;
  chThdTerminate(instance.thr);
  chEvtBroadcast(&orchard_app_terminate);
}

void orchardAppExit(void) {
  instance.next_app = orchard_app_start();  // the first app is the launcher
  chThdTerminate(instance.thr);
  chEvtBroadcast(&orchard_app_terminate);
}

void orchardAppTimer(const OrchardAppContext *context,
                     uint32_t usecs,
                     bool repeating) {

  if (!usecs) {
    chVTReset(&context->instance->timer);
    context->instance->timer_usecs = 0;
    return;
  }

  context->instance->timer_usecs = usecs;
  context->instance->timer_repeating = repeating;
  chVTSet(&context->instance->timer, US2ST(usecs), timer_do_send_message, NULL);
}

void update_sd(int16_t *samples);
static void i2s_full_handler(eventid_t id) {
  (void)id;
  OrchardAppEvent evt;

  if( gen_mic_event ) {
    gen_mic_event = 0;
    
    evt.type = adcEvent;
    evt.adc.code = adcCodeMic;
    if( !ui_override )
      instance.app->event(instance.context, &evt);
  }
  
}

static void i2s_reset_handler(eventid_t id) {
  (void)id;
 
  // sd_offset = DATA_OFFSET_START;   // not yet implemented
}

static void accel_bump_event(eventid_t id) {
  (void) id;
  OrchardAppEvent evt;
  
  evt.type = accelEvent;
  evt.accel.code = accelCodeBump;
  if( !ui_override )
    instance.app->event(instance.context, &evt);
}

static THD_WORKING_AREA(waOrchardAppThread, 0x780); // was 0x900 before we expanded oscope processing, 0xb00 without fft agc mod
static THD_FUNCTION(orchard_app_thread, arg) {

  (void)arg;
  struct orchard_app_instance *instance = arg;
  struct evt_table orchard_app_events;
  OrchardAppContext app_context;

  ui_override = 0;
  memset(&app_context, 0, sizeof(app_context));
  instance->context = &app_context;
  app_context.instance = instance;
  
  // set UI elements to null
  instance->ui = NULL;
  instance->uicontext = NULL;
  instance->ui_result = 0;

  chRegSetThreadName("Orchard App");

  instance->keymask = captouchRead();

  evtTableInit(orchard_app_events, 16);
  evtTableHook(orchard_app_events, touch_event, touchHandler);
  evtTableHook(orchard_app_events, radio_app, radio_app_event);
  evtTableHook(orchard_app_events, ui_completed, ui_complete_cleanup);
  evtTableHook(orchard_app_events, orchard_app_terminate, terminate);
  evtTableHook(orchard_app_events, timer_expired, timer_event);
  evtTableHook(orchard_app_events, celcius_rdy, adc_temp_event);
  evtTableHook(orchard_app_events, i2s_full_event, i2s_full_handler);
  evtTableHook(orchard_app_events, accel_bump, accel_bump_event);
  //  evtTableHook(orchard_app_events, i2s_reset_event, i2s_reset_handler);

  if (instance->app->init)
    app_context.priv_size = instance->app->init(&app_context);
  else
    app_context.priv_size = 0;

  /* Allocate private data on the stack (word-aligned) */
  uint32_t priv_data[app_context.priv_size / 4];
  if (app_context.priv_size) {
    memset(priv_data, 0, sizeof(priv_data));
    app_context.priv = priv_data;
  }
  else
    app_context.priv = NULL;

  if (instance->app->start)
    instance->app->start(&app_context);
  if (instance->app->event) {
    {
      OrchardAppEvent evt;
      evt.type = appEvent;
      evt.app.event = appStart;
      instance->app->event(instance->context, &evt);
    }
    while (!chThdShouldTerminateX())
      chEvtDispatch(evtHandlers(orchard_app_events), chEvtWaitOne(ALL_EVENTS));
  }

  chVTReset(&instance->timer);

  if (instance->app->exit)
    instance->app->exit(&app_context);

  instance->context = NULL;

  /* Set up the next app to run when the orchard_app_terminated message is
     acted upon.*/
  if (instance->next_app)
    instance->app = instance->next_app;
  else
    instance->app = orchard_app_list;
  instance->next_app = NULL;

  chVTReset(&run_launcher_timer);
  run_launcher_timer_engaged = false;

  //  evtTableUnhook(orchard_app_events, i2s_reset_event, i2s_reset_handler);
  evtTableUnhook(orchard_app_events, accel_bump, accel_bump_event);
  evtTableUnhook(orchard_app_events, i2s_full_event, i2s_full_handler);
  evtTableUnhook(orchard_app_events, celcius_rdy, adc_temp_event);
  evtTableUnhook(orchard_app_events, timer_expired, timer_event);
  evtTableUnhook(orchard_app_events, orchard_app_terminate, terminate);
  evtTableUnhook(orchard_app_events, ui_completed, ui_complete_cleanup);
  evtTableUnhook(orchard_app_events, radio_app, radio_app_event);
  evtTableUnhook(orchard_app_events, touch_event, touchHandler);

  /* Atomically broadcasting the event source and terminating the thread,
     there is not a chSysUnlock() because the thread terminates upon return.*/
  chSysLock();
  chEvtBroadcastI(&orchard_app_terminated);
  chThdExitS(MSG_OK);
}

void orchardAppInit(void) {
  int i;

  orchard_app_list = orchard_app_start();
  instance.app = orchard_app_list;
  chEvtObjectInit(&orchard_app_terminated);
  chEvtObjectInit(&orchard_app_terminate);
  chEvtObjectInit(&timer_expired);
  chEvtObjectInit(&chargecheck_timeout);
  chEvtObjectInit(&ping_timeout);
  chEvtObjectInit(&ui_completed);
  chVTReset(&instance.timer);

  /* Hook this outside of the app-specific runloop, so it runs even if
     the app isn't listening for events.*/
  evtTableHook(orchard_events, touch_event, poke_run_launcher_timer);
  
  // usb detection and charge state management is also meta to the apps
  // sequence of events:
  // 0. timer chargecheck_timer is set for CHARGECHECK_INTERVAL
  // 1. timer chargecheck_timer times out, and run_chargecheck callback is executed
  // 2. run_chargecheck callback issues a chargecheck_timeout event and re-schedules itself
  // 3. event sytsem receives chargecheck_timeout event and  dispatches handle_chargecheck_timeout
  // 4. handle_chargecheck_timeout issues an analogUpdateUsbStatus() call and exits
  // 5. analogUpdateUsbStatus() eventually results in a usbdet_rdy event
  // 6. usbdet_rdy event dispatches into the handle_charge_state event handler
  // 7. handle_charge_state runs all the logic for managing charge state

  // Steps 0-7 create a periodic timer that polls the USB D+/D- pin state so we can
  // make a determination of how to correctly set the charger/boost state
  // It's complicated because both the timer and the D+/D- detetion are asynchronous
  // and you have to use events to poke operations that can't happen in interrupt contexts!
  evtTableHook(orchard_events, chargecheck_timeout, handle_chargecheck_timeout);

  evtTableHook(orchard_events, radio_page, handle_radio_page);
  evtTableHook(orchard_events, ping_timeout, handle_ping_timeout);
  radioSetHandler(radioDriver, radio_prot_ping, radio_ping_received);
  radioSetHandler(radioDriver, radio_prot_sex_req, handle_radio_sex_req );
  radioSetHandler(radioDriver, radio_prot_sex_ack, handle_radio_sex_ack );
  radioSetHandler(radioDriver, radio_prot_time, handleRadioTime);
  radioSetHandler(radioDriver, radio_prot_forward, handleRadioForward);
  radioSetHandler(radioDriver, radio_prot_address, handleRadioAddress);
  radioSetHandler(radioDriver, radio_prot_baton, handleRadioBaton);

  chVTReset(&chargecheck_timer);
  chVTSet(&chargecheck_timer, MS2ST(CHARGECHECK_INTERVAL), run_chargecheck, NULL);

  chVTReset(&ping_timer);
  chVTSet(&ping_timer, MS2ST(PING_MIN_INTERVAL + rand() % PING_RAND_INTERVAL), run_ping, NULL);
  chEvtBroadcast(&ping_timeout); // do a ping on startup


  for( i = 0; i < MAX_FRIENDS; i++ ) {
    friends[i] = NULL;
  }
  osalMutexObjectInit(&friend_mutex);
}

void orchardAppRestart(void) {

  /* Recovers memory of the previous application. */
  if (instance.thr) {
    osalDbgAssert(chThdTerminatedX(instance.thr), "App thread still running");
    chThdRelease(instance.thr);
    instance.thr = NULL;
  }

  instance.thr = chThdCreateStatic(waOrchardAppThread,
                                   sizeof(waOrchardAppThread),
                                   ORCHARD_APP_PRIO,
                                   orchard_app_thread,
                                   (void *)&instance);
}

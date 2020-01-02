/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "hal.h"

#include "chprintf.h"
#include "shellcfg.h"
#include "shell.h"
#include <math.h>

#include "radio.h"
#include "oled.h"
#include "gfx.h"
#include "orchard-events.h"
#include "touch.h"
#include "hal.h"
#include "orchard-effects.h"
#include "storage.h"
#include "orchard-math.h"
#include "genes.h"
#include "led.h"
#include "userconfig.h"
#include "orchard-app.h"
#include "charger.h"
#include "mic.h"
#include "paging.h"
#include "analog.h"
#include "pir.h"
#include "gyro.h"
#include "trigger.h"
#include "address.h"
#include "baton.h"

#include "orchard-test.h"

#define SPI_TIMEOUT MS2ST(3000)

static uint8_t fb[LED_COUNT * 3];
static uint8_t ui_fb[LED_COUNT * 3];

static uint8_t event_ready = 0;

struct evt_table orchard_events;

extern const char *gitversion;

void sw_irq(EXTDriver *extp, expchannel_t channel);

void gyro_irq1(EXTDriver *extp, expchannel_t channel) {
  (void)extp;
  (void)channel;
  
  chSysLockFromISR();
  chEvtBroadcastI(&gyro1_process);
  chSysUnlockFromISR();
}
void gyro_irq2(EXTDriver *extp, expchannel_t channel) {
  (void)extp;
  (void)channel;
  
  chSysLockFromISR();
  chEvtBroadcastI(&gyro2_process);
  chSysUnlockFromISR();
}

static const EXTConfig extcfg = {
  {
    {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART, radioInterrupt, PORTE, 1},
    {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART, pir_irq, PORTD, 0},
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART, sw_irq, PORTA, 4},
    {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART, gyro_irq1, PORTA, 18},
    {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART, gyro_irq2, PORTD, 3},
  }
};

static uint32_t sw_debounce;
event_source_t sw_process;
void sw_irq(EXTDriver *extp, expchannel_t channel) {
  (void)extp;
  (void)channel;
  
  chSysLockFromISR();
  chEvtBroadcastI(&sw_process);
  chSysUnlockFromISR();
}

void swStart(void) {
  chEvtObjectInit(&sw_process);
  sw_debounce = chVTGetSystemTime();
}

uint8_t test_switch = 0;
void sw_proc(eventid_t id) {

  (void)id;
  test_switch = 1; // trigger just for test functions *DO NOT USE FOR REGULAR CODE* it is not thread-safe
  if( chVTTimeElapsedSinceX(sw_debounce) > 100 ) {
    chprintf(stream, "switch change effect\n\r");
    effectsNextPattern(0);
  }
  sw_debounce = chVTGetSystemTime();
  // check for press and hold
  while( !(GPIOA->PDIR & 0x10) ) { // this checks pin 4 on port A being held down
    // for some reason the native abstractions of chibiOS are failing on this pin :-/
    //    chprintf(stream, "debug: PORTA->PCR[4] %08x, GPIOA->PDDR %08x, GPIOA->PDIR %08x \n\r", PORTA->PCR[4], GPIOA->PDDR, GPIOA->PDIR );
    if( chVTTimeElapsedSinceX(sw_debounce) > 4000 ) {
      chprintf(stream, "Shutting down after switch is released...\n\r");
      chargerShipMode();
      chprintf(stream, "Shutdown failed, this statement should be unreachable\n\r");
      sw_debounce = chVTGetSystemTime();
    }
    chThdYield();
    chThdSleepMilliseconds(10); // let other threads run
  }
  sw_debounce = chVTGetSystemTime();
}

static const ADCConfig adccfg1 = {
  /* Perform initial calibration */
  true
};

static const SPIConfig spi_config = {
  NULL,
  IOPORT4,
  4,
  KINETIS_SPI_TAR_8BIT_FAST
};


static const I2CConfig i2c_config = {
  100000
};

extern void programDumbRleFile(void);

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(1024)

//#define stream (BaseSequentialStream *)&SD4

static const SerialConfig serialConfig = {
  115200,
};

extern int print_hex(BaseSequentialStream *chp,
                     const void *block, int count, uint32_t start);

static void default_radio_handler(uint8_t type, uint8_t src, uint8_t dst,
                                  uint8_t length, const void *data) {

  chprintf(stream, "\r\nNo handler for packet found.  %02x -> %02x : %02x\r\n",
           src, dst, type);
  print_hex(stream, data, length, 0);
}

static void orchard_app_restart(eventid_t id) {
  static int i = 1;
  (void)id;

  chprintf(stream, "\r\nRunning next app (pid #%d)\r\n", ++i);
  orchardAppRestart();
}

static void freefall(eventid_t id) {

  (void)id;
  // chprintf(stream, "freefall\r\n");
  bump(5);
  chEvtBroadcast(&accel_bump);
}

static void singletapchanged(eventid_t id) {
  (void)id;
  // chprintf(stream, "singletap\r\n");
  singletap();
}

static void doubletapchanged(eventid_t id) {
  (void)id;
 // chprintf(stream, "doubletap\r\n");
  doubletap();
}

static void initRadioAddress(void) {
  if (radioAddress(radioDriver) == RADIO_DEFAULT_NODE_ADDRESS) {
    requestRadioAddress();
  }
}

void spiRuntSetup(SPIDriver *spip);
unsigned int flash_init = 0;


static thread_t *eventThr = NULL;
static THD_WORKING_AREA(waOrchardEventThread, 0x400);
// audit May 26, 2019 -- this should leave about 700 bytes margin
static THD_FUNCTION(orchard_event_thread, arg) {

  (void)arg;
  chRegSetThreadName("Events");

  swStart();
  pirStart();
  gyro_init();
  flashStart();
  orchardTestInit();
  
  addEntropy(SIM->UIDL);  // something unique to each device, but repeatable
  addEntropy(SIM->UIDML);
  addEntropy(SIM->UIDMH);
  
  geneStart();  // this has to start after random pool is initied
  configStart();
  startBaton();  // this has to start after the config records are initialized

  flash_init = 1;

  chThdSleepMilliseconds(5);
  ggOn(); // turn on the gas guage, do last to give time for supplies to stabilize
  chThdSleepMilliseconds(5);
  chgAutoParams(); // set auto charge parameters

  chEvtObjectInit(&chg_keepalive_event);
  chVTObjectInit(&chg_vt); // initialize the charger keep-alive virtual timer

  chThdSleepMilliseconds(5);
  chgStart(1);

  adcStart(&ADCD1, &adccfg1);
  analogStart();

  // reset the radio
  palSetPad(IOPORT1, 19); // set RADIO_RESET, resetting the radio
  chThdSleepMilliseconds(1);
  palClearPad(IOPORT1, 19); // clear RADIO_RESET, taking radio out of reset
  chThdSleepMilliseconds(5);

  uiStart();

  spiObjectInit(&SPID1);

  // setup drive strengths on SPI1
  PORTD_PCR4 = 0x103; // pull up enabled, fast slew  (CS0)
  PORTD_PCR5 = 0x703; // pull up enabled, fast slew (clk)
  PORTD_PCR6 = 0x700; // fast slew (mosi)
  PORTD_PCR7 = 0x707; // slow slew, pull-up (miso)
  
  evtTableInit(orchard_events, 16);
  orchardEventsStart();
  orchardAppInit();

  spiStart(&SPID2, &spi_config);
  spiRuntSetup(&SPID2);
  radioStart(radioDriver, &SPID2);
  radioSetDefaultHandler(radioDriver, default_radio_handler);
  pagingStart();

  micStart(); 
  //i2sStartRx(&I2SD1); // start the audio sampling buffer

  event_ready = 1; // inform that the event thread is initialized
  /*
   * Activates the EXT driver 1.
   */
  extInit();
  extObjectInit(&EXTD1);
  extStart(&EXTD1, &extcfg);

  initRadioAddress();

  evtTableHook(orchard_events, chg_keepalive_event, chgKeepaliveHandler);
  evtTableHook(orchard_events, orchard_app_terminated, orchard_app_restart);
  evtTableHook(orchard_events, gyro_freefall, freefall);
  evtTableHook(orchard_events, gyro_singletap, singletapchanged);
  evtTableHook(orchard_events, gyro_singletap, test_singletap);
  evtTableHook(orchard_events, gyro_doubletap, doubletapchanged); 
  evtTableHook(orchard_events, pir_process, pir_proc);
  evtTableHook(orchard_events, sw_process, sw_proc);
  evtTableHook(orchard_events, gyro1_process, gyro1_proc);
  evtTableHook(orchard_events, gyro2_process, gyro2_proc);
  
  orchardAppRestart();
  
  while (!chThdShouldTerminateX())
    chEvtDispatch(evtHandlers(orchard_events), chEvtWaitOne(ALL_EVENTS));

  evtTableUnhook(orchard_events, gyro2_process, gyro2_proc);
  evtTableUnhook(orchard_events, gyro1_process, gyro1_proc);
  evtTableUnhook(orchard_events, sw_process, sw_proc);
  evtTableUnhook(orchard_events, pir_process, pir_proc);
  evtTableUnhook(orchard_events, gyro_doubletap, doubletapchanged); 
  evtTableUnhook(orchard_events, gyro_singletap, test_singletap);
  evtTableUnhook(orchard_events, gyro_singletap, singletapchanged);
  evtTableUnhook(orchard_events, gyro_freefall, freefall);
  evtTableUnhook(orchard_events, orchard_app_terminated, orchard_app_restart);
  evtTableUnhook(orchard_events, chg_keepalive_event, chgKeepaliveHandler);

  chSysLock();
  chThdExitS(MSG_OK);
}

static thread_t *gyroThr = NULL;
static THD_WORKING_AREA(waGyroThread, 0x200);
// gyro thread size audit May 28, 2019; about 300 extra bytes available
static THD_FUNCTION(gyro_thread, arg) {

  (void)arg;
  int loops = 0;
  
  chRegSetThreadName("Gyro");
  
  // init the gyro
  gyro_init();
  
  while (!chThdShouldTerminateX()) {
    if( (loops % 10) == 0 ) { // gyro doesn't have to update super often
      // check z axis
      struct accel_data data;
      gyro_Get_X_Axes(&data);
      double norm_Of_g = sqrt(data.x * data.x + data.y * data.y + data.z * data.z);
      float znorm = data.z/norm_Of_g;
      z_inclination = (int) (acos(znorm)*(180/3.14159));
      //measure pitch to determine which side is facing
      pitch_angle = (int) (atan(data.y/(sqrt(data.x*data.x + data.z*data.z))) * (180/3.14159));   
      if(pitch_angle > -45 && pitch_angle < 45 && data.x > 0){
        current_side = 0;
      } else if (pitch_angle > -45 && pitch_angle < 45 && data.x < 0){
        current_side = 180;
      } else if (pitch_angle >= 45){
        current_side = 90;
      } else if (pitch_angle <= -45){
        current_side = 270; 
      }
    }
    loops++;
    chThdSleepMilliseconds(10); // give some time for other threads
  }
  chSysLock();
  chThdExitS(MSG_OK);
}


void spiRuntSetup(SPIDriver *spip);

/*
 * Application entry point.
 */
int main(void) {
  
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();

  // K22 has a separate setting for open-drain that has to be explicitly set for I2C
  // this breaks the chibiOS abstractions, which were not made to anticipate this oddity.
  PORT_PCR_REG(PORTB,2) |= PORTx_PCRn_ODE;
  PORT_PCR_REG(PORTB,3) |= PORTx_PCRn_ODE;
  
  PORT_PCR_REG(PORTC,10) |= PORTx_PCRn_ODE;
  PORT_PCR_REG(PORTC,11) |= PORTx_PCRn_ODE;

  chSysInit();

  // IOPORT1 = PORTA, IOPORT2 = PORTB, etc...
  palSetPad(IOPORT2, 1); // set shipmode_n to 1 (disable shipmode)
  palClearPad(IOPORT3, 8); // enable charging by lowering CD pin
  
  palClearPad(IOPORT5, 0); // turn on red LED
  
  sdStart(&SD4, &serialConfig);
  // not to self -- baud rates on other UARTs is kinda hard f'd up due to some XZ hacks to hit 3.125mbps
  
  i2cObjectInit(&I2CD1);
  i2cStart(&I2CD1, &i2c_config);

  chgSetSafety(); // has to be first thing written to the battery controller
  
  shellInit();

  chprintf(stream, SHELL_NEWLINE_STR SHELL_NEWLINE_STR);
  chprintf(stream, "Cubegarden bootloader.  Based on build %s"SHELL_NEWLINE_STR,
	   gitversion);
  chprintf(stream, "Core free memory : %d bytes"SHELL_NEWLINE_STR,
	   chCoreGetStatusX());

  flash_init = 0;

  chgAutoParams();
  
  // this hooks all the events, so start it only after all events are initialized
  eventThr = chThdCreateStatic(waOrchardEventThread,
			       sizeof(waOrchardEventThread),
			       (NORMALPRIO + 16),
			       orchard_event_thread,
			       NULL);

  while(flash_init == 0) // wait until the flash inits from the thread that was spawned
    chThdSleepMilliseconds(10);
    
  if( *((unsigned char *)0x40020003) != (unsigned char) 0xf9 ) {
    chprintf(stream, "WARNING: FOPT not set correctly, check factory options!\n\r");
    chprintf(stream, "FOPT: %02x\n\r", *((unsigned char *)0x40020003));
    chprintf(stream, "FSEC: %02x\n\r", *((unsigned char *)0x40020002));
  } else {
    chprintf(stream, "FOPT check OK.\n\r");
  }

  if(  WDOG->STCTRLH != 0x101 )
    chprintf(stream, "*** WARNING: watchdog disabled. No hard fault recovery. ***\n\r" );
  
  PORTA->PCR[4] |= 0x10; // turn on passive filter for the switch pin
  
  palSetPadMode(IOPORT1, 12, PAL_MODE_OUTPUT_PUSHPULL); // weird, why do i have to have this line???
  palSetPad(IOPORT3, 2); // power on +5V
  ledStart(LED_COUNT, fb, UI_LED_COUNT, ui_fb);

  palSetPad(IOPORT5, 0); // turn off red LED
  
  chprintf(stream, "User flash start: 0x%x  user flash end: 0x%x  length: 0x%x\r\n",
      __storage_start__, __storage_end__, __storage_size__);

  // start the gyro monitoring thread
  gyroThr = chThdCreateStatic(waGyroThread,
			      sizeof(waGyroThread),
			      (NORMALPRIO - 6),
			      gyro_thread,
			      NULL);

  
  uint32_t init_delay = chVTGetSystemTime();
  while( !event_ready ) {
    chThdYield();
    chThdSleepMilliseconds(10);
    if( chVTTimeElapsedSinceX(init_delay) > 4000 ) {
      chprintf(stream, "Subsystem initialization timeout! event: %d\n\r",
	       event_ready);
      break;
    }
  }
  orchardTestRunAll(stream, orchardTestPoweron);

  // start effects only after tests have been run
  effectsStart();

  /*
   * Normal main() thread activity, spawning shells.
   */
  while (true) {
    thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
					    "shell", NORMALPRIO + 1,
					    shellThread, (void *)&shell_cfg);
    chThdWait(shelltp);               /* Waiting termination.             */
    chThdSleepMilliseconds(1000);
  }
}

static OrchardTestResult test_cpu(const char *my_name, OrchardTestType test_type) {
  (void) my_name;
  
  switch(test_type) {
  case orchardTestPoweron:
  case orchardTestTrivial:
  case orchardTestInteractive:
    if( SIM->SDID != 0x22000695 ) // just check the CPUID is correct
      return orchardResultFail;
    else
      return orchardResultPass;
    break;
  case orchardTestComprehensive:
    // no test for this yet
  default:
    return orchardResultNoTest;
  }
  
  return orchardResultNoTest;
}
orchard_test("cpu", test_cpu);

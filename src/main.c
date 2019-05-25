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
#include "barometer.h"
#include "gyro.h"

#include "orchard-test.h"

#define SPI_TIMEOUT MS2ST(3000)

static uint8_t fb[LED_COUNT * 3];
static uint8_t ui_fb[LED_COUNT * 3];

static uint8_t baro_ready = 0;
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
  chprintf(stream, "freefall\r\n");
  bump(5);
  chEvtBroadcast(&accel_bump);
}

static void singletapchanged(eventid_t id) {
  (void)id;
  chprintf(stream, "singletap\r\n");
  singletap();
}

void spiRuntSetup(SPIDriver *spip);
unsigned int flash_init = 0;


static thread_t *eventThr = NULL;
static THD_WORKING_AREA(waOrchardEventThread, 0x800);
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

  flash_init = 1;

  chgSetSafety(); // has to be first thing written to the battery controller
  
  ggOn(); // turn on the gas guage, do last to give time for supplies to stabilize
  chgAutoParams(); // set auto charge parameters

  chVTObjectInit(&chg_vt); // initialize the charger keep-alive virtual timer
  chEvtObjectInit(&chg_keepalive_event);

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
  
  evtTableInit(orchard_events, 12);
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

  // wait for barometer to become ready before starting apps
  uint32_t init_delay = chVTGetSystemTime();
  while( baro_ready == 0 ) {
    chThdYield();
    chThdSleepMilliseconds(10);
    if( chVTTimeElapsedSinceX(init_delay) > 4000 ) {
      chprintf(stream, "Subsystem initialization timeout! baro: %d\n\r", baro_ready);
      break;
    }
  }
  
  evtTableHook(orchard_events, chg_keepalive_event, chgKeepaliveHandler);
  evtTableHook(orchard_events, orchard_app_terminated, orchard_app_restart);
  evtTableHook(orchard_events, gyro_freefall, freefall);
  evtTableHook(orchard_events, gyro_singletap, singletapchanged);
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
  evtTableUnhook(orchard_events, gyro_singletap, singletapchanged);
  evtTableUnhook(orchard_events, gyro_freefall, freefall);
  evtTableUnhook(orchard_events, orchard_app_terminated, orchard_app_restart);
  evtTableUnhook(orchard_events, chg_keepalive_event, chgKeepaliveHandler);

  chSysLock();
  chThdExitS(MSG_OK);
}

float baro_pressure = -1.0;
float baro_temp = -1.0;
float baro_avg = -1.0;
uint8_t baro_avg_valid = 0;

static thread_t *baroThr = NULL;
static THD_WORKING_AREA(waBaroThread, 0x200);
static THD_FUNCTION(baro_thread, arg) {

  (void)arg;
  int16_t oversampling = 7;
  float baro_history[BARO_HISTORY];
  float acc;
  int index = 0;
  int i;
  int loops = 0;
  
  chRegSetThreadName("Barometer");
  chThdSleepMilliseconds(250); // wait for other subsystems to finish booting

  for( i = 0; i < BARO_HISTORY; i++ ) {
    baro_history[i] = 10000.0;
  }
  
  // init the barometer
  baro_init();
  float temperature;
  float pressure;
  // do a dummy read to setup barometer internal state
  baro_measureTempOnce(&temperature, 7);
  baro_measurePressureOnce(&pressure, 7);
  
  baro_ready = 1; // signal to the test subsystem that the barometer init is done
  
  while (!chThdShouldTerminateX()) {
    if( (loops % 30) == 0 ) { // temperature doesn't need to update as fast as barometer
      baro_measureTempOnce(&baro_temp, oversampling);
    }
    loops++;
    baro_measurePressureOnce(&baro_pressure, oversampling);
    baro_history[index] = baro_pressure;

    if(baro_avg_valid == 1 && abs(baro_pressure-baro_avg) > BARO_CHANGE_SENSITIVITY){
      // chprintf(stream, "pressure changed suddenly!\r\n");
      pressureChanged();
    }

    index = (index + 1) % BARO_HISTORY;
    if( index == (BARO_HISTORY - 1) )
      baro_avg_valid = 1;
    
    for( acc = 0, i = 0; i < BARO_HISTORY; i++ ) {
      acc += baro_history[i];
    }
    baro_avg = acc / (float) BARO_HISTORY;
    
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

  chprintf(stream, "i2c check\n\r" );
  chgAutoParams();
  chprintf(stream, "done\n\r" );

  shellInit();

  chprintf(stream, SHELL_NEWLINE_STR SHELL_NEWLINE_STR);
  chprintf(stream, "Cubegarden bootloader.  Based on build %s"SHELL_NEWLINE_STR,
	   gitversion);
  chprintf(stream, "Core free memory : %d bytes"SHELL_NEWLINE_STR,
	   chCoreGetStatusX());

  flash_init = 0;
  
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
  
  PORTA->PCR[4] |= 0x10; // turn on passive filter for the switch pin
  
  palSetPadMode(IOPORT1, 12, PAL_MODE_OUTPUT_PUSHPULL); // weird, why do i have to have this line???
  palSetPad(IOPORT3, 2); // power on +5V
  ledStart(LED_COUNT, fb, UI_LED_COUNT, ui_fb);

  palSetPad(IOPORT5, 0); // turn off red LED
  
  chprintf(stream, "User flash start: 0x%x  user flash end: 0x%x  length: 0x%x\r\n",
      __storage_start__, __storage_end__, __storage_size__);

  // start the barometer monitoring thread
  baroThr = chThdCreateStatic(waBaroThread,
			      sizeof(waBaroThread),
			      (NORMALPRIO - 6),
			      baro_thread,
			      NULL);

  
  uint32_t init_delay = chVTGetSystemTime();
  while( !event_ready && !baro_ready ) {
    chThdYield();
    chThdSleepMilliseconds(10);
    if( chVTTimeElapsedSinceX(init_delay) > 4000 ) {
      chprintf(stream, "Subsystem initialization timeout! event: %d baro: %d\n\r",
	       event_ready, baro_ready);
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

OrchardTestResult test_cpu(const char *my_name, OrchardTestType test_type) {
  (void) my_name;
  
  switch(test_type) {
  case orchardTestPoweron:
  case orchardTestTrivial:
    if( SIM->SDID != 0x22000695 ) // just check the CPUID is correct
      return orchardResultFail;
    else
      return orchardResultPass;
    break;
  default:
    return orchardResultNoTest;
  }
  
  return orchardResultNoTest;
}
orchard_test("cpu", test_cpu);

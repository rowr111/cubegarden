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
#include "accel.h"
#include "mic.h"
#include "paging.h"
#include "analog.h"
#include "pir.h"
#include "barometer.h"

#include "orchard-test.h"

#define SPI_TIMEOUT MS2ST(3000)

#define LED_COUNT 32
#define UI_LED_COUNT 32
static uint8_t fb[LED_COUNT * 3];
static uint8_t ui_fb[LED_COUNT * 3];

struct evt_table orchard_events;

extern const char *gitversion;

static const EXTConfig extcfg = {
  {
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART, accel_irq, PORTC, 1},
    {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART, radioInterrupt, PORTE, 1},
    {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART, pir_irq, PORTD, 0},
  }
};

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


static const SPIConfig spi_config_mmc = {
  NULL,
  GPIOD,
  0, 
  KINETIS_SPI_TAR_8BIT_SD
  //KINETIS_SPI_TAR_8BIT_MED
};

static const SPIConfig spi_config_mmc_ls = {
  NULL,
  GPIOD,
  0, 
  KINETIS_SPI_TAR_8BIT_SLOW
};

static const MMCConfig mmc_config = { 
  &SPID1,
  &spi_config_mmc_ls, // low speed config for init
  &spi_config_mmc // high speed config for run
};

MMCDriver MMCD1;


static const I2CConfig i2c_config = {
  100000
};
static const I2CConfig i2c2_config = {
  400000
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
  chprintf(stream, "A");
  bump(5);
  chEvtBroadcast(&accel_bump);
}

void spiRuntSetup(SPIDriver *spip);
unsigned int flash_init = 0;


static thread_t *eventThr = NULL;
static THD_WORKING_AREA(waOrchardEventThread, 0x800);
static THD_FUNCTION(orchard_event_thread, arg) {

  (void)arg;
  chRegSetThreadName("Events");

  pirStart();
  accelStart(&I2CD1);
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
  mmcObjectInit(&MMCD1);

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

  orchardTestRunAll(stream, orchardTestPoweron);
  /*
   * Activates the EXT driver 1.
   */
  extInit();
  extObjectInit(&EXTD1);
  extStart(&EXTD1, &extcfg);
  
  evtTableHook(orchard_events, chg_keepalive_event, chgKeepaliveHandler);
  evtTableHook(orchard_events, orchard_app_terminated, orchard_app_restart);
  evtTableHook(orchard_events, accel_process, accel_proc);
  evtTableHook(orchard_events, accel_freefall, freefall);
  evtTableHook(orchard_events, pir_process, pir_proc);
  
  orchardAppRestart();
  
  while (!chThdShouldTerminateX())
    chEvtDispatch(evtHandlers(orchard_events), chEvtWaitOne(ALL_EVENTS));

  evtTableUnhook(orchard_events, pir_process, pir_proc);
  evtTableUnhook(orchard_events, accel_freefall, freefall);
  evtTableUnhook(orchard_events, accel_process, accel_proc);
  evtTableUnhook(orchard_events, orchard_app_terminated, orchard_app_restart);
  evtTableUnhook(orchard_events, chg_keepalive_event, chgKeepaliveHandler);

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

  // some PCR overrides for fast SD cards ops to work more reliably
  PORTD_PCR0 = 0x203; // pull up enabled, fast slew  (CS0)
  PORTD_PCR1 = 0x203; // pull up enabled, fast slew (clk)
  PORTD_PCR2 = 0x200; // fast slew (mosi)
  PORTD_PCR3 = 0x200; // fast slew (miso)
  
  // K22 has a separate setting for open-drain that has to be explicitly set for I2C
  // this breaks the chibiOS abstractions, which were not made to anticipate this oddity.
  PORT_PCR_REG(PORTB,2) |= PORTx_PCRn_ODE;
  PORT_PCR_REG(PORTB,3) |= PORTx_PCRn_ODE;
  
  PORT_PCR_REG(PORTC,10) |= PORTx_PCRn_ODE;
  PORT_PCR_REG(PORTC,11) |= PORTx_PCRn_ODE;

  chSysInit();

  // IOPORT1 = PORTA, IOPORT2 = PORTB, etc...
  palSetPad(IOPORT2, 1); // set shipmode_n to 1 (disable shipmode)
  palSetPad(IOPORT3, 8); // set BATT_SRST
  
  palClearPad(IOPORT5, 0); // turn on red LED
  
  sdStart(&SD4, &serialConfig);
  // not to self -- baud rates on other UARTs is kinda hard f'd up due to some XZ hacks to hit 3.125mbps
  
  i2cObjectInit(&I2CD1);
  i2cStart(&I2CD1, &i2c_config);

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
			       (NORMALPRIO - 1),
			       orchard_event_thread,
			       NULL);

  while(flash_init == 0) // wait until the flash inits from the thread that was spawned
    chThdSleepMilliseconds(10);
    
  palSetPadMode(IOPORT1, 12, PAL_MODE_OUTPUT_PUSHPULL); // weird, why do i have to have this line???
  palSetPad(IOPORT3, 2); // power on +5V
  ledStart(LED_COUNT, fb, UI_LED_COUNT, ui_fb);
  effectsStart();

  chThdSleepMilliseconds(200);
  palSetPad(IOPORT5, 0); // turn off red LED
  
  chprintf(stream, "User flash start: 0x%x  user flash end: 0x%x  length: 0x%x\r\n",
      __storage_start__, __storage_end__, __storage_size__);

  // init the barometer
  baro_init();
  
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

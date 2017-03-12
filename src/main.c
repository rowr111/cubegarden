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

#include "oled.h"
#include "gfx.h"
#include "orchard-events.h"
#include "ui.h"
#include "touch.h"
#include "sermon.h"
#include "gps.h"
#include "spmi.h"
#include "fpga_config.h"

#define SPI_TIMEOUT MS2ST(3000)

struct evt_table orchard_events;

extern const char *gitversion;

static const EXTConfig extcfg = {
  {
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART, gpsCb, PORTA, 13},
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART, touchCb, PORTB, 0},
    {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART, spmiCb, PORTD, 7},
  }
};

static const SPIConfig spi_config = {
  NULL,
  IOPORT4,
  4,
  KINETIS_SPI_TAR_8BIT_FAST
};

// roughly 1MHz 
#define KINETIS_SPI_TAR_SYSCLK_DIV_64(n) \
    SPIx_CTARn_FMSZ(((n) - 1)) | \
    SPIx_CTARn_PBR(0) | \
    SPIx_CTARn_BR(0x5) | \
    SPIx_CTARn_CSSCK(0x5) | \
    SPIx_CTARn_ASC(0x7) | \
    SPIx_CTARn_DT(0x7)

static const SPIConfig spi2_config = {
  NULL,
  IOPORT3,
  2,
  KINETIS_SPI_TAR_SYSCLK_DIV_64(16) // 16-bit frame size
};

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

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

#define stream (BaseSequentialStream *)&SD4

static const SerialConfig serialConfig = {
  115200,
};


extern virtual_timer_t chg_vt;
extern event_source_t chg_keepalive_event;
void chgKeepaliveHandler(eventid_t id);

extern virtual_timer_t ui_vt;
extern event_source_t ui_timer_event;

static thread_t *eventThr = NULL;
static THD_WORKING_AREA(waOrchardEventThread, 0x900);
static THD_FUNCTION(orchard_event_thread, arg) {

  (void)arg;
  chRegSetThreadName("Orchard Event Threads");

  evtTableInit(orchard_events, 32);
  orchardEventsStart();
  
  evtTableHook(orchard_events, chg_keepalive_event, chgKeepaliveHandler);
  evtTableHook(orchard_events, ui_timer_event, uiHandler);
  evtTableHook(orchard_events, touch_event, touchHandler);
  evtTableHook(orchard_events, gps_event, gpsHandler);
  evtTableHook(orchard_events, spmi_event, spmiHandler);

  while (!chThdShouldTerminateX())
    chEvtDispatch(evtHandlers(orchard_events), chEvtWaitOne(ALL_EVENTS));

  evtTableUnhook(orchard_events, spmi_event, spmiHandler);
  evtTableUnhook(orchard_events, gps_event, gpsHandler);
  evtTableUnhook(orchard_events, touch_event, touchHandler);
  evtTableUnhook(orchard_events, ui_timer_event, uiHandler);
  evtTableUnhook(orchard_events, chg_keepalive_event, chgKeepaliveHandler);

  chSysLock();
  chThdExitS(MSG_OK);
}

void chgSetSafety(void);
void chgAutoParams(void);
void chgStart(int force);
void ggOn(void);
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
  palSetPad(IOPORT3, 8); // set BATT_SRST

  sdStart(&SD4, &serialConfig);
  
  i2cObjectInit(&I2CD1);
  i2cStart(&I2CD1, &i2c_config);
  chgSetSafety(); // has to be first thing written to the battery controller

  spiStart(&SPID2, &spi_config);
  spiRuntSetup(&SPID2);  // setup our custom runt driver for the fifo-less SPI1 interface
  
  spiUnselect(&SPID2);

  spiStart(&SPID1, &spi2_config);

  shellInit();

  chprintf(stream, SHELL_NEWLINE_STR SHELL_NEWLINE_STR);
  chprintf(stream, "XZ bootloader.  Based on build %s"SHELL_NEWLINE_STR,
	   gitversion);
  chprintf(stream, "Core free memory : %d bytes"SHELL_NEWLINE_STR,
	   chCoreGetStatusX());

  chVTObjectInit(&chg_vt); // initialize the charger keep-alive virtual timer

  chEvtObjectInit(&chg_keepalive_event);
  
  // init I2C objects for graphics
  i2cObjectInit(&I2CD2);
  i2cStart(&I2CD2, &i2c2_config);

  oledStart();
  gfxInit();

  oledBanner();

  palClearPad(IOPORT1, 19); // clear SIM_DET_SYNTH, simulates SIM "in"
  palClearPad(IOPORT2, 19); // clear WLAN_RESET
  palClearPad(IOPORT3, 9); // clear SIM_SEL, selecting sim1

  ggOn(); // turn on the gas guage, do last to give time for supplies to stabilize
  chgAutoParams(); // set auto charge parameters
  chgStart(1);

  touchStart();

  /*
   * Activates the EXT driver 1.
   */
  extInit();
  extObjectInit(&EXTD1);
  extStart(&EXTD1, &extcfg);
  
  uiStart();

  serMonStart();  // serial bus monitoring subsystem
  gpsStart();  // start GPS monitoring
  spmiStart();

  // this hooks all the events, so start it only after all events are initialized
  eventThr = chThdCreateStatic(waOrchardEventThread,
			       sizeof(waOrchardEventThread),
			       (NORMALPRIO - 1),
			       orchard_event_thread,
			       NULL);

  chThdSleepMilliseconds(250);
  fpgaReconfig(); // fire up the FPGA as part of boot
  
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


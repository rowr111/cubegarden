#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#define STC3115_ADDR 0x70
#define FAN5421_ADDR 0x6A
#define LC709203_ADDR 0x0B

#define FAN5421_CTL0_ADR 0
#define FAN5421_CTL1_ADR 1
#define FAN5421_OREG_ADR 2
#define FAN5421_INFO_ADR 3
#define FAN5421_IBAT_ADR 4
#define FAN5421_SPCHG_ADR 5
#define FAN5421_SAFE_ADR 6

#define NL SHELL_NEWLINE_STR

virtual_timer_t chg_vt;
event_source_t chg_keepalive_event;

void ggOn(void);

void chg_keepalive_handler(eventid_t id) {
  (void) id;
  uint8_t tx[2];

  tx[0] = FAN5421_CTL0_ADR;
  tx[1] = 0xC0; // 32sec timer reset, enable stat pin

  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 2, NULL, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);

  ggOn(); // make sure the gas gauge is powered on so it can capture charging
  //  chprintf((BaseSequentialStream *)&SD4, "ping"NL);
}

void chgSetSafety(void) {
  uint8_t tx[2];

  tx[0] = FAN5421_SAFE_ADR;
  tx[1] = 0x60; // 1150mA, 4.2V
  i2cAcquireBus(&I2CD1);
  i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 2, NULL, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
}


static void chg_cb(void *arg) {
  (void) arg;
  chSysLockFromISR();
  chEvtBroadcastI(&chg_keepalive_event);
  chVTSetI(&chg_vt, MS2ST(1000), chg_cb, NULL);
  chSysUnlockFromISR();
}

void chgCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  uint8_t tx[2], rx[1];
  msg_t retval;
  uint32_t temp;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: chg [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    stat      Charger status"SHELL_NEWLINE_STR);
    chprintf(chp, "    id        Charger chip ID"SHELL_NEWLINE_STR);
    chprintf(chp, "    start     Enable charging & keep-alive timer"SHELL_NEWLINE_STR);
    chprintf(chp, "    stop      Disable charging & keep-alive timer"SHELL_NEWLINE_STR);
    chprintf(chp, "    auto      Set sane defaults (doesn't start keep-alive)"SHELL_NEWLINE_STR);
    chprintf(chp, "    dump      Raw dump of registers"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "id")) {
    tx[0] = FAN5421_INFO_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    chprintf(chp, "ID code: %02x"NL, rx[0]);
  }

  else if (!strcasecmp(argv[0], "start")) {
    tx[0] = FAN5421_CTL1_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    tx[1] = rx[0] & 0xFB; // bit 2 low to start charging
    tx[0] = FAN5421_CTL1_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 2, rx, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    chVTSet(&chg_vt, MS2ST(1000), chg_cb, NULL);
  }
  
  else if (!strcasecmp(argv[0], "stop")) {
    tx[0] = FAN5421_CTL1_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    tx[1] = rx[0] | 0x04; // bit 2 high to stop charging
    tx[0] = FAN5421_CTL1_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 2, rx, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    chVTReset(&chg_vt);
  }

  else if (!strcasecmp(argv[0], "auto")) {
    // now set current targets
    tx[0] = FAN5421_IBAT_ADR;
    tx[1] = 0x6 << 3 | 0x2; // 1150mA, termination at 146mA (~C/10)
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 2, rx, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    
    tx[0] = FAN5421_SPCHG_ADR;
    tx[1] = 0x00; // use IOCHARGE to set target current
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 2, rx, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    
    // target "float" voltage
    tx[0] = FAN5421_OREG_ADR;
    tx[1] = (0x22 << 2); // target 4.18 float voltage
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 2, rx, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    
    // initiate charging
    tx[0] = FAN5421_CTL1_ADR;
    tx[1] = 0x18; // weak battery: 3.5V; charge current termination; charger enabled
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 2, rx, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    
  }
  
  else if (!strcasecmp(argv[0], "stat")) {
    tx[0] = FAN5421_CTL0_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    
    chprintf(chp, "Status: ");
    switch( (rx[0] >> 4) & 0x3 ) {
    case 0:
      chprintf(chp, "Ready"NL);
      break;
    case 1:
      chprintf(chp, "Charge in progress"NL);
      break;
    case 2:
      chprintf(chp, "Charge done"NL);
      break;
    case 3:
      chprintf(chp, "Fault ");
      switch( rx[0] & 0x7 ) {
      case 0:
	chprintf(chp, "(unreachable)"NL);
	break;
      case 1:
	chprintf(chp, "Vbus OVP"NL);
	break;
      case 2:
	chprintf(chp, "Sleep mode"NL);
	break;
      case 3:
	chprintf(chp, "Poor input source"NL);
	break;
      case 4:
	chprintf(chp, "Battery OVP"NL);
	break;
      case 5:
	chprintf(chp, "Thermal shutdown"NL);
	break;
      case 6:
	chprintf(chp, "Timer fault"NL);
	break;
      case 7:
	chprintf(chp, "No battery"NL);
	break;
      }
      break;
    }
    
    chprintf(chp, "STAT ");
    rx[0] >> 6 ? chprintf( chp, "enabled"NL ) : chprintf( chp, "disabled"NL );
    
    tx[0] = FAN5421_CTL1_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    temp = ((rx[0] >> 4) & 0x3) + 4;
    chprintf(chp, "Weak battery threshold: 3.%dV"NL, temp);

    chprintf(chp, "Charge current termination ");
    (rx[0] >> 3) & 0x1 ? chprintf( chp, "enabled"NL ) : chprintf( chp, "disabled"NL );

    chprintf(chp, "Charger ");
    (rx[0] >> 2) & 0x1 ? chprintf( chp, "disabled"NL ) : chprintf( chp, "enabled"NL );

    chprintf(chp, "High impedance mode ");
    (rx[0] >> 1) & 0x1 ? chprintf( chp, "enabled"NL ) : chprintf( chp, "disabled"NL );

    tx[0] = FAN5421_OREG_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    chprintf(chp, "Float voltage ");
    temp = 3500 + ((rx[0] >> 2) * 20);
    chprintf(chp, "%dmV"NL, temp);

    tx[0] = FAN5421_SPCHG_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    chprintf(chp, "Disable pin: ");
    (rx[0] >> 3) & 0x1 ? chprintf( chp, "high, charging disabled"NL ) : chprintf( chp, "low, charging controllled by I2C"NL );
    
    chprintf(chp, "Weak charger: ");
    (rx[0] >> 4) & 0x1 ? chprintf( chp, "detected"NL ) : chprintf( chp, "not detected"NL );

    temp = (rx[0] & 0x7) * 80 + 4200;
    chprintf(chp, "Weak charger keep-above voltage: %dmV"NL, temp);
    
    chprintf(chp, "Charge current programming: ");
    (rx[0] >> 5) & 0x1 ? chprintf( chp, "fixed at 325mA"NL ) : chprintf( chp, "per IOCHARGE below"NL );

    
    tx[0] = FAN5421_IBAT_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    temp = 550 + ((rx[0] >> 3) & 0xF) * 100;
    chprintf(chp, "IOCHARGE programmable charge current limit: %dmA"NL, temp);

    temp = ((rx[0] & 0x7) * 49) + 49;
    chprintf(chp, "Termination current limit: %dmA"NL, temp);
    
    tx[0] = FAN5421_SAFE_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    temp = 550 + ((rx[0] >> 4) & 0xF) * 100;
    chprintf(chp, "Max IOCHARGE safety limit: %dmA"NL, temp);
    
    temp = 4200 + (rx[0] & 0xF) * 20;
    chprintf(chp, "Max Vbat safety limit: %dmV"NL, temp);
  }
  
  else if (!strcasecmp(argv[0], "dump")) {
    int i;
    for( i = 0; i < 7; i++ ) {
      tx[0] = i;
      i2cAcquireBus(&I2CD1);
      retval = i2cMasterTransmitTimeout(&I2CD1, FAN5421_ADDR, tx, 1, rx, 1, TIME_INFINITE);
      i2cReleaseBus(&I2CD1);
      if( retval != MSG_OK ) {
	chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
      }
      chprintf(chp, "%02x ", rx[0]);
    }
    chprintf(chp, NL);
  }

  else {
    chprintf(chp, "Unrecognized command: %s"NL, argv[0]);
  }
  
}


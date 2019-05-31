#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "orchard-ui.h"
#include "i2c.h"
#include "charger.h"
#include "shellcfg.h"

#define NL SHELL_NEWLINE_STR

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
    chprintf(chp, "    down      Power off system"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "id")) {
    tx[0] = BQ24157_ID_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    chprintf(chp, "ID code: %02x"NL, rx[0]);
  }

  else if (!strcasecmp(argv[0], "start")) {
    chgStart(1);
    chVTSet(&chg_vt, MS2ST(1000), chg_cb, NULL);
  }
  
  else if (!strcasecmp(argv[0], "down")) {
    chargerShipMode();
  }
  
  else if (!strcasecmp(argv[0], "stop")) {
    tx[0] = BQ24157_CTRL_ADR;
    i2cAcquireBus(&I2CD1);
    i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    
    tx[1] = rx[0] | 0x04; // bit 2 high to stop charging
    tx[0] = BQ24157_CTRL_ADR;
    i2cAcquireBus(&I2CD1);
    i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 2, rx, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);

    palSetPad(IOPORT3, 8); // disable charging pin
    
    chVTReset(&chg_vt);
  }

  else if (!strcasecmp(argv[0], "auto")) {
    chgAutoParams();
    chgStart(1);
  }
  
  else if (!strcasecmp(argv[0], "stat")) {
    chprintf(chp, "Status: %s\n\r", chgStat());
    chprintf(chp, "Fault: %s\n\r", chgFault());
    
    tx[0] = BQ24157_CTRL_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    temp = ((rx[0] >> 6) & 0x3);
    chprintf(chp, "USB current limit: " );
    switch(temp) {
    case 0:
      chprintf(chp, "100mA" );
      break;
    case 1:
      chprintf(chp, "500mA" );
      break;
    case 2:
      chprintf(chp, "800mA" );
      break;
    case 3:
      chprintf(chp, "no limit" );
      break;
    }
    chprintf(chp, "\n\r" );
    
    temp = ((rx[0] >> 4) & 0x3) + 4;
    chprintf(chp, "Weak battery threshold: 3.%dV"NL, temp);

    chprintf(chp, "Charge current termination ");
    (rx[0] >> 3) & 0x1 ? chprintf( chp, "enabled"NL ) : chprintf( chp, "disabled"NL );

    chprintf(chp, "Charger ");
    (rx[0] >> 2) & 0x1 ? chprintf( chp, "disabled"NL ) : chprintf( chp, "enabled"NL );

    chprintf(chp, "High impedance mode ");
    (rx[0] >> 1) & 0x1 ? chprintf( chp, "enabled"NL ) : chprintf( chp, "disabled"NL );

    chprintf(chp, "Boost mode ");
    (rx[0] >> 0) & 0x1 ? chprintf( chp, "enabled"NL ) : chprintf( chp, "disabled"NL );

    tx[0] = BQ24157_BATV_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    chprintf(chp, "Battery regulation voltage ");
    temp = 3500 + ((rx[0] >> 2) * 20);
    chprintf(chp, "%dmV"NL, temp);

    tx[0] = BQ24157_SPCHG_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    chprintf(chp, "Disable pin: ");
    (rx[0] >> 3) & 0x1 ? chprintf( chp, "high, charging disabled"NL ) : chprintf( chp, "low, charging controllled by I2C"NL );
    
    chprintf(chp, "DPM: ");
    (rx[0] >> 4) & 0x1 ? chprintf( chp, "active"NL ) : chprintf( chp, "not active"NL );

    temp = (rx[0] & 0x7) * 80 + 4200;
    chprintf(chp, "DPM threshold voltage: %dmV"NL, temp);
    
    chprintf(chp, "Charge current programming: ");
    (rx[0] >> 5) & 0x1 ? chprintf( chp, "low"NL ) : chprintf( chp, "normal"NL );

    
    tx[0] = BQ24157_IBAT_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    temp = 680 + ((rx[0] >> 3) & 0xF) * 61;
    chprintf(chp, "IOCHARGE programmable charge current limit: %dmA"NL, temp);

    temp = ((rx[0] & 0x7) * 62) + 62;
    chprintf(chp, "Termination current limit: %dmA"NL, temp);
    
    tx[0] = BQ24157_SAFE_ADR;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, BQ24157_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    temp = 680 + ((rx[0] >> 4) & 0xF) * 121;
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

orchard_shell("chg", chgCommand);

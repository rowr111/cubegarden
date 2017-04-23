#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "charger.h"
#include "i2c.h"

#define NL SHELL_NEWLINE_STR

void gg2Command(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  uint8_t tx[4], rx[3];
  msg_t retval;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: gg2 [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    id        Charger chip ID"SHELL_NEWLINE_STR);
    chprintf(chp, "    soc       State of Charge"NL);
    chprintf(chp, "    on        Set gg on"NL);
    chprintf(chp, "    dump      Dump gg regs"NL);
    return;
  }
  
  if (!strcasecmp(argv[0], "id")) {
    tx[0] = 24; // ic version
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, STC3115_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    chprintf(chp, "ID code: %02x"NL, rx[0] );
  }

  else if(!strcasecmp(argv[0], "on")) {
    tx[0] = 0; // mode
    tx[1] = 0x10; // x001 0000 0x10
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, STC3115_ADDR, tx, 2, NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    tx[0] = 1; // ctrl
    tx[1] = 0x0; // x0000000
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, STC3115_ADDR, tx, 2, NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }

    tx[0] = 15; // cc_cnf
    tx[1] = 0xef; // 1776 = 0x6ef
    tx[2] = 0x6;
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, STC3115_ADDR, tx, 3, NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
  }

  else if (!strcasecmp(argv[0], "soc")) {
    tx[0] = 0x2; // soc
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, STC3115_ADDR, tx, 1, rx, 2, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    chprintf(chp, "State of charge: %.1f%%"NL, ((float)(rx[0] | (rx[1] << 8))) * 0.01953125);

    tx[0] = 0x8; // voltage register
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, STC3115_ADDR, tx, 1, rx, 2, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    chprintf(chp, "Voltage: %.1fmV"NL, ((float)(rx[0] | (rx[1] << 8))) * 2.2 );

    tx[0] = 0x6; // current register
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, STC3115_ADDR, tx, 1, rx, 2, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    chprintf(chp, "Current: %.1fmA"NL, ((float) ((int16_t)(rx[0] | (rx[1] << 8))) ) * 0.2672727 ); // 5.88uV / 22mOhm per LSB

    tx[0] = 10; // temperature
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, STC3115_ADDR, tx, 1, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    chprintf(chp, "Temp: %dC"NL, rx[0] ); 
  }

  else if (!strcasecmp(argv[0], "dump")) {
    int i;
    for( i = 0; i <= 24; i++ ) {
      if( i % 8 == 0 )
	chprintf(chp, NL"%02x: ", i);
      tx[0] = i;
      i2cAcquireBus(&I2CD1);
      retval = i2cMasterTransmitTimeout(&I2CD1, STC3115_ADDR, tx, 1, rx, 1, TIME_INFINITE);
      i2cReleaseBus(&I2CD1);
      if( retval != MSG_OK ) {
	chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
      }
      chprintf(chp, "%02x ", rx[0] );
    }
    chprintf(chp, NL);
  }

}

void ggCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  uint8_t tx[4], rx[3];
  msg_t retval;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: gg [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    id        Charger chip ID"SHELL_NEWLINE_STR);
    chprintf(chp, "    soc       State of Charge"NL);
    chprintf(chp, "    dump      Dump gg regs"NL);
    chprintf(chp, "    on        Set gg on"NL);
    chprintf(chp, "    stby      Set gg standby"NL);
    return;
  }

  if (!strcasecmp(argv[0], "id")) {
    tx[0] = 0x11; // ic version
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, LC709203_ADDR, tx, 1, rx, 3, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    chprintf(chp, "ID code: %04x"NL, rx[0] | (rx[1] << 8));
  }

  else if (!strcasecmp(argv[0], "soc")) {
    tx[0] = 0x0f; // ITE register
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, LC709203_ADDR, tx, 1, rx, 3, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    chprintf(chp, "State of charge: %.1f%%"NL, (rx[0] | (rx[1] << 8)) / 10.0);

    tx[0] = 0x09; // voltage register
    i2cAcquireBus(&I2CD1);
    retval = i2cMasterTransmitTimeout(&I2CD1, LC709203_ADDR, tx, 1, rx, 3, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    chprintf(chp, "Voltage: %dmV"NL, (rx[0] | (rx[1] << 8)));
    
  }
  
  else if (!strcasecmp(argv[0], "dump")) {
    int i;
    for( i = 0; i <= 0x1A; i++ ) {
      if( i % 4 == 0 )
	chprintf(chp, NL"%02x: ", i);
      tx[0] = i;
      i2cAcquireBus(&I2CD1);
      retval = i2cMasterTransmitTimeout(&I2CD1, LC709203_ADDR, tx, 1, rx, 3, TIME_INFINITE);
      i2cReleaseBus(&I2CD1);
      if( retval != MSG_OK ) {
	chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
      }
      chprintf(chp, "%04x ", rx[0] | (rx[1] << 8) );
    }
    chprintf(chp, NL);
  }
  
  else if (!strcasecmp(argv[0], "on")) {
    tx[0] = 0x15; // power mode
    i2cAcquireBus(&I2CD1);
    tx[1] = 0x1;
    tx[2] = 0x0;
    comp_crc8(tx);
    retval = i2cMasterTransmitTimeout(&I2CD1, LC709203_ADDR, tx, 4, NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
  }
  
  else if (!strcasecmp(argv[0], "stby")) {
    tx[0] = 0x15; // power mode
    i2cAcquireBus(&I2CD1);
    tx[1] = 0x2;
    tx[2] = 0x0;
    comp_crc8(tx);
    retval = i2cMasterTransmitTimeout(&I2CD1, LC709203_ADDR, tx, 4, NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
  }
  
  else {
    chprintf(chp, "Unrecognized command: %s"NL, argv[0]);
  }
  
}


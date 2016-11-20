#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#define STC3115_ADDR 0x70
#define FAN5421_ADDR 0x6A
#define LC709203_ADDR 0x0B

#define NL SHELL_NEWLINE_STR

static uint8_t crc_update(uint8_t incrc, uint8_t indata) {
  uint8_t i;
  uint8_t data;

  data = incrc ^ indata;
  for( i = 0; i < 8; i++ ) {
    if(( data & 0x80) != 0) {
      data <<= 1;
      data ^= 0x07;
    } else {
      data <<= 1;
    }
  }
  
  return data;
}

static void comp_crc8(uint8_t *tx) {
  int i;
  uint8_t data[5]; // need to include the I2C address

  // pre-pend the I2C address to the tx buffer in a local copy
  data[0] = 0x16; // they use the left-shifted version for computations
  // note for reads this changes to 0x17
  for( i = 1; i < 4; i++ ) {
    data[i] = tx[i-1];
  }
  
  data[4] = 0x00;
  for( i = 0; i < 4; i++ ) {
    data[4] = crc_update(data[4], data[i]);
  }
  
  tx[3] = data[4];
}

void ggOn(void) {
  uint8_t tx[4];
  
  tx[0] = 0x15; // power mode
  i2cAcquireBus(&I2CD1);
  tx[1] = 0x1;
  tx[2] = 0x0;
  comp_crc8(tx);
  i2cMasterTransmitTimeout(&I2CD1, LC709203_ADDR, tx, 4, NULL, 0, TIME_INFINITE);
  i2cReleaseBus(&I2CD1);
}

void ggCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  uint8_t tx[4], rx[3];
  msg_t retval;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: chg [verb]:"SHELL_NEWLINE_STR);
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


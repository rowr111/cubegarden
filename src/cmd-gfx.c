#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"

// 011_1100
#define SSD1306_ADDR  0x3C

#define NL SHELL_NEWLINE_STR

void gfxCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  uint8_t tx[2], rx[2];
  msg_t retval;
  int i;

  if (argc <= 0) {
    chprintf(chp, "Usage: gfx [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    id        SSD1306 ID"SHELL_NEWLINE_STR);
    chprintf(chp, "    reset     Reset display"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "id")) {
    tx[0] = 0;  // command, not continuation
    tx[1] = 0x81; // read contrast control
    i2cAcquireBus(&I2CD2);
    retval = i2cMasterTransmitTimeout(&I2CD2, SSD1306_ADDR, tx, 2, rx, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD2);
    if( retval != MSG_OK ) {
      chprintf(chp, " I2C transaction error: %d"NL, i2cGetErrors(&I2CD1));
    }
    chprintf(chp, "Value of 0x81 (should be 0x7F default): %02x"NL, rx[0]);
  }
  
  else if (!strcasecmp(argv[0], "reset")) {
    palClearPad(IOPORT1, 18);
    chThdSleepMilliseconds(1);
    palSetPad(IOPORT1, 18);
  }
  
  else {
    chprintf(chp, "Unrecognized command: %s"NL, argv[0]);
  }
}

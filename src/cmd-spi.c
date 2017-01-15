#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#define NL SHELL_NEWLINE_STR

static int sequence = 0;

void spiCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  uint16_t tx[3];
  uint16_t rx[3];

  if (argc <= 0) {
    chprintf(chp, "Usage: spi [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    testw  test write spi interface"SHELL_NEWLINE_STR);
    chprintf(chp, "    testr  test read spi interface"SHELL_NEWLINE_STR);
    chprintf(chp, "    res    reset sequence count"NL);
    chprintf(chp, "    test   basic PHY test"NL);
    chprintf(chp, "    sel    select loop for testing"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "testw")) {
    spiSelect(&SPID1);
    tx[0] = 0x101;
    tx[1] = 0xa000 + sequence;
    tx[2] = 0xb000 + sequence;
    sequence++;
    spiStartExchange(&SPID1, 3, tx, rx);
    while(SPID1.state != SPI_READY)
      chThdYield();
    spiUnselect(&SPID1);
    
    chprintf(chp, "rx0: %04x rx1: %04x"NL, rx[0], rx[1]);
  }

  else if (!strcasecmp(argv[0], "testr")) {
    spiSelect(&SPID1);
    tx[0] = 0x102;
    tx[1] = 0xface;
    spiStartExchange(&SPID1, 3, tx, rx);
    while(SPID1.state != SPI_READY)
      chThdYield();
    spiUnselect(&SPID1);
    
    chprintf(chp, "rx0: %04x rx1: %04x, rx2: %04x"NL, rx[0], rx[1], rx[2]);
  }

  else if (!strcasecmp(argv[0], "res")) {
    sequence = 0;
  }  

  else if (!strcasecmp(argv[0], "test")) {
    spiSelect(&SPID1);
    tx[0] = sequence++;
    spiStartExchange(&SPID1, 1, tx, rx);
    while(SPID1.state != SPI_READY)
      chThdYield();
    spiUnselect(&SPID1);
    chprintf(chp, "tx: %04x"NL, tx[0]);
  }  
  
  else if (!strcasecmp(argv[0], "sel")) {
    while(1) {
      spiSelect(&SPID1);
      spiUnselect(&SPID1);
    }
  }
  
  else {
    chprintf(chp, "Unrecognized command: %s"NL, argv[0]);
  }
  
  return;
}

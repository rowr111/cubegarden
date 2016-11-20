#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include "fpga_config.h"

#define NL SHELL_NEWLINE_STR

uint32_t spiRuntSend(SPIDriver *spip, uint32_t count, uint8_t *data);
void fpgaProgramPage(SPIDriver *spidev, uint32_t page, uint32_t spi_data_count, uint8_t *spi_data);

void fpgaReconfig(void) {
    uint8_t dummy = 0xFF;
    
    spiRuntSend(&SPID2, 1, &dummy); // send a dummy byte to get clock to high default polarity
    // do it while bus is /deselected/
    // this is doen just in case we're off a cold boot
    spiSelect(&SPID2); // SS must be low to set config mode correctly

    palClearPad(IOPORT2, 18);
    chThdSleepMilliseconds(1);
    palSetPad(IOPORT2, 18);
    
    chThdSleepMilliseconds(1);
    spiUnselect(&SPID2);
    
    fpgaProgramPage(&SPID2, 0, fpga_config_size, (uint8_t *) fpga_config);
}

void fpgaCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  if (argc <= 0) {
    chprintf(chp, "Usage: fpga [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    reset     reset FPGA"SHELL_NEWLINE_STR);
    chprintf(chp, "    stat      config state"NL);
    chprintf(chp, "    config    load config"NL);
    return;
  }

  if (!strcasecmp(argv[0], "reset")) {
    uint8_t dummy = 0xFF;
    spiRuntSend(&SPID2, 1, &dummy); // send a dummy byte to get clock to high default polarity
    // do it while bus is /deselected/
    // this is doen just in case we're off a cold boot
    
    spiSelect(&SPID2); // SS must be low to set config mode correctly

    palClearPad(IOPORT2, 18);
    chThdSleepMilliseconds(1);
    palSetPad(IOPORT2, 18);
    
    chThdSleepMilliseconds(1);
    spiUnselect(&SPID2);
  }
  
  else if (!strcasecmp(argv[0], "stat")) {
    chprintf(chp, "Done pin status: "NL);
    palReadPad(IOPORT1, 12) == PAL_HIGH ? chprintf(chp, "high"NL) : chprintf(chp, "low"NL);
  }
  
  else if (!strcasecmp(argv[0], "config")) {
    fpgaReconfig();
  }

  else {
    chprintf(chp, "Unrecognized command: %s"NL, argv[0]);
  }
  
  return;
}

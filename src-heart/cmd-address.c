#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "shellcfg.h"

#include <stdlib.h>
#include <string.h>

#include "orchard-app.h"
#include "paging.h"
#include "radio.h"
#include "address.h"
#include "userconfig.h"

void cmd_address(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argc;
  const struct userconfig *config;
  config = getConfig();

  if (argc == 0) {
    chprintf(chp, "Usage: address <command>"SHELL_NEWLINE_STR);
    chprintf(chp, "  get - returns next cube address"SHELL_NEWLINE_STR);
    chprintf(chp, "  set <int> (1-254) - sets next cube address"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "get")) {
    chprintf(chp, "next new cube address: %d"SHELL_NEWLINE_STR, config->cfg_addressCounter);
  }

  if (!strcasecmp(argv[0], "set")) {
    if (argc == 2) {
      uint8_t address = (uint8_t) atoi(argv[1]);
      if (address >= 1 && address <= 254) {
	chprintf(chp, "Setting next new cube address to %d\n\r", address);
        setRadioAddressCounter(address);
      } else {
        chprintf(chp, "Invalid address: %d"SHELL_NEWLINE_STR, address);
      }
    }
  }
  
}

orchard_shell("address", cmd_address);

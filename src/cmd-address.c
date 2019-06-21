#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "shellcfg.h"

#include <stdlib.h>
#include <string.h>

#include "cmd-forward.h"
#include "orchard-app.h"
#include "paging.h"
#include "radio.h"
#include "address.h"

void cmd_address(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argc;

  if (argc == 0) {
    chprintf(chp, "Usage: address <command>"SHELL_NEWLINE_STR);
    chprintf(chp, "  get"SHELL_NEWLINE_STR);
    chprintf(chp, "  set <int> (1-254)"SHELL_NEWLINE_STR);
    chprintf(chp, "  request"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "get")) {
    chprintf(chp, "%d"SHELL_NEWLINE_STR, radioAddress(radioDriver));
  }

  if (!strcasecmp(argv[0], "set")) {
    if (argc == 2) {
      uint8_t address = (uint8_t) atoi(argv[1]);
      if (address >= 1 && address <= 254) {
        radioSetAddress(radioDriver, address);
      } else {
        chprintf(chp, "Invalid address: %d"SHELL_NEWLINE_STR, address);
      }
    }
  }

  if (!strcasecmp(argv[0], "request")) {
    requestRadioAddress();
  }
}

orchard_shell("address", cmd_address);
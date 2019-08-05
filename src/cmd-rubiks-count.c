#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "shellcfg.h"

#include <stdlib.h>
#include <string.h>

#include "cmd-forward.h"
#include "orchard-app.h"
#include "paging.h"
#include "led.h"

void cmd_rubiks_count(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argc;

  if (argc ==0) {
    chprintf(chp, "Usage: rubiks-count <command>"SHELL_NEWLINE_STR);
    chprintf(chp, " update <int> (0-5) <int> (0-5)"SHELL_NEWLINE_STR);
    chprintf(chp, " add <int> (0-5)"SHELL_NEWLINE_STR);
    chprintf(chp, " reset"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "update")) {
      uint8_t old = (uint8_t) atoi(argv[1]);
      uint8_t new = (uint8_t) atoi(argv[2]);
      update_rubiks(old, new);
  }
  if (!strcasecmp(argv[0], "add")) {
      uint8_t color_index = (uint8_t) atoi(argv[1]);
      add_rubiks(color_index);
  }
  if (!strcasecmp(argv[0], "reset")) {
      reset_rubiks();
  }
}

orchard_shell("rubiks-count", cmd_rubiks_count);

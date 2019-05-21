#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "shellcfg.h"

#include <string.h>
#include "orchard-app.h"
#include "paging.h"
#include "time.h"

void cmd_time(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argc;
  (void)argv;

  if (argc == 0) {
    chprintf(chp, "Usage: time on/off/broadcast/get/status"SHELL_NEWLINE_STR);
    chprintf(chp, SHELL_NEWLINE_STR);
    chprintf(chp, "on        Turn on broadcasting of time"SHELL_NEWLINE_STR);
    chprintf(chp, "off       Turn off broadcasting of time"SHELL_NEWLINE_STR);
    chprintf(chp, "ping      Do a one-off broadcast of time"SHELL_NEWLINE_STR);
    chprintf(chp, "get       Get current network time"SHELL_NEWLINE_STR);
    chprintf(chp, "status    Returns whether broadcasting is on or off"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "on")) {
    timekeeper = true;
  }

  if (!strcasecmp(argv[0], "off")) {
    timekeeper = false;
  }

  if (!strcasecmp(argv[0], "ping")) {
    broadcastTime();
  }

  if (!strcasecmp(argv[0], "get")) {
    chprintf(chp, "%d"SHELL_NEWLINE_STR, getNetworkTimeMs());
  }

  if (!strcasecmp(argv[0], "status")) {
    chprintf(chp, "Time broadcast: %s"SHELL_NEWLINE_STR, timekeeper ? "on" : "off");
  }
  
  return;
}

orchard_shell("time", cmd_time);

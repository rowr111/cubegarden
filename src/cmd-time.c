#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "shellcfg.h"

#include <string.h>
#include <stdlib.h>
#include "orchard-app.h"
#include "paging.h"
#include "time.h"

static int should_stop(void) {
  uint8_t bfr[1];
  return chnReadTimeout(&SD2, bfr, sizeof(bfr), 1);
}

extern int32_t offsetMs;
extern int32_t time_slop;

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
    chprintf(chp, "offset    Monitor offset in real time"SHELL_NEWLINE_STR);
    chprintf(chp, "slop <int>  Set time trim latency"SHELL_NEWLINE_STR);
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

  if (!strcasecmp(argv[0], "offset")) {
    while( !should_stop() ) {
      chprintf(chp, "%d\r", offsetMs);
    }
  }

  if (!strcasecmp(argv[0], "slop")) {
    if(argc != 2) {
      chprintf(chp, "Expecting signed integer as argument to slop\n\r" );
    }
    time_slop = strtol(argv[1], NULL, 0);
    chprintf(chp, "Slop set to %d\n\r", time_slop );
  }
  
  return;
}

orchard_shell("time", cmd_time);

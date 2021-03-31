#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include <string.h>

#include "shellcfg.h"

static int should_stop(void) {
  uint8_t bfr[1];
  return chnReadTimeout(&SD2, bfr, sizeof(bfr), 1);
}

#define TELEMETRY_STREAM (BaseSequentialStream *)&SD1

void cmd_telemetry(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  if (argc <= 0) {
    chprintf(chp, "Usage: telemetry <command>\n\r");
    chprintf(chp, "  test\n\r");
    return;
  }

  if (!strcasecmp(argv[0], "test")) {
    chprintf(chp, "sending stream of test character 'a', hit enter to stop\n\r");
    while( !should_stop() ) {
      chprintf(TELEMETRY_STREAM, "a");
    }
  } else {
    chprintf(chp, "telemetry requires an argument\n\r");
  }
  
  return;
}
void cmd_telemetry_alias(BaseSequentialStream *chp, int argc, char *argv[]) {
  cmd_telemetry(chp, argc, argv);
}

orchard_shell("telemetry", cmd_telemetry);
orchard_shell("t", cmd_telemetry_alias);

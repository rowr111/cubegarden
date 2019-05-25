#include <stdlib.h>
#include <strings.h> 
#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "orchard-effects.h"
#include "shellcfg.h"
#include "led.h"

#define NL SHELL_NEWLINE_STR

void tuneCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  
  if (argc <= 0) {
    chprintf(chp, "    stats        constants summary"NL);
    chprintf(chp, "Usage: tune [constant] [value]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    dBbkgd       dB background threshold (total range: 0-120)"NL);
    chprintf(chp, "    dBMax        dB maximum (total range: 0-120)"NL);
    chprintf(chp, "    pressure     amount to trigger change notification in mPa"NL);
    return;
  }

  if (!strcasecmp(argv[0], "stats")) {
      chprintf(stream, "%s", "background dB threshold: ");
      chprintf(stream, "%d\n\r", getdBbkgd());
      chprintf(stream, "%s", "max dB threshold: ");
      chprintf(stream, "%d\n\r", getdBMax());
      chprintf(stream, "%s", "pressure trigger amnt (mPa): ");
      chprintf(stream, "%d\n\r", getPressTriggerAmnt());
  }
  else if (!strcasecmp(argv[0], "dBbkgd") && argc == 2) {
      setdBbkgd(atoi(argv[1]));
  }
  else if (!strcasecmp(argv[0], "dBMax") && argc == 2)  {
      setdBMax(atoi(argv[1]));
  }
  else if (!strcasecmp(argv[0], "pressure") && argc == 2)  {
      setPressTriggerAmnt(atoi(argv[1]));
  }
  
  return;
}

orchard_shell("tune", tuneCommand);

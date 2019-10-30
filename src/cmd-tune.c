#include <stdlib.h>
#include <strings.h> 
#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "orchard-effects.h"
#include "shellcfg.h"
#include "led.h"
#include "userconfig.h"

#define NL SHELL_NEWLINE_STR

void tuneCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  const struct userconfig *config;
  config = getConfig();
  
  if (argc <= 0) {
    chprintf(chp, "    stats       constants summary"NL);
    chprintf(chp, "Usage: tune [constant] [value]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    dBbkgd       dB background threshold (total range: 0-120)"NL);
    chprintf(chp, "    dBmax        dB maximum (total range: 0-120)"NL);
    chprintf(chp, "    bright1      1st battery brightness threshhold (default = 3750)"NL);
    chprintf(chp, "    bright2      2nd battery brightness threshhold (default = 3650)"NL);
    chprintf(chp, "    bright3      3rd battery brightness threshhold (default = 3550)"NL);
    chprintf(chp, "    newcube      time for a newcube to flash in effects (default = 4)"NL);
    return;
  }

  if (!strcasecmp(argv[0], "stats")) {
      chprintf(stream, "%s", "background dB threshold: ");
      chprintf(stream, "%d\n\r", config->cfg_dBbkgd);
      chprintf(stream, "%s", "max dB threshold: ");
      chprintf(stream, "%d\n\r", config->cfg_dBmax);
      chprintf(stream, "%s", "first battery brightness threshold: ");
      chprintf(stream, "%d\n\r", config->cfg_bright_thresh);
      chprintf(stream, "%s", "second battery brightness threshold: ");
      chprintf(stream, "%d\n\r", config->cfg_bright_thresh2);
      chprintf(stream, "%s", "third battery brightness threshold: ");
      chprintf(stream, "%d\n\r", config->cfg_bright_thresh3);
      chprintf(stream, "newcube: %ds\n\r", config->cfg_fx_newcube_time );
  }
  else if (!strcasecmp(argv[0], "dBbkgd") && argc == 2) {
     uint8_t db = atoi(argv[1]);
      db = db > 120 ? 120 : db; //max usable value of 120
      configSetdBbkgd(db);
  }
  else if (!strcasecmp(argv[0], "dBmax") && argc == 2)  {
      uint8_t db = atoi(argv[1]);
      db = db > 120 ? 120 : db; //max usable value of 120
      configSetdBmax(db);
  }
  else if (!strcasecmp(argv[0], "bright1") && argc == 2)  {
    configSetBrightThresh(strtoul(argv[1], NULL, 0));
  }
  else if (!strcasecmp(argv[0], "bright2") && argc == 2)  {
    configSetBrightThresh2(strtoul(argv[1], NULL, 0));
  }
  else if (!strcasecmp(argv[0], "bright3") && argc == 2)  {
    configSetBrightThresh3(strtoul(argv[1], NULL, 0));
  }
  else if (!strcasecmp(argv[0], "newcube") && argc == 2)  {
    configSetFxNewcubeTime((uint8_t) strtoul(argv[1], NULL, 0));
  }
  
  return;
}

orchard_shell("tune", tuneCommand);

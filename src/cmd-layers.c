#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "orchard-effects.h"
#include "shellcfg.h"

#include "led.h"

#include <strings.h>
#include <stdlib.h>

#define NL SHELL_NEWLINE_STR

void lxCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: lx [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    list         list layers"NL);
    chprintf(chp, "    use <layer> switch given layer on/off"NL);
    return;
  }

  else if (!strcasecmp(argv[0], "list")) {
    listLayers();
  }

  else if (!strcasecmp(argv[0], "use") && argc == 3) {
    // validate arguments before using them
    uint8_t layer = layerNameLookup(argv[1]);
    uint8_t active = atoi(argv[2]); // only 0 or 1 expected
    if ((layer != 255) && (active < 2)) {
      effectsSetLayer(layerNameLookup(argv[1]), atoi(argv[2]));
    }
  }
  return;
}

orchard_shell("lx", lxCommand);

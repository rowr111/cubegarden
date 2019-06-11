#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "orchard-effects.h"
#include "shellcfg.h"
#include "led.h"
#include <string.h>
#include <stdlib.h>

#define NL SHELL_NEWLINE_STR

void fxCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: fx [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    next         next effect"NL);
    chprintf(chp, "    prev         previous effect"NL);
    chprintf(chp, "    list         list effects"NL);
    chprintf(chp, "    get          return name of current effect"NL);
    chprintf(chp, "    use <effect> switch to given effect"NL);
    chprintf(chp, "    trigger <effect> <cubeid> trigger request for a particular effect from cube");
    return;
  }

  if (!strcasecmp(argv[0], "next")) {
    effectsNextPattern(0);
  }

  else if (!strcasecmp(argv[0], "prev")) {
    effectsPrevPattern(0);
  }

  else if (!strcasecmp(argv[0], "list")) {
    listEffects();
  }

  else if (!strcasecmp(argv[0], "get")) {
    chprintf(chp, "%s"SHELL_NEWLINE_STR, effectsCurName());
  }

  else if (!strcasecmp(argv[0], "use") && argc == 2) {
    effectsSetPattern(effectsNameLookup(argv[1]));
  }

  else if (!strcasecmp(argv[0], "trigger") && argc == 3) {
    if(!strcasecmp(argv[1], "rainbowblast")){
      trigger_rb((uint8_t)atoi(argv[2]));
    }
  }
  
  return;
}

orchard_shell("fx", fxCommand);

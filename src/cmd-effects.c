#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "orchard-effects.h"

#define NL SHELL_NEWLINE_STR

void fxCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: fx [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    next      next effect"NL);
    chprintf(chp, "    prev      previous effect"NL);
    chprintf(chp, "    list      list effects"NL);
    return;
  }

  if (!strcasecmp(argv[0], "next")) {
    effectsNextPattern();
  }

  else if (!strcasecmp(argv[0], "prev")) {
    effectsPrevPattern();
  }

  else if (!strcasecmp(argv[0], "list")) {
    listEffects();
  }
  
  return;
}

#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "orchard-effects.h"
#include "shellcfg.h"

#include "led.h"

#include <strings.h>
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
    chprintf(chp, "    numcolors [uint8_t]  set number of HSV color options in base color palette"NL);
    chprintf(chp, "    hsvsat [uint8_t]  set saturation of base HSV colors"NL);
    chprintf(chp, "    hsvvalue [uint8_t]  set brightness (value) of base HSV colors"NL);
    chprintf(chp, "    layout [uint8_t]  1=donut, 2=rows of 10, 3=rows of 5"NL);
    chprintf(chp, "    debug        print the current loop state to console"NL);
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
  else if (!strcasecmp(argv[0], "numcolors") && argc == 2) {
     numOfBaseHsvColors =  atoi(argv[1]);
     chprintf(chp, "Number of base HSV colors updated to: %d\n\r", numOfBaseHsvColors);
  }
  else if (!strcasecmp(argv[0], "hsvsat") && argc == 2) {
     baseHsvSaturation =  atoi(argv[1]);
     chprintf(chp, "Base HSV saturation updated to: %d\n\r", baseHsvSaturation);
  }
  else if (!strcasecmp(argv[0], "hsvvalue") && argc == 2) {
     baseHsvValue =  atoi(argv[1]);
     chprintf(chp, "Base HSV brightness updated to: %d\n\r", baseHsvValue);
  }
  else if (!strcasecmp(argv[0], "layout") && argc == 2) {
     cube_layout =  atoi(argv[1]);
     if(cube_layout == 1) {
      chprintf(chp, "cube layout updated to donut (6 offsets).\n\r");
     }
     else if (cube_layout == 2) {
       chprintf(chp, "cube layout updated to rows of 10 (10 offsets).\n\r");
     }
    else if (cube_layout == 3) {
       chprintf(chp, "cube layout updated to rows of 5 (5 offsets).\n\r");
     }
  }
  else if (!strcasecmp(argv[0], "debug")) {
    effectsDebug();
  }
  return;
}

orchard_shell("fx", fxCommand);

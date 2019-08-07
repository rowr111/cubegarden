#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include <stdlib.h>
#include <string.h>

#include "led.h"
#include "shellcfg.h"

#include "charger.h"

/* 
   Adding a new command:

   1. the C file must have the name structure cmd-*.c
   2. the function prototype must be added to the "shell commands" list in shellcfg.c
   3. the command name and function name should be entered into the ShellCommand commands[] structure in shellcfg.c
 */


void cmd_superrgb(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  uint8_t r;
  uint8_t g;
  uint8_t b;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: superrgb [0-255] [0-255] [0-255] (ONLY FOR EFFECT USAGE)"SHELL_NEWLINE_STR);
    return;
  }

  r = atoi(argv[0]);
  g = atoi(argv[1]);
  b = atoi(argv[2]);

  //chprintf(chp, "rgb values: %d %d %d.\n\r", r, g, b);
  
  superRgb.r = r;
  superRgb.g = g;
  superRgb.b = b;

  superRgbLastTime = chVTGetSystemTime(); //set the time every time this command is called

  return;
}

orchard_shell("superrgb", cmd_superrgb);

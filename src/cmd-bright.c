#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include <string.h>

#include "led.h"

/* 
   Adding a new command:

   1. the C file must have the name structure cmd-*.c
   2. the function prototype must be added to the "shell commands" list in shellcfg.c
   3. the command name and function name should be entered into the ShellCommand commands[] structure in shellcfg.c
 */

void cmd_bright(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  uint8_t s;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: bright [+/-]:"SHELL_NEWLINE_STR);
    return;
  }

  if( argv[0][0] == '-' ) {
    s = getShift();
    if( s < 7 )
      s++;
    setShift(s);
  } else if( argv[0][0] == '+' ) {
    s = getShift();
    if( s > 0 )
      s--;
    setShift(s);
  } else {
    chprintf(chp, "bright takes argument of either + or -\n\r");
  }
  
  return;
}

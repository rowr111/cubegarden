#include "hal.h"
#include "shell.h"
#include "chprintf.h"

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
extern uint8_t stashed_shift;

void cmd_bright(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  uint8_t s;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: bright [+/-,0-7]:"SHELL_NEWLINE_STR);
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
  } else if( argv[0][0] >= '0' && argv[0][0] <= '7' ) {
    if( isCharging() ) {
      stashed_shift = 7 - (argv[0][0] - '0');
    } else {
      setShift( 7 - (argv[0][0] - '0') );
    }
  } else {
    chprintf(chp, "bright takes argument of either +, -, or a number 0-7\n\r");
  }
  
  return;
}
void cmd_bright_alias(BaseSequentialStream *chp, int argc, char *argv[]) {
  cmd_bright(chp, argc, argv);
}

orchard_shell("bright", cmd_bright);
orchard_shell("b", cmd_bright_alias);

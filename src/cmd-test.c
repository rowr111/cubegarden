#include "hal.h"
#include "shell.h"
#include "chprintf.h"


void testCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  
  chprintf(chp, "test"SHELL_NEWLINE_STR);

  return;
}

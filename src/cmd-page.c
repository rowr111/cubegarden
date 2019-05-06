#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "shellcfg.h"

#include <string.h>
#include "orchard-app.h"
#include "paging.h"

void cmd_page(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argc;
  (void)argv;

  if (argc > 0) {
    chprintf(chp, "Usage: page:"SHELL_NEWLINE_STR);
    return;
  }

  chprintf(stream, "Synthetic page event generated\n\r");
  
  chEvtBroadcast(&radio_page);
  
  return;

}

orchard_shell("page", cmd_page);

#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "shellcfg.h"

#include <stdlib.h>
#include <string.h>

#include "cmd-forward.h"
#include "orchard-app.h"
#include "radio.h"
#include "baton.h"

void cmd_baton(BaseSequentialStream *chp, int argc, char *argv[]) {
  BatonState *bstate;

  bstate = getBatonState();
  
  if (argc == 0) {
    chprintf(chp, "Usage: baton <command>"SHELL_NEWLINE_STR);
    chprintf(chp, "  status"SHELL_NEWLINE_STR);
    chprintf(chp, "  force"SHELL_NEWLINE_STR);
    chprintf(chp, "  passrand"SHELL_NEWLINE_STR);
    chprintf(chp, "  passinc"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "status")) {
    chprintf(chp, " state: ");
    if( bstate->state == baton_passing )
      chprintf(chp, "passing\n\r");
    if( bstate->state == baton_not_holding )
      chprintf(chp, "not holding\n\r");
    if( bstate->state == baton_holding )
      chprintf(chp, "holding\n\r");

    chprintf(chp, " strategy: ");
    if( bstate->strategy == baton_random )
      chprintf(chp, "random\n\r" );
    if( bstate->strategy == baton_increment )
      chprintf(chp, "increment\n\r" );
    if( bstate->strategy == baton_specified )
      chprintf(chp, "specified\n\r" );

    chprintf(chp, " passing_to_addr: %d\n\r", bstate->passing_to_addr);
    chprintf(chp, " rtry_interval: %d\n\r", bstate->retry_interval);
  }

  if (!strcasecmp(argv[0], "force")) {
    bstate->state = baton_holding;
    chprintf(chp, "I'm now the baton holder.\n\r");
  }

  if (!strcasecmp(argv[0], "passrand")) {
    chprintf(chp, "Initiating a random pass with retry = 500ms.\n\r");
    passBaton(baton_random, 0, 500);
  }
  
  if (!strcasecmp(argv[0], "passinc")) {
    chprintf(chp, "Initiating a incrementing sweep with retry = 500ms.\n\r");
    passBaton(baton_increment, 0, 500);
  }
}


orchard_shell("baton", cmd_baton);

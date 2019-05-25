#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include <string.h>

#include "mic.h"
#include "led.h"
#include "shellcfg.h"

static int should_stop(void) {
  uint8_t bfr[1];
  return chnReadTimeout(&SD4, bfr, sizeof(bfr), 1);
}

uint8_t fft_bin;
void cmd_sound(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argc;
  (void)argv;

  if (argc <= 0) {
    chprintf(chp, "Usage: sound [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "  where verb is one of:\n\r" );
    chprintf(chp, "    db -- monitor sound db\n\r" );
    return;
  }

  if(!strcasecmp(argv[0], "db")) {
    while( !should_stop() ) {
      chprintf( chp, "%3.0f dB\r", cur_db );
      chThdYield();
      chThdSleepMilliseconds(10);
    }
  } else {
    chprintf(chp, "unknown verb %s\n\r", argv[0]);
  }
  return;

}

orchard_shell("sound", cmd_sound);

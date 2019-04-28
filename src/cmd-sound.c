#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include <string.h>

#include "mic.h"
#include "led.h"

static int should_stop(void) {
  uint8_t bfr[1];
  return chnReadTimeout(&SD4, bfr, sizeof(bfr), 1);
}

uint8_t fft_bin;
void cmd_sound(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argc;
  (void)argv;

  uint8_t i;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: sound [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "  where verb is one of:\n\r" );
    chprintf(chp, "    fft -- monitor sound FFT\n\r" );
    return;
  }

  if(!strcasecmp(argv[0], "fft")) {
    while( !should_stop() ) {
      for( i = 0; i < 32; i++ ) {
	chprintf(chp, "%02d ", mic_processed[i] / 1024);
      }
      chprintf(chp, "\n\r" );
    }
  } else if( argv[0][0] == '+' ) {
    if( fft_bin < NUM_SAMPLES / 2 )
      fft_bin++;
  } else if( argv[0][0] == '-' ) {
    if( fft_bin > 0 )
      fft_bin--;
  } else {
    chprintf(chp, "unknown verb %s\n\r", argv[0]);
  }
  return;

}

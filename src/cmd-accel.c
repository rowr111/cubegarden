#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "shellcfg.h"

#include <string.h>

#include "accel.h"

static int should_stop(void) {
  uint8_t bfr[1];
  return chnReadTimeout(&SD4, bfr, sizeof(bfr), 1);
}

void cmd_accel(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argc;
  (void)argv;

  struct old_accel_data data;
  int x, y, z;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: accel [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "  where verb is one of:\n\r" );
    chprintf(chp, "    mon -- monitor current acceleration\n\r" );
    return;
  }

  if(!strcasecmp(argv[0], "mon")) {
    while( !should_stop() ) {
      accelPoll(&data);
      if( data.x >= 2048 ) {
	x = (int)data.x - 4095;
      } else
	x = data.x;
      if( data.y >= 2048 ) {
	y = (int)data.y - 4095;
      } else
	y = data.y;
      if( data.z >= 2048 ) {
	z = (int)data.z - 4095;
      } else
	z = data.z;
      chprintf(chp, "x: %4d, y: %4d, z: %4d\n\r", x, y, z );
    }
  } else {
    chprintf(chp, "unknown verb %s\n\r", argv[0]);
  }
  return;

}

orchard_shell("accel", cmd_accel);

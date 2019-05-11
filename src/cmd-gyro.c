#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "i2c.h"
#include "gyro.h"
#include "shellcfg.h"

#define NL SHELL_NEWLINE_STR


static int should_stop(void) {
  uint8_t bfr[1];
  return chnReadTimeout(&SD4, bfr, sizeof(bfr), 1);
}

void gyroCommand(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argc;
  (void)argv;
   
  if (argc <= 0) {
    chprintf(chp, "Usage: gyro [verb]:"SHELL_NEWLINE_STR);
    //    chprintf(chp, "    init      initialize gyro"SHELL_NEWLINE_STR);
    chprintf(chp, "    tap       tap demo"SHELL_NEWLINE_STR);
    chprintf(chp, "    double    double tap demo"SHELL_NEWLINE_STR);
    chprintf(chp, "    pedo      pedometer demo"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "double")) {
    gyro_init();
    gyro_Enable_X();
    gyro_Enable_Double_Tap_Detection();
    while( !should_stop() ) {
      if( mems_event ) { // mems_event set by external interrupt handler
	mems_event = 0;
	LSM6DS3_Event_Status_t status;
	gyro_Get_Event_Status(&status);
	if (status.DoubleTapStatus) {
	  chprintf(chp, "Double Tap Detected!\n\r");
	}
      }
    }
  } else if(!strcasecmp(argv[0], "tap")) {
    gyro_init();
    gyro_Enable_X();
    gyro_Enable_Single_Tap_Detection();
    while( !should_stop() ) {
      if( mems_event ) { // mems_event set by external interrupt handler
	mems_event = 0;
	LSM6DS3_Event_Status_t status;
	gyro_Get_Event_Status(&status);
	if (status.TapStatus) {
	  chprintf(chp, "Single Tap Detected!\n\r");
	}
      }
    }
  } else if(!strcasecmp(argv[0], "pedo")) {
    uint32_t previous_tick = 0;
    uint16_t step_count = 0;
    
    gyro_init();
    gyro_Enable_X();
    gyro_Enable_Pedometer();
    previous_tick = chVTGetSystemTime();
    
    while( !should_stop() ) {
      if( mems_event ) { // mems_event set by external interrupt handler
	mems_event = 0;
	LSM6DS3_Event_Status_t status;
	gyro_Get_Event_Status(&status);
	if (status.StepStatus) {
	  gyro_Get_Step_Counter(&step_count);
	  chprintf(chp, "Step counter: %d\n\r", step_count);
	}
	if( chVTTimeElapsedSinceX(previous_tick) > 3000 ) { 
	  gyro_Get_Step_Counter(&step_count);
	  chprintf(chp, "Step counter: %d\n\r", step_count);
	  previous_tick = chVTGetSystemTime();
	}
      }
    }
  } else {
    chprintf(chp, "Unrecognized command: %s"NL, argv[0]);
  }
  
}
orchard_shell("gyro", gyroCommand);


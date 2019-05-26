#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "i2c.h"
#include "gyro.h"
#include "shellcfg.h"
#include "math.h"

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
    chprintf(chp, "    freefall  freefall demo"SHELL_NEWLINE_STR);
    chprintf(chp, "    xyz"SHELL_NEWLINE_STR);
    chprintf(chp, "    stepdir   stepdirection demo"SHELL_NEWLINE_STR);
    chprintf(chp, "    zincl     current z inclination degree"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "double")) {
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
    while( !should_stop() ) {
      //gyro_Set_Tap_Threshold(LSM6DS3_TAP_THRESHOLD_LOW);
      if( mems_event ) { // mems_event set by external interrupt handler
        mems_event = 0;
        LSM6DS3_Event_Status_t status;
        gyro_Get_Event_Status(&status);
        if (status.TapStatus) {
          chprintf(chp, "Single Tap Detected!\n\r");
	      }
      }
    }
   } else if(!strcasecmp(argv[0], "zincl")) {
    while( !should_stop() ) {
      chprintf(chp, "z inclination (deg): %d\n\r", z_inclination);
    }
  } else if(!strcasecmp(argv[0], "pedo")) {
    uint32_t previous_tick = 0;
    uint16_t step_count = 0;
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
  }else if(!strcasecmp(argv[0], "freefall")) {
    //gyro_init();
    while( !should_stop() ) {
      if( mems_event ) { // mems_event set by external interrupt handler
      mems_event = 0;
	    LSM6DS3_Event_Status_t status;
	    gyro_Get_Event_Status(&status);
	    if (status.FreeFallStatus) {
	      chprintf(chp, "FreeFall Detected!\n\r");
	      }
      }
    }
  } else if(!strcasecmp(argv[0], "xyz")) {
      struct accel_data xyzData;
      while( !should_stop() ) {
        gyro_Get_X_Axes(&xyzData);
        chprintf(chp, "X: %d", xyzData.x);
        chprintf(chp, " Y: %d", xyzData.y);
        chprintf(chp, " Z: %d\n\r", xyzData.z);
      }
    } else if(!strcasecmp(argv[0], "stepdir")) {
      //direction of acceleration (in the x/y plane) upon step
    gyro_Set_Pedometer_Threshold(LSM6DS3_PEDOMETER_THRESHOLD_MID); //make it a little more sensitive, def is MID_HIGH
    uint16_t step_count = 0;
    while( !should_stop() ) {
      if( mems_event ) { // mems_event set by external interrupt handler
      mems_event = 0;
      LSM6DS3_Event_Status_t status;
      gyro_Get_Event_Status(&status);
        if (status.StepStatus) {
          gyro_Get_Step_Counter(&step_count);
          struct accel_data accelData;
          gyro_Get_X_Axes(&accelData);
          float directionDeg = atan2(accelData.x, accelData.y) * 180/3.14159;
          chprintf(chp, "Accel direction on step (degree): %f\n\r", directionDeg);
        }
      }
    }
  } else {
    chprintf(chp, "Unrecognized command: %s"NL, argv[0]);
  }
  
}
orchard_shell("gyro", gyroCommand);


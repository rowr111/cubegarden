#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "shellcfg.h"

#include <string.h>
#include <stdlib.h>
#include "orchard-app.h"

#include "vl53l1_platform.h"
#include "VL53L1X_api.h"

static int should_stop(void) {
  uint8_t bfr[1];
  return chnReadTimeout(&SD2, bfr, sizeof(bfr), 1);
}

#define NL SHELL_NEWLINE_STR

static uint16_t xtalk_param = 0;

void cmd_lidar(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argc;
  (void)argv;

  if (argc == 0) {
    chprintf(chp, "Usage: lidar test"SHELL_NEWLINE_STR);
    chprintf(chp, SHELL_NEWLINE_STR);
    chprintf(chp, "test        Test interface to LIDAR"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "test")) {
    uint8_t error;
    uint16_t id;

    *((unsigned int *) 0x400ff0c8) = 0x2; // portd clear toggle
    chThdSleepMilliseconds(2);
    *((unsigned int *) 0x400ff0c4) = 0x2; // portd set toggle
    chThdSleepMilliseconds(2);
    /*
    uint8_t tx[2] = { 0x01, 0x0f };
    uint8_t rx[1];
    msg_t retval;
    retval = i2cMasterTransmitTimeout(&I2CD1, 0x29, tx, 2, rx, 1, TIME_INFINITE);
    chprintf(chp, "retval: %d, model_ID: %x\n\r", retval, rx[0]);

    tx[0] = 0x01;
    tx[1] = 0x10;
    retval = i2cMasterTransmitTimeout(&I2CD1, 0x29, tx, 2, rx, 1, TIME_INFINITE);
    chprintf(chp, "retval: %d, module type: %x\n\r", retval, rx[0]);

    tx[0] = 0x01;
    tx[1] = 0x11;
    retval = i2cMasterTransmitTimeout(&I2CD1, 0x29, tx, 2, rx, 1, TIME_INFINITE);
    chprintf(chp, "retval: %d, mask rev: %x\n\r", retval, rx[0]);
    */
    error = VL53L1X_GetSensorId(0, &id);
    chprintf(chp, "error: %d, ID: 0x%04x"NL, error, id);

    uint8_t bootstate = 0;
    while(bootstate == 0) {
      VL53L1X_BootState(0, &bootstate);
      chThdSleepMilliseconds(2);
    }

    VL53L1X_SensorInit(0);

    VL53L1X_SetDistanceMode(0, 2); /* 1=short, 2=long */
    VL53L1X_SetTimingBudgetInMs(0, 100); /* in ms possible values [20, 50, 100, 200, 500] */
    VL53L1X_SetInterMeasurementInMs(0, 100); /* in ms, IM must be > = TB */
    
    VL53L1X_SetXtalk(0, xtalk_param);
    chThdSleepMilliseconds(2);
    uint16_t xtalk = 0;
    VL53L1X_GetXtalk(0, &xtalk);
    chprintf(chp, "current xtalk value: %d\n\r", xtalk);

    VL53L1X_StartRanging(0);

    uint8_t dataready = 0;
    uint16_t Distance;
    uint16_t SignalRate;
    uint16_t AmbientRate;
    uint16_t SpadNum; 
    uint8_t RangeStatus;
    
    while(!should_stop()) {
      while( dataready == 0 ) {
	VL53L1X_CheckForDataReady(0, &dataready);
	chThdSleepMilliseconds(2);
      }
      dataready = 0;
      
      VL53L1X_GetRangeStatus(0, &RangeStatus);
      VL53L1X_GetDistance(0, &Distance);
      VL53L1X_GetSignalRate(0, &SignalRate);
      VL53L1X_GetAmbientRate(0, &AmbientRate);
      VL53L1X_GetSpadNb(0, &SpadNum);
      VL53L1X_ClearInterrupt(0); /* clear interrupt has to be called to enable next interrupt*/
      chprintf(chp, "range %u, dist %u, sigrate %u, ambrate %u, spadnum %u\n\r", RangeStatus, Distance, SignalRate, AmbientRate,SpadNum);
    }

    VL53L1X_StopRanging(0);
  }
  if (!strcasecmp(argv[0], "xtget")) {
    uint16_t xtalk = 0;
    VL53L1X_GetXtalk(0, &xtalk);
    chprintf(chp, "current xtalk value: %d\n\r", xtalk);
  }
  if (!strcasecmp(argv[0], "xtset")) {
    uint16_t xtalk = 0;
    xtalk_param = (uint16_t) atoi(argv[1]);
    VL53L1X_SetXtalk(0, xtalk_param);
    VL53L1X_GetXtalk(0, &xtalk);
    chprintf(chp, "current xtalk value: %d\n\r", xtalk);
  }

  return;
}

orchard_shell("lidar", cmd_lidar);


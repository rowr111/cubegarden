#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "i2c.h"
#include "si1153.h"
#include "shellcfg.h"

#define NL SHELL_NEWLINE_STR

static i2cSensorConfig_t sensI2C;			//Holds i2c information about the connected sensor
static Si115xSample_t samples;				//Stores the sample data from reading the sensor
static uint8_t initialized = SI11XX_NONE;	//Tracks which sensor demo is initialized

void getSensorData(void)
{
	// Start next measurement
	Si115xForce(&sensI2C);

	// Sensor data ready
	// Process measurement
	Si115xHandler(&sensI2C, &samples);
}

static int should_stop(void) {
  uint8_t bfr[1];
  return chnReadTimeout(&SD4, bfr, sizeof(bfr), 1);
}

void oproxCommand(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  int32_t result;
  int32_t mag;
  int32_t lux;
  uint8_t countsDisplayCounter = 0;	//When the counter reaches 10, the proximity value on screen updates
  int32_t countsDisplay = 0;			//These counts variables are used to control update of proximity senso    
  if (argc <= 0) {
    chprintf(chp, "Usage: oprox [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    init      initialize si1153"SHELL_NEWLINE_STR);
    chprintf(chp, "    prox      proximity readings"SHELL_NEWLINE_STR);
    chprintf(chp, "    als       ambient light readings"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "init")) {
    sensI2C.i2cPort = &I2CD1;
    sensI2C.i2cAddress = SI1153_I2C_ADDR;
    sensI2C.irqPort = PORTD;
    sensI2C.irqPin = 1;
    
    Si115xInitProxAls(&sensI2C, false);
    getSensorData(); // populate initial recrods

  } else if(!strcasecmp(argv[0], "prox")) {
    Si115xInitProxAls(&sensI2C, true);
    while( !should_stop() ) {
      getSensorData(); 
      result = (int32_t) samples.ch0;

      if(result < 0){
	result = 0;
      }
      
      if (result >= SENSOR_OVERFLOW_VALUE) {
	// Overflow occurred
	chprintf(stream, "PROX OVERFLOW\n\r");
	mag = 1; // Use 1 because we want to draw a very small square
      } else {
	//Update display every 10 readings
	if(countsDisplayCounter%10 == 0)
	  countsDisplay = (int32_t) result;
	// Display counts
	mag = (uint16_t) result;
	sprintf(ch0Str, "Mag: %d, Prox %d counts", mag, (int)countsDisplay);
      }
      countsDisplayCounter++;
    }
  } else if(!strcasecmp(argv[0], "als")) {
    Si115xInitProxAls(&sensI2C, false);
    while( !should_stop() ) {
      getSensorData(); 
      // Calculate lux values
      lux = (uint32_t)Si1153_getLuxReading(0, &samples);
      chprintf(stream, "Ambient light: %d lux", (int)lux);
    }
  } else {
    chprintf(chp, "Unrecognized command: %s"NL, argv[0]);
  }
  
}
orchard_shell("oprox", oproxCommand);


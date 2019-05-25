#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"

#include "analog.h"

#include <string.h>
#include <math.h>

//notes for this effect:
//need to set some reasonable temperature range that will work well for on-playa
//also need to offset the on-board temperature heating.. one option would be to get some default value at boot and subtract
//otherwise could also just figure it out from testing.
static void temperatureTestEffect(struct effects_config *config){
	uint8_t *fb = config->hwconfig->fb;
	int loop = config->loop;
	int count = config->count;
	//saturation percent for the temp colors (out of 100)
	int satLevel = 80;

	//set some floor/ceiling temperatures.
	const float maxTemp = 40;
	const float minTemp = 20;
	//temp is in milli deg C
	float temp;
	static float persistentTemp;

	if(loop%10==0){
		temp = (float)analogReadTemperature()/(float)1000;
		persistentTemp = temp;
		persistentTemp = persistentTemp < minTemp ? minTemp : persistentTemp;
		persistentTemp = persistentTemp > maxTemp ? maxTemp : persistentTemp;
		//chprintf(stream, "%s", "Current Temp: ");
		//chprintf(stream, "%f\n\r", persistentTemp);
	}

	//then convert to 0-255 for hsv color to make it purdy
	//I'm using float bc I don't like integer rounding.. let's just chop it off afterward.
	float tempHueFloat = ((persistentTemp-minTemp)/((maxTemp-minTemp))*255);
	int tempHue = (int)tempHueFloat;
	int tempSat = 255*satLevel/100;
	HsvColor tempHSV = {tempHue, tempSat, 255};

	//convert back to rgb and set the LED color
	ledSetAllRgbColor(fb, count, HsvToRgb(tempHSV), shift);
}
orchard_effects("temperatureTestEffect", temperatureTestEffect);


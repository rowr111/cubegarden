#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"

#include <string.h>
#include <math.h>

#ifndef MASTER_BADGE

#include "gyro.h"

static void accelEffect(struct effects_config *config) {
  //todo: it does a weird color switch at 0 degrees for some reason, fix this

  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop; 

  struct accel_data data;
  //let's get the xyz coordinates every so often..
  //things seem to get grumpy if you get the values too often :o
  if(loop % 3 == 0){ 
   gyro_Get_X_Axes(&data);
  }


  //let's make an angle from the xy coordinates
  float angle = atan2(data.x, data.y) * (180/3.14159) - 90;
  angle = angle < 0 ? 360 + angle : angle;  // Ensure positive angle
  //convert it into the hue 
	int angleHue = (int)((angle/360)*255);
  //do some slight smoothing:
  static int avg1;
  static int avg2;
  static int avg3;
  static int sum;
  sum = sum - avg1;
  avg1 = avg2;
  avg2 = avg3;
  avg3 = angleHue;
  sum = sum + avg3;
  int avgAngle = sum/3;

	//then convert to 0-255 for hsv color to make it purdy
	HsvColor angleHSV = {avgAngle, 255, 255};

	//convert back to rgb and set the LED color
	ledSetAllRgbColor(fb, count, HsvToRgb(angleHSV), shift);
}
orchard_effects("accel", accelEffect);

#else

static void accelEffect(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  //  int loop = config->loop; 

  int avgAngle = 0;

  //then convert to 0-255 for hsv color to make it purdy
  HsvColor angleHSV = {avgAngle, 255, 255};

  //convert back to rgb and set the LED color
  ledSetAllRgbColor(fb, count, HsvToRgb(angleHSV), shift);
  
}
orchard_effects("accel", accelEffect);

#endif

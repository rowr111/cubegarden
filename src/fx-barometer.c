#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"

#include <string.h>
#include <math.h>

#include "barometer.h"

static void barometerTestEffect(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  static float press;
  press = press == 0 ? baro_pressure : press; 

  static int on = 0;


 if(loop % 10 == 0){
   if (baro_pressure - press > 100){
     chprintf(stream, "%s", "PRESSURE INCREASE!!!");
     on = 1;
   }
   if (baro_pressure - press < -100){
      chprintf(stream, "%s", "PRESSURE DECREASE!!!");
      on = 0;
   }
   //chprintf(stream, "%s", "Current pressure: ");
	 //chprintf(stream, "%f\n\r", baro_pressure);
   //chprintf(stream, "%s", "Prev pressure: ");
	 //chprintf(stream, "%f\n\r", press);
   press = baro_pressure;
 }

  int currHue = loop%255;
  HsvColor currHSV = {currHue, 255, 255};
  RgbColor c = HsvToRgb(currHSV); 
  ledSetAllRGB(fb, count, (c.r*on), (c.g*on), (int)(c.b*on), shift);
}
orchard_effects("barometer", barometerTestEffect);


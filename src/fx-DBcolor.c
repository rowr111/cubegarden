#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"

#include <string.h>
#include <math.h>

#include "mic.h"
#include "analog.h"

extern uint8_t scopemode_g;
static void dbColorChangeAndIntensityEffect(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  // int loop = config->loop; // variable not referenced, comment out to clean up compile
  float level;

  scopemode_g = 2; // this selects db mode
  //let's assume some background and max decibel level 
  int bkgndDB = 30;
  int maxDB = 80;
  //there's no Math.max in C so we have to do something like this to limit the min/max
  //max:
  if(cur_db > maxDB) level = maxDB;
  //min:
  if(cur_db - bkgndDB < 1) level = (float)1;
  else level = (float)(cur_db - bkgndDB);

  level = (level/((float)maxDB-(float)bkgndDB));

  //now let's smooth this puppy out
  //with a very lamely implemented running avg of the last three level readings
  static float avg1;
  static float avg2;
  static float avg3;
  static float sum;
  sum = sum - avg1;
  avg1 = avg2;
  avg2 = avg3;
  avg3 = level;
  sum = sum + avg3;
  float avgLevel = sum/3;
  
  //let's iterate through a rainbow of colors to make it prettier
  //int currHue = loop%255;
  //or, we could also make the hue a function of the current level :o
  //let's do some subtraction to make the top be red
  int currHue = (int)(255-(255*avgLevel));
  HsvColor currHSV = {currHue, 255, 255};
  RgbColor c = HsvToRgb(currHSV);
  
  ledSetAllRGB(fb, count, (int)(c.r*avgLevel), (int)(c.g*avgLevel), (int)(c.b*avgLevel), (int)(shift*avgLevel));
}
orchard_effects("DBcolor", dbColorChangeAndIntensityEffect);


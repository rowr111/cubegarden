#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

#ifndef MASTER_BADGE

#include "analog.h"
#include "gyro.h"
#include "gfx.h"
#include "trigger.h"

/*effect description:
  try setting pulselength to a longer or shorter number.  if you don't see a difference try a bigger change!  like double or half.
  try setting h, the hue color, to any number between 0-255
*/

//this function runs over and over.  every time it runs, loop = loop + 1.
//time between executions is 
static void teacher(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop; 
  int pulselength = 100; //length of pulse 
  static uint16_t h = 151; // hue color

  
  //the following code determines brightness level for a pulse:
  //take modulus of loop counter (loop counter is always increasing), so we always get a value from 0 to pulselength-1
  int brightness = loop%pulselength;
  //this ternary aka conditional statement will make it actually pulse and go from 0 to pulselength/2 and then back to 0
  brightness = brightness > pulselength/2 ? pulselength - brightness : brightness;
  //now turn brightness into a percentage by dividing by our max of pulselength/2 
  float brightperc = (float)brightness/(pulselength/2);
  //small correction for aesthetics so that the cube doesn't get all the way dark
  brightperc = (float)(0.2 + brightperc*0.8); 


   //convert our one hue number to a 'hsv' color:
   //we use hsv format because it lets us set the color, saturation, and brightness separately.
   //hsv has three separate values:
        //hue == our color, max of 255
        //saturation == max of 255
        //value aka the brightness -> max is 255, so we will multiply our brightperc value by 255
   HsvColor currHSV = {h, 255, (int)255*brightperc};
   // convert the hue to rgb values, since this is what the rest of the code understands
   RgbColor c = HsvToRgb(currHSV); 
   // finally, set all the leds to this value
   ledSetAllRGB(fb, count, (c.r), (c.g), (c.b), shift);

}
orchard_effects("teacher", teacher, 0);

#else
static void teacher(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  // Each loop value lasts for 35 ms
  // 1 s is 1000/35 ~ 30 steps
  // 6 colors in the rainbow
  ledSetAllRgbColor(fb, count, vividRainbow[(loop / 30) % 6], shift);
}
orchard_effects("teacher", teacher, 0);
#endif

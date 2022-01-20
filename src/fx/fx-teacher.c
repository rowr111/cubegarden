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

*/

//I just picked some hue values that seemed ok.. (jeanie)
static const uint16_t SakuraLeaf[3] = {64, 85, 106};
static const uint16_t SakuraFlower = 242;

static void teacher(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop; 
  int pulselength = 100; //length of pulse 
  static uint16_t h = 242;

  //gentle pulse - setting brightness
  int brightness = loop%pulselength;
  brightness = brightness > pulselength/2 ? pulselength - brightness : brightness;
  float brightperc = (float)brightness/(pulselength/2);
  brightperc = (float)(0.2 + brightperc*0.8); //let's not let the pulse get all the way dark

   HsvColor currHSV = {h.h-(int)hueoffset, h.s, (int)h.v*brightperc};
   RgbColor c = HsvToRgb(currHSV); 
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

#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include <string.h>
#include <math.h>

#ifndef MASTER_BADGE

#include "address.h"

/*effect description:
    this effect is used for setup only.
    it will pick the offset for the desired configuration, and then pick a hue
    that corresponds to the correct color in the rainbow for that offset (red through purple)
    so that the cubes can be lined up in the correct order for effects that require 
    a distance placement.
*/
static void setupoffset(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;

  HsvColor h;

  uint8_t offset = getCubeLayoutOffset(cube_layout);
  uint8_t hue = 0;
  if(cube_layout == 1){
    hue = (int)((255/6)*offset);
  }
  // 2 == rows of 10 cubes
  else if(cube_layout == 2){
    hue = (int)((255/10)*offset);
  }
  // 3 == rows of 5 cubes
  else if(cube_layout == 3) {
    hue = (int)((255/5)*offset);
  }

  h.h = hue;
  h.s = baseHsvSaturation;
  h.v = baseHsvValue;
  RgbColor c = HsvToRgb(h); 

  ledSetAllRGB(fb, count, (c.r), (c.g), (c.b), shift);
  
}
orchard_effects("setupoffset", setupoffset, 0);

#else
static void setupoffset(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;

  ledSetAllRgbColor(fb, count, vividRainbow[0], shift);
}
orchard_effects("setupoffset", setupoffset, 10000000);
//giving this effect a very long duration so that it is not included in effect autoadvance
//the duration is not used on the master badge except for autoadvance.
#endif



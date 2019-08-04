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

#include "gyro.h"
#include "gfx.h"
#include "trigger.h"

static const RgbColor RED = {255, 0, 0}; // FIXME: Should we make these global to all fx?
static const RgbColor GREEN = {0, 255, 0}; // NOTE: They are in led.c
static const RgbColor BLUE = {0, 0, 255};
static const RgbColor YELLOW = {255, 255, 0};
static const RgbColor MAGENTA = {255, 0, 255};
static const RgbColor WHITE = {255, 255, 255};
static const int NUMSIDES = 6;

/*effect description - this is a game:
1. each cube starts as a random rubiks color (red, green, blue, yellow, white, magenta)
2. each cube will statically remain that color
  d. tilting a cube all the way to 90 degrees (or a little less) will cause the cube to change color
3. once the minimum number of cubes are the same color, master badge will trigger "you won!" effect
*/
static void rubiks(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  static int colorindex;
  static RgbColor c;

  if (patternChanged){
    patternChanged = 0;
    colorindex = (uint32_t)rand() % NUMSIDES; //get an initial color
    colorindex++; //need to be on a scale of 1-numOfBaseHsvColors
  }

  if(z_inclination > 75 && z_inclination < 105){
    RgbColor c;
    if (current_side == 0) {
      c = RED;
    } else if (current_side == 90) {
      c = GREEN;
    } else if (current_side == 180) {
      c = BLUE;
    } else {
      c = YELLOW;
    }
    ledSetAllRgbColor(fb, count, c, shift); // FIXME: set *all*?
  } else {
      if (z_inclination <= 75) {
          c = MAGENTA;
      } else {
          c = WHITE;
      }
  }
}
orchard_effects("rubiks", rubiks, 0);

// FIXME: How to tell that all cubes are the same color?

#else
static void rubiks(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  // FIXME: pick rubiks color for badge
  HsvColor c;
  c.h = 212; //pinkish.
  c.s = 255;
  int brightness = loop%100;
  brightness = brightness > 50 ? 100 - brightness : brightness;
  float brightperc = (float)brightness/50;
  c.v = (int) (255 * brightperc);
  RgbColor x = HsvToRgb(c);
  ledSetAllRGB(fb, count, x.r, x.g, x.b, shift);
}
orchard_effects("rubiks", rubiks);
#endif

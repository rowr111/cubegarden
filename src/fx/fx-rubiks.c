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

/* Track the number of each color to calculate win-state
 */
typedef struct ColorCounts {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t yellow;
  uint8_t magenta;
  uint8_t white;
} ColorCounts;

static ColorCounts COLOR_COUNTS = {0, 0, 0, 0, 0, 0};
static int ORIG_COLOR_INDEX = -1;
static int COLOR_INDEX_OFFSET = 0;

RgbColor getRubiksColor(uint8_t index_offset) {
  uint8_t index = (ORIG_COLOR_INDEX + index_offset) % NUMSIDES;
  if (index == 0) {
    return RED;
  }
  if (index == 1) {
    return GREEN;
  }
  if (index == 2) {
    return BLUE;
  }
  if (index == 3) {
    return YELLOW;
  }
  if (index == 4) {
    return MAGENTA;
  }
  if (index == 5) {
    return WHITE;
  }
}

/*effect description - this is a game:
  1. each cube starts as a random rubiks color (red, green, blue, yellow, white, magenta)
  2. each cube will statically remain that color
  d. tilting a cube all the way to 90 degrees (or a little less) will cause the cube to change color
  3. once the minimum number of cubes are the same color, master badge will trigger "you won!" effect
*/
static void rubiks(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;

  // On start of effect, set to random color in the Rubiks family
  if (patternChanged){
    patternChanged = 0;
    ORIG_COLOR_INDEX = (uint32_t)rand() % NUMSIDES;
    chprintf(stream, "got original random color...: %d\n\r\n", ORIG_COLOR_INDEX);
    ledSetAllRgbColor(fb, count, getRubiksColor(0), shift);
    return;
  }

  int prev_index_offset = COLOR_INDEX_OFFSET;

  if(z_inclination > 75 && z_inclination < 105){
    if (current_side == 0) {
      // FIXME just set off of index
      COLOR_INDEX_OFFSET = 0;
    } else if (current_side == 90) {
      COLOR_INDEX_OFFSET = 1;
    } else if (current_side == 180) {
      COLOR_INDEX_OFFSET = 2;
    } else {
      COLOR_INDEX_OFFSET = 3;
    }
  } else if (z_inclination <= 75) {
    COLOR_INDEX_OFFSET = 4;
  } else if (z_inclination >= 105) {
    COLOR_INDEX_OFFSET = 5;
  }

  // FIXME: Only set RGBs if color changed
  // FIXME: forward color value to master badge
  if (COLOR_INDEX_OFFSET != prev_index_offset) {
    ledSetAllRgbColor(fb, count, getRubiksColor(COLOR_INDEX_OFFSET), shift);
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

  // FIXME: keep track of how many of each color

  // FIXME: Trigger rainbow blast if enough cubes are the same color
  /* if(trigger_count == REQUIRED_CUBE_PARTICIPATION){ */
  /*   char idString[32]; */
  /*   chsnprintf(idString, sizeof(idString), "fx use rainbowblast"); */
  /*   radioAcquire(radioDriver); */
  /*   radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_forward, sizeof(idString), idString); */
  /*   radioRelease(radioDriver); */
  /* } */

}
orchard_effects("rubiks", rubiks);
#endif

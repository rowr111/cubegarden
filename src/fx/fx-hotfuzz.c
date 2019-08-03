#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "paging.h"
#include "chprintf.h"
#include "stdlib.h"

#include <string.h>
#include <math.h>

#ifndef MASTER_BADGE

static void hottFuzzPatternFB(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  
  uint16_t i;
  uint8_t oldshift = shift;
  static uint32_t  nexttime = 0;
  
  uint8_t divide_index = config->loop % count;
  
  for (i = 0; i < count; i++) {
    if ( i < divide_index  ) {
	ledSetRGB(fb, i, 255, 0, 0, shift);
    } else {
        ledSetRGB(fb, i, 0, 0, 255, shift);
    }
  }
  divide_index = (divide_index + 1) % count;
}
orchard_effects("hotfuz", hottFuzzPatternFB, 0);

#else

// TODO: Pick color/mode for master badge
static void hottFuzzPatternFB(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  
  uint16_t i;
  uint8_t oldshift = shift;
  static uint32_t  nexttime = 0;
  static uint8_t   hotfuzmode = 1;
  
  shift = 0;

  ledSetAllRGB(fb, count, 0, 200, 0, shift);

  shift = oldshift;
}
orchard_effects("hotfuz", hottFuzzPatternFB);

#endif

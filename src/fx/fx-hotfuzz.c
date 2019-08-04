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

static void hotFuzzPatternFB(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  
  uint16_t i;
  uint8_t oldshift = shift;
  static uint32_t  nexttime = 0;
  
  uint8_t blue_start = config->loop % count;
  uint8_t red_start = (blue_start + (count / 2)) % count;

  if (blue_start < red_start) {  
    for (i = 0; i < blue_start; i++) {
    	ledSetRGB(fb, i, 255, 0, 0, shift);
    }
    for (i; i < red_start; i++) {
      ledSetRGB(fb, i, 0, 0, 255, shift);
    }
    for (i; i < count; i++) {
      ledSetRGB(fb, i, 255, 0, 0, shift);
    }
  } else {
    for (i = 0; i < red_start; i++) {
      ledSetRGB(fb, i, 0, 0, 255, shift);
    }
    for (i; i < blue_start; i++) {
      ledSetRGB(fb, i, 255, 0, 0, shift);
    }
    for (i; i < count; i++) {
      ledSetRGB(fb, i, 0, 0, 255, shift);
    }
  }
}
orchard_effects("hotfuzz", hotFuzzPatternFB, 0);

#else

static void hotFuzzPatternFB(struct effects_config *config) {
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
orchard_effects("hotfuzz", hotFuzzPatternFB);

#endif

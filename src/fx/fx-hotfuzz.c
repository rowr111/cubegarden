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
  
  shift = 0;

  // TODO some sort of time shift
  // TODO: store  divide_index
  
  
  for (i = 0; i < count; i++) {
    if ( i < divide_index ||  ) {
	ledSetRGB(fb, i, 255, 0, 0, shift);
    } else {
        ledSetRGB(fb, i, 0, 0, 255, shift);
    }
  }
  divide_index = (divide_index + 1) % count;
}
orchard_effects("hotfuz", hottFuzzPatternFB, PAGE_DISPLAY_MS);

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

  if( hotfuzmode && (chVTGetSystemTime() > nexttime) ) {
    for( i = 0; i < count; i++ ) {
      if( (rand() % (unsigned int) count) < ((unsigned int) count / 3) )
	ledSetRGB(fb, i, 8, 8, 8, shift);
      else
	ledSetRGB(fb, i, 0, 0, 0, shift);
    }

    nexttime = chVTGetSystemTime() + 30 + (rand() % 25);
    hotfuzmode = 0;
  }

  else if( !hotfuzmode && (chVTGetSystemTime() > nexttime) ) {
    ledSetAllRGB(fb, count, 0, 0, 0, shift);
    
    nexttime = chVTGetSystemTime() + 30 + (rand() % 25);
    hotfuzmode = 1;
  }

  shift = oldshift;
}
orchard_effects("hotfuz", hottFuzzPatternFB);

#endif

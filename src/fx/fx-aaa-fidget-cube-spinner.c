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

#include "gyro.h"
#include "gfx.h"

static void fidgetCube(struct effects_config *config) {

  chprintf(stream, "Hello world!");

  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  static int pulsenum = 200; //number of times to pulse at this color
  static int pulselength = 100; //length of pulse

  uint8_t i;
  uint8_t oldshift = shift;

  // if( strobemode && (chVTGetSystemTime() > nexttime) ) {
  //   for( i = 0; i < count; i++ ) {
  //     if( (rand() % (unsigned int) count) < ((unsigned int) count / 3) )
	// ledSetRGB(fb, i, 255, 0, 255, shift);
  //     else
	// ledSetRGB(fb, i, 0, 0, 0, shift);
  //   }

  //   nexttime = chVTGetSystemTime() + 30 + (rand() % 25);
  //   strobemode = 0;
  // }

  // else if( !strobemode && (chVTGetSystemTime() > nexttime) ) {
  //   ledSetAllRGB(fb, count, 0, 0, 0, shift);

  //   nexttime = chVTGetSystemTime() + 30 + (rand() % 25);
  //   strobemode = 1;
  // }

  // shift = oldshift;
}

orchard_effects("fidgetCube", fidgetCube, PAGE_DISPLAY_MS);

#else


// The function below was copied over from strobe to be a place-holder (for now)
// Just in case if this file ends up being compiled on the MASTER BADGE (which we're not planning to)
static void fidgetCubeMaster(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;

  uint16_t i;
  uint8_t oldshift = shift;
  static uint32_t  nexttime = 0;
  static uint8_t   strobemode = 1;

  shift = 0;

  // // if( strobemode && (chVTGetSystemTime() > nexttime) ) {
  // //   for( i = 0; i < count; i++ ) {
  // //     if( (rand() % (unsigned int) count) < ((unsigned int) count / 3) )
	// // ledSetRGB(fb, i, 8, 8, 8, shift);
  // //     else
	// // ledSetRGB(fb, i, 0, 0, 0, shift);
  // //   }

  // //   nexttime = chVTGetSystemTime() + 30 + (rand() % 25);
  // //   strobemode = 0;
  // // }

  // // else if( !strobemode && (chVTGetSystemTime() > nexttime) ) {
  // //   ledSetAllRGB(fb, count, 0, 0, 0, shift);

  // //   nexttime = chVTGetSystemTime() + 30 + (rand() % 25);
  // //   strobemode = 1;
  // // }

  // shift = oldshift;
}
orchard_effects("fidgetCubeMaster", fidgetCubeMaster);

#endif
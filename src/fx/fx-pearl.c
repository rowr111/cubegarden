#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include "radio.h"

#include <string.h>
#include <math.h>

static void pearl(struct effects_config *config){
  static bool started = false;
  static RgbColor source = {0, 0, 0};
  static RgbColor destination;
  static int steps = 10;

  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  int step = loop % steps;

  if (!started || step == 0) {
    started = true;
    HsvColor hsv = {(uint8_t) (rand() % 256), 255, 255};
    destination = HsvToRgb(hsv);
  }

  source.r = source.r + (destination.r - source.r) / (steps - step);
  source.g = source.g + (destination.g - source.g) / (steps - step);
  source.b = source.b + (destination.b - source.b) / (steps - step);

  ledSetAllRgbColor(fb, count, source, shift);
}

#ifndef MASTER_BADGE
orchard_effects("pearl", pearl, 0);
#else
orchard_effects("pearl", pearl, 10000000);
#endif
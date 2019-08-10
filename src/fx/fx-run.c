#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include "radio.h"

#include <string.h>
#include <math.h>

#ifndef MASTER_BADGE
static void run(struct effects_config *config){
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  int step = loop % 80;
  int pos = step - radioAddress(radioDriver);

  if (pos >=0 && pos < 30) {
    ledSetAllRGB(fb, count, 255 - pos * 8, 255 - pos * 8, 255 - pos * 8, shift);
  } else {
    ledSetAllRGB(fb, count, 0, 0, 0, shift);
  }
}

orchard_effects("run", run, 0);
#else
static void run(struct effects_config *config){
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  int step = loop % count;

  ledSetAllRGB(fb, count, 0, 0, 0, shift);
  ledSetRGB(fb, step, 255, 255, 255, shift);
}

orchard_effects("run", run, 10000000);
#endif

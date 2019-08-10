#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include "radio.h"

#include <string.h>
#include <math.h>

static void run(struct effects_config *config){
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  int step = loop % 50;

  if (radioAddress(radioDriver) == step) {
    ledSetAllRGB(fb, count, 255, 255, 255, shift);
  } else {
    ledSetAllRGB(fb, count, 0, 0, 0, shift);
  }
}

orchard_effects("run", run, 0);

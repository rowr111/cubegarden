#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include "radio.h"

#include <string.h>
#include <math.h>

const RgbColor black = {0, 0, 0};
const RgbColor white = {255, 255, 255};


static void fx(struct effects_config *config){
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  int step = loop % 256;

  if (radioAddress(radioDriver) == step) {
    ledSetAllRgbColor(fb, count, white, shift);
  } else {
    ledSetAllRgbColor(fb, count, black, shift);
  }
}

orchard_effects("run", fx, 0);

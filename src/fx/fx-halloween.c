#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include "generic-color.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

#ifndef MASTER_BADGE

static void halloween(struct effects_config *config) {
    uint16_t halloweenColors[4][3] = {{255,154,0},{9,255,0},{201,0,255},{251,250,244}};
    genericColor(config, halloweenColors, 4);
}
orchard_effects("halloween", halloween, 0);

#else
static void halloween(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  // Each loop value lasts for 35 ms
  // 1 s is 1000/35 ~ 30 steps
  // 6 colors in the rainbow
  ledSetAllRgbColor(fb, count, vividRainbow[(loop / 30) % 6], shift);
}
orchard_effects("halloween", halloween, 0);
#endif
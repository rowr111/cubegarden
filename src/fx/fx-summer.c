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

static void summer(struct effects_config *config) {
    uint16_t summerColors[5][3] = {{255,89,143},{253,138,94},{224,227,0},{1,221,221},{0,191,175}};
    genericColor(config, summerColors, 5);
}
orchard_effects("summer", summer, 0);

#else
static void summer(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  // Each loop value lasts for 35 ms
  // 1 s is 1000/35 ~ 30 steps
  // 6 colors in the rainbow
  ledSetAllRgbColor(fb, count, vividRainbow[(loop / 30) % 6], shift);
}
orchard_effects("summer", summer, 0);
#endif
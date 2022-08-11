#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

#ifndef MASTER_BADGE
#include "generic-color.h"

static void neon(struct effects_config *config) {
    uint16_t neonColors[5][3] = {{201,255,0},{34,255,0},{0,236,255},{227,0,255},{255,0,129}};
    genericColor(config, neonColors, 5);
}
orchard_effects("neon", neon, 0);

#else
static void neon(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  // Each loop value lasts for 35 ms
  // 1 s is 1000/35 ~ 30 steps
  // 6 colors in the rainbow
  ledSetAllRgbColor(fb, count, vividRainbow[(loop / 30) % 6], shift);
}
orchard_effects("neon", neon, 0);
#endif
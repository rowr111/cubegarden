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

static void spring(struct effects_config *config) {
    uint16_t SpringColors[5][3] = {{255,180,0},{121,198,255},{254,146,255},{214,97,255},{7,223,0}};
    genericColor(config, SpringColors, 5);
}
orchard_effects("spring", spring, 0);

#else
static void spring(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  // Each loop value lasts for 35 ms
  // 1 s is 1000/35 ~ 30 steps
  // 6 colors in the rainbow
  ledSetAllRgbColor(fb, count, vividRainbow[(loop / 30) % 6], shift);
}
orchard_effects("spring", spring, 0);
#endif

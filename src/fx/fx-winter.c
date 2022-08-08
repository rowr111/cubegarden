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

static void winter(struct effects_config *config) {
    uint16_t winterColors[5][3] = {{206,255,247},{199,199,199},{174,155,255},{171,180,255},{140,164,199}};
    genericColor(config, winterColors, 5);
}
orchard_effects("winter", winter, 0);

#else
static void winter(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  // Each loop value lasts for 35 ms
  // 1 s is 1000/35 ~ 30 steps
  // 6 colors in the rainbow
  ledSetAllRgbColor(fb, count, vividRainbow[(loop / 30) % 6], shift);
}
orchard_effects("winter", winter, 0);
#endif
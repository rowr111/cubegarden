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

static void fall(struct effects_config *config) {
    uint16_t fallColors[5][3] = {{255,176,14},{239,105,8},{218,68,0},{162,2,0},{128,0,3}};
    genericColor(config, fallColors, 5);
}
orchard_effects("fall", fall, 0);

#else
static void fall(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  // Each loop value lasts for 35 ms
  // 1 s is 1000/35 ~ 30 steps
  // 6 colors in the rainbow
  ledSetAllRgbColor(fb, count, vividRainbow[(loop / 30) % 6], shift);
}
orchard_effects("fall", fall, 0);
#endif
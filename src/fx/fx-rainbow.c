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

static void rainbow(struct effects_config *config) {
    uint16_t RainbowColors[6][3] = {{255,0,0},{255,128,0},{240,255,0},{15,255,0},{0,70,206},{153,0,201}};
    genericColor(config, RainbowColors, 6);
}
orchard_effects("rainbow", rainbow, 0);

#else
static void rainbow(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  // Each loop value lasts for 35 ms
  // 1 s is 1000/35 ~ 30 steps
  // 6 colors in the rainbow
  ledSetAllRgbColor(fb, count, vividRainbow[(loop / 30) % 6], shift);
}
orchard_effects("rainbow", rainbow, 0);
#endif
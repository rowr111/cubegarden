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

#include "analog.h"
#include "gyro.h"
#include "gfx.h"
#include "trigger.h"

static void spring(struct effects_config *config) {
    uint16_t SakuraLeaf[3] = {64, 85, 106};
    genericColor(config, SakuraLeaf, 3);
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

#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"

#include <string.h>
#include <math.h>

#ifndef MASTER_BADGE
// Time sync test pattern
static void timesynctest(struct effects_config *config){
	uint8_t *fb = config->hwconfig->fb;
	int count = config->count;
  int loop = config->loop;

  // Each loop value lasts for 35 ms
  // 1 s is 1000/35 ~ 30 steps
  // 6 colors in the rainbow
  ledSetAllRgbColor(fb, count, vividRainbow[(loop / 30) % 6], shift);
}
orchard_effects("timesynctest", timesynctest, 0);

#else
// Time sync test pattern
static void timesynctest(struct effects_config *config){
	uint8_t *fb = config->hwconfig->fb;
	int count = config->count;
  int loop = config->loop;

  // Each loop value lasts for 35 ms
  // 1 s is 1000/35 ~ 30 steps
  // 6 colors in the rainbow
  ledSetAllRgbColor(fb, count, vividRainbow[(loop / 30) % 6], shift);
}
orchard_effects("timesynctest", timesynctest, 10000000);
//giving this effect a very long duration so that it is not included in effect autoadvance
//the duration is not used on the master badge except for autoadvance.
#endif

#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"

#include <string.h>
#include <math.h>

#ifndef MASTER_BADGE

static void lg4FB(struct effects_config *config) {
  do_lightgene(config);
}
orchard_effects("Lg4", lg4FB, 0);

#else
static void lg4FB(struct effects_config *config) {
  do_lightgene(config);
}
orchard_effects("Lg4", lg4FB);
#endif



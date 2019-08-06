#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"

#include <string.h>
#include <math.h>

#ifndef MASTER_BADGE
static void lg0FB(struct effects_config *config) {
  do_lightgene(config);
}
orchard_effects("Lg0", lg0FB, 0);

#else
static void lg0FB(struct effects_config *config) {
  do_lightgene(config);
}
orchard_effects("Lg0", lg0FB, 10000000);
//giving this effect a very long duration so that it is not included in effect autoadvance
//the duration is not used on the master badge except for autoadvance.
#endif


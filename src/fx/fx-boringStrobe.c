#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"

#include <string.h>
#include <math.h>

//just a boring blink
static void boringStrobePatternFB(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  static int white = 0;

  if(loop % 6 == 0){
    white = white == 0 ? 255 : 0;
    ledSetAllRGB(fb, count, white, white, white, shift);
  }
}
orchard_effects("boringStrobe", boringStrobePatternFB);


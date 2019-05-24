#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"

#include <string.h>
#include <math.h>

static void interactivePatternFB(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  static uint8_t tapfactor = 1;

  if(pressure_changed){
    pressure_changed = 0;
    chprintf(stream, "%s\n\r", "pressure changed, strobing!");
    effectsSetTempPattern(effectsNameLookup("strobe"), 1000);
  }

  if(bumped){
    bumped = 0;
    chprintf(stream, "%s\n\r", "bumped, strobing!");
    effectsSetTempPattern(effectsNameLookup("strobe"), 1000);
  }

  if(singletapped){
    singletapped = 0;
    tapfactor = tapfactor == 5 ? 1 : tapfactor + 1;
  }

  int currHue = (loop*tapfactor)%255;
  HsvColor currHSV = {currHue, 255, 255};
  RgbColor c = HsvToRgb(currHSV); 
  ledSetAllRGB(fb, count, (c.r), (c.g), (int)(c.b), shift);
}
orchard_effects("AAAinteractive", interactivePatternFB);

#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include <string.h>
#include <math.h>

#ifndef MASTER_BADGE
/*effect description: 
1. brief white swell to max brightness
2. 'explosion' effect in rainbow colors (each cube will flash randomly in random colors)
    (this code is 'borrowed' and modified slightly from the strobe effect)
3. fading out while continuing the explosion effect over a few seconds
*/
static void firework(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  static float fadein; //brightness 
  static float fadeout; 
  static RgbColor c;
 
  if(patternChanged) {
      patternChanged = 0;
      fadein = 0.01; 
      fadeout = 1;
  }
  
  static int x = 0;
  if(fadein <= 1){
      if(loop % 2 == 0){ //increase the brightness to white
          x = (int) (255 * fadein);
          fadein = fadein * 1.5;
      }
         ledSetAllRGB(fb, count, x, x, x, shift);
  }
  else{
      static uint32_t  nexttime = 0;
      static uint8_t   strobemode = 1;
      int bright;
      int hue;
      if(loop % 3 == 0) fadeout = fadeout * 0.95; // slowly fade out the color a bit
    
    if( strobemode && (chVTGetSystemTime() > nexttime) ) {
        if( (rand() % (unsigned int) count) < ((unsigned int) count / 3) ){
            hue = (uint32_t)rand() % 255;
            bright = (int)(255 * fadeout);
            HsvColor currHSV = {hue, 255, bright};
            c = HsvToRgb(currHSV);
            ledSetAllRGB(fb, count, c.r, c.g, c.b, shift);
        }
        else {
            ledSetAllRGB(fb, count, 0, 0, 0, shift);
        }
        nexttime = chVTGetSystemTime() + 30 + (rand() % 25);
        strobemode = 0;
    }

    else if( !strobemode && (chVTGetSystemTime() > nexttime) ) {
        ledSetAllRGB(fb, count, 0, 0, 0, shift);
        
        nexttime = chVTGetSystemTime() + 30 + (rand() % 25);
        strobemode = 1;
    }
  }
}
orchard_effects("firework", firework, 8000);

#else
static void firework(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  HsvColor c;
  c.h = 0; //red.
  c.s = 255;
  int brightness = loop%100;
  brightness = brightness > 50 ? 100 - brightness : brightness;
  float brightperc = (float)brightness/50;
  c.v = (int) (255 * brightperc);
  RgbColor x = HsvToRgb(c);
  ledSetAllRGB(fb, count, x.r, x.g, x.b, shift);
}
orchard_effects("firework", firework, 8000);
#endif



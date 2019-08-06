#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"

#include <string.h>
#include <math.h>

#ifndef MASTER_BADGE

#include "mic.h"
#include "analog.h"
#include "gyro.h"
#include "userconfig.h"

extern uint8_t scopemode_g;
const struct userconfig *uconfig;

static void dbColorChangeAndIntensityEffect(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop; 
  float level;
  RgbColor c;
  
  uconfig = getConfig();
  uint8_t dBbkgd = uconfig->cfg_dBbkgd;
  uint8_t dBmax = uconfig->cfg_dBmax;


  scopemode_g = 2; // this selects db mode

  //there's no Math.max in C so we have to do something like this to limit the min/max
  //max:
  if(cur_db > dBmax) level = (float)(dBmax - dBbkgd); 
  //min:
  if(cur_db - dBbkgd < 1) level = (float)1;
  else level = (float)(cur_db - dBbkgd);

  level = (level/((float)dBmax-(float)dBbkgd));

  //now let's smooth this puppy out
  static float avgs[2];
  avgs[loop%2] = level;
  float sum = 0;
  for(int i = 0; i<2; i++){
    sum +=avgs[i];
  }
  float avgLevel = sum/2;

  //if cube is tilted on its side, it will be fixed at a base color and not pulse
  if(z_inclination > 75 && z_inclination < 105){
    HsvColor h;
    if (current_side == 0) { //white
      h = color0;
    } else if (current_side == 90) { //cyan
      h = color90;
    } else if (current_side == 180) { //magenta
      h = color180;
    } else { //yellow
      h = color270;
    }
    c = HsvToRgb(h); 
    ledSetAllRGB(fb, count, (int)(c.r), (int)(c.g), (int)(c.b), shift);
  }
  else { //otherwise do the db effect!
    //let's iterate through a rainbow of colors to make it prettier
    int currHue = loop%255;
    HsvColor currHSV = {currHue, 255, 255};
    c = HsvToRgb(currHSV);
    //calculate final colors
    avgLevel = pow(avgLevel, 1.5);
    avgLevel = avgLevel < 0.05 ? 0.05 : avgLevel; //minimum value

    if( avgLevel > 0.9 ) { //it's sparkle time, baby!
    int i;
    for( i = 0; i < count; i++ ) {
      if( ((uint32_t)rand() % (unsigned int) count) < ((unsigned int) (count / 1.33) )) {
	      c.r = (int)(c.r*avgLevel);
        c.g = (int)(c.g*avgLevel);
        c.b = (int)(c.b*avgLevel);
        ledSetRGB(fb, i, c.r, c.g, c.b, shift);
      }
      else
	      ledSetRGB(fb, i, 0, 0, 0, shift);
      }
    }
    else{
      c.r = (int)(c.r*avgLevel);
      c.g = (int)(c.g*avgLevel);
      c.b = (int)(c.b*avgLevel);
      ledSetAllRGB(fb, count, (int)(c.r), (int)(c.g), (int)(c.b), shift);
    }
  }
}
orchard_effects("DBcolor", dbColorChangeAndIntensityEffect, 0);

#else
static void dbColorChangeAndIntensityEffect(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  HsvColor c;
  c.h = 191; //blueish.
  c.s = 255;
  int brightness = loop%100;
  brightness = brightness > 50 ? 100 - brightness : brightness;
  float brightperc = (float)brightness/50;
  c.v = (int) (255 * brightperc);
  RgbColor x = HsvToRgb(c);
  ledSetAllRGB(fb, count, x.r, x.g, x.b, shift); 
}
orchard_effects("DBcolor", dbColorChangeAndIntensityEffect, 0);
#endif

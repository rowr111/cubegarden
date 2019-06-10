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
  
  uconfig = getConfig();
  uint8_t dBbkgd = uconfig->cfg_dBbkgd;
  uint8_t dBmax = uconfig->cfg_dBmax;


  scopemode_g = 2; // this selects db mode

  //there's no Math.max in C so we have to do something like this to limit the min/max
  //max:
  if(cur_db > dBbkgd) level = dBmax;
  //min:
  if(cur_db - dBbkgd < 1) level = (float)1;
  else level = (float)(cur_db - dBbkgd);

  level = (level/((float)dBmax-(float)dBbkgd));
  //level = level < 0.1 ? 0.1 : level; //some minimum value for safety

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
    HsvColor c;
    if (current_side == 0) { //white
      c = color0;
    } else if (current_side == 90) { //cyan
      c = color90;
    } else if (current_side == 180) { //magenta
      c = color180;
    } else { //yellow
      c = color270;
    }
    RgbColor cc = HsvToRgb(c); 
    ledSetAllRGB(fb, count, (cc.r), (cc.g), (cc.b), shift);
  }
  else { //otherwise do the db effect!
    //let's iterate through a rainbow of colors to make it prettier
    int currHue = loop%255;
    HsvColor currHSV = {currHue, 255, 255};
    RgbColor c = HsvToRgb(currHSV);
    //calculate final colors
    avgLevel = pow(avgLevel, 2);
    avgLevel = avgLevel < 0.05 ? 0.05 : avgLevel;
    ledSetAllRGB(fb, count, (int)(c.r*avgLevel), (int)(c.g*avgLevel), (int)(c.b*avgLevel), shift);
  }
}
orchard_effects("DBcolor", dbColorChangeAndIntensityEffect, 0);

#else
static void dbColorChangeAndIntensityEffect(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;

  int avgLevel = 50;
  
  int currHue = (int)(255-(255*avgLevel));
  HsvColor currHSV = {currHue, 255, 255};
  RgbColor c = HsvToRgb(currHSV);
  
  ledSetAllRGB(fb, count, (int)(c.r*avgLevel), (int)(c.g*avgLevel), (int)(c.b*avgLevel), shift);
}
orchard_effects("DBcolor", dbColorChangeAndIntensityEffect);
#endif

#include "led.h"
#include "orchard-layers.h"
#include "ch.h"
#include "hal.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"

#include <string.h>
#include <math.h>

#include "mic.h"
#include "analog.h"
#include "gyro.h"
#include "userconfig.h"

extern uint8_t scopemode_g;
const struct userconfig *uconfig;

static void dBbrightness(struct effects_config *config){
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop; 
  float level;
  Color c;
  
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

  avgLevel = pow(avgLevel, 1.5);
  avgLevel = avgLevel < 0.05 ? 0.05 : avgLevel; //minimum value

  if( avgLevel > 0.9 ) { //it's sparkle time, baby!
    int i;
    for( i = 0; i < count; i++ ) {
      c = currentColors[i];
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
      int i;
      for( i = 0; i < count; i++ ) {
        c = currentColors[i];
        c.r = (int)(c.r*avgLevel);
        c.g = (int)(c.g*avgLevel);
        c.b = (int)(c.b*avgLevel);
        ledSetAllRGB(fb, count, (int)(c.r), (int)(c.g), (int)(c.b), shift);
      }
    }

}

orchard_layers("dBbrightness", dBbrightness);
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

  scopemode_g = 2; // this selects db mode

  if(loop%300 ==0){ //debugging data:
    chprintf(stream, "avg_high_db: %f.\n\r", avg_high_db);
    chprintf(stream, "avg_low_db: %f.\n\r", avg_low_db);
  }

  //there's no Math.max in C so we have to do something like this to limit the min/max
  //max:
  if(cur_db > avg_high_db) level = avg_high_db - avg_low_db; 
  //min:
  if(cur_db - avg_low_db < 1) level = (float)1;
  else level = cur_db - avg_low_db;

  level = level/(avg_high_db-avg_low_db); 

  //now let's smooth this puppy out
  static float avgs[2];
  avgs[loop%2] = level;
  float sum = 0;
  for(int i = 0; i<2; i++){
    sum +=avgs[i];
  }
  float avgLevel = sum/2;

  avgLevel = pow(avgLevel, 1.5);

  avgLevel = 0.2 + avgLevel >= 1 ? 1 : 0.2 + avgLevel; //minimum value of 0.2, give overall a nice bump tho

  int i;
  for( i = 0; i < count; i++ ) {
    c = currentColors[i];
    c.r = (int)(c.r*avgLevel);
    c.g = (int)(c.g*avgLevel);
    c.b = (int)(c.b*avgLevel);
    ledSetRGB(fb, i, (int)(c.r), (int)(c.g), (int)(c.b), shift);
  }

}

orchard_layers("dBbrightness", dBbrightness);
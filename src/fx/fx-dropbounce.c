#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include <string.h>
#include <math.h>

#ifndef MASTER_BADGE
static void dropbounce(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  //int id = getId(); //TODO - getID function
  static int id; // there will be 50 cubes.. for now we only have one so just pick a random number
  static int offset;
  static int on;

  float Gravity = -15; //gravity is more dramatic on this planet.. ;)
  int StartHeight = 1;
  static float Height;
  static float ImpactVelocity;
  static float TimeSinceLastBounce;
  static int ClockTimeSinceLastBounce;
  float Dampening = 0.80;

  if(patternChanged){
      patternChanged = 0;
      id = (uint32_t) rand() % 50 + 1; // this is just for now until we get a getId() function
      offset = getCubeLayoutOffset(cube_layout, id);
      if(offset == 0){
        on = 1;
        ClockTimeSinceLastBounce = chVTGetSystemTime();
        Height = StartHeight;
        ImpactVelocity = sqrt( -2 * Gravity * StartHeight );
        TimeSinceLastBounce = 0;
    } 
  }
  //waiting to start
  if(offset > 0){
    on = 0; //black to start
  } 
  if(loop % 3 == 0 && offset > 0){
    offset--;
     chprintf(stream, "offset: %d\n\r", offset);
     if(offset == 0){
        on = 1;
        ClockTimeSinceLastBounce = chVTGetSystemTime();
        Height = StartHeight;
        ImpactVelocity = sqrt( -2 * Gravity * StartHeight );
        TimeSinceLastBounce = 0;
    } 
  }

  if(offset == 0) { 
      TimeSinceLastBounce =  chVTGetSystemTime() - ClockTimeSinceLastBounce;
      Height = 0.5 * Gravity * pow( TimeSinceLastBounce/1000 , 2.0 ) + ImpactVelocity * TimeSinceLastBounce/1000;
      if ( Height < 0 ) {
          Height = 0;
          ImpactVelocity = Dampening * ImpactVelocity;
          ClockTimeSinceLastBounce = chVTGetSystemTime();
        }
    }
  ledSetAllRGB(fb, count, 255*Height*on, 255*Height*on, 255*Height*on, shift);
 
}
orchard_effects("dropbounce", dropbounce, 4400);
#else
static void dropbounce(struct effects_config *config) {
 uint8_t *fb = config->hwconfig->fb;
  int count = config->count;

  float Gravity = -15; //gravity is more dramatic on this planet.. ;)
  int StartHeight = 1;
  static float Height;
  static float ImpactVelocity;
  static float TimeSinceLastBounce;
  static int ClockTimeSinceLastBounce;
  float Dampening = 0.80;

  if(patternChanged){
    patternChanged = 0;
    ClockTimeSinceLastBounce = chVTGetSystemTime();
    Height = StartHeight;
    ImpactVelocity = sqrt( -2 * Gravity * StartHeight );
    TimeSinceLastBounce = 0;
    } 
    
    TimeSinceLastBounce =  chVTGetSystemTime() - ClockTimeSinceLastBounce;
    Height = 0.5 * Gravity * pow( TimeSinceLastBounce/1000 , 2.0 ) + ImpactVelocity * TimeSinceLastBounce/1000;
    if ( Height < 0 ) {
        Height = 0;
        ImpactVelocity = Dampening * ImpactVelocity;
        ClockTimeSinceLastBounce = chVTGetSystemTime();
    }
  ledSetAllRGB(fb, count, 255*Height, 255*Height, 255*Height, shift);
}
orchard_effects("dropbounce", dropbounce);
#endif
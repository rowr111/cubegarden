#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "baton.h"

extern uint8_t baton_holder_g;

#ifndef MASTER_BADGE

#include "gyro.h"
#include "gfx.h"
#include "trigger.h"
#include "radio.h"

/*effect description:
1. one cube is the 'master' cube whose actions control all the other cubes.
2. the master cube is chosen by the masterbadge at the beginning
3. at the start of being chosen, the new mastercube will flash brightly five times
4. actions performed on the mastercube will then cause a broadcast out to other cubes
5. other cubes will now be dependent on the mastercube for their color
6. after 2 minutes, a new master cube will be selected by the current master cube
7. master cube behavior will be the same as confettipulse (minus barometer).
*/
static void LOTC(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  static int pulsenum = 200; //number of times to pulse at this color
  int pulselength = 100; //length of pulse 
  uint32_t batonholdmaxtime = 120000; //ms to hold the baton (2min)
  static uint32_t batonholdnexttime = 0; //when to pass the baton
  uint32_t fadeouttime = 3000; //ms to fade out the baton holding cube 
  BatonState *bstate;
  bstate = getBatonState();
  bstate->fx_uses_baton = 1; // let the baton state machine know we are handling batons
  
  static uint8_t flashcount = 5; //number of times to flash at the beginning of the baton 
  uint8_t flashspeed = 5; //how many loops to flash
  static RgbColor fc;
  
  static int colorindex;
  static HsvColor h;  

  if (bstate->state != baton_not_holding){

  if (patternChanged){
    patternChanged = 0;
    colorindex = (uint32_t)rand() % numOfBaseHsvColors; //get an initial color
    colorindex++; //need to be on a scale of 1-numOfBaseHsvColors
  }

  if(loop % (pulselength*pulsenum) == 0 ) {
    colorindex = (uint32_t)rand() % numOfBaseHsvColors; //change color randomly every pulsenum pulses
    colorindex++; //need to be on a scale of 1-numOfBaseHsvColors
  }

  //tap will advance the color by tapadvance amount
  if(singletapped){
    singletapped = 0;
    colorindex++;
    if (colorindex > numOfBaseHsvColors) colorindex = colorindex % numOfBaseHsvColors; //wrap around
  }


  h = getBaseHsvColor(colorindex);
  float hueoffset = (float)(h.h * (z_inclination%90)/90); //change hue on tilt

  //gentle pulse - setting brightness
  int brightness = loop%pulselength;
  brightness = brightness > pulselength/2 ? pulselength - brightness : brightness;
  float brightperc = (float)brightness/(pulselength/2);
  brightperc = (float)(0.2 + brightperc*0.8); //let's not let the pulse get all the way dark

  //if cube is tilted all the way on its side, it will be fixed at a base color and not pulse
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

    //flash flashcount times at beginning of baton holding
    if ((bstate->state == baton_holding) && flashcount > 0){
      if(loop % flashspeed == 0){
        if(fc.r == 0){
          fc.r = 255;
          fc.g = 255;
          fc.b = 255;
        }
        else{
          fc.r = 0;
          fc.g = 0;
          fc.b = 0;
        }
        flashcount--;
      }
      ledSetAllRGB(fb, count, (fc.r), (fc.g), (fc.b), shift);
    }
    else
    {
      if ((bstate->state == baton_holding) && (chVTGetSystemTime() > (batonholdnexttime - fadeouttime)))//if we're about to hand off the baton, fade out this particular cube
      {
        float perc = (float)(((float)batonholdnexttime - (float)chVTGetSystemTime())/(float)fadeouttime);
        if(loop%10==0){
          chprintf(stream, "%f\n\r", perc);
          chprintf(stream, "batonholdnexttime: %d\n\r", batonholdnexttime);
          chprintf(stream, "currentime: %d\n\r", chVTGetSystemTime());
          chprintf(stream, "fadeouttime: %d\n\r", fadeouttime);
        }
        cc.r = (int)(cc.r * perc);
        cc.g = (int)(cc.g * perc);
        cc.b = (int)(cc.b * perc);
      }
      ledSetAllRGB(fb, count, (cc.r), (cc.g), (cc.b), shift);
    }
    if(loop%2 == 0){ //very slightly less spammy..
      char idString[32];
      chsnprintf(idString, sizeof(idString), "superrgb %d %d %d", cc.r, cc.g, cc.b);
      radioAcquire(radioDriver);
      radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_forward, sizeof(idString), idString);
      radioRelease(radioDriver);
    }
  }
  //
  else { //otherwise, pulse at the expected color
    HsvColor currHSV = {h.h-(int)hueoffset, h.s, (int)h.v*brightperc};
    RgbColor c = HsvToRgb(currHSV); 
     //flash flashcount times at beginning of baton holding
    if ((bstate->state == baton_holding) && flashcount > 0){
      if(loop % flashspeed == 0){
        if(fc.r == 0){
          fc.r = 255;
          fc.g = 255;
          fc.b = 255;
          flashcount--;
        }
        else{
          fc.r = 0;
          fc.g = 0;
          fc.b = 0;
        }
      }
      ledSetAllRGB(fb, count, (fc.r), (fc.g), (fc.b), shift);
    }
    else{
      if ((bstate->state == baton_holding) && (chVTGetSystemTime() > (batonholdnexttime - fadeouttime)))//if we're about to hand off the baton, fade out this particular cube
      {
        float perc = (float)(((float)batonholdnexttime - (float)chVTGetSystemTime())/(float)fadeouttime);
        if(loop%10==0){
          chprintf(stream, "%f\n\r", perc);
          chprintf(stream, "batonholdnexttime: %d\n\r", batonholdnexttime);
          chprintf(stream, "currentime: %d\n\r", chVTGetSystemTime());
          chprintf(stream, "fadeouttime: %d\n\r", fadeouttime);
        }
        c.r = (int)(c.r * perc);
        c.g = (int)(c.g * perc);
        c.b = (int)(c.b * perc);
      }
      ledSetAllRGB(fb, count, (c.r), (c.g), (c.b), shift);
    }
    if(loop%2 == 0){ //very slightly less spammy..
      char idString[32];
      chsnprintf(idString, sizeof(idString), "superrgb %d %d %d", c.r, c.g, c.b);
      radioAcquire(radioDriver);
      radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_forward, sizeof(idString), idString);
      radioRelease(radioDriver);
    }
  }
  }
  else
  {
       ledSetAllRGB(fb, count, (superRgb.r), (superRgb.g), (superRgb.b), shift);
  }
  //pass the baton if we're out of time
  if (bstate->state == baton_holding){
    if(batonholdnexttime == 0){ //if == 0 get start time
       batonholdnexttime = chVTGetSystemTime() + batonholdmaxtime;
       flashcount = 5; //reset flashcount
    }
    if(chVTGetSystemTime() > batonholdnexttime){ //if out of time, pass baton
      chprintf(stream, "reached max time of %d, passing baton.\n\r", batonholdmaxtime);
      bstate->fx++;
      passBaton(baton_random, 0, 500);
      batonholdnexttime = 0; //reset baton holding time
    }
  }
}
orchard_effects("LOTC", LOTC, 0);

#else
extern uint32_t last_ping_g;
static void LOTC(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  BatonState *bstate;
  bstate = getBatonState();

  HsvColor c;
  c.h = 212; //pinkish.
  c.s = 255;
  int brightness = loop%100;
  brightness = brightness > 50 ? 100 - brightness : brightness;
  float brightperc = (float)brightness/50;
  c.v = (int) (255 * brightperc);
  RgbColor x = HsvToRgb(c);
  ledSetAllRGB(fb, count, x.r, x.g, x.b, shift); 
  //if no one is holding the baton, pass it randomly to kick things off
  if( chVTTimeElapsedSinceX(last_ping_g) > 30000 ) {   // 30 seconds no baton, pass a new one out
    chprintf(stream, "no baton holder, trying to pass to random\n\r");
    baton_new_random();
  } 
}
orchard_effects("LOTC", LOTC);
#endif



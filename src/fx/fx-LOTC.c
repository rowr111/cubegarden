#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

#ifndef MASTER_BADGE

#include "gyro.h"
#include "gfx.h"
#include "trigger.h"
#include "radio.h"

/*effect description:
1. one cube is the 'master' cube whose actions control all the other cubes.
2. the master cube is chosen by the masterbadge
3. at the start of being chosen, the new mastercube will flash brightly three times
4. actions performed on the mastercube will then cause a broadcast out to other cubes
5. other cubes will now be dependent on the mastercube for their color
6. after 1 (?) minute or so, a new master cube will be selected
7. master cube behavior will be the same as confettipulse.
*/
static void LOTC(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  static int pulsenum = 200; //number of times to pulse at this color
  int pulselength = 100; //length of pulse 
  
  static int colorindex;
  static HsvColor h;  
  static bool mastercube = false;

  if (mastercube){

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
    ledSetAllRGB(fb, count, (cc.r), (cc.g), (cc.b), shift);

    char idString[32];
    chsnprintf(idString, sizeof(idString), "superrgb %d %d %d", cc.r, cc.g, cc.b);
    radioAcquire(radioDriver);
    radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_forward, sizeof(idString), idString);
    radioRelease(radioDriver);
  }
  //
  else { //otherwise, pulse at the expected color
    HsvColor currHSV = {h.h-(int)hueoffset, h.s, (int)h.v*brightperc};
    RgbColor c = HsvToRgb(currHSV); 
    ledSetAllRGB(fb, count, (c.r), (c.g), (c.b), shift);
    char idString[32];
    chsnprintf(idString, sizeof(idString), "superrgb %d %d %d", c.r, c.g, c.b);
    radioAcquire(radioDriver);
    radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_forward, sizeof(idString), idString);
    radioRelease(radioDriver);
  }
  }
  else
  {
       ledSetAllRGB(fb, count, (superRgb.r), (superRgb.g), (superRgb.b), shift);
  }
}
orchard_effects("LOTC", LOTC, 0);

#else
static void LOTC(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  HsvColor c;
  c.h = 212; //pinkish.
  c.s = 255;
  int brightness = loop%100;
  brightness = brightness > 50 ? 100 - brightness : brightness;
  float brightperc = (float)brightness/50;
  c.v = (int) (255 * brightperc);
  RgbColor x = HsvToRgb(c);
  ledSetAllRGB(fb, count, x.r, x.g, x.b, shift); 
}
orchard_effects("LOTC", LOTC);
#endif



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

#include "analog.h"
#include "gyro.h"
#include "gfx.h"
#include "trigger.h"

/*effect description:
cubes randomly pick if they are a leaf or a sakura (starting mix is 75/25)
leaves randomly pick a green hue and pulse at that hue
sakura do a pink and white spin
upon single tap, switch who you are
*/

//I just picked some hue values that seemed ok.. (jeanie)
static const uint16_t SakuraLeaf[3] = {64, 85, 106};
static const uint16_t SakuraFlower = 232;

static void sakura(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop; 
  int pulselength = 100; //length of pulse 
  static uint16_t hue = 0;
  static bool sakura = false; //am I a sakura flower or not?

  if(patternChanged){
      patternChanged = 0;
      //pick who I am and pick the leaf color
      if((uint32_t)rand() % 4 == 2){
          sakura = true; //25% will be sakura to start
      }
      else{
          sakura = false;
          hue = SakuraLeaf[(uint32_t)rand() % 3];
      } 
      chprintf(stream, "starting hue: %d\n\r", hue);
  }
  //if tapped, change who you are
  if(singletapped){
    singletapped = 0;
    sakura = !sakura;
    if(sakura){
        chprintf(stream, "changed to sakura\n\r");
    }
    else{
        chprintf(stream, "changed to leaf\n\r");
    }
  }

  //set the colors for both
  if(sakura){ 
    //do spin
    uint8_t pinkoffset = (loop/2) % count;
    RgbColor white = {255, 255, 255}; //white as default
    for(int i = 0; i<count; i++){
        if (i<count/2){
            HsvColor sakurahsv = {SakuraFlower, 255, 255};
            RgbColor sakurargb = HsvToRgb(sakurahsv); 
            ledSetRgbColor(fb, (i+pinkoffset)%count, sakurargb, shift);
        }
        else{
            ledSetRgbColor(fb, (i+pinkoffset)%count, white, shift); 
        }
    }
    for(int i = count/2; i<count; i++){
       
    }
  }
  else{ //Im a leaf!
    //pulse at leaf color
    if(hue == 0){
        //get the color if for some reason it hasn't been gotten
        hue = SakuraLeaf[(uint32_t)rand() % 3];
    }
    //gentle pulse - setting brightness
    int brightness = loop%pulselength;
    brightness = brightness > pulselength/2 ? pulselength - brightness : brightness;
    float brightperc = (float)brightness/(pulselength/2);
    brightperc = (float)(0.2 + brightperc*0.8); //let's not let the pulse get all the way dark
    HsvColor currHSV = {hue, 255, (int)255*brightperc};
    RgbColor c = HsvToRgb(currHSV); 
    ledSetAllRGB(fb, count, (c.r), (c.g), (c.b), shift);
  }
}
orchard_effects("AAAsakura", sakura, 0);

#else
static void sakura(struct effects_config *config) {
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
orchard_effects("sakura", sakura, 0);
#endif

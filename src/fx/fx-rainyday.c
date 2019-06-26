#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"

#include <string.h>
#include <math.h>

#ifndef MASTER_BADGE

#include "analog.h"

/*effect description:
This effect iterates through a rainstorm and then sun comes out, sets again, and back to the beginning.
The whole thing should take about 3min?
1. the cubes are a seething sea of blues and purples (1min)
2. lightning is flashing randomly amongst the cubes
3. lighting stops and the colors calm and start to lighten to a light blue (15sec)
4. the colors shift slowly up from pastel pinks, reds, oranges, and then a bright pulsing yellow (30sec)
5. the sun! there are some clouds here and there.. (30sec)
6. the colors start shift slowly back down to the blues and purples, and lightning starts again (45sec)
*/
static void rainyday(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop; 
  
  HsvColor c;
  static uint8_t hue;
  static uint8_t sat;
  static uint8_t brt;
  uint16_t totallength = 3000; //total of number of loops for the pattern
  uint16_t current = loop % totallength;
  static uint32_t lightningtime = 0;
  static uint8_t lightningcount = 3;
  static bool dolightning = false;

  if(current % 10 == 0){
      //chprintf(stream, "current: %d\n\r", current);
      //chprintf(stream, "sat: %d\n\r", sat);
  }

  if(current < (totallength/3)){
      if(current == 0 || lightningtime == 0){
        lightningtime = (uint32_t)rand() % (totallength/4); 
        //chprintf(stream, "lightningtime: %d\n\r", lightningtime);
      } 
      if(current % lightningtime == 0){
          dolightning = true;
          lightningcount = (uint32_t)rand() % 5 + 1;
      }
      //do rain
      //150-193 - blue to magenta-ish 
      //iterating over 43 (86 to go back and forth)
      hue = current % 86;
      hue = hue >= 43 ? 86 - hue : hue;
      hue = 150 + hue;
      sat = 255;
      brt = 255;
  }
  else if(current >= (totallength/3) && current < (totallength*5/12)){
      //lighting stops, colors lightening to light blue
      //destination color == 180
      //destination sat == 180
      int lengthdiff = (totallength*5/12) - (totallength/3);
      int colordiff = abs(hue - 180);
      int colorincfreq = lengthdiff/colordiff - 1;
      int satdiff =  abs(sat - 180);
      int satdecfreq = lengthdiff/satdiff - 1;
      if(current % colorincfreq == 0 && hue != 180){
          if(hue < 180) hue++;
          else hue--;
      }
      if(current % satdecfreq == 0 && sat != 128){
          sat--;
      }
      brt = 255;
  }
  else if(current >= (totallength*5/12) && current < (totallength*7/12)){
      //pastel pinks, reds, oranges, and then a bright pulsing yellow
      //destination color == 43
      //destination sat == 255
      int lengthdiff = (totallength*7/12) - (totallength*5/12);
      int colordiff = 255 - 180 + 43;
      int colorincfreq = lengthdiff/colordiff - 1;
      int satdiff =  255 - 128;
      int satdecfreq = lengthdiff/satdiff - 1;
      if(current % colorincfreq == 0 && hue != 43){
          if(hue == 255){
            hue = 0;
          } 
          else hue++;
      }
      if(current % satdecfreq == 0 && sat != 255){
          sat++;
      }
      brt = 255;
  }
  else if(current >= (totallength*7/12) && current < (totallength*3/4)){
      dolightning = false; //make sure lightning is stopped
      //the sun! there are some clouds here and there..
      //target sat == 200
      static uint8_t direction;
      if(direction == 0){
          if(sat <= 201) direction = 1;
          sat = sat - 2;
      }
      if(direction == 1){
          if(sat >= 253) direction = 0;
          sat = sat + 2;
      }
      brt = 255;
  }
  else if(current > (totallength*3/4)){
      //shift back down to purples
      //target hue == 150
      //target sat == 255
      int lengthdiff = totallength - (totallength*3/4);
      int colordiff = 255 - 150 + 43;
      int colorincfreq = lengthdiff/colordiff - 1;
      int satdiff =  255 - sat;
      int satdecfreq = lengthdiff/satdiff - 1;
      if(current % colorincfreq == 0 && hue != 180){
          if(hue == 0){
            hue = 255;
          } 
          else hue--;
      }
      if(current % satdecfreq == 0 && sat != 255){
          sat++;
      }
      brt = 255;
  }
  if(dolightning == true && current < ((totallength/3)-1)){ //don't 
    static uint32_t  nexttime = 0;
    static uint8_t   strobemode = 1;

    if( strobemode && (chVTGetSystemTime() > nexttime) ) {
        sat = 0;
        brt = 255;
        nexttime = chVTGetSystemTime() + 30 + (rand() % 25);
        strobemode = 0;
        lightningcount--;
    }
    else if( !strobemode && (chVTGetSystemTime() > nexttime) ) {
        brt = 0;
        nexttime = chVTGetSystemTime() + 30 + (rand() % 25);
        strobemode = 1;
    }
    if(lightningcount == 0){
        //lightningtime = (uint32_t)rand() % (totallength/6);
        dolightning = false;
    } 
  }
  c.h = hue;
  c.s = sat;
  c.v = brt;
  RgbColor x = HsvToRgb(c);
  ledSetAllRGB(fb, count, x.r, x.g, x.b, shift); 
}
orchard_effects("rainyday", rainyday, 0);

#else
static void rainyday(struct effects_config *config) {
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
orchard_effects("rainyday", rainyday);
#endif

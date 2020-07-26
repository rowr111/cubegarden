#include "led.h"
#include "orchard-layers.h"
#include "ch.h"
#include "hal.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"

#include "analog.h"
#include "userconfig.h"

static void pulse(struct effects_config *config){
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop; 
  int pulselength = 100; //length of pulse 
  Color c;

  //gentle pulse - setting brightness
  int brightness = loop%pulselength;	
  brightness = brightness > pulselength/2 ? pulselength - brightness : brightness;	
  float brightperc = (float)brightness/(pulselength/2);	
  brightperc = (float)(0.2 + brightperc*0.8); //let's not let the pulse get all the way dark	
  if(loop%30 == 0){
    chprintf(stream, "current brightperc: %f.\n\r", brightperc);
  }
  int i;
  for( i = 0; i < count; i++ ) {
    c = currentColors[i];
    c.r = (int)(c.r*brightperc);
    c.g = (int)(c.g*brightperc);
    c.b = (int)(c.b*brightperc);
    ledSetRGB(fb, i, (int)(c.r), (int)(c.g), (int)(c.b), shift);
  }
}

orchard_layers("pulse", pulse);
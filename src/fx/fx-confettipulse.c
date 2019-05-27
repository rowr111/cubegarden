#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include "gyro.h"

#include <string.h>
#include <math.h>

/*general plan:
1. each cube picks a random color
2. each cube will pulse light and dark on that color while offering interactivity
  a. tapping will change the color
  b. tilting will change hue
  c. sitting or lifting will induce a temporary color  //todo
  d. tilting all the way to 90 deg will cause the cube to be fixed on a particular color, depending on the side. 
3. each cube will change its base color every so often, also a bit random but will do color for a 
    min amount of time.
*/
static void confettipulse(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  static int pulsenum = 200; //number of times to pulse at this color
  int pulselength = 100; //length of pulse 
  static int colorindex;
  static HsvColor h;  
  static bool started = false;

  if (!started){
      colorindex = rand() % numOfBaseHsvColors; //get an initial color
      started = true;
  }

  if(loop % (pulselength*pulsenum) == 0 ) {
      colorindex = rand() % numOfBaseHsvColors; //change color randomly every pulsenum pulses
  }

  //tap will advance the color by tapadvance amount
  if(singletapped){
    singletapped = 0;
    colorindex++;
  }

  h = getBaseHsvColor(colorindex);
  float hueoffset = (float)(h.h * (z_inclination%90)/90); //change hue on tilt

  //gentle pulse - setting brightness
  int brightness = loop%pulselength;
  brightness = brightness > pulselength/2 ? pulselength - brightness : brightness;
  float brightperc = (float)brightness/(pulselength/2);
  brightperc = (float)(0.2 + brightperc*0.8); //let's not let the pulse get all the way dark

  //if cube is tilted on its side, it will be fixed at a base color and not pulse
  if(z_inclination > 75 && z_inclination < 105){
    RgbColor c;
    if (current_side == 0) { //white
      c.r = 255;
      c.g = 255;
      c.b = 255;
    } else if (current_side == 90) { //cyan
      c.r = 0;
      c.g = 255;
      c.b = 255;
    } else if (current_side == 180) { //magenta
      c.r = 255;
      c.g = 0;
      c.b = 255;
    } else { //yellow
      c.r = 255;
      c.g = 255;
      c.b = 0;
    }
    ledSetAllRGB(fb, count, (c.r), (c.g), (c.b), shift);
  }
  else {
    HsvColor currHSV = {h.h-(int)hueoffset, h.s, (int)h.v*brightperc};
    RgbColor c = HsvToRgb(currHSV); 
    ledSetAllRGB(fb, count, (c.r), (c.g), (c.b), shift);
  }
}
orchard_effects("confettipulse", confettipulse);
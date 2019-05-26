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
3. each cube will change its base color every so often, also a bit random but will do color for a 
    min amount of time.
*/
static void confettipulse(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  static int pulsenum = 200; //number of times to pulse at this color
  int pulselength = 100; //length of pulse 
  int tapadvance = 30; //how far to advance the color upon singletap
  static int hue; 
  static bool started = false;

  if (!started){
      hue = rand() % 255; //get an initial color
      pulsenum = (rand() % 10) + 10; //pulse at least 10 times, but up to 20
      started = true;
  }

  if(loop % (pulselength*pulsenum) == 0 ) {
      hue = rand() % 255; //change color randomly every pulsenum pulses
  }

  //tap will advance the color by tapadvance amount
  if(singletapped){
    singletapped = 0;
    hue = hue + tapadvance;
    hue = hue % 255;
  }

  float hueoffset = (float)(hue * (z_inclination%90)/90); //change hue on tilt

  //gentle pulse - setting brightness
  int brightness = loop%pulselength;
  brightness = brightness > pulselength/2 ? pulselength - brightness : brightness;
  float brightperc = (float)brightness/(pulselength/2);
  brightperc = (float)(0.2 + brightperc*0.8); //let's not let the pulse get all the way dark

  HsvColor currHSV = {hue-(int)hueoffset, 255, (int)255*brightperc};
  RgbColor c = HsvToRgb(currHSV); 
  ledSetAllRGB(fb, count, (c.r), (c.g), (int)(c.b), shift);
}
orchard_effects("confettipulse", confettipulse);

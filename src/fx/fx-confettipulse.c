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
  int pulselength = 100; //length of pulse in loops
  static int pulsenum; //number of times to pulse at this color
  int tapadvance = 25; //how far to advance the color upon singletap
  static int hue; 
  static bool started = false;

  if (!started){
      hue = rand() % 255; //get an initial color
      pulsenum = (rand() % 10) + 10; //pulse at least 10 times, but up to 20
      started = true;
  }

  static float hueoffset;

  //let's get the xyz coordinates every so often..
  //things seem to get grumpy if you get the values too often :o
  if(loop % 3 == 0){ 
   struct accel_data data;
   gyro_Get_X_Axes(&data);
   double norm_Of_g = sqrt(data.x * data.x + data.y * data.y + data.z * data.z);
   int inclination = (int) (acos(data.z / norm_Of_g)*(180/3.14159));
   //calculate hue offset due to tilting - I only care about turning it on its side
   inclination = inclination%90;
   hueoffset = (float)hue * ((float)inclination/90); 
  }

  if(loop % (pulselength*pulsenum) == 0 ) {
      hue = rand() % 255; //change color randomly every pulsenum pulses
      pulsenum = (rand() % 10) + 10; //update how many pulses to do for next time
  }

  //tap will advance the color by tapadvance amount
  if(singletapped){
    singletapped = 0;
    hue = hue + tapadvance;
    hue = hue % 255;
  }

  //gentle pulse - setting brightness
  int brightness = loop%pulselength;
  brightness = brightness > pulselength/2 ? pulselength - brightness : brightness;
  float brightperc = (float)brightness/(pulselength/2);

  HsvColor currHSV = {hue-(int)hueoffset, 255, (int)255*brightperc};
  RgbColor c = HsvToRgb(currHSV); 
  ledSetAllRGB(fb, count, (c.r), (c.g), (int)(c.b), shift);
}
orchard_effects("confettipulse", confettipulse);

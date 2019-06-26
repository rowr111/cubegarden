#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include <string.h>
#include <math.h>

#ifndef MASTER_BADGE
/*effect description: a rainbow radiating out 
1. cubes will get their starting positions (divided into 6 groups)
2. cubes will start and end (briefly) as dark
3. a rainbow will iterate outward 5 times and end.
*/
static void rainbowblast(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  int rainbowtimes = 5; //how many times to iterate through the rainbow
  static int rainbowcount;
  static int rainbowcolor; 
  static int offset;
  static RgbColor c;
  RgbColor black = {0,0,0};
 
  if(patternChanged) {
      patternChanged = 0;
      offset = getCubeLayoutOffset(cube_layout);
      rainbowcount = 0;
      rainbowcolor = 0;
  }
  //waiting to start
  if(offset > 0){
    c = black; //black to start
  }

  if(loop % 3 == 0 && offset > 0){
    offset--;
  }
  else if(loop % 3 == 0 && rainbowcount < rainbowtimes){
    c = vividRainbow[(rainbowcolor % 6)];
    if(rainbowcolor % 6 == 0){
        rainbowcount++;
    }
    rainbowcolor++;
    if(rainbowcolor > 5) rainbowcolor = 0;
  } else if (rainbowcount >= rainbowtimes) {
    c = black; //black after everything is done.
  }
   ledSetAllRGB(fb, count, (c.r), (c.g), (c.b), shift);
  
}
orchard_effects("rainbowblast", rainbowblast, 4000);

#else
static void rainbowblast(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  ledSetAllRgbColor(fb, count, vividRainbow[(loop / 30) % 6], shift);
}
orchard_effects("rainbowblast", rainbowblast);
#endif



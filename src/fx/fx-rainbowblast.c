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
  //int id = getId(); //TODO - getID function
  static int id; // there will be 50 cubes.. for now we only have one so just pick a random number
  static int offset;
  static int started; //only bother with calculating offset logic once
  static int lasttimerun;
  int coolingperiod = 4000; //only let it run every 4 seconds
  static RgbColor c;
  RgbColor black = {0,0,0};
 


  if(started == 0 || (getNetworkTimeMs() - lasttimerun) > coolingperiod) {
      started = 1;
      id = (uint32_t) rand() % 50 + 1; // this is just for now until we get a getId() function
      offset = getCubeLayoutOffset(cube_layout, id);
      rainbowcount = 0;
      rainbowcolor = 0;
      lasttimerun = getNetworkTimeMs();
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



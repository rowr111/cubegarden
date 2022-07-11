#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "analog.h"
#include "gyro.h"
#include "gfx.h"
#include "trigger.h"

/*effect description:
cubes randomly pick if they are a color in the pattern 
they spin at random rates
upon single tap, switch who you are
*/

void genericColor(struct effects_config *config, uint16_t colors[][3], uint8_t numOfColors) {
    uint8_t *fb = config->hwconfig->fb;
    int count = config->count;
    int loop = config->loop; 
    static uint16_t myColor[3] = {255, 255, 255};
    static uint16_t hue = 0;
    float factor = 0.711111111;

    if(patternChanged){
        patternChanged = 0;
        //pick the color
        hue = (uint32_t)rand() % numOfColors;
        chprintf(stream, "starting color: %d %d %d\n\r", colors[hue][0], colors[hue][1], colors[hue][2]);
    }
    //if tapped, change who you are
    if(singletapped){
        singletapped = 0;
        uint16_t oldhue = hue;
        while(hue == oldhue) {
            hue = (uint32_t)rand() % numOfColors;
        }
        chprintf(stream, "changed to new color: %d %d %d\n\r", colors[hue][0], colors[hue][1], colors[hue][2]);
    }

    //do spin
    uint8_t coloroffset = (loop/2) % count;
    RgbColor white = {255, 255, 255}; //white as default
    for(int i = 0; i<count; i++){
        if (i<(int)((float)count/1.25)){
            HsvColor myhsv = {(int)(colors[hue][0]*factor), (int)(colors[hue][1]*factor), (int)(colors[hue][2]*factor)};
            RgbColor myrgb = HsvToRgb(myhsv); 
            ledSetRgbColor(fb, (i+coloroffset)%count, myrgb, shift);
        }
        else{
            ledSetRgbColor(fb, (i+coloroffset)%count, white, shift); 
        }
    }
}


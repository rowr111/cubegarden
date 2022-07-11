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
cubes randomly pick if they are a color in the pattern 
they spin at random rates
upon single tap, switch who you are
*/


static void genericColor(struct effects_config config, int colors[], int numOfColors) {
    uint8_t *fb = config->hwconfig->fb;
    int count = config->count;
    int loop = config->loop; 
    int pulselength = 100; //length of pulse 
    static uint16_t hue = 0;

    if(patternChanged){
        patternChanged = 0;
        //pick the color
        hue = colors[(uint32_t)rand() % numOfColors];
        chprintf(stream, "starting hue: %d\n\r", hue);
    }
    //if tapped, change who you are
    if(singletapped){
        singletapped = 0;
        hue = colors[(uint32_t)rand() % numOfColors];
        chprintf(stream, "changed to new color\n\r");
    }

    //do spin
    uint8_t coloroffset = (loop/2) % count;
    RgbColor white = {255, 255, 255}; //white as default
    for(int i = 0; i<count; i++){
        if (i<(int)((float)count/1.5)){
            HsvColor myhsv = {hue, 255, 255};
            RgbColor myrgb = HsvToRgb(myhsv); 
            ledSetRgbColor(fb, (i+coloroffset)%count, myrgb, shift);
        }
        else{
            ledSetRgbColor(fb, (i+coloroffset)%count, white, shift); 
        }
    }
}


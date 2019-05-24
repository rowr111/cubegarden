#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"

#include <string.h>
#include <math.h>

static void changeOnDropVividRainbow(struct effects_config *config){
	uint8_t *fb = config->hwconfig->fb;
	int count = config->count;

	static int currentColor;
	//if there isn't a color already we need to run this to set the color
  ledSetAllRgbColor(fb, count, vividRainbow[currentColor], shift/2);

	if(bumped){
		bumped=0;
		//chprintf(stream, "%s", "New Color: ");
		//chprintf(stream, "%d\n\r", currentColor);
		//update the color by one and set it
		currentColor = (currentColor + 1)%6;
    ledSetAllRgbColor(fb, count, vividRainbow[currentColor], shift/2);
	}
}
orchard_effects("changeOnDropVividRainbow", changeOnDropVividRainbow);


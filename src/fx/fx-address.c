#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include "radio.h"

#include <string.h>
#include <math.h>

const RgbColor black = {0, 0, 0};
const RgbColor red = {255, 0, 0};
const RgbColor green = {0, 255, 0};
const RgbColor blue = {0, 0, 255};
RgbColor colors[4];

static void calculateAddressInColors(uint8_t address, uint8_t *addressInColors);
static void initColors(void);

/**
 * Displays the node's address as a sequence of colors.
 * 
 * The node's address is converted to base 4 and each digit in 
 * the value is converted to a color using a lookup table and
 * displayed for 1 second.
 */
static void address(struct effects_config *config){
	uint8_t *fb = config->hwconfig->fb;
	int count = config->count;
  int loop = config->loop;
   // Each loop value lasts for 35 ms
  // 1 s is 1000/35 ~ 30 steps
  // Max 4 digits in the address + 1 for delimiter
  int index = (loop / 30) % 5;
  static uint8_t addressInColors[4];
  static uint8_t address = 0;

  if (address != radioAddress(radioDriver)) {
    address = radioAddress(radioDriver);
    calculateAddressInColors(address, addressInColors);
    initColors();
  }

  if (index != 4) {
    ledSetAllRgbColor(fb, count, colors[addressInColors[index]], shift);
  } else {
    ledSetAllRGB(fb, count, 255, 255, 255, shift);
  }
}

static void calculateAddressInColors(uint8_t address, uint8_t *addressInColors) {
  for (int i = 3; i >= 0; i--) {
    addressInColors[i] = address % 4;
    address /= 4;
  }
}

static void initColors(void) {
  colors[0] = black;
  colors[1] = red;
  colors[2] = green;
  colors[3] = blue;
}

#ifndef MASTER_BADGE
orchard_effects("address", address, 0);
#else
orchard_effects("address", address);
#endif
#include "ch.h"
#include "hal.h"
#include "orchard-effects.h"
#include "led.h"
#include "radio.h"

#include "chprintf.h"
#include "stdlib.h"
#include "orchard-math.h"
#include "fixmath.h"

#include "storage.h"
#include "time.h"
#include "radio.h"
#include "address.h"

#include <string.h>
#include <math.h>


#define BUMP_DEBOUNCE 300 // 300ms debounce to next bump
#define PRESSURE_DEBOUNCE 300 // 300ms debounce to next pressure change event
#define SINGLETAP_DEBOUNCE 300 // 300ms debounce to next singletap
#define DOUBLETAP_DEBOUNCE 300 // 300ms debounce to next doubletap

uint32_t bump_amount = 0;
uint8_t bumped = 0;
uint8_t pressure_changed = 0;
uint8_t singletapped = 0;
uint8_t doubletapped = 0;
unsigned int pressurechangedtime = 0;
unsigned int singletaptime = 0;
unsigned int doubletaptime = 0;

void bump(uint32_t amount) {
  static unsigned int bumptime = 0;
  
  bump_amount = amount;
  if( chVTGetSystemTime() - bumptime > BUMP_DEBOUNCE ) {
    bumptime = chVTGetSystemTime();
    bumped = 1;
  }
}

void pressureChanged(void){
  if( chVTGetSystemTime() - pressurechangedtime > PRESSURE_DEBOUNCE ) {
    pressurechangedtime = chVTGetSystemTime();
    pressure_changed = 1;
  }
}

void singletap(void) {
  if( chVTGetSystemTime() - singletaptime > SINGLETAP_DEBOUNCE ) {
    singletaptime = chVTGetSystemTime();
    singletapped = 1;
  }
}

void doubletap(void) {
  if( chVTGetSystemTime() - doubletaptime > DOUBLETAP_DEBOUNCE ) {
    doubletaptime = chVTGetSystemTime();
    doubletapped = 1;
  }
}

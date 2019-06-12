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

#include <string.h>
#include <math.h>


#define BUMP_DEBOUNCE 300 // 300ms debounce to next bump
#define PRESSURE_DEBOUNCE 300 // 300ms debounce to next pressure change event
#define SINGLETAP_DEBOUNCE 300 // 300ms debounce to next singletap
#define DOUBLETAP_DEBOUNCE 300 // 300ms debounce to next doubletap
#define DOUBLETAP_HISTORY 10

uint32_t bump_amount = 0;
uint8_t bumped = 0;
uint8_t pressure_changed = 0;
uint8_t singletapped = 0;
uint8_t doubletapped = 0;
unsigned int pressurechangedtime = 0;
unsigned int singletaptime = 0;
unsigned int doubletaptime = 0;
static uint32_t doubletap_history[DOUBLETAP_HISTORY];
static uint8_t doubletapindex; 
int keepdoubletap = 10000; //ms to keep doubletaps in history

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

void checkdoubletapTrigger(void){
   int cubeid; // there will be 50 cubes.. for now we only have one so just pick a random number
    //int id = getId(); //TODO - getID function
   cubeid = (uint32_t) rand() % 50 + 1; // this is just for now until we get a getId() function

    //check doubletap_history and cleanup if in history longer than keepdoubletap
  for(int i=0; i<DOUBLETAP_HISTORY; i++){
    if(doubletap_history[i] + keepdoubletap < chVTGetSystemTime()){
      doubletap_history[i] = 0;
    }
  }
  //add doubletap to history if it exists
  if(doubletapped){
    doubletapped = 0;
    doubletap_history[doubletapindex] = chVTGetSystemTime();
    doubletapindex = (doubletapindex + 1) % DOUBLETAP_HISTORY;
  }
  //check and see how many doubletaps are in history and send notification if it's equal to DOUBLETAP_HISTORY
  int doubletapcount = 0;
    for(int i=0; i<DOUBLETAP_HISTORY; i++){
    if(doubletap_history[i] > 0) doubletapcount++;
  }
  if(doubletapcount == DOUBLETAP_HISTORY){
    char idString[32];
    chsnprintf(idString, sizeof(idString), "fx trigger rainbowblast %d", cubeid);
    radioAcquire(radioDriver);
    radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_forward, sizeof(idString), idString);
    radioRelease(radioDriver);
  }
}
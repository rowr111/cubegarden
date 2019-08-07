#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "chprintf.h"
#include "stdlib.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "baton.h"
#include "userconfig.h"

extern uint8_t baton_holder_g;

#ifndef MASTER_BADGE

#include "gyro.h"
#include "gfx.h"
#include "trigger.h"
#include "radio.h"

/*
  effect description:

  * all the cubes start "blue"
  * one cube gets the baton, and it sparkles. If hit before a time-out, the baton is passed and the cube turns red
  * If things time-out, the baton is passed, and all cubes go blue again
  * If N or more baton passes happen before the time-out, then all the cubes strobe for ten seconds, then reset to blue
*/

/*
  game states.

  INIT - all blue, baton 0
  
  
 */

typedef enum {
  wmole_idle,
  wmole_whacked,
  wmole_sparkle,
} whack_fx;

#define STATE_MASK  0xC0    // top two bits are for tracking the game state
#define COUNT_MASK  0x3F    // bottom 6 bits are for tracking the cube whack count
// #define HOLD_TIMEOUT (6 * 1000)  // time for the mole to be hit NOW USERCONFIG
#define REMISSION_TIMEOUT (15 * 1000)  // how long to wait before making a new mole in event of loss

static void Whackamole(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  
  static uint8_t hue;
  static uint8_t sat;
  static uint8_t brt;
  uint16_t totallength = 86*2; //total of number of loops for the pattern
  uint16_t current = loop % totallength;
  static uint32_t  nexttime = 0;
  static uint8_t   strobemode = 1;

  static whack_fx  wmole_fx = wmole_idle;
  static whack_fx  wmole_fx_stash = wmole_idle;
  static uint32_t sparkle_time = 0;
  static uint32_t sparkle_duration = 0;
  BatonState *bstate = getBatonState();
  bstate->fx_uses_baton = 1; // let the baton state machine know we are handling batons
  static uint32_t num_baton_retries = 0;

  HsvColor c;
  RgbColor x;
  int i;
  uint8_t oldshift;

  const struct userconfig *uconfig;
  uconfig = getConfig();

  static uint8_t last_gamestate = 0;
  static uint8_t was_holding = 0;
  static uint32_t holding_time = 0;
  const char winner[] = "Whackamole winner!";
  if( bstate->state == baton_holding ) {
    // do holder stuff
    if( was_holding == 0 ) {
      holding_time = chVTGetSystemTime();
      was_holding = 1;
      chprintf(stream, "Became the mole!\n\r");
    }
    
    if( (loop % 4) == 0 ) {
      bstate->announce_time = chVTGetSystemTime();
      sendBatonHoldingPacket();
    }
      
    if( chVTTimeElapsedSinceX(holding_time) < ((uconfig->cfg_fx_newcube_time + 2) * 1000) ) {
      // add some extra time for notice + run
      wmole_fx = wmole_sparkle;
      sparkle_time = chVTGetSystemTime(); // sparkle forever in this state
      sparkle_duration = 100000; // sparkle forever
      
      if( mems_event ) { // mems_event set by external interrupt handler
        mems_event = 0;
        LSM6DS3_Event_Status_t status;
        gyro_Get_Event_Status(&status);
        if (status.TapStatus || status.DoubleTapStatus) { // we'll take single or double taps
          chprintf(stream, "Mole whacked on time!\n\r");
	  uint8_t series = bstate->fx & COUNT_MASK;
	  uint8_t progression = (bstate->fx & STATE_MASK) >> 6;
	  switch( progression ) {
	  case 0:
	    progression = 1;
	    break;
	  case 1:
	    if( series > 3 )
	      progression = 2;
	    break;
	  case 2:
	    if( series > 9 ) {
	      progression = 3;
	      // ANNOUNCE THE WINNAR!!!
	      for( i = 0; i < 5; i++ ) {
		radioAcquire(radioDriver);
		radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_paging, strlen(winner)+1, winner);
		radioRelease(radioDriver);
		chThdSleepMilliseconds(10);
	      }
	    }
	    break;
	  case 3:
	    progression = 0; // reset the game to zero
	    break;
	  }
	  series++;
	  
	  bstate->fx = ((progression & 0x3) << 6) | (series & 0x3F);
	  passBaton(baton_random, 0, 0); // we will retry the baton pass every cycle we are in passing
	  num_baton_retries = 0;
	  wmole_fx = wmole_whacked;
	  wmole_fx_stash = wmole_whacked;
	}
      }
    } else if( chVTTimeElapsedSinceX(holding_time) <
	       (REMISSION_TIMEOUT + ((uconfig->cfg_fx_newcube_time + 2) * 1000)) ) {
      // we timed out
      bstate->fx = 0;  // reinitialize the game state to 0
      wmole_fx = wmole_idle;
      wmole_fx_stash = wmole_idle;
    } else {
      was_holding = 0;
      chprintf(stream, "Passing to new mole!\n\r");
      passBaton(baton_random, 0, 0); // pass on the baton, start a new cycle
      num_baton_retries = 0;
    }
  } else if( bstate->state == baton_passing ) {
    num_baton_retries++;
    if( num_baton_retries < 30 )
      sendBatonPassPacket(); // low-level resend
    else {
      passBaton(baton_random, 0, 0); // give up, try a new address
      num_baton_retries = 0;
    }
  } else {
    was_holding = 0;
    // we're an observer
    if( last_gamestate != (bstate->fx & STATE_MASK) ) {
      sparkle_time = chVTGetSystemTime();
      switch( (bstate->fx & STATE_MASK) >> 6 ) {
      case 1:
	wmole_fx_stash = wmole_fx;
	wmole_fx = wmole_sparkle;
	sparkle_duration = 500;
	break;
      case 2:
	wmole_fx_stash = wmole_fx;
	wmole_fx = wmole_sparkle;
	sparkle_duration = 1000;
	break;
      case 3:
	wmole_fx_stash = wmole_fx;
	wmole_fx = wmole_sparkle;
	sparkle_duration = 8000;  // this is long enough that you'll never find the new mole, forcing a loss
	break;
      default:
	// the game was "lost" or ended, reset everything to idle
	wmole_fx = wmole_idle;
	wmole_fx_stash = wmole_idle;
	sparkle_duration = 0;
	break;
      }
      last_gamestate = bstate->fx & STATE_MASK;
    }
  }
  
  switch( wmole_fx ) {
  case wmole_whacked:
    hue = current % 43;
    hue = hue >= 21 ? 43 - hue : hue;
    sat = 150;
    brt = 255;
    
    c.h = hue;
    c.s = sat;
    c.v = brt;
    x = HsvToRgb(c);
    ledSetAllRGB(fb, count, x.r, x.g, x.b, shift); 
    break;
  case wmole_sparkle:
    // check to see if the sparkle has timed out or not
    if( chVTTimeElapsedSinceX(sparkle_time) > sparkle_duration ) {
      wmole_fx = wmole_fx_stash; // revert to previous state
      break;
    }
    
    oldshift = shift; // make it obnoxiously bright, eh?
    shift = 0;
    if( strobemode && (chVTGetSystemTime() > nexttime) ) {
      for( i = 0; i < count; i++ ) {
	if( (rand() % (unsigned int) count) < ((unsigned int) count / 3) )
	  ledSetRGB(fb, i, 255, 255, 255, shift);
	else
	  ledSetRGB(fb, i, 0, 0, 0, shift);
      }

      nexttime = chVTGetSystemTime() + 30 + (rand() % 25);
      strobemode = 0;
    }

    else if( !strobemode && (chVTGetSystemTime() > nexttime) ) {
      ledSetAllRGB(fb, count, 0, 0, 0, shift);
    
      nexttime = chVTGetSystemTime() + 30 + (rand() % 25);
      strobemode = 1;
    }
    shift = oldshift;
    break;
  case wmole_idle:
  default:
    // blue to magenta-ish, from rainyday effect
    hue = current % 86;
    hue = hue >= 43 ? 86 - hue : hue;
    hue = 150 + hue;
    sat = 255;
    brt = 255;
    
    c.h = hue;
    c.s = sat;
    c.v = brt;
    x = HsvToRgb(c);
    ledSetAllRGB(fb, count, x.r, x.g, x.b, shift); 
    break;
  }
}
orchard_effects("Whackamole", Whackamole, 0);

#else
extern uint32_t last_ping_g;
static void Whackamole(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  BatonState *bstate;
  bstate = getBatonState();

  HsvColor c;
  c.h = 88; 
  c.s = 255;
  int brightness = loop%100;
  brightness = brightness > 50 ? 100 - brightness : brightness;
  float brightperc = (float)brightness/50;
  c.v = (int) (255 * brightperc);
  RgbColor x = HsvToRgb(c);
  ledSetAllRGB(fb, count, x.r, x.g, x.b, shift); 
  //if no one is holding the baton, pass it randomly to kick things off
  if( chVTTimeElapsedSinceX(last_ping_g) > 30000 ) {   // 30 seconds no baton, pass a new one out
    chprintf(stream, "no baton holder, trying to pass to random\n\r");
    baton_new_random();
  } 
}
orchard_effects("Whackamole", Whackamole, 0);
#endif



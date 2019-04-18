#include <limits.h>

#include "ch.h"
#include "hal.h"

#include "orchard-app.h"
#include "led.h"
#include "genes.h"
#include "radio.h"
#include "storage.h"
#include "userconfig.h"
#include "charger.h"

#include "mic.h"

#include "orchard-ui.h"
#include <string.h>
#include <math.h>

#include "fixmath.h"
#include "fix16_fft.h"

#define SEXTEST 0

static uint8_t friend_index = 0;
static uint8_t numlines = 1;
static uint8_t friend_total = 0;

#define BUMP_LIMIT 32  // 32 shakes to get to the limit
#define RETIRE_RATE 100 // retire one bump per "80ms"
static uint8_t bump_level = 0;

#define REFRACTORY_PERIOD 5000  // timeout for having sex
#define UI_LOCKOUT_TIME 6000  // timeout on friend list sorting/deletion after UI interaction

#define OSCOPE_IDLE_TIME 30000  // 30 seconds of idle, and we switch to an oscilloscope mode UI...because it's pretty
#define OSCOPE_UPDATE_FAIL 500 // 0.5 seconds until auto retrigger of update, in case ui is disrupted
static uint32_t last_ui_time = 0;
static uint32_t last_oscope_time = 0;
static uint32_t last_oscope_update = 0;

extern uint8_t scopemode_g;  // used by the oscope routine
static uint8_t oscope_running = 0;
static uint16_t *samples;
static uint32_t oscope_trigger = 1;

extern uint8_t sex_running;
extern uint8_t sex_done;
uint8_t sex_clear_event = 0;
uint32_t sex_timer; 

static char partner[GENE_NAMELENGTH];  // name of current sex partner

#define SEX_TIMEOUT (8 * 1000)  // 15 seconds for partner to respond before giving up

#define NUM_SAMPLES MIC_SAMPLE_DEPTH  // was NUM_RX_SAMPLES / 4

void agc(uint16_t  *sample, uint16_t *output);
void agc_fft(uint8_t  *sample);

// happens within a gfxlock
void precompute(uint16_t *samples);
void dbcompute(uint16_t *sample);

static void do_oscope(void) {

  if( oscope_trigger ) {
    oscope_trigger = 0;
  
    if( scopemode_g == 0 || scopemode_g == 1 ) {
      // call compute before flush, so stack isn't shared between two memory intensive functions
      precompute(samples);
    } else {
      dbcompute(samples);
    }

  } 
}


static void redraw_ui(uint8_t uimode) {

  if( oscope_running ) {
    // do oscope stuff
    do_oscope();
    return;
  }

  if( (chVTGetSystemTime() - last_ui_time) > UI_LOCKOUT_TIME )
    friendsSort();

  friend_total = friendCount();
  // reset the index in case the total # of friends shrank on us while we weren't looking
  if( (friend_index >= friend_total) && (friend_total > 0) )
    friend_index = friend_total - 1;
  if( friend_total == 0 )
    friend_index = 0;
}

static uint32_t led_init(OrchardAppContext *context) {

  (void)context;
  return 0;
}

static void led_start(OrchardAppContext *context) {
  
  (void)context;
  font_t font;
  coord_t height;
  coord_t fontheight;

  scopemode_g = 0; // start in oscope mode, not dB mode
  
  last_ui_time = chVTGetSystemTime();
  last_oscope_time = chVTGetSystemTime();
  oscope_running = 0;
  sex_running = 0;

  listEffects();

  sex_timer = chVTGetSystemTime();
  bump_level = 0;
  orchardAppTimer(context, RETIRE_RATE * 1000 * 1000, true);  // fire every 500ms to retire bumps
  redraw_ui(0);
}

void led_event(OrchardAppContext *context, const OrchardAppEvent *event) {

  (void)context;
  uint8_t shift;
  uint8_t selected = 0;
  char sexpacket[GENE_NAMELENGTH * 2 + 2];
  int i;
  int16_t *sample_in;
  
  if (event->type == keyEvent) {
    if (event->key.flags == keyDown) {
      if( sex_clear_event == 1 ) {
	sex_clear_event = 0; // ignore the first keydown event after sex, because it might be from the rubbing
      } else if( (bump_level < BUMP_LIMIT) && (sex_running) ) {
	bump_level++;
	// next is an else-if because we want no buttons to trigger in sex mode
      } else if ( event->key.code == keyBottomR ) {
	shift = getShift();
	shift++;
	if (shift > 6)
	  shift = 6;
	setShift(shift);
      }
      else if( event->key.code == keyTopR ) {
	shift = getShift();
	if( shift > 0 )
	  shift--;
	setShift(shift);
      }
      else if ( event->key.code == keyRight ) {
	effectsNextPattern(1);
	last_oscope_time = chVTGetSystemTime();
	oscope_running = 0;
      }
      else if ( event->key.code == keyLeft ) {
	effectsPrevPattern(1);
	last_oscope_time = chVTGetSystemTime();
	oscope_running = 0;
      }
      else if( event->key.code == keyBottom ) {
	if( friend_total != 0 )
	  friend_index = (friend_index + 1) % friend_total;
	else
	  friend_index = 0;
	last_ui_time = chVTGetSystemTime();
	last_oscope_time = chVTGetSystemTime();
	oscope_running = 0;
      } else if( event->key.code == keyTop) {
	if( friend_total != 0 ) {
	  if( friend_index == 0 )
	    friend_index = friend_total;
	  friend_index--;
	} else {
	  friend_index = 0;
	}
	last_ui_time = chVTGetSystemTime();
	last_oscope_time = chVTGetSystemTime();
	oscope_running = 0;
      } else if( event->key.code == keySelect ) {
	last_ui_time = chVTGetSystemTime();
	// oscope timer does not reset on select as it should swap between FFT and time domain mode
	if( oscope_running ) {
	  scopemode_g = (scopemode_g + 1) % 3;
	} else {
	  if( friend_total != 0 ) { // we have friends
	    if( strncmp(effectsCurName(), "Lg", 2) == 0 ) { // and we're sporting a genetic light
	      // trigger sex protocol
	      // confirm_sex(context); // consent always given for cube
	    }
	  }
	}
      }
    }
  } else if(event->type == radioEvent) {
    // placeholder
  } else if( event->type == uiEvent ) {
    last_ui_time = chVTGetSystemTime();
    last_oscope_time = chVTGetSystemTime();
    
    selected = 0; // always consent for cubes
    if(selected == 0) { // 0 means we said yes based on list item order in the UI
      sex_done = 0;
      sex_running = 1;
      sex_timer = chVTGetSystemTime();
#if SEXTEST
      {
	const struct genes *family;
	family = (const struct genes *) storageGetData(GENE_BLOCK);
	handle_radio_sex_req(radio_prot_sex_req, 255, 255, 
			     strlen(family->name), family->name);
      }
#else
      {
	const struct genes *family;
	family = (const struct genes *) storageGetData(GENE_BLOCK);
	strncpy(sexpacket, &(partner[1]), GENE_NAMELENGTH);
	strncpy(&(sexpacket[strlen(&(partner[1]))+1]), family->name, GENE_NAMELENGTH);
	radioAcquire(radioDriver);
	sexmode = 1;
	radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_sex_req,
		  GENE_NAMELENGTH * 2 + 2, sexpacket);
	sexmode = 0;
	radioRelease(radioDriver);
      }
#endif
      configIncSexInitiations(); // track # times we tried to have sex
    }
  } else if( event->type == adcEvent) {
    if( oscope_running ) {
      last_oscope_update = chVTGetSystemTime();
      if( event->adc.code == adcCodeMic ) {
	sample_in = analogReadMic();
	samples = (uint16_t *) sample_in;
	for( i = 0; i < NUM_RX_SAMPLES * NUM_RX_BLOCKS; i++ ) {
	  samples[i] = (uint16_t) (((int32_t) sample_in[i] + 32768L) & 0xFFFF);
	}

	oscope_trigger = 1;
	if( context->instance->ui == NULL )
	  redraw_ui(0);
      }
      analogUpdateMic();
    }
  } else if (event->type == timerEvent) {
    if( sex_running == 0 )
      bump_level = 0;
    if( bump_level > 0 )
      bump_level--;
    if( context->instance->ui == NULL )
      redraw_ui(0);
  } else if( event->type == accelEvent ) {
    if( (bump_level < BUMP_LIMIT) && (sex_running) )
      bump_level++;
    
    if( context->instance->ui == NULL )
      redraw_ui(0);
  }

  if( sex_running ) {
    last_oscope_time = chVTGetSystemTime();  // don't allow oscope to start while having sex
    if( ((chVTGetSystemTime() - sex_timer) > SEX_TIMEOUT) ) {
      sex_running = 0;
      sex_done = 0;
      redraw_ui(1);  // indicate that sex has timed out
    }
    if( sex_done ) {
      sex_running = 0;
      sex_done = 0;
      check_lightgene_hack();
      redraw_ui(2);  // indicate that sex is done
      sex_clear_event = 1;  // flush event pipe so rubbing doesn't activate pads
    }
  }
  
  // redraw UI on any event
  if( context->instance->ui == NULL ) {
    redraw_ui(0);

    // only kick off oscope if we're not in a UI mode...
    if( ((chVTGetSystemTime() - last_oscope_time) > OSCOPE_IDLE_TIME) && !oscope_running ) {
      analogUpdateMic(); // this kicks off the ADC sampling; once this returns, the UI will swap modes automagically
      last_oscope_update = chVTGetSystemTime();
      oscope_running = 1;
    }
    
    if( ((chVTGetSystemTime() - last_oscope_update) > OSCOPE_UPDATE_FAIL) && oscope_running ) {
      // refresh UI in case of a long interruption, e.g. sex prompts etc.
      analogUpdateMic();
      last_oscope_update = chVTGetSystemTime();
    }
  }
}

static void led_exit(OrchardAppContext *context) {

  (void)context;
}

orchard_app("Phasians and Sex!", led_init, led_start, led_event, led_exit);



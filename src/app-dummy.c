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

#define BUMP_LIMIT 32  // 32 shakes to get to the limit
#define RETIRE_RATE 100 // retire one bump per "80ms"
static uint8_t bump_level = 0;

#define OSCOPE_IDLE_TIME 30000  // 30 seconds of idle, and we switch to an oscilloscope mode UI...because it's pretty
#define OSCOPE_UPDATE_FAIL 500 // 0.5 seconds until auto retrigger of update, in case ui is disrupted

extern uint8_t scopemode_g;  // used by the oscope routine
static uint32_t oscope_trigger = 1;

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
      precompute(raw_samples);
    } else {
      dbcompute(raw_samples);
    }

  } 
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

  scopemode_g = 1; // 0 picks time domain, 1 picks FFT, 2 picks dB
  
  listEffects();

  bump_level = 0;
  orchardAppTimer(context, RETIRE_RATE * 1000 * 1000, true);  // fire every 500ms to retire bumps
  do_oscope();

  analogUpdateMic();  // this fires up the sound sampling
}

void led_event(OrchardAppContext *context, const OrchardAppEvent *event) {

  (void)context;
  char sexpacket[GENE_NAMELENGTH * 2 + 2];
  int i;
  int16_t *sample_in;
  uint16_t samples[NUM_RX_SAMPLES * NUM_RX_BLOCKS];
  
  if(event->type == radioEvent) {
    // placeholder
  } else if( event->type == adcEvent) {
    if( event->adc.code == adcCodeMic ) {
      sample_in = analogReadMic();
      // samples = (uint16_t *) sample_in;
      // convert input number format to uint16_t representation
      for( i = 0; i < NUM_RX_SAMPLES * NUM_RX_BLOCKS; i++ ) {
	samples[i] = (uint16_t) (((int32_t) sample_in[i] + 32768L) & 0xFFFF);
      }
      // cheezy decimate to NUM_SAMPLES; maybe use a better anti-aliasing routine later
      for( i = 0; i < NUM_SAMPLES; i++  ) {
	raw_samples[i] = samples[i];
	//	raw_samples[i] = samples[i * (NUM_RX_SAMPLES * NUM_RX_BLOCKS) / NUM_SAMPLES];
      }
      
      oscope_trigger = 1;
      do_oscope();
    }
    analogUpdateMic();
  } else if (event->type == timerEvent) {
    if( bump_level > 0 )
      bump_level--;

      //update temperature regularly for use by effects
      analogUpdateTemperature();
      
  } else if( event->type == accelEvent ) {
    if( (bump_level < BUMP_LIMIT) )
      bump_level++;
  }
  
}

static void led_exit(OrchardAppContext *context) {

  (void)context;
}

orchard_app("Phasians and Sex!", led_init, led_start, led_event, led_exit);

#include "orchard-app.h"
#include "orchard-ui.h"

#include "mic.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include "fixmath.h"
#include "fix16_fft.h"

static int mode = 2;

#define NUM_SAMPLES (NUM_RX_SAMPLES / 4)

uint8_t dblog[DBLOGLEN];
uint8_t dblogptr = 0;

static void agc(uint16_t  *sample, uint16_t *output) {
  uint16_t min, max;
  uint16_t i;
  float scale;
  uint32_t temp;

  // cheesy AGC to get something on the screen even if it's pretty quiet
  // e.g., "comfort noise"
  // note abstraction violation: we're going to hard-code this for a screen height of 64
  if( sample == NULL )
    return;

  min = 65535; max = 0;
  for( i = 0; i < NUM_RX_SAMPLES; i++ ) { // input sample buffer is deeper, search all the way through
    if( sample[i] > max )
      max = sample[i];
    if( sample[i] < min )
      min = sample[i];
  }

  uint16_t span = max - min;
  scale = 65535.0 / (float) span;

  for( i = 0; i < NUM_RX_SAMPLES; i += 4 ) { // decimate by 4
    temp = sample[i] - min;
    temp = (uint32_t) (((float)temp) * scale);
    output[i / 4] = (uint16_t) temp; 
  }
}

static void agc_fft(uint8_t  *sample) {
  uint8_t min, max;
  uint8_t scale = 1;
  int16_t temp;
  uint16_t i;

  // cheesy AGC to get something on the screen even if it's pretty quiet
  // e.g., "comfort noise"
  // note abstraction violation: we're going to hard-code this for a screen height of 64
  if( sample == NULL )
    return;

  min = 255; max = 0;
  for( i = 2; i < NUM_SAMPLES; i++ ) {
    if( sample[i] > max )
      max = sample[i];
    if( sample[i] < min )
      min = sample[i];
  }

  if( (max - min) < 128 )
    scale = 2;
  if( (max - min) < 64 )
    scale = 4;
  if( (max - min) < 32 )
    scale = 8;
  if( (max - min) < 16 )
    scale = 16;
  
  for( i = 0; i < MIC_SAMPLE_DEPTH; i++ ) {
    temp = sample[i];
    temp *= scale;
    sample[i] = (uint8_t) temp;
  }
}

// happens within a gfxlock
static void precompute(uint16_t *samples) {
  fix16_t real[NUM_SAMPLES];
  fix16_t imag[NUM_SAMPLES];
  uint16_t agc_samp[NUM_SAMPLES];
  uint8_t fft_samp[NUM_SAMPLES];
  coord_t height;
  coord_t width;
  uint16_t i;
  uint16_t scale;

  agc( samples, agc_samp );
  if ( mode ) {
    for( i = 0; i < NUM_SAMPLES; i++ ) {
      fft_samp[i] = (uint8_t) (agc_samp[i] >> 8) & 0xFF;
    }
    fix16_fft(fft_samp, real, imag, NUM_SAMPLES);
    for( i = 0; i < NUM_SAMPLES; i++ ) {
      fft_samp[i] = fix16_to_int( fix16_sqrt(fix16_sadd(fix16_mul(real[i],real[i]),
						       fix16_mul(imag[i],imag[i]))) );
    }
    
    agc_fft(fft_samp);
    for( i = 0; i < NUM_SAMPLES; i++ ) {
      agc_samp[i] = ((uint16_t) fft_samp[i]) << 8;
    }
  }
  
  height = gdispGetHeight();
  scale = 65535 / height;

  gdispClear(Black);

  width = gdispGetWidth();
  for( i = 1; i < width; i++ ) {
    if( samples != NULL ) {
#if 1
      // below for dots, change starting index to i=0
      //      gdispDrawPixel((coord_t)i, (coord_t) (255 - samples[i]) / scale , White);
      gdispDrawLine((coord_t)i-1, (coord_t) ((65535 - agc_samp[i-1]) / scale), 
		    (coord_t)i, (coord_t) ((65535 - agc_samp[i]) / scale), 
		    White);
#else
      gdispDrawLine((coord_t)i-1, (coord_t) samples[i-1], 
		    (coord_t)i, (coord_t) samples[i], 
		    White);
#endif
    } else
      gdispDrawPixel((coord_t)i, (coord_t) 32, White);
  }
}

static void dbcompute(uint16_t *sample) {
  char uiStr[32];
  
  coord_t width;
  coord_t height;
  coord_t tallheight;
  font_t font;
  font_t font2;
  int db = 0;

  font = gdispOpenFont("fixed_5x8");
  width = gdispGetWidth();
  height = gdispGetFontMetric(font, fontHeight);

  gdispClear(Black);
  gdispFillArea(0, 0, width, height, White);

  gdispDrawStringBox(0, 0, width, height,
                     "Sound level", font, Black, justifyCenter);

  gdispCloseFont(font);
  font2 = gdispOpenFont("DejaVuSans32");
  tallheight = gdispGetFontMetric(font2, fontHeight);

  // now compute dbs...
#if 0
  uint16_t min, max;
#endif
  uint16_t i;
  float cum = 0.0;
  int32_t temp;

#if 0
  min = 65535; max = 0;
  for( i = 0; i < NUM_RX_SAMPLES; i++ ) {
    if( sample[i] > max )
      max = sample[i];
    if( sample[i] < min )
      min = sample[i];
  }
  int32_t mid = (max + min) / 2;
#else
  cum = 0.0;
  for( i = 0; i < NUM_RX_SAMPLES; i++ ) {
    cum += (float)sample[i];
  }
  int32_t mid = (int32_t) (cum / (float) NUM_RX_SAMPLES);
#endif
  
  cum = 0.0;
  for( i = 0; i < NUM_RX_SAMPLES; i++ ) {
    temp = (((int32_t)sample[i]) - mid);
    cum += (float) (temp * temp);
  }
  cum /= (float) NUM_RX_SAMPLES;
  cum = sqrt(cum);
  db = (int)  (24.0 + 20.0 * log10(cum)); // assumes 120dB is peak value, from AOP on datasheet
  dblog[dblogptr] = (uint8_t) db;
  dblogptr = (dblogptr + 1) % DBLOGLEN;
  
  cum = 0.0;
  for( i = 0; i < DBLOGLEN; i++ ) {
    cum += (float) dblog[i];
  }
  cum /= (float) DBLOGLEN;

  chsnprintf(uiStr, sizeof(uiStr), "%ddB", (int) cum );
  gdispDrawStringBox(0, height*2, width, tallheight,
		     uiStr, font2, White, justifyCenter);
  
  gdispCloseFont(font2);
}

static void redraw_ui(uint16_t *samples) {
  
  orchardGfxStart();
  if( mode == 0 || mode == 1 ) {
    // call compute before flush, so stack isn't shared between two memory intensive functions
    precompute(samples);
  } else {
    dbcompute(samples);
  }
  gdispFlush();
   
  orchardGfxEnd();
}

static uint32_t oscope_init(OrchardAppContext *context) {
  (void)context;

  return 0;
}

static void oscope_start(OrchardAppContext *context) {
  (void)context;

  redraw_ui(NULL);
  analogUpdateMic();
}

void oscope_event(OrchardAppContext *context, const OrchardAppEvent *event) {
  int i;
  
  (void)context;
  int16_t *sample_in;
  uint16_t *samples;
  
  if (event->type == keyEvent) {
    if ( (event->key.flags == keyDown) && (event->key.code == keyLeft) ) {
      // orchardAppExit();  // I think this is a misfeature as "home" hold should be the way out
    } else  if ( (event->key.flags == keyDown) && (event->key.code == keySelect) ) {
      mode = (mode + 1) % 3;
    }
  } else if( event->type == adcEvent) {
    if( event->adc.code == adcCodeMic ) {
      sample_in = analogReadMic();
      samples = (uint16_t *) sample_in;
      for( i = 0; i < NUM_RX_SAMPLES; i++ ) {
	samples[i] = (uint16_t) (((int32_t) sample_in[i] + 32768L) & 0xFFFF);
      }
      redraw_ui(samples);
    }
    analogUpdateMic();
  }
}

static void oscope_exit(OrchardAppContext *context) {

  (void)context;
}

orchard_app("Sound scope", oscope_init, oscope_start, oscope_event, oscope_exit);



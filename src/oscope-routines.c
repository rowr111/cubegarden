#include "orchard-app.h"
#include "orchard-ui.h"

#include "mic.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include "fixmath.h"
#include "fix16_fft.h"

uint8_t scopemode_g = 2;

uint8_t dblog[DBLOGLEN];
uint8_t dblogptr = 0;

// this is the output record for the sound device
uint16_t mic_processed[NUM_SAMPLES];
uint16_t raw_samples[NUM_SAMPLES];

float cur_db;

void agc(uint16_t  *sample, uint16_t *output) {
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
  for( i = 0; i < NUM_SAMPLES; i++ ) {
    if( sample[i] > max )
      max = sample[i];
    if( sample[i] < min )
      min = sample[i];
  }

  float span = (float) max - (float) min;
  scale = (65535.0 / span);

  for( i = 0; i < NUM_SAMPLES; i ++ ) {
    temp = sample[i] - min;
    temp = (uint32_t) (((float)temp) * scale);
    output[i] = (uint16_t) temp; 
  }
}

void agc_fft(uint8_t  *sample) {
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
  
  for( i = 0; i < NUM_SAMPLES; i++ ) {
    temp = sample[i];
    temp *= scale;
    sample[i] = (uint8_t) temp;
  }
}

void agc_fft_u16(uint16_t  *sample) {
  uint16_t max;
  uint16_t scale = 1;
  uint16_t temp;
  uint16_t i;

  // cheesy AGC to get something on the screen even if it's pretty quiet
  // e.g., "comfort noise"
  // note abstraction violation: we're going to hard-code this for a screen height of 64
  if( sample == NULL )
    return;

  max = 0;
  for( i = 2; i < NUM_SAMPLES; i++ ) {
    if( sample[i] > max )
      max = sample[i];
  }

  scale = 65535 / max;
  if( scale > 1024 )
    scale = 1024; // so that silence isn't blinding
  
  for( i = 0; i < NUM_SAMPLES; i++ ) {
    temp = sample[i];
    temp *= scale;
    sample[i] = temp;
  }
}

#define USE_AGC 0
void precompute(uint16_t *samples) {
  fix16_t real[NUM_SAMPLES];
  fix16_t imag[NUM_SAMPLES];
  coord_t height;
  coord_t width;
  uint16_t i;
  uint16_t scale;

#if USE_AGC  
  agc( samples, mic_processed );
#else
  for( i = 0; i < NUM_SAMPLES; i++ ) {
    mic_processed[i] = samples[i];
  }
#endif
  
#if 0
  uint8_t fft_samp[NUM_SAMPLES];
  
  if ( scopemode_g ) {
    for( i = 0; i < NUM_SAMPLES; i++ ) {
      fft_samp[i] = (uint8_t) (mic_processed[i] >> 8) & 0xFF;
    }
    fix16_fft(fft_samp, real, imag, NUM_SAMPLES);
    for( i = 0; i < NUM_SAMPLES; i++ ) {
      fft_samp[i] = fix16_to_int( fix16_sqrt(fix16_sadd(fix16_mul(real[i],real[i]),
						       fix16_mul(imag[i],imag[i]))) );
    }

    //    agc_fft(fft_samp);
    for( i = 0; i < NUM_SAMPLES; i++ ) {
      mic_processed[i] = ((uint16_t) fft_samp[i]) << 8;
    }
  }
#else
  if ( scopemode_g ) {
    fix16_fft(mic_processed, real, imag, NUM_SAMPLES);
    for( i = 0; i < NUM_SAMPLES; i++ ) {
      mic_processed[i] = fix16_to_int( fix16_sqrt(fix16_sadd(fix16_mul(real[i],real[i]),
						       fix16_mul(imag[i],imag[i]))) );
    }

    agc_fft_u16(mic_processed);
  }
#endif

}

void dbcompute(uint16_t *sample) {
  char uiStr[32];
  int db = 0;
  
  // now compute dbs...
  uint16_t i;
  float cum = 0.0;
  int32_t temp;

  cum = 0.0;
  for( i = 0; i < NUM_SAMPLES; i++ ) {
    cum += (float)sample[i];
  }
  int32_t mid = (int32_t) (cum / (float) (NUM_SAMPLES));
  
  cum = 0.0;
  for( i = 0; i < NUM_SAMPLES; i++ ) {
    temp = (((int32_t)sample[i]) - mid);
    cum += (float) (temp * temp);
  }
  cum /= (float) (NUM_SAMPLES);
  cum = sqrt(cum);
  db = (int)  (24.0 + 20.0 * log10(cum)); // assumes 120dB is peak value, from AOP on datasheet
  dblog[dblogptr] = (uint8_t) db;
  dblogptr = (dblogptr + 1) % DBLOGLEN;
  
  cum = 0.0;
  for( i = 0; i < DBLOGLEN; i++ ) {
    cum += (float) dblog[i];
  }
  cum /= (float) DBLOGLEN;

  cur_db = cum;
}


#include "hal.h"
#include "ch.h"
#include "shell.h"
#include "chprintf.h"

#include "orchard-app.h"
#include "orchard-ui.h"

#include "mic.h"

#include <string.h>
#include <stdlib.h>

static int sd_error = 0;
extern MMCDriver MMCD1;

extern int sd_active;

static void redraw_ui() {
  char uiStr[32];
  
  coord_t width;
  coord_t height;
  coord_t tallheight;
  font_t font;
  font_t font2;
  
  orchardGfxStart();

  font = gdispOpenFont("fixed_5x8");
  width = gdispGetWidth();
  height = gdispGetFontMetric(font, fontHeight);

  gdispClear(Black);
  gdispFillArea(0, 0, width, height, White);

  gdispDrawStringBox(0, 0, width, height,
                     "Clip Recorder", font, Black, justifyCenter);

  gdispCloseFont(font);
  font2 = gdispOpenFont("DejaVuSans32");
  tallheight = gdispGetFontMetric(font2, fontHeight);

#if 0
  if( (ST2MS(chVTGetSystemTime()) / 1000) % 2 ) {
    chsnprintf(uiStr, sizeof(uiStr), "%s", "ACTIVE" );
  } else {
    chsnprintf(uiStr, sizeof(uiStr), " " );
  }
#else
  chsnprintf(uiStr, sizeof(uiStr), "%s", "ACTIVE" );
#endif
  gdispDrawStringBox(0, height*2, width, tallheight,
		     uiStr, font2, White, justifyCenter);
  
  gdispCloseFont(font);
  gdispCloseFont(font2);
  gdispFlush();
  orchardGfxEnd();
}

#define SECTOR_BYTES       MMCSD_BLOCK_SIZE  // should be 512

// should see the string 'RIFF' at these offsets in the SD card if it's formatted correctly
#define CLIP1_OFFSET_BYTES  0x0052800
#define CLIP2_OFFSET_BYTES  0x12D4000
#define CLIP3_OFFSET_BYTES  0x2555800
#define CLIP4_OFFSET_BYTES  0x37D7000
#define CLIP5_OFFSET_BYTES  0x4A58800
#define CLIP6_OFFSET_BYTES  0x5CDA000
const unsigned int clip_offset_bytes[] = { CLIP1_OFFSET_BYTES, CLIP2_OFFSET_BYTES, CLIP3_OFFSET_BYTES,
					   CLIP4_OFFSET_BYTES, CLIP5_OFFSET_BYTES, CLIP6_OFFSET_BYTES };

uint8_t *block = NULL;

// all offsets are in bytes
#define WAV_RIFF         0x0  // 'RIFF' located here
#define WAV_DATA_OFFSET  0x2C // data starts here
#define DATA_END_OFFSET  (0x1281400) // end of data offset, about 4 minutes

const char wav_header[WAV_DATA_OFFSET] =
  { 0x52, 0x49, 0x46, 0x46, 0x86, 0x15, 0x28, 0x01, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20,
    0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x44, 0xac, 0x00, 0x00, 0x88, 0x58, 0x01, 0x00,
    0x02, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0xe0, 0x14, 0x28, 0x01 };

static uint32_t sd_offset = CLIP1_OFFSET_BYTES + DATA_END_OFFSET; // start at end, to trigger initialization
static uint8_t clip_num = 0;

void do_agc(int16_t *samples) {
  int32_t min, max;
  int32_t peak;
  uint16_t i;
  int shift = 0;

  max = -32768;
  min = 32767;
  for( i = 0; i < NUM_RX_SAMPLES; i++ ) {
    if( samples[i] < min )
      min = samples[i];
    if(samples [i] > max )
      max = samples[i];
  }

  if( abs(min) > max )
    peak = abs(min);
  else
    peak = abs(max);

  // find how many powers of 2 the leading 1 is down
  while( ((peak & 0x8000) == 0) && (shift < 14) ) {
    shift ++;
    peak = peak << 1;
  }
  // back off for headroom
  if( shift > 0 )
    shift--;

  // limit gain
  if( shift > 9 )
    shift = 9;

  for( i = 0; i < NUM_RX_SAMPLES; i++ ) {
    samples[i] = samples[i] * (1 << shift);
  }
}

void update_sd(int16_t *samples) {
  unsigned int i;

  do_agc(samples);
  
  if( sd_error ) // don't update the SD if things are broken
    return;

  // check if we fell off the end; if so, loop to the front, reconstruting the WAV header
  if( sd_offset >= (clip_offset_bytes[clip_num] + DATA_END_OFFSET) ) {
    sd_offset = clip_offset_bytes[clip_num];
    
    for( i = 0; i < MMCSD_BLOCK_SIZE; i++ ) {
      if( i < WAV_DATA_OFFSET )
	block[i] = wav_header[i];
      else 
	block[i] = 0;
    }
    if( !HAL_SUCCESS == MMCD1.vmt->write(&MMCD1, sd_offset / SECTOR_BYTES, block, 1) )
      sd_error = 1;
    
    sd_offset += SECTOR_BYTES;
  }
  

  // else keep filling in the data
  if( !HAL_SUCCESS == 
      MMCD1.vmt->write(&MMCD1, sd_offset / SECTOR_BYTES, 
		       (uint8_t *) samples, (NUM_RX_SAMPLES * sizeof(int16_t)) / SECTOR_BYTES) ) {
    chprintf(stream, "mmc_write failed\n\r");
    sd_error = 1;
    return;
  }
  sd_offset += NUM_RX_SAMPLES * sizeof(int16_t);
}

static uint32_t rec_init(OrchardAppContext *context) {
  (void)context;

  block = chHeapAlloc(NULL, sizeof(uint8_t) * SECTOR_BYTES * 1);
  if( block == NULL ) {
    sd_error = 1;
    return 1;
  }

  //  chThdSetPriority(NORMALPRIO + 10); // give this thread higher priority
  return 0;
}

static void rec_start(OrchardAppContext *context) {
  (void)context;

  if( !HAL_SUCCESS == MMCD1.vmt->connect(&MMCD1) )
    sd_error = 1;
  else
    sd_error = 0;
    
  palClearPad(IOPORT5, 0); // turn on red LED
  redraw_ui();

  analogUpdateMic(); // don't generate events as we're going to record in the app thread directly
  sd_active = 1;

  //  orchardAppTimer(context, 1000 * 1000 * 500, true); //update ui maybe 10 times a second?
  
}

void rec_event(OrchardAppContext *context, const OrchardAppEvent *event) {

  (void)context;
  int16_t *samples;

  if (event->type == keyEvent) {
    if ( (event->key.flags == keyDown) && (event->key.code == keyLeft) ) {
      // orchardAppExit();  // I think this is a misfeature as "home" hold should be the way out
    } else  if ( (event->key.flags == keyDown) && (event->key.code == keySelect) ) {
      // put the code to "Commit" a recording here
    }
  } else if( event->type == adcEvent) {
    if( event->adc.code == adcCodeMic ) {
      update_sd(analogReadMic()); // recording happens in the app thread, not event thread
      analogUpdateMic();
    }
  }
}

static void rec_exit(OrchardAppContext *context) {

  (void)context;

  sd_active = 0;
  
  chHeapFree(block);
  
  palSetPad(IOPORT5, 0); // turn off red LED

  chprintf(stream, "rec_exit\n\r");
  chThdSleepMilliseconds(100); // wait for any converions to complete
 
  if( !HAL_SUCCESS == MMCD1.vmt->sync(&MMCD1) )
    chprintf(stream, "mmcSync failed\n\r" );
  
  //  if( !HAL_SUCCESS == MMCD1.vmt->disconnect(&MMCD1) )
  //    chprintf(stream, "mmcDisconnect failed\n\r");
  chThdSleepMilliseconds(100); // wait for any converions to complete
  
  //  chThdSetPriority(LOWPRIO + 2);
}

orchard_app("Record Clips", rec_init, rec_start, rec_event, rec_exit);



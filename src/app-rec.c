#include "hal.h"
#include "ch.h"
#include "shell.h"
#include "chprintf.h"

#include "orchard-app.h"
#include "orchard-ui.h"

#include "mic.h"
#include "led.h"
#include "userconfig.h"
#include "radio.h" // absorb the "anonymous" extern
#include <math.h>

#include <string.h>
#include <stdlib.h>

static int sd_error = 0;
extern MMCDriver MMCD1;

int sd_active = 0;

static int prompt_state = 1;
static int A_state = 0;
static uint32_t last_update = 0;
static uint8_t prev_anonymous = 0;

#define REC_IDLE 0
#define REC_REC  1
#define REC_ERR  2
#define REC_STREAM_ERR  3
#define REC_RECONNECT   4

static int rec_state = REC_IDLE;

#define AGC_NOW_LOUD 0
#define VOLDOWN_LOUD_THRESH  70 // it's been loud -- so likely to be loud again. clamp down on AGC more quickly in this state
#define VOLUP_LOUD_THRESH 50  // if currently loud, and volume below this, start raising volume

#define AGC_NOW_QUIET 1
#define VOLDOWN_QUIET_THRESH 90  // linked to max multiply of 16x
#define VOLUP_QUIET_THRESH 70    // if currently quiet, and volume is below this, start raising volume

#define MAX_AGC_GAIN 16.0
#define AGC_GAIN_STEP 0.5

uint8_t sd_dbg_val = 0;
static uint16_t cur_db = 120; 
static uint16_t agc_state = AGC_NOW_LOUD;
static float agc_gain = 1.0;
static float last_agc_gain = 1.0;

#define DCLOGLEN 16
static float dc_level = 0.0;
static float dclog[DCLOGLEN];
static uint8_t dclogptr = 0;

static uint8_t agc_enable = 1;

#define SECTOR_BYTES       MMCSD_BLOCK_SIZE  // should be 512

#define STOP_FX 0

// should see the string 'RIFF' at these offsets in the SD card if it's formatted correctly
/*  // for 44.1khz
#define CLIP1_OFFSET_BYTES  0x0052800
#define CLIP2_OFFSET_BYTES  0x12D4000
#define CLIP3_OFFSET_BYTES  0x2555800
#define CLIP4_OFFSET_BYTES  0x37D7000
#define CLIP5_OFFSET_BYTES  0x4A58800
#define CLIP6_OFFSET_BYTES  0x5CDA000
*/

// for 32khz
#define CLIP1_OFFSET_BYTES  0x0052800
#define CLIP2_OFFSET_BYTES  0x12A2800
#define CLIP3_OFFSET_BYTES  0x24F2800
#define CLIP4_OFFSET_BYTES  0x3742800
#define CLIP5_OFFSET_BYTES  0x4992800
#define CLIP6_OFFSET_BYTES  0x5BE2800

const unsigned int clip_offset_bytes[] = { CLIP1_OFFSET_BYTES, CLIP2_OFFSET_BYTES, CLIP3_OFFSET_BYTES,
					   CLIP4_OFFSET_BYTES, CLIP5_OFFSET_BYTES, CLIP6_OFFSET_BYTES };

// all offsets are in bytes
#define WAV_RIFF         0x0  // 'RIFF' located here
#define WAV_DATA_OFFSET  0x2C // data starts here
#define DATA_END_OFFSET  (0x124f800) // end of data offset, about 4 minutes
// #define DATA_END_OFFSET  (0x1281400) // end of data offset, about 4 minutes

const char wav_header[WAV_DATA_OFFSET] =
  { 0x52, 0x49, 0x46, 0x46, 0x90, 0xf8, 0x24, 0x01, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20,
    0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00,
    0x02, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x00, 0xf8, 0x24, 0x01 };
/*  { 0x52, 0x49, 0x46, 0x46, 0x86, 0x15, 0x28, 0x01, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20,
    0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x44, 0xac, 0x00, 0x00, 0x88, 0x58, 0x01, 0x00,
    0x02, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0xe0, 0x14, 0x28, 0x01 }; */

static uint8_t clip_num = 0;
static uint32_t sd_offset = CLIP1_OFFSET_BYTES + DATA_END_OFFSET; // start at end, to trigger initialization


static void redraw_ui(void) {
  char uiStr[32];
  
  coord_t width;
  coord_t height;
  coord_t header_height;
  coord_t tallheight;
  font_t font;
  font_t font2;

  int i;
  const struct userconfig *config;
  
  orchardGfxStart();

  font = gdispOpenFont("fixed_5x8");
  width = gdispGetWidth();
  height = gdispGetFontMetric(font, fontHeight);
  header_height = height;

  gdispClear(Black);
  gdispFillArea(0, 0, width, height, White);

  gdispDrawStringBox(0, 0, width, height,
                     "Clip Recorder", font, Black, justifyCenter);

  if( rec_state == REC_REC ) {
    font2 = gdispOpenFont("DejaVuSans32");
    tallheight = gdispGetFontMetric(font2, fontHeight);

    chsnprintf(uiStr, sizeof(uiStr), "REC %d", clip_num + 1 );
    gdispDrawStringBox(0, height*2, width, tallheight,
		       uiStr, font2, White, justifyCenter);
  
    gdispCloseFont(font2);

    if( A_state == 0 )
      gdispDrawStringBox(0, height*2 + tallheight - 2, width, header_height,
			 "Press & hold > to stop", font, White, justifyCenter);
    else
      gdispDrawStringBox(0, height*2 + tallheight - 2, width, header_height,
			 "Hold > for 3 seconds", font, White, justifyCenter);

    if( agc_enable )
      chsnprintf(uiStr, sizeof(uiStr), "Gain: %2.1f; ^ to disable", agc_gain );
    else
      chsnprintf(uiStr, sizeof(uiStr), "AGC off, hit ^ to enable", agc_gain );
      
    gdispDrawStringBox(0, height*3 + tallheight - 2, width, header_height,
		       uiStr, font, White, justifyCenter);
    
    
  } else if( rec_state == REC_IDLE ) {
    config = getConfig();
    
    color_t draw_color = White;
    for (i = 0; i < MAX_CLIPS; i++) {
      draw_color = White;
      
      if( config->cfg_clip_used[i] )
	chsnprintf(uiStr, sizeof(uiStr), "Clip %d  (used)", i + 1);
      else
	chsnprintf(uiStr, sizeof(uiStr), "Clip %d (blank)", i + 1);
	
      if (i == clip_num) {
	gdispFillArea(0, header_height + i * height,
		      width, height, White);
	draw_color = Black;
      }
      gdispDrawStringBox(0, header_height + i * height,
			 width, height,
			 uiStr, font, draw_color, justifyCenter);
    } 

    draw_color = White;
    if( clip_num == MAX_CLIPS ) {
      gdispFillArea(0, header_height + i * height,
		    width, height, White);
      draw_color = Black;
    }
    chsnprintf(uiStr, sizeof(uiStr), "Reset clip used state");
    gdispDrawStringBox(0, header_height + i * height,
		       width, height,
		       uiStr, font, draw_color, justifyCenter);
  } else if( rec_state == REC_ERR ) {
    gdispDrawStringBox(0, height*2, width, header_height,
		       "SD card error!", font, White, justifyCenter);
    gdispDrawStringBox(0, height*3, width, header_height,
		       "To record, insert", font, White, justifyCenter);
    gdispDrawStringBox(0, height*4, width, header_height,
		       "pre-formatted card and/or", font, White, justifyCenter);
    gdispDrawStringBox(0, height*5, width, header_height,
		       "power cycle the badge.", font, White, justifyCenter);
  } else if( rec_state == REC_STREAM_ERR ) {
    gdispDrawStringBox(0, height*2, width, header_height,
		       "SD card error!", font, White, justifyCenter);
    gdispDrawStringBox(0, height*3, width, header_height,
		       "Can't reconnect,", font, White, justifyCenter);
    gdispDrawStringBox(0, height*4, width, header_height,
		       "restart app.", font, White, justifyCenter);
  } else if( rec_state == REC_RECONNECT ) {
    gdispDrawStringBox(0, height*2, width, header_height,
		       "SD card error!", font, White, justifyCenter);
    gdispDrawStringBox(0, height*4, width, header_height,
		       "Reconnecting...", font, White, justifyCenter);
  }

  gdispCloseFont(font);
  
  gdispFlush();
  orchardGfxEnd();
}

void update_sd(int16_t *samples) {
  unsigned int i;
  unsigned int retry;
  uint8_t *block; 

  float agcval;
  int reduce_gain = 0;
  // apply AGC
  if( agc_enable ) {
    for( i = 0; i < NUM_RX_SAMPLES * NUM_RX_BLOCKS; i++ ) {
      agcval = (((float) samples[i] - dc_level) * agc_gain);
      // check for clipping
      if( agcval > 32767 ) {
	agcval = 32767;
	reduce_gain = 1;
      } if( agcval < -32768 ) {
	agcval = -32768;
	reduce_gain = 1;
      }
      samples[i] = (int16_t) agcval;
    }
    
    if( reduce_gain ) {
      agc_state = AGC_NOW_LOUD; // set to loud thresholds, so we don't fight the clipping mechanism
      agc_gain -= 4.0;
      if( agc_gain < 1.0 )
	agc_gain = 1.0;
    }
  }
  
  // check if we fell off the end; if so, loop to the front, reconstruting the WAV header, overwriting sound data
  if( sd_offset >= (clip_offset_bytes[clip_num] + DATA_END_OFFSET) ) {
    sd_offset = clip_offset_bytes[clip_num];

    block = (uint8_t *) samples;
    for( i = 0; i < WAV_DATA_OFFSET; i++ ) {
      block[i] = wav_header[i];
    }
  }
    
  // else keep filling in the data
  retry = 0;
  if( !HAL_SUCCESS == 
      MMCD1.vmt->write(&MMCD1, sd_offset / SECTOR_BYTES, 
		       (uint8_t *) samples, (NUM_RX_SAMPLES * sizeof(int16_t)) * NUM_RX_BLOCKS / SECTOR_BYTES) ) {
    chprintf(stream, "mmc_write failed (orig) code 0x%02x offset %d\n\r", sd_dbg_val, sd_offset);
    while( retry < 2 ) {
      if( !HAL_SUCCESS == 
	  MMCD1.vmt->write(&MMCD1, sd_offset / SECTOR_BYTES, 
			   (uint8_t *) samples, (NUM_RX_SAMPLES * sizeof(int16_t)) * NUM_RX_BLOCKS / SECTOR_BYTES) ) {
	retry++;
	chprintf(stream, "mmc_write failed (retry %d) 0x%02x\n\r", retry, sd_dbg_val);
      } else {
	chprintf(stream, "mmc_write succeeded on retry\n\r");
	
	sd_offset += NUM_RX_SAMPLES * sizeof(int16_t);
	return;
      }
    }

    // if repeated failure, try to re-init/reconnect to the card
    retry = 0;

    rec_state = REC_RECONNECT;
    redraw_ui();
    chThdSleepMilliseconds(200); // this will cause a stutter, but we're fucked anyways at this point
    while( retry < 5 ) {
      if( !HAL_SUCCESS == MMCD1.vmt->connect(&MMCD1) ) { 
	retry++;
	chThdSleepMilliseconds(500); // this will cause a stutter, but we're fucked anyways at this point
      } else {
	chprintf(stream, "reconnect succeeded on retry %d\n\r", retry + 1);
	rec_state = REC_REC;
	redraw_ui();
	
	sd_offset += NUM_RX_SAMPLES * sizeof(int16_t);
	return;
      }
      chprintf(stream, "couldn't re-connect to MMC\n\r");
      sd_error = 1;
      rec_state = REC_STREAM_ERR; // notify user of need to restart the app
      redraw_ui();
    }
    // return; // actually keep going
    // just keep going and skip the sector???
  }
  
  sd_offset += NUM_RX_SAMPLES * sizeof(int16_t) * NUM_RX_BLOCKS;
}

#define DB_WA_SIZE   THD_WORKING_AREA_SIZE(1280)
THD_FUNCTION(dbThread, arg) {
  (void) arg;
  uint16_t i;
  float cum = 0.0;
  int32_t temp;
  uint16_t samples[NUM_RX_SAMPLES];
  int db = 0;
  float avg = 0.0;

  while (!chThdShouldTerminateX()) {
    avg = 0.0;
    // compute DC offset first -- for actual AGC comps
    for( i = 0; i < NUM_RX_SAMPLES * NUM_RX_BLOCKS; i++ ) {
      avg += (float) rx_savebuf[i];
    }
    avg /= NUM_RX_SAMPLES * NUM_RX_BLOCKS;
    dclog[dclogptr] = avg;
    dclogptr = (dclogptr + 1) % DCLOGLEN;

    avg = 0.0;
    for( i = 0; i < DCLOGLEN; i++ ) {
      avg += dclog[i];
    }
    avg /= (float) DCLOGLEN;
    dc_level = avg;

    // now normalize to a 0-65535 scale, so we can do the power math
    for( i = 0; i < NUM_RX_SAMPLES; i++ ) {
      samples[i] = (uint16_t) (((int32_t) rx_savebuf[i] + 32768L) & 0xFFFF);
    }
    // recompute a local DC offset now that we're normalized...lol!
    cum = 0.0;
    for( i = 0; i < NUM_RX_SAMPLES; i++ ) {
      cum += (float)samples[i];
    }
    int32_t mid = (int32_t) (cum / (float) NUM_RX_SAMPLES);

    // measure total power using RMS method
    cum = 0.0;
    for( i = 0; i < NUM_RX_SAMPLES; i++ ) {
      temp = (((int32_t)samples[i]) - mid);
      cum += (float) (temp * temp);
    }
    cum /= (float) NUM_RX_SAMPLES;
    cum = sqrt(cum);
    db = (int)  (24.0 + 20.0 * log10(cum)); // assumes 120dB is peak value, from AOP on datasheet
    
    dblog[dblogptr] = (uint8_t) db;
    dblogptr = (dblogptr + 1) % DBLOGLEN;

    // average out the instantaneous dB reading
    cum = 0.0;
    for( i = 0; i < DBLOGLEN; i++ ) {
      cum += (float) dblog[i];
    }
    cum /= (float) DBLOGLEN;

    cur_db = (uint16_t) cum;

    // run the agc state machine
    if( agc_state == AGC_NOW_QUIET ) {
      if( (cur_db < VOLUP_QUIET_THRESH) && (agc_gain < MAX_AGC_GAIN) ) {
	agc_gain += AGC_GAIN_STEP;
      }
      if( (cur_db > VOLDOWN_QUIET_THRESH) && (agc_gain > 1.0) ) {
	agc_gain -= AGC_GAIN_STEP;
      }
      if( (cur_db > VOLDOWN_QUIET_THRESH) && agc_gain == 1.0 )
	agc_state = AGC_NOW_LOUD;
      
    } else if( agc_state == AGC_NOW_LOUD ) {
      if( (cur_db < VOLUP_LOUD_THRESH) && (agc_gain < MAX_AGC_GAIN) ) {
	agc_gain += AGC_GAIN_STEP;
      }
      if( (cur_db > VOLDOWN_LOUD_THRESH) && (agc_gain > 1.0) ) {
	agc_gain -= AGC_GAIN_STEP;
      }
      if( (cur_db < VOLUP_LOUD_THRESH) && (agc_gain == MAX_AGC_GAIN) )
	agc_state = AGC_NOW_QUIET;
    }

    if( agc_gain > (MAX_AGC_GAIN - 0.1) ) // clean up rounding errors
      agc_gain = MAX_AGC_GAIN;
    if( agc_gain < 1.1 )
      agc_gain = 1.0;

    if( (agc_gain != last_agc_gain) && agc_enable ) {
      redraw_ui();
    }
    last_agc_gain = agc_gain;

    //    chprintf( stream, "%0.2f %d\n\r", agc_gain, agc_state );
    chThdYield();
    chThdSleepMilliseconds(100);
  }
  
}

static thread_t *dbtp;
static uint32_t rec_init(OrchardAppContext *context) {
  (void)context;
  int i;
  
  sd_error = 0;
  prev_anonymous = anonymous;
  anonymous = 1; // don't send/receive pings when recording
  rec_state = REC_IDLE;

  for( i = 0; i < DCLOGLEN; i++ ) {
    dclog[i] = 0.0;
  }
  dbtp = chThdCreateFromHeap(NULL, DB_WA_SIZE,
					  "decibels", LOWPRIO + 1,
					  dbThread, NULL );
  
  //  chThdSetPriority(NORMALPRIO + 10); // give this thread higher priority
  return 0;
}

static void rec_start(OrchardAppContext *context) {
  (void)context;

#if STOP_FX  
  while(ledsOff == 0) {
    effectsStop();
    chThdYield();
    chThdSleepMilliseconds(50);
  }
#endif

  A_state = 0;
  rec_state = REC_IDLE;
  
  if( !HAL_SUCCESS == MMCD1.vmt->connect(&MMCD1) ) {
    chprintf(stream, "couldn't connect to MMC\n\r");
    sd_error = 1;
    rec_state = REC_ERR;
  }
  
  sd_active = 1;
    
  chThdSleepMilliseconds(100); // wait for any messages to print
  
  palSetPad(IOPORT5, 0); // turn off red LED
  redraw_ui();

  last_update = ST2MS(chVTGetSystemTime());
  //orchardAppTimer(context, 1000 * 1000 * 50, true); //update ui maybe 10 times a second?
  
}

static int select_time = 0;
static struct OrchardUiContext listUiContext;
static const  char title[] = "Really clear state?";

void rec_event(OrchardAppContext *context, const OrchardAppEvent *event) {

  (void)context;
  const OrchardUi *listUi;
  uint8_t selected;

  if (event->type == keyEvent) {
    if( rec_state == REC_IDLE ) {
      palSetPad(IOPORT5, 0); // turn off red LED
      if(event->key.flags == keyDown)  {
	if(event->key.code == keyBottom) {
	  clip_num = (clip_num + 1) % (MAX_CLIPS + 1);  // one off the bottom is for the UI entry to reset record state
	  
	  redraw_ui();
	} else if( event->key.code == keyTop ) {
	  if( clip_num > 0 )
	    clip_num = clip_num - 1;
	  else
	    clip_num = MAX_CLIPS; // one off the bottom is for the UI entry to reset record state
	  
	  redraw_ui();
	} else if (event->key.code == keySelect)  {
	  if( clip_num < MAX_CLIPS ) {
	    palClearPad(IOPORT5, 0); // turn on red LED
	    configClipMarkUsed(clip_num);
	    configFlush();
	    chThdSleepMilliseconds(100); // give some time for clocks to re-stabilize after writing to flash
	    
	    last_update = ST2MS(chVTGetSystemTime());
	    select_time = ST2MS(chVTGetSystemTime());
	    rec_state = REC_REC;
	    sd_offset = clip_offset_bytes[clip_num] + DATA_END_OFFSET;	    
	    redraw_ui();
	    
	    analogUpdateMic();
	  } else if( clip_num == MAX_CLIPS ) {
	    listUi = getUiByName("list");
	    listUiContext.total = 2;  
	    listUiContext.selected = 0;
	    listUiContext.itemlist = (const char **) chHeapAlloc(NULL, sizeof(char *) * 3); // 3 lines incl header
	    if( listUiContext.itemlist == NULL )
	      return;
	
	    listUiContext.itemlist[0] = title;
	    listUiContext.itemlist[1] = "Yes";
	    listUiContext.itemlist[2] = "No";
	
	    if( listUi != NULL ) {
	      context->instance->uicontext = &listUiContext;
	      context->instance->ui = listUi;
	    }
	    listUi->start(context);
	  }
	}
      }
    } else if (rec_state == REC_REC) {
      if(event->key.flags == keyDown)  {
	if (event->key.code == keyRight)  {
	  select_time = ST2MS(chVTGetSystemTime());
	  A_state = 1;
	  chprintf(stream, "A down %d\n\r", select_time);
	  redraw_ui();
	}
	if (event->key.code == keyTop) {
	  agc_enable = !agc_enable;
	  redraw_ui();
	}
      }
      if(event->key.flags == keyUp) {
	if (event->key.code == keyRight)  {
	  A_state = 0;
	  uint32_t curtime = ST2MS(chVTGetSystemTime());
	  chprintf(stream, "A up %d\n\r", curtime - select_time);
	  if( curtime - select_time > 3000 ) {
	    rec_state = REC_IDLE;
	    palSetPad(IOPORT5, 0); // turn off red LED
	  }
	  redraw_ui();
	}
      }
    }
  } else if( event->type == adcEvent) {
    if( event->adc.code == adcCodeMic && rec_state == REC_REC ) {
      update_sd(analogReadMic()); // recording happens in the app thread, not event thread
      analogUpdateMic();
      
      if( (ST2MS(chVTGetSystemTime()) - last_update) > 1500 ) {
	prompt_state = !prompt_state;
	// just flash the LED
	if( prompt_state ) {
	  palClearPad(IOPORT5, 0); // turn on red LED
	} else {
	  palSetPad(IOPORT5, 0); // turn off red LED
	}
	last_update = ST2MS(chVTGetSystemTime());
	
	/*  // UI drawing takes too much CPU time, can't do it concurrently with recording
	redraw_ui();
	last_update = ST2MS(chVTGetSystemTime());*/
      }
    }
  } else if( event->type == timerEvent ) {
    // redraw_ui();
  } else if( event->type == uiEvent ) {
    chHeapFree(listUiContext.itemlist); // free the itemlist passed to the UI
    selected = (uint8_t) context->instance->ui_result;
    context->instance->ui = NULL;
    context->instance->uicontext = NULL;

    if( selected == 0 ) { // this is the "yes" option
      configClipClearMarks();
      chThdSleepMilliseconds(100); // give some time for clocks to re-stabilize after writing to flash
    } else { // no option
      // do nothing
    }
    redraw_ui();
  }
}

static void rec_exit(OrchardAppContext *context) {

  (void)context;
  int retry = 0;

  chThdTerminate(dbtp);
  
  anonymous = prev_anonymous;  // restore ping state
  sd_active = 0;
  
  palSetPad(IOPORT5, 0); // turn off red LED

  chThdSleepMilliseconds(100); // wait for any converions to complete
 
  if( !HAL_SUCCESS == MMCD1.vmt->sync(&MMCD1) ) {
    chprintf(stream, "mmcSync failed\n\r" );
    while( retry < 2 ) {
      if( !HAL_SUCCESS == MMCD1.vmt->sync(&MMCD1) ) {
	retry ++;
	chprintf(stream, "mmcSync failed retry %d\n\r", retry );
      } else {
	break;
      }
    }
  }
  
  //  if( !HAL_SUCCESS == MMCD1.vmt->disconnect(&MMCD1) )
  //    chprintf(stream, "mmcDisconnect failed\n\r");
  chThdSleepMilliseconds(100); // wait for any converions to complete
  
  //  chThdSetPriority(LOWPRIO + 2);
#if STOP_FX
  effectsStart();
#endif
}

orchard_app("Record Clips (beta)", rec_init, rec_start, rec_event, rec_exit);



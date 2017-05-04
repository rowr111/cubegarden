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

#define LED_UI_FONT  "fixed_5x8"
#define REFRACTORY_PERIOD 5000  // timeout for having sex
#define UI_LOCKOUT_TIME 6000  // timeout on friend list sorting/deletion after UI interaction

#define OSCOPE_IDLE_TIME 30000  // 30 seconds of idle, and we switch to an oscilloscope mode UI...because it's pretty
#define OSCOPE_UPDATE_FAIL 500 // 0.5 seconds until auto retrigger of update, in case ui is disrupted

static uint32_t last_ui_time = 0;
static uint32_t last_oscope_time = 0;
static uint32_t last_oscope_update = 0;

static char title[32];
static  const char item1[] = "Yes";
static  const char item2[] = "No";
static struct OrchardUiContext listUiContext;
static char partner[GENE_NAMELENGTH];  // name of current sex partner

static uint8_t mode = 0;  // used by the oscope routine
static uint8_t oscope_running = 0;
static uint16_t *samples;

uint8_t sex_running = 0;
uint8_t sex_done = 0;
uint32_t sex_timer; 

#define SEX_TIMEOUT (8 * 1000)  // 15 seconds for partner to respond before giving up

#define NUM_SAMPLES (NUM_RX_SAMPLES / 4)

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
  uint8_t i;

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
  uint16_t min, max;
  uint16_t i;
  float cum = 0;
  int32_t temp;

  min = 65535; max = 0;
  for( i = 0; i < NUM_RX_SAMPLES; i++ ) {
    if( sample[i] > max )
      max = sample[i];
    if( sample[i] < min )
      min = sample[i];
  }
  int32_t mid = (max + min) / 2;
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

static void do_oscope(void) {
  
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

uint8_t getMutationRate(void) {
  return ((256 / BUMP_LIMIT) * bump_level) + 2;
}

static void do_shaker(void) {
  font_t font;
  coord_t width;
  coord_t fontheight, header_height;
  uint8_t fillwidth, intwidth;
  char tmp[24];

  orchardGfxStart();
  font = gdispOpenFont("fixed_5x8");
  width = gdispGetWidth();
  intwidth = width / BUMP_LIMIT;
  intwidth = intwidth * BUMP_LIMIT;
  fontheight = gdispGetFontMetric(font, fontHeight);
  header_height = fontheight;

  gdispClear(Black);
  
  gdispDrawStringBox(0, 0, width, fontheight,
		     "sex! sex! sex!", font, White, justifyCenter);
  gdispDrawStringBox(0, header_height + 1 * fontheight, width, fontheight,
		     "shake it up!", font, White, justifyCenter);
  chsnprintf(tmp, sizeof(tmp), "Mutation rate: %d%%", (bump_level * 100) / BUMP_LIMIT);
  gdispDrawStringBox(0, header_height + 2 * fontheight, width, fontheight,
		     tmp, font, White, justifyCenter);
  
  gdispDrawBox((width - intwidth) / 2, header_height + 4 * fontheight,
	       intwidth, fontheight, White);

  fillwidth = (bump_level * intwidth) / BUMP_LIMIT;
  gdispFillArea((width - intwidth) / 2, header_height + 4 * fontheight,
		fillwidth, fontheight, White);
  
  gdispCloseFont(font);
  gdispFlush();
  orchardGfxEnd();
}

static void redraw_ui(uint8_t uimode) {
  font_t font;
  coord_t width;
  coord_t fontheight, header_height;
  uint8_t i, starti, j;
  color_t text_color = White;
  color_t bg_color = Black;
  const char **friends;
  char tmp[24];

  // theory: we just need to lockout sorting
  // in the case that a new friend is added, it gets put
  // on the bottom of the list -- so it won't affect our current
  // index state
  // in the case that an existing friend is pinged, that's also
  // fine as the record updates but doesn't change order
  // in the case that a friend record fades out, we would possibly
  // see the list shrink and the current index could become invalid.
  // however, since the list is sorted by ping count, and nearby friends
  // would have a strong ping count, we probably wouldn't see this as
  // a problem -- it would only be trouble if you're trying to have
  // sex with someone at the edge of reception, which would be unreliable
  // anyways and some difficulty in selecting the friend due to fading
  // names would give you a clue of the problem
  if( sex_running ) {
    do_shaker();
    return;
  }
  
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

  orchardGfxStart();

  if( uimode == 1 ) { // timeout
    font = gdispOpenFont("ui2");
    fontheight = gdispGetFontMetric(font, fontHeight);
    width = gdispGetWidth();
    gdispClear(Black);
    gdispDrawStringBox(0, 32, width, fontheight,
		       "DENIED!", font, White, justifyCenter);
    gdispFlush();
    gdispCloseFont(font);
    orchardGfxEnd();
    chThdSleepMilliseconds(1500);
    return;
  } else if ( uimode == 2 ) { // done
    font = gdispOpenFont("ui2");
    fontheight = gdispGetFontMetric(font, fontHeight);
    width = gdispGetWidth();
    gdispClear(Black);
    gdispDrawStringBox(0, 32, width, fontheight,
		       "SUCCESS!", font, White, justifyCenter);
    gdispFlush();
    gdispCloseFont(font);
    orchardGfxEnd();
    chThdSleepMilliseconds(1500);
    return;
  }
  
  font = gdispOpenFont(LED_UI_FONT);
  width = gdispGetWidth();
  fontheight = gdispGetFontMetric(font, fontHeight);
  header_height = fontheight;

  gdispClear(Black);

  // generate the title bar
  gdispFillArea(0, 0, width, header_height - 1, White);
  if( strncmp(effectsCurName(), "Lg", 2) != 0 )
    chsnprintf(tmp, sizeof(tmp), "%s", effectsCurName());
  else {
    chsnprintf(tmp, sizeof(tmp), "%s", lightgeneName());
  }
  gdispDrawStringBox(0, 0, width, header_height,
                     tmp, font, Black, justifyLeft);

  if( friend_total > 0 )
    chsnprintf(tmp, sizeof(tmp), "%d/%d %d %d%%", friend_index + 1, friend_total, 6 - getShift(),
	       ggStateofCharge());
  else 
    chsnprintf(tmp, sizeof(tmp), "b%d %d%%", 6 - getShift(), ggStateofCharge());
    
  gdispDrawStringBox(0, 0, width, header_height,
                     tmp, font, Black, justifyRight);
  

  // generate the friends list
  if( friend_total > 0 ) {
    friendsLock();
    friends = friendsGet();
    starti = friend_index - (friend_index % numlines);
    for( i = starti, j = 0; i < starti + numlines; i++, j++ ) {
      if( friends[i] == NULL )
	continue;
      if( i == friend_index ) {
	text_color = Black;
	bg_color = White;
      } else {
	text_color = White;
	bg_color = Black;
      }
    
      gdispFillArea(0, header_height + j * fontheight, width, fontheight, bg_color);
      gdispDrawStringBox(0, header_height + j * fontheight, width, fontheight,
			 &(friends[i][1]), font, text_color, justifyLeft);

      // 16 is based on the total space available given other UI constraints
      // it will cut the last 4 characters off of some names but improves
      // legibility of the "n of m" motif which is important for navigating lost
      // lists of items
      chsnprintf(tmp, 16, "%d", (int) friends[i][0]);
      gdispDrawStringBox(0, header_height + j * fontheight, width, fontheight,
			 tmp, font, text_color, justifyRight);
    
    }
    friendsUnlock();
  } else {
    if( anonymous ) {
      gdispDrawStringBox(0, header_height + 3 * fontheight, width, fontheight,
			 "Scan disabled", font, text_color, justifyCenter);
      gdispDrawStringBox(0, header_height + 4 * fontheight, width, fontheight,
			 "in silent mode", font, text_color, justifyCenter);
    } else {
      gdispDrawStringBox(0, header_height + 3 * fontheight, width, fontheight,
			 "no friends in range", font, text_color, justifyCenter);
      gdispDrawStringBox(0, header_height + 4 * fontheight, width, fontheight,
			 ":-(", font, text_color, justifyCenter);
    }
  }
  
  gdispCloseFont(font);
  
  gdispFlush();
  orchardGfxEnd();
}

static void confirm_sex(OrchardAppContext *context) {
  const char **friends;
  const OrchardUi *listUi;

  if( friend_total == 0 )
    return;

  friendsLock();
  friends = friendsGet();
  if( friends[friend_index] == NULL ) {
    friendsUnlock();
    return;
  }
  strncpy(partner, friends[friend_index], GENE_NAMELENGTH);
  friendsUnlock();

  listUi = getUiByName("list");
  listUiContext.total = 2;  // two items, yes and no
  listUiContext.selected = 0;
  listUiContext.itemlist = (const char **) chHeapAlloc(NULL, sizeof(char *) * 3); // 3 lines incl header
  if( listUiContext.itemlist == NULL )
    return;

  chsnprintf(title, sizeof(title), "Sex with %s", &(partner[1]));
  
  listUiContext.itemlist[0] = title;
  listUiContext.itemlist[1] = item1;
  listUiContext.itemlist[2] = item2;

  if( listUi != NULL ) {
    context->instance->uicontext = &listUiContext;
    context->instance->ui = listUi;
  }
  listUi->start(context);
  
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

  last_ui_time = chVTGetSystemTime();
  last_oscope_time = chVTGetSystemTime();
  oscope_running = 0;
  sex_running = 0;
  orchardGfxStart();
  // determine # of lines total displayble on the screen based on the font
  font = gdispOpenFont(LED_UI_FONT);
  height = gdispGetHeight();
  fontheight = gdispGetFontMetric(font, fontHeight);
  numlines = (height / fontheight) - 1;   // one for the title bar
  gdispCloseFont(font);
  orchardGfxEnd();
  
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
  
  if (event->type == keyEvent) {
    if (event->key.flags == keyDown) {
      if ( event->key.code == keyBottomR ) {
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
	  mode = (mode + 1) % 3;
	} else {
	  if( friend_total != 0 ) { // we have friends
	    if( strncmp(effectsCurName(), "Lg", 2) == 0 ) { // and we're sporting a genetic light
	      // trigger sex protocol
	      confirm_sex(context);
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
    
    chHeapFree(listUiContext.itemlist); // free the itemlist passed to the UI
    selected = (uint8_t) context->instance->ui_result;
    context->instance->ui = NULL;
    context->instance->uicontext = NULL;

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
	radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_sex_req,
		  GENE_NAMELENGTH * 2 + 2, sexpacket);
      }
#endif
      configIncSexInitiations(); // track # times we tried to have sex
    }
  } else if( event->type == adcEvent) {
    if( oscope_running ) {
      last_oscope_update = chVTGetSystemTime();
      if( event->adc.code == adcCodeMic ) {
	samples = analogReadMic();
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

orchard_app("Blinkies and Sex!", led_init, led_start, led_event, led_exit);



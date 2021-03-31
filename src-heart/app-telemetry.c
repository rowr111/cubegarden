#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include "orchard-app.h"

#include <string.h>

#include "shellcfg.h"

#define SCREEN_WIDTH_CHARS 25 
#define SCREEN_HEIGHT_LINES 7
#define TELEMETRY_STREAM (BaseSequentialStream *)&SD1

#define CARAT '_'

static  uint8_t line = 0;
static  uint8_t position = 0;
static  char buffer[SCREEN_HEIGHT_LINES][SCREEN_WIDTH_CHARS + 1];
  
static void redraw_ui(void) {
  coord_t width;
  coord_t height;
  font_t font;

  orchardGfxStart();
  // draw the title bar
  font = gdispOpenFont("fixed_5x8");
  width = gdispGetWidth();
  height = gdispGetFontMetric(font, fontHeight);

  gdispClear(Black);

  // draw title bar
  gdispClear(Black);
  gdispFillArea(0, 0, width, height, White);

  gdispDrawStringBox(0, 0, width, height,
                     "Cube Telemetry", font, Black, justifyCenter);

  // draw text
  for( int i = 0; i < SCREEN_HEIGHT_LINES; i++ ) {
    gdispDrawStringBox(0, height + i * height, width, height,
		       buffer[i], font, White, justifyLeft);
  }
  
  gdispFlush();
  gdispCloseFont(font);
  orchardGfxEnd();
}

static void otelemetry_start(OrchardAppContext *context) {

  (void)context;

  for( int i = 0; i < SCREEN_HEIGHT_LINES; i++ ) {
    for( int j = 0;  j < SCREEN_WIDTH_CHARS; j++ ) {
      buffer[i][j] = '\0';
    }
  }
  line = 0;
  position = 0;
  buffer[0][0] = CARAT;

  redraw_ui();
  orchardAppTimer(context, 1 * 1000 * 1000, true);
}

void otelemetry_event(OrchardAppContext *context, const OrchardAppEvent *event) {
  (void)context;

  int got_char = 0;
  char c;

  if (event->type == timerEvent) {
    // streamRead(TELEMETRY_STREAM, (uint8_t *) &c, 1)
    while( iqReadTimeout(&SD1.iqueue, (uint8_t *) &c, 1, TIME_IMMEDIATE)  != 0 ) {
      // chprintf(stream, "%c", c);
      got_char = 1;
      if (c == '\n') {
	buffer[line][position] = '\0';
	line = line + 1;
	position = 0;
	line %= SCREEN_HEIGHT_LINES;
	buffer[line][position] = CARAT;
      } else if ( c == '\r' ) {
	for( int i = 0; i < SCREEN_HEIGHT_LINES; i++ ) {
	  for( int j = 0;  j < SCREEN_WIDTH_CHARS; j++ ) {
	    buffer[i][j] = '\0';
	  }
	}
	line = 0;
	position = 0;
      } else {
	if( position < SCREEN_WIDTH_CHARS ) {
	  if( (c >= 0x20) && (c <= 0x7e) ) {
	    buffer[line][position] = c;
	    position += 1;
	    buffer[line][position] = CARAT;
	  } else {
	    buffer[line][position] = '.';
	    // position += 1;  // don't overwhelm the screen with line noise
	  }
	}
      }
    }

    if(got_char != 0)
      redraw_ui();
  } else if (event->type == keyEvent) {
    if( (event->key.code == keyTopR) ) {
      for( int i = 0; i < SCREEN_HEIGHT_LINES; i++ ) {
	for( int j = 0;  j < SCREEN_WIDTH_CHARS; j++ ) {
	  buffer[i][j] = '\0';
	}
      }
      line = 0;
      position = 0;
      buffer[0][0] = CARAT;
      redraw_ui();
    }
  }
}

orchard_app("Cube Telemetry", NULL, otelemetry_start, otelemetry_event, NULL);

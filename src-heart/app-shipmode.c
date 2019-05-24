#include "orchard-app.h"
#include "charger.h"
#include "userconfig.h"

static uint8_t pluggedin = 0;

static void redraw_ui(void) {
  coord_t width;
  coord_t height;
  font_t font;
  color_t text_color;
  color_t bg_color;

  orchardGfxStart();
  // draw the title bar
  font = gdispOpenFont("fixed_5x8");
  width = gdispGetWidth();
  height = gdispGetFontMetric(font, fontHeight);

  // draw title box
  gdispClear(Black);

  if( pluggedin ) {
    gdispDrawStringBox(0, height * 2, width, height,
		       "Can't power down", font, White, justifyCenter);
    gdispDrawStringBox(0, height * 3, width, height,
		       "while charging, unplug", font, White, justifyCenter);
    gdispDrawStringBox(0, height * 4, width, height,
		       "to continue...", font, White, justifyCenter);
  } else {
    gdispDrawStringBox(0, height * 2, width, height,
		       "Press select to", font, White, justifyCenter);
    gdispDrawStringBox(0, height * 3, width, height,
		       "power off, or hold", font, White, justifyCenter);
    gdispDrawStringBox(0, height * 4, width, height,
		       "home to abort...", font, White, justifyCenter);
  }
  
  gdispFlush();
  gdispCloseFont(font);
  orchardGfxEnd();
}

static void shipmode_start(OrchardAppContext *context) {

  (void)context;

  configFlush();  // flush all volatile state before shutting down
  
  redraw_ui();
  orchardAppTimer(context, 125 * 1000 * 1000, true);
}

void shipmode_event(OrchardAppContext *context, const OrchardAppEvent *event) {
  (void)context;

  if (event->type == keyEvent) {
    if ((event->key.flags == keyDown) && (event->key.code == keySelect)) {
      if( !pluggedin )
	chargerShipMode();
    }
  } else if (event->type == timerEvent) {
    pluggedin = isCharging();
    redraw_ui();
  }
}

orchard_app("power off", NULL, shipmode_start, shipmode_event, NULL);

#include "orchard-app.h"
#include "orchard-ui.h"
#include "led.h"

static char *oldfxname;

static uint32_t ostrobe_init(OrchardAppContext *ctx) {
  (void) ctx;

  return 0;
}

static void redraw_ui(void) {
  coord_t width;
  coord_t height;
  font_t font;

  orchardGfxStart();
  font = gdispOpenFont("fixed_5x8");
  width = gdispGetWidth();
  height = gdispGetFontMetric(font, fontHeight);

  gdispClear(Black);
  
  gdispDrawStringBox(0, height*3, width, height,
		     "Press any key to stop!", font, White, justifyCenter);
  
  gdispFlush();
  orchardGfxEnd();
}

static void ostrobe_setup(OrchardAppContext *ctx) {
  orchardAppTimer(ctx, 50 * 1000 * 1000, true);
  redraw_ui();
  oldfxname = (char *) effectsCurName();
  effectsSetPattern(effectsNameLookup("strobe"), 0);
}

static void ostrobe_event(OrchardAppContext *ctx, const OrchardAppEvent *event) {
  (void) ctx;
  
  if (event->type == keyEvent) {
    if( (event->key.flags == keyDown) )
      orchardAppExit(); 
  } else if( event->type == timerEvent ) {
    redraw_ui(); // need this to cover up any prompts that pop up
  }
}

static void ostrobe_exit(OrchardAppContext *ctx) {
  (void)ctx;
  
  effectsSetPattern(effectsNameLookup(oldfxname), 0);
}

orchard_app("Strobe", ostrobe_init, ostrobe_setup, ostrobe_event, ostrobe_exit);

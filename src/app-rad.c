#include "orchard-app.h"
#include "orchard-ui.h"
#include "userconfig.h"
#include "radio.h"

#define NUM_LINES 3
#define LINE_TXBOOST 1
#define LINE_SILENT 2
#define LINE_CHAN 0

static uint8_t line = 0;
uint8_t anonymous = 0;
static struct OrchardUiContext listUiContext;
static const  char title[] = "Pick a channel";

static void redraw_ui(void) {
  char tmp[] = "Settings";
  char uiStr[32];
  
  coord_t width;
  coord_t height;
  font_t font;
  color_t draw_color = White;
  const struct userconfig *config;

  config = getConfig();
  
  orchardGfxStart();
  // draw the title bar
  font = gdispOpenFont("fixed_5x8");
  width = gdispGetWidth();
  height = gdispGetFontMetric(font, fontHeight);

  gdispClear(Black);
  gdispFillArea(0, 0, width, height, White);
  gdispDrawStringBox(0, 0, width, height,
                     tmp, font, Black, justifyCenter);

  // 1st line: Channel
  switch( config->cfg_channel ) {
  case 0:
    chsnprintf(uiStr, sizeof(uiStr), "Channel: Institute");
    break;
  case 1:
    chsnprintf(uiStr, sizeof(uiStr), "Channel: Disorient");
    break;
  case 2:
    chsnprintf(uiStr, sizeof(uiStr), "Channel: Kaos");
    break;
  default:
    chsnprintf(uiStr, sizeof(uiStr), "Channel: Other");
  }
  if( line == LINE_CHAN ) {
    gdispFillArea(0, height*(LINE_CHAN + 1), width, height, White);
    draw_color = Black;
  } else {
    draw_color = White;
  }
  gdispDrawStringBox(0, height * (LINE_CHAN +1), width, height,
		     uiStr, font, draw_color, justifyLeft);

  // 2nd line: Tx boost
  chsnprintf(uiStr, sizeof(uiStr), "Tx boost: %s", config->cfg_txboost ? "on" : "off");
  if( line == LINE_TXBOOST ) {
    gdispFillArea(0, height*(LINE_TXBOOST + 1), width, height, White);
    draw_color = Black;
  } else {
    draw_color = White;
  }
  gdispDrawStringBox(0, height * (LINE_TXBOOST + 1), width, height,
		     uiStr, font, draw_color, justifyLeft);

  // 3rd line: anonymous mode
  chsnprintf(uiStr, sizeof(uiStr), "Silent mode: %s", anonymous ? "on" : "off");
  if( line == LINE_SILENT ) {
    gdispFillArea(0, height*(LINE_SILENT+1), width, height, White);
    draw_color = Black;
  } else {
    draw_color = White;
  }
  gdispDrawStringBox(0, height*(LINE_SILENT+1), width, height,
		     uiStr, font, draw_color, justifyLeft);

  // 4th line: firmware version -- not selectable
  chsnprintf(uiStr, 24, "git: %s", gitversion); // limit length so it doesn't go off screen
  gdispFillArea(0, height*(NUM_LINES+2), width, height, Black);
  gdispDrawStringBox(0, height*(NUM_LINES+2), width, height,
		     uiStr, font, White, justifyLeft);
  
  
  gdispFlush();
  orchardGfxEnd();
}

static uint32_t setting_init(OrchardAppContext *context) {

  (void)context;
  return 0;
}

static void setting_start(OrchardAppContext *context) {

  (void)context;

}

static void setting_event(OrchardAppContext *context, const OrchardAppEvent *event) {

  (void)context;
  const OrchardUi *listUi;
  uint8_t selected;

  if (event->type == keyEvent) {
    if( (event->key.flags == keyDown) && ((event->key.code == keyBottom)) ) {
      line++;
      line %= NUM_LINES;
    } else if( (event->key.flags == keyDown) && ((event->key.code == keyTop)) ) {
      if( line > 0 )
	line--;
      else
	line = NUM_LINES - 1;
    } else if( (event->key.flags == keyDown) && (event->key.code == keySelect) ) {
      if( line == LINE_TXBOOST )
	configToggleBoost();
      else if( line == LINE_SILENT ) {
	anonymous = !anonymous;
	if( anonymous )
	  friendClear();
      } else if( line == LINE_CHAN ) {
	listUi = getUiByName("list");
	listUiContext.total = 4;  
	listUiContext.selected = 0;
	listUiContext.itemlist = (const char **) chHeapAlloc(NULL, sizeof(char *) * 5); // 5 lines incl header
	if( listUiContext.itemlist == NULL )
	  return;
	
	listUiContext.itemlist[0] = title;
	listUiContext.itemlist[1] = "Institute";
	listUiContext.itemlist[2] = "Disorient";
	listUiContext.itemlist[3] = "Kaos";
	listUiContext.itemlist[4] = "Other";
	
	if( listUi != NULL ) {
	  context->instance->uicontext = &listUiContext;
	  context->instance->ui = listUi;
	}
	listUi->start(context);
      }
    }
  } else if( event->type == uiEvent ) {
    chHeapFree(listUiContext.itemlist); // free the itemlist passed to the UI
    selected = (uint8_t) context->instance->ui_result;
    context->instance->ui = NULL;
    context->instance->uicontext = NULL;
    
    // handle channel config
    configSetChannel((uint32_t) selected);
    radioUpdateChannelFromConfig(radioDriver);
    friendClear();
  }

  if( context->instance->ui == NULL ) {
    redraw_ui();
  }
}

static void setting_exit(OrchardAppContext *context) {

  (void)context;
}

orchard_app("Settings", setting_init, setting_start, setting_event, setting_exit);

#include "orchard-app.h"
#include "orchard-ui.h"
#include "userconfig.h"
#include "radio.h"
#include <string.h>

#define NUM_LINES 7
#define LINE_AUTOADV 6
#define LINE_BATT3 5
#define LINE_BATT2 4
#define LINE_BATT1 3
#define LINE_TXBOOST 1
#define LINE_SILENT 2
#define LINE_CHAN 0

static uint8_t line = 0;
uint8_t anonymous = 0;
static uint32_t batt1 = 0;
static uint32_t batt2 = 0;
static uint32_t batt3 = 0;
static uint32_t autoadv = 0;
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

  // 4th line: first battery brightness threshold
  chsnprintf(uiStr, sizeof(uiStr), "bright_thresh: %d", batt1);
  if( line == LINE_BATT1 ) {
    gdispFillArea(0, height*(LINE_BATT1+1), width, height, White);
    draw_color = Black;
  } else {
    draw_color = White;
  }
  gdispDrawStringBox(0, height*(LINE_BATT1+1), width, height,
		     uiStr, font, draw_color, justifyLeft);

  // 5th line: 2nd battery brightness threshold
  chsnprintf(uiStr, sizeof(uiStr), "bright_thresh2: %d", batt2);
  if( line == LINE_BATT2 ) {
    gdispFillArea(0, height*(LINE_BATT2+1), width, height, White);
    draw_color = Black;
  } else {
    draw_color = White;
  }
  gdispDrawStringBox(0, height*(LINE_BATT2+1), width, height,
		     uiStr, font, draw_color, justifyLeft);

  // 6th line: 3rd battery brightness threshold
  chsnprintf(uiStr, sizeof(uiStr), "bright_thresh3: %d", batt3);
  if( line == LINE_BATT3 ) {
    gdispFillArea(0, height*(LINE_BATT3+1), width, height, White);
    draw_color = Black;
  } else {
    draw_color = White;
  }
  gdispDrawStringBox(0, height*(LINE_BATT3+1), width, height,
		     uiStr, font, draw_color, justifyLeft);

  // 7th line: 3rd battery brightness threshold
  chsnprintf(uiStr, sizeof(uiStr), "fx auto adv: %d", autoadv);
  if( line == LINE_AUTOADV ) {
    gdispFillArea(0, height*(LINE_AUTOADV+1), width, height, White);
    draw_color = Black;
  } else {
    draw_color = White;
  }
  gdispDrawStringBox(0, height*(LINE_AUTOADV+1), width, height,
		     uiStr, font, draw_color, justifyLeft);

  // 10th line: firmware version -- not selectable
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
  const struct userconfig *config;
  config = getConfig();
  batt1 = config->cfg_bright_thresh;
  batt2 = config->cfg_bright_thresh2;
  batt3 = config->cfg_bright_thresh3;
  autoadv = config->cfg_autoadv;
}

#define MTUNE_RETRIES 5
#define MTUNE_RETRY_DELAY 37
static void setting_event(OrchardAppContext *context, const OrchardAppEvent *event) {

  (void)context;
  const OrchardUi *listUi;
  uint8_t selected;

  if (event->type == keyEvent) {
    if( (event->key.flags == keyDown) && ((event->key.code == keyBottom)) ) {
      line++;
      line %= NUM_LINES;
    } else if( (event->key.flags == keyDown) && ((event->key.code == keyTop)) ) {
      if( line > 0 ) line--;
      else line = NUM_LINES - 1;
    } else if( (event->key.flags == keyDown) && (event->key.code == keySelect) ) {
      if( line == LINE_TXBOOST ) configToggleBoost();
      else if( line == LINE_SILENT ) {
        anonymous = !anonymous;
        if( anonymous ) friendClear();
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
    } else if( (event->key.flags == keyDown) && ((event->key.code == keyTopR) || (event->key.code == keyBottomR)) ) {
      // this is the "A" key or "B" key
      // if "B" key transmit all the params
      char effect_cmd[128];
      int i;
      if(line == LINE_BATT1 || event->key.code == keyBottomR) {
	for( i = 0; i < MTUNE_RETRIES; i++ ) {
	  chsnprintf(effect_cmd, sizeof(effect_cmd), "tune bright1 %d", batt1);
	  radioAcquire(radioDriver);
	  radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_forward, strlen(effect_cmd) + 1, effect_cmd);
	  radioRelease(radioDriver);
	  chThdSleepMilliseconds(MTUNE_RETRY_DELAY);	  
	
	}
      }
      if(line == LINE_BATT2 || event->key.code == keyBottomR) {
	for( i = 0; i < MTUNE_RETRIES; i++ ) {
	  chsnprintf(effect_cmd, sizeof(effect_cmd), "tune bright2 %d", batt2);
	  radioAcquire(radioDriver);
	  radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_forward, strlen(effect_cmd) + 1, effect_cmd);
	  radioRelease(radioDriver);
	  chThdSleepMilliseconds(MTUNE_RETRY_DELAY);	  
	
	}
      }
      if(line == LINE_BATT3 || event->key.code == keyBottomR) {
	for( i = 0; i < MTUNE_RETRIES; i++ ) {
	  chsnprintf(effect_cmd, sizeof(effect_cmd), "tune bright3 %d", batt3);
	  radioAcquire(radioDriver);
	  radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_forward, strlen(effect_cmd) + 1, effect_cmd);
	  radioRelease(radioDriver);
	  chThdSleepMilliseconds(MTUNE_RETRY_DELAY);	  
	}
      }
    }


    else if(event->key.code == keyRight){
       if(line == LINE_BATT1) {
          batt1 = batt1 >= 4300 ? 4300 : batt1 + 1; //must be less than max of 4300
        }
       if(line == LINE_BATT2){
          batt2 = batt2 >= batt1 ? batt1 - 1 : batt2 + 1; //must be less than batt1
        }
       if(line == LINE_BATT3){
           batt3 = batt3 >= batt2 ? batt2 - 1 : batt3 + 1; //must be less than batt2
       }
       if(line == LINE_AUTOADV){
         autoadv = autoadv + 1;
       }
  }
  else if(event->key.code == keyLeft){
       if(line == LINE_BATT1) {
          batt1 = batt1 <= batt2 ? batt2 + 1 : batt1 - 1; //must be > batt2
       }
       if(line == LINE_BATT2){
          batt2 = batt2 <= batt3 ? batt3 + 1 : batt2 - 1; //must be > batt3
       }
       if(line == LINE_BATT3){
           batt3 = batt3 == 0 ? 0 : batt3 - 1; // must be > 0
       }
       if(line == LINE_AUTOADV){
          autoadv = autoadv == 0 ? 0 : autoadv - 1; //must be > 0
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
  const struct userconfig *config;
  config = getConfig();

  if(batt1 != config->cfg_bright_thresh){
    configSetBrightThresh(batt1);
  }
  if(batt2 != config->cfg_bright_thresh2){
    configSetBrightThresh2(batt2);
  }
  if(batt3 != config->cfg_bright_thresh3){
    configSetBrightThresh3(batt3);
  }
    if(autoadv != config->cfg_autoadv){
    configSetAutoAdv(autoadv);
  }
}

orchard_app("Settings", setting_init, setting_start, setting_event, setting_exit);

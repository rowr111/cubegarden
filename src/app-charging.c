#include "orchard-app.h"
#include "orchard-ui.h"

#include "charger.h"
#include "radio.h"

#include <string.h>
#include <stdlib.h>

static int32_t celcius;

// flag argument should probably be turned into an enum in a refactor
// flag = 0 means normal UI
// flag = 1 means first-time UI (indicate ADC is measuring)
// flag = 2 means confirm override
static void redraw_ui(uint8_t flag) {
  char tmp[] = "System stats";
  char uiStr[32];
  
  coord_t width;
  coord_t height;
  font_t font;
  color_t draw_color = White;

  orchardGfxStart();
  // draw the title bar
  font = gdispOpenFont("fixed_5x8");
  width = gdispGetWidth();
  height = gdispGetFontMetric(font, fontHeight);

  gdispClear(Black);
  gdispFillArea(0, 0, width, height, White);
  gdispDrawStringBox(0, 0, width, height,
                     tmp, font, Black, justifyCenter);

  // 1st line: CPU temp
  if( flag == 1 ) {
    chsnprintf(uiStr, sizeof(uiStr), "CPU Temp: measuring...");
  } else {
    chsnprintf(uiStr, sizeof(uiStr), "CPU Temp: %d.%dC", celcius / 1000, 
	       abs((celcius % 1000) / 100));
  }
  gdispDrawStringBox(0, height, width, height,
		     uiStr, font, White, justifyLeft);


  // 2nd line left: Radio temp
  chsnprintf(uiStr, sizeof(uiStr), "Uptime: %dh %dm %ds", uptime / 3600, (uptime / 60) % 60, uptime % 60);
  gdispDrawStringBox(0, height*2, width, height,
		     uiStr, font, White, justifyLeft);
  
  // 3rd line left
  chsnprintf(uiStr, sizeof(uiStr), "Volts: %dmV", ggVoltage());
  gdispDrawStringBox(0, height*3, width, height,
		     uiStr, font, White, justifyLeft);

  // 4th line left
  chsnprintf(uiStr, sizeof(uiStr), "Est capacity: %d%%", ggStateofCharge());
  gdispDrawStringBox(0, height*4, width, height,
		     uiStr, font, White, justifyLeft);

  // 5th line left
  chsnprintf(uiStr, sizeof(uiStr), "Chg stat: %s", chgStat());
  gdispDrawStringBox(0, height*5, width, height,
		     uiStr, font, White, justifyLeft);

  // 6th line left
  chsnprintf(uiStr, sizeof(uiStr), "Fault: %s", chgFault());
  gdispDrawStringBox(0, height*6, width, height,
		     uiStr, font, White, justifyLeft);
  // 7th line left
  chsnprintf(uiStr, sizeof(uiStr), "CRC fails: %d", crcfails);  // this is from the radio subsystem
  gdispDrawStringBox(0, height*7, width, height,
		     uiStr, font, White, justifyLeft);

  gdispFlush();
  orchardGfxEnd();
}

static uint32_t charging_init(OrchardAppContext *context) {

  (void)context;

  return 0;
}

static void charging_start(OrchardAppContext *context) {
  
  analogUpdateTemperature(); 
  orchardAppTimer(context, 250 * 1000 * 1000, true); // update UI every 500 ms
  
  redraw_ui(1);
  // all this app does is launch a text entry box and store the name
}

void charging_event(OrchardAppContext *context, const OrchardAppEvent *event) {

  (void)context;
  
  if (event->type == keyEvent) {
    if ( (event->key.flags == keyDown) && (event->key.code == keyLeft) ) {
      // orchardAppExit();  this is probably a misfeature
    }
  } else if (event->type == timerEvent) {
    analogUpdateTemperature(); // the actual value won't update probably until the UI is redrawn...
    redraw_ui(0);
  } else if( event->type == adcEvent) {
    celcius = analogReadTemperature();
  }
}

static void charging_exit(OrchardAppContext *context) {

  (void)context;
}

orchard_app("System stats", charging_init, charging_start, charging_event, charging_exit);



#include <stdlib.h>
#include <string.h>

#include "hal.h"

#include "chprintf.h"
#include "gfx.h"
#include "oled.h"
#include "ui.h"

virtual_timer_t ui_vt;
event_source_t ui_timer_event;

ui_battery  uibat;
ui_input    uiinput;

void uiHandler(eventid_t id) {
  (void) id;

  coord_t width, fontheight;
  coord_t fontlinespace;
  coord_t cur_line = 0;
  font_t font;
  font_t font_small;
  char str[32];
  char substr[8];
  int i;

  oledGfxStart();
  width = gdispGetWidth();
  
  gdispClear(Black);

  font = gdispOpenFont("fixed_5x8"); // can fit 32 chars wide
  fontheight = gdispGetFontMetric(font, fontHeight);
  chsnprintf(str, sizeof(str), " %4dmV              %3d%%", uibat.batt_mv, uibat.batt_soc / 10 );
  gdispDrawStringBox(0, 0, width, fontheight,
		     str, font, White, justifyLeft);
  cur_line += fontheight;
  gdispCloseFont(font);
    
  font = gdispOpenFont("fixed_7x14"); // can fit 18 chars wide
  fontheight = gdispGetFontMetric(font, fontHeight);
  fontlinespace = fontheight - 4; // line space a bit closer than the total height of font reported

  chsnprintf(substr, sizeof(substr), "woot");
  chsnprintf(str, sizeof(str), "%s: %dms", substr, ST2MS(chVTGetSystemTime()));
  gdispFillStringBox(0, cur_line, width, fontlinespace, str, font, Black, White, justifyCenter);
  cur_line += fontlinespace;
  gdispCloseFont(font);
  
  gdispFlush();
  oledGfxEnd();
  
}

static void ui_cb(void *arg) {
  (void) arg;
  chSysLockFromISR();
  chEvtBroadcastI(&ui_timer_event);
  chVTSetI(&ui_vt, MS2ST(60), ui_cb, NULL);
  chSysUnlockFromISR();
}

void uiStart(void) {
  int i;
  
  chEvtObjectInit(&ui_timer_event);
  chVTSet(&ui_vt, MS2ST(60), ui_cb, NULL);

}

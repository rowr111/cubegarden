#include <stdlib.h>
#include <string.h>

#include "hal.h"

#include "chprintf.h"
#include "gfx.h"
#include "oled.h"
#include "ui.h"

virtual_timer_t ui_vt;
event_source_t ui_timer_event;

ui_monitor  uimon;
ui_config   uicfg;
ui_battery  uibat;
ui_input    uiinput;

#define STATE_MON 0
#define STATE_CFG 1
static uint8_t ui_state = STATE_MON;
static uint8_t state_line = 1;
#define CONFIG_LINES 2

void uiHandler(eventid_t id) {
  (void) id;

  coord_t width, fontheight;
  coord_t fontlinespace;
  coord_t cur_line = 0;
  font_t font;
  char str[32];

  // this should go in a separate monitor timer thread, eventually -- just for demo now
  uimon.dark_time = ST2MS(chVTGetSystemTime());
  
  // rising edge detect on uiinput.a
  if( uiinput.a == 1 ) {
    if( ui_state == STATE_MON )
      ui_state = STATE_CFG;
    else
      ui_state = STATE_MON;

    uiinput.a = 0;
  }
  
  oledGfxStart();
  width = gdispGetWidth();
  
  if( ui_state == STATE_MON ) {
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

    if( uimon.status == UI_DARK )
      chsnprintf(str, sizeof(str), "Status: DARK");
    else
      chsnprintf(str, sizeof(str), "Status: LIVE");
    gdispDrawStringBox(0, cur_line, width, cur_line + fontheight,
		       str, font, White, justifyLeft);
    cur_line += fontlinespace;

    chsnprintf(str, sizeof(str), "Dark for: %dh%dm%ds", (uimon.dark_time / 3600000),
	       (uimon.dark_time / 60000) % 60, (uimon.dark_time / 1000) % 60 );
    gdispDrawStringBox(0, cur_line, width, cur_line + fontheight,
		       str, font, White, justifyLeft);
    cur_line += fontlinespace;

    chsnprintf(str, sizeof(str), "gps: %s cell: %s", uimon.gps_ok == UI_OK ? " OK" : "BAD",
	       uimon.cell_ok == UI_OK ? " OK " : "BAD" );
    gdispDrawStringBox(0, cur_line, width, cur_line + fontheight,
		       str, font, White, justifyLeft);
    cur_line += fontlinespace;
    
    chsnprintf(str, sizeof(str), "bt : %s wifi: %s", uimon.bt_ok == UI_OK ? " OK" : "BAD",
	       uimon.wifi_ok == UI_OK ? " OK " : "BAD" );
    gdispDrawStringBox(0, cur_line, width, cur_line + fontheight,
		       str, font, White, justifyLeft);
    
    
  } else if( ui_state == STATE_CFG ) {
    if( uiinput.down == 1 ) {
      state_line++;
      if( state_line == (CONFIG_LINES + 1) )
	state_line = 1;
      uiinput.down = 0;
    }
    if( uiinput.up == 1 ) {
      state_line--;
      if( state_line == 0 )
	state_line = CONFIG_LINES;
      uiinput.up = 0;
    }
    
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
    fontlinespace = fontheight; // line space a bit closer than the total height of font reported
    
    // put cfg ui here
    //// TODO: add self-test UI here
    if( state_line == 1) {
      gdispDrawString(0, cur_line, ">", font, White);
      if( uiinput.right == 1 ) {
	touch_debounce = MS2ST(2000);
	if(uicfg.simsel == 1) {
	  // action here
	  palSetPad(IOPORT1, 19); // sim OUT
	  palSetPad(IOPORT3, 9); // set SIM_SEL, selecting sim2
	  chThdSleepMilliseconds(1000);
	  palClearPad(IOPORT1, 19); // sim IN
	  uicfg.simsel = 2;
	} else { 
	  // action here
	  palSetPad(IOPORT1, 19); // sim OUT
	  palClearPad(IOPORT3, 9); // clear SIM_SEL, selecting sim1
	  chThdSleepMilliseconds(1000);
	  palClearPad(IOPORT1, 19); // sim IN
	  uicfg.simsel = 1;
	}
	uiinput.right = 0;
      }
    }
    gdispDrawString(0, cur_line, " SIM: ", font, White);
    if( uicfg.simsel == 1 ) {
      gdispFillString(gdispGetStringWidth(" SIM: ", font) , cur_line, "SIM1", font, Black, White);
      gdispFillString(gdispGetStringWidth(" SIM: SIM1 ", font) , cur_line, "SIM2", font, White, Black);
    } else {
      gdispFillString(gdispGetStringWidth(" SIM: ", font) , cur_line, "SIM1", font, White, Black);
      gdispFillString(gdispGetStringWidth(" SIM: SIM1 ", font) , cur_line, "SIM2", font, Black, White);
    }
    
    cur_line += fontlinespace;

    if( state_line == 2) {
      gdispDrawString(0, cur_line, ">", font, White);
      if( uiinput.right == 1 ) {
	if(uicfg.alarmon == 1) {
	  // action here
	  uicfg.alarmon = 0;
	} else {
	  // action here
	  uicfg.alarmon = 1;
	}
	uiinput.right = 0;
      }
    }
    gdispDrawString(0, cur_line, " Alarm: ", font, White);
    if( uicfg.alarmon == 1 ) {
      gdispFillString(gdispGetStringWidth(" Alarm: ", font) , cur_line, "ON", font, Black, White);
      gdispFillString(gdispGetStringWidth(" Alarm: ON ", font) , cur_line, "OFF", font, White, Black);
    } else {
      gdispFillString(gdispGetStringWidth(" Alarm: ", font) , cur_line, "ON", font, White, Black);
      gdispFillString(gdispGetStringWidth(" Alarm: ON ", font) , cur_line, "OFF", font, Black, White);
    }
    cur_line += fontlinespace;
    
    gdispCloseFont(font);
  }
  
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
  chEvtObjectInit(&ui_timer_event);
  chVTSet(&ui_vt, MS2ST(60), ui_cb, NULL);
}

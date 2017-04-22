#include "orchard-ui.h"
#include <string.h>

// mutex to lock the graphics subsystem for safe multi-threaded drawing
mutex_t orchard_gfxMutex;
ui_battery uibat;
ui_input uiinput;

const OrchardUi *getUiByName(const char *name) {
  const OrchardUi *current;

  current = orchard_ui_start();
  while(current->name) {
    if( !strncmp(name, current->name, 16) ) {
      return current;
    }
    current++;
  }
  return NULL;
}

void uiStart(void) {
  
}

void orchardGfxStart(void) {

  osalMutexLock(&orchard_gfxMutex);
  
}

void orchardGfxEnd(void) {
  
  osalMutexUnlock(&orchard_gfxMutex);
  
}

// used to draw UI prompts for tests to the screen
// print up to 2 lines of text
// interaction_delay specifies how long we should wait before we declare failure
//   0 means don't delay
//   negative numbers delay that amount of seconds, but don't print a timeout failure
//    (meant for prompts that need to be read, but not interacted with)
OrchardTestResult orchardTestPrompt(char *line1, char *line2,
                                    int8_t interaction_delay) {
  coord_t width;
  coord_t height;
  font_t font;
  uint32_t val;
  char timer[16];
  uint32_t starttime;
  uint32_t curtime, updatetime;
  OrchardTestResult result = orchardResultUnsure;
  uint8_t countdown;

  val = captouchRead();
  countdown = (uint8_t) abs(interaction_delay);

  orchardGfxStart();
  font = gdispOpenFont("ui2");
  width = gdispGetWidth();
  height = gdispGetFontMetric(font, fontHeight);

  gdispClear(Black);

  gdispDrawStringBox(0, height * 2, width, height,
                     line1, font, White, justifyCenter);

  gdispDrawStringBox(0, height * 3, width, height,
                     line2, font, White, justifyCenter);

  if( interaction_delay != 0 ) {
    chsnprintf(timer, sizeof(timer), "%d", countdown);
    gdispDrawStringBox(0, height * 4, width, height,
                       timer, font, White, justifyCenter);
    countdown--;
  }

  gdispFlush();

  starttime = chVTGetSystemTime();
  updatetime = starttime + 1000;
  if (interaction_delay != 0) {
    while(1) {
      curtime = chVTGetSystemTime();
      if ((val != captouchRead())) {
        result = orchardResultPass;
        break;
      }
      if ((curtime - starttime) > ((uint32_t) abs(interaction_delay) * 1000)) {
        result = orchardResultFail;
        break;
      }

      if (curtime > updatetime) {
        chsnprintf(timer, sizeof(timer), "%d", countdown);
        gdispFillArea(0, height * 4, width, height, Black);

        gdispDrawStringBox(0, height * 4, width, height,
               timer, font, White, justifyCenter);
        gdispFlush();
        countdown--;
        updatetime += 1000;
      }
    }
  }

  if (result == orchardResultFail) {
    if( interaction_delay >= 0 ) {
      chsnprintf(timer, sizeof(timer), "timeout!");
      gdispFillArea(0, height * 4, width, height, Black);
      
      gdispDrawStringBox(0, height * 4, width, height,
                       timer, font, White, justifyCenter);

      gdispFlush();
      chThdSleepMilliseconds(2000);
    }
  }

  gdispCloseFont(font);
  orchardGfxEnd();

  return result;
}

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

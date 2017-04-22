#ifndef __ORCHARD_UI_H__
#define __ORCHARD_UI_H__

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "shell.h"
#include "orchard.h"
#include "orchard-events.h"
#include "gfx.h"

struct _OrchardUi;
typedef struct _OrchardUi OrchardUi;
struct _OrchardAppContext;
typedef struct _OrchardAppContext OrchardAppContext;

// used to define lists of items for passing to UI elements
// for selection lists, the definition is straightfoward
// for dialog boxes, the first list item is the message; the next 1 or 2 items are button texts
// lists are linked lists, as we don't know a priori how long the lists will be
typedef struct OrchardUiContext {
  unsigned int selected;
  unsigned int total;
  const char **itemlist;
} OrchardUiContext;

typedef struct _OrchardUi {
  char *name;
  void (*start)(OrchardAppContext *ui);
  void (*event)(OrchardAppContext *ui, const OrchardAppEvent *event);
  void (*exit)(OrchardAppContext *ui);
} OrchardUi;

#define orchard_ui_start()                                                   \
({                                                                            \
  static char start[0] __attribute__((unused,                                 \
    aligned(4), section(".chibi_list_ui_1")));                               \
  (const OrchardUi *)&start;                                                 \
})

#define orchard_ui(_name, _start, _event, _exit)                        \
  const OrchardUi _orchard_ui_list_##_start##_event##_exit           \
  __attribute__((unused, aligned(4), section(".chibi_list_ui_2_" # _start # _event # _exit))) =  \
     { _name, _start, _event, _exit }

#define orchard_ui_end()                                                     \
  const OrchardUi _orchard_ui_list_final                                    \
  __attribute__((unused, aligned(4), section(".chibi_list_ui_3_end"))) =     \
     { NULL, NULL, NULL, NULL }


#define TEXTENTRY_MAXLEN  19  // maximum length of any entered text, not including null char

const OrchardUi *getUiByName(const char *name);

void uiStart(void);
void orchardGfxStart(void);
void orchardGfxEnd(void);

typedef struct ui_battery {
  unsigned int batt_soc; // divide by 10 to get into %
  unsigned int batt_mv;
  unsigned int chg_state;
} ui_battery;

typedef struct ui_input {
  unsigned int up;
  unsigned int up_last;
  unsigned int down;
  unsigned int down_last;
  unsigned int left;
  unsigned int left_last;
  unsigned int right;
  unsigned int right_last;
  unsigned int a;
  unsigned int a_last;
  unsigned int b;
  unsigned int b_last;
} ui_input;

extern ui_battery uibat;
extern ui_input uiinput;

void orchardGfxStart(void);
void orchardGfxEnd(void);

typedef enum _OrchardTestResult {
  orchardResultPass = 0,
  orchardResultFail = 1,
  orchardResultUnsure = 2,
  orchardResultNoTest = 3,  // for cases where there is no test for a function
} OrchardTestResult;


#endif /* __ORCHARD_UI_H__ */

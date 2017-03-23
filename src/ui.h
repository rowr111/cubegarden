#define STUCK_TIMEOUT MS2ST(2000)  // two second max time for stuckage
#define TOUCH_DEBOUNCE MS2ST(300)
extern unsigned int touch_debounce;

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

void uiStart(void);
void uiHandler(eventid_t id);

extern ui_battery uibat;
extern ui_input uiinput;


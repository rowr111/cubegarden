#define UI_DARK 0
#define UI_LIVE 1

#define UI_NOT_OK 0
#define UI_OK 1

#define STUCK_TIMEOUT MS2ST(2000)  // two second max time for stuckage
#define TOUCH_DEBOUNCE MS2ST(300)
extern unsigned int touch_debounce;

// data to be displayed on the main status monitor
typedef struct ui_monitor {
  unsigned int status;  // can be dark or live
  unsigned int dark_time; // time dark in ms
  unsigned int gps_ok;
  unsigned int gps_time;
  unsigned int cell_ok;
  unsigned int cell_time;
  unsigned int wifi_ok;
  unsigned int wifi_time;
  unsigned int bt_ok;
  unsigned int bt_time;
} ui_monitor;

typedef struct ui_config {
  unsigned int simsel;
  unsigned int alarmon;
  unsigned int selftest;
} ui_config;

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
extern ui_monitor uimon;
extern ui_config uicfg;
extern ui_input uiinput;

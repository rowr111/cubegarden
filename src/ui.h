#define UI_DARK 0
#define UI_LIVE 1
#define UI_ALRM 2
#define UI_NOTE 3
#define UI_LIVE_DEBOUNCE 4

#define UI_NOT_OK 0
#define UI_OK 1

#define STUCK_TIMEOUT MS2ST(2000)  // two second max time for stuckage
#define TOUCH_DEBOUNCE MS2ST(300)
extern unsigned int touch_debounce;

#define TIME_STATES 5
extern unsigned int time_map[TIME_STATES]; // time options, in seconds

// data to be displayed on the main status monitor
typedef struct ui_monitor {
  unsigned int gps_ok;  // indicates self-test pass or fail
  unsigned int cell_ok;
  unsigned int wifi_ok;
  unsigned int bt_ok;
  unsigned int status;     // options are: LIVE DARK ALRM NOTE
  systime_t state_entry_time;
  systime_t state_debounce_time;
} ui_monitor;

#define LOGLEN 96
#define LOGMAX 65535
typedef struct ui_graph {
  uint16_t gps_events[LOGLEN];
  uint16_t bt_events[LOGLEN];
  uint16_t wifi_events[LOGLEN];
  uint16_t cell_events[LOGLEN];
  systime_t last_log_time;  // kept as system time -- need to use ST2MS to get real time
  uint8_t log_index; // current index for new log entries
  mutex_t log_mutex;
} ui_graph;

#define NOTIFY_TIME 800
typedef struct ui_config {
  unsigned int simsel;
  unsigned int selftest;
  unsigned int notifyon;  // transition notification (short beep into/out of dark)
  unsigned int alarmon;   // alarm on (longer alarm coming out of dark)
  unsigned int alarmtime; // duration of alarm, index into time_map
  unsigned int darkdelay; // time till auto-dark, index into time_map
  uint32_t log_interval;  // nominally once every 2000ms
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
extern ui_graph uigraph;

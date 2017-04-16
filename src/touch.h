void touchStart(void);
void touchHandler(eventid_t id);
void touchCb(EXTDriver *extp, expchannel_t channel);
uint8_t captouchRead(void);

extern event_source_t touch_event;
extern uint8_t touch_state;

#define TOUCH_DEBOUNCE MS2ST(300)
#define STUCK_TIMEOUT MS2ST(3000)  // three second max time for stuckage

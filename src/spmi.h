void spmiCb(EXTDriver *extp, expchannel_t channel);
void spmiStart(void);
void spmiHandler(eventid_t id);

extern event_source_t spmi_event;
extern uint32_t spmiEventCount;
extern uint32_t spmiLastTime;
extern uint32_t spmiElapsedTime;

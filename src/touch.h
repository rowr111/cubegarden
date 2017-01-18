void touchStart(void);
void touchHandler(eventid_t id);
void touchCb(EXTDriver *extp, expchannel_t channel);

extern event_source_t touch_event;

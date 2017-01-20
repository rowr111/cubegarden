void gpsCb(EXTDriver *extp, expchannel_t channel);
void gpsStart(void);
void gpsHandler(eventid_t id);

extern event_source_t gps_event;
extern uint32_t gpsEventCount;
extern uint32_t gpsLastTime; // in ms
extern uint32_t gpsElapsedTime; // elapsed time in ms since last GPS event

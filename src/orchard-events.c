#include "ch.h"
#include "hal.h"
#include "orchard-events.h"

event_source_t rf_pkt_rdy;
event_source_t radio_page;
event_source_t radio_sex_req;
event_source_t radio_sex_ack;
event_source_t radio_app;

void orchardEventsStart(void) {

  chEvtObjectInit(&rf_pkt_rdy);

  chEvtObjectInit(&radio_page);
  chEvtObjectInit(&radio_sex_req);
  chEvtObjectInit(&radio_sex_ack);
  chEvtObjectInit(&radio_app);
  
}

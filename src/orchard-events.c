#include "ch.h"
#include "hal.h"
#include "orchard-events.h"

event_source_t celcius_rdy;
event_source_t rf_pkt_rdy;

event_source_t radio_page;
event_source_t radio_sex_req;
event_source_t radio_sex_ack;
event_source_t radio_app;

event_source_t accel_bump;

void orchardEventsStart(void) {

  // ADC-related events
  chEvtObjectInit(&celcius_rdy);

  // radio protocol events
  chEvtObjectInit(&rf_pkt_rdy);
  
  chEvtObjectInit(&radio_page);
  chEvtObjectInit(&radio_sex_req);
  chEvtObjectInit(&radio_sex_ack);
  chEvtObjectInit(&radio_app);
  
  // accel events
  chEvtObjectInit(&accel_bump);
}

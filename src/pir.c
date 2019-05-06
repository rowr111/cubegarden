#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "orchard.h"

#include "pir.h"

event_source_t pir_process;

// IRQ contexts are special, so basically, we do as little as possible
// in them. Here, we lock the system and put an event in the queue which
// a user-mode process can dispatch into a more advanced handler
void pir_irq(EXTDriver *extp, expchannel_t channel) {
  (void)extp;
  (void)channel;
  
  chSysLockFromISR();
  chEvtBroadcastI(&pir_process);
  chSysUnlockFromISR();
}

void pirStart(void) {
  chEvtObjectInit(&pir_process);
}

/*
  This is the main area where PIR-related code should go. It runs in
  user-mode so we can use all sorts of functions here.
 */
void pir_proc(eventid_t id) {

  (void)id;
  chprintf(stream, "PIR event\n\r");
}

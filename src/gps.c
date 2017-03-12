#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include "gps.h"
#include "ui.h"

#define NL SHELL_NEWLINE_STR

event_source_t gps_event;
uint32_t gpsEventCount;
uint32_t gpsLastTime;
uint32_t gpsElapsedTime;

void gpsCb(EXTDriver *extp, expchannel_t channel) {
  (void)extp;
  (void)channel;

  chSysLockFromISR();
  chEvtBroadcastI(&gps_event);
  chSysUnlockFromISR();
}

void gpsHandler(eventid_t id) {
  (void) id;
  uint32_t temp;

  gpsEventCount++;
  chMtxLock(&uigraph.log_mutex);
  if( uigraph.gps_events[uigraph.log_index] < LOGMAX )
    uigraph.gps_events[uigraph.log_index]++;
  chMtxUnlock(&uigraph.log_mutex);
  
  temp = gpsLastTime;
  gpsLastTime = ST2MS(chVTGetSystemTime());
  gpsElapsedTime = gpsLastTime - temp;
}

void gpsStart(void) {
  chEvtObjectInit(&gps_event);
  gpsEventCount = 0;
  gpsElapsedTime = 0;
  gpsLastTime = ST2MS(chVTGetSystemTime());
}

void gpsCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: gps [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    stat  current GPS monitoring status"NL);
    return;
  }
  
  if (!strcasecmp(argv[0], "stat")) {
    chprintf(chp, "gps events: %d"NL, gpsEventCount );
    chprintf(chp, "gps last time: %d"NL, gpsLastTime );
    chprintf(chp, "gps elapsed time: %d"NL, gpsElapsedTime );
  }
}

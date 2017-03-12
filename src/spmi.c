#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include "spmi.h"
#include "ui.h"

#define NL SHELL_NEWLINE_STR

event_source_t spmi_event;
uint32_t spmiEventCount;
uint32_t spmiLastTime;
uint32_t spmiElapsedTime;

#define SPMI_FIFO_DEPTH 256
uint16_t spmi_buf[SPMI_FIFO_DEPTH]; // total depth of FIFO
static uint16_t spmi_wr = 0;
static uint16_t spmi_rd = 0;
static uint8_t spmi_overflow = 0;
static uint8_t spmi_underflow = 0;

// write should be greater than read, e.g., write = 2, read = 0 means depth 2; write = 0, read = 2 means depth 254
uint8_t spmiFifoFull(void) {
  if( spmi_rd > 0 )
    return(spmi_wr == (spmi_rd - 1));
  else
    return(spmi_wr == (SPMI_FIFO_DEPTH - 1));
}

uint8_t spmiFifoEmpty(void) {
  return( (spmi_wr == spmi_rd) ); // empty when wr == rd
}

uint16_t spmiFifoEntries(void) {
  if( spmi_wr >= spmi_rd ) {
    return( (uint16_t) (spmi_wr - spmi_rd) );
  } else {
    return( (uint16_t) (SPMI_FIFO_DEPTH - spmi_rd + spmi_wr) );
  }
}

static void spmi_fifo_wr(uint16_t data) {
  if( !spmiFifoFull() ) {
    spmi_buf[spmi_wr] = data;
    spmi_wr++;
    if( spmi_wr == SPMI_FIFO_DEPTH )
      spmi_wr = 0;
  } else {
    spmi_overflow = 1;
  }
}

static uint16_t spmi_fifo_rd(void) {
  uint16_t retval;
  if( !spmiFifoEmpty() ) {
    retval = spmi_buf[spmi_rd];
    spmi_rd++;
    if( spmi_rd == SPMI_FIFO_DEPTH )
      spmi_rd = 0;
  } else {
    spmi_underflow = 1;
    retval = -1;
  }

  return retval;
}

uint16_t spmiFifoRd(void) {
  return spmi_fifo_rd();
}

void spmiCb(EXTDriver *extp, expchannel_t channel) {
  (void)extp;
  (void)channel;

  chSysLockFromISR();
  chEvtBroadcastI(&spmi_event);
  chSysUnlockFromISR();
}

void spmiStart(void) {
  chEvtObjectInit(&spmi_event);
  spmiEventCount = 0;
  spmiElapsedTime = 0;
  spmiLastTime = ST2MS(chVTGetSystemTime());
}

void spmiHandler(eventid_t id) {
  (void) id;
  uint32_t temp;
  uint16_t tx[3];
  uint16_t rx[3];

  spmiEventCount++;
  chMtxLock(&uigraph.log_mutex);
  if( uigraph.cell_events[uigraph.log_index] < LOGMAX )
    uigraph.cell_events[uigraph.log_index]++;
  chMtxUnlock(&uigraph.log_mutex);
  
  temp = spmiLastTime;
  spmiLastTime = ST2MS(chVTGetSystemTime());
  spmiElapsedTime = spmiLastTime - temp;

  while(palReadPad(IOPORT4, 7) == PAL_HIGH && !spmiFifoFull()) {
  // if( !spmiFifoFull() ) {
    spiSelect(&SPID1);
    // depracated protocol
    //    tx[0] = 0x102;   // read command
    //    tx[1] = 0xface;  // dummy bytes in tx, we only care about the rx
    tx[0] = 0xface; // dummy byte
    spiStartExchange(&SPID1, 1, tx, rx);
    while(SPID1.state != SPI_READY)
      chThdYield();
    spiUnselect(&SPID1);
    
    spmi_fifo_wr(rx[0]);
  }
  
}

void spmiCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: spmi [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    stat  current SPMI monitoring status"NL);
    chprintf(chp, "    dump  dump SPMI FIFO"NL);
    return;
  }
  
  if (!strcasecmp(argv[0], "stat")) {
    chprintf(chp, "spmi events: %d"NL, spmiEventCount );
    chprintf(chp, "spmi last time: %d"NL, spmiLastTime );
    chprintf(chp, "spmi elapsed time: %d"NL, spmiElapsedTime );
    chprintf(chp, "spmi fifo depth: %d"NL, spmiFifoEntries() );
  } else if(!strcasecmp(argv[0], "dump")) {
    int i;
    uint16_t entries = spmiFifoEntries();
    uint16_t data;
    for( i = 0; i < entries; i ++ ) {
      if( ( i % 8 ) == 0 )
	chprintf(chp, NL"%3d: ", i );
      data = spmiFifoRd();
      chprintf(chp, "%2d.%04x ", (data >> 12) & 0xE, data & 0x1FFF);
    }
    chprintf(chp, NL);
  }
  
}

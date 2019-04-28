#include "ch.h"
#include "hal.h"

#include "orchard.h"

#include "mic.h"

#include <string.h>
#include <limits.h>

#include "shell.h"
#include "chprintf.h"
#include "orchard-test.h"
#include "test-audit.h"

#include "orderedmem.h"

//int32_t rx_samples[NUM_RX_SAMPLES];
extern int32_t __ram1_start__[];
//int32_t *rx_samples = __ram1__;
#define rx_samples __ram1_start__

int16_t rx_savebuf[NUM_RX_SAMPLES * NUM_RX_BLOCKS];
uint8_t rx_block = 0;

uint32_t rx_cb_count = 0;

uint8_t gen_mic_event = 0;

volatile I2S_TypeDef *I2S = I2S0; // pointer to I2S hardware bank

extern event_source_t i2s_full_event;

int rx_dma_count = 0;

#define I2S_RX_WATERMARK 4
#define DMA_I2S 2
#define DMA_BACKBUFFER 3

OSAL_IRQ_HANDLER(KINETIS_DMA3_IRQ_VECTOR) {
  int i;
  
  OSAL_IRQ_PROLOGUE();

  writeb( &DMA->CINT, DMA_BACKBUFFER );
  rx_dma_count++;

  // grab a copy of the data
  for( i = 0; i < NUM_RX_SAMPLES; i++ ) {
    // !! we actually throw away a couple of bits, >> 14 would give us everything the mic gives us,
    // but we're limited on space and CPU so toss the LSBs...
    rx_savebuf[i + rx_block * NUM_RX_SAMPLES] = (int16_t) (rx_samples[i] >> 16);
  }

  // re-enable the requests now that the back-buffer is copied
  writeb( &DMA->SERQ, DMA_BACKBUFFER );

  rx_block ++;
  if( rx_block == NUM_RX_BLOCKS ) {
    rx_block = 0;
    // kick out an event to write data to disk
    chSysLockFromISR();
    chEvtBroadcastI(&i2s_full_event);
    chSysUnlockFromISR();
  }

  OSAL_IRQ_EPILOGUE();
}

void micStart(void) {
  volatile DMA_TCD_TypeDef *tcd;
  volatile DMA_TCD_TypeDef *tcd2;

  // enable clocks
  SIM->SCGC6 |= 0x8000; // turn on I2S
  SIM->SCGC6 |= SIM_SCGC6_DMAMUX;
  SIM->SCGC7 |= SIM_SCGC7_DMA;

  // setup sampling rate (32kHz)
  writel( &I2S->MCR, 0x40000000 ); // setup the clocks
  writel( &I2S->MDR, 0x3db70 );  // 0xf9b70 = 32kHz, 0x7cb70 = 16kHz (technically out of spec), 0x3dcb70 = 8khz

  // configure receive framing
  writel( &I2S->RCR1, I2S_RX_WATERMARK ); // watermark
  writel( &I2S->RCR2, 0x07000001 ); // clock config
  writel( &I2S->RCR4, 0x11f1b ); // fifo and framing
  writel( &I2S->RCR5, 0x1f1f1f00 ); // word widths
  writel( &I2S->RMR, 0x2 ); // receive masking

  // setup receive DMA
  tcd = &(DMA->TCD[DMA_I2S]); // channel 2 is I2S
  tcd2 = &(DMA->TCD[DMA_BACKBUFFER]); // channel 3 is backbuffer

  // disable DMA requests
  writeb( &DMA->CERQ, DMA_I2S ); // dma channel 2 is for rx
  writeb( &DMA->CERQ, DMA_BACKBUFFER ); // dma channel 3 is for backbuffer copying

  // writel( &DMA->CR, readl(&DMA->CR) | DMA_CR_ERCA_MASK ); // round-robin priority
  writeb(&DMA->DCHPRI0, 0xC1); // dummy setting, cannot preempt or suspend
  writeb(&DMA->DCHPRI1, 0xC2); // dummy setting, cannot preempt or suspend 
  writeb(&DMA->DCHPRI2, 0x00); // cannot suspend a lower priority channel; cannot be suspended by others
  writeb(&DMA->DCHPRI3, 0xC3); // back-buffer copy cannot suspend a lower priority channel, can be suspended

  // configure the I2S read TCD
  writel( &tcd->SADDR, (uint32_t) &(I2S->RDR[0]) ); // source is read fifo
  writew( &tcd->SOFF, 0); // don't increment when reading from fifo
  writel( &tcd->SLAST, 0); // dont change after last address

  writel( &tcd->DADDR, (uint32_t) rx_samples );  // destination is RAM
  writew( &tcd->DOFF, 4 ); 
  //  writel( &tcd->DLASTSGA, -(NUM_RX_SAMPLES * sizeof(uint32_t)) );
  writel( &tcd->DLASTSGA, 0 );
  
  writew( &tcd->ATTR, (DMA_ATTR_SSIZE(2) | DMA_ATTR_DSIZE(2) | DMA_ATTR_DMOD(10)) ); // 32-bit source and dest size

  writel( &tcd->NBYTES_MLNO, I2S_RX_WATERMARK * sizeof(uint32_t) );  // minor loop count:

  writew( &tcd->CITER_ELINKNO, NUM_RX_SAMPLES / I2S_RX_WATERMARK ); // major loop count
  writew( &tcd->BITER_ELINKNO, NUM_RX_SAMPLES / I2S_RX_WATERMARK ); // these two have to match
  
  // configure the back buffer copy TCD
  writel( &tcd2->SADDR, (uint32_t) rx_samples );
  writew( &tcd2->SOFF, 4 );
  //writel( &tcd2->SLAST, -(NUM_RX_SAMPLES * sizeof(uint32_t)) );
  writel( &tcd2->SLAST, 0 );

  writel( &tcd2->DADDR, (uint32_t) rx_samples + 1024 ); // double-buffer location
  writew( &tcd2->DOFF, 4 );  
  //  writel( &tcd2->DLASTSGA, -(NUM_RX_SAMPLES * sizeof(uint32_t)) ); //-1024;
  writel( &tcd2->DLASTSGA, 0 ); //-1024;
  
  writew( &tcd2->ATTR, (DMA_ATTR_SSIZE(2) | DMA_ATTR_DSIZE(2) | DMA_ATTR_DMOD(10) | DMA_ATTR_SMOD(10)) ); 

  writel( &tcd2->NBYTES_MLNO, NUM_RX_SAMPLES );  // minor loop count

  writew( &tcd2->CITER_ELINKNO, 1 ); // major loop count
  writew( &tcd2->BITER_ELINKNO, 1 ); // these two have to match

  // configure the DMA mux
  writeb( &DMAMUX->CHCFG[DMA_I2S], (DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(12)) ); // i2s rx (channel 12) -> DMA channel 2
  
  // enable interrupt to dump back buffer to processing buffer
  nvicEnableVector(DMA3_IRQn, KINETIS_I2S_RX_DMA_PRIORITY);
  chprintf( stream, "DMA Rx Priority: %d\n", KINETIS_I2S_RX_DMA_PRIORITY );

  //tcd->CSR = DMA_CSR_INTMAJOR_MASK | DMA_CSR_DREQ_MASK | DMA_CSR_BWC(2);
  // configure channel 2 (i2s) DMA engine to link request to the back-buffer copy, and turn on bandwidth control
  writew( &tcd->CSR, DMA_CSR_MAJORLINKCH(DMA_BACKBUFFER) | DMA_CSR_MAJORELINK(1) /* | DMA_CSR_BWC(2) */ );

  // configure channel 3 (backbuffer copy) DMA engine to fire interrupts when major loop is done
  // requests disabled on major loop complete, int handler re-enables
  writew( &tcd2->CSR, DMA_CSR_INTMAJOR_MASK | DMA_CSR_BWC(2) | DMA_CSR_DREQ_MASK ); // some LED flicker seen if this BWC isn't on

  // now enable the requests to happen
  writeb( &DMA->SERQ, DMA_BACKBUFFER ); // first enable back buffer copy
  writeb( &DMA->SERQ, DMA_I2S ); // now enable the I2S copy

  // now enable the I2S engine proper, data should auto-flow at this point
  writel( &I2S->RCR3, 0x00010000 ); // receive channel enable
  writel( &I2S->RCSR, 0x90100401 ); // starts the rx
}

void analogUpdateMic(void) {
  gen_mic_event = 1;
}

int16_t *analogReadMic(void) {
    return rx_savebuf;
}


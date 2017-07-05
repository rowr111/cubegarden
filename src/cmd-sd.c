/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "orchard.h"
#include "shell.h"
#include "chprintf.h"

#include "shell.h"

//#define DATA_OFFSET 1024   // as sector number
#define DATA_OFFSET 0   // as sector number

extern MMCDriver MMCD1;

#if 0
uint32_t test_dest[512];
uint32_t test_source[4];

void mon_dma(void) {
  int i;

  for( i = 0; i < 512; i++ ) {
    if( i % 8 == 0 )
      chprintf(stream, "\n\r");
    chprintf(stream, "%8x ", test_dest[i]);
  }
  chprintf(stream, "\n\r");
  chprintf(stream, "rx_dma_count: %d\n\r", rx_dma_count);
}

void trigger_dma(volatile DMA_TCD_TypeDef *tcd) {
  int i;
  
  // DMA->ERQ |= 0x2; // enable channel 1 DMA requests
  /// now for the triggering
  DMA->SERQ = 1; // a write of 1 causes ERQ of DMA channel 1 to be set
  DMA->SSRT = 0x1; // set start on TCD for channel 1

  i = 0;
  while( i < 512/4 ) {
    DMA->SSRT = 0x1; // set start on TCD for channel 1
    chThdSleepMilliseconds(1);
    test_source[0] += 1;
    i++;
  }

  //  char dma_mux0_i2srx_req_src = 12;
  // enable the DMA mux
  //  DMAMUX_CHCFG1 |= (DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(dma_mux0_i2srx_req_src));

#if 0
  I2SD1->RCSR |= I2S_TCSR_FRDE_MASK; // request DMA enable on I2S
  // and then finally enable the Rx module, which is done elsewhere
#endif
}

void try_dma(void) {
  volatile DMA_TCD_TypeDef *tcd;
  int i;

  SIM->SCGC6 |= SIM_SCGC6_DMAMUX;
  SIM->SCGC7 |= SIM_SCGC7_DMA;
  
  for( i = 0; i < 4; i++ ) {
    test_source[i] = i + 0xbabe;
  }
  for( i = 0; i < 512; i++ ) {
    test_dest[i] = 0;
  }
  for( i = 0; i < 512; i++ ) {
    if( i % 8 == 0 )
      chprintf(stream, "\n\r");
    chprintf(stream, "%8x ", test_dest[i]);
  }
  chprintf(stream, "\n\r");

  tcd = &(DMA->TCD[1]);

  DMA->ERQ &= ~0x2; // disable channel 1 DMA requests
  
  DMA->CR |= DMA_CR_EMLM_MASK; // enable minor looping -- I /think/ this is the right thing to do

  // EDMA_DRV_ConfigLoopTransfer call
  tcd->SADDR = test_source;
  tcd->SOFF = 0; // will be 0 for reading from the FIFO
  
  tcd->SLAST = 0;
  tcd->DLASTSGA = 0;
  
  tcd->ATTR = DMA_ATTR_SSIZE(2) | DMA_ATTR_DSIZE(2); // 32-bit source and dest size
  //tcd->ATTR = DMA_ATTR_SSIZE(0) | DMA_ATTR_DSIZE(0); // 32-bit source and dest size

  tcd->NBYTES_MLNO = 16;  // minor loop count
  //  tcd->NBYTES_MLOFFYES = 0xC0004010;
  
  tcd->DADDR = test_dest;
  tcd->DOFF = 4; 

  tcd->CITER_ELINKNO = 512/16; // major loop count
  tcd->BITER_ELINKNO = 512/16; // these two have to match
  // tcd->CITER_ELINKNO = 1; // major loop count
  // tcd->BITER_ELINKNO = 1; // these two have to match
  
  //  tcd->CSR = 0; // clear initially
  //  tcd->CSR |= DMA_CSR_INTMAJOR_MASK; // generate DMA interrupt on completion of major loop
  //  tcd->CSR &= ~DMA_CSR_DREQ_MASK; // automatically disable the dreq when major loop is done
  //  tcd->CSR &= ~DMA_CSR_ESG_MASK;
  tcd->CSR = DMA_CSR_INTMAJOR_MASK | DMA_CSR_DREQ_MASK;
  //  tcd->SLAST = -16; // will be 0 for reading from the FIFO
  //  tcd->NBYTES_MLOFFYES = 0xc000_0010;
  //  tcd->DLASTSGA = -512 * sizeof(uint32_t); 
  // end EDMA_DRV_ConfigLoopTransfer

  nvicEnableVector(DMA1_IRQn, 8); // enable the interrupt before firing

  trigger_dma(tcd);
}
#endif

void cmd_sd(BaseSequentialStream *chp, int argc, char *argv[])
{
  BlockDeviceInfo bdip;
  uint32_t i;
  uint8_t *block;

  (void)argv;
  if (argc <= 0) {
    chprintf(chp, "Usage: sd [verb]:"SHELL_NEWLINE_STR);
    //    chprintf(chp, "    mount     mount drive"SHELL_NEWLINE_STR);
    //    chprintf(chp, "    unmount   unmount drive"SHELL_NEWLINE_STR);
    chprintf(chp, "    cid       dump card ID"SHELL_NEWLINE_STR);
    //    chprintf(chp, "    r         read some sectors (testing)"SHELL_NEWLINE_STR);
    //    chprintf(chp, "    dma       dma test (testing)"SHELL_NEWLINE_STR);
    //    chprintf(chp, "    mon       monitor dma test (testing)"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "cid")) {
    if( !HAL_SUCCESS == MMCD1.vmt->connect(&MMCD1) )
      chprintf(chp, "mmcConnect() failed\n\r");
    if( !HAL_SUCCESS == MMCD1.vmt->get_info(&MMCD1, &bdip) )
      chprintf(chp, "mmcGetInfo() failed\n\r");

    chprintf(chp, "Capacity: %d\n\r", bdip.blk_num );
    chprintf(chp, "Block size: %d\n\r", bdip.blk_size );

    chprintf(chp, "CSD: \n\r");
    for( i = 0; i < 4; i++ )
    chprintf(chp, "%08x ", MMCD1.csd[i]);
  
    chprintf(chp, "\n\rCID: \n\r");
    for( i = 0; i < 4; i++ ) 
      chprintf(chp, "%08x ", MMCD1.cid[i]);
    chprintf(chp, "\n\r" );
    
    block = chHeapAlloc(NULL, sizeof(uint8_t) * MMCSD_BLOCK_SIZE * 1);

    if( !HAL_SUCCESS == MMCD1.vmt->read(&MMCD1, DATA_OFFSET, block, 1) )
      chprintf(chp, "mmc_read failed\n\r");

    if( !HAL_SUCCESS == MMCD1.vmt->sync(&MMCD1) )
      chprintf(chp, "mmcSync failed\n\r" );

    if( !HAL_SUCCESS == MMCD1.vmt->disconnect(&MMCD1) )
      chprintf(chp, "mmcDisconnect failed\n\r");

    for( i = 0; i < MMCSD_BLOCK_SIZE * 1; i++ ) {
      if( (i % 16) == 0 )
	chprintf(chp, "\n\r %3x: ", i );
      chprintf(chp, "%02x ", block[i]);
    }
    chprintf(chp, "\n\r");
    chHeapFree(block);
  }
#if 0
  else if (!strcasecmp(argv[0], "w")) {
    if( !HAL_SUCCESS == MMCD1.vmt->connect(&MMCD1) )
      chprintf(chp, "mmcConnect() failed\n\r");
    if( !HAL_SUCCESS == MMCD1.vmt->get_info(&MMCD1, &bdip) )
      chprintf(chp, "mmcGetInfo() failed\n\r");

    chprintf(chp, "Capacity: %d\n\r", bdip.blk_num );
    chprintf(chp, "Block size: %d\n\r", bdip.blk_size );
    
    block = chHeapAlloc(NULL, sizeof(uint8_t) * MMCSD_BLOCK_SIZE * 1);

    for( i = 0; i < MMCSD_BLOCK_SIZE * 1; i++ ) {
      block[i] = (uint8_t) i;
    }
    if( !HAL_SUCCESS == MMCD1.vmt->write(&MMCD1, DATA_OFFSET, block, 1) )
      chprintf(chp, "mmc_write failed\n\r");
    
    if( !HAL_SUCCESS == MMCD1.vmt->sync(&MMCD1) )
      chprintf(chp, "mmcSync failed\n\r" );

    if( !HAL_SUCCESS == MMCD1.vmt->disconnect(&MMCD1) )
      chprintf(chp, "mmcDisconnect failed\n\r");

    chHeapFree(block);
  }

  else if (!strcasecmp(argv[0], "r")) {
    block = chHeapAlloc(NULL, sizeof(uint8_t) * MMCSD_BLOCK_SIZE * 1);

    if( !HAL_SUCCESS == MMCD1.vmt->read(&MMCD1, DATA_OFFSET, block, 1) )
      chprintf(chp, "mmc_read failed\n\r");
    
    for( i = 0; i < MMCSD_BLOCK_SIZE * 1; i++ ) {
      if( (i % 16) == 0 )
	chprintf(chp, "\n\r %3x: ", i );
      chprintf(chp, "%02x ", block[i]);
    }
    chprintf(chp, "\n\r");
    chHeapFree(block);
  }
  
  else if (!strcasecmp(argv[0], "mount")) {
    if( !HAL_SUCCESS == MMCD1.vmt->connect(&MMCD1) )
      chprintf(chp, "mmcConnect() failed\n\r");
    if( !HAL_SUCCESS == MMCD1.vmt->get_info(&MMCD1, &bdip) )
      chprintf(chp, "mmcGetInfo() failed\n\r");

    chprintf(chp, "Capacity: %d\n\r", bdip.blk_num );
    chprintf(chp, "Block size: %d\n\r", bdip.blk_size );

    chprintf(chp, "CSD: \n\r");
    for( i = 0; i < 4; i++ )
      chprintf(chp, "%08x ", MMCD1.csd[i]);
    
    chprintf(chp, "\n\rCID: \n\r");
    for( i = 0; i < 4; i++ ) 
      chprintf(chp, "%08x ", MMCD1.cid[i]);
    chprintf(chp, "\n\r" );
  }

  else if (!strcasecmp(argv[0], "unmount")) {
    if( !HAL_SUCCESS == MMCD1.vmt->sync(&MMCD1) )
      chprintf(chp, "mmcSync failed\n\r" );

    if( !HAL_SUCCESS == MMCD1.vmt->disconnect(&MMCD1) )
      chprintf(chp, "mmcDisconnect failed\n\r");
  }
#endif
  
#if 0
  else if (!strcasecmp(argv[0], "dma")) {
    try_dma();
  }
  else if (!strcasecmp(argv[0], "mon")) {
    mon_dma();
  }
#endif
}

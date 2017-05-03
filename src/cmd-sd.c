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

#include "hal.h"
#include "ch.h"
#include "shell.h"
#include "chprintf.h"

#include "shell.h"

//#define DATA_OFFSET 1024   // as sector number
#define DATA_OFFSET 0   // as sector number

extern MMCDriver MMCD1;

void cmd_sd(BaseSequentialStream *chp, int argc, char *argv[])
{
  BlockDeviceInfo bdip;
  uint32_t i;
  uint8_t *block;

  (void)argv;
  if (argc <= 0) {
    chprintf(chp, "Usage: sd [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    mount     mount drive"SHELL_NEWLINE_STR);
    chprintf(chp, "    unmount   unmount drive"SHELL_NEWLINE_STR);
    chprintf(chp, "    cid       dump card ID"SHELL_NEWLINE_STR);
    chprintf(chp, "    r         read some sectors (testing)"SHELL_NEWLINE_STR);
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
}

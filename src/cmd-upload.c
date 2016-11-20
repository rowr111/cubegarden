#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "xmodem.h"

#include "fpga_config.h"

#define NL SHELL_NEWLINE_STR

#define driver &SD4
#define SPIDEV &SPID2

// clocked off of bus clock @ 50MHz
// BR(0) -> /2, PBR() -> /2, final speed is 50/4 = 12.5MHz
#define KINETIS_SPI_TAR_BUSCLK_XZ(n)\
    SPIx_CTARn_FMSZ((n) - 1) | \
    SPIx_CTARn_CPOL | \
    SPIx_CTARn_CPHA | \
    SPIx_CTARn_DBR | \
    SPIx_CTARn_PBR(0) | \
    SPIx_CTARn_BR(0) | \
    SPIx_CTARn_CSSCK(0) | \
    SPIx_CTARn_ASC(0) | \
    SPIx_CTARn_DT(0)

__attribute__((aligned(4))) static uint8_t spi_data[256];
static uint32_t spi_data_count;

// spi1 interface is "runty" and has only a 4-deep FIFO, so not suitable for DMA
// returns bytes transferred

void spiRuntSetup(SPIDriver *spip) {
  nvicDisableVector(DMA1_IRQn); // disable IRQs coz we're polling this one
  
  spip->spi->MCR = SPIx_MCR_MSTR | SPIx_MCR_CLR_TXF | SPIx_MCR_CLR_RXF;
  spip->spi->CTAR[0] = KINETIS_SPI_TAR_BUSCLK_XZ(8);  // 8-bit frame size
}

uint32_t spiRuntSend(SPIDriver *spip, uint32_t count, uint8_t *data) {
  uint32_t i = 0;
  uint32_t pushr;

  // poll loop to send data
  while( i < count ) {
    while ( (spip->spi->SR & SPI_SR_TFFF_MASK) && (i < count) ) {
      if( i == 0 ) // mark first with clear transfer count
	pushr = SPI_PUSHR_CONT_MASK | SPI_PUSHR_CTCNT_MASK | data[i];
      else
	pushr = SPI_PUSHR_CONT_MASK | data[i];
      
      spip->spi->PUSHR = pushr;
      i++;
      
      //      while( !(spip->spi->SR & SPI_SR_TCF_MASK) )
      //	; // wait for transfer to complete, because TFFF lies
      while( (spip->spi->TCR >> 16) != i )
	; // wait for transfer to complete
    }
  }

  // note that checking TCF isn't good enough -- the CPU runs fast enough
  // that TCR doesn't update for a few cycles after TCF and you'll get
  // an out of date TCR that's short the last transfer
  // so we just wait until the TCR reflects the actual amount received
  
  while( (spip->spi->TCR >> 16) != count )
    ; // wait for transfer to complete
  
  return (spip->spi->TCR >> 16);
}

void fpgaProgramPage(SPIDriver *spidev, uint32_t page, uint32_t spi_data_count, uint8_t *spi_data) {
  (void) page;
  uint32_t sent;
  uint8_t dummy[8];
  
  spiSelect(spidev);
  sent = spiRuntSend(spidev, spi_data_count, spi_data);
  chprintf((BaseSequentialStream *)&SD4, "uploaded %d bytes to FPGA"NL, sent);

  sent = spiRuntSend(spidev, 8, dummy);
  chprintf((BaseSequentialStream *)&SD4, "sent %d dummy bytes to activate IO"NL, sent);
  spiUnselect(spidev);
}

void uploadCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  
  uint8_t last_sequence_id = 0;
  int retries = 0;
  int finished = 0;
  int page = 0;
  uint32_t bytes_written = 0;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: upload [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    go        Start upload"SHELL_NEWLINE_STR);
    chprintf(chp, "    test      send a test packet"SHELL_NEWLINE_STR);
    return;
  }

  if (!strcasecmp(argv[0], "go")) {
    xmodemSendByte(driver, XMODEM_NAK, 200);

    while (!finished) {
      uint8_t sequence_id;
      int ret;

      ret = xmodemReadBlock(driver, spi_data + spi_data_count, sizeof(spi_data), &sequence_id);
      
      if (ret < 0) {
	retries++;
	xmodemSendByte(driver, XMODEM_NAK, 200);
	continue;
      }
      xmodemSendByte(driver, XMODEM_ACK, 200);
      
      /* Pad out the final packet with 0xff */
      if (ret == 0) {
	while (spi_data_count <= sizeof(spi_data))
	  spi_data[spi_data_count++] = 0xff;
	finished = 1;
      }
      
      if (retries >= 500)
	finished = 1;

      if (sequence_id == last_sequence_id) {
	continue;
      }

      last_sequence_id = sequence_id;
      
      spi_data_count += ret;
      if (spi_data_count >= sizeof(spi_data)) {
	fpgaProgramPage(SPIDEV, page++, spi_data_count, spi_data);
	//	spinorProgramPage(SPIDEV, page++, spi_data_count, spi_data);
	bytes_written += spi_data_count;
	spi_data_count = 0;
      }
    }
  }

  else if (!strcasecmp(argv[0], "test")) {
    //    memcpy(spi_data, _binary_fpga_bin_start, 8);
    chprintf(chp, "FPGA config area size %d"NL, fpga_config_size);
    fpgaProgramPage(SPIDEV, 0, fpga_config_size, (uint8_t *) fpga_config);
  }
  
  else {
    chprintf(chp, "Unrecognized command: %s"NL, argv[0]);
  }
  
}


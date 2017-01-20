#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#define NL SHELL_NEWLINE_STR

static const SerialConfig serialConfig = {
  3125100,
};

#define SER_RX 0
#define SER_TX 1
#define BT_SD    (&SD1)
#define WIFI_SD  (&SD2)
#define BB_SD    (&SD3)

typedef struct serial_monitor {
  uint8_t wifi_txrx;
  uint8_t bt_txrx;
  uint8_t bb_txrx;
  SerialDriver *cl_uart;  // UART selected for command line monitoring
} serial_monitor;
serial_monitor sermon;

int isprint(int c) {
  return ((c >= ' ' && c <= '~') ? 1 : 0);
}

void serMonStart(void) {
  sdStart(&SD1, &serialConfig);
  sdStart(&SD2, &serialConfig);
  sdStart(&SD3, &serialConfig);
  
  palSetPad(IOPORT5, 0); // configure to monitor the Rx; 1 is Tx
  sermon.wifi_txrx = SER_TX;
  
  palClearPad(IOPORT2, 16); // configure to monitor the Rx; 1 is Tx
  sermon.bt_txrx = SER_RX;

  palClearPad(IOPORT4, 2); // configure to monitor the Rx; 1 is Tx
  sermon.bb_txrx = SER_RX;

  // start by monitoring wifi
  sermon.cl_uart = WIFI_SD;
}

static int should_stop(void) {
  uint8_t bfr[1];
  return chnReadTimeout(&SD4, bfr, sizeof(bfr), 1);
}

void serialCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  uint8_t buf[64];
  uint8_t numbytes = 0;
  int i;
  
  if (argc <= 0) {
    chprintf(chp, "Usage: ser [verb]:"SHELL_NEWLINE_STR);
    chprintf(chp, "    test  test command"NL);
    chprintf(chp, "    mon   print data sensed on serial line"NL);
    chprintf(chp, "    swap  swap tx/rx sense of current uart"NL);
    chprintf(chp, "    wifi  select wifi uart"NL);
    chprintf(chp, "    bb    select bb uart"NL);
    chprintf(chp, "    bt    select bt uart"NL);
    return;
  }

  if (!strcasecmp(argv[0], "test")) {
    chprintf(chp, "test"NL );
  }

  if (!strcasecmp(argv[0], "mon")) {
    if( sermon.cl_uart == BT_SD ) {
      chprintf(chp, "monitoring BT ");
      if( sermon.bt_txrx == SER_RX )
	chprintf(chp, "RX"NL);
      else
	chprintf(chp, "TX"NL);
    } else if( sermon.cl_uart == BB_SD ) {
      chprintf(chp, "monitoring BB ");
      if( sermon.bb_txrx == SER_RX )
	chprintf(chp, "RX"NL);
      else
	chprintf(chp, "TX"NL);
    } else if( sermon.cl_uart == WIFI_SD ) {
      chprintf(chp, "monitoring Wifi ");
      if( sermon.wifi_txrx == SER_RX )
	chprintf(chp, "RX"NL);
      else
	chprintf(chp, "TX"NL);
    }
    while( !should_stop() ) {
      numbytes = chnReadTimeout(sermon.cl_uart, buf, 63, TIME_IMMEDIATE);
      if( numbytes ) {
	for( i = 0; i < numbytes; i++ ) { // iterate through because null characters should print through
	  chprintf(chp, "%c", isprint(buf[i]) ? buf[i] : '.' );
	}
      }
    }
  }

  if (!strcasecmp(argv[0], "swap")) {
    if( sermon.cl_uart == WIFI_SD ) {
      if( sermon.wifi_txrx == SER_RX ) {
	chprintf(chp, "Was monitoring wifi RX"NL );
	palSetPad(IOPORT5, 0);
	sermon.wifi_txrx = SER_TX;
	chprintf(chp, "Now monitoring wifi TX"NL );
      } else {
	chprintf(chp, "Was monitoring wifi TX"NL );
	palClearPad(IOPORT5, 0);
	sermon.wifi_txrx = SER_RX;
	chprintf(chp, "Now monitoring wifi RX"NL );
      }
    } else if( sermon.cl_uart == BT_SD ) {
      if( sermon.bt_txrx == SER_RX ) {
	chprintf(chp, "Was monitoring BT RX"NL );
	palSetPad(IOPORT2, 16);
	sermon.bt_txrx = SER_TX;
	chprintf(chp, "Now monitoring BT TX"NL );
      } else {
	chprintf(chp, "Was monitoring BT TX"NL );
	palClearPad(IOPORT2, 16);
	sermon.bt_txrx = SER_RX;
	chprintf(chp, "Now monitoring BT RX"NL );
      }
    } else if( sermon.cl_uart == BB_SD ) {
      if( sermon.bb_txrx == SER_RX ) {
	chprintf(chp, "Was monitoring BB RX"NL );
	palSetPad(IOPORT4, 2);
	sermon.bb_txrx = SER_TX;
	chprintf(chp, "Now monitoring BB TX"NL );
      } else {
	chprintf(chp, "Was monitoring BB TX"NL );
	palClearPad(IOPORT4, 2);
	sermon.bb_txrx = SER_RX;
	chprintf(chp, "Now monitoring BB RX"NL );
      }
    }
  }

  if (!strcasecmp(argv[0], "wifi")) {
    sermon.cl_uart = WIFI_SD;
    if( sermon.wifi_txrx == SER_RX )
      palClearPad(IOPORT5, 0);
    else
      palSetPad(IOPORT5, 0);
  }
  
  if (!strcasecmp(argv[0], "bt")) {
    sermon.cl_uart = BT_SD;
    if( sermon.bt_txrx == SER_RX )
      palClearPad(IOPORT2, 16);
    else
      palSetPad(IOPORT2, 16);
  }

  if (!strcasecmp(argv[0], "bb")) {
    sermon.cl_uart = BB_SD;
    if( sermon.bb_txrx == SER_RX )
      palClearPad(IOPORT4, 2);
    else
      palSetPad(IOPORT4, 2);
  }
  
  return;
}

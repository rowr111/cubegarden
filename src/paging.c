#include "orchard-app.h"
#include "radio.h"

#include "led.h"
#include "paging.h"
#include "genes.h"
#include "storage.h"
#include <string.h>

static char message[MSG_MAXLEN];
static uint32_t rxseq = 0;

static void redraw_ui(void) {
}

void radioPagePopup(void) {
  
  chThdSleepMilliseconds(100);  // wait 100ms before doing a redraw to flush event queues
  redraw_ui();
  chThdSleepMilliseconds(PAGE_DISPLAY_MS);
}

static void radio_message_received(uint8_t prot, uint8_t src, uint8_t dst,
                                   uint8_t length, const void *data) {

  (void)length;
  (void)prot;
  chprintf(stream, "Received %s message from %02x: %s\r\n",
      (dst == RADIO_BROADCAST_ADDRESS) ? "broadcast" : "direct", src, data);

  strncpy(message, data, MSG_MAXLEN);
  message[MSG_MAXLEN - 1] = '\0'; // force a null termination (by truncation) if there isn't one
  rxseq++;

  chEvtBroadcast(&radio_page);
}


void pagingStart(void) {
  radioSetHandler(radioDriver, radio_prot_paging, radio_message_received);
}

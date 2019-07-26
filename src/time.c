#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#include "orchard.h"
#include "radio.h"


/**
 * Offset from network time in milliseconds.
 */
int32_t offsetMs = 0;

uint32_t lastmasterping = 0;

/**
 * Returns current network time in milliseconds from boot of time master.
 * If no time message has been received, simply returns system time unmodified.
 */
int32_t getNetworkTimeMs(void) {
  int32_t systemTimeMs = (int32_t) ST2MS(chVTGetSystemTime());
  int32_t networkTimeMs = (int32_t) systemTimeMs - offsetMs;

  return networkTimeMs;
}

/**
 * Sends the current time to all nodes.
 */
void broadcastTime(void) {
  uint32_t systemTimeMs = ST2MS(chVTGetSystemTime());

  radioAcquire(radioDriver);
  radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_time, sizeof(systemTimeMs), &systemTimeMs);
  radioRelease(radioDriver);
}

/**
 * Handler for network time messages.
 * The payload is an uint32_t containing the current network time.
 */
void handleRadioTime(uint8_t prot, uint8_t src, uint8_t dst, uint8_t length, const void *data) {
  (void) prot;
  (void) src;
  (void) dst;
  (void) length;
    
  uint32_t systemTimeMs = ST2MS(chVTGetSystemTime());
  lastmasterping = systemTimeMs; //save last master ping time
  uint32_t networkTimeMs = *((uint32_t *) data);

  offsetMs = systemTimeMs - networkTimeMs;
  chprintf(stream, "offset %d\n\r", offsetMs);
}

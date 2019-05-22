#include "ch.h"
#include "hal.h"

#include "orchard.h"
#include "radio.h"


/**
 * Offset from network time in milliseconds.
 */
int32_t offsetMs = 0;

/**
 * Returns current network time in milliseconds from boot of time master.
 * If no time message has been received, simply returns system time unmodified.
 */
uint32_t getNetworkTimeMs(void) {
  uint32_t systemTimeMs = ST2MS(chVTGetSystemTime());
  uint32_t networkTimeMs = systemTimeMs - offsetMs;

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
  uint32_t systemTimeMs = ST2MS(chVTGetSystemTime());
  uint32_t networkTimeMs = *((uint32_t *) data);

  offsetMs = systemTimeMs - networkTimeMs;
}

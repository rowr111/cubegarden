#include "ch.h"
#include "hal.h"
#include "shell.h"

#include "chprintf.h"

#include "led.h"
#include "orchard.h"
#include "radio.h"
#include "address.h"
#include "userconfig.h"


static void handleAddressRequest(NodeId id);

uint8_t getRadioAddressCounter(void) {
  return getConfig()->cfg_addressCounter;
}

void setRadioAddressCounter(uint8_t i) {
  configSetAddressCounter(i);
}

void handleRadioAddress(uint8_t prot, uint8_t src, uint8_t dst, uint8_t length, const void *data) {
  (void) prot;
  (void) src;
  (void) dst;
  (void) length;

  AddressPacket *pkt = (AddressPacket *)data;

  if (pkt->type == address_request) {
    handleAddressRequest(pkt->id);
  }

  // See src for address_response handler
}

static void handleAddressRequest(NodeId id) {
  uint8_t addressCounter = getRadioAddressCounter();
  
  if (addressCounter > MAX_ASSIGNABLE_ADDRESS) {
    chprintf(stream, "Out of addresses to assign"SHELL_NEWLINE_STR);
    return;
  }

  chprintf(stream, "Assigning address %d to %x-%x-%x-%x"SHELL_NEWLINE_STR, addressCounter, id.high, id.mediumHigh, id.mediumLow, id.low);

  chThdSleepMilliseconds(10);

  AddressPacket pkt;
  pkt.type = address_response;
  pkt.id = id;
  pkt.address = addressCounter;

  radioAcquire(radioDriver);
  radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_address, sizeof(pkt), &pkt);
  radioRelease(radioDriver);

  setRadioAddressCounter(addressCounter + 1); 
}


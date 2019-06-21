#include "ch.h"
#include "hal.h"
#include "shell.h"

#include "chprintf.h"

#include "led.h"
#include "orchard.h"
#include "radio.h"
#include "address.h"
#include "userconfig.h"


static void handleAddressResponse(AddressPacket *pkt);

void requestRadioAddress(void) {
  chprintf(stream, "Requesting address"SHELL_NEWLINE_STR);

  NodeId id;
  id.high = SIM->UIDH;
  id.mediumHigh = SIM->UIDMH;
  id.mediumLow = SIM->UIDML;
  id.low = SIM->UIDL;

  AddressPacket pkt;
  pkt.type = address_request;
  pkt.id = id;

  radioAcquire(radioDriver);
  radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_address, sizeof(pkt), &pkt);
  radioRelease(radioDriver);
}

void handleRadioAddress(uint8_t prot, uint8_t src, uint8_t dst, uint8_t length, const void *data) {
  (void) prot;
  (void) src;
  (void) dst;
  (void) length;

  AddressPacket *pkt = (AddressPacket *)data;

  if (pkt->type == address_response) {
    handleAddressResponse(pkt);
  }

  // See src-heart for address_request handler
}

static void handleAddressResponse(AddressPacket *pkt) {
  NodeId id = pkt->id;

  // Make sure packet is for this node
  if (id.high == SIM->UIDH && id.mediumHigh == SIM->UIDMH && id.mediumLow == SIM->UIDML && id.low == SIM->UIDL) {
    chprintf(stream, "Received address %d"SHELL_NEWLINE_STR, pkt->address);

    radioSetAddress(radioDriver, pkt->address);
    
    effectsSetPattern(effectsNameLookup("address"));
  }
}

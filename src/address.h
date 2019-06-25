#ifndef __ORCHARD_ADDRESS__
#define __ORCHARD_ADDRESS__

#include "ch.h"

typedef enum {
  address_request      = 1,
  address_response     = 2,
} address_packet_type;

typedef struct _NodeId {
  uint32_t high;
  uint32_t mediumHigh;
  uint32_t mediumLow;
  uint32_t low;
} NodeId;

typedef struct _AddressPacket {
  address_packet_type type;       /* Type of address packet */
  NodeId id; /* Id to correlate request and response before node has an address */
  uint8_t address; /* Address to set */
} AddressPacket;

void requestRadioAddress(void);
void handleRadioAddress(uint8_t prot, uint8_t src, uint8_t dst, uint8_t length, const void *data);

#endif // __ORCHARD_ADDRESS__
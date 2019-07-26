#ifndef __ORCHARD_TIME__
#define __ORCHARD_TIME__

#include "ch.h"

extern uint32_t lastmasterping;
int32_t getNetworkTimeMs(void);
void broadcastTime(void);
void handleRadioTime(uint8_t prot, uint8_t src, uint8_t dst, uint8_t length, const void *data);

#endif // __ORCHARD_TIME__

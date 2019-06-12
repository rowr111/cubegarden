#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "orchard.h"

extern uint32_t bump_amount;
extern uint8_t bumped;
extern uint8_t pressure_changed;
extern uint8_t singletapped;
extern uint8_t doubletapped;
extern unsigned int pressurechangedtime;
extern unsigned int singletaptime;
extern unsigned int doubletaptime;

void bump(uint32_t amount);
void pressureChanged(void);
void singletap(void);
void doubletap(void);
void checkdoubletapTrigger(void);
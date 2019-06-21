#ifndef __CMD_FORWARD__
#define __CMD_FORWARD__

#include "ch.h"
#include "hal.h"
#include "shell.h"

void handleRadioForward(uint8_t prot, uint8_t src, uint8_t dst, uint8_t length, const void *data);
void cmd_forward(BaseSequentialStream *chp, int argc, char *argv[]);

#endif // __CMD_FORWARD__
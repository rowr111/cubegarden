#ifndef __CMD_FORWARD__
#define __CMD_FORWARD__

#include "ch.h"
#include "hal.h"
#include "shell.h"


void handleRadioForward(uint8_t prot, uint8_t src, uint8_t dst, uint8_t length, const void *data);
static char *parse_arguments(char *str, char **saveptr);
static bool cmdexec(const ShellCommand *scp, BaseSequentialStream *chp,
                      char *name, int argc, char *argv[]);

#endif // __CMD_FORWARD__
/*
    Copyright (C) 2016 Jonathan Struebel

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    common/shellcfg.c
 * @brief   CLI shell config.
 *
 * @addtogroup SHELL
 * @{
 */
#include <stdlib.h>

#include "hal.h"
#include "shell.h"
#include "chprintf.h"

char ** endptr;

/*
 * Shell history buffer
 */
char history_buffer[SHELL_MAX_HIST_BUFF];

/*
 * Shell completion buffer
 */
char *completion_buffer[SHELL_MAX_COMPLETIONS];

/*
 * Shell commands
 */
void testCommand(BaseSequentialStream *chp, int argc, char *argv[]);
void i2cCommand(BaseSequentialStream *chp, int argc, char *argv[]);
void chgCommand(BaseSequentialStream *chp, int argc, char *argv[]);
void ggCommand(BaseSequentialStream *chp, int argc, char *argv[]);
void fxCommand(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_radio(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_msg(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_gename(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_genetweak(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_geneseq(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_friendlocal(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_friendlist(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_friendadd(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_friendping(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_friendsim(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_printaudit(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_auditcheck(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_testall(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_name(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_bright(BaseSequentialStream *chp, int argc, char *argv[]);
//void cmd_sound(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_accel(BaseSequentialStream *chp, int argc, char *argv[]);
#ifdef HAS_STC3115
void gg2Command(BaseSequentialStream *chp, int argc, char *argv[]);
#endif

static const ShellCommand commands[] = {
  //  {"sound", cmd_sound},
  {"accel", cmd_accel},
  {"bright", cmd_bright},
  {"test", testCommand},
  {"i2c", i2cCommand},
  {"chg", chgCommand},
  {"gg", ggCommand},
  {"fx", fxCommand},
  {"radio", cmd_radio},
  {"msg", cmd_msg},
  {"gename", cmd_gename},
  {"geneseq", cmd_geneseq},
  {"genetweak", cmd_genetweak},
  {"friendlocal", cmd_friendlocal},
  {"friendlist", cmd_friendlist},
  {"friendping", cmd_friendping},
  {"friendsim", cmd_friendsim},
  {"auditlog", cmd_printaudit},
  {"auditcheck", cmd_auditcheck},
  {"testall", cmd_testall},
  {"name", cmd_name},
#ifdef HAS_STC3115
  {"gg2", gg2Command},
#endif
  {NULL, NULL}
};

/*
 * Shell configuration
 */
const ShellConfig shell_cfg = {
  (BaseSequentialStream *)&SD4,
  commands,
  history_buffer,
  sizeof(history_buffer),
  completion_buffer
};

/** @} */

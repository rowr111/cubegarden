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
 * @file    common/shellcfg.h
 * @brief   CLI shell config header.
 *
 * @addtogroup SHELL
 * @{
 */

#ifndef SHELLCFG_H
#define SHELLCFG_H

#include "shell.h"

extern const ShellConfig shell_cfg;

#define orchard_shell_start() \
({ \
  static char start[0] __attribute__((unused,  \
    aligned(4), section(".chibi_list_zshell_1")));        \
  (const ShellCommand *)&start;            \
})

#define orchard_shell(_name, _func) \
  const ShellCommand _orchard_shell_list_##_func \
  __attribute__((unused, aligned(4), section(".chibi_list_zshell_2_" _name))) = \
     { _name, _func }

#define orchard_shell_end() \
  const ShellCommand _orchard_shell_list_##_func \
  __attribute__((unused, aligned(4), section(".chibi_list_zshell_3_end"))) = \
     { NULL, NULL }

#endif  /* SHELLCFG_H */

/** @} */

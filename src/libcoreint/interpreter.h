/* Copyright 2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INTERPRETER_H
#define	INTERPRETER_H

#ifdef JERRY_NDEBUG
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "opcodes.h"
#include "ecma-globals.h"

OPCODE __program[128];

opfunc __opfuncs[LAST_OP];

struct __int_data
{
  int pos;
  ecma_Object_t *pThisBinding; /**< this binding for current context */
  ecma_Object_t *pLexEnv; /**< current lexical environment */
  int *root_op_addr;
};

void gen_bytecode (void);
void run_int (void);
void run_int_from_pos (struct __int_data *);

#endif	/* INTERPRETER_H */


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

#ifndef OPCODES_H
#define	OPCODES_H

#ifdef __HOST
#include <stdio.h>
#endif

#include "globals.h"

#define OPCODE struct __opcode
#define OP_DEF(name) struct __op_##name
#define OP(name) struct __op_##name name

#define OP_TYPE_IDX uint8_t

OPCODE;

typedef void (*opfunc)(OPCODE);

#include "opcode-structures.h"

union __opdata
{
  OP (loop_inf);
  OP (call_1);
  OP (call_2);
  OP (call_n);

  OP (jmp);
  OP (nop);
} data;

OPCODE{
  opfunc opfunc_ptr;
  union __opdata data;
}
__packed;

#ifdef __HOST
void save_op_data (FILE*, OPCODE);
#elif __MCU
void save_op_data (OPCODE);
#endif


OPCODE get_op_loop_inf (int);
OPCODE get_op_call_1 (int, int);
OPCODE get_op_jmp (int arg1);

#endif	/* OPCODES_H */


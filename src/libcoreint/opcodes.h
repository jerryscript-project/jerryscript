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

#include "stdio.h"

#define OPCODE struct __opcode
#define OP_DEF(name) struct __op_##name
#define OP(name) struct __op_##name name

OPCODE;

typedef void (*opfunc)(OPCODE);

OP_DEF (loop_inf)
{
  int opcode_idx;
};

/** Call with 1 argument */
OP_DEF (call_1)
{
  int name_literal_idx;
  int arg_literal_idx;
};

OP_DEF (jmp)
{
  int opcode_idx;
};

OPCODE{
  opfunc opfunc_ptr;

  /** OPCODES */
  union __opdata
  {
    OP (loop_inf);
    OP (call_1);
    OP (jmp);
  } data;
}
__packed;

void save_op_jmp (FILE *, OPCODE, int);
void save_op_call_1 (FILE *, OPCODE, int, int);
void save_op_loop_inf (FILE *, OPCODE, int);

#endif	/* OPCODES_H */


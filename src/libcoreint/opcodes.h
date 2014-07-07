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

#define OP_STRUCT_FIELD(name) struct __op_##name name;
#define OP_ENUM_FIELD(name) name ,
#define OP_FUNC_DECL(name) void opfunc_##name  (OPCODE);

OPCODE;
typedef void (*opfunc)(OPCODE);

#define OP_LIST(op) \
  op(loop_inf) \
  op(call_1) \
  op(jmp) \
  //op(call_2) \
  op(call_n) \
  op(nop)

#include "opcode-structures.h"

enum __opcode_idx
{
  OP_LIST (OP_ENUM_FIELD)
  LAST_OP
};

union __opdata
{
  OP_LIST (OP_STRUCT_FIELD)
};

OP_LIST (OP_FUNC_DECL)

OPCODE{
  T_IDX op_idx;
  union __opdata data;
}
__packed;

void save_op_data (OPCODE);

#endif	/* OPCODES_H */


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

#include "opcodes.h"
#include "interpreter.h"

void
save_op_data (OPCODE opdata)
{
  __program[__int_data.pos++] = opdata;
}

void
opfunc_loop_inf (OPCODE opdata)
{
#ifdef __HOST
  printf ("%d::loop_inf:idx:%d\n",
          __int_data.pos,
          opdata.data.loop_inf.opcode_idx);
#endif

  __int_data.pos = opdata.data.loop_inf.opcode_idx;
}

void
opfunc_call_1 (OPCODE opdata)
{
#ifdef __HOST
  printf ("%d::op_call_1:idx:%d:%d\n",
          __int_data.pos,
          opdata.data.call_1.name_literal_idx,
          opdata.data.call_1.arg1_literal_idx);
#endif

  __int_data.pos++;
}

void
opfunc_jmp (OPCODE opdata)
{
#ifdef __HOST
  printf ("%d::op_jmp:idx:%d\n",
          __int_data.pos,
          opdata.data.jmp.opcode_idx);
#endif

  __int_data.pos = opdata.data.jmp.opcode_idx;
}

OPCODE
getop_jmp (int arg1)
{
  OPCODE opdata;

  opdata.opfunc_ptr = opfunc_jmp;
  opdata.data.jmp.opcode_idx = arg1;

  return opdata;
}

OPCODE
getop_call_1 (int arg1, int arg2)
{
  OPCODE opdata;

  opdata.opfunc_ptr = opfunc_call_1;
  opdata.data.call_1.name_literal_idx = arg1;
  opdata.data.call_1.arg1_literal_idx = arg2;

  return opdata;
}

OPCODE
getop_loop_inf (int arg1)
{
  OPCODE opdata;

  opdata.opfunc_ptr = opfunc_loop_inf;
  opdata.data.loop_inf.opcode_idx = arg1;

  return opdata;
}

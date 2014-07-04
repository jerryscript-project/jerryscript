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

#include <stdio.h>
#include <stdlib.h>

void
opfunc_loop_inf (OPCODE opdata)
{
  printf ("loop_inf:idx:%d\n",
          opdata.data.loop_inf.opcode_idx);

  __int_data.pos = opdata.data.loop_inf.opcode_idx;
}

void
opfunc_op_call_1 (OPCODE opdata)
{
  printf ("op_call_1:idx:%d:%d\n",
          opdata.data.call_1.name_literal_idx,
          opdata.data.call_1.arg1_literal_idx);
}

void
opfunc_op_jmp (OPCODE opdata)
{
  printf ("op_jmp:idx:%d\n",
          opdata.data.jmp.opcode_idx);

  __int_data.pos = opdata.data.jmp.opcode_idx;
}

OPCODE
get_op_jmp (int arg1)
{
  OPCODE opdata;

  opdata.opfunc_ptr = opfunc_op_jmp;
  opdata.data.jmp.opcode_idx = arg1;

  return opdata;
}

OPCODE
get_op_call_1 (int arg1, int arg2)
{
  OPCODE opdata;

  opdata.opfunc_ptr = opfunc_op_call_1;
  opdata.data.call_1.name_literal_idx = arg1;
  opdata.data.call_1.arg1_literal_idx = arg2;

  return opdata;
}

OPCODE
get_op_loop_inf (int arg1)
{
  OPCODE opdata;

  opdata.opfunc_ptr = opfunc_op_call_1;
  opdata.data.loop_inf.opcode_idx = arg1;

  return opdata;
}

void
save_op_data (FILE *file, OPCODE opdata)
{
  if (file == NULL)
  {
    return;
  }

  fwrite (&opdata, sizeof (OPCODE), 1, file);
}
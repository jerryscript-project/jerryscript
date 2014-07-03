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

#include <stdio.h>
#include <stdlib.h>

void
opfunc_loop_inf (union __opcodes opdata)
{
  printf ("loop_inf:idx:%d\n",
          opdata.op_loop_inf.opcode_idx);
}

void
opfunc_op_call_1 (union __opcodes opdata)
{
  printf ("op_call_1:idx:%d:%d\n",
          opdata.op_call_1.name_literal_idx,
          opdata.op_call_1.arg_literal_idx);
}



void
opfunc_op_jmp (union __opcodes opdata)
{
  printf ("op_jmp:idx:%d\n",
          opdata.op_jmp.opcode_idx);
}

void proxy_loop_inf (union __opcodes opdata)
{
  opdata.op_loop_inf.opfunc_ptr (opdata);
}


void proxy_op_call_1 (union __opcodes opdata)
{
  opdata.op_call_1.opfunc_ptr (opdata);
}

void proxy_op_jmp (union __opcodes opdata)
{
  opdata.op_jmp.opfunc_ptr (opdata);
}



void
save_op_jmp (FILE *file, union __opcodes opdata, int arg1)
{
  if (file == NULL)
  {
    return;
  }

  opdata.opfunc_ptr = proxy_op_jmp;
  
  opdata.op_jmp.opfunc_ptr = opfunc_op_jmp;
  opdata.op_jmp.opcode_idx = arg1;

  fwrite (&opdata, sizeof (union __opcodes), 1, file);
}

void
save_op_call_1 (FILE *file, union __opcodes opdata, int arg1, int arg2)
{
  if (file == NULL)
  {
    return;
  }

  opdata.opfunc_ptr = proxy_op_call_1;
  
  opdata.op_call_1.opfunc_ptr = opfunc_op_call_1;
  opdata.op_call_1.name_literal_idx = arg1;
  opdata.op_call_1.arg_literal_idx = arg2;

  fwrite (&opdata, sizeof (union __opcodes), 1, file);
}

void
save_op_loop_inf (FILE *file, union __opcodes opdata, int arg1)
{
  if (file == NULL)
  {
    return;
  }
    
  opdata.opfunc_ptr = proxy_loop_inf;

  opdata.op_loop_inf.opfunc_ptr = opfunc_loop_inf;
  opdata.op_loop_inf.opcode_idx = arg1;

  fwrite (&opdata, sizeof (union __opcodes), 1, file);
}
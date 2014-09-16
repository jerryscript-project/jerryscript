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

#include "optimizer-passes.h"
#include "opcodes.h"
#include "deserializer.h"
#include "serializer.h"
#include "jerry-libc.h"

#define NAME_TO_ID(op) (__op__idx_##op)

/* Reorder bytecode like

   call_n ...
   assignment ... +
   var_?_end

   to

   assignment ... +
   call_n ...
   var_?_end
 */
static void
optimize_calls (opcode_t *opcodes)
{
  opcode_t *current_opcode;

  for (current_opcode = opcodes;
       current_opcode->op_idx != NAME_TO_ID (exitval);
       current_opcode++)
  {
    if (current_opcode->op_idx == NAME_TO_ID (call_n)
        && (current_opcode + 1)->op_idx == NAME_TO_ID (assignment))
    {
      opcode_t temp = *current_opcode;
      *current_opcode = *(current_opcode + 1);
      *(current_opcode + 1) = temp;
    }
  }
}

void
optimizer_run_passes (opcode_t *opcodes)
{
  optimize_calls (opcodes);
}

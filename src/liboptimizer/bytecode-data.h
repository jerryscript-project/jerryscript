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

#ifndef BYTECODE_DATA_H
#define BYTECODE_DATA_H

#include "opcodes.h"
#include "stack.h"
#include "jerry-libc.h"

#ifndef OPCODE_T_STACK_DEFINED
DEFINE_STACK_TYPE (opcode_counter_t, opcode_t)
#define OPCODE_T_STACK_DEFINED
#endif

/* bytecode_data contains identifiers, string and num literals.
   Memory map if the following.

   bytecode_data {
      U8 strs_count;
      U8 string_offsets[str_count];
      U8* strings[str_count];

      U8 nums_count;
      U32 nums[nums_count];
   } */
extern uint8_t *bytecode_data;

enum
{
  bytecode_opcodes_global_size
};
STACK (opcode_t, bytecode_opcodes)

#endif // BYTECODE_DATA_H

/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#ifndef COMMON_H
#define COMMON_H

#define NAME_TO_ID(op) (__op__idx_##op)

#define __OPCODE_SIZE(name, arg1, arg2, arg3) \
  (uint8_t) (sizeof (__op_##name) + 1),

static uint8_t opcode_sizes[] =
{
  OP_LIST (OPCODE_SIZE)
  0
};

static bool opcodes_equal (const opcode_t *, opcode_t *, uint16_t) __attr_unused___;

static bool
opcodes_equal (const opcode_t *opcodes1, opcode_t *opcodes2, uint16_t size)
{
  uint16_t i;
  for (i = 0; i < size; i++)
  {
    uint8_t opcode_num1 = opcodes1[i].op_idx, opcode_num2 = opcodes2[i].op_idx;
    uint8_t j;

    if (opcode_num1 != opcode_num2)
    {
      return false;
    }

    if (opcode_num1 == NAME_TO_ID (nop) || opcode_num1 == NAME_TO_ID (ret))
    {
      return true;
    }

    for (j = 1; j < opcode_sizes[opcode_num1]; j++)
    {
      if (((uint8_t*)&opcodes1[i])[j] != ((uint8_t*)&opcodes2[i])[j])
      {
        return false;
      }
    }
  }

  return true;
}

#endif // COMMON_H

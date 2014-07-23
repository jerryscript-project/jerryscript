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

#include "serializer.h"
#include "jerry-libc.h"
#include "opcodes.h"

FILE *dump;

#define OPCODE_STR(op) \
  #op,

static char* massive[] = {
  OP_LIST (OPCODE_STR)
  ""
};

void
serializer_init (void)
{
}

uint8_t
serializer_dump_strings (const char *strings[], uint8_t size)
{
  uint8_t i;
  uint8_t offset = size;

  __printf ("STRINGS %d:\n", size);
  for (i = 0; i < size; i++)
    {
      __printf ("%3d %3d %20s\n", i, offset, strings[i]);
      offset = (uint8_t ) (offset + __strlen (strings[i]));
    }

  return offset;
}

void 
serializer_dump_nums (const int nums[], uint8_t size, uint8_t offset, uint8_t strings_num)
{
  uint8_t i;

  offset = (uint8_t) (offset + size);
  __printf ("NUMS %d:\n", size);
  for (i = 0; i < size; i++)
    {
      __printf ("%3d %3d %7d\n", i + strings_num, offset, nums[i]);
      offset = (uint8_t) (offset + 4);
    }
}

static int opcode_counter = 0;

void
serializer_dump_opcode (const void *opcode)
{
  uint8_t i;

  __printf ("%03d: %20s ", opcode_counter++, massive[(int)((char*)opcode)[0]]);
  for (i = 1; i < 4; i++)
    __printf ("%4d ", ((char*)opcode)[i]);

  __printf ("\n");
}

void
serializer_rewrite_opcode (const int8_t offset, const void *opcode)
{
  uint8_t i;

  __printf ("%03d: %20s ", opcode_counter + offset, massive[(int)((char*)opcode)[0]]);
  for (i = 1; i < 4; i++)
    __printf ("%4d ", ((char*)opcode)[i]);

  __printf ("// REWRITE\n");
}

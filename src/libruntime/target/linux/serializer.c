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
#include "bytecode-linux.h"
#include "deserializer.h"

_FILE *dump;

#define OPCODE_STR(op) \
  #op,

#define OPCODE_SIZE(op) \
  sizeof (struct __op_##op) + 1,

static char* opcode_names[] = {
  OP_LIST (OPCODE_STR)
  ""
};

static uint8_t opcode_sizes[] = {
  OP_LIST (OPCODE_SIZE)
  0
};

void
serializer_init (void)
{
}

uint8_t
serializer_dump_strings (const char *strings[], uint8_t size)
{
  uint8_t i;
  uint8_t offset = (uint8_t) (size + 1), res;

  __printf ("STRINGS %d:\n", size);
  for (i = 0; i < size; i++)
    {
      __printf ("%3d %3d %20s\n", i, offset, strings[i]);
      offset = (uint8_t) (offset + __strlen (strings[i]) + 1);
    }

  bytecode_data = mem_heap_alloc_block (offset, MEM_HEAP_ALLOC_SHORT_TERM);
  res = offset;

  bytecode_data[0] = size;
  offset = (uint8_t) (size + 1);
  for (i = 0; i < size; i++)
    {
      bytecode_data[i + 1] = offset;
      offset = (uint8_t) (offset + __strlen (strings[i]) + 1);
    }

  for (i = 0; i < size; i++)
    __strncpy ((void *) (bytecode_data + bytecode_data[i + 1]), strings[i], __strlen (strings[i]) + 1);

#ifndef JERRY_NDEBUG
  for (i = 0; i < size; i++)
    {
      JERRY_ASSERT (!__strcmp (strings[i], (const char *) deserialize_string_by_id (i)));
    }
#endif

  return res;
}

void 
serializer_dump_nums (const int32_t nums[], uint8_t size, uint8_t offset, uint8_t strings_num)
{
  uint8_t i, *data, num_offset;

  num_offset = (uint8_t) (offset + size + 1);
  __printf ("NUMS %d:\n", size);
  for (i = 0; i < size; i++)
    {
      __printf ("%3d %3d %7d\n", i + strings_num, num_offset, nums[i]);
      num_offset = (uint8_t) (num_offset + 4);
    }

  __printf ("\n");

  data = mem_heap_alloc_block ((size_t) (offset + size * 4 + 1), MEM_HEAP_ALLOC_LONG_TERM);
  __memcpy (data, bytecode_data, offset);
  mem_heap_free_block (bytecode_data);
  bytecode_data = data;
  data += offset;
  data[0] = size;
  data++;
  for (i = 0; i < size; i++)
    {
      __memcpy (data, nums + i, 4);
      data += 4;
    }

#ifndef JERRY_NDEBUG
  for (i = 0; i < size; i++)
    {
      JERRY_ASSERT (nums[i] == deserialize_num_by_id ((uint8_t) (i + strings_num)));
    }

  JERRY_ASSERT (deserialize_min_temp () == (uint8_t) (size + strings_num));
#endif
}

static int opcode_counter = 0;

void
serializer_dump_opcode (const void *opcode)
{
  uint8_t i;
  int opcode_num = (int)((char*)opcode)[0];

  bytecode_opcodes[opcode_counter] = *((OPCODE*)opcode);
  __printf ("%03d: %20s ", opcode_counter++, opcode_names[opcode_num]);
  for (i = 1; i < opcode_sizes[opcode_num]; i++)
    __printf ("%4d ", ((char*)opcode)[i]);

  __printf ("\n");
}

void
serializer_rewrite_opcode (const uint8_t loc, const void *opcode)
{
  uint8_t i;
  int opcode_num = (int)((char*)opcode)[0];

  bytecode_opcodes[loc] = *((OPCODE*)opcode);
  __printf ("%03d: %20s ", loc, opcode_names[opcode_num]);
  for (i = 1; i < opcode_sizes[opcode_num]; i++)
    __printf ("%4d ", ((char*)opcode)[i]);

  __printf ("// REWRITE\n");
}

void
serializer_print_opcodes (void)
{
  int loc = -1, i;
  OPCODE* opcode;
  int opcode_num;

  __printf ("AFTER OPTIMIZER:\n");

  do
    {
      loc++;

      opcode = bytecode_opcodes + loc;
      opcode_num = (int)((char*)opcode)[0];

      __printf ("%03d: %20s ", loc, opcode_names[opcode_num]);
      for (i = 1; i < opcode_sizes[opcode_num]; i++)
        __printf ("%4d ", ((char*)opcode)[i]);
      __printf ("\n");
    }
  while (opcode->op_idx != __op__idx_exitval);
}

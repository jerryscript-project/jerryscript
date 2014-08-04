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
#include "parser.h"
#include "globals.h"
#include "serializer.h"
#include "jerry-libc.h"
#include "bytecode-stm.h"
#include "deserializer.h"

TODO (Dump to flash)

void
serializer_init (bool show_opcodes __unused)
{
}

uint16_t
serializer_dump_strings (const char *strings[], uint8_t size)
{
  uint8_t i;
  uint16_t offset = (uint8_t) (size * 2 + 1), res;

  for (i = 0; i < size; i++)
    {
      offset = (uint16_t) (offset + __strlen (strings[i]) + 1);
    }

  bytecode_data = mem_heap_alloc_block (offset, MEM_HEAP_ALLOC_SHORT_TERM);
  res = offset;

  bytecode_data[0] = size;
  offset = (uint16_t) (size * 2 + 1);
  for (i = 0; i < size; i++)
    {
      *((uint16_t *) (bytecode_data + i * 2 + 1)) = offset;
      offset = (uint16_t) (offset + __strlen (strings[i]) + 1);
    }

  for (i = 0; i < size; i++)
    {
      offset = *((uint16_t *) (bytecode_data + i * 2 + 1));
      __strncpy ((char *) (bytecode_data + offset), strings[i], __strlen (strings[i]) + 1);
    }

#ifndef JERRY_NDEBUG
  for (i = 0; i < size; i++)
    {
      JERRY_ASSERT (!__strcmp (strings[i], (const char *) deserialize_string_by_id (i)));
    }
#endif

  return res;
}

void 
serializer_dump_nums (const int32_t nums[], uint8_t size, uint16_t offset, uint8_t strings_num __unused)
{
  uint8_t i, *data;

  data = mem_heap_alloc_block ((size_t) (offset + size * 4 + 1), MEM_HEAP_ALLOC_LONG_TERM);
  if (!data)
    parser_fatal (ERR_MEMORY);
  
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
serializer_dump_opcode (OPCODE opcode)
{
  bytecode_opcodes[opcode_counter++] = opcode;
}

void
serializer_rewrite_opcode (const opcode_counter_t loc, OPCODE opcode)
{
  bytecode_opcodes[loc] = opcode;
}

void
serializer_print_opcodes (void)
{
  JERRY_UNREACHABLE ();
}

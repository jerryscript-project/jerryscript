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

#include "deserializer.h"
#include "bytecode-data.h"

const ecma_char_t *
deserialize_string_by_id (uint8_t id)
{
  JERRY_ASSERT (id < bytecode_data.strs_count);
  JERRY_ASSERT (bytecode_data.strings[id].str[bytecode_data.strings[id].length] == '\0');

  return ((const ecma_char_t *) bytecode_data.strings[id].str);
}

ecma_number_t
deserialize_num_by_id (uint8_t id)
{
  JERRY_ASSERT (id >= bytecode_data.strs_count);

  id = (uint8_t) (id - bytecode_data.strs_count);

  JERRY_ASSERT (id < bytecode_data.nums_count);

  return bytecode_data.nums[id];
}

const void *
deserialize_bytecode (void)
{
  JERRY_ASSERT (bytecode_data.opcodes != NULL);
  return bytecode_data.opcodes;
}

opcode_t
deserialize_opcode (opcode_counter_t oc)
{
  return scopes_tree_opcode (current_scope, oc);
}

uint8_t
deserialize_min_temp (void)
{
  return (uint8_t) (bytecode_data.strs_count + bytecode_data.nums_count);
}

void
deserializer_init (void)
{
}

void
deserializer_free (void)
{
  mem_heap_free_block ((uint8_t *) bytecode_data.strings);
  mem_heap_free_block ((uint8_t *) bytecode_data.nums);
  mem_heap_free_block ((uint8_t *) bytecode_data.opcodes);
}

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
#include "ecma-helpers.h"

const ecma_char_t *strings_buffer;

lit_id_table_key
create_lit_id_table_key (idx_t id, opcode_counter_t oc)
{
  return (lit_id_table_key)
  {
    .uid = id,
    .oc = oc / BLOCK_SIZE,
    .reserved = 0
  };
}

void
deserializer_set_strings_buffer (const ecma_char_t *s)
{
  strings_buffer = s;
}

literal
deserialize_literal_by_id (literal_index_t id)
{
  JERRY_ASSERT (id != INVALID_LITERAL);
  JERRY_ASSERT (id < bytecode_data.literals_count);
  return bytecode_data.literals[id];
}

literal_index_t
deserialize_lit_id_by_uid (uint8_t id, opcode_counter_t oc)
{
  // __printf ("uid: %d, oc: %d\n", id, oc);
  // if (id == 2 && oc == 64)
  // {
  //   __printf ("HIT!\n");
  // }
  if (bytecode_data.lit_id_hash == null_hash)
  {
    return INVALID_LITERAL;
  }
  lit_id_table_key key = create_lit_id_table_key (id, oc);
  void *res = hash_table_lookup (bytecode_data.lit_id_hash, &key);
  JERRY_ASSERT (res != NULL);
  return *(literal_index_t *) res;
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
  if (bytecode_data.opcodes == NULL)
  {
    return deserialize_op_meta (oc).op;
  }
  JERRY_ASSERT (oc < bytecode_data.opcodes_count);
  return bytecode_data.opcodes[oc];
}

op_meta
deserialize_op_meta (opcode_counter_t oc)
{
  JERRY_ASSERT (current_scope);
  return scopes_tree_op_meta (current_scope, oc);
}

uint8_t
deserialize_min_temp (void)
{
  return 128;
}

void
deserializer_init (void)
{
  bytecode_data.literals = NULL;
  strings_buffer = NULL;
  bytecode_data.opcodes = NULL;
}

void
deserializer_free (void)
{
  if (strings_buffer)
  {
    mem_heap_free_block ((uint8_t *) strings_buffer);
  }
  if (bytecode_data.lit_id_hash != null_hash)
  {
    hash_table_free (bytecode_data.lit_id_hash);
  }
  if (bytecode_data.literals != NULL)
  {
    mem_heap_free_block ((uint8_t *) bytecode_data.literals);
  }
  mem_heap_free_block ((uint8_t *) bytecode_data.opcodes);
}

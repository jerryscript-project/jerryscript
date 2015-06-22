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

#include "serializer.h"
#include "bytecode-data.h"
#include "pretty-printer.h"
#include "array-list.h"

static bytecode_data_t bytecode_data;
static scopes_tree current_scope;
static bool print_opcodes;

static void
serializer_print_opcodes (const opcode_t *opcodes_p,
                          size_t opcodes_count);

op_meta
serializer_get_op_meta (opcode_counter_t oc)
{
  JERRY_ASSERT (current_scope);
  return scopes_tree_op_meta (current_scope, oc);
}

/**
 * Get byte-code instruction from current scope, or specified byte-code array
 *
 * @return byte-code instruction
 */
opcode_t
serializer_get_opcode (const opcode_t *opcodes_p, /**< pointer to byte-code array (or NULL,
                                                   *   if instruction should be taken from
                                                   *   instruction list of current scope) */
                       opcode_counter_t oc) /**< opcode counter of the intruction */
{
  if (opcodes_p == NULL)
  {
    return serializer_get_op_meta (oc).op;
  }
  else
  {
    JERRY_ASSERT (oc < GET_BYTECODE_HEADER (opcodes_p)->instructions_number);
    return opcodes_p[oc];
  }
} /* serializer_get_opcode */

/**
 * Convert literal id (operand value of instruction) to compressed pointer to literal
 *
 * Bytecode is divided into blocks of fixed size and each block has independent encoding of variable names,
 * which are represented by 8 bit numbers - ids.
 * This function performs conversion from id to literal.
 *
 * @return compressed pointer to literal
 */
lit_cpointer_t
serializer_get_literal_cp_by_uid (uint8_t id, /**< literal idx */
                                  const opcode_t *opcodes_p, /**< pointer to bytecode */
                                  opcode_counter_t oc) /**< position in the bytecode */
{
  lit_id_hash_table *lit_id_hash = GET_HASH_TABLE_FOR_BYTECODE (opcodes_p == NULL ? bytecode_data.opcodes : opcodes_p);
  if (lit_id_hash == null_hash)
  {
    return INVALID_LITERAL;
  }
  return lit_id_hash_table_lookup (lit_id_hash, id, oc);
} /* serializer_get_literal_cp_by_uid */

void
serializer_set_strings_buffer (const ecma_char_t *s)
{
  bytecode_data.strings_buffer = s;
}

void
serializer_set_scope (scopes_tree new_scope)
{
  current_scope = new_scope;
}

const opcode_t *
serializer_merge_scopes_into_bytecode (void)
{
  bytecode_data.opcodes_count = scopes_tree_count_opcodes (current_scope);

  const size_t buckets_count = scopes_tree_count_literals_in_blocks (current_scope);
  const size_t blocks_count = (size_t) bytecode_data.opcodes_count / BLOCK_SIZE + 1;
  const opcode_counter_t opcodes_count = scopes_tree_count_opcodes (current_scope);

  const size_t opcodes_array_size = JERRY_ALIGNUP (sizeof (opcodes_header_t) + opcodes_count * sizeof (opcode_t),
                                                   MEM_ALIGNMENT);
  const size_t lit_id_hash_table_size = JERRY_ALIGNUP (lit_id_hash_table_get_size_for_table (buckets_count,
                                                                                             blocks_count),
                                                       MEM_ALIGNMENT);

  uint8_t *buffer_p = (uint8_t*) mem_heap_alloc_block (opcodes_array_size + lit_id_hash_table_size,
                                                       MEM_HEAP_ALLOC_LONG_TERM);

  lit_id_hash_table *lit_id_hash = lit_id_hash_table_init (buffer_p + opcodes_array_size,
                                                           lit_id_hash_table_size,
                                                           buckets_count, blocks_count);

  const opcode_t *opcodes_p = scopes_tree_raw_data (current_scope, buffer_p, opcodes_array_size, lit_id_hash);

  opcodes_header_t *header_p = (opcodes_header_t*) buffer_p;
  MEM_CP_SET_POINTER (header_p->next_opcodes_cp, bytecode_data.opcodes);
  header_p->instructions_number = opcodes_count;
  bytecode_data.opcodes = opcodes_p;

  if (print_opcodes)
  {
    lit_dump_literals ();
    serializer_print_opcodes (opcodes_p, bytecode_data.opcodes_count);
  }

  return opcodes_p;
}

void
serializer_dump_op_meta (op_meta op)
{
  JERRY_ASSERT (scopes_tree_opcodes_num (current_scope) < MAX_OPCODES);

  scopes_tree_add_op_meta (current_scope, op);

#ifdef JERRY_ENABLE_PRETTY_PRINTER
  if (print_opcodes)
  {
    pp_op_meta (NULL, (opcode_counter_t) (scopes_tree_opcodes_num (current_scope) - 1), op, false);
  }
#endif
}

opcode_counter_t
serializer_get_current_opcode_counter (void)
{
  return scopes_tree_opcodes_num (current_scope);
}

opcode_counter_t
serializer_count_opcodes_in_subscopes (void)
{
  return (opcode_counter_t) (scopes_tree_count_opcodes (current_scope) - scopes_tree_opcodes_num (current_scope));
}

void
serializer_set_writing_position (opcode_counter_t oc)
{
  scopes_tree_set_opcodes_num (current_scope, oc);
}

void
serializer_rewrite_op_meta (const opcode_counter_t loc, op_meta op)
{
  scopes_tree_set_op_meta (current_scope, loc, op);

#ifdef JERRY_ENABLE_PRETTY_PRINTER
  if (print_opcodes)
  {
    pp_op_meta (NULL, loc, op, true);
  }
#endif
}

static void
serializer_print_opcodes (const opcode_t *opcodes_p,
                          size_t opcodes_count)
{
#ifdef JERRY_ENABLE_PRETTY_PRINTER
  for (opcode_counter_t loc = 0; loc < opcodes_count; loc++)
  {
    op_meta opm;

    opm.op = opcodes_p[loc];
    for (int i = 0; i < 3; i++)
    {
      opm.lit_id[i] = NOT_A_LITERAL;
    }

    pp_op_meta (opcodes_p, loc, opm, false);
  }
#else
  (void) opcodes_p;
  (void) opcodes_count;
#endif
}

void
serializer_init ()
{
  current_scope = NULL;
  print_opcodes = false;

  bytecode_data.strings_buffer = NULL;
  bytecode_data.opcodes = NULL;

  lit_init ();
}

void serializer_set_show_opcodes (bool show_opcodes)
{
  print_opcodes = show_opcodes;
}

void
serializer_free (void)
{
  if (bytecode_data.strings_buffer)
  {
    mem_heap_free_block ((uint8_t *) bytecode_data.strings_buffer);
  }

  lit_finalize ();

  while (bytecode_data.opcodes != NULL)
  {
    opcodes_header_t *header_p = GET_BYTECODE_HEADER (bytecode_data.opcodes);
    bytecode_data.opcodes = MEM_CP_GET_POINTER (opcode_t, header_p->next_opcodes_cp);

    mem_heap_free_block (header_p);
  }
}

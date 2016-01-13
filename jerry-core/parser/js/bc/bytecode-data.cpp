/* Copyright 2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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

#include "bytecode-data.h"
#include "pretty-printer.h"
#include "opcodes-dumper.h"

/**
 * First node of the list of bytecodes
 */
static bytecode_data_header_t *first_bytecode_header_p = NULL;

/**
 * Bytecode header in snapshot
 */
typedef struct
{
  uint32_t size; /**< size of this bytecode data record */
  uint32_t instrs_size; /**< size of instructions array */
  uint32_t idx_to_lit_map_size; /**< size of idx-to-lit map */
  uint32_t func_scopes_count; /**< count of function scopes inside current scope */
  uint32_t var_decls_count; /**< count of variable declarations insdie current scope */

  uint8_t is_strict : 1; /**< code is strict mode code */
  uint8_t is_ref_arguments_identifier : 1; /**< code doesn't reference 'arguments' identifier */
  uint8_t is_ref_eval_identifier : 1; /**< code doesn't reference 'eval' identifier */
  uint8_t is_args_moved_to_regs : 1; /**< the function's arguments are moved to registers,
                                      *   so should be initialized in vm registers,
                                      *   and not in lexical environment */
  uint8_t is_no_lex_env : 1; /**< no lex. env. is necessary for the scope */
} jerry_snapshot_bytecode_header_t;

/**
 * Fill the fields of bytecode data header with specified values
 */
static void
bc_fill_bytecode_data_header (bytecode_data_header_t *bc_header_p, /**< byte-code scope data header to fill */
                              lit_id_hash_table *lit_id_hash_table_p, /**< (idx, block id) -> literal hash table */
                              vm_instr_t *bytecode_p, /**< byte-code instructions array */
                              mem_cpointer_t *declarations_p, /**< array of function / variable declarations */
                              uint16_t func_scopes_count, /**< number of function declarations / expressions
                                                           *   located immediately in the corresponding scope */
                              uint16_t var_decls_count, /**< number of variable declarations immediately in the scope */
                              bool is_strict, /**< is the scope's code strict mode code? */
                              bool is_ref_arguments_identifier, /**< does the scope's code
                                                                 *    reference 'arguments' identifier? */
                              bool is_ref_eval_identifier, /**< does the scope's code
                                                            *   reference 'eval' identifier? */
                              bool is_vars_and_args_to_regs_possible, /**< is it scope, for which variables / arguments
                                                                       *   can be moved to registers */
                              bool is_arguments_moved_to_regs, /**< is it function scope, for which arguments
                                                                *   are located on registers, not in variables? */
                              bool is_no_lex_env) /**< is lexical environment unused in the scope? */

{
  MEM_CP_SET_POINTER (bc_header_p->lit_id_hash_cp, lit_id_hash_table_p);
  bc_header_p->instrs_p = bytecode_p;
  bc_header_p->instrs_count = 0;
  MEM_CP_SET_POINTER (bc_header_p->declarations_cp, declarations_p);
  bc_header_p->func_scopes_count = func_scopes_count;
  bc_header_p->var_decls_count = var_decls_count;
  bc_header_p->next_header_cp = MEM_CP_NULL;

  bc_header_p->is_strict = is_strict;
  bc_header_p->is_ref_arguments_identifier = is_ref_arguments_identifier;
  bc_header_p->is_ref_eval_identifier = is_ref_eval_identifier;
  bc_header_p->is_vars_and_args_to_regs_possible = is_vars_and_args_to_regs_possible;
  bc_header_p->is_args_moved_to_regs = is_arguments_moved_to_regs;
  bc_header_p->is_no_lex_env = is_no_lex_env;
} /* bc_fill_bytecode_data_header */

/**
 * Free memory occupied by bytecode data
 */
static void
bc_free_bytecode_data (bytecode_data_header_t *bytecode_data_p) /**< byte-code scope data header */
{
  bytecode_data_header_t *next_to_handle_list_p = bytecode_data_p;

  while (next_to_handle_list_p != NULL)
  {
    bytecode_data_header_t *bc_header_list_iter_p = next_to_handle_list_p;
    next_to_handle_list_p = NULL;

    while (bc_header_list_iter_p != NULL)
    {
      bytecode_data_header_t *header_p = bc_header_list_iter_p;

      bc_header_list_iter_p = MEM_CP_GET_POINTER (bytecode_data_header_t, header_p->next_header_cp);

      mem_cpointer_t *declarations_p = MEM_CP_GET_POINTER (mem_cpointer_t, header_p->declarations_cp);

      for (uint32_t index = 0; index < header_p->func_scopes_count; index++)
      {
        bytecode_data_header_t *child_scope_header_p = MEM_CP_GET_NON_NULL_POINTER (bytecode_data_header_t,
                                                                                    declarations_p[index]);
        JERRY_ASSERT (child_scope_header_p->next_header_cp == MEM_CP_NULL);

        MEM_CP_SET_POINTER (child_scope_header_p->next_header_cp, next_to_handle_list_p);

        next_to_handle_list_p = child_scope_header_p;
      }

      mem_heap_free_block (header_p);
    }

    JERRY_ASSERT (bc_header_list_iter_p == NULL);
  }
} /* bc_free_bytecode_data */

/**
 * Delete bytecode and associated hash table
 */
void
bc_remove_bytecode_data (const bytecode_data_header_t *bytecode_data_p) /**< byte-code scope data header */
{
  bytecode_data_header_t *prev_header_p = NULL;
  bytecode_data_header_t *cur_header_p = first_bytecode_header_p;

  while (cur_header_p != NULL)
  {
    if (cur_header_p == bytecode_data_p)
    {
      if (prev_header_p)
      {
        prev_header_p->next_header_cp = cur_header_p->next_header_cp;
      }
      else
      {
        first_bytecode_header_p = MEM_CP_GET_POINTER (bytecode_data_header_t, cur_header_p->next_header_cp);
      }

      cur_header_p->next_header_cp = MEM_CP_NULL;

      bc_free_bytecode_data (cur_header_p);

      break;
    }

    prev_header_p = cur_header_p;
    cur_header_p = MEM_CP_GET_POINTER (bytecode_data_header_t, cur_header_p->next_header_cp);
  }
} /* bc_remove_bytecode_data */

vm_instr_t bc_get_instr (const bytecode_data_header_t *bytecode_data_p, /**< byte-code scope data header */
                         vm_instr_counter_t oc) /**< instruction position */
{
  JERRY_ASSERT (oc < bytecode_data_p->instrs_count);
  return bytecode_data_p->instrs_p[oc];
}

/**
 * Print bytecode instructions
 */
void
bc_print_instrs (const bytecode_data_header_t *bytecode_data_p) /**< byte-code scope data header */
{
#ifdef JERRY_ENABLE_PRETTY_PRINTER
  for (vm_instr_counter_t loc = 0; loc < bytecode_data_p->instrs_count; loc++)
  {
    op_meta opm;

    opm.op = bytecode_data_p->instrs_p[loc];
    for (int i = 0; i < 3; i++)
    {
      opm.lit_id[i] = NOT_A_LITERAL;
    }

    pp_op_meta (bytecode_data_p, loc, opm, false);
  }
#else
  (void) bytecode_data_p;
#endif
} /* bc_print_instrs */

/**
 * Dump single scopes tree into bytecode
 *
 * @return pointer to bytecode header of the outer most scope
 */
bytecode_data_header_t *
bc_dump_single_scope (scopes_tree scope_p) /**< a node of scopes tree */
{
  const size_t entries_count = scope_p->max_uniq_literals_num;
  const vm_instr_counter_t instrs_count = scopes_tree_instrs_num (scope_p);
  const size_t blocks_count = JERRY_ALIGNUP (instrs_count, BLOCK_SIZE) / BLOCK_SIZE;
  const size_t func_scopes_count = scopes_tree_child_scopes_num (scope_p);
  const uint16_t var_decls_count = linked_list_get_length (scope_p->var_decls);
  const size_t bytecode_size = JERRY_ALIGNUP (instrs_count * sizeof (vm_instr_t), MEM_ALIGNMENT);
  const size_t hash_table_size = lit_id_hash_table_get_size_for_table (entries_count, blocks_count);
  const size_t declarations_area_size = JERRY_ALIGNUP (func_scopes_count * sizeof (mem_cpointer_t)
                                                       + var_decls_count * sizeof (lit_cpointer_t),
                                                       MEM_ALIGNMENT);
  const size_t header_and_tables_size = JERRY_ALIGNUP ((sizeof (bytecode_data_header_t)
                                                        + hash_table_size
                                                        + declarations_area_size),
                                                       MEM_ALIGNMENT);

  uint8_t *buffer_p = (uint8_t *) mem_heap_alloc_block (bytecode_size + header_and_tables_size,
                                                        MEM_HEAP_ALLOC_LONG_TERM);

  lit_id_hash_table *lit_id_hash_p = lit_id_hash_table_init (buffer_p + sizeof (bytecode_data_header_t),
                                                             hash_table_size,
                                                             entries_count, blocks_count);

  mem_cpointer_t *declarations_p = (mem_cpointer_t *) (buffer_p + sizeof (bytecode_data_header_t) + hash_table_size);

  for (size_t i = 0; i < func_scopes_count; i++)
  {
    declarations_p[i] = MEM_CP_NULL;
  }

  scopes_tree_dump_var_decls (scope_p, (lit_cpointer_t *) (declarations_p + func_scopes_count));

  vm_instr_t *bytecode_p = (vm_instr_t *) (buffer_p + header_and_tables_size);

  JERRY_ASSERT (scope_p->max_uniq_literals_num >= lit_id_hash_p->current_bucket_pos);

  bytecode_data_header_t *header_p = (bytecode_data_header_t *) buffer_p;

  if ((uint16_t) func_scopes_count != func_scopes_count)
  {
    jerry_fatal (ERR_OUT_OF_MEMORY);
  }

  bc_fill_bytecode_data_header (header_p,
                                lit_id_hash_p, bytecode_p,
                                declarations_p,
                                (uint16_t) func_scopes_count,
                                var_decls_count,
                                scope_p->strict_mode,
                                scope_p->ref_arguments,
                                scope_p->ref_eval,
                                scope_p->is_vars_and_args_to_regs_possible,
                                false,
                                false);

  JERRY_ASSERT (scope_p->bc_header_cp == MEM_CP_NULL);
  MEM_CP_SET_NON_NULL_POINTER (scope_p->bc_header_cp, header_p);

  return header_p;
} /* bc_dump_single_scope */

void
bc_register_root_bytecode_header (bytecode_data_header_t *bc_header_p)
{
  MEM_CP_SET_POINTER (bc_header_p->next_header_cp, first_bytecode_header_p);
  first_bytecode_header_p = bc_header_p;
} /* bc_register_root_bytecode_header */

/**
 * Free all bytecode data which was allocated
 */
void
bc_finalize (void)
{
  while (first_bytecode_header_p != NULL)
  {
    bytecode_data_header_t *header_p = first_bytecode_header_p;
    first_bytecode_header_p = MEM_CP_GET_POINTER (bytecode_data_header_t, header_p->next_header_cp);

    header_p->next_header_cp = MEM_CP_NULL;

    bc_free_bytecode_data (header_p);
  }
} /* bc_finalize */

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
bc_get_literal_cp_by_uid (uint8_t id, /**< literal idx */
                          const bytecode_data_header_t *bytecode_data_p, /**< pointer to bytecode */
                          vm_instr_counter_t oc) /**< position in the bytecode */
{
  JERRY_ASSERT (bytecode_data_p);

  lit_id_hash_table *lit_id_hash = MEM_CP_GET_POINTER (lit_id_hash_table, bytecode_data_p->lit_id_hash_cp);

  if (lit_id_hash == NULL)
  {
    return INVALID_LITERAL;
  }

  return lit_id_hash_table_lookup (lit_id_hash, id, oc);
} /* bc_get_literal_cp_by_uid */

#ifdef JERRY_ENABLE_SNAPSHOT
/**
 * Find literal offset in the table literal->offset
 */
uint32_t
bc_find_lit_offset (lit_cpointer_t lit_cp, /**< literal to find */
                    const lit_mem_to_snapshot_id_map_entry_t *lit_map_p, /**< map from literal
                                                                          *   identifiers in
                                                                          *   literal storage
                                                                          *   to literal offsets
                                                                          *   in snapshot */
                    uint32_t literals_num) /**< number of entries in the map */
{
  uint32_t lit_index;
  for (lit_index = 0; lit_index < literals_num; lit_index++)
  {
    if (lit_map_p[lit_index].literal_id.packed_value == lit_cp.packed_value)
    {
      break;
    }
  }

  JERRY_ASSERT (lit_index < literals_num);

  return lit_map_p[lit_index].literal_offset;
} /* bc_find_lit_offset */

/**
 * Write alignment bytes to outptut buffer to align 'in_out_size' to MEM_ALIGNEMENT
 *
 * @return true if alignment bytes were written successfully
 *         else otherwise
 */
bool
bc_align_data_in_output_buffer (uint32_t *in_out_size, /**< in: unaligned size, out: aligned size */
                                uint8_t *buffer_p, /**< buffer where to write */
                                size_t buffer_size, /**< buffer size */
                                size_t *in_out_buffer_offset_p) /**< current offset in buffer */
{
  uint32_t aligned_size = JERRY_ALIGNUP (*in_out_size, MEM_ALIGNMENT);

  if (aligned_size != (*in_out_size))
  {
    JERRY_ASSERT (aligned_size > (*in_out_size));

    uint32_t padding_bytes_num = (uint32_t) (aligned_size - (*in_out_size));
    uint8_t padding = 0;

    for (uint32_t i = 0; i < padding_bytes_num; i++)
    {
      if (!jrt_write_to_buffer_by_offset (buffer_p, buffer_size, in_out_buffer_offset_p, &padding, sizeof (padding)))
      {
        return false;
      }
    }

    *in_out_size = aligned_size;
  }

  return true;
} /* bc_align_data_in_output_buffer */

/**
 * Dump byte-code and idx-to-literal map of a single scope to snapshot
 *
 * @return true, upon success (i.e. buffer size is enough),
 *         false - otherwise.
 */
static bool
bc_save_bytecode_with_idx_map (uint8_t *buffer_p, /**< buffer to dump to */
                               size_t buffer_size, /**< buffer size */
                               size_t *in_out_buffer_offset_p, /**< in-out: buffer write offset */
                               const bytecode_data_header_t *bytecode_data_p, /**< byte-code data */
                               const lit_mem_to_snapshot_id_map_entry_t *lit_map_p, /**< map from literal
                                                                                     *   identifiers in
                                                                                     *   literal storage
                                                                                     *   to literal offsets
                                                                                     *   in snapshot */
                               uint32_t literals_num) /**< literals number */
{
  JERRY_ASSERT (JERRY_ALIGNUP (*in_out_buffer_offset_p, MEM_ALIGNMENT) == *in_out_buffer_offset_p);

  jerry_snapshot_bytecode_header_t bytecode_header;
  bytecode_header.func_scopes_count = bytecode_data_p->func_scopes_count;
  bytecode_header.var_decls_count = bytecode_data_p->var_decls_count;
  bytecode_header.is_strict = bytecode_data_p->is_strict;
  bytecode_header.is_ref_arguments_identifier = bytecode_data_p->is_ref_arguments_identifier;
  bytecode_header.is_ref_eval_identifier = bytecode_data_p->is_ref_eval_identifier;
  bytecode_header.is_args_moved_to_regs = bytecode_data_p->is_args_moved_to_regs;
  bytecode_header.is_no_lex_env = bytecode_data_p->is_no_lex_env;
  size_t bytecode_header_offset = *in_out_buffer_offset_p;

  /* Dump instructions */
  *in_out_buffer_offset_p += JERRY_ALIGNUP (sizeof (jerry_snapshot_bytecode_header_t), MEM_ALIGNMENT);

  vm_instr_counter_t instrs_num = bytecode_data_p->instrs_count;

  const size_t instrs_array_size = sizeof (vm_instr_t) * instrs_num;
  if (*in_out_buffer_offset_p + instrs_array_size > buffer_size)
  {
    return false;
  }
  memcpy (buffer_p + *in_out_buffer_offset_p, bytecode_data_p->instrs_p, instrs_array_size);
  *in_out_buffer_offset_p += instrs_array_size;

  bytecode_header.instrs_size = (uint32_t) (sizeof (vm_instr_t) * instrs_num);

  /* Dump variable declarations */
  mem_cpointer_t *func_scopes_p = MEM_CP_GET_POINTER (mem_cpointer_t, bytecode_data_p->declarations_cp);
  lit_cpointer_t *var_decls_p = (lit_cpointer_t *) (func_scopes_p + bytecode_data_p->func_scopes_count);
  uint32_t null_var_decls_num = 0;
  for (uint32_t i = 0; i < bytecode_header.var_decls_count; ++i)
  {
    lit_cpointer_t lit_cp = var_decls_p[i];

    if (lit_cp.packed_value == MEM_CP_NULL)
    {
      null_var_decls_num++;
      continue;
    }

    uint32_t offset = bc_find_lit_offset (lit_cp, lit_map_p, literals_num);
    if (!jrt_write_to_buffer_by_offset (buffer_p, buffer_size, in_out_buffer_offset_p, &offset, sizeof (offset)))
    {
      return false;
    }
  }
  bytecode_header.var_decls_count -= null_var_decls_num;

  /* Dump uid->lit_cp hash table */
  lit_id_hash_table *lit_id_hash_p = MEM_CP_GET_POINTER (lit_id_hash_table, bytecode_data_p->lit_id_hash_cp);
  uint32_t idx_to_lit_map_size = lit_id_hash_table_dump_for_snapshot (buffer_p,
                                                                      buffer_size,
                                                                      in_out_buffer_offset_p,
                                                                      lit_id_hash_p,
                                                                      lit_map_p,
                                                                      literals_num,
                                                                      instrs_num);

  if (idx_to_lit_map_size == 0)
  {
    return false;
  }

  bytecode_header.idx_to_lit_map_size = idx_to_lit_map_size;

  /* Align to write next bytecode data at aligned address */
  bytecode_header.size = (uint32_t) (*in_out_buffer_offset_p - bytecode_header_offset);
  JERRY_ASSERT (bytecode_header.size == JERRY_ALIGNUP (sizeof (jerry_snapshot_bytecode_header_t), MEM_ALIGNMENT)
                                        + bytecode_header.instrs_size
                                        + bytecode_header.var_decls_count * sizeof (uint32_t)
                                        + idx_to_lit_map_size);

  if (!bc_align_data_in_output_buffer (&bytecode_header.size,
                                       buffer_p,
                                       buffer_size,
                                       in_out_buffer_offset_p))
  {
    return false;
  }

  /* Dump header at the saved offset */
  if (!jrt_write_to_buffer_by_offset (buffer_p,
                                      buffer_size,
                                      &bytecode_header_offset,
                                      &bytecode_header,
                                      sizeof (bytecode_header)))
  {
    return false;
  }

  return true;
} /* bc_save_bytecode_with_idx_map */


/**
 * Dump bytecode and summplementary data of all existing scopes to snapshot
 *
 * @return true if snapshot was dumped successfully
 *         false otherwise
 */
bool
bc_save_bytecode_data (uint8_t *buffer_p, /**< buffer to dump to */
                       size_t buffer_size, /**< buffer size */
                       size_t *in_out_buffer_offset_p, /**< in-out: buffer write offset */
                       const bytecode_data_header_t *bytecode_data_p, /**< byte-code data */
                       const lit_mem_to_snapshot_id_map_entry_t *lit_map_p, /**< map from literal
                                                                             *   identifiers in
                                                                             *   literal storage
                                                                             *   to literal offsets
                                                                             *   in snapshot */
                       uint32_t literals_num, /**< literals number */
                       uint32_t *out_scopes_num) /**< number of scopes written */
{
  bytecode_data_header_t *next_to_handle_list_p = first_bytecode_header_p;

  while (next_to_handle_list_p != NULL)
  {
    if (next_to_handle_list_p == bytecode_data_p)
    {
      break;
    }
    next_to_handle_list_p = MEM_CP_GET_POINTER (bytecode_data_header_t, next_to_handle_list_p->next_header_cp);
  }

  JERRY_ASSERT (next_to_handle_list_p);
  JERRY_ASSERT (next_to_handle_list_p->next_header_cp == MEM_CP_NULL);

  *out_scopes_num = 0;
  while (next_to_handle_list_p!= NULL)
  {
    bytecode_data_header_t *bc_header_list_iter_p = next_to_handle_list_p;
    next_to_handle_list_p = NULL;


    mem_cpointer_t *declarations_p = MEM_CP_GET_POINTER (mem_cpointer_t, bc_header_list_iter_p->declarations_cp);

    if (!bc_save_bytecode_with_idx_map (buffer_p,
                                        buffer_size,
                                        in_out_buffer_offset_p,
                                        bc_header_list_iter_p,
                                        lit_map_p,
                                        literals_num))
    {
      return false;
    }

    (*out_scopes_num)++;

    next_to_handle_list_p = MEM_CP_GET_POINTER (bytecode_data_header_t, bc_header_list_iter_p->next_header_cp);

    for (uint32_t index = bc_header_list_iter_p->func_scopes_count; index > 0 ; index--)
    {
      bytecode_data_header_t *child_scope_header_p = MEM_CP_GET_NON_NULL_POINTER (bytecode_data_header_t,
                                                                                  declarations_p[index-1]);

      JERRY_ASSERT (child_scope_header_p->next_header_cp == MEM_CP_NULL);

      MEM_CP_SET_POINTER (child_scope_header_p->next_header_cp, next_to_handle_list_p);

      next_to_handle_list_p = child_scope_header_p;
    }

    bc_header_list_iter_p->next_header_cp = MEM_CP_NULL;
  }

  return true;
} /* bc_save_bytecode_data */


/**
 * Register bytecode and supplementary data of a single scope from snapshot
 *
 * NOTE:
 *      If is_copy flag is set, bytecode is copied from snapshot, else bytecode is referenced directly
 *      from snapshot
 *
 * @return pointer to byte-code header, upon success,
 *         NULL - upon failure (i.e., in case snapshot format is not valid)
 */
static bytecode_data_header_t *
bc_load_bytecode_with_idx_map (const uint8_t *snapshot_data_p, /**< buffer with instructions array
                                                                *   and idx to literals map from
                                                                *   snapshot */
                               size_t snapshot_size, /**< remaining size of snapshot */
                               const lit_mem_to_snapshot_id_map_entry_t *lit_map_p, /**< map of in-snapshot
                                                                                     *   literal offsets
                                                                                     *   to literal identifiers,
                                                                                     *   created in literal
                                                                                     *   storage */
                               uint32_t literals_num, /**< number of literals */
                               bool is_copy, /** flag, indicating whether the passed in-snapshot data
                                              *  should be copied to engine's memory (true),
                                              *  or it can be referenced until engine is stopped
                                              *  (i.e. until call to jerry_cleanup) */
                               uint32_t *out_bytecode_data_size) /**< out: size occupied by bytecode data
                                                                  *   in snapshot */
{
  size_t buffer_offset = 0;
  jerry_snapshot_bytecode_header_t bytecode_header;
  if (!jrt_read_from_buffer_by_offset (snapshot_data_p,
                                       snapshot_size,
                                       &buffer_offset,
                                       &bytecode_header,
                                       sizeof (bytecode_header)))
  {
    return NULL;
  }

  *out_bytecode_data_size = bytecode_header.size;

  buffer_offset += (JERRY_ALIGNUP (sizeof (jerry_snapshot_bytecode_header_t), MEM_ALIGNMENT)
                    - sizeof (jerry_snapshot_bytecode_header_t));

  JERRY_ASSERT (bytecode_header.size <= snapshot_size);

  /* Read uid->lit_cp hash table size */
  const uint8_t *idx_to_lit_map_p = (snapshot_data_p
                                    + buffer_offset +
                                    + bytecode_header.instrs_size
                                    + bytecode_header.var_decls_count * sizeof (uint32_t));

  size_t instructions_number = bytecode_header.instrs_size / sizeof (vm_instr_t);
  size_t blocks_count = JERRY_ALIGNUP (instructions_number, BLOCK_SIZE) / BLOCK_SIZE;

  uint32_t idx_num_total;
  size_t idx_to_lit_map_offset = 0;
  if (!jrt_read_from_buffer_by_offset (idx_to_lit_map_p,
                                       bytecode_header.idx_to_lit_map_size,
                                       &idx_to_lit_map_offset,
                                       &idx_num_total,
                                       sizeof (idx_num_total)))
  {
    return NULL;
  }

  /* Alloc bytecode_header for runtime */
  const size_t bytecode_alloc_size = JERRY_ALIGNUP (bytecode_header.instrs_size, MEM_ALIGNMENT);
  const size_t hash_table_size = lit_id_hash_table_get_size_for_table (idx_num_total, blocks_count);
  const size_t declarations_area_size = JERRY_ALIGNUP (bytecode_header.func_scopes_count * sizeof (mem_cpointer_t)
                                                       + bytecode_header.var_decls_count * sizeof (lit_cpointer_t),
                                                       MEM_ALIGNMENT);
  const size_t header_and_tables_size = JERRY_ALIGNUP ((sizeof (bytecode_data_header_t)
                                                        + hash_table_size
                                                        + declarations_area_size),
                                                       MEM_ALIGNMENT);
  const size_t alloc_size = header_and_tables_size + (is_copy ? bytecode_alloc_size : 0);

  uint8_t *buffer_p = (uint8_t*) mem_heap_alloc_block (alloc_size, MEM_HEAP_ALLOC_LONG_TERM);
  bytecode_data_header_t *header_p = (bytecode_data_header_t *) buffer_p;

  vm_instr_t *instrs_p;
  vm_instr_t *snapshot_instrs_p = (vm_instr_t *) (snapshot_data_p + buffer_offset);
  if (is_copy)
  {
    instrs_p = (vm_instr_t *) (buffer_p + header_and_tables_size);
    memcpy (instrs_p, snapshot_instrs_p, bytecode_header.instrs_size);
  }
  else
  {
    instrs_p = snapshot_instrs_p;
  }

  buffer_offset += bytecode_header.instrs_size; /* buffer_offset is now offset of variable declarations */

  /* Read uid->lit_cp hash table */
  uint8_t *lit_id_hash_table_buffer_p = buffer_p + sizeof (bytecode_data_header_t);
  if (!(lit_id_hash_table_load_from_snapshot (blocks_count,
                                             idx_num_total,
                                             idx_to_lit_map_p + idx_to_lit_map_offset,
                                             bytecode_header.idx_to_lit_map_size - idx_to_lit_map_offset,
                                             lit_map_p,
                                             literals_num,
                                             lit_id_hash_table_buffer_p,
                                             hash_table_size)
       && (vm_instr_counter_t) instructions_number == instructions_number))
  {
    mem_heap_free_block (buffer_p);
    return NULL;
  }

  /* Fill with NULLs child scopes declarations for this scope */
  mem_cpointer_t *declarations_p = (mem_cpointer_t *) (buffer_p + sizeof (bytecode_data_header_t) + hash_table_size);
  memset (declarations_p, 0, bytecode_header.func_scopes_count * sizeof (mem_cpointer_t));

  /* Read variable declarations for this scope */
  lit_cpointer_t *var_decls_p = (lit_cpointer_t *) (declarations_p + bytecode_header.func_scopes_count);
  for (uint32_t i = 0; i < bytecode_header.var_decls_count; i++)
  {
    uint32_t lit_offset_from_snapshot;
    if (!jrt_read_from_buffer_by_offset (snapshot_data_p,
                                         buffer_offset + bytecode_header.var_decls_count * sizeof (uint32_t),
                                         &buffer_offset,
                                         &lit_offset_from_snapshot,
                                         sizeof (lit_offset_from_snapshot)))
    {
      mem_heap_free_block (buffer_p);
      return NULL;
    }
    /**
     * TODO: implement binary search here
     */
    lit_cpointer_t lit_cp = NOT_A_LITERAL;
    uint32_t j;
    for (j = 0; j < literals_num; j++)
    {
      if (lit_map_p[j].literal_offset == lit_offset_from_snapshot)
      {
        lit_cp.packed_value = lit_map_p[j].literal_id.packed_value;
        break;
      }
    }

    if (j == literals_num)
    {
      mem_heap_free_block (buffer_p);
      return NULL;
    }

    var_decls_p[i] = lit_cp;
  }

  /* Fill bytecode_data_header */
  bc_fill_bytecode_data_header (header_p,
                                (lit_id_hash_table *) lit_id_hash_table_buffer_p,
                                instrs_p,
                                declarations_p,
                                (uint16_t) bytecode_header.func_scopes_count,
                                (uint16_t) bytecode_header.var_decls_count,
                                bytecode_header.is_strict,
                                bytecode_header.is_ref_arguments_identifier,
                                bytecode_header.is_ref_eval_identifier,
                                bytecode_header.is_args_moved_to_regs,
                                bytecode_header.is_args_moved_to_regs,
                                bytecode_header.is_no_lex_env);

  return header_p;
} /* bc_load_bytecode_with_idx_map */

/**
 * Register bytecode and supplementary data of all scopes from snapshot
 *
 * NOTE:
 *      If is_copy flag is set, bytecode is copied from snapshot, else bytecode is referenced directly
 *      from snapshot
 *
 * @return pointer to byte-code header, upon success,
 *         NULL - upon failure (i.e., in case snapshot format is not valid)
 */
const bytecode_data_header_t *
bc_load_bytecode_data (const uint8_t *snapshot_data_p, /**< buffer with instructions array
                                                        *   and idx to literals map from
                                                        *   snapshot */
                       size_t snapshot_size, /**< remaining size of snapshot */
                       const lit_mem_to_snapshot_id_map_entry_t *lit_map_p, /**< map of in-snapshot
                                                                             *   literal offsets
                                                                             *   to literal identifiers,
                                                                             *   created in literal
                                                                             *   storage */
                       uint32_t literals_num, /**< number of literals */
                       bool is_copy, /** flag, indicating whether the passed in-snapshot data
                                      *  should be copied to engine's memory (true),
                                      *  or it can be referenced until engine is stopped
                                      *  (i.e. until call to jerry_cleanup) */
                       uint32_t expected_scopes_num) /**< scopes number read from snapshot header */
{
  uint32_t snapshot_offset = 0;
  uint32_t out_bytecode_data_size = 0;
  uint32_t scopes_num = 0;

  bytecode_data_header_t *bc_header_p = bc_load_bytecode_with_idx_map (snapshot_data_p,
                                                                       snapshot_size,
                                                                       lit_map_p,
                                                                       literals_num,
                                                                       is_copy,
                                                                       &out_bytecode_data_size);

  scopes_num++;
  snapshot_offset += out_bytecode_data_size;
  JERRY_ASSERT (snapshot_offset <= snapshot_size);

  bytecode_data_header_t* next_to_handle_list_p = bc_header_p;

  while (next_to_handle_list_p != NULL)
  {
    mem_cpointer_t *declarations_p = MEM_CP_GET_POINTER (mem_cpointer_t, next_to_handle_list_p->declarations_cp);
    uint32_t child_scope_index = 0;
    while (child_scope_index < next_to_handle_list_p->func_scopes_count
           && declarations_p[child_scope_index] != MEM_CP_NULL)
    {
      child_scope_index++;
    }

    if (child_scope_index == next_to_handle_list_p->func_scopes_count)
    {
      bytecode_data_header_t *bc_header_list_iter_p = MEM_CP_GET_POINTER (bytecode_data_header_t,
                                                                          next_to_handle_list_p->next_header_cp);

      next_to_handle_list_p->next_header_cp = MEM_CP_NULL;
      next_to_handle_list_p = bc_header_list_iter_p;

      if (next_to_handle_list_p == MEM_CP_NULL)
      {
        break;
      }
      else
      {
        continue;
      }
    }

    JERRY_ASSERT (snapshot_offset < snapshot_size);
    bytecode_data_header_t *next_header_p = bc_load_bytecode_with_idx_map (snapshot_data_p + snapshot_offset,
                                                                           snapshot_size - snapshot_offset,
                                                                           lit_map_p,
                                                                           literals_num,
                                                                           is_copy,
                                                                           &out_bytecode_data_size);

    scopes_num++;

    snapshot_offset += out_bytecode_data_size;
    JERRY_ASSERT (snapshot_offset <= snapshot_size);

    MEM_CP_SET_NON_NULL_POINTER (declarations_p[child_scope_index], next_header_p);

    if (next_header_p->func_scopes_count > 0)
    {
      JERRY_ASSERT (next_header_p->next_header_cp == MEM_CP_NULL);

      MEM_CP_SET_POINTER (next_header_p->next_header_cp, next_to_handle_list_p);
      next_to_handle_list_p = next_header_p;
    }
  }

  if (expected_scopes_num != scopes_num)
  {
    return NULL;
  }

  MEM_CP_SET_POINTER (bc_header_p->next_header_cp, first_bytecode_header_p);

  first_bytecode_header_p = bc_header_p;

  return bc_header_p;
} /* bc_load_bytecode_data */

#endif /* JERRY_ENABLE_SNAPSHOT */

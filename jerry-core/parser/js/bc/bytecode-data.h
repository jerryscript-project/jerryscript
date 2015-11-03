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

#ifndef BYTECODE_DATA_H
#define BYTECODE_DATA_H

#include "opcodes.h"
#include "mem-allocator.h"
#include "lit-id-hash-table.h"
#include "scopes-tree.h"

/*
 * All literals are kept in the 'literals' array.
 * Literal structure doesn't hold real string. All program-specific strings
 * are kept in the 'strings_buffer' and literal has pointer to this buffer.
 *
 * Literal id is its index in 'literals' array of bytecode_data_t structure.
 *
 * Bytecode, which is kept in the 'instrs' field, is divided into blocks
 * of 'BLOCK_SIZE' operands. Every block has its own numbering of literals.
 * Literal uid could be in range [0, 127] in every block.
 *
 * To map uid to literal id 'lit_id_hash' table is used.
 */
#define BLOCK_SIZE 32u

/**
 * Header of byte-code memory region, containing byte-code array and literal identifiers hash table
 */
typedef struct __attribute__ ((aligned (MEM_ALIGNMENT))) bytecode_data_header_t
{
  vm_instr_t *instrs_p; /**< pointer to the bytecode */
  vm_instr_counter_t instrs_count; /**< number of instructions in the byte-code array */
  mem_cpointer_t lit_id_hash_cp; /**< pointer to literal identifiers hash table
                                  *   See also: lit_id_hash_table_init */

  mem_cpointer_t declarations_cp; /**< function scopes and variable declarations inside current scope */
  uint16_t func_scopes_count; /**< count of function scopes inside current scope */
  uint16_t var_decls_count; /**< count of variable declrations inside current scope */

  mem_cpointer_t next_header_cp; /**< pointer to next instructions data header */

  uint8_t is_strict : 1; /**< code is strict mode code */
  uint8_t is_ref_arguments_identifier : 1; /**< code doesn't reference 'arguments' identifier */
  uint8_t is_ref_eval_identifier : 1; /**< code doesn't reference 'eval' identifier */
  uint8_t is_vars_and_args_to_regs_possible : 1; /**< flag, indicating whether it is possible
                                                  *   to safely perform var-to-reg
                                                  *   optimization on the scope
                                                  *
                                                  *   TODO: remove the flag when var-to-reg optimization
                                                  *         would be moved from post-parse to dump stage */
  uint8_t is_args_moved_to_regs : 1; /**< the function's arguments are moved to registers,
                                      *   so should be initialized in vm registers,
                                      *   and not in lexical environment */
  uint8_t is_no_lex_env : 1; /**< no lex. env. is necessary for the scope */
} bytecode_data_header_t;

JERRY_STATIC_ASSERT (sizeof (bytecode_data_header_t) % MEM_ALIGNMENT == 0);

void bc_remove_bytecode_data (const bytecode_data_header_t *);

vm_instr_t bc_get_instr (const bytecode_data_header_t *,
                         vm_instr_counter_t);

void bc_print_instrs (const bytecode_data_header_t *);

bytecode_data_header_t *bc_dump_single_scope (scopes_tree);
void bc_register_root_bytecode_header (bytecode_data_header_t *);

void bc_finalize ();

lit_cpointer_t
bc_get_literal_cp_by_uid (uint8_t,
                          const bytecode_data_header_t *,
                          vm_instr_counter_t);


#ifdef JERRY_ENABLE_SNAPSHOT
/*
 * Snapshot-related
 */
uint32_t
bc_find_lit_offset (lit_cpointer_t, const lit_mem_to_snapshot_id_map_entry_t *, uint32_t);

bool
bc_align_data_in_output_buffer (uint32_t *, uint8_t *, size_t, size_t *);

bool
bc_save_bytecode_data (uint8_t *, size_t, size_t *, const bytecode_data_header_t *,
                       const lit_mem_to_snapshot_id_map_entry_t *, uint32_t, uint32_t *);

const bytecode_data_header_t *
bc_load_bytecode_data (const uint8_t *, size_t,
                       const lit_mem_to_snapshot_id_map_entry_t *, uint32_t, bool, uint32_t);
#endif /* JERRY_ENABLE_SNAPSHOT */

#endif /* BYTECODE_DATA_H */

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

#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "bytecode-data.h"
#include "jrt.h"
#include "ecma-globals.h"
#include "opcodes.h"
#include "vm.h"
#include "scopes-tree.h"

#define NOT_A_LITERAL (lit_cpointer_t::null_cp ())

void serializer_init ();
void serializer_set_show_instrs (bool show_instrs);
op_meta serializer_get_op_meta (vm_instr_counter_t);
op_meta serializer_get_var_decl (vm_instr_counter_t);
vm_instr_t serializer_get_instr (const bytecode_data_header_t *, vm_instr_counter_t);
lit_cpointer_t serializer_get_literal_cp_by_uid (uint8_t, const bytecode_data_header_t *, vm_instr_counter_t);
void serializer_set_scope (scopes_tree);
void serializer_dump_subscope (scopes_tree);
const bytecode_data_header_t *serializer_merge_scopes_into_bytecode (void);
void serializer_dump_op_meta (op_meta);
void serializer_dump_var_decl (op_meta);
vm_instr_counter_t serializer_get_current_instr_counter (void);
vm_instr_counter_t serializer_get_current_var_decls_counter (void);
vm_instr_counter_t serializer_count_instrs_in_subscopes (void);
void serializer_set_writing_position (vm_instr_counter_t);
void serializer_rewrite_op_meta (vm_instr_counter_t, op_meta);
void serializer_remove_bytecode_data (const bytecode_data_header_t *);
void serializer_free (void);

#ifdef JERRY_ENABLE_SNAPSHOT
/*
 * Snapshot-related
 */
bool serializer_dump_bytecode_with_idx_map (uint8_t *buffer_p,
                                            size_t buffer_size,
                                            size_t *in_out_buffer_offset_p,
                                            const bytecode_data_header_t *bytecode_data_p,
                                            const lit_mem_to_snapshot_id_map_entry_t *lit_map_p,
                                            uint32_t literals_num,
                                            uint32_t *out_bytecode_size_p,
                                            uint32_t *out_idx_to_lit_map_size_p);

const bytecode_data_header_t *
serializer_load_bytecode_with_idx_map (const uint8_t *bytecode_and_idx_map_p,
                                       uint32_t bytecode_size,
                                       uint32_t idx_to_lit_map_size,
                                       const lit_mem_to_snapshot_id_map_entry_t *lit_map_p,
                                       uint32_t literals_num,
                                       bool is_copy);
#endif /* JERRY_ENABLE_SNAPSHOT */

#endif /* SERIALIZER_H */

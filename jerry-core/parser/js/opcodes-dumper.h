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

#ifndef OPCODES_DUMPER_H
#define OPCODES_DUMPER_H

#include "ecma-globals.h"
#include "jsp-internal.h"
#include "lexer.h"
#include "lit-literal.h"
#include "opcodes.h"
#include "rcs-records.h"
#include "scopes-tree.h"

/**
 * Operand types
 */
typedef enum __attr_packed___
{
  JSP_OPERAND_TYPE_EMPTY, /**< empty operand */
  JSP_OPERAND_TYPE_STRING_LITERAL, /**< operand contains string literal value */
  JSP_OPERAND_TYPE_NUMBER_LITERAL, /**< operand contains number literal value */
  JSP_OPERAND_TYPE_REGEXP_LITERAL, /**< operand contains regexp literal value */
  JSP_OPERAND_TYPE_SIMPLE_VALUE, /**< operand contains a simple ecma value */
  JSP_OPERAND_TYPE_SMALLINT, /**< operand contains small integer value (less than 256) */
  JSP_OPERAND_TYPE_IDENTIFIER, /**< Identifier reference */
  JSP_OPERAND_TYPE_THIS_BINDING, /**< ThisBinding operand */
  JSP_OPERAND_TYPE_TMP, /**< operand contains byte-code register index */
  JSP_OPERAND_TYPE_IDX_CONST, /**< operand contains an integer constant that fits vm_idx_t */
  JSP_OPERAND_TYPE_UNKNOWN, /**< operand, representing unknown value that would be rewritten later */
  JSP_OPERAND_TYPE_UNINITIALIZED /**< uninitialized operand
                                  *
                                  *   Note:
                                  *      For use only in assertions to check that operands
                                  *      are initialized before actual usage */
} jsp_operand_type_t;

/**
 * Operand (descriptor of value or reference in context of parser)
 */
typedef struct
{
  union
  {
    vm_idx_t idx_const; /**< idx constant value (for jsp_operand_t::IDX_CONST) */
    vm_idx_t uid; /**< register index (for jsp_operand_t::TMP) */
    lit_cpointer_t lit_id; /**< literal (for jsp_operand_t::LITERAL) */
    lit_cpointer_t identifier; /**< Identifier reference (is_value_based_ref flag not set) */
    uint8_t smallint_value; /**< small integer value */
    uint8_t simple_value; /**< simple ecma value */
  } data;

  jsp_operand_type_t type; /**< type of operand */
} jsp_operand_t;

extern jsp_operand_t jsp_make_uninitialized_operand (void);
extern jsp_operand_t jsp_make_empty_operand (void);
extern jsp_operand_t jsp_make_this_operand (void);
extern jsp_operand_t jsp_make_unknown_operand (void);
extern jsp_operand_t jsp_make_idx_const_operand (vm_idx_t);
extern jsp_operand_t jsp_make_smallint_operand (uint8_t);
extern jsp_operand_t jsp_make_simple_value_operand (ecma_simple_value_t);
extern jsp_operand_t jsp_make_string_lit_operand (lit_cpointer_t);
extern jsp_operand_t jsp_make_regexp_lit_operand (lit_cpointer_t);
extern jsp_operand_t jsp_make_number_lit_operand (lit_cpointer_t);
extern jsp_operand_t jsp_make_identifier_operand (lit_cpointer_t);
extern jsp_operand_t jsp_make_reg_operand (vm_idx_t);

extern bool jsp_is_empty_operand (jsp_operand_t);
extern bool jsp_is_this_operand (jsp_operand_t);
extern bool jsp_is_unknown_operand (jsp_operand_t);
extern bool jsp_is_idx_const_operand (jsp_operand_t);
extern bool jsp_is_register_operand (jsp_operand_t);
extern bool jsp_is_simple_value_operand (jsp_operand_t);
extern bool jsp_is_smallint_operand (jsp_operand_t);
extern bool jsp_is_number_lit_operand (jsp_operand_t);
extern bool jsp_is_string_lit_operand (jsp_operand_t);
extern bool jsp_is_regexp_lit_operand (jsp_operand_t);
extern bool jsp_is_identifier_operand (jsp_operand_t);

extern lit_cpointer_t jsp_operand_get_identifier_name (jsp_operand_t);
extern lit_cpointer_t jsp_operand_get_literal (jsp_operand_t);
extern vm_idx_t jsp_operand_get_idx (jsp_operand_t);
extern vm_idx_t jsp_operand_get_idx_const (jsp_operand_t);
extern ecma_simple_value_t jsp_operand_get_simple_value (jsp_operand_t);
extern uint8_t jsp_operand_get_smallint_value (jsp_operand_t);

JERRY_STATIC_ASSERT (sizeof (jsp_operand_t) == 4);

typedef enum __attr_packed___
{
  VARG_FUNC_DECL,
  VARG_FUNC_EXPR,
  VARG_ARRAY_DECL,
  VARG_OBJ_DECL,
  VARG_CONSTRUCT_EXPR,
  VARG_CALL_EXPR
} varg_list_type;

extern jsp_operand_t empty_operand (void);
extern jsp_operand_t tmp_operand (void);
extern bool operand_is_empty (jsp_operand_t);

extern void dumper_init (jsp_ctx_t *, bool);

extern vm_instr_counter_t dumper_get_current_instr_counter (jsp_ctx_t *);

extern void dumper_start_move_of_vars_to_regs (jsp_ctx_t *);
extern bool dumper_start_move_of_args_to_regs (jsp_ctx_t *, uint32_t args_num);
extern bool dumper_try_replace_identifier_name_with_reg (jsp_ctx_t *, bytecode_data_header_t *, op_meta *);
extern void dumper_alloc_reg_for_unused_arg (jsp_ctx_t *);

extern void dumper_new_statement (jsp_ctx_t *);
extern void dumper_save_reg_alloc_ctx (jsp_ctx_t *, vm_idx_t *, vm_idx_t *);
extern void dumper_restore_reg_alloc_ctx (jsp_ctx_t *, vm_idx_t, vm_idx_t, bool);
extern vm_idx_t dumper_save_reg_alloc_counter (jsp_ctx_t *);
extern void dumper_restore_reg_alloc_counter (jsp_ctx_t *, vm_idx_t);

extern bool dumper_is_eval_literal (jsp_operand_t);

extern void dump_variable_assignment (jsp_ctx_t *, jsp_operand_t, jsp_operand_t);

extern vm_instr_counter_t dump_varg_header_for_rewrite (jsp_ctx_t *, varg_list_type, jsp_operand_t, jsp_operand_t);
extern void rewrite_varg_header_set_args_count (jsp_ctx_t *, size_t, vm_instr_counter_t);
extern void dump_call_additional_info (jsp_ctx_t *, opcode_call_flags_t, jsp_operand_t);
extern void dump_varg (jsp_ctx_t *, jsp_operand_t);

extern void dump_prop_name_and_value (jsp_ctx_t *, jsp_operand_t, jsp_operand_t);
extern void dump_prop_getter_decl (jsp_ctx_t *, jsp_operand_t, jsp_operand_t);
extern void dump_prop_setter_decl (jsp_ctx_t *, jsp_operand_t, jsp_operand_t);
extern void dump_prop_getter (jsp_ctx_t *, jsp_operand_t, jsp_operand_t, jsp_operand_t);
extern void dump_prop_setter (jsp_ctx_t *, jsp_operand_t, jsp_operand_t, jsp_operand_t);

extern vm_instr_counter_t dump_conditional_check_for_rewrite (jsp_ctx_t *, jsp_operand_t);
extern void rewrite_conditional_check (jsp_ctx_t *, vm_instr_counter_t);
extern vm_instr_counter_t dump_jump_to_end_for_rewrite (jsp_ctx_t *);
extern void rewrite_jump_to_end (jsp_ctx_t *, vm_instr_counter_t);

extern vm_instr_counter_t dumper_set_next_iteration_target (jsp_ctx_t *);
extern vm_instr_counter_t dump_simple_or_nested_jump_for_rewrite (jsp_ctx_t *,
                                                           bool,
                                                           bool,
                                                           bool,
                                                           jsp_operand_t,
                                                           vm_instr_counter_t);
extern vm_instr_counter_t rewrite_simple_or_nested_jump_and_get_next (jsp_ctx_t *,
                                                                      vm_instr_counter_t,
                                                                      vm_instr_counter_t);
extern void dump_continue_iterations_check (jsp_ctx_t *, vm_instr_counter_t, jsp_operand_t);

extern void dump_delete (jsp_ctx_t *, jsp_operand_t, jsp_operand_t);
extern void dump_delete_prop (jsp_ctx_t *, jsp_operand_t, jsp_operand_t, jsp_operand_t);

extern void dump_typeof (jsp_ctx_t *, jsp_operand_t, jsp_operand_t);

extern void dump_unary_op (jsp_ctx_t *, vm_op_t, jsp_operand_t, jsp_operand_t);
extern void dump_binary_op (jsp_ctx_t *, vm_op_t, jsp_operand_t, jsp_operand_t, jsp_operand_t);

extern vm_instr_counter_t dump_with_for_rewrite (jsp_ctx_t *, jsp_operand_t);
extern void rewrite_with (jsp_ctx_t *, vm_instr_counter_t);
extern void dump_with_end (jsp_ctx_t *);

extern vm_instr_counter_t dump_for_in_for_rewrite (jsp_ctx_t *, jsp_operand_t);
extern void rewrite_for_in (jsp_ctx_t *, vm_instr_counter_t);
extern void dump_for_in_end (jsp_ctx_t *);

extern vm_instr_counter_t dump_try_for_rewrite (jsp_ctx_t *);
extern vm_instr_counter_t dump_catch_for_rewrite (jsp_ctx_t *, jsp_operand_t);
extern vm_instr_counter_t dump_finally_for_rewrite (jsp_ctx_t *);
extern void rewrite_try (jsp_ctx_t *, vm_instr_counter_t);
extern void rewrite_catch (jsp_ctx_t *, vm_instr_counter_t);
extern void rewrite_finally (jsp_ctx_t *, vm_instr_counter_t);
extern void dump_end_try_catch_finally (jsp_ctx_t *);
extern void dump_throw (jsp_ctx_t *, jsp_operand_t);

extern void dump_variable_declaration (jsp_ctx_t *, lit_cpointer_t);

extern vm_instr_counter_t dump_reg_var_decl_for_rewrite (jsp_ctx_t *);
extern void rewrite_reg_var_decl (jsp_ctx_t *, vm_instr_counter_t);

extern void dump_ret (jsp_ctx_t *);
extern void dump_retval (jsp_ctx_t *, jsp_operand_t);

extern op_meta dumper_get_op_meta (jsp_ctx_t *, vm_instr_counter_t);
extern void dumper_rewrite_op_meta (jsp_ctx_t *, vm_instr_counter_t, op_meta);

#endif /* !OPCODES_DUMPER_H */

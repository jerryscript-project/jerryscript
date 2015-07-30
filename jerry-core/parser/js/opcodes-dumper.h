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

#include "opcodes.h"
#include "ecma-globals.h"
#include "lexer.h"

typedef enum __attr_packed___
{
  OPERAND_LITERAL,
  OPERAND_TMP
} operand_type;

typedef struct
{
  operand_type type;
  union
  {
    idx_t uid;
    lit_cpointer_t lit_id;
  } data;
} operand;

typedef enum __attr_packed___
{
  VARG_FUNC_DECL,
  VARG_FUNC_EXPR,
  VARG_ARRAY_DECL,
  VARG_OBJ_DECL,
  VARG_CONSTRUCT_EXPR,
  VARG_CALL_EXPR
} varg_list_type;

operand empty_operand (void);
operand literal_operand (lit_cpointer_t);
operand eval_ret_operand (void);
operand jsp_create_operand_for_in_special_reg (void);
bool operand_is_empty (operand);

void dumper_init (void);
void dumper_free (void);

void dumper_new_statement (void);
void dumper_new_scope (void);
void dumper_finish_scope (void);
void dumper_start_varg_code_sequence (void);
void dumper_finish_varg_code_sequence (void);

extern bool dumper_is_eval_literal (operand);

void dump_boolean_assignment (operand, bool);
operand dump_boolean_assignment_res (bool);
void dump_string_assignment (operand, lit_cpointer_t);
operand dump_string_assignment_res (lit_cpointer_t);
void dump_number_assignment (operand, lit_cpointer_t);
operand dump_number_assignment_res (lit_cpointer_t);
void dump_regexp_assignment (operand, lit_cpointer_t);
operand dump_regexp_assignment_res (lit_cpointer_t);
void dump_smallint_assignment (operand, idx_t);
operand dump_smallint_assignment_res (idx_t);
void dump_undefined_assignment (operand);
operand dump_undefined_assignment_res (void);
void dump_null_assignment (operand);
operand dump_null_assignment_res (void);
void dump_variable_assignment (operand, operand);
operand dump_variable_assignment_res (operand);

void dump_varg_header_for_rewrite (varg_list_type, operand);
operand rewrite_varg_header_set_args_count (size_t);
void dump_call_additional_info (opcode_call_flags_t, operand);
void dump_varg (operand);

void dump_prop_name_and_value (operand, operand);
void dump_prop_getter_decl (operand, operand);
void dump_prop_setter_decl (operand, operand);
void dump_prop_getter (operand, operand, operand);
operand dump_prop_getter_res (operand, operand);
void dump_prop_setter (operand, operand, operand);

void dump_function_end_for_rewrite (void);
void rewrite_function_end (varg_list_type);

void dump_this (operand);
operand dump_this_res (void);

void dump_post_increment (operand, operand);
operand dump_post_increment_res (operand);
void dump_post_decrement (operand, operand);
operand dump_post_decrement_res (operand);
void dump_pre_increment (operand, operand);
operand dump_pre_increment_res (operand);
void dump_pre_decrement (operand, operand);
operand dump_pre_decrement_res (operand);
void dump_unary_plus (operand, operand);
operand dump_unary_plus_res (operand);
void dump_unary_minus (operand, operand);
operand dump_unary_minus_res (operand);
void dump_bitwise_not (operand, operand);
operand dump_bitwise_not_res (operand);
void dump_logical_not (operand, operand);
operand dump_logical_not_res (operand);

void dump_multiplication (operand, operand, operand);
operand dump_multiplication_res (operand, operand);
void dump_division (operand, operand, operand);
operand dump_division_res (operand, operand);
void dump_remainder (operand, operand, operand);
operand dump_remainder_res (operand, operand);
void dump_addition (operand, operand, operand);
operand dump_addition_res (operand, operand);
void dump_substraction (operand, operand, operand);
operand dump_substraction_res (operand, operand);
void dump_left_shift (operand, operand, operand);
operand dump_left_shift_res (operand, operand);
void dump_right_shift (operand, operand, operand);
operand dump_right_shift_res (operand, operand);
void dump_right_shift_ex (operand, operand, operand);
operand dump_right_shift_ex_res (operand, operand);
void dump_less_than (operand, operand, operand);
operand dump_less_than_res (operand, operand);
void dump_greater_than (operand, operand, operand);
operand dump_greater_than_res (operand, operand);
void dump_less_or_equal_than (operand, operand, operand);
operand dump_less_or_equal_than_res (operand, operand);
void dump_greater_or_equal_than (operand, operand, operand);
operand dump_greater_or_equal_than_res (operand, operand);
void dump_instanceof (operand, operand, operand);
operand dump_instanceof_res (operand, operand);
void dump_in (operand, operand, operand);
operand dump_in_res (operand, operand);
void dump_equal_value (operand, operand, operand);
operand dump_equal_value_res (operand, operand);
void dump_not_equal_value (operand, operand, operand);
operand dump_not_equal_value_res (operand, operand);
void dump_equal_value_type (operand, operand, operand);
operand dump_equal_value_type_res (operand, operand);
void dump_not_equal_value_type (operand, operand, operand);
operand dump_not_equal_value_type_res (operand, operand);
void dump_bitwise_and (operand, operand, operand);
operand dump_bitwise_and_res (operand, operand);
void dump_bitwise_xor (operand, operand, operand);
operand dump_bitwise_xor_res (operand, operand);
void dump_bitwise_or (operand, operand, operand);
operand dump_bitwise_or_res (operand, operand);

void start_dumping_logical_and_checks (void);
void dump_logical_and_check_for_rewrite (operand);
void rewrite_logical_and_checks (void);
void start_dumping_logical_or_checks (void);
void dump_logical_or_check_for_rewrite (operand);
void rewrite_logical_or_checks (void);
void dump_conditional_check_for_rewrite (operand);
void rewrite_conditional_check (void);
void dump_jump_to_end_for_rewrite (void);
void rewrite_jump_to_end (void);

void start_dumping_assignment_expression (void);
operand dump_prop_setter_or_variable_assignment_res (operand, operand);
operand dump_prop_setter_or_addition_res (operand, operand);
operand dump_prop_setter_or_multiplication_res (operand, operand);
operand dump_prop_setter_or_division_res (operand, operand);
operand dump_prop_setter_or_remainder_res (operand, operand);
operand dump_prop_setter_or_substraction_res (operand, operand);
operand dump_prop_setter_or_left_shift_res (operand, operand);
operand dump_prop_setter_or_right_shift_res (operand, operand);
operand dump_prop_setter_or_right_shift_ex_res (operand, operand);
operand dump_prop_setter_or_bitwise_and_res (operand, operand);
operand dump_prop_setter_or_bitwise_xor_res (operand, operand);
operand dump_prop_setter_or_bitwise_or_res (operand, operand);

void dumper_set_break_target (void);
void dumper_set_continue_target (void);
void dumper_set_next_interation_target (void);
vm_instr_counter_t
dump_simple_or_nested_jump_for_rewrite (bool is_simple_jump,
                                        vm_instr_counter_t next_jump_for_tg_oc);
vm_instr_counter_t
rewrite_simple_or_nested_jump_and_get_next (vm_instr_counter_t jump_oc,
                                            vm_instr_counter_t target_oc);
void dump_continue_iterations_check (operand);

void start_dumping_case_clauses (void);
void dump_case_clause_check_for_rewrite (operand, operand);
void dump_default_clause_check_for_rewrite (void);
void rewrite_case_clause (void);
void rewrite_default_clause (void);
void finish_dumping_case_clauses (void);

void dump_delete (operand, operand, bool, locus);
operand dump_delete_res (operand, bool, locus);

void dump_typeof (operand, operand);
operand dump_typeof_res (operand);

vm_instr_counter_t dump_with_for_rewrite (operand);
void rewrite_with (vm_instr_counter_t);
void dump_with_end (void);

vm_instr_counter_t dump_for_in_for_rewrite (operand);
void rewrite_for_in (vm_instr_counter_t);
void dump_for_in_end (void);

void dump_try_for_rewrite (void);
void rewrite_try (void);
void dump_catch_for_rewrite (operand);
void rewrite_catch (void);
void dump_finally_for_rewrite (void);
void rewrite_finally (void);
void dump_end_try_catch_finally (void);
void dump_throw (operand);

bool dumper_variable_declaration_exists (lit_cpointer_t);
void dump_variable_declaration (lit_cpointer_t);

vm_instr_counter_t dump_scope_code_flags_for_rewrite (void);
void rewrite_scope_code_flags (vm_instr_counter_t scope_code_flags_oc,
                               opcode_scope_code_flags_t scope_flags);

void dump_reg_var_decl_for_rewrite (void);
void rewrite_reg_var_decl (void);

void dump_ret (void);
void dump_retval (operand op);

#endif /* OPCODES_DUMPER_H */

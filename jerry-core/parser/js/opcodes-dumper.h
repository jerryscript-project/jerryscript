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
#include "lexer.h"
#include "opcodes.h"
#include "scopes-tree.h"
#include "serializer.h"

/**
 * Operand (descriptor of value or reference in context of parser)
 */
class jsp_operand_t
{
public:
  enum type_t : uint8_t
  {
    EMPTY, /**< empty operand */
    LITERAL, /**< operand contains literal value */
    TMP, /**< operand contains byte-code register index */
    IDX_CONST, /**< operand contains an integer constant that fits vm_idx_t */
    UNKNOWN, /**< operand, representing unknown value that would be rewritten later */
    UNINITIALIZED /**< uninitialized operand
                   *
                   *   Note:
                   *      For use only in assertions to check that operands
                   *      are initialized before actual usage */
  };

  /**
   * Construct operand template
   */
  jsp_operand_t (void)
  {
    _type = jsp_operand_t::UNINITIALIZED;
    _is_to_be_freed = true;
  } /* jsp_operand_t */

  /**
   * Copy constructor
   *
   * NOTE:
   *     When copy of an operand is created, the new operand becomes responsible for clearing its register
   *     (only for TMP operands)
   */
  jsp_operand_t (const jsp_operand_t &op) : /**< operand, which copy is created */
  _data (op._data)
  {
    _type = op._type;
    _is_to_be_freed = op._is_to_be_freed;

    op._is_to_be_freed = false;
  } /* jsp_operand_t */

  jsp_operand_t& operator= (const jsp_operand_t &op);

  void free_reg () const;

  ~jsp_operand_t ();

  /**
   * Construct empty operand
   *
   * @return constructed operand
   */
  static jsp_operand_t
  make_empty_operand (void)
  {
    jsp_operand_t ret;

    ret._type = jsp_operand_t::EMPTY;

    return ret;
  } /* make_empty_operand */

  /**
   * Construct unknown operand
   *
   * @return constructed operand
   */
  static jsp_operand_t
  make_unknown_operand (void)
  {
    jsp_operand_t ret;

    ret._type = jsp_operand_t::UNKNOWN;

    return ret;
  } /* make_unknown_operand */

  /**
   * Construct idx-constant operand
   *
   * @return constructed operand
   */
  static jsp_operand_t
  make_idx_const_operand (vm_idx_t cnst) /**< integer in vm_idx_t range */
  {
    jsp_operand_t ret;

    ret._type = jsp_operand_t::IDX_CONST;
    ret._data.idx_const = cnst;

    return ret;
  } /* make_idx_const_operand */

  /**
   * Construct literal operand
   *
   * @return constructed operand
   */
  static jsp_operand_t
  make_lit_operand (lit_cpointer_t lit_id) /**< literal identifier */
  {
    JERRY_ASSERT (lit_id.packed_value != NOT_A_LITERAL.packed_value);

    jsp_operand_t ret;

    ret._type = jsp_operand_t::LITERAL;
    ret._data.lit_id = lit_id;

    return ret;
  } /* make_lit_operand */

  /**
   * Construct register operand
   *
   * @return constructed operand
   */
  static jsp_operand_t
  make_reg_operand (vm_idx_t reg_index) /**< register index */
  {
    /*
     * The following check currently leads to 'comparison is always true
     * due to limited range of data type' warning, so it is turned off.
     *
     * If VM_IDX_GENERAL_VALUE_FIRST is changed to value greater than 0,
     * the check should be restored.
     */
    // JERRY_ASSERT (reg_index >= VM_IDX_GENERAL_VALUE_FIRST);
    static_assert (VM_IDX_GENERAL_VALUE_FIRST == 0, "See comment above");

    JERRY_ASSERT (reg_index <= VM_IDX_GENERAL_VALUE_LAST);

    jsp_operand_t ret;

    ret._type = jsp_operand_t::TMP;
    ret._data.uid = reg_index;

    return ret;
  } /* make_reg_operand */


  /**
   * Creates a copy of an operand
   *
   * NOTE:
   *   For TMP operands: responsibility for freeing occupied register remains at the original operand.
   *
   * @return copy of an operand
   */
  static jsp_operand_t
  dup_operand (const jsp_operand_t &op) /**< operand to copy */
  {
    jsp_operand_t op_copy;
    op_copy._type = op._type;
    op_copy._data = op._data;
    op_copy._is_to_be_freed = false;

    return op_copy;
  } /* dup_operand */

  /**
   * Is it empty operand?
   *
   * @return true / false
   */
  bool
  is_empty_operand (void) const
  {
    JERRY_ASSERT (_type != jsp_operand_t::UNINITIALIZED);

    return (_type == jsp_operand_t::EMPTY);
  } /* is_empty_operand */

  /**
   * Is it unknown operand?
   *
   * @return true / false
   */
  bool
  is_unknown_operand (void) const
  {
    JERRY_ASSERT (_type != jsp_operand_t::UNINITIALIZED);

    return (_type == jsp_operand_t::UNKNOWN);
  } /* is_unknown_operand */

  /**
   * Is it idx-constant operand?
   *
   * @return true / false
   */
  bool
  is_idx_const_operand (void) const
  {
    JERRY_ASSERT (_type != jsp_operand_t::UNINITIALIZED);

    return (_type == jsp_operand_t::IDX_CONST);
  } /* is_idx_const_operand */

  /**
   * Is it byte-code register operand?
   *
   * @return true / false
   */
  bool
  is_register_operand (void) const
  {
    JERRY_ASSERT (_type != jsp_operand_t::UNINITIALIZED);

    return (_type == jsp_operand_t::TMP);
  } /* is_register_operand */

  /**
   * Is it literal operand?
   *
   * @return true / false
   */
  bool
  is_literal_operand (void) const
  {
    JERRY_ASSERT (_type != jsp_operand_t::UNINITIALIZED);

    return (_type == jsp_operand_t::LITERAL);
  } /* is_literal_operand */

  /**
   * Get idx for operand
   *
   * @return VM_IDX_REWRITE_LITERAL_UID (for jsp_operand_t::LITERAL),
   *         or register index (for jsp_operand_t::TMP).
   */
  vm_idx_t
  get_idx (void) const
  {
    JERRY_ASSERT (_type != jsp_operand_t::UNINITIALIZED);

    if (_type == jsp_operand_t::TMP)
    {
      return _data.uid;
    }
    else if (_type == jsp_operand_t::LITERAL)
    {
      return VM_IDX_REWRITE_LITERAL_UID;
    }
    else
    {
      JERRY_ASSERT (_type == jsp_operand_t::EMPTY);

      return VM_IDX_EMPTY;
    }
  } /* get_idx */

  /**
   * Get literal from operand
   *
   * @return literal identifier (for jsp_operand_t::LITERAL),
   *         or NOT_A_LITERAL (for jsp_operand_t::TMP).
   */
  lit_cpointer_t
  get_literal (void) const
  {
    JERRY_ASSERT (_type != jsp_operand_t::UNINITIALIZED);

    if (_type == jsp_operand_t::TMP)
    {
      return NOT_A_LITERAL;
    }
    else if (_type == jsp_operand_t::LITERAL)
    {
      return _data.lit_id;
    }
    else
    {
      JERRY_ASSERT (_type == jsp_operand_t::EMPTY);

      return NOT_A_LITERAL;
    }
  } /* get_literal */

  /**
   * Get constant from idx-constant operand
   *
   * @return an integer
   */
  vm_idx_t
  get_idx_const (void) const
  {
    JERRY_ASSERT (is_idx_const_operand ());

    return _data.idx_const;
  } /* get_idx_const */

private:
  union
  {
    vm_idx_t idx_const; /**< idx constant value (for jsp_operand_t::IDX_CONST) */
    vm_idx_t uid; /**< byte-code register index (for jsp_operand_t::TMP) */
    lit_cpointer_t lit_id; /**< literal (for jsp_operand_t::LITERAL) */
  } _data;

  type_t _type; /**< type of operand */
  mutable bool _is_to_be_freed; /**< flag, indicating if the register occupied by the operand
                                 * should be freed in destructor (only for TMP operands)*/
};

typedef enum __attr_packed___
{
  VARG_FUNC_DECL,
  VARG_FUNC_EXPR,
  VARG_ARRAY_DECL,
  VARG_OBJ_DECL,
  VARG_CONSTRUCT_EXPR,
  VARG_CALL_EXPR
} varg_list_type;

jsp_operand_t empty_operand (void);
jsp_operand_t literal_operand (lit_cpointer_t);
jsp_operand_t eval_ret_operand (void);
jsp_operand_t jsp_create_operand_for_in_special_reg (void);
bool operand_is_empty (jsp_operand_t &);

void dumper_init (void);
void dumper_free (void);

bool dumper_try_replace_var_with_reg (scopes_tree, op_meta *);

void dumper_new_statement (void);
void dumper_new_scope (void);
void dumper_finish_scope (void);
void dumper_start_varg_code_sequence (void);
void dumper_finish_varg_code_sequence (void);

extern bool dumper_is_eval_literal (const jsp_operand_t &);

jsp_operand_t dump_array_hole_assignment_res (void);
void dump_boolean_assignment (jsp_operand_t, bool);
jsp_operand_t dump_boolean_assignment_res (bool);
void dump_string_assignment (jsp_operand_t, lit_cpointer_t);
jsp_operand_t dump_string_assignment_res (lit_cpointer_t);
void dump_number_assignment (jsp_operand_t, lit_cpointer_t);
jsp_operand_t dump_number_assignment_res (lit_cpointer_t);
void dump_regexp_assignment (jsp_operand_t, lit_cpointer_t);
jsp_operand_t dump_regexp_assignment_res (lit_cpointer_t);
void dump_smallint_assignment (jsp_operand_t, vm_idx_t);
jsp_operand_t dump_smallint_assignment_res (vm_idx_t);
void dump_undefined_assignment (jsp_operand_t);
jsp_operand_t dump_undefined_assignment_res (void);
void dump_null_assignment (jsp_operand_t);
jsp_operand_t dump_null_assignment_res (void);
void dump_variable_assignment (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_variable_assignment_res (jsp_operand_t);

void dump_varg_header_for_rewrite (varg_list_type, jsp_operand_t);
jsp_operand_t rewrite_varg_header_set_args_count (size_t);
void dump_call_additional_info (opcode_call_flags_t, jsp_operand_t);
void dump_varg (jsp_operand_t);

void dump_prop_name_and_value (jsp_operand_t, jsp_operand_t);
void dump_prop_getter_decl (jsp_operand_t, jsp_operand_t);
void dump_prop_setter_decl (jsp_operand_t, jsp_operand_t);
void dump_prop_getter (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_prop_getter_res (jsp_operand_t, jsp_operand_t);
void dump_prop_setter (jsp_operand_t, jsp_operand_t, jsp_operand_t);

void dump_function_end_for_rewrite (void);
void rewrite_function_end ();

void dump_this (jsp_operand_t);
jsp_operand_t dump_this_res (void);

void dump_post_increment (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_post_increment_res (jsp_operand_t);
void dump_post_decrement (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_post_decrement_res (jsp_operand_t);
void dump_pre_increment (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_pre_increment_res (jsp_operand_t);
void dump_pre_decrement (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_pre_decrement_res (jsp_operand_t);
void dump_unary_plus (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_unary_plus_res (jsp_operand_t);
void dump_unary_minus (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_unary_minus_res (jsp_operand_t);
void dump_bitwise_not (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_bitwise_not_res (jsp_operand_t);
void dump_logical_not (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_logical_not_res (jsp_operand_t);

void dump_multiplication (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_multiplication_res (jsp_operand_t, jsp_operand_t);
void dump_division (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_division_res (jsp_operand_t, jsp_operand_t);
void dump_remainder (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_remainder_res (jsp_operand_t, jsp_operand_t);
void dump_addition (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_addition_res (jsp_operand_t, jsp_operand_t);
void dump_substraction (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_substraction_res (jsp_operand_t, jsp_operand_t);
void dump_left_shift (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_left_shift_res (jsp_operand_t, jsp_operand_t);
void dump_right_shift (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_right_shift_res (jsp_operand_t, jsp_operand_t);
void dump_right_shift_ex (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_right_shift_ex_res (jsp_operand_t, jsp_operand_t);
void dump_less_than (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_less_than_res (jsp_operand_t, jsp_operand_t);
void dump_greater_than (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_greater_than_res (jsp_operand_t, jsp_operand_t);
void dump_less_or_equal_than (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_less_or_equal_than_res (jsp_operand_t, jsp_operand_t);
void dump_greater_or_equal_than (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_greater_or_equal_than_res (jsp_operand_t, jsp_operand_t);
void dump_instanceof (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_instanceof_res (jsp_operand_t, jsp_operand_t);
void dump_in (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_in_res (jsp_operand_t, jsp_operand_t);
void dump_equal_value (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_equal_value_res (jsp_operand_t, jsp_operand_t);
void dump_not_equal_value (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_not_equal_value_res (jsp_operand_t, jsp_operand_t);
void dump_equal_value_type (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_equal_value_type_res (jsp_operand_t, jsp_operand_t);
void dump_not_equal_value_type (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_not_equal_value_type_res (jsp_operand_t, jsp_operand_t);
void dump_bitwise_and (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_bitwise_and_res (jsp_operand_t, jsp_operand_t);
void dump_bitwise_xor (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_bitwise_xor_res (jsp_operand_t, jsp_operand_t);
void dump_bitwise_or (jsp_operand_t, jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_bitwise_or_res (jsp_operand_t, jsp_operand_t);

void start_dumping_logical_and_checks (void);
void dump_logical_and_check_for_rewrite (jsp_operand_t);
void rewrite_logical_and_checks (void);
void start_dumping_logical_or_checks (void);
void dump_logical_or_check_for_rewrite (jsp_operand_t);
void rewrite_logical_or_checks (void);
void dump_conditional_check_for_rewrite (jsp_operand_t);
void rewrite_conditional_check (void);
void dump_jump_to_end_for_rewrite (void);
void rewrite_jump_to_end (void);

void start_dumping_assignment_expression (const jsp_operand_t &);
jsp_operand_t dump_prop_setter_or_variable_assignment_res (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_prop_setter_or_addition_res (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_prop_setter_or_multiplication_res (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_prop_setter_or_division_res (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_prop_setter_or_remainder_res (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_prop_setter_or_substraction_res (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_prop_setter_or_left_shift_res (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_prop_setter_or_right_shift_res (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_prop_setter_or_right_shift_ex_res (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_prop_setter_or_bitwise_and_res (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_prop_setter_or_bitwise_xor_res (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_prop_setter_or_bitwise_or_res (jsp_operand_t, jsp_operand_t);

void dumper_set_break_target (void);
void dumper_set_continue_target (void);
void dumper_set_next_interation_target (void);
vm_instr_counter_t
dump_simple_or_nested_jump_for_rewrite (bool, vm_instr_counter_t);
vm_instr_counter_t
rewrite_simple_or_nested_jump_and_get_next (vm_instr_counter_t, vm_instr_counter_t);
void dump_continue_iterations_check (jsp_operand_t);

void start_dumping_case_clauses (void);
void dump_case_clause_check_for_rewrite (jsp_operand_t, jsp_operand_t);
void dump_default_clause_check_for_rewrite (void);
void rewrite_case_clause (void);
void rewrite_default_clause (void);
void finish_dumping_case_clauses (void);

jsp_operand_t dump_delete_res (jsp_operand_t, bool, locus);

void dump_typeof (jsp_operand_t, jsp_operand_t);
jsp_operand_t dump_typeof_res (jsp_operand_t);

vm_instr_counter_t dump_with_for_rewrite (jsp_operand_t);
void rewrite_with (vm_instr_counter_t);
void dump_with_end (void);

vm_instr_counter_t dump_for_in_for_rewrite (jsp_operand_t);
void rewrite_for_in (vm_instr_counter_t);
void dump_for_in_end (void);

void dump_try_for_rewrite (void);
void rewrite_try (void);
void dump_catch_for_rewrite (jsp_operand_t);
void rewrite_catch (void);
void dump_finally_for_rewrite (void);
void rewrite_finally (void);
void dump_end_try_catch_finally (void);
void dump_throw (jsp_operand_t);

bool dumper_variable_declaration_exists (lit_cpointer_t);
void dump_variable_declaration (lit_cpointer_t);

vm_instr_counter_t dump_scope_code_flags_for_rewrite (void);
void rewrite_scope_code_flags (vm_instr_counter_t, opcode_scope_code_flags_t);

void dump_reg_var_decl_for_rewrite (void);
void rewrite_reg_var_decl (void);

void dump_ret (void);
void dump_retval (jsp_operand_t);

#endif /* OPCODES_DUMPER_H */

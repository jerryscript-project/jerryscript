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
 * Operand (descriptor of value or reference in context of parser)
 */
class jsp_operand_t
{
public:
  enum type_t : uint8_t
  {
    EMPTY, /**< empty operand */
    STRING_LITERAL, /**< operand contains string literal value */
    NUMBER_LITERAL, /**< operand contains number literal value */
    REGEXP_LITERAL, /**< operand contains regexp literal value */
    SIMPLE_VALUE, /**< operand contains a simple ecma value */
    SMALLINT, /**< operand contains small integer value (less than 256) */
    IDENTIFIER, /**< Identifier reference */
    THIS_BINDING, /**< ThisBinding operand */
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
#ifndef JERRY_NDEBUG
    _type = jsp_operand_t::UNINITIALIZED;
#endif /* !JERRY_NDEBUG */
  } /* jsp_operand_t */

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
   * Construct ThisBinding operand
   *
   * @return constructed operand
   */
  static jsp_operand_t
  make_this_operand (void)
  {
    jsp_operand_t ret;

    ret._type = jsp_operand_t::THIS_BINDING;

    return ret;
  } /* make_this_operand */

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
   * Construct small integer operand
   *
   * @return constructed operand
   */
  static jsp_operand_t
  make_smallint_operand (uint8_t integer_value) /**< small integer value */
  {
    jsp_operand_t ret;

    ret._type = jsp_operand_t::SMALLINT;
    ret._data.smallint_value = integer_value;

    return ret;
  } /* make_smallint_operand */

  /**
   * Construct simple ecma value operand
   *
   * @return constructed operand
   */
  static jsp_operand_t
  make_simple_value_operand (ecma_simple_value_t simple_value) /**< simple ecma value */
  {
    jsp_operand_t ret;

    ret._type = jsp_operand_t::SIMPLE_VALUE;
    ret._data.simple_value = simple_value;

    return ret;
  } /* make_simple_value_operand */

  /**
   * Construct string literal operand
   *
   * @return constructed operand
   */
  static jsp_operand_t
  make_string_lit_operand (lit_cpointer_t lit_id) /**< literal identifier */
  {
    JERRY_ASSERT (lit_id.packed_value != NOT_A_LITERAL.packed_value);

#ifndef JERRY_NDEBUG
    lit_literal_t lit = lit_get_literal_by_cp (lit_id);

    JERRY_ASSERT (RCS_RECORD_IS_CHARSET (lit)
                  || RCS_RECORD_IS_MAGIC_STR (lit)
                  || RCS_RECORD_IS_MAGIC_STR_EX (lit));
#endif /* !JERRY_NDEBUG */

    jsp_operand_t ret;

    ret._type = jsp_operand_t::STRING_LITERAL;
    ret._data.lit_id = lit_id;

    return ret;
  } /* make_string_lit_operand */

  /**
   * Construct RegExp literal operand
   *
   * @return constructed operand
   */
  static jsp_operand_t
  make_regexp_lit_operand (lit_cpointer_t lit_id) /**< literal identifier */
  {
    JERRY_ASSERT (lit_id.packed_value != NOT_A_LITERAL.packed_value);

#ifndef JERRY_NDEBUG
    lit_literal_t lit = lit_get_literal_by_cp (lit_id);

    JERRY_ASSERT (RCS_RECORD_IS_CHARSET (lit)
                  || RCS_RECORD_IS_MAGIC_STR (lit)
                  || RCS_RECORD_IS_MAGIC_STR_EX (lit));
#endif /* !JERRY_NDEBUG */

    jsp_operand_t ret;

    ret._type = jsp_operand_t::REGEXP_LITERAL;
    ret._data.lit_id = lit_id;

    return ret;
  } /* make_regexp_lit_operand */

  /**
   * Construct number literal operand
   *
   * @return constructed operand
   */
  static jsp_operand_t
  make_number_lit_operand (lit_cpointer_t lit_id) /**< literal identifier */
  {
    JERRY_ASSERT (lit_id.packed_value != NOT_A_LITERAL.packed_value);

#ifndef JERRY_NDEBUG
    lit_literal_t lit = lit_get_literal_by_cp (lit_id);

    JERRY_ASSERT (RCS_RECORD_IS_NUMBER (lit));
#endif /* !JERRY_NDEBUG */

    jsp_operand_t ret;

    ret._type = jsp_operand_t::NUMBER_LITERAL;
    ret._data.lit_id = lit_id;

    return ret;
  } /* make_number_lit_operand */

  /**
   * Construct identifier reference operand
   *
   * @return constructed operand
   */
  static jsp_operand_t
  make_identifier_operand (lit_cpointer_t lit_id) /**< literal identifier */
  {
    JERRY_ASSERT (lit_id.packed_value != NOT_A_LITERAL.packed_value);

    jsp_operand_t ret;

    ret._type = jsp_operand_t::IDENTIFIER;
    ret._data.identifier = lit_id;

    return ret;
  } /* make_identifier_operand */

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
   * Is it ThisBinding operand?
   *
   * @return true / false
   */
  bool
  is_this_operand (void) const
  {
    JERRY_ASSERT (_type != jsp_operand_t::UNINITIALIZED);

    return (_type == jsp_operand_t::THIS_BINDING);
  } /* is_this_operand */

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
   * Is it simple ecma value operand?
   *
   * @return true / false
   */
  bool
  is_simple_value_operand (void) const
  {
    JERRY_ASSERT (_type != jsp_operand_t::UNINITIALIZED);

    return (_type == jsp_operand_t::SIMPLE_VALUE);
  } /* is_simple_value_operand */

  /**
   * Is it small integer operand?
   *
   * @return true / false
   */
  bool
  is_smallint_operand (void) const
  {
    JERRY_ASSERT (_type != jsp_operand_t::UNINITIALIZED);

    return (_type == jsp_operand_t::SMALLINT);
  } /* is_smallint_operand */

  /**
   * Is it number literal operand?
   *
   * @return true / false
   */
  bool
  is_number_lit_operand (void) const
  {
    JERRY_ASSERT (_type != jsp_operand_t::UNINITIALIZED);

    return (_type == jsp_operand_t::NUMBER_LITERAL);
  } /* is_number_lit_operand */

  /**
   * Is it string literal operand?
   *
   * @return true / false
   */
  bool
  is_string_lit_operand (void) const
  {
    JERRY_ASSERT (_type != jsp_operand_t::UNINITIALIZED);

    return (_type == jsp_operand_t::STRING_LITERAL);
  } /* is_string_lit_operand */

  /**
   * Is it RegExp literal operand?
   *
   * @return true / false
   */
  bool
  is_regexp_lit_operand (void) const
  {
    JERRY_ASSERT (_type != jsp_operand_t::UNINITIALIZED);

    return (_type == jsp_operand_t::REGEXP_LITERAL);
  } /* is_regexp_lit_operand */

  /**
   * Is it identifier reference operand?
   *
   * @return true / false
   */
  bool is_identifier_operand (void) const
  {
    JERRY_ASSERT (_type != jsp_operand_t::UNINITIALIZED);

    return (_type == jsp_operand_t::IDENTIFIER);
  } /* is_identifier_operand */

  /**
   * Get string literal - name of Identifier reference
   *
   * @return literal identifier
   */
  lit_cpointer_t get_identifier_name (void) const
  {
    JERRY_ASSERT (is_identifier_operand ());

    return (_data.identifier);
  } /* get_identifier_name */

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
    else if (_type == jsp_operand_t::STRING_LITERAL
             || _type == jsp_operand_t::NUMBER_LITERAL)
    {
      return VM_IDX_REWRITE_LITERAL_UID;
    }
    else if (_type == jsp_operand_t::THIS_BINDING)
    {
      return VM_REG_SPECIAL_THIS_BINDING;
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
    else if (_type == jsp_operand_t::STRING_LITERAL
             || _type == jsp_operand_t::NUMBER_LITERAL
             || _type == jsp_operand_t::REGEXP_LITERAL)
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

  /**
   * Get small integer constant from operand
   *
   * @return an integer
   */
  uint8_t
  get_smallint_value (void) const
  {
    JERRY_ASSERT (is_smallint_operand ());

    return _data.smallint_value;
  } /* get_smallint_value */

  /**
   * Get simple value from operand
   *
   * @return a simple ecma value
   */
  ecma_simple_value_t
  get_simple_value (void) const
  {
    JERRY_ASSERT (is_simple_value_operand ());

    return (ecma_simple_value_t) _data.simple_value;
  } /* get_simple_value */
private:
  union
  {
    vm_idx_t idx_const; /**< idx constant value (for jsp_operand_t::IDX_CONST) */
    vm_idx_t uid; /**< register index (for jsp_operand_t::TMP) */
    lit_cpointer_t lit_id; /**< literal (for jsp_operand_t::LITERAL) */
    lit_cpointer_t identifier; /**< Identifier reference (is_value_based_ref flag not set) */
    uint8_t smallint_value; /**< small integer value */
    uint8_t simple_value; /**< simple ecma value */
  } _data;

  type_t _type; /**< type of operand */
};

static_assert (sizeof (jsp_operand_t) == 4, "");

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
jsp_operand_t tmp_operand (void);
bool operand_is_empty (jsp_operand_t);

void dumper_init (jsp_ctx_t *, bool);

vm_instr_counter_t dumper_get_current_instr_counter (jsp_ctx_t *);

void dumper_start_move_of_vars_to_regs (jsp_ctx_t *);
bool dumper_start_move_of_args_to_regs (jsp_ctx_t *, uint32_t args_num);
bool dumper_try_replace_identifier_name_with_reg (jsp_ctx_t *, bytecode_data_header_t *, op_meta *);
void dumper_alloc_reg_for_unused_arg (jsp_ctx_t *);

void dumper_new_statement (jsp_ctx_t *);
void dumper_save_reg_alloc_ctx (jsp_ctx_t *, vm_idx_t *, vm_idx_t *);
void dumper_restore_reg_alloc_ctx (jsp_ctx_t *, vm_idx_t, vm_idx_t, bool);
vm_idx_t dumper_save_reg_alloc_counter (jsp_ctx_t *);
void dumper_restore_reg_alloc_counter (jsp_ctx_t *, vm_idx_t);

extern bool dumper_is_eval_literal (jsp_operand_t);

void dump_variable_assignment (jsp_ctx_t *, jsp_operand_t, jsp_operand_t);

vm_instr_counter_t dump_varg_header_for_rewrite (jsp_ctx_t *, varg_list_type, jsp_operand_t, jsp_operand_t);
void rewrite_varg_header_set_args_count (jsp_ctx_t *, size_t, vm_instr_counter_t);
void dump_call_additional_info (jsp_ctx_t *, opcode_call_flags_t, jsp_operand_t);
void dump_varg (jsp_ctx_t *, jsp_operand_t);

void dump_prop_name_and_value (jsp_ctx_t *, jsp_operand_t, jsp_operand_t);
void dump_prop_getter_decl (jsp_ctx_t *, jsp_operand_t, jsp_operand_t);
void dump_prop_setter_decl (jsp_ctx_t *, jsp_operand_t, jsp_operand_t);
void dump_prop_getter (jsp_ctx_t *, jsp_operand_t, jsp_operand_t, jsp_operand_t);
void dump_prop_setter (jsp_ctx_t *, jsp_operand_t, jsp_operand_t, jsp_operand_t);

vm_instr_counter_t dump_conditional_check_for_rewrite (jsp_ctx_t *, jsp_operand_t);
void rewrite_conditional_check (jsp_ctx_t *, vm_instr_counter_t);
vm_instr_counter_t dump_jump_to_end_for_rewrite (jsp_ctx_t *);
void rewrite_jump_to_end (jsp_ctx_t *, vm_instr_counter_t);

vm_instr_counter_t dumper_set_next_iteration_target (jsp_ctx_t *);
vm_instr_counter_t dump_simple_or_nested_jump_for_rewrite (jsp_ctx_t *,
                                                           bool,
                                                           bool,
                                                           bool,
                                                           jsp_operand_t,
                                                           vm_instr_counter_t);
vm_instr_counter_t rewrite_simple_or_nested_jump_and_get_next (jsp_ctx_t *, vm_instr_counter_t, vm_instr_counter_t);
void dump_continue_iterations_check (jsp_ctx_t *, vm_instr_counter_t, jsp_operand_t);

void dump_delete (jsp_ctx_t *, jsp_operand_t, jsp_operand_t);
void dump_delete_prop (jsp_ctx_t *, jsp_operand_t, jsp_operand_t, jsp_operand_t);

void dump_typeof (jsp_ctx_t *, jsp_operand_t, jsp_operand_t);

void dump_unary_op (jsp_ctx_t *, vm_op_t, jsp_operand_t, jsp_operand_t);
void dump_binary_op (jsp_ctx_t *, vm_op_t, jsp_operand_t, jsp_operand_t, jsp_operand_t);

vm_instr_counter_t dump_with_for_rewrite (jsp_ctx_t *, jsp_operand_t);
void rewrite_with (jsp_ctx_t *, vm_instr_counter_t);
void dump_with_end (jsp_ctx_t *);

vm_instr_counter_t dump_for_in_for_rewrite (jsp_ctx_t *, jsp_operand_t);
void rewrite_for_in (jsp_ctx_t *, vm_instr_counter_t);
void dump_for_in_end (jsp_ctx_t *);

vm_instr_counter_t dump_try_for_rewrite (jsp_ctx_t *);
vm_instr_counter_t dump_catch_for_rewrite (jsp_ctx_t *, jsp_operand_t);
vm_instr_counter_t dump_finally_for_rewrite (jsp_ctx_t *);
void rewrite_try (jsp_ctx_t *, vm_instr_counter_t);
void rewrite_catch (jsp_ctx_t *, vm_instr_counter_t);
void rewrite_finally (jsp_ctx_t *, vm_instr_counter_t);
void dump_end_try_catch_finally (jsp_ctx_t *);
void dump_throw (jsp_ctx_t *, jsp_operand_t);

void dump_variable_declaration (jsp_ctx_t *, lit_cpointer_t);

vm_instr_counter_t dump_reg_var_decl_for_rewrite (jsp_ctx_t *);
void rewrite_reg_var_decl (jsp_ctx_t *, vm_instr_counter_t);

void dump_ret (jsp_ctx_t *);
void dump_retval (jsp_ctx_t *, jsp_operand_t);

op_meta dumper_get_op_meta (jsp_ctx_t *, vm_instr_counter_t);
void dumper_rewrite_op_meta (jsp_ctx_t *, vm_instr_counter_t, op_meta);

#endif /* OPCODES_DUMPER_H */

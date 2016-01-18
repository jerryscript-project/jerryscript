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

#include "jsp-early-error.h"
#include "opcodes-dumper.h"
#include "pretty-printer.h"
#include "bytecode-data.h"

/**
 * Register allocator's counter
 */
static vm_idx_t jsp_reg_next;

/**
 * Maximum identifier of a register, allocated for intermediate value storage
 *
 * See also:
 *          dumper_save_reg_alloc_ctx, dumper_restore_reg_alloc_ctx
 */
static vm_idx_t jsp_reg_max_for_temps;

/**
 * Maximum identifier of a register, allocated for storage of a variable value.
 *
 * The value can be VM_IDX_EMPTY, indicating that no registers were allocated for variable values.
 *
 * Note:
 *      Registers for variable values are always allocated after registers for temporary values,
 *      so the value, if not equal to VM_IDX_EMPTY, is always greater than jsp_reg_max_for_temps.
 *
 * See also:
 *          dumper_try_replace_identifier_name_with_reg
 */
static vm_idx_t jsp_reg_max_for_local_var;

/**
 * Maximum identifier of a register, allocated for storage of an argument value.
 *
 * The value can be VM_IDX_EMPTY, indicating that no registers were allocated for argument values.
 *
 * Note:
 *      Registers for argument values are always allocated after registers for variable values,
 *      so the value, if not equal to VM_IDX_EMPTY, is always greater than jsp_reg_max_for_local_var.
 *
 * See also:
 *          dumper_try_replace_identifier_name_with_reg
 */
static vm_idx_t jsp_reg_max_for_args;

/**
 * Flag, indicating if instructions should be printed
 */
bool is_print_instrs = false;

/**
 * Construct uninitialized operand
 *
 * @return constructed operand
 */
jsp_operand_t
jsp_make_uninitialized_operand (void)
{
  jsp_operand_t ret;
  ret.type = JSP_OPERAND_TYPE_UNINITIALIZED;

  return ret;
} /* jsp_make_uninitialized_operand */

/**
 * Construct empty operand
 *
 * @return constructed operand
 */
jsp_operand_t
jsp_make_empty_operand (void)
{
  jsp_operand_t ret;
  ret.type = JSP_OPERAND_TYPE_EMPTY;

  return ret;
} /* jsp_make_empty_operand */

/**
 * Construct ThisBinding operand
 *
 * @return constructed operand
 */
jsp_operand_t
jsp_make_this_operand (void)
{
  jsp_operand_t ret;
  ret.type = JSP_OPERAND_TYPE_THIS_BINDING;

  return ret;
} /* jsp_make_this_operand */

/**
 * Construct unknown operand
 *
 * @return constructed operand
 */
jsp_operand_t
jsp_make_unknown_operand (void)
{
  jsp_operand_t ret;
  ret.type = JSP_OPERAND_TYPE_UNKNOWN;

  return ret;
} /* jsp_make_unknown_operand */

/**
 * Construct idx-constant operand
 *
 * @return constructed operand
 */
jsp_operand_t
jsp_make_idx_const_operand (vm_idx_t cnst) /**< integer in vm_idx_t range */
{
  jsp_operand_t ret;

  ret.type = JSP_OPERAND_TYPE_IDX_CONST;
  ret.data.idx_const = cnst;

  return ret;
} /* jsp_make_idx_const_operand */

/**
 * Construct small integer operand
 *
 * @return constructed operand
 */
jsp_operand_t
jsp_make_smallint_operand (uint8_t integer_value) /**< small integer value */
{
  jsp_operand_t ret;

  ret.type = JSP_OPERAND_TYPE_SMALLINT;
  ret.data.smallint_value = integer_value;

  return ret;
} /* jsp_make_smallint_operand */

/**
 * Construct simple ecma value operand
 *
 * @return constructed operand
 */
jsp_operand_t
jsp_make_simple_value_operand (ecma_simple_value_t simple_value) /**< simple ecma value */
{
  jsp_operand_t ret;

  ret.type = JSP_OPERAND_TYPE_SIMPLE_VALUE;
  ret.data.simple_value = simple_value;

  return ret;
} /* jsp_make_simple_value_operand */

/**
 * Construct string literal operand
 *
 * @return constructed operand
 */
jsp_operand_t
jsp_make_string_lit_operand (lit_cpointer_t lit_id) /**< literal identifier */
{
  JERRY_ASSERT (lit_id.packed_value != NOT_A_LITERAL.packed_value);

#ifndef JERRY_NDEBUG
  lit_literal_t lit = lit_get_literal_by_cp (lit_id);

  JERRY_ASSERT (RCS_RECORD_IS_CHARSET (lit)
                || RCS_RECORD_IS_MAGIC_STR (lit)
                || RCS_RECORD_IS_MAGIC_STR_EX (lit));
#endif /* !JERRY_NDEBUG */

  jsp_operand_t ret;

  ret.type = JSP_OPERAND_TYPE_STRING_LITERAL;
  ret.data.lit_id = lit_id;

  return ret;
} /* jsp_make_string_lit_operand */

/**
 * Construct RegExp literal operand
 *
 * @return constructed operand
 */
jsp_operand_t
jsp_make_regexp_lit_operand (lit_cpointer_t lit_id) /**< literal identifier */
{
  JERRY_ASSERT (lit_id.packed_value != NOT_A_LITERAL.packed_value);

#ifndef JERRY_NDEBUG
  lit_literal_t lit = lit_get_literal_by_cp (lit_id);

  JERRY_ASSERT (RCS_RECORD_IS_CHARSET (lit)
                || RCS_RECORD_IS_MAGIC_STR (lit)
                || RCS_RECORD_IS_MAGIC_STR_EX (lit));
#endif /* !JERRY_NDEBUG */

  jsp_operand_t ret;

  ret.type = JSP_OPERAND_TYPE_REGEXP_LITERAL;
  ret.data.lit_id = lit_id;

  return ret;
} /* jsp_make_regexp_lit_operand */

/**
 * Construct number literal operand
 *
 * @return constructed operand
 */
jsp_operand_t
jsp_make_number_lit_operand (lit_cpointer_t lit_id) /**< literal identifier */
{
  JERRY_ASSERT (lit_id.packed_value != NOT_A_LITERAL.packed_value);

#ifndef JERRY_NDEBUG
  lit_literal_t lit = lit_get_literal_by_cp (lit_id);

  JERRY_ASSERT (RCS_RECORD_IS_NUMBER (lit));
#endif /* !JERRY_NDEBUG */

  jsp_operand_t ret;

  ret.type = JSP_OPERAND_TYPE_NUMBER_LITERAL;
  ret.data.lit_id = lit_id;

  return ret;
} /* jsp_make_number_lit_operand */

/**
 * Construct identifier reference operand
 *
 * @return constructed operand
 */
jsp_operand_t
jsp_make_identifier_operand (lit_cpointer_t lit_id) /**< literal identifier */
{
  JERRY_ASSERT (lit_id.packed_value != NOT_A_LITERAL.packed_value);

  jsp_operand_t ret;

  ret.type = JSP_OPERAND_TYPE_IDENTIFIER;
  ret.data.identifier = lit_id;

  return ret;
} /* jsp_make_identifier_operand */

/**
 * Construct register operand
 *
 * @return constructed operand
 */
jsp_operand_t
jsp_make_reg_operand (vm_idx_t reg_index) /**< register index */
{
  /*
   * The following check currently leads to 'comparison is always true
   * due to limited range of data type' warning, so it is turned off.
   *
   * If VM_IDX_GENERAL_VALUE_FIRST is changed to value greater than 0,
   * the check should be restored.
   */
  // JERRY_ASSERT (reg_index >= VM_IDX_GENERAL_VALUE_FIRST);
  JERRY_STATIC_ASSERT (VM_IDX_GENERAL_VALUE_FIRST == 0);

  JERRY_ASSERT (reg_index <= VM_IDX_GENERAL_VALUE_LAST);

  jsp_operand_t ret;

  ret.type = JSP_OPERAND_TYPE_TMP;
  ret.data.uid = reg_index;

  return ret;
} /* jsp_make_reg_operand */

/**
 * Is it empty operand?
 *
 * @return true / false
 */
bool
jsp_is_empty_operand (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (operand.type != JSP_OPERAND_TYPE_UNINITIALIZED);

  return (operand.type == JSP_OPERAND_TYPE_EMPTY);
} /* jsp_is_empty_operand */

/**
 * Is it ThisBinding operand?
 *
 * @return true / false
 */
bool
jsp_is_this_operand (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (operand.type != JSP_OPERAND_TYPE_UNINITIALIZED);

  return (operand.type == JSP_OPERAND_TYPE_THIS_BINDING);
} /* jsp_is_this_operand */

/**
 * Is it unknown operand?
 *
 * @return true / false
 */
bool
jsp_is_unknown_operand (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (operand.type != JSP_OPERAND_TYPE_UNINITIALIZED);

  return (operand.type == JSP_OPERAND_TYPE_UNKNOWN);
} /* jsp_is_unknown_operand */

/**
 * Is it idx-constant operand?
 *
 * @return true / false
 */
bool
jsp_is_idx_const_operand (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (operand.type != JSP_OPERAND_TYPE_UNINITIALIZED);

  return (operand.type == JSP_OPERAND_TYPE_IDX_CONST);
} /* jsp_is_idx_const_operand */

/**
 * Is it byte-code register operand?
 *
 * @return true / false
 */
bool
jsp_is_register_operand (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (operand.type != JSP_OPERAND_TYPE_UNINITIALIZED);

  return (operand.type == JSP_OPERAND_TYPE_TMP);
} /* jsp_is_register_operand */

/**
 * Is it simple ecma value operand?
 *
 * @return true / false
 */
bool
jsp_is_simple_value_operand (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (operand.type != JSP_OPERAND_TYPE_UNINITIALIZED);

  return (operand.type == JSP_OPERAND_TYPE_SIMPLE_VALUE);
} /* jsp_is_simple_value_operand */

/**
 * Is it small integer operand?
 *
 * @return true / false
 */
bool
jsp_is_smallint_operand (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (operand.type != JSP_OPERAND_TYPE_UNINITIALIZED);

  return (operand.type == JSP_OPERAND_TYPE_SMALLINT);
} /* jsp_is_smallint_operand */

/**
 * Is it number literal operand?
 *
 * @return true / false
 */
bool
jsp_is_number_lit_operand (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (operand.type != JSP_OPERAND_TYPE_UNINITIALIZED);

  return (operand.type == JSP_OPERAND_TYPE_NUMBER_LITERAL);
} /* jsp_is_number_lit_operand */

/**
 * Is it string literal operand?
 *
 * @return true / false
 */
bool
jsp_is_string_lit_operand (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (operand.type != JSP_OPERAND_TYPE_UNINITIALIZED);

  return (operand.type == JSP_OPERAND_TYPE_STRING_LITERAL);
} /* jsp_is_string_lit_operand */

/**
 * Is it RegExp literal operand?
 *
 * @return true / false
 */
bool
jsp_is_regexp_lit_operand (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (operand.type != JSP_OPERAND_TYPE_UNINITIALIZED);

  return (operand.type == JSP_OPERAND_TYPE_REGEXP_LITERAL);
} /* jsp_is_regexp_lit_operand */

/**
 * Is it identifier reference operand?
 *
 * @return true / false
 */
bool
jsp_is_identifier_operand (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (operand.type != JSP_OPERAND_TYPE_UNINITIALIZED);

  return (operand.type == JSP_OPERAND_TYPE_IDENTIFIER);
} /* jsp_is_identifier_operand */

/**
 * Get string literal - name of Identifier reference
 *
 * @return literal identifier
 */
lit_cpointer_t
jsp_operand_get_identifier_name (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (jsp_is_identifier_operand (operand));

  return (operand.data.identifier);
} /* jsp_operand_get_identifier_name */

/**
 * Get idx for operand
 *
 * @return VM_IDX_REWRITE_LITERAL_UID (for LITERAL),
 *         or register index (for TMP).
 */
vm_idx_t
jsp_operand_get_idx (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (operand.type != JSP_OPERAND_TYPE_UNINITIALIZED);

  if (operand.type == JSP_OPERAND_TYPE_TMP)
  {
    return operand.data.uid;
  }

  if (operand.type == JSP_OPERAND_TYPE_STRING_LITERAL || operand.type == JSP_OPERAND_TYPE_NUMBER_LITERAL)
  {
    return VM_IDX_REWRITE_LITERAL_UID;
  }

  if (operand.type == JSP_OPERAND_TYPE_THIS_BINDING)
  {
    return VM_REG_SPECIAL_THIS_BINDING;
  }

  JERRY_ASSERT (operand.type == JSP_OPERAND_TYPE_EMPTY);
  return VM_IDX_EMPTY;
} /* jsp_operand_get_idx */

/**
 * Get literal from operand
 *
 * @return literal identifier (for LITERAL),
 *         or NOT_A_LITERAL (for TMP).
 */
lit_cpointer_t
jsp_operand_get_literal (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (operand.type != JSP_OPERAND_TYPE_UNINITIALIZED);

  if (operand.type == JSP_OPERAND_TYPE_TMP)
  {
    return NOT_A_LITERAL;
  }

  if (operand.type == JSP_OPERAND_TYPE_STRING_LITERAL
      || operand.type == JSP_OPERAND_TYPE_NUMBER_LITERAL
      || operand.type == JSP_OPERAND_TYPE_REGEXP_LITERAL)
  {
    return operand.data.lit_id;
  }

  JERRY_ASSERT (operand.type == JSP_OPERAND_TYPE_EMPTY);
  return NOT_A_LITERAL;
} /* jsp_operand_get_literal */

/**
 * Get constant from idx-constant operand
 *
 * @return an integer
 */
vm_idx_t
jsp_operand_get_idx_const (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (jsp_is_idx_const_operand (operand));

  return operand.data.idx_const;
} /* jsp_operand_get_idx_const */

/**
 * Get small integer constant from operand
 *
 * @return an integer
 */
uint8_t
jsp_operand_get_smallint_value (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (jsp_is_smallint_operand (operand));

  return operand.data.smallint_value;
} /* jsp_operand_get_smallint_value */

/**
 * Get simple value from operand
 *
 * @return a simple ecma value
 */
ecma_simple_value_t
jsp_operand_get_simple_value (jsp_operand_t operand) /**< operand */
{
  JERRY_ASSERT (jsp_is_simple_value_operand (operand));

  return (ecma_simple_value_t) operand.data.simple_value;
} /* jsp_operand_get_simple_value */

/**
 * Determine if operand with specified index could be encoded as a literal
 *
 * @return true, if operand could be literal
 *         false, otherwise
 */
static bool
is_possible_literal (uint16_t mask, /**< mask encoding which operands could be literals */
                     uint8_t index) /**< index of operand */
{
  int res;
  switch (index)
  {
    case 0:
    {
      res = mask >> 8;
      break;
    }
    case 1:
    {
      res = (mask & 0xF0) >> 4;
      break;
    }
    default:
    {
      JERRY_ASSERT (index = 2);
      res = mask & 0x0F;
    }
  }
  JERRY_ASSERT (res == 0 || res == 1);
  return res == 1;
} /* is_possible_literal */

/**
 * Get specified operand value from instruction
 *
 * @return operand value
 */
static vm_idx_t
get_uid (op_meta *op, /**< intermediate instruction description, from which operand should be extracted */
         size_t i) /**< index of the operand */
{
  JERRY_ASSERT (i < 3);
  return op->op.data.raw_args[i];
} /* get_uid */

/**
 * Insert literals from instruction to array of seen literals
 * This is needed for prediction of bytecode's hash table size
 */
static void
insert_uids_to_lit_id_map (jsp_ctx_t *ctx_p, /**< parser context */
                           op_meta *om, /**< intermediate instruction description */
                           uint16_t mask) /**< mask of possibe literal operands */
{
  for (uint8_t i = 0; i < 3; i++)
  {
    if (is_possible_literal (mask, i))
    {
      if (get_uid (om, i) == VM_IDX_REWRITE_LITERAL_UID)
      {
        JERRY_ASSERT (om->lit_id[i].packed_value != MEM_CP_NULL);

        jsp_account_next_bytecode_to_literal_reference (ctx_p, om->lit_id[i]);
      }
      else
      {
        JERRY_ASSERT (om->lit_id[i].packed_value == MEM_CP_NULL);
      }
    }
    else
    {
      JERRY_ASSERT (om->lit_id[i].packed_value == MEM_CP_NULL);
    }
  }
}

/**
 * Count number of literals in instruction which were not seen previously
 */
static void
count_new_literals_in_instr (jsp_ctx_t *ctx_p, /**< parser context */
                             vm_instr_counter_t instr_pos, /**< position of instruction */
                             op_meta *om_p) /**< instruction */
{
  if (instr_pos % BLOCK_SIZE == 0)
  {
    jsp_empty_tmp_literal_set (ctx_p);
  }

  switch (om_p->op.op_idx)
  {
    case VM_OP_PROP_GETTER:
    case VM_OP_PROP_SETTER:
    case VM_OP_DELETE_PROP:
    case VM_OP_B_SHIFT_LEFT:
    case VM_OP_B_SHIFT_RIGHT:
    case VM_OP_B_SHIFT_URIGHT:
    case VM_OP_B_AND:
    case VM_OP_B_OR:
    case VM_OP_B_XOR:
    case VM_OP_EQUAL_VALUE:
    case VM_OP_NOT_EQUAL_VALUE:
    case VM_OP_EQUAL_VALUE_TYPE:
    case VM_OP_NOT_EQUAL_VALUE_TYPE:
    case VM_OP_LESS_THAN:
    case VM_OP_GREATER_THAN:
    case VM_OP_LESS_OR_EQUAL_THAN:
    case VM_OP_GREATER_OR_EQUAL_THAN:
    case VM_OP_INSTANCEOF:
    case VM_OP_IN:
    case VM_OP_ADDITION:
    case VM_OP_SUBSTRACTION:
    case VM_OP_DIVISION:
    case VM_OP_MULTIPLICATION:
    case VM_OP_REMAINDER:
    {
      insert_uids_to_lit_id_map (ctx_p, om_p, 0x111);
      break;
    }
    case VM_OP_CALL_N:
    case VM_OP_CONSTRUCT_N:
    case VM_OP_FUNC_EXPR_N:
    case VM_OP_DELETE_VAR:
    case VM_OP_TYPEOF:
    case VM_OP_B_NOT:
    case VM_OP_LOGICAL_NOT:
    case VM_OP_POST_INCR:
    case VM_OP_POST_DECR:
    case VM_OP_PRE_INCR:
    case VM_OP_PRE_DECR:
    case VM_OP_UNARY_PLUS:
    case VM_OP_UNARY_MINUS:
    {
      insert_uids_to_lit_id_map (ctx_p, om_p, 0x110);
      break;
    }
    case VM_OP_ASSIGNMENT:
    {
      switch (om_p->op.data.assignment.type_value_right)
      {
        case OPCODE_ARG_TYPE_SIMPLE:
        case OPCODE_ARG_TYPE_SMALLINT:
        case OPCODE_ARG_TYPE_SMALLINT_NEGATE:
        {
          insert_uids_to_lit_id_map (ctx_p, om_p, 0x100);
          break;
        }
        case OPCODE_ARG_TYPE_NUMBER:
        case OPCODE_ARG_TYPE_NUMBER_NEGATE:
        case OPCODE_ARG_TYPE_REGEXP:
        case OPCODE_ARG_TYPE_STRING:
        case OPCODE_ARG_TYPE_VARIABLE:
        {
          insert_uids_to_lit_id_map (ctx_p, om_p, 0x101);
          break;
        }
      }
      break;
    }
    case VM_OP_FUNC_DECL_N:
    case VM_OP_FUNC_EXPR_REF:
    case VM_OP_FOR_IN:
    case VM_OP_ARRAY_DECL:
    case VM_OP_OBJ_DECL:
    case VM_OP_WITH:
    case VM_OP_THROW_VALUE:
    case VM_OP_IS_TRUE_JMP_UP:
    case VM_OP_IS_TRUE_JMP_DOWN:
    case VM_OP_IS_FALSE_JMP_UP:
    case VM_OP_IS_FALSE_JMP_DOWN:
    case VM_OP_VAR_DECL:
    case VM_OP_RETVAL:
    {
      insert_uids_to_lit_id_map (ctx_p, om_p, 0x100);
      break;
    }
    case VM_OP_RET:
    case VM_OP_TRY_BLOCK:
    case VM_OP_JMP_UP:
    case VM_OP_JMP_DOWN:
    case VM_OP_JMP_BREAK_CONTINUE:
    case VM_OP_REG_VAR_DECL:
    {
      insert_uids_to_lit_id_map (ctx_p, om_p, 0x000);
      break;
    }
    case VM_OP_META:
    {
      switch (om_p->op.data.meta.type)
      {
        case OPCODE_META_TYPE_VARG_PROP_DATA:
        case OPCODE_META_TYPE_VARG_PROP_GETTER:
        case OPCODE_META_TYPE_VARG_PROP_SETTER:
        {
          insert_uids_to_lit_id_map (ctx_p, om_p, 0x011);
          break;
        }
        case OPCODE_META_TYPE_VARG:
        case OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER:
        {
          insert_uids_to_lit_id_map (ctx_p, om_p, 0x010);
          break;
        }
        case OPCODE_META_TYPE_UNDEFINED:
        case OPCODE_META_TYPE_END_WITH:
        case OPCODE_META_TYPE_FUNCTION_END:
        case OPCODE_META_TYPE_CATCH:
        case OPCODE_META_TYPE_FINALLY:
        case OPCODE_META_TYPE_END_TRY_CATCH_FINALLY:
        case OPCODE_META_TYPE_CALL_SITE_INFO:
        {
          insert_uids_to_lit_id_map (ctx_p, om_p, 0x000);
          break;
        }
      }
      break;
    }

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* count_new_literals_in_instr */

/**
 * Allocate next register for intermediate value
 *
 * @return identifier of the allocated register
 */
static vm_idx_t
jsp_alloc_reg_for_temp (void)
{
  JERRY_ASSERT (jsp_reg_max_for_local_var == VM_IDX_EMPTY);
  JERRY_ASSERT (jsp_reg_max_for_args == VM_IDX_EMPTY);

  vm_idx_t next_reg = jsp_reg_next++;

  if (next_reg > VM_REG_GENERAL_LAST)
  {
    /*
     * FIXME:
     *       Implement mechanism, allowing reusage of register variables
     */
    PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "Not enough register variables", LIT_ITERATOR_POS_ZERO);
  }

  if (jsp_reg_max_for_temps < next_reg)
  {
    jsp_reg_max_for_temps = next_reg;
  }

  return next_reg;
} /* jsp_alloc_reg_for_temp */

/**
 * Get current instruction counter
 *
 * @return number of currently generated instructions
 */
vm_instr_counter_t
dumper_get_current_instr_counter (jsp_ctx_t *ctx_p) /**< parser context */
{
  if (jsp_is_dump_mode (ctx_p))
  {
    bytecode_data_header_t *bc_header_p = jsp_get_current_bytecode_header (ctx_p);

    return bc_header_p->instrs_count;
  }
  else
  {
    return scopes_tree_instrs_num (jsp_get_current_scopes_tree_node (ctx_p));
  }
} /* dumper_get_current_instr_counter */

/**
 * Convert instruction at the specified position to intermediate instruction description
 *
 * @return intermediate instruction description
 */
op_meta
dumper_get_op_meta (jsp_ctx_t *ctx_p, /**< parser context */
                    vm_instr_counter_t pos) /**< instruction position */
{
  op_meta opm;
  if (jsp_is_dump_mode (ctx_p))
  {
    bytecode_data_header_t *bc_header_p = jsp_get_current_bytecode_header (ctx_p);
    JERRY_ASSERT (pos < bc_header_p->instrs_count);

    vm_instr_t *instr_p = bc_header_p->instrs_p + pos;

    opm.op = *instr_p;

    uint16_t mask;

    switch (opm.op.op_idx)
    {
      case VM_OP_PROP_GETTER:
      case VM_OP_PROP_SETTER:
      case VM_OP_DELETE_PROP:
      case VM_OP_B_SHIFT_LEFT:
      case VM_OP_B_SHIFT_RIGHT:
      case VM_OP_B_SHIFT_URIGHT:
      case VM_OP_B_AND:
      case VM_OP_B_OR:
      case VM_OP_B_XOR:
      case VM_OP_EQUAL_VALUE:
      case VM_OP_NOT_EQUAL_VALUE:
      case VM_OP_EQUAL_VALUE_TYPE:
      case VM_OP_NOT_EQUAL_VALUE_TYPE:
      case VM_OP_LESS_THAN:
      case VM_OP_GREATER_THAN:
      case VM_OP_LESS_OR_EQUAL_THAN:
      case VM_OP_GREATER_OR_EQUAL_THAN:
      case VM_OP_INSTANCEOF:
      case VM_OP_IN:
      case VM_OP_ADDITION:
      case VM_OP_SUBSTRACTION:
      case VM_OP_DIVISION:
      case VM_OP_MULTIPLICATION:
      case VM_OP_REMAINDER:
      {
        mask = 0x111;
        break;
      }
      case VM_OP_CALL_N:
      case VM_OP_CONSTRUCT_N:
      case VM_OP_FUNC_EXPR_N:
      case VM_OP_DELETE_VAR:
      case VM_OP_TYPEOF:
      case VM_OP_B_NOT:
      case VM_OP_LOGICAL_NOT:
      case VM_OP_POST_INCR:
      case VM_OP_POST_DECR:
      case VM_OP_PRE_INCR:
      case VM_OP_PRE_DECR:
      case VM_OP_UNARY_PLUS:
      case VM_OP_UNARY_MINUS:
      {
        mask = 0x110;
        break;
      }
      case VM_OP_ASSIGNMENT:
      {
        switch (opm.op.data.assignment.type_value_right)
        {
          case OPCODE_ARG_TYPE_SIMPLE:
          case OPCODE_ARG_TYPE_SMALLINT:
          case OPCODE_ARG_TYPE_SMALLINT_NEGATE:
          {
            mask = 0x100;
            break;
          }
          case OPCODE_ARG_TYPE_NUMBER:
          case OPCODE_ARG_TYPE_NUMBER_NEGATE:
          case OPCODE_ARG_TYPE_REGEXP:
          case OPCODE_ARG_TYPE_STRING:
          case OPCODE_ARG_TYPE_VARIABLE:
          {
            mask = 0x101;
            break;
          }
          default:
          {
            JERRY_UNREACHABLE ();
          }
        }
        break;
      }
      case VM_OP_FUNC_DECL_N:
      case VM_OP_FUNC_EXPR_REF:
      case VM_OP_FOR_IN:
      case VM_OP_ARRAY_DECL:
      case VM_OP_OBJ_DECL:
      case VM_OP_WITH:
      case VM_OP_THROW_VALUE:
      case VM_OP_IS_TRUE_JMP_UP:
      case VM_OP_IS_TRUE_JMP_DOWN:
      case VM_OP_IS_FALSE_JMP_UP:
      case VM_OP_IS_FALSE_JMP_DOWN:
      case VM_OP_VAR_DECL:
      case VM_OP_RETVAL:
      {
        mask = 0x100;
        break;
      }
      case VM_OP_RET:
      case VM_OP_TRY_BLOCK:
      case VM_OP_JMP_UP:
      case VM_OP_JMP_DOWN:
      case VM_OP_JMP_BREAK_CONTINUE:
      case VM_OP_REG_VAR_DECL:
      {
        mask = 0x000;
        break;
      }
      case VM_OP_META:
      {
        switch (opm.op.data.meta.type)
        {
          case OPCODE_META_TYPE_VARG_PROP_DATA:
          case OPCODE_META_TYPE_VARG_PROP_GETTER:
          case OPCODE_META_TYPE_VARG_PROP_SETTER:
          {
            mask = 0x011;
            break;
          }
          case OPCODE_META_TYPE_VARG:
          case OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER:
          {
            mask = 0x010;
            break;
          }
          case OPCODE_META_TYPE_UNDEFINED:
          case OPCODE_META_TYPE_END_WITH:
          case OPCODE_META_TYPE_FUNCTION_END:
          case OPCODE_META_TYPE_CATCH:
          case OPCODE_META_TYPE_FINALLY:
          case OPCODE_META_TYPE_END_TRY_CATCH_FINALLY:
          case OPCODE_META_TYPE_END_FOR_IN:
          case OPCODE_META_TYPE_CALL_SITE_INFO:
          {
            mask = 0x000;
            break;
          }
          default:
          {
            JERRY_UNREACHABLE ();
          }
        }
        break;
      }

      default:
      {
        JERRY_UNREACHABLE ();
      }
    }

    for (uint8_t i = 0; i < 3; i++)
    {
      JERRY_STATIC_ASSERT (VM_IDX_LITERAL_FIRST == 0);

      if (is_possible_literal (mask, i)
          && opm.op.data.raw_args[i] <= VM_IDX_LITERAL_LAST)
      {
        opm.lit_id[i] = bc_get_literal_cp_by_uid (opm.op.data.raw_args[i], bc_header_p, pos);
      }
      else
      {
        opm.lit_id[i] = NOT_A_LITERAL;
      }
    }
  }

  return opm;
} /* dumper_get_op_meta */

/**
 * Dump instruction
 */
static void
dumper_dump_op_meta (jsp_ctx_t *ctx_p, /**< parser context */
                     op_meta opm) /**< intermediate instruction description */
{
  if (jsp_is_dump_mode (ctx_p))
  {
    bytecode_data_header_t *bc_header_p = jsp_get_current_bytecode_header (ctx_p);

    JERRY_ASSERT (bc_header_p->instrs_count < MAX_OPCODES);

#ifdef JERRY_ENABLE_PRETTY_PRINTER
    if (is_print_instrs)
    {
      pp_op_meta (bc_header_p, bc_header_p->instrs_count, opm, false);
    }
#endif /* JERRY_ENABLE_PRETTY_PRINTER */

    vm_instr_t *new_instr_place_p = bc_header_p->instrs_p + bc_header_p->instrs_count;

    bc_header_p->instrs_count++;

    *new_instr_place_p = opm.op;
  }
  else
  {
    scopes_tree scope_p = jsp_get_current_scopes_tree_node (ctx_p);

    count_new_literals_in_instr (ctx_p, scope_p->instrs_count, &opm);

    scope_p->instrs_count++;
  }
} /* dumper_dump_op_meta */

/**
 * Rewrite instruction at specified offset
 */
void
dumper_rewrite_op_meta (jsp_ctx_t *ctx_p, /**< parser context */
                        const vm_instr_counter_t loc, /**< instruction location */
                        op_meta opm) /**< intermediate instruction description */
{
  if (jsp_is_dump_mode (ctx_p))
  {
    bytecode_data_header_t *bc_header_p = jsp_get_current_bytecode_header (ctx_p);
    JERRY_ASSERT (loc < bc_header_p->instrs_count);

    vm_instr_t *instr_p = bc_header_p->instrs_p + loc;

    *instr_p = opm.op;

#ifdef JERRY_ENABLE_PRETTY_PRINTER
    if (is_print_instrs)
    {
      pp_op_meta (bc_header_p, loc, opm, true);
    }
#endif /* JERRY_ENABLE_PRETTY_PRINTER */
  }
} /* dumper_rewrite_op_meta */

#ifdef CONFIG_PARSER_ENABLE_PARSE_TIME_BYTE_CODE_OPTIMIZER
/**
 * Start move of variable values to registers optimization pass
 */
void
dumper_start_move_of_vars_to_regs (jsp_ctx_t *ctx_p __attr_unused___)
{
  JERRY_ASSERT (jsp_reg_max_for_local_var == VM_IDX_EMPTY);
  JERRY_ASSERT (jsp_reg_max_for_args == VM_IDX_EMPTY);

  jsp_reg_max_for_local_var = jsp_reg_max_for_temps;
} /* dumper_start_move_of_vars_to_regs */

/**
 * Start move of argument values to registers optimization pass
 *
 * @return true - if optimization can be performed successfully (i.e. there are enough free registers),
 *         false - otherwise.
 */
bool
dumper_start_move_of_args_to_regs (jsp_ctx_t *ctx_p __attr_unused___,
                                   uint32_t args_num) /**< number of arguments */
{
  JERRY_ASSERT (jsp_reg_max_for_args == VM_IDX_EMPTY);

  if (jsp_reg_max_for_local_var == VM_IDX_EMPTY)
  {
    if (args_num + jsp_reg_max_for_temps >= VM_REG_GENERAL_LAST)
    {
      return false;
    }

    jsp_reg_max_for_args = jsp_reg_max_for_temps;
  }
  else
  {
    if (args_num + jsp_reg_max_for_local_var >= VM_REG_GENERAL_LAST)
    {
      return false;
    }

    jsp_reg_max_for_args = jsp_reg_max_for_local_var;
  }

  return true;
} /* dumper_start_move_of_args_to_regs */

/**
 * Try to move local variable to a register
 *
 * Note:
 *      First instruction of the scope should be either func_decl_n or func_expr_n, as the scope is function scope,
 *      and the optimization is not applied to 'new Function ()'-like constructed functions.
 *
 * See also:
 *          parse_source_element_list
 *          parser_parse_program
 *
 * @return true, if optimization performed successfully, i.e.:
 *                - there is a free register to use;
 *                - the variable name is not equal to any of the function's argument names;
 *         false - otherwise.
 */
bool
dumper_try_replace_identifier_name_with_reg (jsp_ctx_t *ctx_p, /**< parser context */
                                             bytecode_data_header_t *bc_header_p, /**< a byte-code data header,
                                                                                   *   created for function declaration
                                                                                   *   or function expresssion */
                                             op_meta *om_p) /**< operation meta of corresponding
                                                             *   variable declaration */
{
  lit_cpointer_t lit_cp;
  bool is_arg;

  if (om_p->op.op_idx == VM_OP_VAR_DECL)
  {
    JERRY_ASSERT (om_p->lit_id[0].packed_value != NOT_A_LITERAL.packed_value);
    JERRY_ASSERT (om_p->lit_id[1].packed_value == NOT_A_LITERAL.packed_value);
    JERRY_ASSERT (om_p->lit_id[2].packed_value == NOT_A_LITERAL.packed_value);

    lit_cp = om_p->lit_id[0];

    is_arg = false;
  }
  else
  {
    JERRY_ASSERT (om_p->op.op_idx == VM_OP_META);

    JERRY_ASSERT (om_p->op.data.meta.type == OPCODE_META_TYPE_VARG);
    JERRY_ASSERT (om_p->lit_id[0].packed_value == NOT_A_LITERAL.packed_value);
    JERRY_ASSERT (om_p->lit_id[1].packed_value != NOT_A_LITERAL.packed_value);
    JERRY_ASSERT (om_p->lit_id[2].packed_value == NOT_A_LITERAL.packed_value);

    lit_cp = om_p->lit_id[1];

    is_arg = true;
  }

  vm_idx_t reg;

  if (is_arg)
  {
    JERRY_ASSERT (jsp_reg_max_for_args != VM_IDX_EMPTY);
    JERRY_ASSERT (jsp_reg_max_for_args < VM_REG_GENERAL_LAST);

    reg = ++jsp_reg_max_for_args;
  }
  else
  {
    JERRY_ASSERT (jsp_reg_max_for_local_var != VM_IDX_EMPTY);

    if (jsp_reg_max_for_local_var == VM_REG_GENERAL_LAST)
    {
      /* not enough registers */
      return false;
    }
    JERRY_ASSERT (jsp_reg_max_for_local_var < VM_REG_GENERAL_LAST);

    reg = ++jsp_reg_max_for_local_var;
  }

  for (vm_instr_counter_t instr_pos = 0;
       instr_pos < bc_header_p->instrs_count;
       instr_pos++)
  {
    op_meta om = dumper_get_op_meta (ctx_p, instr_pos);

    vm_op_t opcode = (vm_op_t) om.op.op_idx;

    int args_num = 0;

#define VM_OP_0(opcode_name, opcode_name_uppercase) \
    if (opcode == VM_OP_ ## opcode_name_uppercase) \
    { \
      args_num = 0; \
    }
#define VM_OP_1(opcode_name, opcode_name_uppercase, arg1, arg1_type) \
    if (opcode == VM_OP_ ## opcode_name_uppercase) \
    { \
      JERRY_STATIC_ASSERT (((arg1_type) & VM_OP_ARG_TYPE_TYPE_OF_NEXT) == 0); \
      args_num = 1; \
    }
#define VM_OP_2(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type) \
    if (opcode == VM_OP_ ## opcode_name_uppercase) \
    { \
      JERRY_STATIC_ASSERT (((arg1_type) & VM_OP_ARG_TYPE_TYPE_OF_NEXT) == 0); \
      JERRY_STATIC_ASSERT (((arg2_type) & VM_OP_ARG_TYPE_TYPE_OF_NEXT) == 0); \
      args_num = 2; \
    }
#define VM_OP_3(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type, arg3, arg3_type) \
    if (opcode == VM_OP_ ## opcode_name_uppercase) \
    { \
      /*
       * See also:
       *          The loop below
       */ \
      \
      JERRY_ASSERT ((opcode == VM_OP_ASSIGNMENT && (arg2_type) == VM_OP_ARG_TYPE_TYPE_OF_NEXT) \
                    || (opcode != VM_OP_ASSIGNMENT && ((arg2_type) & VM_OP_ARG_TYPE_TYPE_OF_NEXT) == 0)); \
      JERRY_ASSERT ((opcode == VM_OP_META && ((arg1_type) & VM_OP_ARG_TYPE_TYPE_OF_NEXT) != 0) \
                    || (opcode != VM_OP_META && ((arg1_type) & VM_OP_ARG_TYPE_TYPE_OF_NEXT) == 0)); \
      JERRY_STATIC_ASSERT (((arg3_type) & VM_OP_ARG_TYPE_TYPE_OF_NEXT) == 0); \
      args_num = 3; \
    }

#include "vm-opcodes.inc.h"

    for (int arg_index = 0; arg_index < args_num; arg_index++)
    {
      /*
       * 'assignment' and 'meta' are the only opcodes with statically unspecified argument type
       * (checked by assertions above)
       */
      if (opcode == VM_OP_ASSIGNMENT
          && arg_index == 1
          && om.op.data.assignment.type_value_right != VM_OP_ARG_TYPE_VARIABLE)
      {
        break;
      }

      if (opcode == VM_OP_META
          && (((om.op.data.meta.type == OPCODE_META_TYPE_VARG_PROP_DATA
                || om.op.data.meta.type == OPCODE_META_TYPE_VARG_PROP_GETTER
                || om.op.data.meta.type == OPCODE_META_TYPE_VARG_PROP_SETTER)
               && arg_index == 1)
              || om.op.data.meta.type == OPCODE_META_TYPE_END_WITH
              || om.op.data.meta.type == OPCODE_META_TYPE_CATCH
              || om.op.data.meta.type == OPCODE_META_TYPE_FINALLY
              || om.op.data.meta.type == OPCODE_META_TYPE_END_TRY_CATCH_FINALLY
              || om.op.data.meta.type == OPCODE_META_TYPE_END_FOR_IN))
      {
        continue;
      }

      if (om.lit_id[arg_index].packed_value == lit_cp.packed_value)
      {
        om.lit_id[arg_index] = NOT_A_LITERAL;

        om.op.data.raw_args[arg_index] = reg;
      }
    }

    dumper_rewrite_op_meta (ctx_p, instr_pos, om);
  }

  return true;
} /* dumper_try_replace_identifier_name_with_reg */
#endif /* CONFIG_PARSER_ENABLE_PARSE_TIME_BYTE_CODE_OPTIMIZER */

/**
 * Just allocate register for argument that is never used due to duplicated argument names
 */
void
dumper_alloc_reg_for_unused_arg (jsp_ctx_t *ctx_p __attr_unused___)
{
  JERRY_ASSERT (jsp_reg_max_for_args != VM_IDX_EMPTY);
  JERRY_ASSERT (jsp_reg_max_for_args < VM_REG_GENERAL_LAST);

  ++jsp_reg_max_for_args;
} /* dumper_alloc_reg_for_unused_arg */

/**
 * Generate instruction with specified opcode and operands
 *
 * @return VM instruction
 */
static vm_instr_t
jsp_dmp_gen_instr (jsp_ctx_t *ctx_p, /**< parser context */
                   vm_op_t opcode, /**< operation code */
                   jsp_operand_t ops[], /**< operands */
                   size_t ops_num) /**< operands number */
{
  vm_instr_t instr;

  instr.op_idx = opcode;

  for (size_t i = 0; i < ops_num; i++)
  {
    if (jsp_is_empty_operand (ops[i]))
    {
      instr.data.raw_args[i] = VM_IDX_EMPTY;
    }
    else if (jsp_is_unknown_operand (ops[i]))
    {
      instr.data.raw_args[i] = VM_IDX_REWRITE_GENERAL_CASE;
    }
    else if (jsp_is_idx_const_operand (ops[i]))
    {
      instr.data.raw_args[i] = jsp_operand_get_idx_const (ops[i]);
    }
    else if (jsp_is_smallint_operand (ops[i]))
    {
      instr.data.raw_args[i] = jsp_operand_get_smallint_value (ops[i]);
    }
    else if (jsp_is_simple_value_operand (ops[i]))
    {
      instr.data.raw_args[i] = jsp_operand_get_simple_value (ops[i]);
    }
    else if (jsp_is_register_operand (ops[i]) || jsp_is_this_operand (ops[i]))
    {
      instr.data.raw_args[i] = jsp_operand_get_idx (ops[i]);
    }
    else
    {
      lit_cpointer_t lit_cp;

      if (jsp_is_identifier_operand (ops[i]))
      {
        lit_cp = jsp_operand_get_identifier_name (ops[i]);
      }
      else
      {
        JERRY_ASSERT (jsp_is_number_lit_operand (ops[i])
                      || jsp_is_string_lit_operand (ops[i])
                      || jsp_is_regexp_lit_operand (ops[i]));

        lit_cp = jsp_operand_get_literal (ops[i]);
      }

      vm_idx_t idx = VM_IDX_REWRITE_LITERAL_UID;

      if (jsp_is_dump_mode (ctx_p))
      {
        bytecode_data_header_t *bc_header_p = jsp_get_current_bytecode_header (ctx_p);
        lit_id_hash_table *lit_id_hash_p = MEM_CP_GET_NON_NULL_POINTER (lit_id_hash_table,
                                                                        bc_header_p->lit_id_hash_cp);

        idx = lit_id_hash_table_insert (lit_id_hash_p, bc_header_p->instrs_count, lit_cp);
      }

      instr.data.raw_args[i] = idx;
    }
  }

  for (size_t i = ops_num; i < 3; i++)
  {
    instr.data.raw_args[i] = VM_IDX_EMPTY;
  }

  return instr;
} /* jsp_dmp_gen_instr */

/**
 * Create intermediate instruction description, containing pointers to literals,
 * associated with the instruction's arguments, if there are any.
 *
 * @return intermediate operation description
 */
static op_meta
jsp_dmp_create_op_meta (jsp_ctx_t *ctx_p, /**< parser context */
                        vm_op_t opcode, /**< opcode */
                        jsp_operand_t ops[], /**< operands */
                        size_t ops_num) /**< operands number */
{
  op_meta ret;

  ret.op = jsp_dmp_gen_instr (ctx_p, opcode, ops, ops_num);

  for (size_t i = 0; i < ops_num; i++)
  {
    if (jsp_is_number_lit_operand (ops[i])
        || jsp_is_string_lit_operand (ops[i])
        || jsp_is_regexp_lit_operand (ops[i]))
    {
      ret.lit_id[i] = jsp_operand_get_literal (ops[i]);
    }
    else if (jsp_is_identifier_operand (ops[i]))
    {
      ret.lit_id[i] = jsp_operand_get_identifier_name (ops[i]);
    }
    else
    {
      ret.lit_id[i] = NOT_A_LITERAL;
    }
  }

  for (size_t i = ops_num; i < 3; i++)
  {
    ret.lit_id[i] = NOT_A_LITERAL;
  }

  return ret;
} /* jsp_dmp_create_op_meta */

/**
 * Create intermediate instruction description (for instructions without arguments)
 *
 * See also:
 *          jsp_dmp_create_op_meta
 *
 * @return intermediate instruction description
 */
static op_meta
jsp_dmp_create_op_meta_0 (jsp_ctx_t *ctx_p,
                          vm_op_t opcode) /**< opcode */
{
  return jsp_dmp_create_op_meta (ctx_p, opcode, NULL, 0);
} /* jsp_dmp_create_op_meta_0 */

/**
 * Create intermediate instruction description (for instructions with 1 argument)
 *
 * See also:
 *          jsp_dmp_create_op_meta
 *
 * @return intermediate instruction description
 */
static op_meta
jsp_dmp_create_op_meta_1 (jsp_ctx_t *ctx_p, /**< parser context */
                          vm_op_t opcode, /**< opcode */
                          jsp_operand_t operand1) /**< first operand */
{
  return jsp_dmp_create_op_meta (ctx_p, opcode, &operand1, 1);
} /* jsp_dmp_create_op_meta_1 */

/**
 * Create intermediate instruction description (for instructions with 2 arguments)
 *
 * See also:
 *          jsp_dmp_create_op_meta
 *
 * @return intermediate instruction description
 */
static op_meta
jsp_dmp_create_op_meta_2 (jsp_ctx_t *ctx_p, /**< parser context */
                          vm_op_t opcode, /**< opcode */
                          jsp_operand_t operand1, /**< first operand */
                          jsp_operand_t operand2) /**< second operand */
{
  jsp_operand_t ops[] = { operand1, operand2 };
  return jsp_dmp_create_op_meta (ctx_p, opcode, ops, 2);
} /* jsp_dmp_create_op_meta_2 */

/**
 * Create intermediate instruction description (for instructions with 3 arguments)
 *
 * See also:
 *          jsp_dmp_create_op_meta
 *
 * @return intermediate instruction description
 */
static op_meta
jsp_dmp_create_op_meta_3 (jsp_ctx_t *ctx_p, /**< parser context */
                          vm_op_t opcode, /**< opcode */
                          jsp_operand_t operand1, /**< first operand */
                          jsp_operand_t operand2, /**< second operand */
                          jsp_operand_t operand3) /**< third operand */
{
  jsp_operand_t ops[] = { operand1, operand2, operand3 };
  return jsp_dmp_create_op_meta (ctx_p, opcode, ops, 3);
} /* jsp_dmp_create_op_meta_3 */

/**
 * Create temporary operand (alloc available temporary register)
 */
jsp_operand_t
tmp_operand (void)
{
  return jsp_make_reg_operand (jsp_alloc_reg_for_temp ());
} /* tmp_operand */

/**
 * Store instruction counter into two operands
 */
static void
split_instr_counter (vm_instr_counter_t oc, /**< instruction counter */
                     vm_idx_t *id1, /**< pointer to the first operand */
                     vm_idx_t *id2) /**< pointer to the second operand */
{
  JERRY_ASSERT (id1 != NULL);
  JERRY_ASSERT (id2 != NULL);
  *id1 = (vm_idx_t) (oc >> JERRY_BITSINBYTE);
  *id2 = (vm_idx_t) (oc & ((1 << JERRY_BITSINBYTE) - 1));
  JERRY_ASSERT (oc == vm_calc_instr_counter_from_idx_idx (*id1, *id2));
} /* split_instr_counter */

/**
 * Dump single address instruction
 */
static void
dump_single_address (jsp_ctx_t *ctx_p, /**< parser context */
                     vm_op_t opcode, /**< opcode */
                     jsp_operand_t op) /**< first operand */
{
  dumper_dump_op_meta (ctx_p, jsp_dmp_create_op_meta_1 (ctx_p, opcode, op));
} /* dump_single_address */

/*
 * Dump double address instruction
 */
static void
dump_double_address (jsp_ctx_t *ctx_p, /**< parser context */
                     vm_op_t opcode, /**< opcode */
                     jsp_operand_t res, /**< result operand */
                     jsp_operand_t obj) /**< argument operand */
{
  dumper_dump_op_meta (ctx_p, jsp_dmp_create_op_meta_2 (ctx_p, opcode, res, obj));
} /* dump_double_address */

/*
 * Dump triple address instruction
 */
static void
dump_triple_address (jsp_ctx_t *ctx_p, /**< parser context */
                     vm_op_t opcode, /**< opcode */
                     jsp_operand_t res, /**< result operand */
                     jsp_operand_t lhs, /**< first operand */
                     jsp_operand_t rhs) /**< second operand */
{
  dumper_dump_op_meta (ctx_p, jsp_dmp_create_op_meta_3 (ctx_p, opcode, res, lhs, rhs));
} /* dump_triple_address */

/**
 * Get offset from specified instruction (to calculate distance for jump)
 *
 * @return distance between specified and current instruction
 */
static vm_instr_counter_t
get_diff_from (jsp_ctx_t *ctx_p, /**< parser context */
               vm_instr_counter_t oc) /**< position of instruction */
{
  return (vm_instr_counter_t) (dumper_get_current_instr_counter (ctx_p) - oc);
} /* get_diff_from */

/**
 * Create empty operand
 */
jsp_operand_t
empty_operand (void)
{
  return jsp_make_empty_operand ();
} /* empty_operand */

/**
 * Check if operand is empty
 */
bool
operand_is_empty (jsp_operand_t op) /**< operand */
{
  return jsp_is_empty_operand (op);
} /* operand_is_empty */

/**
 * Start dump of a new statement (mark all temporary registers as unused)
 */
void
dumper_new_statement (jsp_ctx_t *ctx_p __attr_unused___)
{
  jsp_reg_next = VM_REG_GENERAL_FIRST;
} /* dumper_new_statement */

/**
 * Save temporary registers context
 */
void
dumper_save_reg_alloc_ctx (jsp_ctx_t *ctx_p __attr_unused___,
                           vm_idx_t *out_saved_reg_next_p, /**< pointer to store index of the next free temporary
                                                            *   register */
                           vm_idx_t *out_saved_reg_max_for_temps_p) /**< pointer to store maximum identifier of a
                                                                     *   register, allocated for intermediate
                                                                     *   value storage */
{
  JERRY_ASSERT (jsp_reg_max_for_local_var == VM_IDX_EMPTY);
  JERRY_ASSERT (jsp_reg_max_for_args == VM_IDX_EMPTY);

  *out_saved_reg_next_p = jsp_reg_next;
  *out_saved_reg_max_for_temps_p = jsp_reg_max_for_temps;

  jsp_reg_next = VM_REG_GENERAL_FIRST;
  jsp_reg_max_for_temps = jsp_reg_next;
} /* dumper_save_reg_alloc_ctx */

/**
 * Restore temporary registers context
 */
void
dumper_restore_reg_alloc_ctx (jsp_ctx_t *ctx_p __attr_unused___,
                              vm_idx_t saved_reg_next, /**< identifier of next free register */
                              vm_idx_t saved_reg_max_for_temps, /**< maximum identifier of register, allocated for
                                                                 *   intermediate value storage */
                              bool is_overwrite_max) /**< flag, indicating if maximum identifier of register,
                                                      *   allocated for intermediate value storage, should be
                                                      *   overwritten, or maximum of current and new value
                                                      *   should be chosen */
{
  JERRY_ASSERT (jsp_reg_max_for_local_var == VM_IDX_EMPTY);
  JERRY_ASSERT (jsp_reg_max_for_args == VM_IDX_EMPTY);

  if (is_overwrite_max)
  {
    jsp_reg_max_for_temps = saved_reg_max_for_temps;
  }
  else
  {
    jsp_reg_max_for_temps = JERRY_MAX (jsp_reg_max_for_temps, saved_reg_max_for_temps);
  }

  jsp_reg_next = saved_reg_next;
} /* dumper_restore_reg_alloc_ctx */

/**
 * Save identifier of the next free register
 *
 * @return identifier of the next free register
 */
vm_idx_t
dumper_save_reg_alloc_counter (jsp_ctx_t *ctx_p __attr_unused___)
{
  return jsp_reg_next;
} /* dumper_save_reg_alloc_counter */

/**
 * Restore value of tthe next free register
 */
void
dumper_restore_reg_alloc_counter (jsp_ctx_t *ctx_p __attr_unused___,
                                  vm_idx_t reg_alloc_counter) /**< value, returned by corresponding
                                                               *   dumper_save_reg_alloc_counter */
{
  jsp_reg_next = reg_alloc_counter;
} /* dumper_restore_reg_alloc_counter */

/**
 * Check that byte-code operand refers to 'eval' string
 *
 * @return true - if specified byte-code operand's type is literal, and value of corresponding
 *                literal is equal to LIT_MAGIC_STRING_EVAL string,
 *         false - otherwise.
 */
bool
dumper_is_eval_literal (jsp_operand_t obj) /**< byte-code operand */
{
  /*
   * FIXME: Switch to corresponding magic string
   */
  if (jsp_is_identifier_operand (obj))
  {
    lit_literal_t lit = lit_get_literal_by_cp (jsp_operand_get_identifier_name (obj));

    return lit_literal_equal_type_cstr (lit, "eval");
  }

  return false;
} /* dumper_is_eval_literal */

/**
 * Dump variable assignment
 */
void
dump_variable_assignment (jsp_ctx_t *ctx_p, /**< parser context */
                          jsp_operand_t res, /**< result operand */
                          jsp_operand_t var) /**< operand, holding the value to assign */
{
  jsp_operand_t type_operand;

  if (jsp_is_string_lit_operand (var))
  {
    type_operand = jsp_make_idx_const_operand (OPCODE_ARG_TYPE_STRING);
  }
  else if (jsp_is_number_lit_operand (var))
  {
    type_operand = jsp_make_idx_const_operand (OPCODE_ARG_TYPE_NUMBER);
  }
  else if (jsp_is_regexp_lit_operand (var))
  {
    type_operand = jsp_make_idx_const_operand (OPCODE_ARG_TYPE_REGEXP);
  }
  else if (jsp_is_smallint_operand (var))
  {
    type_operand = jsp_make_idx_const_operand (OPCODE_ARG_TYPE_SMALLINT);
  }
  else if (jsp_is_simple_value_operand (var))
  {
    type_operand = jsp_make_idx_const_operand (OPCODE_ARG_TYPE_SIMPLE);
  }
  else
  {
    JERRY_ASSERT (jsp_is_identifier_operand (var)
                  || jsp_is_register_operand (var)
                  || jsp_is_this_operand (var));

    type_operand = jsp_make_idx_const_operand (OPCODE_ARG_TYPE_VARIABLE);
  }

  dump_triple_address (ctx_p, VM_OP_ASSIGNMENT, res, type_operand, var);
} /* dump_variable_assignment */

/**
 * Dump instruction, which implies variable number of arguments after it
 */
vm_instr_counter_t
dump_varg_header_for_rewrite (jsp_ctx_t *ctx_p, /**< parser context */
                              varg_list_type vlt, /**< type of instruction */
                              jsp_operand_t res, /**< result operand */
                              jsp_operand_t obj) /**< argument operand */
{
  vm_instr_counter_t pos = dumper_get_current_instr_counter (ctx_p);

  switch (vlt)
  {
    case VARG_FUNC_EXPR:
    {
      dump_triple_address (ctx_p,
                           VM_OP_FUNC_EXPR_N,
                           res,
                           obj,
                           jsp_make_unknown_operand ());
      break;
    }
    case VARG_CONSTRUCT_EXPR:
    {
      dump_triple_address (ctx_p,
                           VM_OP_CONSTRUCT_N,
                           res,
                           obj,
                           jsp_make_unknown_operand ());
      break;
    }
    case VARG_CALL_EXPR:
    {
      dump_triple_address (ctx_p,
                           VM_OP_CALL_N,
                           res,
                           obj,
                           jsp_make_unknown_operand ());
      break;
    }
    case VARG_FUNC_DECL:
    {
      dump_double_address (ctx_p,
                           VM_OP_FUNC_DECL_N,
                           obj,
                           jsp_make_unknown_operand ());
      break;
    }
    case VARG_ARRAY_DECL:
    {
      dump_double_address (ctx_p,
                           VM_OP_ARRAY_DECL,
                           res,
                           jsp_make_unknown_operand ());
      break;
    }
    case VARG_OBJ_DECL:
    {
      dump_double_address (ctx_p,
                           VM_OP_OBJ_DECL,
                           res,
                           jsp_make_unknown_operand ());
      break;
    }
  }

  return pos;
} /* dump_varg_header_for_rewrite */

/**
 * Enumeration of possible rewrite types
 */
typedef enum
{
  REWRITE_VARG_HEADER,
  REWRITE_FUNCTION_END,
  REWRITE_CONDITIONAL_CHECK,
  REWRITE_JUMP_TO_END,
  REWRITE_SIMPLE_OR_NESTED_JUMP,
  REWRITE_CASE_CLAUSE,
  REWRITE_DEFAULT_CLAUSE,
  REWRITE_WITH,
  REWRITE_FOR_IN,
  REWRITE_TRY,
  REWRITE_CATCH,
  REWRITE_FINALLY,
  REWRITE_SCOPE_CODE_FLAGS,
  REWRITE_REG_VAR_DECL,
} rewrite_type_t;

/**
 * Assert operands in rewrite operation
 */
static void
dumper_assert_op_fields (jsp_ctx_t *ctx_p, /**< parser context */
                         rewrite_type_t rewrite_type, /**< type of rewrite */
                         op_meta meta) /**< intermediate instruction description */
{
  if (!jsp_is_dump_mode (ctx_p))
  {
    return;
  }

  if (rewrite_type == REWRITE_FUNCTION_END)
  {
    JERRY_ASSERT (meta.op.op_idx == VM_OP_META);
    JERRY_ASSERT (meta.op.data.meta.type == OPCODE_META_TYPE_FUNCTION_END);
    JERRY_ASSERT (meta.op.data.meta.data_1 == VM_IDX_REWRITE_GENERAL_CASE);
    JERRY_ASSERT (meta.op.data.meta.data_2 == VM_IDX_REWRITE_GENERAL_CASE);
  }
  else if (rewrite_type == REWRITE_CONDITIONAL_CHECK)
  {
    JERRY_ASSERT (meta.op.op_idx == VM_OP_IS_FALSE_JMP_DOWN);
  }
  else if (rewrite_type == REWRITE_JUMP_TO_END)
  {
    JERRY_ASSERT (meta.op.op_idx == VM_OP_JMP_DOWN);
  }
  else if (rewrite_type == REWRITE_CASE_CLAUSE)
  {
    JERRY_ASSERT (meta.op.op_idx == VM_OP_IS_TRUE_JMP_DOWN);
  }
  else if (rewrite_type == REWRITE_DEFAULT_CLAUSE)
  {
    JERRY_ASSERT (meta.op.op_idx == VM_OP_JMP_DOWN);
  }
  else if (rewrite_type == REWRITE_TRY)
  {
    JERRY_ASSERT (meta.op.op_idx == VM_OP_TRY_BLOCK);
  }
  else if (rewrite_type == REWRITE_CATCH)
  {
    JERRY_ASSERT (meta.op.op_idx == VM_OP_META
                  && meta.op.data.meta.type == OPCODE_META_TYPE_CATCH);
  }
  else if (rewrite_type == REWRITE_FINALLY)
  {
    JERRY_ASSERT (meta.op.op_idx == VM_OP_META
                  && meta.op.data.meta.type == OPCODE_META_TYPE_FINALLY);
  }
  else if (rewrite_type == REWRITE_SCOPE_CODE_FLAGS)
  {
    JERRY_ASSERT (meta.op.op_idx == VM_OP_META);
    JERRY_ASSERT (meta.op.data.meta.data_1 == VM_IDX_REWRITE_GENERAL_CASE);
    JERRY_ASSERT (meta.op.data.meta.data_2 == VM_IDX_EMPTY);
  }
  else if (rewrite_type == REWRITE_REG_VAR_DECL)
  {
    JERRY_ASSERT (meta.op.op_idx == VM_OP_REG_VAR_DECL);
  }
  else
  {
    JERRY_UNREACHABLE ();
  }
} /* dumper_assert_op_fields */

/**
 * Rewrite args_count field of an instruction
 */
void
rewrite_varg_header_set_args_count (jsp_ctx_t *ctx_p, /**< parser context */
                                    size_t args_count, /**< number of arguments */
                                    vm_instr_counter_t pos) /**< position of instruction to rewrite */
{
  /*
   * FIXME:
   *       Remove formal parameters / arguments number from instruction,
   *       after ecma-values collection would become extendable (issue #310).
   *       In the case, each 'varg' instruction would just append corresponding
   *       argument / formal parameter name to values collection.
   */

  if (!jsp_is_dump_mode (ctx_p))
  {
    return;
  }

  op_meta om = dumper_get_op_meta (ctx_p, pos);

  switch (om.op.op_idx)
  {
    case VM_OP_FUNC_EXPR_N:
    case VM_OP_CONSTRUCT_N:
    case VM_OP_CALL_N:
    {
      if (args_count > 255)
      {
        PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX,
                     "No more than 255 formal parameters / arguments are currently supported",
                     LIT_ITERATOR_POS_ZERO);
      }
      om.op.data.func_expr_n.arg_list = (vm_idx_t) args_count;
      dumper_rewrite_op_meta (ctx_p, pos, om);
      break;
    }
    case VM_OP_FUNC_DECL_N:
    {
      if (args_count > 255)
      {
        PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX,
                     "No more than 255 formal parameters are currently supported",
                     LIT_ITERATOR_POS_ZERO);
      }
      om.op.data.func_decl_n.arg_list = (vm_idx_t) args_count;
      dumper_rewrite_op_meta (ctx_p, pos, om);
      break;
    }
    case VM_OP_ARRAY_DECL:
    case VM_OP_OBJ_DECL:
    {
      if (args_count > 65535)
      {
        PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX,
                     "No more than 65535 formal parameters are currently supported",
                     LIT_ITERATOR_POS_ZERO);
      }
      om.op.data.obj_decl.list_1 = (vm_idx_t) (args_count >> 8);
      om.op.data.obj_decl.list_2 = (vm_idx_t) (args_count & 0xffu);
      dumper_rewrite_op_meta (ctx_p, pos, om);
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* rewrite_varg_header_set_args_count */

/**
 * Dump 'meta' instruction of 'call additional information' type,
 * containing call flags and, optionally, 'this' argument
 */
void
dump_call_additional_info (jsp_ctx_t *ctx_p, /**< parser context */
                           opcode_call_flags_t flags, /**< call flags */
                           jsp_operand_t this_arg) /**< 'this' argument - if flags
                                                    *   include OPCODE_CALL_FLAGS_HAVE_THIS_ARG,
                                                    *   or empty operand - otherwise */
{
  if (flags & OPCODE_CALL_FLAGS_HAVE_THIS_ARG)
  {
    JERRY_ASSERT (jsp_is_register_operand (this_arg) || jsp_is_this_operand (this_arg));
    JERRY_ASSERT (!operand_is_empty (this_arg));
  }
  else
  {
    JERRY_ASSERT (operand_is_empty (this_arg));
  }

  dump_triple_address (ctx_p,
                       VM_OP_META,
                       jsp_make_idx_const_operand (OPCODE_META_TYPE_CALL_SITE_INFO),
                       jsp_make_idx_const_operand (flags),
                       this_arg);
} /* dump_call_additional_info */

/**
 * Dump meta instruction, specifying the value of the argument
 */
void
dump_varg (jsp_ctx_t *ctx_p, /**< parser context */
           jsp_operand_t op) /**< operand, holding the value of the argument */
{
  dump_triple_address (ctx_p,
                       VM_OP_META,
                       jsp_make_idx_const_operand (OPCODE_META_TYPE_VARG),
                       op,
                       jsp_make_empty_operand ());
} /* dump_varg */

void
dump_prop_name_and_value (jsp_ctx_t *ctx_p, /**< parser context */
                          jsp_operand_t name, /**< property name */
                          jsp_operand_t value) /**< property value */
{
  JERRY_ASSERT (jsp_is_string_lit_operand (name));

  dump_triple_address (ctx_p,
                       VM_OP_META,
                       jsp_make_idx_const_operand (OPCODE_META_TYPE_VARG_PROP_DATA),
                       name,
                       value);
} /* dump_prop_name_and_value */

void
dump_prop_getter_decl (jsp_ctx_t *ctx_p, /**< parser context */
                       jsp_operand_t name, /**< property name */
                       jsp_operand_t func) /**< property getter */
{
  JERRY_ASSERT (jsp_is_string_lit_operand (name));
  JERRY_ASSERT (jsp_is_register_operand (func));

  dump_triple_address (ctx_p,
                       VM_OP_META,
                       jsp_make_idx_const_operand (OPCODE_META_TYPE_VARG_PROP_GETTER),
                       name,
                       func);
} /* dump_prop_getter_decl */

void
dump_prop_setter_decl (jsp_ctx_t *ctx_p, /**< parser context */
                       jsp_operand_t name, /**< property name */
                       jsp_operand_t func) /**< property setter */
{
  JERRY_ASSERT (jsp_is_string_lit_operand (name));
  JERRY_ASSERT (jsp_is_register_operand (func));

  dump_triple_address (ctx_p,
                       VM_OP_META,
                       jsp_make_idx_const_operand (OPCODE_META_TYPE_VARG_PROP_SETTER),
                       name,
                       func);
} /* dump_prop_setter_decl */

/**
 * Dump property getter
 */
void
dump_prop_getter (jsp_ctx_t *ctx_p, /**< parser context */
                  jsp_operand_t obj, /**< result operand */
                  jsp_operand_t base, /**< object, which property is being acquired */
                  jsp_operand_t prop_name) /**< operand, holding property name */
{
  dump_triple_address (ctx_p, VM_OP_PROP_GETTER, obj, base, prop_name);
} /* dump_prop_getter */

/**
 * Dump property setter
 */
void
dump_prop_setter (jsp_ctx_t *ctx_p, /**< parser context */
                  jsp_operand_t base, /**< object, which property is being set */
                  jsp_operand_t prop_name, /**< operand, holding property name */
                  jsp_operand_t obj) /**< operand, holding value to store */
{
  dump_triple_address (ctx_p, VM_OP_PROP_SETTER, base, prop_name, obj);
} /* dump_prop_setter */

/**
 * Dump instruction, which deletes propery of an object
 */
void
dump_delete_prop (jsp_ctx_t *ctx_p, /**< parser context */
                  jsp_operand_t res, /**< operand to store result of delete oeprator */
                  jsp_operand_t base, /**< operand, holding object, from which property is deleted */
                  jsp_operand_t prop_name) /**< operand, holding property name */
{
  dump_triple_address (ctx_p, VM_OP_DELETE_PROP, res, base, prop_name);
} /* dump_delete_prop */

/**
 * Dump unary operation
 */
void
dump_unary_op (jsp_ctx_t *ctx_p, /**< parser context */
               vm_op_t opcode,
               jsp_operand_t res,
               jsp_operand_t op)
{
  dump_double_address (ctx_p, opcode, res, op);
} /* dump_unary_op */

/**
 * Dump binary operation
 */
void
dump_binary_op (jsp_ctx_t *ctx_p, /**< parser context */
                vm_op_t opcode, /**< opcode */
                jsp_operand_t res, /**< result operand */
                jsp_operand_t op1, /**< first operand */
                jsp_operand_t op2) /**< second operand */
{
  dump_triple_address (ctx_p, opcode, res, op1, op2);
} /* dump_binary_op */

/**
 * Dump conditional check, jump offset in which would be updated later
 *
 * @return position of dumped instruction
 */
vm_instr_counter_t
dump_conditional_check_for_rewrite (jsp_ctx_t *ctx_p, /**< parser context */
                                    jsp_operand_t op) /**< operand, holding the value to check */
{
  vm_instr_counter_t pos = dumper_get_current_instr_counter (ctx_p);

  dump_triple_address (ctx_p,
                       VM_OP_IS_FALSE_JMP_DOWN,
                       op,
                       jsp_make_unknown_operand (),
                       jsp_make_unknown_operand ());

  return pos;
} /* dump_conditional_check_for_rewrite */

/**
 * Rewrite jump offset in conditional check
 */
void
rewrite_conditional_check (jsp_ctx_t *ctx_p, /**< parser context */
                           vm_instr_counter_t pos) /**< position to jump to */
{
  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (ctx_p, pos), &id1, &id2);

  op_meta jmp_op_meta = dumper_get_op_meta (ctx_p, pos);
  dumper_assert_op_fields (ctx_p, REWRITE_CONDITIONAL_CHECK, jmp_op_meta);

  jmp_op_meta.op.data.is_false_jmp_down.oc_idx_1 = id1;
  jmp_op_meta.op.data.is_false_jmp_down.oc_idx_2 = id2;

  dumper_rewrite_op_meta (ctx_p, pos, jmp_op_meta);
} /* rewrite_conditional_check */

/**
 * Dump jump to end of the loop, jump offset would be updated by rewrite
 */
vm_instr_counter_t
dump_jump_to_end_for_rewrite (jsp_ctx_t *ctx_p) /**< parser context */
{
  vm_instr_counter_t pos = dumper_get_current_instr_counter (ctx_p);

  dump_double_address (ctx_p,
                       VM_OP_JMP_DOWN,
                       jsp_make_unknown_operand (),
                       jsp_make_unknown_operand ());

  return pos;
} /* dump_jump_to_end_for_rewrite */

/**
 * Rewrite jump offset in jump instruction
 */
void
rewrite_jump_to_end (jsp_ctx_t *ctx_p, /**< parser context */
                     vm_instr_counter_t pos)/**< position to jump to */
{
  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (ctx_p, pos), &id1, &id2);

  op_meta jmp_op_meta = dumper_get_op_meta (ctx_p, pos);
  dumper_assert_op_fields (ctx_p, REWRITE_JUMP_TO_END, jmp_op_meta);

  jmp_op_meta.op.data.jmp_down.oc_idx_1 = id1;
  jmp_op_meta.op.data.jmp_down.oc_idx_2 = id2;

  dumper_rewrite_op_meta (ctx_p, pos, jmp_op_meta);
} /* rewrite_jump_to_end */

/**
 * Get current instruction counter to use as jump target for loop iteration
 */
vm_instr_counter_t
dumper_set_next_iteration_target (jsp_ctx_t *ctx_p) /**< parser context */
{
  return dumper_get_current_instr_counter (ctx_p);
} /* dumper_set_next_iteration_target */

/**
 * Dump conditional/unconditional jump to next iteration of the loop
 */
void
dump_continue_iterations_check (jsp_ctx_t *ctx_p, /**< parser context */
                                vm_instr_counter_t pos, /**< position to jump to */
                                jsp_operand_t op)/**< operand to check in condition
                                                   *   if operand is empty, unconditional jump is
                                                   *   dumped */
{
  const vm_instr_counter_t diff = (vm_instr_counter_t) (dumper_get_current_instr_counter (ctx_p) - pos);
  vm_idx_t id1, id2;
  split_instr_counter (diff, &id1, &id2);

  if (operand_is_empty (op))
  {
    dump_double_address (ctx_p,
                         VM_OP_JMP_UP,
                         jsp_make_idx_const_operand (id1),
                         jsp_make_idx_const_operand (id2));
  }
  else
  {
    dump_triple_address (ctx_p,
                         VM_OP_IS_TRUE_JMP_UP,
                         op,
                         jsp_make_idx_const_operand (id1),
                         jsp_make_idx_const_operand (id2));
  }
}

/**
 * Dump template of a jump instruction.
 *
 * Note:
 *      the instruction's flags field is written later (see also: rewrite_simple_or_nested_jump_get_next).
 *
 * @return position of dumped instruction
 */
vm_instr_counter_t
dump_simple_or_nested_jump_for_rewrite (jsp_ctx_t *ctx_p, /**< parser context */
                                        bool is_nested, /**< flag, indicating whether nested (true)
                                                         *   or simple (false) jump should be dumped */
                                        bool is_conditional, /**< flag, indicating whether conditional (true)
                                                              *   or unconditional jump should be dumped */
                                        bool is_inverted_condition, /**< if is_conditional is set, this flag
                                                                     *   indicates whether to invert the condition */
                                        jsp_operand_t cond, /**< condition (for conditional jumps),
                                                             *   empty operand - for non-conditional */
                                        vm_instr_counter_t next_jump_for_tgt_oc) /**< instr counter of next
                                                                                  *   template targetted to
                                                                                  *   the same target - if any,
                                                                                  *   or MAX_OPCODES - otherwise */
{
  vm_idx_t id1, id2;
  split_instr_counter (next_jump_for_tgt_oc, &id1, &id2);

  vm_instr_counter_t ret = dumper_get_current_instr_counter (ctx_p);

  vm_op_t jmp_opcode;

  if (is_nested)
  {
    jmp_opcode = VM_OP_JMP_BREAK_CONTINUE;
  }
  else if (is_conditional)
  {
    if (is_inverted_condition)
    {
      jmp_opcode = VM_OP_IS_FALSE_JMP_DOWN;
    }
    else
    {
      jmp_opcode = VM_OP_IS_TRUE_JMP_DOWN;
    }
  }
  else
  {
    jmp_opcode = VM_OP_JMP_DOWN;
  }

  if (jmp_opcode == VM_OP_JMP_DOWN
      || jmp_opcode == VM_OP_JMP_BREAK_CONTINUE)
  {
    JERRY_ASSERT (jsp_is_empty_operand (cond));

    dump_double_address (ctx_p,
                         jmp_opcode,
                         jsp_make_idx_const_operand (id1),
                         jsp_make_idx_const_operand (id2));
  }
  else
  {
    JERRY_ASSERT (!jsp_is_empty_operand (cond));

    JERRY_ASSERT (jmp_opcode == VM_OP_IS_FALSE_JMP_DOWN
                  || jmp_opcode == VM_OP_IS_TRUE_JMP_DOWN);

    dump_triple_address (ctx_p,
                         jmp_opcode,
                         cond,
                         jsp_make_idx_const_operand (id1),
                         jsp_make_idx_const_operand (id2));
  }

  return ret;
} /* dump_simple_or_nested_jump_for_rewrite */

/**
 * Write jump target position into previously dumped template of jump (simple or nested) instruction
 *
 * @return instr counter value that was encoded in the jump before rewrite
 */
vm_instr_counter_t
rewrite_simple_or_nested_jump_and_get_next (jsp_ctx_t *ctx_p, /**< parser context */
                                            vm_instr_counter_t jump_oc, /**< position of jump to rewrite */
                                            vm_instr_counter_t target_oc) /**< the jump's target */
{
  if (!jsp_is_dump_mode (ctx_p))
  {
    return MAX_OPCODES;
  }

  op_meta jump_op_meta = dumper_get_op_meta (ctx_p, jump_oc);

  vm_op_t jmp_opcode = (vm_op_t) jump_op_meta.op.op_idx;

  vm_idx_t id1, id2, id1_prev, id2_prev;
  if (target_oc < jump_oc)
  {
    split_instr_counter ((vm_instr_counter_t) (jump_oc - target_oc), &id1, &id2);
  }
  else
  {
    split_instr_counter ((vm_instr_counter_t) (target_oc - jump_oc), &id1, &id2);
  }

  if (jmp_opcode == VM_OP_JMP_DOWN)
  {
    if (target_oc < jump_oc)
    {
      jump_op_meta.op.op_idx = VM_OP_JMP_UP;

      id1_prev = jump_op_meta.op.data.jmp_up.oc_idx_1;
      id2_prev = jump_op_meta.op.data.jmp_up.oc_idx_2;

      jump_op_meta.op.data.jmp_up.oc_idx_1 = id1;
      jump_op_meta.op.data.jmp_up.oc_idx_2 = id2;
    }
    else
    {
      id1_prev = jump_op_meta.op.data.jmp_down.oc_idx_1;
      id2_prev = jump_op_meta.op.data.jmp_down.oc_idx_2;

      jump_op_meta.op.data.jmp_down.oc_idx_1 = id1;
      jump_op_meta.op.data.jmp_down.oc_idx_2 = id2;
    }
  }
  else if (jmp_opcode == VM_OP_IS_TRUE_JMP_DOWN)
  {
    if (target_oc < jump_oc)
    {
      jump_op_meta.op.op_idx = VM_OP_IS_TRUE_JMP_UP;

      id1_prev = jump_op_meta.op.data.is_true_jmp_up.oc_idx_1;
      id2_prev = jump_op_meta.op.data.is_true_jmp_up.oc_idx_2;

      jump_op_meta.op.data.is_true_jmp_up.oc_idx_1 = id1;
      jump_op_meta.op.data.is_true_jmp_up.oc_idx_2 = id2;
    }
    else
    {
      id1_prev = jump_op_meta.op.data.is_true_jmp_down.oc_idx_1;
      id2_prev = jump_op_meta.op.data.is_true_jmp_down.oc_idx_2;

      jump_op_meta.op.data.is_true_jmp_down.oc_idx_1 = id1;
      jump_op_meta.op.data.is_true_jmp_down.oc_idx_2 = id2;
    }
  }
  else if (jmp_opcode == VM_OP_IS_FALSE_JMP_DOWN)
  {
    if (target_oc < jump_oc)
    {
      jump_op_meta.op.op_idx = VM_OP_IS_FALSE_JMP_UP;

      id1_prev = jump_op_meta.op.data.is_false_jmp_up.oc_idx_1;
      id2_prev = jump_op_meta.op.data.is_false_jmp_up.oc_idx_2;

      jump_op_meta.op.data.is_false_jmp_up.oc_idx_1 = id1;
      jump_op_meta.op.data.is_false_jmp_up.oc_idx_2 = id2;
    }
    else
    {
      id1_prev = jump_op_meta.op.data.is_false_jmp_down.oc_idx_1;
      id2_prev = jump_op_meta.op.data.is_false_jmp_down.oc_idx_2;

      jump_op_meta.op.data.is_false_jmp_down.oc_idx_1 = id1;
      jump_op_meta.op.data.is_false_jmp_down.oc_idx_2 = id2;
    }
  }
  else
  {
    JERRY_ASSERT (!jsp_is_dump_mode (ctx_p) || (jmp_opcode == VM_OP_JMP_BREAK_CONTINUE));

    JERRY_ASSERT (target_oc > jump_oc);

    id1_prev = jump_op_meta.op.data.jmp_break_continue.oc_idx_1;
    id2_prev = jump_op_meta.op.data.jmp_break_continue.oc_idx_2;

    jump_op_meta.op.data.jmp_break_continue.oc_idx_1 = id1;
    jump_op_meta.op.data.jmp_break_continue.oc_idx_2 = id2;
  }

  dumper_rewrite_op_meta (ctx_p, jump_oc, jump_op_meta);

  return vm_calc_instr_counter_from_idx_idx (id1_prev, id2_prev);
} /* rewrite_simple_or_nested_jump_get_next */

/**
 * Dump template of 'with' instruction.
 *
 * Note:
 *      the instruction's flags field is written later (see also: rewrite_with).
 *
 * @return position of dumped instruction
 */
vm_instr_counter_t
dump_with_for_rewrite (jsp_ctx_t *ctx_p, /**< parser context */
                       jsp_operand_t op) /**< jsp_operand_t - result of evaluating Expression
                                          *   in WithStatement */
{
  vm_instr_counter_t oc = dumper_get_current_instr_counter (ctx_p);

  dump_triple_address (ctx_p,
                       VM_OP_WITH,
                       op,
                       jsp_make_unknown_operand (),
                       jsp_make_unknown_operand ());

  return oc;
} /* dump_with_for_rewrite */

/**
 * Write position of 'with' block's end to specified 'with' instruction template,
 * dumped earlier (see also: dump_with_for_rewrite).
 */
void
rewrite_with (jsp_ctx_t *ctx_p, /**< parser context */
              vm_instr_counter_t oc) /**< instr counter of the instruction template */
{
  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (ctx_p, oc), &id1, &id2);

  op_meta with_op_meta = dumper_get_op_meta (ctx_p, oc);

  with_op_meta.op.data.with.oc_idx_1 = id1;
  with_op_meta.op.data.with.oc_idx_2 = id2;

  dumper_rewrite_op_meta (ctx_p, oc, with_op_meta);
} /* rewrite_with */

/**
 * Dump 'meta' instruction of 'end with' type
 */
void
dump_with_end (jsp_ctx_t *ctx_p) /**< parser context */
{
  dump_triple_address (ctx_p,
                       VM_OP_META,
                       jsp_make_idx_const_operand (OPCODE_META_TYPE_END_WITH),
                       jsp_make_empty_operand (),
                       jsp_make_empty_operand ());
} /* dump_with_end */

/**
 * Dump template of 'for_in' instruction.
 *
 * Note:
 *      the instruction's flags field is written later (see also: rewrite_for_in).
 *
 * @return position of dumped instruction
 */
vm_instr_counter_t
dump_for_in_for_rewrite (jsp_ctx_t *ctx_p, /**< parser context */
                         jsp_operand_t op) /**< jsp_operand_t - result of evaluating Expression
                                            *   in for-in statement */
{
  vm_instr_counter_t oc = dumper_get_current_instr_counter (ctx_p);

  dump_triple_address (ctx_p,
                       VM_OP_FOR_IN,
                       op,
                       jsp_make_unknown_operand (),
                       jsp_make_unknown_operand ());

  return oc;
} /* dump_for_in_for_rewrite */

/**
 * Write position of 'for_in' block's end to specified 'for_in' instruction template,
 * dumped earlier (see also: dump_for_in_for_rewrite).
 */
void
rewrite_for_in (jsp_ctx_t *ctx_p, /**< parser context */
                vm_instr_counter_t oc) /**< instr counter of the instruction template */
{
  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (ctx_p, oc), &id1, &id2);

  op_meta for_in_op_meta = dumper_get_op_meta (ctx_p, oc);

  for_in_op_meta.op.data.for_in.oc_idx_1 = id1;
  for_in_op_meta.op.data.for_in.oc_idx_2 = id2;

  dumper_rewrite_op_meta (ctx_p, oc, for_in_op_meta);
} /* rewrite_for_in */

/**
 * Dump 'meta' instruction of 'end for_in' type
 */
void
dump_for_in_end (jsp_ctx_t *ctx_p) /**< parser context */
{
  dump_triple_address (ctx_p,
                       VM_OP_META,
                       jsp_make_idx_const_operand (OPCODE_META_TYPE_END_FOR_IN),
                       jsp_make_empty_operand (),
                       jsp_make_empty_operand ());
} /* dump_for_in_end */

/**
 * Dump instruction, designating start of the try block
 *
 * @return position of dumped instruction
 */
vm_instr_counter_t
dump_try_for_rewrite (jsp_ctx_t *ctx_p) /**< parser context */
{
  vm_instr_counter_t pos = dumper_get_current_instr_counter (ctx_p);

  dump_double_address (ctx_p,
                       VM_OP_TRY_BLOCK,
                       jsp_make_unknown_operand (),
                       jsp_make_unknown_operand ());

  return pos;
} /* dump_try_for_rewrite */

/**
 * Rewrite jump offset in instruction, designating start of the try block
 */
void
rewrite_try (jsp_ctx_t *ctx_p, /**< parser context */
             vm_instr_counter_t pos) /**< position of try block end */
{
  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (ctx_p, pos), &id1, &id2);

  op_meta try_op_meta = dumper_get_op_meta (ctx_p, pos);
  dumper_assert_op_fields (ctx_p, REWRITE_TRY, try_op_meta);

  try_op_meta.op.data.try_block.oc_idx_1 = id1;
  try_op_meta.op.data.try_block.oc_idx_2 = id2;

  dumper_rewrite_op_meta (ctx_p, pos, try_op_meta);
} /* rewrite_try */

/**
 * Dump instruction, designating start of the catch block
 *
 * @return position of the dumped instruction
 */
vm_instr_counter_t
dump_catch_for_rewrite (jsp_ctx_t *ctx_p, /**< parser context */
                        jsp_operand_t op)
{
  vm_instr_counter_t pos = dumper_get_current_instr_counter (ctx_p);

  JERRY_ASSERT (jsp_is_string_lit_operand (op));

  dump_triple_address (ctx_p,
                       VM_OP_META,
                       jsp_make_idx_const_operand (OPCODE_META_TYPE_CATCH),
                       jsp_make_unknown_operand (),
                       jsp_make_unknown_operand ());

  dump_triple_address (ctx_p,
                       VM_OP_META,
                       jsp_make_idx_const_operand (OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER),
                       op,
                       jsp_make_empty_operand ());

  return pos;
} /* dump_catch_for_rewrite */

/**
 * Rewrite jump offset in instruction, designating start of the catch block
 */
void
rewrite_catch (jsp_ctx_t *ctx_p, /**< parser context */
               vm_instr_counter_t pos) /**< position of the catch block end */
{
  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (ctx_p, pos), &id1, &id2);

  op_meta catch_op_meta = dumper_get_op_meta (ctx_p, pos);
  dumper_assert_op_fields (ctx_p, REWRITE_CATCH, catch_op_meta);

  catch_op_meta.op.data.meta.data_1 = id1;
  catch_op_meta.op.data.meta.data_2 = id2;

  dumper_rewrite_op_meta (ctx_p, pos, catch_op_meta);
} /* rewrite_catch */

/**
 * Dump instruction, designating start of the finally block
 *
 * @return position of the dumped instruction
 */
vm_instr_counter_t
dump_finally_for_rewrite (jsp_ctx_t *ctx_p) /**< parser context */
{
  vm_instr_counter_t pos = dumper_get_current_instr_counter (ctx_p);

  dump_triple_address (ctx_p,
                       VM_OP_META,
                       jsp_make_idx_const_operand (OPCODE_META_TYPE_FINALLY),
                       jsp_make_unknown_operand (),
                       jsp_make_unknown_operand ());

  return pos;
} /* dump_finally_for_rewrite */

/**
 * Rewrite jump offset in instruction, designating start of the finally block
 */
void
rewrite_finally (jsp_ctx_t *ctx_p, /**< parser context */
                 vm_instr_counter_t pos)  /**< position of the finally block end */
{
  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (ctx_p, pos), &id1, &id2);

  op_meta finally_op_meta = dumper_get_op_meta (ctx_p, pos);
  dumper_assert_op_fields (ctx_p, REWRITE_FINALLY, finally_op_meta);

  finally_op_meta.op.data.meta.data_1 = id1;
  finally_op_meta.op.data.meta.data_2 = id2;

  dumper_rewrite_op_meta (ctx_p, pos, finally_op_meta);
} /* rewrite_finally */

/**
 * Dump end of try-catch-finally block
 */
void
dump_end_try_catch_finally (jsp_ctx_t *ctx_p) /**< parser context */
{
  dump_triple_address (ctx_p,
                       VM_OP_META,
                       jsp_make_idx_const_operand (OPCODE_META_TYPE_END_TRY_CATCH_FINALLY),
                       jsp_make_empty_operand (),
                       jsp_make_empty_operand ());
} /* dump_end_try_catch_finally */

/**
 * Dump throw instruction
 */
void
dump_throw (jsp_ctx_t *ctx_p, /**< parser context */
            jsp_operand_t op)
{
  dump_single_address (ctx_p, VM_OP_THROW_VALUE, op);
}

/**
 * Dump instruction designating variable declaration
 */
void
dump_variable_declaration (jsp_ctx_t *ctx_p, /**< parser context */
                           lit_cpointer_t lit_id) /**< literal which holds variable's name */
{
  jsp_operand_t op_var_name = jsp_make_string_lit_operand (lit_id);
  op_meta op = jsp_dmp_create_op_meta (ctx_p, VM_OP_VAR_DECL, &op_var_name, 1);

  JERRY_ASSERT (!jsp_is_dump_mode (ctx_p));
  scopes_tree_add_var_decl (jsp_get_current_scopes_tree_node (ctx_p), op);
} /* dump_variable_declaration */

/**
 * Dump return instruction
 */
void
dump_ret (jsp_ctx_t *ctx_p) /**< parser context */
{
  dumper_dump_op_meta (ctx_p, jsp_dmp_create_op_meta_0 (ctx_p, VM_OP_RET));
} /* dump_ret */

/**
 * Dump 'reg_var_decl' instruction template
 *
 * @return position of the dumped instruction
 */
vm_instr_counter_t
dump_reg_var_decl_for_rewrite (jsp_ctx_t *ctx_p) /**< parser context */
{
  vm_instr_counter_t oc = dumper_get_current_instr_counter (ctx_p);

  dump_triple_address (ctx_p,
                       VM_OP_REG_VAR_DECL,
                       jsp_make_unknown_operand (),
                       jsp_make_unknown_operand (),
                       jsp_make_unknown_operand ());

  return oc;
} /* dump_reg_var_decl_for_rewrite */

/**
 * Rewrite 'reg_var_decl' instruction's template with current scope's register counts
 */
void
rewrite_reg_var_decl (jsp_ctx_t *ctx_p, /**< parser context */
                      vm_instr_counter_t reg_var_decl_oc) /**< position of dumped 'reg_var_decl' template */
{
  op_meta opm = dumper_get_op_meta (ctx_p, reg_var_decl_oc);
  dumper_assert_op_fields (ctx_p, REWRITE_REG_VAR_DECL, opm);

  opm.op.data.reg_var_decl.tmp_regs_num = (vm_idx_t) (jsp_reg_max_for_temps - VM_REG_GENERAL_FIRST + 1);

  if (jsp_reg_max_for_local_var != VM_IDX_EMPTY)
  {
    JERRY_ASSERT (jsp_reg_max_for_local_var >= jsp_reg_max_for_temps);
    opm.op.data.reg_var_decl.local_var_regs_num = (vm_idx_t) (jsp_reg_max_for_local_var - jsp_reg_max_for_temps);

    jsp_reg_max_for_local_var = VM_IDX_EMPTY;
  }
  else
  {
    opm.op.data.reg_var_decl.local_var_regs_num = 0;
  }

  if (jsp_reg_max_for_args != VM_IDX_EMPTY)
  {
    if (jsp_reg_max_for_local_var != VM_IDX_EMPTY)
    {
      JERRY_ASSERT (jsp_reg_max_for_args >= jsp_reg_max_for_local_var);
      opm.op.data.reg_var_decl.arg_regs_num = (vm_idx_t) (jsp_reg_max_for_args - jsp_reg_max_for_local_var);
    }
    else
    {
      JERRY_ASSERT (jsp_reg_max_for_args >= jsp_reg_max_for_temps);
      opm.op.data.reg_var_decl.arg_regs_num = (vm_idx_t) (jsp_reg_max_for_args - jsp_reg_max_for_temps);
    }

    jsp_reg_max_for_args = VM_IDX_EMPTY;
  }
  else
  {
    opm.op.data.reg_var_decl.arg_regs_num = 0;
  }

  dumper_rewrite_op_meta (ctx_p, reg_var_decl_oc, opm);
} /* rewrite_reg_var_decl */

/**
 * Dump return instruction
 */
void
dump_retval (jsp_ctx_t *ctx_p, /**< parser context */
             jsp_operand_t op) /**< operand, holding a value to return */
{
  dump_single_address (ctx_p, VM_OP_RETVAL, op);
} /* dump_retval */

/**
 * Dumper initialization function
 */
void
dumper_init (jsp_ctx_t *ctx_p __attr_unused___,
             bool show_instrs)
{
  is_print_instrs = show_instrs;

  jsp_reg_next = VM_REG_GENERAL_FIRST;
  jsp_reg_max_for_temps = VM_REG_GENERAL_FIRST;
  jsp_reg_max_for_local_var = VM_IDX_EMPTY;
  jsp_reg_max_for_args = VM_IDX_EMPTY;
} /* dumper_init */


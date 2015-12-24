/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
#include "jrt.h"
#include "jrt-bit-fields.h"
#include "opcodes.h"
#include "opcodes-ecma-support.h"
#include "rcs-records.h"

/**
 * Note:
 *      The note describes exception handling in opcode handlers that perform operations,
 *      that can throw exceptions, and do not themself handle the exceptions.
 *
 *      Generally, each opcode handler consists of sequence of operations.
 *      Some of these operations (exceptionable operations) can throw exceptions and other - cannot.
 *
 *      1. At the beginning of the handler there should be declared opcode handler's 'return value' variable.
 *
 *      2. All exceptionable operations except the last should be enclosed in ECMA_TRY_CATCH macro.
 *         All subsequent operations in the opcode handler should be placed into block between
 *         the ECMA_TRY_CATCH and corresponding ECMA_FINALIZE.
 *
 *      3. The last exceptionable's operation result should be assigned directly to opcode handler's
 *         'return value' variable without using ECMA_TRY_CATCH macro.
 *
 *      4. After last ECMA_FINALIZE statement there should be only one operator.
 *         The operator should return from the opcode handler with it's 'return value'.
 *
 *      5. No other operations with opcode handler's 'return value' variable should be performed.
 *
 * Note:
 *      get_variable_value, taking an idx should be called with instruction counter value,
 *      corresponding to the instruction, containing the idx argument.
 */

/**
 * 'Assignment' opcode handler.
 *
 * Note:
 *      This handler implements case of assignment of a literal's or a variable's
 *      value to a variable. Assignment to an object's property is not implemented
 *      by this opcode.
 *
 * See also: ECMA-262 v5, 11.13.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_assignment (vm_instr_t instr, /**< instruction */
                   vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.assignment.var_left;
  const opcode_arg_type_operand type_value_right = (opcode_arg_type_operand) instr.data.assignment.type_value_right;
  const vm_idx_t src_val_descr = instr.data.assignment.value_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (type_value_right == OPCODE_ARG_TYPE_SIMPLE)
  {
    ret_value = set_variable_value (frame_ctx_p,
                                    frame_ctx_p->pos,
                                    dst_var_idx,
                                    ecma_make_simple_value ((ecma_simple_value_t) src_val_descr));
  }
  else if (type_value_right == OPCODE_ARG_TYPE_STRING)
  {
    lit_cpointer_t lit_cp = bc_get_literal_cp_by_uid (src_val_descr,
                                                      frame_ctx_p->bytecode_header_p,
                                                      frame_ctx_p->pos);
    ecma_string_t *string_p = ecma_new_ecma_string_from_lit_cp (lit_cp);

    ret_value = set_variable_value (frame_ctx_p,
                                    frame_ctx_p->pos,
                                    dst_var_idx,
                                    ecma_make_string_value (string_p));

    ecma_deref_ecma_string (string_p);
  }
  else if (type_value_right == OPCODE_ARG_TYPE_VARIABLE)
  {
    ECMA_TRY_CATCH (var_value,
                    get_variable_value (frame_ctx_p,
                                        src_val_descr,
                                        false),
                    ret_value);

    ret_value = set_variable_value (frame_ctx_p,
                                    frame_ctx_p->pos,
                                    dst_var_idx,
                                    var_value);

    ECMA_FINALIZE (var_value);
  }
  else if (type_value_right == OPCODE_ARG_TYPE_NUMBER)
  {
    ecma_number_t *num_p = frame_ctx_p->tmp_num_p;

    lit_cpointer_t lit_cp = bc_get_literal_cp_by_uid (src_val_descr,
                                                      frame_ctx_p->bytecode_header_p,
                                                      frame_ctx_p->pos);
    lit_literal_t lit = lit_get_literal_by_cp (lit_cp);
    JERRY_ASSERT (RCS_RECORD_IS_NUMBER (lit));

    *num_p = lit_number_literal_get_number (lit);

    ret_value = set_variable_value (frame_ctx_p,
                                    frame_ctx_p->pos,
                                    dst_var_idx,
                                    ecma_make_number_value (num_p));
  }
  else if (type_value_right == OPCODE_ARG_TYPE_NUMBER_NEGATE)
  {
    ecma_number_t *num_p = frame_ctx_p->tmp_num_p;

    lit_cpointer_t lit_cp = bc_get_literal_cp_by_uid (src_val_descr,
                                                      frame_ctx_p->bytecode_header_p,
                                                      frame_ctx_p->pos);
    lit_literal_t lit = lit_get_literal_by_cp (lit_cp);
    JERRY_ASSERT (RCS_RECORD_IS_NUMBER (lit));

    *num_p = lit_number_literal_get_number (lit);

    ret_value = set_variable_value (frame_ctx_p,
                                    frame_ctx_p->pos,
                                    dst_var_idx,
                                    ecma_make_number_value (num_p));
  }
  else if (type_value_right == OPCODE_ARG_TYPE_SMALLINT)
  {
    ecma_number_t *num_p = frame_ctx_p->tmp_num_p;

    *num_p = src_val_descr;

    ret_value = set_variable_value (frame_ctx_p,
                                    frame_ctx_p->pos,
                                    dst_var_idx,
                                    ecma_make_number_value (num_p));
  }
  else if (type_value_right == OPCODE_ARG_TYPE_REGEXP)
  {
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN
    lit_cpointer_t lit_cp = bc_get_literal_cp_by_uid (src_val_descr,
                                                              frame_ctx_p->bytecode_header_p,
                                                              frame_ctx_p->pos);
    ecma_string_t *string_p = ecma_new_ecma_string_from_lit_cp (lit_cp);

    lit_utf8_size_t re_utf8_buffer_size = ecma_string_get_size (string_p);
    MEM_DEFINE_LOCAL_ARRAY (re_utf8_buffer_p, re_utf8_buffer_size, lit_utf8_byte_t);

    ssize_t sz = ecma_string_to_utf8_string (string_p, re_utf8_buffer_p, (ssize_t) re_utf8_buffer_size);
    JERRY_ASSERT (sz >= 0);

    lit_utf8_byte_t *ch_p = re_utf8_buffer_p;
    lit_utf8_byte_t *last_slash_p = NULL;
    while (ch_p < re_utf8_buffer_p + re_utf8_buffer_size)
    {
      if (*ch_p == '/')
      {
        last_slash_p = ch_p;
      }
      ch_p++;
    }

    JERRY_ASSERT (last_slash_p != NULL);
    JERRY_ASSERT ((re_utf8_buffer_p < last_slash_p) && (last_slash_p < ch_p));
    JERRY_ASSERT ((last_slash_p - re_utf8_buffer_p) > 0);
    ecma_string_t *pattern_p = ecma_new_ecma_string_from_utf8 (re_utf8_buffer_p,
                                                               (lit_utf8_size_t) (last_slash_p - re_utf8_buffer_p));
    ecma_string_t *flags_p = NULL;

    if ((ch_p - last_slash_p) > 1)
    {
      flags_p = ecma_new_ecma_string_from_utf8 (last_slash_p + 1,
                                                (lit_utf8_size_t) ((ch_p - last_slash_p - 1)));
    }

    ECMA_TRY_CATCH (regexp_obj_value,
                    ecma_op_create_regexp_object (pattern_p, flags_p),
                    ret_value);

    ret_value = set_variable_value (frame_ctx_p,
                                    frame_ctx_p->pos,
                                    dst_var_idx,
                                    regexp_obj_value);

    ECMA_FINALIZE (regexp_obj_value);

    ecma_deref_ecma_string (pattern_p);
    if (flags_p != NULL)
    {
      ecma_deref_ecma_string (flags_p);
    }

    MEM_FINALIZE_LOCAL_ARRAY (re_utf8_buffer_p)
    ecma_deref_ecma_string (string_p);
#else
    JERRY_UNIMPLEMENTED ("Regular Expressions are not supported in compact profile!");
#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
  }
  else
  {
    JERRY_ASSERT (type_value_right == OPCODE_ARG_TYPE_SMALLINT_NEGATE);
    ecma_number_t *num_p = frame_ctx_p->tmp_num_p;

    *num_p = ecma_number_negate (src_val_descr);

    ret_value = set_variable_value (frame_ctx_p,
                                    frame_ctx_p->pos,
                                    dst_var_idx,
                                    ecma_make_number_value (num_p));
  }

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_assignment */

/**
 * 'Pre increment' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_pre_incr (vm_instr_t instr, /**< instruction */
                 vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.pre_incr.dst;
  const vm_idx_t incr_var_idx = instr.data.pre_incr.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  // 1., 2., 3.
  ECMA_TRY_CATCH (old_value, get_variable_value (frame_ctx_p, incr_var_idx, true), ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (old_num, old_value, ret_value);

  // 4.
  ecma_number_t* new_num_p = frame_ctx_p->tmp_num_p;

  *new_num_p = ecma_number_add (old_num, ECMA_NUMBER_ONE);

  ecma_value_t new_num_value = ecma_make_number_value (new_num_p);

  // 5.
  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos,
                                  incr_var_idx,
                                  new_num_value);

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (frame_ctx_p, frame_ctx_p->pos,
                                                                   dst_var_idx,
                                                                   new_num_value);
  JERRY_ASSERT (ecma_is_completion_value_empty (reg_assignment_res));

  ECMA_OP_TO_NUMBER_FINALIZE (old_num);
  ECMA_FINALIZE (old_value);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_pre_incr */

/**
 * 'Pre decrement' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_pre_decr (vm_instr_t instr, /**< instruction */
                 vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.pre_decr.dst;
  const vm_idx_t decr_var_idx = instr.data.pre_decr.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  // 1., 2., 3.
  ECMA_TRY_CATCH (old_value, get_variable_value (frame_ctx_p, decr_var_idx, true), ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (old_num, old_value, ret_value);

  // 4.
  ecma_number_t* new_num_p = frame_ctx_p->tmp_num_p;

  *new_num_p = ecma_number_substract (old_num, ECMA_NUMBER_ONE);

  ecma_value_t new_num_value = ecma_make_number_value (new_num_p);

  // 5.
  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos,
                                  decr_var_idx,
                                  new_num_value);

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (frame_ctx_p, frame_ctx_p->pos,
                                                                   dst_var_idx,
                                                                   new_num_value);
  JERRY_ASSERT (ecma_is_completion_value_empty (reg_assignment_res));

  ECMA_OP_TO_NUMBER_FINALIZE (old_num);
  ECMA_FINALIZE (old_value);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_pre_decr */

/**
 * 'Post increment' opcode handler.
 *
 * See also: ECMA-262 v5, 11.3.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_post_incr (vm_instr_t instr, /**< instruction */
                  vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.post_incr.dst;
  const vm_idx_t incr_var_idx = instr.data.post_incr.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  // 1., 2., 3.
  ECMA_TRY_CATCH (old_value, get_variable_value (frame_ctx_p, incr_var_idx, true), ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (old_num, old_value, ret_value);

  // 4.
  ecma_number_t* new_num_p = frame_ctx_p->tmp_num_p;

  *new_num_p = ecma_number_add (old_num, ECMA_NUMBER_ONE);

  // 5.
  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos,
                                  incr_var_idx,
                                  ecma_make_number_value (new_num_p));

  ecma_number_t *tmp_p = frame_ctx_p->tmp_num_p;
  *tmp_p = old_num;

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (frame_ctx_p, frame_ctx_p->pos,
                                                                   dst_var_idx,
                                                                   ecma_make_number_value (tmp_p));
  JERRY_ASSERT (ecma_is_completion_value_empty (reg_assignment_res));

  ECMA_OP_TO_NUMBER_FINALIZE (old_num);
  ECMA_FINALIZE (old_value);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_post_incr */

/**
 * 'Post decrement' opcode handler.
 *
 * See also: ECMA-262 v5, 11.3.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_post_decr (vm_instr_t instr, /**< instruction */
                  vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.post_decr.dst;
  const vm_idx_t decr_var_idx = instr.data.post_decr.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  // 1., 2., 3.
  ECMA_TRY_CATCH (old_value, get_variable_value (frame_ctx_p, decr_var_idx, true), ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (old_num, old_value, ret_value);

  // 4.
  ecma_number_t* new_num_p = frame_ctx_p->tmp_num_p;

  *new_num_p = ecma_number_substract (old_num, ECMA_NUMBER_ONE);

  // 5.
  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos,
                                  decr_var_idx,
                                  ecma_make_number_value (new_num_p));

  ecma_number_t *tmp_p = frame_ctx_p->tmp_num_p;
  *tmp_p = old_num;

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (frame_ctx_p, frame_ctx_p->pos,
                                                                   dst_var_idx,
                                                                   ecma_make_number_value (tmp_p));
  JERRY_ASSERT (ecma_is_completion_value_empty (reg_assignment_res));

  ECMA_OP_TO_NUMBER_FINALIZE (old_num);
  ECMA_FINALIZE (old_value);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_post_decr */

/**
 * 'Register variable declaration' opcode handler.
 *
 * The opcode is meta-opcode that is not supposed to be executed.
 */
ecma_completion_value_t
opfunc_reg_var_decl (vm_instr_t instr __attr_unused___, /**< instruction */
                     vm_frame_ctx_t *frame_ctx_p __attr_unused___) /**< interpreter context */
{
  JERRY_UNREACHABLE ();
} /* opfunc_reg_var_decl */

/**
 * 'Variable declaration' opcode handler.
 *
 * See also: ECMA-262 v5, 10.5 - Declaration binding instantiation (block 8).
 *
 * @return completion value
 *         Returned value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_completion_value_t
opfunc_var_decl (vm_instr_t instr __attr_unused___, /**< instruction */
                 vm_frame_ctx_t *frame_ctx_p __attr_unused___) /**< interpreter context */
{
  JERRY_UNREACHABLE ();
} /* opfunc_var_decl */

/**
 * Function declaration helper
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
vm_function_declaration (const bytecode_data_header_t *bytecode_header_p, /**< byte-code header */
                         bool is_strict, /**< is function declared in strict mode code? */
                         bool is_eval_code, /**< is function declared in eval code? */
                         ecma_object_t *lex_env_p) /**< lexical environment to use */
{
  vm_instr_t *function_instrs_p = bytecode_header_p->instrs_p;

  vm_instr_counter_t instr_pos = 0;

  vm_instr_t func_header_instr = function_instrs_p[instr_pos];

  bool is_func_decl;
  vm_idx_t function_name_idx;
  ecma_length_t params_number;

  if (func_header_instr.op_idx == VM_OP_FUNC_DECL_N)
  {
    is_func_decl = true;

    function_name_idx = func_header_instr.data.func_decl_n.name_lit_idx;
    params_number = func_header_instr.data.func_decl_n.arg_list;
  }
  else
  {
    is_func_decl = false;

    function_name_idx = func_header_instr.data.func_expr_n.name_lit_idx;
    params_number = func_header_instr.data.func_expr_n.arg_list;
  }

  lit_cpointer_t function_name_lit_cp = NOT_A_LITERAL;

  if (function_name_idx != VM_IDX_EMPTY)
  {
    function_name_lit_cp = bc_get_literal_cp_by_uid (function_name_idx,
                                                     bytecode_header_p,
                                                     instr_pos);
  }
  else
  {
    JERRY_ASSERT (!is_func_decl);
  }

  instr_pos++;

  ecma_completion_value_t ret_value;

  ecma_collection_header_t *formal_params_collection_p = NULL;

  if (bytecode_header_p->is_args_moved_to_regs)
  {
    instr_pos = (vm_instr_counter_t) (instr_pos + params_number);
  }
  else if (params_number != 0)
  {
    formal_params_collection_p = ecma_new_strings_collection (NULL, 0);

    instr_pos = vm_fill_params_list (bytecode_header_p,
                                     instr_pos,
                                     params_number,
                                     formal_params_collection_p);
  }

  if (is_func_decl)
  {
    const bool is_configurable_bindings = is_eval_code;

    ecma_string_t *function_name_string_p = ecma_new_ecma_string_from_lit_cp (function_name_lit_cp);

    ret_value = ecma_op_function_declaration (lex_env_p,
                                              function_name_string_p,
                                              bytecode_header_p,
                                              instr_pos,
                                              formal_params_collection_p,
                                              is_strict,
                                              is_configurable_bindings);

    ecma_deref_ecma_string (function_name_string_p);
  }
  else
  {
    ecma_object_t *scope_p;
    ecma_string_t *function_name_string_p = NULL;

    bool is_named_func_expr = (function_name_idx != VM_IDX_EMPTY);

    if (is_named_func_expr)
    {
      scope_p = ecma_create_decl_lex_env (lex_env_p);

      JERRY_ASSERT (function_name_lit_cp.packed_value != MEM_CP_NULL);

      function_name_string_p = ecma_new_ecma_string_from_lit_cp (function_name_lit_cp);
      ecma_op_create_immutable_binding (scope_p, function_name_string_p);
    }
    else
    {
      scope_p = lex_env_p;
      ecma_ref_object (scope_p);
    }

    ecma_object_t *func_obj_p = ecma_op_create_function_object (formal_params_collection_p,
                                                                scope_p,
                                                                is_strict,
                                                                bytecode_header_p,
                                                                instr_pos);

    ret_value = ecma_make_normal_completion_value (ecma_make_object_value (func_obj_p));

    if (is_named_func_expr)
    {
      ecma_op_initialize_immutable_binding (scope_p,
                                            function_name_string_p,
                                            ecma_make_object_value (func_obj_p));
      ecma_deref_ecma_string (function_name_string_p);
    }

    ecma_deref_object (scope_p);
  }

  return ret_value;
} /* vm_function_declaration */

/**
 * 'Function declaration header' opcode handler.
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_func_decl_n (vm_instr_t instr __attr_unused___, /**< instruction */
                    vm_frame_ctx_t *frame_ctx_p __attr_unused___) /**< interpreter context */
{
  JERRY_UNREACHABLE ();
} /* opfunc_func_decl_n */

/**
 * 'Function expression header' opcode handler.
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_func_expr_n (vm_instr_t instr __attr_unused___, /**< instruction */
                    vm_frame_ctx_t *frame_ctx_p __attr_unused___) /**< interpreter context */
{
  JERRY_UNREACHABLE ();
} /* opfunc_func_expr_n */

/**
 * 'Function expression' opcode handler.
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_func_expr_ref (vm_instr_t instr, /**< instruction */
                      vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_instr_counter_t lit_oc = frame_ctx_p->pos;

  frame_ctx_p->pos++;

  const vm_idx_t dst_var_idx = instr.data.func_expr_ref.lhs;
  const vm_idx_t idx1 = instr.data.func_expr_ref.idx1;
  const vm_idx_t idx2 = instr.data.func_expr_ref.idx2;

  uint16_t index = (uint16_t) jrt_set_bit_field_value (jrt_set_bit_field_value (0, idx1, 0, JERRY_BITSINBYTE),
                                                       idx2, JERRY_BITSINBYTE, JERRY_BITSINBYTE);

  const bytecode_data_header_t *header_p = frame_ctx_p->bytecode_header_p;
  mem_cpointer_t *declarations_p = MEM_CP_GET_POINTER (mem_cpointer_t, header_p->declarations_cp);
  JERRY_ASSERT (index < header_p->func_scopes_count);

  const bytecode_data_header_t *func_expr_bc_header_p = MEM_CP_GET_NON_NULL_POINTER (const bytecode_data_header_t,
                                                                                     declarations_p[index]);

  JERRY_ASSERT (func_expr_bc_header_p->instrs_p[0].op_idx == VM_OP_FUNC_EXPR_N);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (func_obj_value,
                  vm_function_declaration (func_expr_bc_header_p,
                                           frame_ctx_p->is_strict,
                                           frame_ctx_p->is_eval_code,
                                           frame_ctx_p->lex_env_p),
                  ret_value);

  JERRY_ASSERT (ecma_is_constructor (func_obj_value));

  ret_value = set_variable_value (frame_ctx_p, lit_oc, dst_var_idx, func_obj_value);

  ECMA_FINALIZE (func_obj_value);

  return ret_value;
} /* opfunc_func_expr_ref */

/**
 * Get 'this' argument value and call flags mask for function call
 *
 * See also:
 *          ECMA-262 v5, 11.2.3, steps 6-7

 * @return ecma-value
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
vm_helper_call_get_call_flags_and_this_arg (vm_frame_ctx_t *int_data_p, /**< interpreter context */
                                            vm_instr_counter_t var_idx_lit_oc, /**< instruction counter of instruction
                                                                                    with var_idx */
                                            vm_idx_t var_idx, /**< idx, used to retrieve the called function object */
                                            opcode_call_flags_t *out_flags_p) /**< out: call flags */
{
  JERRY_ASSERT (out_flags_p != NULL);

  bool is_increase_instruction_pointer;

  opcode_call_flags_t call_flags = OPCODE_CALL_FLAGS__EMPTY;
  vm_idx_t this_arg_var_idx = VM_IDX_EMPTY;

  vm_instr_t next_opcode = vm_get_instr (int_data_p->bytecode_header_p->instrs_p, int_data_p->pos);
  if (next_opcode.op_idx == VM_OP_META
      && next_opcode.data.meta.type == OPCODE_META_TYPE_CALL_SITE_INFO)
  {
    call_flags = (opcode_call_flags_t) next_opcode.data.meta.data_1;

    if (call_flags & OPCODE_CALL_FLAGS_HAVE_THIS_ARG)
    {
      this_arg_var_idx = next_opcode.data.meta.data_2;
      JERRY_ASSERT (vm_is_reg_variable (this_arg_var_idx));

      JERRY_ASSERT ((call_flags & OPCODE_CALL_FLAGS_DIRECT_CALL_TO_EVAL_FORM) == 0);
    }

    is_increase_instruction_pointer = true;
  }
  else
  {
    is_increase_instruction_pointer = false;
  }

  ecma_completion_value_t get_this_completion_value;
  ecma_value_t this_value;

  if (call_flags & OPCODE_CALL_FLAGS_HAVE_THIS_ARG)
  {
    /* 6.a.i */
    get_this_completion_value = get_variable_value (int_data_p, this_arg_var_idx, false);
  }
  else
  {
    /*
     * Note:
     *      If function object is in a register variable, then base is not lexical environment
     *
     * See also:
     *          parse_argument_list
     */
    if (!vm_is_reg_variable (var_idx))
    {
      /*
       * FIXME [PERF]:
       *              The second search for base lexical environment can be removed or performed less frequently.
       *
       *              For example, in one of the following ways:
       *               - parser could dump meta information about whether current lexical environment chain
       *                 can contain object-bound lexical environment, created using 'with' statement
       *                 (in the case, the search should be performed, otherwise - it is unnecessary,
       *                  as base lexical environment would be either the Global lexical environment,
       *                  or a declarative lexical environment);
       *               - get_variable_value and opfunc_prop_getter could save last resolved base
       *                 into interpreter context (in the case, `have_this_arg` can be unnecesary,
       *                 as base would always be accessible through interpreter context; on the other hand,
       *                 some stack-like description of active nested call expressions should be provided).
       *
       */

      /* 6.b.i */
      ecma_string_t var_name_string;
      lit_cpointer_t lit_cp = bc_get_literal_cp_by_uid (var_idx,
                                                        int_data_p->bytecode_header_p,
                                                        var_idx_lit_oc);
      ecma_new_ecma_string_on_stack_from_lit_cp (&var_name_string, lit_cp);

      ecma_object_t *ref_base_lex_env_p = ecma_op_resolve_reference_base (int_data_p->lex_env_p,
                                                                          &var_name_string);
      get_this_completion_value = ecma_op_implicit_this_value (ref_base_lex_env_p);
      JERRY_ASSERT (ref_base_lex_env_p != NULL);

      ecma_check_that_ecma_string_need_not_be_freed (&var_name_string);
    }
    else
    {
      /* 7. a */
      get_this_completion_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
  }
  JERRY_ASSERT (ecma_is_completion_value_normal (get_this_completion_value));

  this_value = ecma_get_completion_value_value (get_this_completion_value);

  if (is_increase_instruction_pointer)
  {
    int_data_p->pos++;
  }

  *out_flags_p = call_flags;

  return this_value;
} /* vm_helper_call_get_call_flags_and_this_arg */

/**
 * 'Function call' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_call_n (vm_instr_t instr, /**< instruction */
               vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t lhs_var_idx = instr.data.call_n.lhs;
  const vm_idx_t function_var_idx = instr.data.call_n.function_var_idx;
  const vm_idx_t args_number_idx = instr.data.call_n.arg_list;
  const vm_instr_counter_t lit_oc = frame_ctx_p->pos;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (func_value, get_variable_value (frame_ctx_p, function_var_idx, false), ret_value);

  frame_ctx_p->pos++;

  JERRY_ASSERT (!frame_ctx_p->is_call_in_direct_eval_form);

  opcode_call_flags_t call_flags;
  ecma_value_t this_value = vm_helper_call_get_call_flags_and_this_arg (frame_ctx_p,
                                                                        lit_oc,
                                                                        function_var_idx,
                                                                        &call_flags);

  ecma_collection_header_t *arg_collection_p = ecma_new_values_collection (NULL, 0, true);

  ecma_completion_value_t get_arg_completion = vm_fill_varg_list (frame_ctx_p,
                                                                  args_number_idx,
                                                                  arg_collection_p);
  ecma_length_t args_read = arg_collection_p->unit_number;

  if (ecma_is_completion_value_empty (get_arg_completion))
  {
    JERRY_ASSERT (args_read == args_number_idx);

    if (!ecma_op_is_callable (func_value))
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      if (call_flags & OPCODE_CALL_FLAGS_DIRECT_CALL_TO_EVAL_FORM)
      {
        frame_ctx_p->is_call_in_direct_eval_form = true;
      }

      ecma_object_t *func_obj_p = ecma_get_object_from_value (func_value);

      ECMA_TRY_CATCH (call_ret_value,
                      ecma_op_function_call (func_obj_p,
                                             this_value,
                                             arg_collection_p),
                      ret_value);

      ret_value = set_variable_value (frame_ctx_p, lit_oc,
                                      lhs_var_idx,
                                      call_ret_value);

      ECMA_FINALIZE (call_ret_value);

      if (call_flags & OPCODE_CALL_FLAGS_DIRECT_CALL_TO_EVAL_FORM)
      {
        JERRY_ASSERT (frame_ctx_p->is_call_in_direct_eval_form);
        frame_ctx_p->is_call_in_direct_eval_form = false;
      }
      else
      {
        JERRY_ASSERT (!frame_ctx_p->is_call_in_direct_eval_form);
      }
    }
  }
  else
  {
    JERRY_ASSERT (!ecma_is_completion_value_normal (get_arg_completion));

    ret_value = get_arg_completion;
  }

  ecma_free_values_collection (arg_collection_p, true);
  ecma_free_value (this_value, true);

  ECMA_FINALIZE (func_value);

  return ret_value;
} /* opfunc_call_n */

/**
 * 'Constructor call' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_construct_n (vm_instr_t instr, /**< instruction */
                    vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t lhs_var_idx = instr.data.construct_n.lhs;
  const vm_idx_t constructor_name_lit_idx = instr.data.construct_n.name_lit_idx;
  const vm_idx_t args_number = instr.data.construct_n.arg_list;
  const vm_instr_counter_t lit_oc = frame_ctx_p->pos;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (constructor_value,
                  get_variable_value (frame_ctx_p, constructor_name_lit_idx, false),
                  ret_value);

  frame_ctx_p->pos++;

  ecma_collection_header_t *arg_collection_p = ecma_new_values_collection (NULL, 0, true);

  ecma_completion_value_t get_arg_completion = vm_fill_varg_list (frame_ctx_p,
                                                                  args_number,
                                                                  arg_collection_p);
  ecma_length_t args_read = arg_collection_p->unit_number;

  if (ecma_is_completion_value_empty (get_arg_completion))
  {
    JERRY_ASSERT (args_read == args_number);

    if (!ecma_is_constructor (constructor_value))
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      ecma_object_t *constructor_obj_p = ecma_get_object_from_value (constructor_value);

      ECMA_TRY_CATCH (construction_ret_value,
                      ecma_op_function_construct (constructor_obj_p,
                                                  arg_collection_p),
                      ret_value);

      ret_value = set_variable_value (frame_ctx_p, lit_oc, lhs_var_idx,
                                      construction_ret_value);

      ECMA_FINALIZE (construction_ret_value);
    }
  }
  else
  {
    JERRY_ASSERT (!ecma_is_completion_value_normal (get_arg_completion));

    ret_value = get_arg_completion;
  }

  ecma_free_values_collection (arg_collection_p, true);

  ECMA_FINALIZE (constructor_value);

  return ret_value;
} /* opfunc_construct_n */

/**
 * 'Array initializer' opcode handler.
 *
 * See also: ECMA-262 v5, 11.1.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_array_decl (vm_instr_t instr, /**< instruction */
                   vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t lhs_var_idx = instr.data.array_decl.lhs;
  const vm_idx_t args_number_high_byte = instr.data.array_decl.list_1;
  const vm_idx_t args_number_low_byte = instr.data.array_decl.list_2;
  const vm_instr_counter_t lit_oc = frame_ctx_p->pos;

  frame_ctx_p->pos++;

  ecma_length_t args_number = (((ecma_length_t) args_number_high_byte << JERRY_BITSINBYTE)
                               + (ecma_length_t) args_number_low_byte);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ecma_collection_header_t *arg_collection_p = ecma_new_values_collection (NULL, 0, true);

  ecma_completion_value_t get_arg_completion = vm_fill_varg_list (frame_ctx_p,
                                                                  args_number,
                                                                  arg_collection_p);
  ecma_length_t args_read = arg_collection_p->unit_number;

  if (ecma_is_completion_value_empty (get_arg_completion))
  {
    JERRY_ASSERT (args_read == args_number);

    MEM_DEFINE_LOCAL_ARRAY (arg_values, args_read, ecma_value_t);

    ecma_collection_iterator_t arg_collection_iter;
    ecma_collection_iterator_init (&arg_collection_iter,
                                   arg_collection_p);

    for (ecma_length_t arg_index = 0;
         ecma_collection_iterator_next (&arg_collection_iter);
         arg_index++)
    {
      arg_values[arg_index] = *arg_collection_iter.current_value_p;
    }

    ECMA_TRY_CATCH (array_obj_value,
                    ecma_op_create_array_object (arg_values,
                                                 args_number,
                                                 false),
                    ret_value);

    ret_value = set_variable_value (frame_ctx_p, lit_oc,
                                    lhs_var_idx,
                                    array_obj_value);

    ECMA_FINALIZE (array_obj_value);

    MEM_FINALIZE_LOCAL_ARRAY (arg_values);
  }
  else
  {
    JERRY_ASSERT (!ecma_is_completion_value_normal (get_arg_completion));

    ret_value = get_arg_completion;
  }

  ecma_free_values_collection (arg_collection_p, true);

  return ret_value;
} /* opfunc_array_decl */

/**
 * 'Object initializer' opcode handler.
 *
 * See also: ECMA-262 v5, 11.1.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_obj_decl (vm_instr_t instr, /**< instruction */
                 vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t lhs_var_idx = instr.data.obj_decl.lhs;
  const vm_idx_t args_number_high_byte = instr.data.obj_decl.list_1;
  const vm_idx_t args_number_low_byte = instr.data.obj_decl.list_2;
  const vm_instr_counter_t obj_lit_oc = frame_ctx_p->pos;

  frame_ctx_p->pos++;

  ecma_length_t args_number = (((ecma_length_t) args_number_high_byte << JERRY_BITSINBYTE)
                               + (ecma_length_t) args_number_low_byte);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ecma_object_t *obj_p = ecma_op_create_object_object_noarg ();

  for (uint32_t prop_index = 0;
       prop_index < args_number && ecma_is_completion_value_empty (ret_value);
       prop_index++)
  {
    ecma_completion_value_t evaluate_prop_completion = vm_loop (frame_ctx_p, NULL);

    if (ecma_is_completion_value_empty (evaluate_prop_completion))
    {
      vm_instr_t next_opcode = vm_get_instr (frame_ctx_p->bytecode_header_p->instrs_p, frame_ctx_p->pos);
      JERRY_ASSERT (next_opcode.op_idx == VM_OP_META);

      const opcode_meta_type type = (opcode_meta_type) next_opcode.data.meta.type;
      JERRY_ASSERT (type == OPCODE_META_TYPE_VARG_PROP_DATA
                    || type == OPCODE_META_TYPE_VARG_PROP_GETTER
                    || type == OPCODE_META_TYPE_VARG_PROP_SETTER);

      const vm_idx_t prop_name_idx = next_opcode.data.meta.data_1;

      const vm_idx_t value_for_prop_desc_var_idx = next_opcode.data.meta.data_2;

      ECMA_TRY_CATCH (value_for_prop_desc,
                      get_variable_value (frame_ctx_p,
                                          value_for_prop_desc_var_idx,
                                          false),
                      ret_value);

      lit_cpointer_t prop_name_lit_cp = bc_get_literal_cp_by_uid (prop_name_idx,
                                                                  frame_ctx_p->bytecode_header_p,
                                                                  frame_ctx_p->pos);
      JERRY_ASSERT (prop_name_lit_cp.packed_value != MEM_CP_NULL);
      ecma_string_t *prop_name_string_p = ecma_new_ecma_string_from_lit_cp (prop_name_lit_cp);

      bool is_throw_syntax_error = false;

      ecma_property_t *previous_p = ecma_op_object_get_own_property (obj_p, prop_name_string_p);

      const bool is_previous_undefined = (previous_p == NULL);
      const bool is_previous_data_desc = (!is_previous_undefined
                                          && previous_p->type == ECMA_PROPERTY_NAMEDDATA);
      const bool is_previous_accessor_desc = (!is_previous_undefined
                                              && previous_p->type == ECMA_PROPERTY_NAMEDACCESSOR);
      JERRY_ASSERT (is_previous_undefined || is_previous_data_desc || is_previous_accessor_desc);

      ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();
      {
        prop_desc.is_enumerable_defined = true;
        prop_desc.is_enumerable = true;

        prop_desc.is_configurable_defined = true;
        prop_desc.is_configurable = true;
      }

      if (type == OPCODE_META_TYPE_VARG_PROP_DATA)
      {
        prop_desc.is_value_defined = true;
        prop_desc.value = value_for_prop_desc;

        prop_desc.is_writable_defined = true;
        prop_desc.is_writable = true;

        if (!is_previous_undefined
            && ((is_previous_data_desc
                 && frame_ctx_p->is_strict)
                || is_previous_accessor_desc))
        {
          is_throw_syntax_error = true;
        }
      }
      else if (type == OPCODE_META_TYPE_VARG_PROP_GETTER)
      {
        prop_desc.is_get_defined = true;
        prop_desc.get_p = ecma_get_object_from_value (value_for_prop_desc);

        if (!is_previous_undefined
            && is_previous_data_desc)
        {
          is_throw_syntax_error = true;
        }
      }
      else
      {
        prop_desc.is_set_defined = true;
        prop_desc.set_p = ecma_get_object_from_value (value_for_prop_desc);

        if (!is_previous_undefined
            && is_previous_data_desc)
        {
          is_throw_syntax_error = true;
        }
      }

      /* The SyntaxError should be treated as an early error  */
      JERRY_ASSERT (!is_throw_syntax_error);

      ecma_completion_value_t define_prop_completion = ecma_op_object_define_own_property (obj_p,
                                                                                           prop_name_string_p,
                                                                                           &prop_desc,
                                                                                           false);
      JERRY_ASSERT (ecma_is_completion_value_normal_true (define_prop_completion)
                    || ecma_is_completion_value_normal_false (define_prop_completion));

      ecma_deref_ecma_string (prop_name_string_p);

      ECMA_FINALIZE (value_for_prop_desc);

      frame_ctx_p->pos++;
    }
    else
    {
      JERRY_ASSERT (ecma_is_completion_value_throw (evaluate_prop_completion));

      ret_value = evaluate_prop_completion;
    }
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    ret_value = set_variable_value (frame_ctx_p, obj_lit_oc, lhs_var_idx, ecma_make_object_value (obj_p));
  }
  else
  {
    JERRY_ASSERT (ecma_is_completion_value_throw (ret_value));
  }

  ecma_deref_object (obj_p);

  return ret_value;
} /* opfunc_obj_decl */
/**
 * 'Return with no expression' opcode handler.
 *
 * See also: ECMA-262 v5, 12.9
 *
 * @return completion value
 *         Returned value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_completion_value_t
opfunc_ret (vm_instr_t instr __attr_unused___, /**< instruction */
            vm_frame_ctx_t *frame_ctx_p __attr_unused___) /**< interpreter context */
{
  return ecma_make_return_completion_value (ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED));
} /* opfunc_ret */

/**
 * 'Return with expression' opcode handler.
 *
 * See also: ECMA-262 v5, 12.9
 *
 * @return completion value
 *         Returned value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_completion_value_t
opfunc_retval (vm_instr_t instr __attr_unused___, /**< instruction */
               vm_frame_ctx_t *frame_ctx_p __attr_unused___) /**< interpreter context */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (expr_val, get_variable_value (frame_ctx_p, instr.data.retval.ret_value, false), ret_value);

  ret_value = ecma_make_return_completion_value (ecma_copy_value (expr_val, true));

  ECMA_FINALIZE (expr_val);

  return ret_value;
} /* opfunc_retval */

/**
 * 'Property getter' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.1
 *           ECMA-262 v5, 11.13.1
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_prop_getter (vm_instr_t instr __attr_unused___, /**< instruction */
                    vm_frame_ctx_t *frame_ctx_p __attr_unused___) /**< interpreter context */
{
  const vm_idx_t lhs_var_idx = instr.data.prop_getter.lhs;
  const vm_idx_t base_var_idx = instr.data.prop_getter.obj;
  const vm_idx_t prop_name_var_idx = instr.data.prop_getter.prop;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (base_value,
                  get_variable_value (frame_ctx_p, base_var_idx, false),
                  ret_value);
  ECMA_TRY_CATCH (prop_name_value,
                  get_variable_value (frame_ctx_p, prop_name_var_idx, false),
                  ret_value);
  ECMA_TRY_CATCH (check_coercible_ret,
                  ecma_op_check_object_coercible (base_value),
                  ret_value);
  ECMA_TRY_CATCH (prop_name_str_value,
                  ecma_op_to_string (prop_name_value),
                  ret_value);

  ecma_string_t *prop_name_string_p = ecma_get_string_from_value (prop_name_str_value);
  ecma_reference_t ref = ecma_make_reference (base_value, prop_name_string_p, frame_ctx_p->is_strict);

  ECMA_TRY_CATCH (prop_value, ecma_op_get_value_object_base (ref), ret_value);

  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos, lhs_var_idx, prop_value);

  ECMA_FINALIZE (prop_value);

  ecma_free_reference (ref);

  ECMA_FINALIZE (prop_name_str_value);
  ECMA_FINALIZE (check_coercible_ret);
  ECMA_FINALIZE (prop_name_value);
  ECMA_FINALIZE (base_value);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_prop_getter */

/**
 * 'Property setter' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.1
 *           ECMA-262 v5, 11.13.1
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_prop_setter (vm_instr_t instr __attr_unused___, /**< instruction */
                    vm_frame_ctx_t *frame_ctx_p __attr_unused___) /**< interpreter context */
{
  const vm_idx_t base_var_idx = instr.data.prop_setter.obj;
  const vm_idx_t prop_name_var_idx = instr.data.prop_setter.prop;
  const vm_idx_t rhs_var_idx = instr.data.prop_setter.rhs;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (base_value,
                  get_variable_value (frame_ctx_p, base_var_idx, false),
                  ret_value);
  ECMA_TRY_CATCH (prop_name_value,
                  get_variable_value (frame_ctx_p, prop_name_var_idx, false),
                  ret_value);
  ECMA_TRY_CATCH (check_coercible_ret,
                  ecma_op_check_object_coercible (base_value),
                  ret_value);
  ECMA_TRY_CATCH (prop_name_str_value,
                  ecma_op_to_string (prop_name_value),
                  ret_value);

  ecma_string_t *prop_name_string_p = ecma_get_string_from_value (prop_name_str_value);
  ecma_reference_t ref = ecma_make_reference (base_value,
                                              prop_name_string_p,
                                              frame_ctx_p->is_strict);

  ECMA_TRY_CATCH (rhs_value, get_variable_value (frame_ctx_p, rhs_var_idx, false), ret_value);
  ret_value = ecma_op_put_value_object_base (ref, rhs_value);
  ECMA_FINALIZE (rhs_value);

  ecma_free_reference (ref);

  ECMA_FINALIZE (prop_name_str_value);
  ECMA_FINALIZE (check_coercible_ret);
  ECMA_FINALIZE (prop_name_value);
  ECMA_FINALIZE (base_value);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_prop_setter */

/**
 * 'Logical NOT Operator' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.9
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_logical_not (vm_instr_t instr, /**< instruction */
                    vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.logical_not.dst;
  const vm_idx_t right_var_idx = instr.data.logical_not.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);

  ecma_simple_value_t old_value = ECMA_SIMPLE_VALUE_TRUE;
  ecma_completion_value_t to_bool_value = ecma_op_to_boolean (right_value);

  if (ecma_is_value_true (ecma_get_completion_value_value (to_bool_value)))
  {
    old_value = ECMA_SIMPLE_VALUE_FALSE;
  }

  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos,
                                  dst_var_idx,
                                  ecma_make_simple_value (old_value));

  ECMA_FINALIZE (right_value);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_logical_not */

/**
 * 'With' opcode handler.
 *
 * See also: ECMA-262 v5, 12.10
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_with (vm_instr_t instr, /**< instruction */
             vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t expr_var_idx = instr.data.with.expr;
  const vm_idx_t block_end_oc_idx_1 = instr.data.with.oc_idx_1;
  const vm_idx_t block_end_oc_idx_2 = instr.data.with.oc_idx_2;
  const vm_instr_counter_t with_end_oc = (vm_instr_counter_t) (
    vm_calc_instr_counter_from_idx_idx (block_end_oc_idx_1, block_end_oc_idx_2) + frame_ctx_p->pos);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (expr_value,
                  get_variable_value (frame_ctx_p,
                                      expr_var_idx,
                                      false),
                  ret_value);
  ECMA_TRY_CATCH (obj_expr_value,
                  ecma_op_to_object (expr_value),
                  ret_value);

  frame_ctx_p->pos++;

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_expr_value);

  ecma_object_t *old_env_p = frame_ctx_p->lex_env_p;
  ecma_object_t *new_env_p = ecma_create_object_lex_env (old_env_p,
                                                         obj_p,
                                                         true);
  frame_ctx_p->lex_env_p = new_env_p;

#ifndef JERRY_NDEBUG
  vm_instr_t meta_opcode = vm_get_instr (frame_ctx_p->bytecode_header_p->instrs_p, with_end_oc);
  JERRY_ASSERT (meta_opcode.op_idx == VM_OP_META);
  JERRY_ASSERT (meta_opcode.data.meta.type == OPCODE_META_TYPE_END_WITH);
#endif /* !JERRY_NDEBUG */

  vm_run_scope_t run_scope_with = { frame_ctx_p->pos, with_end_oc };
  ecma_completion_value_t with_completion = vm_loop (frame_ctx_p, &run_scope_with);

  if (ecma_is_completion_value_empty (with_completion))
  {
    JERRY_ASSERT (frame_ctx_p->pos == with_end_oc);

    frame_ctx_p->pos++;
  }
  else
  {
    JERRY_ASSERT (ecma_is_completion_value_throw (with_completion)
                  || ecma_is_completion_value_return (with_completion)
                  || ecma_is_completion_value_jump (with_completion));
    JERRY_ASSERT (frame_ctx_p->pos <= with_end_oc);
  }

  ret_value = with_completion;

  frame_ctx_p->lex_env_p = old_env_p;

  ecma_deref_object (new_env_p);

  ECMA_FINALIZE (obj_expr_value);
  ECMA_FINALIZE (expr_value);

  return ret_value;
} /* opfunc_with */

/**
 * 'Throw' opcode handler.
 *
 * See also: ECMA-262 v5, 12.13
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_throw_value (vm_instr_t instr, /**< instruction */
                    vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t var_idx = instr.data.throw_value.var;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (var_value,
                  get_variable_value (frame_ctx_p,
                                      var_idx,
                                      false),
                  ret_value);

  ret_value = ecma_make_throw_completion_value (ecma_copy_value (var_value, true));

  ECMA_FINALIZE (var_value);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_throw_value */

/**
 * Evaluate argument of typeof.
 *
 * See also: ECMA-262 v5, 11.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
evaluate_arg_for_typeof (vm_frame_ctx_t *frame_ctx_p, /**< interpreter context */
                         vm_idx_t var_idx) /**< arg variable identifier */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (vm_is_reg_variable (var_idx))
  {
    // 2.b
    ret_value = get_variable_value (frame_ctx_p,
                                    var_idx,
                                    false);
    JERRY_ASSERT (ecma_is_completion_value_normal (ret_value));
  }
  else
  {
    lit_cpointer_t lit_cp = bc_get_literal_cp_by_uid (var_idx,
                                                      frame_ctx_p->bytecode_header_p,
                                                      frame_ctx_p->pos);
    JERRY_ASSERT (lit_cp.packed_value != MEM_CP_NULL);

    ecma_string_t *var_name_string_p = ecma_new_ecma_string_from_lit_cp (lit_cp);

    ecma_object_t *ref_base_lex_env_p = ecma_op_resolve_reference_base (frame_ctx_p->lex_env_p,
                                                                        var_name_string_p);
    if (ref_base_lex_env_p == NULL)
    {
      ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else
    {
      ret_value = ecma_op_get_value_lex_env_base (ref_base_lex_env_p,
                                                  var_name_string_p,
                                                  frame_ctx_p->is_strict);
    }

    ecma_deref_ecma_string (var_name_string_p);
  }

  return ret_value;
} /* evaluate_arg_for_typeof */

/**
 * 'typeof' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_typeof (vm_instr_t instr, /**< instruction */
               vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.typeof.lhs;
  const vm_idx_t obj_var_idx = instr.data.typeof.obj;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (typeof_arg,
                  evaluate_arg_for_typeof (frame_ctx_p,
                                           obj_var_idx),
                  ret_value);


  ecma_string_t *type_str_p = NULL;

  if (ecma_is_value_undefined (typeof_arg))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_UNDEFINED);
  }
  else if (ecma_is_value_null (typeof_arg))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_OBJECT);
  }
  else if (ecma_is_value_boolean (typeof_arg))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_BOOLEAN);
  }
  else if (ecma_is_value_number (typeof_arg))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_NUMBER);
  }
  else if (ecma_is_value_string (typeof_arg))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_STRING);
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_object (typeof_arg));

    if (ecma_op_is_callable (typeof_arg))
    {
      type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_FUNCTION);
    }
    else
    {
      type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_OBJECT);
    }
  }

  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos,
                                  dst_var_idx,
                                  ecma_make_string_value (type_str_p));

  ecma_deref_ecma_string (type_str_p);

  ECMA_FINALIZE (typeof_arg);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_typeof */

/**
 * 'delete' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_delete_var (vm_instr_t instr, /**< instruction */
                   vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.delete_var.lhs;
  const vm_idx_t name_lit_idx = instr.data.delete_var.name;
  const vm_instr_counter_t lit_oc = frame_ctx_p->pos;

  frame_ctx_p->pos++;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  lit_cpointer_t lit_cp = bc_get_literal_cp_by_uid (name_lit_idx, frame_ctx_p->bytecode_header_p, lit_oc);
  JERRY_ASSERT (lit_cp.packed_value != MEM_CP_NULL);

  ecma_string_t *name_string_p = ecma_new_ecma_string_from_lit_cp (lit_cp);

  ecma_reference_t ref = ecma_op_get_identifier_reference (frame_ctx_p->lex_env_p,
                                                           name_string_p,
                                                           frame_ctx_p->is_strict);

  if (ref.is_strict)
  {
    /* SyntaxError should be treated as an early error */
    JERRY_UNREACHABLE ();
  }
  else
  {
    if (ecma_is_value_undefined (ref.base))
    {
      ret_value = set_variable_value (frame_ctx_p, lit_oc,
                                      dst_var_idx,
                                      ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE));
    }
    else
    {
      ecma_object_t *bindings_p = ecma_get_object_from_value (ref.base);
      JERRY_ASSERT (ecma_is_lexical_environment (bindings_p));

      ECMA_TRY_CATCH (delete_completion,
                      ecma_op_delete_binding (bindings_p,
                                              ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                         ref.referenced_name_cp)),
                      ret_value);

      ret_value = set_variable_value (frame_ctx_p, lit_oc, dst_var_idx, delete_completion);

      ECMA_FINALIZE (delete_completion);
    }
  }

  ecma_free_reference (ref);

  ecma_deref_ecma_string (name_string_p);

  return ret_value;
} /* opfunc_delete_var */


/**
 * 'delete' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_delete_prop (vm_instr_t instr, /**< instruction */
                    vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.delete_prop.lhs;
  const vm_idx_t base_var_idx = instr.data.delete_prop.base;
  const vm_idx_t name_var_idx = instr.data.delete_prop.name;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (base_value,
                  get_variable_value (frame_ctx_p, base_var_idx, false),
                  ret_value);
  ECMA_TRY_CATCH (name_value,
                  get_variable_value (frame_ctx_p, name_var_idx, false),
                  ret_value);
  ECMA_TRY_CATCH (check_coercible_ret,
                  ecma_op_check_object_coercible (base_value),
                  ret_value);
  ECMA_TRY_CATCH (str_name_value,
                  ecma_op_to_string (name_value),
                  ret_value);

  JERRY_ASSERT (ecma_is_value_string (str_name_value));
  ecma_string_t *name_string_p = ecma_get_string_from_value (str_name_value);

  if (ecma_is_value_undefined (base_value))
  {
    if (frame_ctx_p->is_strict)
    {
      /* SyntaxError should be treated as an early error */
      JERRY_UNREACHABLE ();
    }
    else
    {
      ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
    }
  }
  else
  {
    ECMA_TRY_CATCH (obj_value, ecma_op_to_object (base_value), ret_value);

    JERRY_ASSERT (ecma_is_value_object (obj_value));
    ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);
    JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));

    ECMA_TRY_CATCH (delete_op_ret_val,
                    ecma_op_object_delete (obj_p, name_string_p, frame_ctx_p->is_strict),
                    ret_value);

    ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos, dst_var_idx, delete_op_ret_val);

    ECMA_FINALIZE (delete_op_ret_val);
    ECMA_FINALIZE (obj_value);
  }

  ECMA_FINALIZE (str_name_value);
  ECMA_FINALIZE (check_coercible_ret);
  ECMA_FINALIZE (name_value);
  ECMA_FINALIZE (base_value);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_delete_prop */

/**
 * 'meta' opcode handler.
 *
 * @return implementation-defined meta completion value
 */
ecma_completion_value_t
opfunc_meta (vm_instr_t instr, /**< instruction */
             vm_frame_ctx_t *frame_ctx_p __attr_unused___) /**< interpreter context */
{
  const opcode_meta_type type = (opcode_meta_type) instr.data.meta.type;

  switch (type)
  {
    case OPCODE_META_TYPE_VARG:
    case OPCODE_META_TYPE_VARG_PROP_DATA:
    case OPCODE_META_TYPE_VARG_PROP_GETTER:
    case OPCODE_META_TYPE_VARG_PROP_SETTER:
    case OPCODE_META_TYPE_END_WITH:
    case OPCODE_META_TYPE_CATCH:
    case OPCODE_META_TYPE_FINALLY:
    case OPCODE_META_TYPE_END_TRY_CATCH_FINALLY:
    case OPCODE_META_TYPE_END_FOR_IN:
    {
      return ecma_make_meta_completion_value ();
    }

    case OPCODE_META_TYPE_UNDEFINED:
    case OPCODE_META_TYPE_CALL_SITE_INFO:
    case OPCODE_META_TYPE_FUNCTION_END:
    case OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER:
    {
      JERRY_UNREACHABLE ();
    }
  }

  JERRY_UNREACHABLE ();
} /* opfunc_meta */

/**
 * Calculate instruction counter from 'meta' instruction's data arguments.
 *
 * @return instruction counter
 */
vm_instr_counter_t
vm_calc_instr_counter_from_idx_idx (const vm_idx_t oc_idx_1, /**< first idx */
                                    const vm_idx_t oc_idx_2) /**< second idx */
{
  vm_instr_counter_t counter;

  counter = oc_idx_1;
  counter = (vm_instr_counter_t) (counter << (sizeof (vm_idx_t) * JERRY_BITSINBYTE));
  counter = (vm_instr_counter_t) (counter | oc_idx_2);

  return counter;
} /* vm_calc_instr_counter_from_idx_idx */

/**
 * Read instruction counter from specified instruction,
 * that should be 'meta' instruction of specified type.
 */
vm_instr_counter_t
vm_read_instr_counter_from_meta (opcode_meta_type expected_type, /**< expected type of meta instruction */
                                 const bytecode_data_header_t *bytecode_header_p, /**< byte-code header */
                                 vm_instr_counter_t instr_pos) /**< position of the instruction */
{
  vm_instr_t meta_opcode = vm_get_instr (bytecode_header_p->instrs_p, instr_pos);
  JERRY_ASSERT (meta_opcode.data.meta.type == expected_type);

  const vm_idx_t data_1 = meta_opcode.data.meta.data_1;
  const vm_idx_t data_2 = meta_opcode.data.meta.data_2;

  return vm_calc_instr_counter_from_idx_idx (data_1, data_2);
} /* vm_read_instr_counter_from_meta */

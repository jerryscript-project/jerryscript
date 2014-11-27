/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#include "deserializer.h"
#include "globals.h"
#include "interpreter.h"
#include "opcodes.h"
#include "opcodes-ecma-support.h"

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
 */

#define OP_UNIMPLEMENTED_LIST(op) \
    static char __unused unimplemented_list_end

#define DEFINE_UNIMPLEMENTED_OP(op) \
  ecma_completion_value_t opfunc_ ## op (opcode_t opdata, int_data_t *int_data) \
  { \
    JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (opdata, int_data); \
  }

OP_UNIMPLEMENTED_LIST (DEFINE_UNIMPLEMENTED_OP);
#undef DEFINE_UNIMPLEMENTED_OP

/**
 * 'Nop' opcode handler.
 */
ecma_completion_value_t
opfunc_nop (opcode_t opdata __unused, /**< operation data */
            int_data_t *int_data) /**< interpreter context */
{
  int_data->pos++;

  return ecma_make_empty_completion_value ();
} /* opfunc_nop */

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
opfunc_assignment (opcode_t opdata, /**< operation data */
                   int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.assignment.var_left;
  const opcode_arg_type_operand type_value_right = opdata.data.assignment.type_value_right;
  const idx_t src_val_descr = opdata.data.assignment.value_right;

  int_data->pos++;

  ecma_completion_value_t get_value_completion;

  switch (type_value_right)
  {
    case OPCODE_ARG_TYPE_SIMPLE:
    {
      get_value_completion = ecma_make_simple_completion_value (src_val_descr);
      break;
    }
    case OPCODE_ARG_TYPE_STRING:
    {
      ecma_string_t *ecma_string_p = ecma_new_ecma_string_from_lit_index (src_val_descr);

      get_value_completion = ecma_make_normal_completion_value (ecma_make_string_value (ecma_string_p));
      break;
    }
    case OPCODE_ARG_TYPE_VARIABLE:
    {
      get_value_completion = get_variable_value (int_data,
                                                 src_val_descr,
                                                 false);

      break;
    }
    case OPCODE_ARG_TYPE_NUMBER:
    {
      ecma_number_t *num_p = ecma_alloc_number ();
      const literal lit = deserialize_literal_by_id (src_val_descr);
      JERRY_ASSERT (lit.type == LIT_NUMBER);
      *num_p = lit.data.num;

      get_value_completion = ecma_make_normal_completion_value (ecma_make_number_value (num_p));
      break;
    }
    case OPCODE_ARG_TYPE_SMALLINT:
    {
      ecma_number_t *num_p = ecma_alloc_number ();
      *num_p = src_val_descr;

      get_value_completion = ecma_make_normal_completion_value (ecma_make_number_value (num_p));
      break;
    }
  }

  if (unlikely (ecma_is_completion_value_throw (get_value_completion)))
  {
    return get_value_completion;
  }
  else
  {
    JERRY_ASSERT (ecma_is_completion_value_normal (get_value_completion));

    ecma_value_t value_to_assign = ecma_get_completion_value_value (get_value_completion);
    ecma_completion_value_t assignment_completion_value = set_variable_value (int_data,
                                                                              dst_var_idx,
                                                                              value_to_assign);

    ecma_free_completion_value (get_value_completion);

    return assignment_completion_value;
  }
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
opfunc_pre_incr (opcode_t opdata, /**< operation data */
                 int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.pre_incr.dst;
  const idx_t incr_var_idx = opdata.data.pre_incr.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  // 1., 2., 3.
  ECMA_TRY_CATCH (old_value, get_variable_value (int_data, incr_var_idx, true), ret_value);
  ECMA_TRY_CATCH (old_num_value, ecma_op_to_number (ecma_get_completion_value_value (old_value)), ret_value);

  // 4.
  ecma_number_t* new_num_p = ecma_alloc_number ();

  ecma_number_t* old_num_p = ecma_get_number_from_completion_value (old_num_value);
  *new_num_p = ecma_number_add (*old_num_p, ECMA_NUMBER_ONE);

  ecma_value_t new_num_value = ecma_make_number_value (new_num_p);

  // 5.
  ret_value = set_variable_value (int_data,
                                  incr_var_idx,
                                  new_num_value);

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (int_data,
                                                                   dst_var_idx,
                                                                   new_num_value);
  JERRY_ASSERT (ecma_is_completion_value_empty (reg_assignment_res));

  ecma_dealloc_number (new_num_p);

  ECMA_FINALIZE (old_num_value);
  ECMA_FINALIZE (old_value);

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
opfunc_pre_decr (opcode_t opdata, /**< operation data */
                 int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.pre_decr.dst;
  const idx_t decr_var_idx = opdata.data.pre_decr.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  // 1., 2., 3.
  ECMA_TRY_CATCH (old_value, get_variable_value (int_data, decr_var_idx, true), ret_value);
  ECMA_TRY_CATCH (old_num_value, ecma_op_to_number (ecma_get_completion_value_value (old_value)), ret_value);

  // 4.
  ecma_number_t* new_num_p = ecma_alloc_number ();

  ecma_number_t* old_num_p = ecma_get_number_from_completion_value (old_num_value);
  *new_num_p = ecma_number_substract (*old_num_p, ECMA_NUMBER_ONE);

  ecma_value_t new_num_value = ecma_make_number_value (new_num_p);

  // 5.
  ret_value = set_variable_value (int_data,
                                  decr_var_idx,
                                  new_num_value);

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (int_data,
                                                                   dst_var_idx,
                                                                   new_num_value);
  JERRY_ASSERT (ecma_is_completion_value_empty (reg_assignment_res));

  ecma_dealloc_number (new_num_p);

  ECMA_FINALIZE (old_num_value);
  ECMA_FINALIZE (old_value);

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
opfunc_post_incr (opcode_t opdata, /**< operation data */
                  int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.post_incr.dst;
  const idx_t incr_var_idx = opdata.data.post_incr.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  // 1., 2., 3.
  ECMA_TRY_CATCH (old_value, get_variable_value (int_data, incr_var_idx, true), ret_value);
  ECMA_TRY_CATCH (old_num_value, ecma_op_to_number (ecma_get_completion_value_value (old_value)), ret_value);

  // 4.
  ecma_number_t* new_num_p = ecma_alloc_number ();

  ecma_number_t* old_num_p = ecma_get_number_from_completion_value (old_num_value);
  *new_num_p = ecma_number_add (*old_num_p, ECMA_NUMBER_ONE);

  // 5.
  ret_value = set_variable_value (int_data,
                                  incr_var_idx,
                                  ecma_make_number_value (new_num_p));

  ecma_dealloc_number (new_num_p);

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (int_data,
                                                                   dst_var_idx,
                                                                   ecma_get_completion_value_value (old_num_value));
  JERRY_ASSERT (ecma_is_completion_value_empty (reg_assignment_res));

  ECMA_FINALIZE (old_num_value);
  ECMA_FINALIZE (old_value);

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
opfunc_post_decr (opcode_t opdata, /**< operation data */
                  int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.post_decr.dst;
  const idx_t decr_var_idx = opdata.data.post_decr.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  // 1., 2., 3.
  ECMA_TRY_CATCH (old_value, get_variable_value (int_data, decr_var_idx, true), ret_value);
  ECMA_TRY_CATCH (old_num_value, ecma_op_to_number (ecma_get_completion_value_value (old_value)), ret_value);

  // 4.
  ecma_number_t* new_num_p = ecma_alloc_number ();

  ecma_number_t* old_num_p = ecma_get_number_from_completion_value (old_num_value);
  *new_num_p = ecma_number_substract (*old_num_p, ECMA_NUMBER_ONE);

  // 5.
  ret_value = set_variable_value (int_data,
                                  decr_var_idx,
                                  ecma_make_number_value (new_num_p));

  ecma_dealloc_number (new_num_p);

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (int_data,
                                                                   dst_var_idx,
                                                                   ecma_get_completion_value_value (old_num_value));
  JERRY_ASSERT (ecma_is_completion_value_empty (reg_assignment_res));

  ECMA_FINALIZE (old_num_value);
  ECMA_FINALIZE (old_value);

  return ret_value;
} /* opfunc_post_decr */

/**
 * 'Register variable declaration' opcode handler.
 *
 * The opcode is meta-opcode that is not supposed to be executed.
 */
ecma_completion_value_t
opfunc_reg_var_decl (opcode_t opdata __unused, /**< operation data */
                     int_data_t *int_data __unused) /**< interpreter context */
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
opfunc_var_decl (opcode_t opdata, /**< operation data */
                 int_data_t *int_data) /**< interpreter context */
{
  ecma_string_t *var_name_string_p = ecma_new_ecma_string_from_lit_index (opdata.data.var_decl.variable_name);

  if (!ecma_op_has_binding (int_data->lex_env_p, var_name_string_p))
  {
    const bool is_configurable_bindings = int_data->is_eval_code;

    ecma_completion_value_t completion = ecma_op_create_mutable_binding (int_data->lex_env_p,
                                                                         var_name_string_p,
                                                                         is_configurable_bindings);

    JERRY_ASSERT (ecma_is_completion_value_empty (completion));

    /* Skipping SetMutableBinding as we have already checked that there were not
     * any binding with specified name in current lexical environment
     * and CreateMutableBinding sets the created binding's value to undefined */
    JERRY_ASSERT (ecma_is_completion_value_normal_simple_value (ecma_op_get_binding_value (int_data->lex_env_p,
                                                                                           var_name_string_p,
                                                                                           true),
                                                                ECMA_SIMPLE_VALUE_UNDEFINED));
  }

  ecma_deref_ecma_string (var_name_string_p);

  int_data->pos++;

  return ecma_make_empty_completion_value ();
} /* opfunc_var_decl */

/**
 * Function declaration helper
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
function_declaration (int_data_t *int_data, /**< interpreter context */
                      idx_t function_name_lit_idx, /**< identifier of literal with function name */
                      ecma_string_t* args_names[], /**< names of arguments */
                      ecma_length_t args_number) /**< number of arguments */
{
  bool is_strict = int_data->is_strict;
  const bool is_configurable_bindings = int_data->is_eval_code;

  const opcode_counter_t function_code_end_oc = (opcode_counter_t) (
    read_meta_opcode_counter (OPCODE_META_TYPE_FUNCTION_END, int_data) + int_data->pos);
  int_data->pos++;

  opcode_t next_opcode = read_opcode (int_data->pos);
  if (next_opcode.op_idx == __op__idx_meta
      && next_opcode.data.meta.type == OPCODE_META_TYPE_STRICT_CODE)
  {
    is_strict = true;

    int_data->pos++;
  }

  ecma_string_t *function_name_string_p = ecma_new_ecma_string_from_lit_index (function_name_lit_idx);

  ecma_completion_value_t ret_value = ecma_op_function_declaration (int_data->lex_env_p,
                                                                    function_name_string_p,
                                                                    int_data->pos,
                                                                    args_names,
                                                                    args_number,
                                                                    is_strict,
                                                                    is_configurable_bindings);
  ecma_deref_ecma_string (function_name_string_p);

  int_data->pos = function_code_end_oc;

  return ret_value;
} /* function_declaration */

/**
 * 'Function declaration' opcode handler.
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_func_decl_n (opcode_t opdata, /**< operation data */
                    int_data_t *int_data) /**< interpreter context */
{
  int_data->pos++;

  const idx_t function_name_idx = opdata.data.func_decl_n.name_lit_idx;
  const ecma_length_t params_number = opdata.data.func_decl_n.arg_list;

  ecma_string_t *params_names[params_number + 1 /* length of array should not be zero */];
  fill_params_list (int_data, params_number, params_names);

  ecma_completion_value_t ret_value = function_declaration (int_data,
                                                            function_name_idx,
                                                            params_names,
                                                            params_number);

  for (uint32_t param_index = 0;
       param_index < params_number;
       param_index++)
  {
    ecma_deref_ecma_string (params_names[param_index]);
  }

  return ret_value;
} /* opfunc_func_decl_n */

/**
 * 'Function expression' opcode handler.
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_func_expr_n (opcode_t opdata, /**< operation data */
                    int_data_t *int_data) /**< interpreter context */
{
  int_data->pos++;

  const idx_t dst_var_idx = opdata.data.func_expr_n.lhs;
  const idx_t function_name_lit_idx = opdata.data.func_expr_n.name_lit_idx;
  const ecma_length_t params_number = opdata.data.func_expr_n.arg_list;
  const bool is_named_func_expr = (!is_reg_variable (int_data, function_name_lit_idx));

  ecma_string_t *params_names[params_number + 1 /* length of array should not be zero */];
  fill_params_list (int_data, params_number, params_names);

  bool is_strict = int_data->is_strict;

  const opcode_counter_t function_code_end_oc = (opcode_counter_t) (
    read_meta_opcode_counter (OPCODE_META_TYPE_FUNCTION_END, int_data) + int_data->pos);
  int_data->pos++;

  opcode_t next_opcode = read_opcode (int_data->pos);
  if (next_opcode.op_idx == __op__idx_meta
      && next_opcode.data.meta.type == OPCODE_META_TYPE_STRICT_CODE)
  {
    is_strict = true;

    int_data->pos++;
  }

  ecma_object_t *scope_p;
  ecma_string_t *function_name_string_p = NULL;
  if (is_named_func_expr)
  {
    scope_p = ecma_create_decl_lex_env (int_data->lex_env_p);
    function_name_string_p = ecma_new_ecma_string_from_lit_index (function_name_lit_idx);
    ecma_op_create_immutable_binding (scope_p,
                                      function_name_string_p);
  }
  else
  {
    scope_p = int_data->lex_env_p;
    ecma_ref_object (scope_p);
  }

  ecma_object_t *func_obj_p = ecma_op_create_function_object (params_names,
                                                              params_number,
                                                              scope_p,
                                                              is_strict,
                                                              int_data->pos);

  ecma_completion_value_t ret_value = set_variable_value (int_data,
                                                          dst_var_idx,
                                                          ecma_make_object_value (func_obj_p));

  if (is_named_func_expr)
  {
    ecma_op_initialize_immutable_binding (scope_p,
                                          function_name_string_p,
                                          ecma_make_object_value (func_obj_p));
    ecma_deref_ecma_string (function_name_string_p);
  }

  ecma_deref_object (func_obj_p);
  ecma_deref_object (scope_p);

  for (uint32_t param_index = 0;
       param_index < params_number;
       param_index++)
  {
    ecma_deref_ecma_string (params_names[param_index]);
  }

  int_data->pos = function_code_end_oc;

  return ret_value;
} /* opfunc_func_expr_n */

/**
 * 'Function call' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_call_n (opcode_t opdata, /**< operation data */
               int_data_t *int_data) /**< interpreter context */
{
  const idx_t lhs_var_idx = opdata.data.call_n.lhs;
  const idx_t func_name_lit_idx = opdata.data.call_n.name_lit_idx;
  const idx_t args_number_idx = opdata.data.call_n.arg_list;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (func_value, get_variable_value (int_data, func_name_lit_idx, false), ret_value);

  bool this_arg_var_idx_set = false;
  idx_t this_arg_var_idx;
  idx_t args_number;

  opcode_t next_opcode = read_opcode (int_data->pos);
  if (next_opcode.op_idx == __op__idx_meta
      && next_opcode.data.meta.type == OPCODE_META_TYPE_THIS_ARG)
  {
    this_arg_var_idx = next_opcode.data.meta.data_1;
    JERRY_ASSERT (is_reg_variable (int_data, this_arg_var_idx));

    this_arg_var_idx_set = true;

    JERRY_ASSERT (args_number_idx > 0);
    args_number = (idx_t) (args_number_idx - 1);

    int_data->pos++;
  }
  else
  {
    args_number = args_number_idx;
  }

  ecma_value_t arg_values[args_number + 1 /* length of array should not be zero */];

  ecma_length_t args_read;
  ecma_completion_value_t get_arg_completion = fill_varg_list (int_data,
                                                               args_number,
                                                               arg_values,
                                                               &args_read);

  if (ecma_is_completion_value_empty (get_arg_completion))
  {
    JERRY_ASSERT (args_read == args_number);

    ecma_completion_value_t this_value;

    if (this_arg_var_idx_set)
    {
      this_value = get_variable_value (int_data, this_arg_var_idx, false);
    }
    else
    {
      this_value = ecma_op_implicit_this_value (int_data->lex_env_p);
    }
    JERRY_ASSERT (ecma_is_completion_value_normal (this_value));

    if (!ecma_op_is_callable (ecma_get_completion_value_value (func_value)))
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      ecma_object_t *func_obj_p = ecma_get_object_from_completion_value (func_value);

      ECMA_TRY_CATCH (call_completion,
                      ecma_op_function_call (func_obj_p,
                                             ecma_get_completion_value_value (this_value),
                                             arg_values,
                                             args_number),
                      ret_value);

      ret_value = set_variable_value (int_data,
                                      lhs_var_idx,
                                      ecma_get_completion_value_value (call_completion));

      ECMA_FINALIZE (call_completion);

    }

    ecma_free_completion_value (this_value);
  }
  else
  {
    JERRY_ASSERT (!ecma_is_completion_value_normal (get_arg_completion));

    ret_value = get_arg_completion;
  }

  for (ecma_length_t arg_index = 0;
       arg_index < args_read;
       arg_index++)
  {
    ecma_free_value (arg_values[arg_index], true);
  }

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
opfunc_construct_n (opcode_t opdata, /**< operation data */
                    int_data_t *int_data) /**< interpreter context */
{
  const idx_t lhs_var_idx = opdata.data.construct_n.lhs;
  const idx_t constructor_name_lit_idx = opdata.data.construct_n.name_lit_idx;
  const idx_t args_number = opdata.data.construct_n.arg_list;

  int_data->pos++;

  ecma_completion_value_t ret_value;
  ECMA_TRY_CATCH (constructor_value, get_variable_value (int_data, constructor_name_lit_idx, false), ret_value);

  ecma_value_t arg_values[args_number + 1 /* length of array should not be zero */];

  ecma_length_t args_read;
  ecma_completion_value_t get_arg_completion = fill_varg_list (int_data,
                                                               args_number,
                                                               arg_values,
                                                               &args_read);

  if (ecma_is_completion_value_empty (get_arg_completion))
  {
    JERRY_ASSERT (args_read == args_number);

    if (!ecma_is_constructor (ecma_get_completion_value_value (constructor_value)))
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      ecma_object_t *constructor_obj_p = ecma_get_object_from_completion_value (constructor_value);

      ECMA_TRY_CATCH (construction_completion,
                      ecma_op_function_construct (constructor_obj_p,
                                                  arg_values,
                                                  args_number),
                      ret_value);

      ret_value = set_variable_value (int_data, lhs_var_idx, ecma_get_completion_value_value (construction_completion));

      ECMA_FINALIZE (construction_completion);
    }
  }
  else
  {
    JERRY_ASSERT (!ecma_is_completion_value_normal (get_arg_completion));

    ret_value = get_arg_completion;
  }

  for (ecma_length_t arg_index = 0;
       arg_index < args_read;
       arg_index++)
  {
    ecma_free_value (arg_values[arg_index], true);
  }

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
opfunc_array_decl (opcode_t opdata, /**< operation data */
                   int_data_t *int_data) /**< interpreter context */
{
  const idx_t lhs_var_idx = opdata.data.array_decl.lhs;
  const idx_t args_number = opdata.data.array_decl.list;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ecma_value_t arg_values[args_number + 1 /* length of array should not be zero */];

  ecma_length_t args_read;
  ecma_completion_value_t get_arg_completion = fill_varg_list (int_data,
                                                               args_number,
                                                               arg_values,
                                                               &args_read);

  if (ecma_is_completion_value_empty (get_arg_completion))
  {
    JERRY_ASSERT (args_read == args_number);

    ECMA_TRY_CATCH (array_obj_value,
                    ecma_op_create_array_object (arg_values,
                                                 args_number,
                                                 false),
                    ret_value);

    ret_value = set_variable_value (int_data,
                                    lhs_var_idx,
                                    ecma_get_completion_value_value (array_obj_value));

    ECMA_FINALIZE (array_obj_value);
  }
  else
  {
    JERRY_ASSERT (!ecma_is_completion_value_normal (get_arg_completion));

    ret_value = get_arg_completion;
  }

  for (ecma_length_t arg_index = 0;
       arg_index < args_read;
       arg_index++)
  {
    ecma_free_value (arg_values[arg_index], true);
  }

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
opfunc_obj_decl (opcode_t opdata, /**< operation data */
                 int_data_t *int_data) /**< interpreter context */
{
  const idx_t lhs_var_idx = opdata.data.obj_decl.lhs;
  const idx_t args_number = opdata.data.obj_decl.list;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ecma_completion_value_t completion = ecma_make_empty_completion_value ();
  ecma_object_t *obj_p = ecma_op_create_object_object_noarg ();

  for (uint32_t prop_index = 0;
       prop_index < args_number;
       prop_index++)
  {
    ecma_completion_value_t evaluate_prop_completion = run_int_loop (int_data);

    if (ecma_is_completion_value_normal (evaluate_prop_completion))
    {
      JERRY_ASSERT (ecma_is_completion_value_empty (evaluate_prop_completion));

      opcode_t next_opcode = read_opcode (int_data->pos);
      JERRY_ASSERT (next_opcode.op_idx == __op__idx_meta);

      const opcode_meta_type type = next_opcode.data.meta.type;
      JERRY_ASSERT (type == OPCODE_META_TYPE_VARG_PROP_DATA
                    || type == OPCODE_META_TYPE_VARG_PROP_GETTER
                    || type == OPCODE_META_TYPE_VARG_PROP_SETTER);

      const idx_t prop_name_var_idx = next_opcode.data.meta.data_1;
      const idx_t value_for_prop_desc_var_idx = next_opcode.data.meta.data_2;

      ecma_completion_value_t value_for_prop_desc = get_variable_value (int_data,
                                                                        value_for_prop_desc_var_idx,
                                                                        false);

      if (ecma_is_completion_value_normal (value_for_prop_desc))
      {
        JERRY_ASSERT (is_reg_variable (int_data, prop_name_var_idx));

        ECMA_TRY_CATCH (prop_name_value,
                        get_variable_value (int_data,
                                            prop_name_var_idx,
                                            false),
                        ret_value);
        ECMA_TRY_CATCH (prop_name_str_value,
                        ecma_op_to_string (ecma_get_completion_value_value (prop_name_value)),
                        ret_value);

        bool is_throw_syntax_error = false;

        ecma_string_t *prop_name_string_p = ecma_get_string_from_completion_value (prop_name_str_value);
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
          prop_desc.enumerable = ECMA_PROPERTY_ENUMERABLE;

          prop_desc.is_configurable_defined = true;
          prop_desc.configurable = ECMA_PROPERTY_CONFIGURABLE;
        }

        if (type == OPCODE_META_TYPE_VARG_PROP_DATA)
        {
          prop_desc.is_value_defined = true;
          prop_desc.value = ecma_get_completion_value_value (value_for_prop_desc);
          
          prop_desc.is_writable_defined = true;
          prop_desc.writable = ECMA_PROPERTY_WRITABLE;

          if (!is_previous_undefined
              && ((is_previous_data_desc
                   && int_data->is_strict)
                  || is_previous_accessor_desc))
          {
            is_throw_syntax_error = true;
          }
        }
        else if (type == OPCODE_META_TYPE_VARG_PROP_GETTER)
        {
          prop_desc.is_get_defined = true;
          prop_desc.get_p = ecma_get_object_from_completion_value (value_for_prop_desc);

          if (!is_previous_undefined
              && is_previous_data_desc)
          {
            is_throw_syntax_error = true;
          }
        }
        else
        {
          prop_desc.is_set_defined = true;
          prop_desc.set_p = ecma_get_object_from_completion_value (value_for_prop_desc);

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
                                                                                             prop_desc,
                                                                                             false);
        JERRY_ASSERT (ecma_is_completion_value_normal_true (define_prop_completion)
                      || ecma_is_completion_value_normal_false (define_prop_completion));

        ecma_free_completion_value (value_for_prop_desc);

        ECMA_FINALIZE (prop_name_str_value);
        ECMA_FINALIZE (prop_name_value);
      }
      else
      {
        completion = value_for_prop_desc;

        break;
      }

      int_data->pos++;
    }
    else
    {
      JERRY_ASSERT (!ecma_is_completion_value_normal (evaluate_prop_completion));

      completion = evaluate_prop_completion;

      break;
    }
  }

  if (ecma_is_completion_value_empty (completion))
  {
    ret_value = set_variable_value (int_data, lhs_var_idx, ecma_make_object_value (obj_p));
  }
  else
  {
    ret_value = completion;
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
opfunc_ret (opcode_t opdata __unused, /**< operation data */
            int_data_t *int_data __unused) /**< interpreter context */
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
opfunc_retval (opcode_t opdata __unused, /**< operation data */
               int_data_t *int_data __unused) /**< interpreter context */
{
  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (expr_val, get_variable_value (int_data, opdata.data.retval.ret_value, false), ret_value);

  ret_value = ecma_make_return_completion_value (ecma_copy_value (ecma_get_completion_value_value (expr_val), true));

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
opfunc_prop_getter (opcode_t opdata __unused, /**< operation data */
                    int_data_t *int_data __unused) /**< interpreter context */
{
  const idx_t lhs_var_idx = opdata.data.prop_getter.lhs;
  const idx_t base_var_idx = opdata.data.prop_getter.obj;
  const idx_t prop_name_var_idx = opdata.data.prop_getter.prop;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (base_value,
                  get_variable_value (int_data, base_var_idx, false),
                  ret_value);
  ECMA_TRY_CATCH (prop_name_value,
                  get_variable_value (int_data, prop_name_var_idx, false),
                  ret_value);
  ECMA_TRY_CATCH (check_coercible_ret,
                  ecma_op_check_object_coercible (ecma_get_completion_value_value (base_value)),
                  ret_value);
  ECMA_TRY_CATCH (prop_name_str_value,
                  ecma_op_to_string (ecma_get_completion_value_value (prop_name_value)),
                  ret_value);

  ecma_string_t *prop_name_string_p = ecma_get_string_from_completion_value (prop_name_str_value);
  ecma_reference_t ref = ecma_make_reference (ecma_get_completion_value_value (base_value),
                                              prop_name_string_p,
                                              int_data->is_strict);

  ECMA_TRY_CATCH (prop_value, ecma_op_get_value_object_base (ref), ret_value);

  ret_value = set_variable_value (int_data, lhs_var_idx, ecma_get_completion_value_value (prop_value));

  ECMA_FINALIZE (prop_value);

  ecma_free_reference (ref);

  ECMA_FINALIZE (prop_name_str_value);
  ECMA_FINALIZE (check_coercible_ret);
  ECMA_FINALIZE (prop_name_value);
  ECMA_FINALIZE (base_value);

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
opfunc_prop_setter (opcode_t opdata __unused, /**< operation data */
                    int_data_t *int_data __unused) /**< interpreter context */
{
  const idx_t base_var_idx = opdata.data.prop_setter.obj;
  const idx_t prop_name_var_idx = opdata.data.prop_setter.prop;
  const idx_t rhs_var_idx = opdata.data.prop_setter.rhs;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (base_value,
                  get_variable_value (int_data, base_var_idx, false),
                  ret_value);
  ECMA_TRY_CATCH (prop_name_value,
                  get_variable_value (int_data, prop_name_var_idx, false),
                  ret_value);
  ECMA_TRY_CATCH (check_coercible_ret,
                  ecma_op_check_object_coercible (ecma_get_completion_value_value (base_value)),
                  ret_value);
  ECMA_TRY_CATCH (prop_name_str_value,
                  ecma_op_to_string (ecma_get_completion_value_value (prop_name_value)),
                  ret_value);

  ecma_string_t *prop_name_string_p = ecma_get_string_from_completion_value (prop_name_str_value);
  ecma_reference_t ref = ecma_make_reference (ecma_get_completion_value_value (base_value),
                                              prop_name_string_p,
                                              int_data->is_strict);

  ECMA_TRY_CATCH (rhs_value, get_variable_value (int_data, rhs_var_idx, false), ret_value);
  ret_value = ecma_op_put_value_object_base (ref, ecma_get_completion_value_value (rhs_value));
  ECMA_FINALIZE (rhs_value);

  ecma_free_reference (ref);

  ECMA_FINALIZE (prop_name_str_value);
  ECMA_FINALIZE (check_coercible_ret);
  ECMA_FINALIZE (prop_name_value);
  ECMA_FINALIZE (base_value);

  return ret_value;
} /* opfunc_prop_setter */

/**
 * Exit from script with specified status code:
 *   0 - for successful completion
 *   1 - to indicate failure.
 *
 * Note: this is not ECMA specification-defined, but internal
 *       implementation-defined opcode for end of script
 *       and assertions inside of unit tests.
 *
 * @return completion value
 *         Returned value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_completion_value_t
opfunc_exitval (opcode_t opdata, /**< operation data */
                int_data_t *int_data __unused) /**< interpreter context */
{
  JERRY_ASSERT (opdata.data.exitval.status_code == 0
                || opdata.data.exitval.status_code == 1);

  return ecma_make_exit_completion_value (opdata.data.exitval.status_code == 0);
} /* opfunc_exitval */

/**
 * 'Logical NOT Operator' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.9
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_logical_not (opcode_t opdata, /**< operation data */
                    int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.logical_not.dst;
  const idx_t right_var_idx = opdata.data.logical_not.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ecma_simple_value_t old_value = ECMA_SIMPLE_VALUE_TRUE;
  ecma_completion_value_t to_bool_value = ecma_op_to_boolean (ecma_get_completion_value_value (right_value));

  if (ecma_is_value_true (ecma_get_completion_value_value (to_bool_value)))
  {
    old_value = ECMA_SIMPLE_VALUE_FALSE;
  }

  ret_value = set_variable_value (int_data,
                                  dst_var_idx,
                                  ecma_make_simple_value (old_value));

  ECMA_FINALIZE (right_value);

  return ret_value;
} /* opfunc_logical_not */

/**
 * 'This' opcode handler.
 *
 * See also: ECMA-262 v5, 11.1.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_this (opcode_t opdata, /**< operation data */
             int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.this.lhs;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ret_value = set_variable_value (int_data,
                                  dst_var_idx,
                                  int_data->this_binding);

  return ret_value;
} /* opfunc_this */

/**
 * 'With' opcode handler.
 *
 * See also: ECMA-262 v5, 12.10
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_with (opcode_t opdata, /**< operation data */
             int_data_t *int_data) /**< interpreter context */
{
  const idx_t expr_var_idx = opdata.data.with.expr;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (expr_value,
                  get_variable_value (int_data,
                                      expr_var_idx,
                                      false),
                  ret_value);
  ECMA_TRY_CATCH (obj_expr_value,
                  ecma_op_to_object (ecma_get_completion_value_value (expr_value)),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_completion_value (obj_expr_value);

  ecma_object_t *old_env_p = int_data->lex_env_p;
  ecma_object_t *new_env_p = ecma_create_object_lex_env (old_env_p,
                                                         obj_p,
                                                         true);
  int_data->lex_env_p = new_env_p;

  ecma_completion_value_t evaluation_completion = run_int_loop (int_data);

  if (ecma_is_completion_value_normal (evaluation_completion))
  {
    JERRY_ASSERT (ecma_is_completion_value_empty (evaluation_completion));

    opcode_t meta_opcode = read_opcode (int_data->pos);
    JERRY_ASSERT (meta_opcode.op_idx == __op__idx_meta);
    JERRY_ASSERT (meta_opcode.data.meta.type == OPCODE_META_TYPE_END_WITH);

    int_data->pos++;

    ret_value = ecma_make_empty_completion_value ();
  }
  else
  {
    ret_value = evaluation_completion;
  }

  int_data->lex_env_p = old_env_p;

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
opfunc_throw (opcode_t opdata, /**< operation data */
              int_data_t *int_data) /**< interpreter context */
{
  const idx_t var_idx = opdata.data.throw.var;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (var_value,
                  get_variable_value (int_data,
                                      var_idx,
                                      false),
                  ret_value);

  ret_value = ecma_make_throw_completion_value (ecma_copy_value (ecma_get_completion_value_value (var_value), true));

  ECMA_FINALIZE (var_value);

  return ret_value;
} /* opfunc_throw */

/**
 * Evaluate argument of typeof.
 *
 * See also: ECMA-262 v5, 11.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
evaluate_arg_for_typeof (int_data_t *int_data, /**< interpreter context */
                         idx_t var_idx) /**< arg variable identifier */
{
  ecma_completion_value_t ret_value;

  if (is_reg_variable (int_data, var_idx))
  {
    // 2.b
    ret_value = get_variable_value (int_data,
                                    var_idx,
                                    false);
    JERRY_ASSERT (ecma_is_completion_value_normal (ret_value));
  }
  else
  {
    ecma_string_t *var_name_string_p = ecma_new_ecma_string_from_lit_index (var_idx);

    ecma_object_t *ref_base_lex_env_p = ecma_op_resolve_reference_base (int_data->lex_env_p,
                                                                        var_name_string_p);
    if (ref_base_lex_env_p == NULL)
    {
      ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else
    {
      ret_value = ecma_op_get_value_lex_env_base (ref_base_lex_env_p,
                                                  var_name_string_p,
                                                  int_data->is_strict);
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
opfunc_typeof (opcode_t opdata, /**< operation data */
               int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.typeof.lhs;
  const idx_t obj_var_idx = opdata.data.typeof.obj;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (typeof_evaluate_arg_completion,
                  evaluate_arg_for_typeof (int_data,
                                           obj_var_idx),
                  ret_value);

  ecma_value_t typeof_arg = ecma_get_completion_value_value (typeof_evaluate_arg_completion);

  ecma_string_t *type_str_p = NULL;

  if (ecma_is_value_undefined (typeof_arg))
  {
    type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_UNDEFINED);
  }
  else if (ecma_is_value_null (typeof_arg))
  {
    type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_OBJECT);
  }
  else if (ecma_is_value_boolean (typeof_arg))
  {
    type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_BOOLEAN);
  }
  else if (ecma_is_value_number (typeof_arg))
  {
    type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_NUMBER);
  }
  else if (ecma_is_value_string (typeof_arg))
  {
    type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_STRING);
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_object (typeof_arg));

    if (ecma_op_is_callable (typeof_arg))
    {
      type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_FUNCTION);
    }
    else
    {
      type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_OBJECT);
    }
  }

  ret_value = set_variable_value (int_data,
                                  dst_var_idx,
                                  ecma_make_string_value (type_str_p));

  ecma_deref_ecma_string (type_str_p);

  ECMA_FINALIZE (typeof_evaluate_arg_completion);

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
opfunc_delete_var (opcode_t opdata, /**< operation data */
                   int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.delete_var.lhs;
  const idx_t name_lit_idx = opdata.data.delete_var.name;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ecma_string_t *name_string_p = ecma_new_ecma_string_from_lit_index (name_lit_idx);

  ecma_reference_t ref = ecma_op_get_identifier_reference (int_data->lex_env_p,
                                                           name_string_p,
                                                           int_data->is_strict);

  if (ref.is_strict)
  {
    /* SyntaxError should be treated as an early error */
    JERRY_UNREACHABLE ();
  }
  else
  {
    if (ecma_is_value_undefined (ref.base))
    {
      ret_value = set_variable_value (int_data,
                                      dst_var_idx,
                                      ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE));
    }
    else
    {
      JERRY_ASSERT (ecma_is_value_object (ref.base));
      ecma_object_t *bindings_p = ECMA_GET_NON_NULL_POINTER (ref.base.value);
      JERRY_ASSERT (ecma_is_lexical_environment (bindings_p));

      ECMA_TRY_CATCH (delete_completion,
                      ecma_op_delete_binding (bindings_p,
                                              ECMA_GET_NON_NULL_POINTER (ref.referenced_name_cp)),
                      ret_value);

      ret_value = set_variable_value (int_data, dst_var_idx, ecma_get_completion_value_value (delete_completion));

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
opfunc_delete_prop (opcode_t opdata, /**< operation data */
                    int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.delete_prop.lhs;
  const idx_t base_var_idx = opdata.data.delete_prop.base;
  const idx_t name_var_idx = opdata.data.delete_prop.name;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (base_value,
                  get_variable_value (int_data, base_var_idx, false),
                  ret_value);
  ECMA_TRY_CATCH (name_value,
                  get_variable_value (int_data, name_var_idx, false),
                  ret_value);
  ECMA_TRY_CATCH (check_coercible_ret,
                  ecma_op_check_object_coercible (ecma_get_completion_value_value (base_value)),
                  ret_value);
  ECMA_TRY_CATCH (str_name_value,
                  ecma_op_to_string (ecma_get_completion_value_value (name_value)),
                  ret_value);

  JERRY_ASSERT (ecma_is_value_string (ecma_get_completion_value_value (str_name_value)));
  ecma_string_t *name_string_p = ecma_get_string_from_completion_value (str_name_value);

  if (ecma_is_value_undefined (ecma_get_completion_value_value (base_value)))
  {
    if (int_data->is_strict)
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
    ECMA_TRY_CATCH (obj_value, ecma_op_to_object (ecma_get_completion_value_value (base_value)), ret_value);

    JERRY_ASSERT (ecma_is_value_object (ecma_get_completion_value_value (obj_value)));
    ecma_object_t *obj_p = ecma_get_object_from_completion_value (obj_value);
    JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));

    ECMA_TRY_CATCH (delete_op_completion,
                    ecma_op_object_delete (obj_p, name_string_p, int_data->is_strict),
                    ret_value);

    ret_value = set_variable_value (int_data, dst_var_idx, ecma_get_completion_value_value (delete_op_completion));

    ECMA_FINALIZE (delete_op_completion);
    ECMA_FINALIZE (obj_value);
  }

  ECMA_FINALIZE (str_name_value);
  ECMA_FINALIZE (check_coercible_ret);
  ECMA_FINALIZE (name_value);
  ECMA_FINALIZE (base_value);

  return ret_value;
} /* opfunc_delete_prop */

/**
 * 'meta' opcode handler.
 *
 * @return implementation-defined meta completion value
 */
ecma_completion_value_t
opfunc_meta (opcode_t opdata, /**< operation data */
             int_data_t *int_data __unused) /**< interpreter context */
{
  const opcode_meta_type type = opdata.data.meta.type;

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
    {
      return ecma_make_meta_completion_value ();
    }

    case OPCODE_META_TYPE_STRICT_CODE:
    {
      FIXME (/* Handle in run_int_from_pos */);
      return ecma_make_meta_completion_value ();
    }

    case OPCODE_META_TYPE_UNDEFINED:
    case OPCODE_META_TYPE_THIS_ARG:
    case OPCODE_META_TYPE_FUNCTION_END:
    case OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER:
    {
      JERRY_UNREACHABLE ();
    }
  }

  JERRY_UNREACHABLE ();
} /* opfunc_meta */

/**
 * Calculate opcode counter from 'meta' opcode's data arguments.
 *
 * @return opcode counter
 */
opcode_counter_t
calc_opcode_counter_from_idx_idx (const idx_t oc_idx_1, /**< first idx */
                                  const idx_t oc_idx_2) /**< second idx */
{
  opcode_counter_t counter;

  counter = oc_idx_1;
  counter = (opcode_counter_t) (counter << (sizeof (idx_t) * JERRY_BITSINBYTE));
  counter = (opcode_counter_t) (counter | oc_idx_2);
  
  return counter;
} /* calc_meta_opcode_counter_from_meta_data */

/**
 * Read opcode counter from current opcode,
 * that should be 'meta' opcode of type 'opcode counter'.
 */
opcode_counter_t
read_meta_opcode_counter (opcode_meta_type expected_type, /**< expected type of meta opcode */
                          int_data_t *int_data) /**< interpreter context */
{
  opcode_t meta_opcode = read_opcode (int_data->pos);
  JERRY_ASSERT (meta_opcode.data.meta.type == expected_type);

  const idx_t data_1 = meta_opcode.data.meta.data_1;
  const idx_t data_2 = meta_opcode.data.meta.data_2;

  return calc_opcode_counter_from_idx_idx (data_1, data_2);
} /* read_meta_opcode_counter */

#define GETOP_DEF_1(a, name, field1) \
        opcode_t getop_##name (idx_t arg1) \
        { \
          opcode_t opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          return opdata; \
        }

#define GETOP_DEF_2(a, name, field1, field2) \
        opcode_t getop_##name (idx_t arg1, idx_t arg2) \
        { \
          opcode_t opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          opdata.data.name.field2 = arg2; \
          return opdata; \
        }

#define GETOP_DEF_3(a, name, field1, field2, field3) \
        opcode_t getop_##name (idx_t arg1, idx_t arg2, idx_t arg3) \
        { \
          opcode_t opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          opdata.data.name.field2 = arg2; \
          opdata.data.name.field3 = arg3; \
          return opdata; \
        }

OP_ARGS_LIST (GETOP_DEF)

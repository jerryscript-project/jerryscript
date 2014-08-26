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

#include "opcodes-ecma-support.h"

#include "globals.h"
#include "interpreter.h"
#include "jerry-libc.h"
#include "mem-heap.h"
#include "opcodes.h"

#include "actuators.h"
#include "common-io.h"
#include "sensors.h"
#include "ecma-objects.h"

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

/**
 * String literal copy descriptor.
 */
typedef struct
{
  ecma_char_t *str_p; /**< pointer to copied string literal */
  ecma_char_t literal_copy[32]; /**< buffer with string literal,
                                     if it is stored locally
                                     (i.e. not in the heap) */
} string_literal_copy;

/**
 * Initialize string literal copy.
 */
static void
init_string_literal_copy (idx_t idx, /**< literal identifier */
                          string_literal_copy *str_lit_descr_p) /**< pointer to string literal copy descriptor */
{
  JERRY_ASSERT (str_lit_descr_p != NULL);

  ssize_t sz = try_get_string_by_idx (idx,
                                      str_lit_descr_p->literal_copy,
                                      sizeof (str_lit_descr_p->literal_copy));
  if (sz > 0)
  {
    str_lit_descr_p->str_p = str_lit_descr_p->literal_copy;
  }
  else
  {
    JERRY_ASSERT (sz < 0);

    ssize_t req_sz = -sz;

    str_lit_descr_p->str_p = mem_heap_alloc_block ((size_t) req_sz,
                                                   MEM_HEAP_ALLOC_SHORT_TERM);

    sz = try_get_string_by_idx (idx,
                                str_lit_descr_p->str_p,
                                req_sz);

    JERRY_ASSERT (sz > 0);
  }
} /* init_string_literal */

/**
 * Free string literal copy.
 */
static void
free_string_literal_copy (string_literal_copy *str_lit_descr_p) /**< string literal copy descriptor */
{
  JERRY_ASSERT (str_lit_descr_p != NULL);
  JERRY_ASSERT (str_lit_descr_p->str_p != NULL);

  if (str_lit_descr_p->str_p == str_lit_descr_p->literal_copy)
  {
    /* copy is inside descriptor */
  }
  else
  {
    mem_heap_free_block (str_lit_descr_p->str_p);
  }

  str_lit_descr_p->str_p = NULL;

  return;
} /* free_string_literal */

#define OP_UNIMPLEMENTED_LIST(op) \
    op (native_call)                     \
    op (array_decl)                      \
    op (prop)                            \
    op (prop_get_decl)                   \
    op (prop_set_decl)                   \
    op (obj_decl)                        \
    op (delete)                          \
    op (with)                            \
    op (end_with)                        \
    op (meta)                            \
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

ecma_completion_value_t
opfunc_call_1 (opcode_t opdata __unused, int_data_t *int_data)
{
  ecma_completion_value_t ret_value;
  ret_value = ecma_make_empty_completion_value ();

  int_data->pos++;

  ECMA_TRY_CATCH (arg_value, get_variable_value (int_data, opdata.data.call_1.arg1_lit_idx, false), ret_value);


  /**********************************************************/

  FIXME (/* Native call, i.e. opfunc_native_call */);
  bool is_native_call = true;

  string_literal_copy function_name;
  init_string_literal_copy (opdata.data.call_1.name_lit_idx, &function_name);

  if (!__strcmp ((const char*) function_name.str_p, "LEDToggle"))
  {
    JERRY_ASSERT (arg_value.value.value_type == ECMA_TYPE_NUMBER);
    ecma_number_t * num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_value.value.value);
    uint32_t int_num = (uint32_t) * num_p;
    led_toggle (int_num);
    ret_value = ecma_make_empty_completion_value ();
  }
  else if (!__strcmp ((const char*) function_name.str_p, "LEDOn"))
  {
    JERRY_ASSERT (arg_value.value.value_type == ECMA_TYPE_NUMBER);
    ecma_number_t * num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_value.value.value);
    uint32_t int_num = (uint32_t) * num_p;
    led_on (int_num);
    ret_value = ecma_make_empty_completion_value ();
  }
  else if (!__strcmp ((const char*) function_name.str_p, "LEDOff"))
  {
    JERRY_ASSERT (arg_value.value.value_type == ECMA_TYPE_NUMBER);
    ecma_number_t * num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_value.value.value);
    uint32_t int_num = (uint32_t) * num_p;
    led_off (int_num);
    ret_value = ecma_make_empty_completion_value ();
  }
  else if (!__strcmp ((const char*) function_name.str_p, "LEDOnce"))
  {
    JERRY_ASSERT (arg_value.value.value_type == ECMA_TYPE_NUMBER);
    ecma_number_t * num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_value.value.value);
    uint32_t int_num = (uint32_t) * num_p;
    led_blink_once (int_num);
    ret_value = ecma_make_empty_completion_value ();
  }
  else if (!__strcmp ((const char*) function_name.str_p, "wait"))
  {
    JERRY_ASSERT (arg_value.value.value_type == ECMA_TYPE_NUMBER);
    ecma_number_t * num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_value.value.value);
    uint32_t int_num = (uint32_t) * num_p;
    wait_ms (int_num);
    ret_value = ecma_make_empty_completion_value ();
  }
  else if (!__strcmp ((const char *) function_name.str_p, "exit"))
  {
    JERRY_ASSERT (arg_value.value.value_type == ECMA_TYPE_NUMBER);
    ecma_number_t * num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_value.value.value);
    uint32_t int_num = (uint32_t) * num_p;
    ecma_value_t exit_status = ecma_make_simple_value (int_num == 0 ? ECMA_SIMPLE_VALUE_TRUE
                                                       : ECMA_SIMPLE_VALUE_FALSE);
    ret_value = ecma_make_completion_value (ECMA_COMPLETION_TYPE_EXIT,
                                            exit_status,
                                            ECMA_TARGET_ID_RESERVED);
  }
  else
  {
    is_native_call = false;
  }

  if (is_native_call)
  {
#ifdef __TARGET_HOST_x64
    __printf ("%d::op_call_1:idx:%d:%d\t",
              int_data->pos,
              opdata.data.call_1.name_lit_idx,
              opdata.data.call_1.arg1_lit_idx);

    __printf ("%s\n", function_name.str_p);
#endif
  }

  free_string_literal_copy (&function_name);

  /**********************************************************/


  if (!is_native_call)
  {
    const idx_t func_name_lit_idx = opdata.data.call_1.name_lit_idx;
    const idx_t lhs_var_idx = opdata.data.call_1.lhs;

    ECMA_TRY_CATCH (func_value, get_variable_value (int_data, func_name_lit_idx, false), ret_value);

    if (!ecma_op_is_callable (func_value.value))
    {
      ret_value = ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      ecma_object_t *func_obj_p = ECMA_GET_POINTER (func_value.value.value);

      ECMA_TRY_CATCH (this_value, ecma_op_implicit_this_value (int_data->lex_env_p), ret_value);
      ECMA_FUNCTION_CALL (call_completion,
                          ecma_op_function_call (func_obj_p, this_value.value, &arg_value.value, 1),
                          ret_value);

      ret_value = set_variable_value (int_data, lhs_var_idx, call_completion.value);

      ECMA_FINALIZE (call_completion);
      ECMA_FINALIZE (this_value);
    }

    ECMA_FINALIZE (func_value);
  }

  ECMA_FINALIZE (arg_value);

  return ret_value;
}

/**
 * 'Assignment' opcode handler.
 *
 * Note:
 *      This handler implements case of assignment of a literal's or a variable's
 *      value to a variable. Assignment to an object's property is not implemented
 *      by this opcode.
 *
 *      FIXME: Add cross link with 'property accessor assignment` opcode.
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
      get_value_completion = ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                                         ecma_make_simple_value (src_val_descr),
                                                         ECMA_TARGET_ID_RESERVED);
      break;
    }
    case OPCODE_ARG_TYPE_STRING:
    {
      ecma_string_t *ecma_string_p = ecma_new_ecma_string_from_lit_index (src_val_descr);

      get_value_completion = ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                                         ecma_make_string_value (ecma_string_p),
                                                         ECMA_TARGET_ID_RESERVED);
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
      *num_p = get_number_by_idx (src_val_descr);

      get_value_completion = ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                                         ecma_make_number_value (num_p),
                                                         ECMA_TARGET_ID_RESERVED);
      break;
    }
    case OPCODE_ARG_TYPE_SMALLINT:
    {
      ecma_number_t *num_p = ecma_alloc_number ();
      *num_p = src_val_descr;

      get_value_completion = ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                                         ecma_make_number_value (num_p),
                                                         ECMA_TARGET_ID_RESERVED);
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

    ecma_completion_value_t assignment_completion_value = set_variable_value (int_data,
                                                                              dst_var_idx,
                                                                              get_value_completion.value);

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
  ECMA_TRY_CATCH (old_num_value, ecma_op_to_number (old_value.value), ret_value);

  // 4.
  ecma_number_t* new_num_p = ecma_alloc_number ();

  ecma_number_t* old_num_p = (ecma_number_t*) ECMA_GET_POINTER (old_num_value.value.value);
  *new_num_p = ecma_op_number_add (*old_num_p, ECMA_NUMBER_ONE);

  ecma_value_t new_num_value = ecma_make_number_value (new_num_p);

  // 5.
  ret_value = set_variable_value (int_data,
                                  incr_var_idx,
                                  new_num_value);

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (int_data,
                                                                   dst_var_idx,
                                                                   new_num_value);
  JERRY_ASSERT (ecma_is_empty_completion_value (reg_assignment_res));

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
  ECMA_TRY_CATCH (old_num_value, ecma_op_to_number (old_value.value), ret_value);

  // 4.
  ecma_number_t* new_num_p = ecma_alloc_number ();

  ecma_number_t* old_num_p = (ecma_number_t*) ECMA_GET_POINTER (old_num_value.value.value);
  *new_num_p = ecma_op_number_substract (*old_num_p, ECMA_NUMBER_ONE);

  ecma_value_t new_num_value = ecma_make_number_value (new_num_p);

  // 5.
  ret_value = set_variable_value (int_data,
                                  decr_var_idx,
                                  new_num_value);

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (int_data,
                                                                   dst_var_idx,
                                                                   new_num_value);
  JERRY_ASSERT (ecma_is_empty_completion_value (reg_assignment_res));

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
  ECMA_TRY_CATCH (old_num_value, ecma_op_to_number (old_value.value), ret_value);

  // 4.
  ecma_number_t* new_num_p = ecma_alloc_number ();

  ecma_number_t* old_num_p = (ecma_number_t*) ECMA_GET_POINTER (old_num_value.value.value);
  *new_num_p = ecma_op_number_add (*old_num_p, ECMA_NUMBER_ONE);

  // 5.
  ret_value = set_variable_value (int_data,
                                  incr_var_idx,
                                  ecma_make_number_value (new_num_p));

  ecma_dealloc_number (new_num_p);

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (int_data,
                                                                   dst_var_idx,
                                                                   old_value.value);
  JERRY_ASSERT (ecma_is_empty_completion_value (reg_assignment_res));

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
  ECMA_TRY_CATCH (old_num_value, ecma_op_to_number (old_value.value), ret_value);

  // 4.
  ecma_number_t* new_num_p = ecma_alloc_number ();

  ecma_number_t* old_num_p = (ecma_number_t*) ECMA_GET_POINTER (old_num_value.value.value);
  *new_num_p = ecma_op_number_substract (*old_num_p, ECMA_NUMBER_ONE);

  // 5.
  ret_value = set_variable_value (int_data,
                                  decr_var_idx,
                                  ecma_make_number_value (new_num_p));

  ecma_dealloc_number (new_num_p);

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (int_data,
                                                                   dst_var_idx,
                                                                   old_value.value);
  JERRY_ASSERT (ecma_is_empty_completion_value (reg_assignment_res));

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

  if (ecma_is_completion_value_normal_false (ecma_op_has_binding (int_data->lex_env_p,
                                                                  var_name_string_p)))
  {
    FIXME ("Pass configurableBindings that is true if and only if current code is eval code");

    ecma_completion_value_t completion = ecma_op_create_mutable_binding (int_data->lex_env_p,
                                                                         var_name_string_p,
                                                                         false);

    JERRY_ASSERT (ecma_is_empty_completion_value (completion));

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
  TODO ("Check if code of function itself is strict");

  const bool is_strict = int_data->is_strict;
  const bool is_configurable_bindings = int_data->is_eval_code;

  const opcode_counter_t jmp_down_opcode_idx = (opcode_counter_t) (int_data->pos);
  opcode_t jmp_down_opcode = read_opcode (jmp_down_opcode_idx);
  JERRY_ASSERT (jmp_down_opcode.op_idx == __op__idx_jmp_down);
  int_data->pos = (opcode_counter_t) (jmp_down_opcode_idx + jmp_down_opcode.data.jmp_down.opcode_count);

  const opcode_counter_t function_code_opcode_idx = (opcode_counter_t) (jmp_down_opcode_idx + 1);

  ecma_string_t *function_name_string_p = ecma_new_ecma_string_from_lit_index (function_name_lit_idx);

  ecma_completion_value_t ret_value = ecma_op_function_declaration (int_data->lex_env_p,
                                                                    function_name_string_p,
                                                                    function_code_opcode_idx,
                                                                    args_names,
                                                                    args_number,
                                                                    is_strict,
                                                                    is_configurable_bindings);
  ecma_deref_ecma_string (function_name_string_p);

  return ret_value;
} /* function_declaration */

/**
 * 'Function declaration with no parameters' opcode handler.
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_func_decl_0 (opcode_t opdata, /**< operation data */
                    int_data_t *int_data) /**< interpreter context */
{
  int_data->pos++;

  return function_declaration (int_data,
                               opdata.data.func_decl_0.name_lit_idx,
                               NULL,
                               0);
} /* opfunc_func_decl_0 */

/**
 * 'Function declaration with one parameter' opcode handler.
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_func_decl_1 (opcode_t opdata, /**< operation data */
                    int_data_t *int_data) /**< interpreter context */
{
  int_data->pos++;

  ecma_string_t *arg_name_string_p = ecma_new_ecma_string_from_lit_index (opdata.data.func_decl_1.arg1_lit_idx);

  ecma_completion_value_t ret_value = function_declaration (int_data,
                                                            opdata.data.func_decl_1.name_lit_idx,
                                                            &arg_name_string_p,
                                                            1);

  ecma_deref_ecma_string (arg_name_string_p);

  return ret_value;
} /* opfunc_func_decl_1 */

/**
 * 'Function declaration with two parameters' opcode handler.
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_func_decl_2 (opcode_t opdata, /**< operation data */
                    int_data_t *int_data) /**< interpreter context */
{
  int_data->pos++;

  ecma_string_t* arg_names_strings[2] =
  {
    ecma_new_ecma_string_from_lit_index (opdata.data.func_decl_2.arg1_lit_idx),
    ecma_new_ecma_string_from_lit_index (opdata.data.func_decl_2.arg2_lit_idx)
  };

  ecma_completion_value_t ret_value = function_declaration (int_data,
                                                            opdata.data.func_decl_1.name_lit_idx,
                                                            arg_names_strings,
                                                            2);

  ecma_deref_ecma_string (arg_names_strings[0]);
  ecma_deref_ecma_string (arg_names_strings[1]);

  return ret_value;
} /* opfunc_func_decl_2 */

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
  const ecma_length_t args_number = opdata.data.func_decl_n.arg_list;

  ecma_string_t *args_names[args_number + 1 /* length of array should not be zero */];

  ecma_length_t arg_index = 0;
  opcode_t next_opcode = read_opcode (int_data->pos);
  while (next_opcode.op_idx == __op__idx_varg)
  {
    const idx_t arg_lit_idx = next_opcode.data.varg.arg_lit_idx;
    args_names[arg_index++] = ecma_new_ecma_string_from_lit_index (arg_lit_idx);

    JERRY_ASSERT (arg_index <= args_number);

    int_data->pos++;
    next_opcode = read_opcode (int_data->pos);
  }
  JERRY_ASSERT (arg_index == args_number);

  ecma_completion_value_t ret_value = function_declaration (int_data,
                                                            function_name_idx,
                                                            args_names,
                                                            args_number);

  for (arg_index = 0;
       arg_index < args_number;
       arg_index++)
  {
    ecma_deref_ecma_string (args_names[arg_index]);
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
  const ecma_length_t args_number = opdata.data.func_expr_n.arg_list;

  const bool is_named_func_expr = (!is_reg_variable (int_data, function_name_lit_idx));

  TODO ("Check if code of function itself is strict");

  const bool is_strict = int_data->is_strict;

  ecma_string_t *args_names[args_number + 1 /* length of array should not be zero */];

  ecma_length_t arg_index = 0;
  opcode_t next_opcode = read_opcode (int_data->pos);
  while (next_opcode.op_idx == __op__idx_varg)
  {
    const idx_t arg_lit_idx = next_opcode.data.varg.arg_lit_idx;
    args_names[arg_index++] = ecma_new_ecma_string_from_lit_index (arg_lit_idx);

    JERRY_ASSERT (arg_index <= args_number);

    int_data->pos++;
    next_opcode = read_opcode (int_data->pos);
  }
  JERRY_ASSERT (arg_index == args_number);

  const opcode_counter_t jmp_down_opcode_idx = (opcode_counter_t) (int_data->pos);
  opcode_t jmp_down_opcode = read_opcode (jmp_down_opcode_idx);
  JERRY_ASSERT (jmp_down_opcode.op_idx == __op__idx_jmp_down);
  int_data->pos = (opcode_counter_t) (jmp_down_opcode_idx + jmp_down_opcode.data.jmp_down.opcode_count);

  const opcode_counter_t function_code_opcode_idx = (opcode_counter_t) (jmp_down_opcode_idx + 1);

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

  ecma_object_t *func_obj_p = ecma_op_create_function_object (args_names,
                                                              args_number,
                                                              scope_p,
                                                              is_strict,
                                                              function_code_opcode_idx);

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

  for (arg_index = 0;
       arg_index < args_number;
       arg_index++)
  {
    ecma_deref_ecma_string (args_names[arg_index]);
  }

  return ret_value;
} /* opfunc_func_expr_n */

/**
 * Function call with no arguments' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_call_0 (opcode_t opdata, /**< operation data */
               int_data_t *int_data) /**< interpreter context */
{
  const idx_t func_name_lit_idx = opdata.data.call_0.name_lit_idx;
  const idx_t lhs_var_idx = opdata.data.call_0.lhs;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (func_value, get_variable_value (int_data, func_name_lit_idx, false), ret_value);

  if (!ecma_op_is_callable (func_value.value))
  {
    ret_value = ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_object_t *func_obj_p = ECMA_GET_POINTER (func_value.value.value);

    ECMA_TRY_CATCH (this_value, ecma_op_implicit_this_value (int_data->lex_env_p), ret_value);
    ECMA_FUNCTION_CALL (call_completion,
                        ecma_op_function_call (func_obj_p, this_value.value, NULL, 0),
                        ret_value);

    ret_value = set_variable_value (int_data, lhs_var_idx, call_completion.value);

    ECMA_FINALIZE (call_completion);
    ECMA_FINALIZE (this_value);
  }

  ECMA_FINALIZE (func_value);

  return ret_value;
} /* opfunc_call_0 */

/**
 * Fill arguments' list
 *
 * @return empty completion value if argument list was filled successfully,
 *         otherwise - not normal completion value indicating completion type
 *         of last expression evaluated
 */
static ecma_completion_value_t
fill_varg_list (int_data_t *int_data, /**< interpreter context */
                ecma_length_t args_number, /**< number of arguments */
                ecma_value_t arg_values[], /**< out: arguments' values */
                ecma_length_t *out_arg_number_p) /**< out: number of arguments
                                                      successfully read */
{
  ecma_completion_value_t get_arg_completion = ecma_make_empty_completion_value ();

  ecma_length_t arg_index;
  for (arg_index = 0;
       arg_index < args_number;
       arg_index++)
  {
    opcode_t next_opcode = read_opcode (int_data->pos);
    if (next_opcode.op_idx == __op__idx_varg)
    {
      get_arg_completion = get_variable_value (int_data, next_opcode.data.varg.arg_lit_idx, false);
      if (unlikely (ecma_is_completion_value_throw (get_arg_completion)))
      {
        break;
      }
      else
      {
        JERRY_ASSERT (ecma_is_completion_value_normal (get_arg_completion));
        arg_values[arg_index] = get_arg_completion.value;
      }
    }
    else
    {
      get_arg_completion = run_int_loop (int_data);

      if (get_arg_completion.type == ECMA_COMPLETION_TYPE_VARG)
      {
        arg_values[arg_index] = get_arg_completion.value;
      }
      else
      {
        break;
      }
    }

    int_data->pos++;
  }

  *out_arg_number_p = arg_index;

  if (get_arg_completion.type == ECMA_COMPLETION_TYPE_NORMAL
      || get_arg_completion.type == ECMA_COMPLETION_TYPE_VARG)
  {
    /* values are copied to arg_values */
    return ecma_make_empty_completion_value ();
  }
  else
  {
    return get_arg_completion;
  }
} /* fill_varg_list */

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
  const idx_t args_number = opdata.data.call_n.arg_list;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (func_value, get_variable_value (int_data, func_name_lit_idx, false), ret_value);

  ecma_value_t arg_values[args_number + 1 /* length of array should not be zero */];

  ecma_length_t args_read;
  ecma_completion_value_t get_arg_completion = fill_varg_list (int_data,
                                                               args_number,
                                                               arg_values,
                                                               &args_read);

  if (ecma_is_empty_completion_value (get_arg_completion))
  {
    ecma_completion_value_t this_value;

    opcode_t next_opcode = read_opcode (int_data->pos);
    if (next_opcode.op_idx == __op__idx_meta
        && next_opcode.data.meta.type == OPCODE_META_TYPE_THIS_ARG)
    {
      const idx_t this_arg_var_idx = next_opcode.data.meta.data_1;

      JERRY_ASSERT (is_reg_variable (int_data, this_arg_var_idx));
      this_value = get_variable_value (int_data, this_arg_var_idx, false);
    }
    else
    {
      this_value = ecma_op_implicit_this_value (int_data->lex_env_p);
      JERRY_ASSERT (ecma_is_completion_value_normal (this_value));
    }

    if (!ecma_op_is_callable (func_value.value))
    {
      ret_value = ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      ecma_object_t *func_obj_p = ECMA_GET_POINTER (func_value.value.value);

      ECMA_FUNCTION_CALL (call_completion,
                          ecma_op_function_call (func_obj_p, this_value.value, arg_values, args_number),
                          ret_value);

      ret_value = set_variable_value (int_data, lhs_var_idx, call_completion.value);

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

  if (ecma_is_empty_completion_value (get_arg_completion))
  {
    ecma_completion_value_t this_value;

    opcode_t next_opcode = read_opcode (int_data->pos);
    if (next_opcode.op_idx == __op__idx_meta
        && next_opcode.data.meta.type == OPCODE_META_TYPE_THIS_ARG)
    {
      const idx_t this_arg_var_idx = next_opcode.data.meta.data_1;

      JERRY_ASSERT (is_reg_variable (int_data, this_arg_var_idx));
      this_value = get_variable_value (int_data, this_arg_var_idx, false);
    }
    else
    {
      this_value = ecma_op_implicit_this_value (int_data->lex_env_p);
      JERRY_ASSERT (ecma_is_completion_value_normal (this_value));
    }

    if (!ecma_is_constructor (constructor_value.value))
    {
      ret_value = ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      ecma_object_t *constructor_obj_p = ECMA_GET_POINTER (constructor_value.value.value);

      ECMA_TRY_CATCH (construction_completion,
                      ecma_op_function_construct (constructor_obj_p,
                                                  arg_values,
                                                  args_number),
                      ret_value);

      ret_value = set_variable_value (int_data, lhs_var_idx, construction_completion.value);

      ECMA_FINALIZE (construction_completion);

      ecma_free_completion_value (this_value);
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
 * 'varg' opcode handler
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_varg (opcode_t opdata, /**< operation data */
             int_data_t *int_data) /**< interpreter context */
{
  const idx_t arg_lit_idx = opdata.data.varg.arg_lit_idx;

  ecma_completion_value_t completion = get_variable_value (int_data,
                                                           arg_lit_idx,
                                                           false);

  if (ecma_is_completion_value_normal (completion))
  {
    completion.type = ECMA_COMPLETION_TYPE_VARG;
  }

  return completion;
} /* opfunc_varg */

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
  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_RETURN,
                                     ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                                     ECMA_TARGET_ID_RESERVED);
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

  ret_value = ecma_make_completion_value (ECMA_COMPLETION_TYPE_RETURN,
                                          ecma_copy_value (expr_val.value, true),
                                          ECMA_TARGET_ID_RESERVED);

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

  ECMA_TRY_CATCH (base_value, get_variable_value (int_data, base_var_idx, false), ret_value);
  ECMA_TRY_CATCH (prop_name_value, get_variable_value (int_data, prop_name_var_idx, false), ret_value);
  ECMA_TRY_CATCH (check_coercible_ret, ecma_op_check_object_coercible (base_value.value), ret_value);
  ECMA_TRY_CATCH (prop_name_str_value, ecma_op_to_string (prop_name_value.value), ret_value);

  ecma_string_t *prop_name_string_p = ECMA_GET_POINTER (prop_name_str_value.value.value);
  ecma_reference_t ref = ecma_make_reference (base_value.value,
                                              prop_name_string_p,
                                              int_data->is_strict);

  ECMA_TRY_CATCH (prop_value, ecma_op_get_value (ref), ret_value);

  ret_value = set_variable_value (int_data, lhs_var_idx, prop_value.value);

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

  ECMA_TRY_CATCH (base_value, get_variable_value (int_data, base_var_idx, false), ret_value);
  ECMA_TRY_CATCH (prop_name_value, get_variable_value (int_data, prop_name_var_idx, false), ret_value);
  ECMA_TRY_CATCH (check_coercible_ret, ecma_op_check_object_coercible (base_value.value), ret_value);
  ECMA_TRY_CATCH (prop_name_str_value, ecma_op_to_string (prop_name_value.value), ret_value);

  ecma_string_t *prop_name_string_p = ECMA_GET_POINTER (prop_name_str_value.value.value);
  ecma_reference_t ref = ecma_make_reference (base_value.value,
                                              prop_name_string_p,
                                              int_data->is_strict);

  ECMA_TRY_CATCH (rhs_value, get_variable_value (int_data, rhs_var_idx, false), ret_value);
  ret_value = ecma_op_put_value (ref, rhs_value.value);
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

  ecma_value_t exit_status = ecma_make_simple_value (opdata.data.exitval.status_code == 0 ? ECMA_SIMPLE_VALUE_TRUE
                                                     : ECMA_SIMPLE_VALUE_FALSE);
  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_EXIT,
                                     exit_status,
                                     ECMA_TARGET_ID_RESERVED);
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
  ecma_completion_value_t to_bool_value = ecma_op_to_boolean (right_value.value);

  if (ecma_is_value_true (to_bool_value.value))
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
 * 'Logical OR Operator' opcode handler.
 *
 * See also: ECMA-262 v5, 11.11
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_logical_or (opcode_t opdata, /**< operation data */
                   int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.logical_or.dst;
  const idx_t left_var_idx = opdata.data.logical_or.var_left;
  const idx_t right_var_idx = opdata.data.logical_or.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ecma_completion_value_t to_bool_value = ecma_op_to_boolean (left_value.value);

  if (ecma_is_value_true (to_bool_value.value))
  {
    ret_value = set_variable_value (int_data,
                                    dst_var_idx,
                                    left_value.value);
  }
  else
  {
    ret_value = set_variable_value (int_data,
                                    dst_var_idx,
                                    right_value.value);
  }

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_logical_or */

/**
 * 'Logical AND Operator' opcode handler.
 *
 * See also: ECMA-262 v5, 11.11
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_logical_and (opcode_t opdata, /**< operation data */
                   int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.logical_and.dst;
  const idx_t left_var_idx = opdata.data.logical_and.var_left;
  const idx_t right_var_idx = opdata.data.logical_and.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ecma_completion_value_t to_bool_value = ecma_op_to_boolean (left_value.value);

  if (ecma_is_value_true (to_bool_value.value) == false)
  {
    ret_value = set_variable_value (int_data,
                                    dst_var_idx,
                                    left_value.value);
  }
  else
  {
    ret_value = set_variable_value (int_data,
                                    dst_var_idx,
                                    right_value.value);
  }

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_logical_and */

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

    ecma_reference_t ref = ecma_op_get_identifier_reference (int_data->lex_env_p,
                                                             var_name_string_p,
                                                             int_data->is_strict);

    ecma_deref_ecma_string (var_name_string_p);

    const bool is_unresolvable_reference = ecma_is_value_undefined (ref.base);

    if (is_unresolvable_reference)
    {
      ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else
    {
      ret_value = ecma_op_get_value (ref);
    }

    ecma_free_reference (ref);
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

  ecma_value_t typeof_arg = typeof_evaluate_arg_completion.value;

  ecma_string_t *type_str_p = NULL;

  switch ((ecma_type_t)typeof_arg.value_type)
  {
    case ECMA_TYPE_SIMPLE:
    {
      switch ((ecma_simple_value_t)typeof_arg.value)
      {
        case ECMA_SIMPLE_VALUE_UNDEFINED:
        {
          type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_UNDEFINED);
          break;
        }
        case ECMA_SIMPLE_VALUE_NULL:
        {
          type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_NULL);
          break;
        }
        case ECMA_SIMPLE_VALUE_FALSE:
        {
          type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_FALSE);
          break;
        }
        case ECMA_SIMPLE_VALUE_TRUE:
        {
          type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_TRUE);
          break;
        }
        case ECMA_SIMPLE_VALUE_EMPTY:
        case ECMA_SIMPLE_VALUE_ARRAY_REDIRECT:
        case ECMA_SIMPLE_VALUE__COUNT:
        {
          JERRY_UNREACHABLE ();
        }
      }
      break;
    }
    case ECMA_TYPE_NUMBER:
    {
      type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_NUMBER);
      break;
    }
    case ECMA_TYPE_STRING:
    {
      type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_STRING);
      break;
    }
    case ECMA_TYPE_OBJECT:
    {
      if (ecma_op_is_callable (typeof_arg))
      {
        type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_FUNCTION);
      }
      else
      {
        type_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_OBJECT);
      }
      break;
    }
  }

  ret_value = set_variable_value (int_data,
                                  dst_var_idx,
                                  ecma_make_string_value (type_str_p));

  ecma_deref_ecma_string (type_str_p);

  ECMA_FINALIZE (typeof_evaluate_arg_completion);

  return ret_value;
} /* opfunc_typeof */

#define GETOP_DEF_1(a, name, field1) \
        inline opcode_t getop_##name (idx_t arg1) \
        { \
          opcode_t opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          return opdata; \
        }

#define GETOP_DEF_2(a, name, field1, field2) \
        inline opcode_t getop_##name (idx_t arg1, idx_t arg2) \
        { \
          opcode_t opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          opdata.data.name.field2 = arg2; \
          return opdata; \
        }

#define GETOP_DEF_3(a, name, field1, field2, field3) \
        inline opcode_t getop_##name (idx_t arg1, idx_t arg2, idx_t arg3) \
        { \
          opcode_t opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          opdata.data.name.field2 = arg2; \
          opdata.data.name.field3 = arg3; \
          return opdata; \
        }

OP_ARGS_LIST (GETOP_DEF)

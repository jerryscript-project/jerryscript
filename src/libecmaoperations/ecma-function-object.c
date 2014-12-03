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

#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-try-catch-macro.h"
#include "globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmafunctionobject ECMA Function object related routines
 * @{
 */

/**
 * Pack 'is_strict' flag and opcode index to value
 * that can be stored in an [[Code]] internal property.
 *
 * @return packed value
 */
static uint32_t
ecma_pack_code_internal_property_value (bool is_strict, /**< is code strict? */
                                        opcode_counter_t opcode_idx) /**< index of first opcode */
{
  uint32_t value = opcode_idx;
  const uint32_t is_strict_bit_offset = sizeof (value) * JERRY_BITSINBYTE - 1;

  JERRY_ASSERT(((value) & (1u << is_strict_bit_offset)) == 0);

  if (is_strict)
  {
    value |= (1u << is_strict_bit_offset);
  }

  return value;
} /* ecma_pack_code_internal_property_value */

/**
 * Unpack 'is_strict' flag and opcode index from value
 * that can be stored in an [[Code]] internal property.
 *
 * @return opcode index
 */
static opcode_counter_t
ecma_unpack_code_internal_property_value (uint32_t value, /**< packed value */
                                          bool* out_is_strict_p) /**< out: is code strict? */
{
  JERRY_ASSERT(out_is_strict_p != NULL);

  const uint32_t is_strict_bit_offset = sizeof (value) * JERRY_BITSINBYTE - 1;

  bool is_strict = ((value & (1u << is_strict_bit_offset)) != 0);
  *out_is_strict_p = is_strict;

  opcode_counter_t opcode_idx = (opcode_counter_t) (value & ~(1u << is_strict_bit_offset));

  return opcode_idx;
} /* ecma_unpack_code_internal_property_value */

/**
 * IsCallable operation.
 *
 * See also: ECMA-262 v5, 9.11
 *
 * @return true, if value is callable object;
 *         false - otherwise.
 */
bool
ecma_op_is_callable (ecma_value_t value) /**< ecma-value */
{
  if (!ecma_is_value_object (value))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (value);

  JERRY_ASSERT(obj_p != NULL);
  JERRY_ASSERT(!ecma_is_lexical_environment (obj_p));

  return (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_FUNCTION
          || ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION
          || ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION);
} /* ecma_op_is_callable */

/**
 * Check whether the value is Object that implements [[Construct]].
 *
 * @return true, if value is constructor object;
 *         false - otherwise.
 */
bool
ecma_is_constructor (ecma_value_t value) /**< ecma-value */
{
  if (!ecma_is_value_object (value))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (value);

  JERRY_ASSERT(obj_p != NULL);
  JERRY_ASSERT(!ecma_is_lexical_environment (obj_p));

  return (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_FUNCTION
          || ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);
} /* ecma_is_constructor */

/**
 * Function object creation operation.
 *
 * See also: ECMA-262 v5, 13.2
 *
 * @return pointer to newly created Function object
 */
ecma_object_t*
ecma_op_create_function_object (ecma_string_t* formal_parameter_list_p[], /**< formal parameters list */
                                ecma_length_t formal_parameters_number, /**< formal parameters list's length */
                                ecma_object_t *scope_p, /**< function's scope */
                                bool is_strict, /**< 'strict' flag */
                                opcode_counter_t first_opcode_idx) /**< index of first opcode of function's body */
{
  // 1., 4., 13.
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);

  ecma_object_t *f = ecma_create_object (prototype_obj_p, true, ECMA_OBJECT_TYPE_FUNCTION);

  ecma_deref_object (prototype_obj_p);

  // 2., 6., 7., 8.
  /*
   * We don't setup [[Get]], [[Call]], [[Construct]], [[HasInstance]] for each function object.
   * Instead we set the object's type to ECMA_OBJECT_TYPE_FUNCTION
   * that defines which version of the routine should be used on demand.
   */

  // 3.
  ecma_property_t *class_prop_p = ecma_create_internal_property (f, ECMA_INTERNAL_PROPERTY_CLASS);
  class_prop_p->u.internal_property.value = ECMA_MAGIC_STRING_FUNCTION_UL;

  // 9.
  ecma_property_t *scope_prop_p = ecma_create_internal_property (f, ECMA_INTERNAL_PROPERTY_SCOPE);
  ECMA_SET_POINTER(scope_prop_p->u.internal_property.value, scope_p);
  ecma_gc_update_may_ref_younger_object_flag_by_object (f, scope_p);

  // 10., 11.
  ecma_property_t *formal_parameters_prop_p = ecma_create_internal_property (f,
                                                                             ECMA_INTERNAL_PROPERTY_FORMAL_PARAMETERS);
  if (formal_parameters_number != 0)
  {
    ecma_collection_header_t *formal_parameters_collection_p = ecma_new_strings_collection (formal_parameter_list_p,
                                                                                            formal_parameters_number);
    ECMA_SET_POINTER (formal_parameters_prop_p->u.internal_property.value, formal_parameters_collection_p);
  }
  else
  {
    JERRY_ASSERT (formal_parameters_prop_p->u.internal_property.value == ECMA_NULL_POINTER);
  }

  // 12.
  ecma_property_t *code_prop_p = ecma_create_internal_property (f, ECMA_INTERNAL_PROPERTY_CODE);
  code_prop_p->u.internal_property.value = ecma_pack_code_internal_property_value (is_strict,
                                                                                   first_opcode_idx);

  // 14.
  ecma_number_t* len_p = ecma_alloc_number ();
  *len_p = ecma_uint32_to_number (formal_parameters_number);

  // 15.
  ecma_property_descriptor_t length_prop_desc = ecma_make_empty_property_descriptor ();
  length_prop_desc.is_value_defined = true;
  length_prop_desc.value = ecma_make_number_value (len_p);

  ecma_string_t* magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);
  ecma_completion_value_t completion = ecma_op_object_define_own_property (f,
                                                                           magic_string_length_p,
                                                                           length_prop_desc,
                                                                           false);
  ecma_deref_ecma_string (magic_string_length_p);

  JERRY_ASSERT (ecma_is_completion_value_normal_true (completion)
                || ecma_is_completion_value_normal_false (completion));

  ecma_dealloc_number (len_p);
  len_p = NULL;

  // 16.
  ecma_object_t *proto_p = ecma_op_create_object_object_noarg ();

  // 17.
  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();
  {
    prop_desc.is_value_defined = true;
    prop_desc.value = ecma_make_object_value (f);

    prop_desc.is_writable_defined = true;
    prop_desc.is_writable = true;

    prop_desc.is_enumerable_defined = true;
    prop_desc.is_enumerable = false;

    prop_desc.is_configurable_defined = true;
    prop_desc.is_configurable = true;
  }

  ecma_string_t *magic_string_constructor_p = ecma_get_magic_string (ECMA_MAGIC_STRING_CONSTRUCTOR);
  ecma_op_object_define_own_property (proto_p,
                                      magic_string_constructor_p,
                                      prop_desc,
                                      false);
  ecma_deref_ecma_string (magic_string_constructor_p);

  // 18.
  prop_desc.value = ecma_make_object_value (proto_p);
  prop_desc.is_configurable = false;
  ecma_string_t *magic_string_prototype_p = ecma_get_magic_string (ECMA_MAGIC_STRING_PROTOTYPE);
  ecma_op_object_define_own_property (f,
                                      magic_string_prototype_p,
                                      prop_desc,
                                      false);
  ecma_deref_ecma_string (magic_string_prototype_p);

  ecma_deref_object (proto_p);

  // 19.
  if (is_strict)
  {
    ecma_object_t *thrower_p = ecma_builtin_get (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER);

    prop_desc = ecma_make_empty_property_descriptor ();
    {
      prop_desc.is_enumerable_defined = true;
      prop_desc.is_enumerable = false;

      prop_desc.is_configurable_defined = true;
      prop_desc.is_configurable = false;

      prop_desc.is_get_defined = true;
      prop_desc.get_p = thrower_p;

      prop_desc.is_set_defined = true;
      prop_desc.set_p = thrower_p;
    }

    ecma_string_t *magic_string_caller_p = ecma_get_magic_string (ECMA_MAGIC_STRING_CALLER);
    ecma_op_object_define_own_property (f,
                                        magic_string_caller_p,
                                        prop_desc,
                                        false);
    ecma_deref_ecma_string (magic_string_caller_p);

    ecma_string_t *magic_string_arguments_p = ecma_get_magic_string (ECMA_MAGIC_STRING_ARGUMENTS);
    ecma_op_object_define_own_property (f,
                                        magic_string_arguments_p,
                                        prop_desc,
                                        false);
    ecma_deref_ecma_string (magic_string_arguments_p);

    ecma_deref_object (thrower_p);
  }

  return f;
} /* ecma_op_create_function_object */

/**
 * Setup variables for arguments listed in formal parameter list.
 *
 * See also:
 *          Declaration binding instantiation (ECMA-262 v5, 10.5), block 4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
ecma_function_call_setup_args_variables (ecma_object_t *func_obj_p, /**< Function object */
                                         ecma_object_t *env_p, /**< lexical environment */
                                         ecma_value_t *arguments_list_p, /**< arguments list */
                                         ecma_length_t arguments_list_len, /**< length of argument list */
                                         bool is_strict) /**< flag indicating strict mode */
{
  ecma_property_t *formal_parameters_prop_p = ecma_get_internal_property (func_obj_p,
                                                                          ECMA_INTERNAL_PROPERTY_FORMAL_PARAMETERS);
  ecma_collection_header_t *formal_parameters_p;
  formal_parameters_p = ECMA_GET_POINTER (formal_parameters_prop_p->u.internal_property.value);

  if (formal_parameters_p == NULL)
  {
    return ecma_make_empty_completion_value ();
  }

  ecma_length_t formal_parameters_count = formal_parameters_p->unit_number;

  ecma_collection_iterator_t formal_params_iterator;
  ecma_collection_iterator_init (&formal_params_iterator, formal_parameters_p);

  for (size_t n = 0;
       n < formal_parameters_count;
       n++)
  {
    ecma_value_t v;
    if (n >= arguments_list_len)
    {
      v = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else
    {
      v = arguments_list_p[n];
    }

    bool is_moved = ecma_collection_iterator_next (&formal_params_iterator);
    JERRY_ASSERT (is_moved);

    ecma_value_t formal_parameter_name_value = *formal_params_iterator.current_value_p;
    ecma_string_t *formal_parameter_name_string_p = ecma_get_string_from_value (formal_parameter_name_value);
    
    bool arg_already_declared = ecma_op_has_binding (env_p, formal_parameter_name_string_p);
    if (!arg_already_declared)
    {
      ecma_completion_value_t completion = ecma_op_create_mutable_binding (env_p,
                                                                           formal_parameter_name_string_p,
                                                                           false);
      if (ecma_is_completion_value_throw (completion))
      {
        return completion;
      }

      JERRY_ASSERT (ecma_is_completion_value_empty (completion));

      completion = ecma_op_set_mutable_binding (env_p,
                                                formal_parameter_name_string_p,
                                                v,
                                                is_strict);

      if (ecma_is_completion_value_throw (completion))
      {
        return completion;
      }

      JERRY_ASSERT (ecma_is_completion_value_empty (completion));
    }
  }

  return ecma_make_empty_completion_value ();
} /* ecma_function_call_setup_args_variables */

/**
 * [[Call]] implementation for Function objects,
 * created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION)
 * or 15.3.4.5 (ECMA_OBJECT_TYPE_BOUND_FUNCTION),
 * and for built-in Function objects
 * from section 15 (ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION).
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_function_has_instance (ecma_object_t *func_obj_p, /**< Function object */
                               ecma_value_t value) /**< argument 'V' */
{
  JERRY_ASSERT(func_obj_p != NULL
               && !ecma_is_lexical_environment (func_obj_p));

  if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION)
  {
    if (!ecma_is_value_object (value))
    {
      return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
    }

    ecma_object_t* v_obj_p = ecma_get_object_from_value (value);

    ecma_string_t *prototype_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_PROTOTYPE);

    ecma_completion_value_t ret_value;

    ECMA_TRY_CATCH (prototype_obj_value,
                    ecma_op_object_get (func_obj_p, prototype_magic_string_p),
                    ret_value);

    if (!ecma_is_value_object (ecma_get_completion_value_value (prototype_obj_value)))
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      ecma_object_t *prototype_obj_p = ecma_get_object_from_completion_value (prototype_obj_value);
      JERRY_ASSERT (prototype_obj_p != NULL);

      do
      {
        v_obj_p = ecma_get_object_prototype (v_obj_p);

        if (v_obj_p == NULL)
        {
          ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);

          break;
        }
        else if (v_obj_p == prototype_obj_p)
        {
          ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);

          break;
        }
      } while (true);
    }

    ECMA_FINALIZE (prototype_obj_value);

    ecma_deref_ecma_string (prototype_magic_string_p);

    return ret_value;
  }
  else if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION)
  {
    return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    JERRY_UNIMPLEMENTED ("Bound functions are not implemented.");
  }
} /* ecma_op_function_has_instance */

/**
 * [[Call]] implementation for Function objects,
 * created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION)
 * or 15.3.4.5 (ECMA_OBJECT_TYPE_BOUND_FUNCTION),
 * and for built-in Function objects
 * from section 15 (ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION).
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_function_call (ecma_object_t *func_obj_p, /**< Function object */
                       ecma_value_t this_arg_value, /**< 'this' argument's value */
                       ecma_value_t* arguments_list_p, /**< arguments list */
                       ecma_length_t arguments_list_len) /**< length of arguments list */
{
  JERRY_ASSERT(func_obj_p != NULL
               && !ecma_is_lexical_environment (func_obj_p));
  JERRY_ASSERT(ecma_op_is_callable (ecma_make_object_value (func_obj_p)));
  JERRY_ASSERT(arguments_list_len == 0 || arguments_list_p != NULL);

  if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION)
  {
    if (unlikely (ecma_get_object_is_builtin (func_obj_p)))
    {
      return ecma_builtin_dispatch_call (func_obj_p, this_arg_value, arguments_list_p, arguments_list_len);
    }

    ecma_completion_value_t ret_value;

    /* Entering Function Code (ECMA-262 v5, 10.4.3) */

    ecma_property_t *scope_prop_p = ecma_get_internal_property (func_obj_p, ECMA_INTERNAL_PROPERTY_SCOPE);
    ecma_property_t *code_prop_p = ecma_get_internal_property (func_obj_p, ECMA_INTERNAL_PROPERTY_CODE);

    ecma_object_t *scope_p = ECMA_GET_NON_NULL_POINTER(scope_prop_p->u.internal_property.value);
    uint32_t code_prop_value = code_prop_p->u.internal_property.value;

    bool is_strict;
    // 8.
    opcode_counter_t code_first_opcode_idx = ecma_unpack_code_internal_property_value (code_prop_value, &is_strict);

    ecma_value_t this_binding;
    // 1.
    if (is_strict)
    {
      this_binding = ecma_copy_value (this_arg_value, true);
    }
    else if (ecma_is_value_undefined (this_arg_value)
             || ecma_is_value_null (this_arg_value))
    {
      // 2.
      this_binding = ecma_make_object_value (ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL));
    }
    else
    {
      // 3., 4.
      ecma_completion_value_t completion = ecma_op_to_object (this_arg_value);
      JERRY_ASSERT (ecma_is_completion_value_normal (completion));

      this_binding = ecma_get_completion_value_value (completion);
    }

    // 5.
    ecma_object_t *local_env_p = ecma_create_decl_lex_env (scope_p);

    // 9.
    ECMA_TRY_CATCH (args_var_declaration_completion,
                    ecma_function_call_setup_args_variables (func_obj_p,
                                                             local_env_p,
                                                             arguments_list_p,
                                                             arguments_list_len,
                                                             is_strict),
                    ret_value);

    ecma_completion_value_t completion = run_int_from_pos (code_first_opcode_idx,
                                                           this_binding,
                                                           local_env_p,
                                                           is_strict,
                                                           false);
    if (ecma_is_completion_value_return (completion))
    {
      ret_value = ecma_make_normal_completion_value (ecma_get_completion_value_value (completion));
    }
    else
    {
      ret_value = completion;
    }

    ECMA_FINALIZE (args_var_declaration_completion);

    ecma_deref_object (local_env_p);
    ecma_free_value (this_binding, true);

    return ret_value;
  }
  else if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION)
  {
    return ecma_builtin_dispatch_call (func_obj_p, this_arg_value, arguments_list_p, arguments_list_len);
  }
  else
  {
    JERRY_ASSERT(ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    JERRY_UNIMPLEMENTED ("Bound functions are not implemented.");
  }
} /* ecma_op_function_call */

/**
 * [[Construct]] implementation for Function objects,
 * created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION)
 * or 15.3.4.5 (ECMA_OBJECT_TYPE_BOUND_FUNCTION).
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_function_construct (ecma_object_t *func_obj_p, /**< Function object */
                            ecma_value_t* arguments_list_p, /**< arguments list */
                            ecma_length_t arguments_list_len) /**< length of arguments list */
{
  JERRY_ASSERT(func_obj_p != NULL
               && !ecma_is_lexical_environment (func_obj_p));
  JERRY_ASSERT(ecma_is_constructor (ecma_make_object_value (func_obj_p)));
  JERRY_ASSERT(arguments_list_len == 0 || arguments_list_p != NULL);

  if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION)
  {
    if (unlikely (ecma_get_object_is_builtin (func_obj_p)))
    {
      return ecma_builtin_dispatch_construct (func_obj_p, arguments_list_p, arguments_list_len);
    }

    ecma_completion_value_t ret_value;

    ecma_string_t *prototype_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_PROTOTYPE);

    // 5.
    ECMA_TRY_CATCH (func_obj_prototype_prop_value,
                    ecma_op_object_get (func_obj_p,
                                        prototype_magic_string_p),
                    ret_value);

    //  6.
    ecma_object_t *prototype_p;
    if (ecma_is_value_object (ecma_get_completion_value_value (func_obj_prototype_prop_value)))
    {
      prototype_p = ecma_get_object_from_completion_value (func_obj_prototype_prop_value);
      ecma_ref_object (prototype_p);
    }
    else
    {
      // 7.
      prototype_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
    }

    // 1., 2., 4.
    ecma_object_t *obj_p = ecma_create_object (prototype_p, true, ECMA_OBJECT_TYPE_GENERAL);

    // 3.
    ecma_property_t *class_prop_p = ecma_create_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_CLASS);
    class_prop_p->u.internal_property.value = ECMA_MAGIC_STRING_FUNCTION_UL;

    ecma_deref_object (prototype_p);

    // 8.
    ECMA_TRY_CATCH (call_completion,
                    ecma_op_function_call (func_obj_p,
                                           ecma_make_object_value (obj_p),
                                           arguments_list_p,
                                           arguments_list_len),
                    ret_value);

    ecma_value_t obj_value;

    // 9.
    if (ecma_is_value_object (ecma_get_completion_value_value (call_completion)))
    {
      ecma_deref_object (obj_p);

      obj_value = ecma_copy_value (ecma_get_completion_value_value (call_completion), true);
    }
    else
    {
      // 10.
      obj_value = ecma_make_object_value (obj_p);
    }

    ret_value = ecma_make_normal_completion_value (obj_value);

    ECMA_FINALIZE (call_completion);
    ECMA_FINALIZE (func_obj_prototype_prop_value);

    ecma_deref_ecma_string (prototype_magic_string_p);

    return ret_value;
  }
  else
  {
    JERRY_ASSERT(ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    JERRY_UNIMPLEMENTED ("Bound functions are not implemented.");
  }
} /* ecma_op_function_construct */
/**
 * Function declaration.
 *
 * See also: ECMA-262 v5, 10.5 - Declaration binding instantiation (block 5).
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_op_function_declaration (ecma_object_t *lex_env_p, /**< lexical environment */
                              ecma_string_t *function_name_p, /**< function name */
                              opcode_counter_t function_code_opcode_idx, /**< index of first opcode of function code */
                              ecma_string_t* formal_parameter_list_p[], /**< formal parameters list */
                              ecma_length_t formal_parameter_list_length, /**< length of formal parameters list */
                              bool is_strict, /**< flag indicating if function is declared in strict mode code */
                              bool is_configurable_bindings) /**< flag indicating whether function
                                                                  is declared in eval code */
{
  // b.
  ecma_object_t *func_obj_p = ecma_op_create_function_object (formal_parameter_list_p,
                                                              formal_parameter_list_length,
                                                              lex_env_p,
                                                              is_strict,
                                                              function_code_opcode_idx);

  // c.
  bool func_already_declared = ecma_op_has_binding (lex_env_p, function_name_p);

  // d.
  ecma_completion_value_t completion = ecma_make_empty_completion_value ();

  if (!func_already_declared)
  {
    completion = ecma_op_create_mutable_binding (lex_env_p,
                                                 function_name_p,
                                                 is_configurable_bindings);

    JERRY_ASSERT (ecma_is_completion_value_empty (completion));
  }
  else if (ecma_is_lexical_environment_global (lex_env_p))
  {
    // e.
    ecma_object_t *glob_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL);

    ecma_property_t *existing_prop_p = ecma_op_object_get_property (glob_obj_p, function_name_p);

    if (ecma_is_property_configurable (existing_prop_p))
    {
      ecma_property_descriptor_t property_desc = ecma_make_empty_property_descriptor ();
      {
        property_desc.is_value_defined = true;
        property_desc.value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

        property_desc.is_writable_defined = true;
        property_desc.is_writable = true;

        property_desc.is_enumerable_defined = true;
        property_desc.is_enumerable = true;

        property_desc.is_configurable_defined = true;
        property_desc.is_configurable = is_configurable_bindings;
      }

      completion = ecma_op_object_define_own_property (glob_obj_p,
                                                       function_name_p,
                                                       property_desc,
                                                       true);
    }
    else if (existing_prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR)
    {
      completion = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      JERRY_ASSERT (existing_prop_p->type == ECMA_PROPERTY_NAMEDDATA);

      if (!ecma_is_property_writable (existing_prop_p)
          || !ecma_is_property_enumerable (existing_prop_p))
      {
        completion = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
      }
    }

    ecma_deref_object (glob_obj_p);
  }

  ecma_completion_value_t ret_value;

  if (ecma_is_completion_value_throw (completion))
  {
    ret_value = completion;
  }
  else
  {
    JERRY_ASSERT (ecma_is_completion_value_empty (completion));

    // f.
    ret_value = ecma_op_set_mutable_binding (lex_env_p,
                                             function_name_p,
                                             ecma_make_object_value (func_obj_p),
                                             is_strict);
  }

  ecma_deref_object (func_obj_p);

  return ret_value;
} /* ecma_op_function_declaration */

/**
 * @}
 * @}
 */

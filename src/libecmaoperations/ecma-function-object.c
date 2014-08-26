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
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-global-object.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-magic-strings.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "globals.h"

/** \addtogroup ecma ---TODO---
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
  if (value.value_type != ECMA_TYPE_OBJECT)
  {
    return false;
  }

  ecma_object_t *obj_p = ECMA_GET_POINTER(value.value);

  JERRY_ASSERT(obj_p != NULL);
  JERRY_ASSERT(!obj_p->is_lexical_environment);

  return (obj_p->u.object.type == ECMA_OBJECT_TYPE_FUNCTION
          || obj_p->u.object.type == ECMA_OBJECT_TYPE_BOUND_FUNCTION);
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
  if (value.value_type != ECMA_TYPE_OBJECT)
  {
    return false;
  }

  ecma_object_t *obj_p = ECMA_GET_POINTER(value.value);

  JERRY_ASSERT(obj_p != NULL);
  JERRY_ASSERT(!obj_p->is_lexical_environment);

  return (obj_p->u.object.type == ECMA_OBJECT_TYPE_FUNCTION
          || obj_p->u.object.type == ECMA_OBJECT_TYPE_BOUND_FUNCTION);
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
  FIXME(Setup prototype of Function object to built-in Function prototype object (15.3.3.1));

  ecma_object_t *f = ecma_create_object (NULL, true, ECMA_OBJECT_TYPE_FUNCTION);

  // 2., 6., 7., 8.
  /*
   * We don't setup [[Get]], [[Call]], [[Construct]], [[HasInstance]] for each function object.
   * Instead we set the object's type to ECMA_OBJECT_TYPE_FUNCTION
   * that defines which version of the routine should be used on demand.
   */

  // 3.
  ecma_property_t *class_prop_p = ecma_create_internal_property (f, ECMA_INTERNAL_PROPERTY_CLASS);
  class_prop_p->u.internal_property.value = ECMA_OBJECT_CLASS_FUNCTION;

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
  length_prop_desc.is_writable_defined = false;
  length_prop_desc.writable = ECMA_PROPERTY_NOT_WRITABLE;
  length_prop_desc.is_enumerable_defined = false;
  length_prop_desc.enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
  length_prop_desc.is_configurable_defined = false;
  length_prop_desc.configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;

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
  FIXME(Use 'new Object ()' instead);
  ecma_object_t *proto_p = ecma_create_object (NULL, true, ECMA_OBJECT_TYPE_GENERAL);

  // 17.
  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();
  {
    prop_desc.is_value_defined = true;
    prop_desc.value = ecma_make_object_value (f);

    prop_desc.is_writable_defined = true;
    prop_desc.writable = ECMA_PROPERTY_WRITABLE;

    prop_desc.is_enumerable_defined = true;
    prop_desc.enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;

    prop_desc.is_configurable_defined = true;
    prop_desc.configurable = ECMA_PROPERTY_CONFIGURABLE;
  }

  ecma_string_t *magic_string_constructor_p = ecma_get_magic_string (ECMA_MAGIC_STRING_CONSTRUCTOR);
  ecma_op_object_define_own_property (proto_p,
                                      magic_string_constructor_p,
                                      prop_desc,
                                      false);
  ecma_deref_ecma_string (magic_string_constructor_p);

  // 18.
  prop_desc.value = ecma_make_object_value (proto_p);
  prop_desc.configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;
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
    ecma_object_t *thrower_p = ecma_op_get_throw_type_error ();

    prop_desc = ecma_make_empty_property_descriptor ();
    {
      prop_desc.is_enumerable_defined = true;
      prop_desc.enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;

      prop_desc.is_configurable_defined = true;
      prop_desc.configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;

      prop_desc.is_get_defined = true;
      prop_desc.get_p = thrower_p;

      prop_desc.is_set_defined = true;
      prop_desc.set_p = thrower_p;
    }

    ecma_string_t *magic_string_caller_p =ecma_get_magic_string (ECMA_MAGIC_STRING_CALLER) ;
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
    JERRY_ASSERT (formal_parameter_name_value.value_type == ECMA_TYPE_STRING);
    ecma_string_t *formal_parameter_name_string_p = ECMA_GET_POINTER (formal_parameter_name_value.value);
    
    ecma_completion_value_t arg_already_declared = ecma_op_has_binding (env_p,
                                                                        formal_parameter_name_string_p);
    if (!ecma_is_completion_value_normal_true (arg_already_declared))
    {
      ecma_completion_value_t completion = ecma_op_create_mutable_binding (env_p,
                                                                           formal_parameter_name_string_p,
                                                                           false);
      if (ecma_is_completion_value_throw (completion))
      {
        return completion;
      }

      JERRY_ASSERT (ecma_is_empty_completion_value (completion));

      completion = ecma_op_set_mutable_binding (env_p,
                                                formal_parameter_name_string_p,
                                                v,
                                                is_strict);

      if (ecma_is_completion_value_throw (completion))
      {
        return completion;
      }

      JERRY_ASSERT (ecma_is_empty_completion_value (completion));
    }
  }

  return ecma_make_empty_completion_value ();
} /* ecma_function_call_setup_args_variables */

/**
 * [[Call]] implementation for Function objects,
 * created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION)
 * or 15.3.4.5 (ECMA_OBJECT_TYPE_BOUND_FUNCTION).
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
  JERRY_ASSERT(func_obj_p != NULL && !func_obj_p->is_lexical_environment);
  JERRY_ASSERT(ecma_op_is_callable (ecma_make_object_value (func_obj_p)));
  JERRY_ASSERT(arguments_list_len == 0 || arguments_list_p != NULL);

  if (func_obj_p->u.object.type == ECMA_OBJECT_TYPE_FUNCTION)
  {
    ecma_completion_value_t ret_value;

    /* Entering Function Code (ECMA-262 v5, 10.4.3) */

    ecma_property_t *scope_prop_p = ecma_get_internal_property (func_obj_p, ECMA_INTERNAL_PROPERTY_SCOPE);
    ecma_property_t *code_prop_p = ecma_get_internal_property (func_obj_p, ECMA_INTERNAL_PROPERTY_CODE);

    ecma_object_t *scope_p = ECMA_GET_POINTER(scope_prop_p->u.internal_property.value);
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
      this_binding = ecma_make_object_value (ecma_get_global_object ());
    }
    else
    {
      // 3., 4.
      ecma_completion_value_t completion = ecma_op_to_object (this_arg_value);
      JERRY_ASSERT (ecma_is_completion_value_normal (completion));

      this_binding = completion.value;
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
    if (ecma_is_completion_value_normal (completion))
    {
      JERRY_ASSERT(ecma_is_empty_completion_value (completion));

      ret_value = ecma_make_completion_value (ECMA_COMPLETION_TYPE_RETURN,
                                              ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                                              ECMA_TARGET_ID_RESERVED);
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
  else
  {
    JERRY_ASSERT(func_obj_p->u.object.type == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    JERRY_UNIMPLEMENTED();
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
  JERRY_ASSERT(func_obj_p != NULL && !func_obj_p->is_lexical_environment);
  JERRY_ASSERT(ecma_op_is_callable (ecma_make_object_value (func_obj_p)));
  JERRY_ASSERT(arguments_list_len == 0 || arguments_list_p != NULL);

  if (func_obj_p->u.object.type == ECMA_OBJECT_TYPE_FUNCTION)
  {
    ecma_completion_value_t ret_value;

    ecma_string_t *prototype_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_PROTOTYPE);

    // 5.
    ECMA_TRY_CATCH (func_obj_prototype_prop_value,
                    ecma_op_object_get (func_obj_p,
                                        prototype_magic_string_p),
                    ret_value);

    //  6.
    ecma_object_t *prototype_p;
    if (func_obj_prototype_prop_value.value.value_type == ECMA_TYPE_OBJECT)
    {
      prototype_p = ECMA_GET_POINTER (func_obj_prototype_prop_value.value.value);
      ecma_ref_object (prototype_p);
    }
    else
    {
      // 7.
      FIXME (/* Set to built-in Object prototype (15.2.4) */);

      prototype_p = ecma_create_object (NULL, false, ECMA_OBJECT_TYPE_GENERAL);
    }

    // 1., 2., 4.
    ecma_object_t *obj_p = ecma_create_object (prototype_p, true, ECMA_OBJECT_TYPE_GENERAL);

    // 3.
    ecma_property_t *class_prop_p = ecma_create_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_CLASS);
    class_prop_p->u.internal_property.value = ECMA_OBJECT_CLASS_FUNCTION;

    ecma_deref_object (prototype_p);

    // 8.
    ECMA_FUNCTION_CALL (call_completion,
                        ecma_op_function_call (func_obj_p,
                                               ecma_make_object_value (obj_p),
                                               arguments_list_p,
                                               arguments_list_len),
                        ret_value);

    ecma_value_t obj_value;

    // 9.
    if (call_completion.value.value_type == ECMA_TYPE_OBJECT)
    {
      ecma_deref_object (obj_p);

      obj_value = ecma_copy_value (call_completion.value, true);
    }
    else
    {
      // 10.
      obj_value = ecma_make_object_value (obj_p);
    }

    ret_value = ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                            obj_value,
                                            ECMA_TARGET_ID_RESERVED);

    ECMA_FINALIZE (call_completion);
    ECMA_FINALIZE (func_obj_prototype_prop_value);

    return ret_value;
  }
  else
  {
    JERRY_ASSERT(func_obj_p->u.object.type == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    JERRY_UNIMPLEMENTED();
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
  bool func_already_declared = ecma_is_completion_value_normal_true (ecma_op_has_binding (lex_env_p,
                                                                                          function_name_p));


  // d.
  ecma_completion_value_t completion = ecma_make_empty_completion_value ();

  if (!func_already_declared)
  {
    completion = ecma_op_create_mutable_binding (lex_env_p,
                                                 function_name_p,
                                                 is_configurable_bindings);

    JERRY_ASSERT (ecma_is_empty_completion_value (completion));
  }
  else if (ecma_is_lexical_environment_global (lex_env_p))
  {
    // e.
    ecma_object_t *glob_obj_p = ecma_get_global_object ();

    ecma_property_t *existing_prop_p = ecma_op_object_get_property (glob_obj_p, function_name_p);

    if (ecma_is_property_configurable (existing_prop_p))
    {
      ecma_property_descriptor_t property_desc = ecma_make_empty_property_descriptor ();
      {
        property_desc.is_value_defined = true;
        property_desc.value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

        property_desc.is_writable_defined = true;
        property_desc.writable = ECMA_PROPERTY_WRITABLE;

        property_desc.is_enumerable_defined = true;
        property_desc.enumerable = ECMA_PROPERTY_ENUMERABLE;

        property_desc.is_configurable_defined = true;
        property_desc.configurable = (is_configurable_bindings ?
                                      ECMA_PROPERTY_CONFIGURABLE :
                                      ECMA_PROPERTY_NOT_CONFIGURABLE);
      }

      completion = ecma_op_object_define_own_property (glob_obj_p,
                                                       function_name_p,
                                                       property_desc,
                                                       true);
    }
    else if (existing_prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR)
    {
      completion = ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      JERRY_ASSERT (existing_prop_p->type == ECMA_PROPERTY_NAMEDDATA);

      if (existing_prop_p->u.named_data_property.writable != ECMA_PROPERTY_WRITABLE
          || existing_prop_p->u.named_data_property.enumerable != ECMA_PROPERTY_ENUMERABLE)
      {
        completion = ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
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
    JERRY_ASSERT (ecma_is_empty_completion_value (completion));

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
 * Get [[ThrowTypeError]] Function Object
 *
 * @return pointer to unique [[ThrowTypeError]] Function Object
 */
ecma_object_t*
ecma_op_get_throw_type_error (void)
{
  TODO(Create [[ThrowTypeError]] during engine initialization and return it from here);

  JERRY_UNIMPLEMENTED();
} /* ecma_op_get_throw_type_error */

/**
 * @}
 * @}
 */

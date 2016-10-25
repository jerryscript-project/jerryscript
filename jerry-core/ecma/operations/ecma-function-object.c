/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-objects-arguments.h"
#include "ecma-try-catch-macro.h"
#include "jcontext.h"

#define JERRY_INTERNAL
#include "jerry-internal.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmafunctionobject ECMA Function object related routines
 * @{
 */

/**
 * IsCallable operation.
 *
 * See also: ECMA-262 v5, 9.11
 *
 * @return true, if value is callable object;
 *         false - otherwise.
 */
bool
ecma_op_is_callable (ecma_value_t value) /**< ecma value */
{
  if (!ecma_is_value_object (value))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (value);

  JERRY_ASSERT (obj_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));

  return (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_FUNCTION
          || ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION
          || ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);
} /* ecma_op_is_callable */

/**
 * Check whether the value is Object that implements [[Construct]].
 *
 * @return true, if value is constructor object;
 *         false - otherwise.
 */
bool
ecma_is_constructor (ecma_value_t value) /**< ecma value */
{
  if (!ecma_is_value_object (value))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (value);

  JERRY_ASSERT (obj_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));

  if (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_FUNCTION)
  {
    return (!ecma_get_object_is_builtin (obj_p)
            || !ecma_builtin_function_is_routine (obj_p));
  }

  return (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION
          || ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);
} /* ecma_is_constructor */

/**
 * Function object creation operation.
 *
 * See also: ECMA-262 v5, 13.2
 *
 * @return pointer to newly created Function object
 */
ecma_object_t *
ecma_op_create_function_object (ecma_object_t *scope_p, /**< function's scope */
                                bool is_decl_in_strict_mode, /**< is function declared in strict mode code? */
                                const ecma_compiled_code_t *bytecode_data_p) /**< byte-code array */
{
  bool is_strict_mode_code = is_decl_in_strict_mode;

  if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE)
  {
    is_strict_mode_code = true;
  }

  // 1., 4., 13.
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);

  ecma_object_t *func_p = ecma_create_object (prototype_obj_p,
                                              sizeof (ecma_extended_object_t),
                                              ECMA_OBJECT_TYPE_FUNCTION);

  ecma_deref_object (prototype_obj_p);

  // 2., 6., 7., 8.
  /*
   * We don't setup [[Get]], [[Call]], [[Construct]], [[HasInstance]] for each function object.
   * Instead we set the object's type to ECMA_OBJECT_TYPE_FUNCTION
   * that defines which version of the routine should be used on demand.
   */

  // 3.
  /*
   * [[Class]] property is not stored explicitly for objects of ECMA_OBJECT_TYPE_FUNCTION type.
   *
   * See also: ecma_object_get_class_name
   */

  ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) func_p;

  // 9.
  ECMA_SET_INTERNAL_VALUE_POINTER (ext_func_p->u.function.scope_cp, scope_p);

  // 10., 11., 12.
  ECMA_SET_INTERNAL_VALUE_POINTER (ext_func_p->u.function.bytecode_cp, bytecode_data_p);
  ecma_bytecode_ref ((ecma_compiled_code_t *) bytecode_data_p);

  // 14., 15., 16., 17., 18.
  /*
   * 'length' and 'prototype' properties are instantiated lazily
   *
   * See also: ecma_op_function_try_lazy_instantiate_property
   */

  // 19.
  if (is_strict_mode_code)
  {
    ecma_object_t *thrower_p = ecma_builtin_get (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER);

    ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();
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

    ecma_string_t *magic_string_caller_p = ecma_get_magic_string (LIT_MAGIC_STRING_CALLER);
    ecma_op_object_define_own_property (func_p,
                                        magic_string_caller_p,
                                        &prop_desc,
                                        false);
    ecma_deref_ecma_string (magic_string_caller_p);

    ecma_string_t *magic_string_arguments_p = ecma_get_magic_string (LIT_MAGIC_STRING_ARGUMENTS);
    ecma_op_object_define_own_property (func_p,
                                        magic_string_arguments_p,
                                        &prop_desc,
                                        false);
    ecma_deref_ecma_string (magic_string_arguments_p);

    ecma_deref_object (thrower_p);
  }

  return func_p;
} /* ecma_op_create_function_object */

/**
 * List names of a Function object's lazy instantiated properties,
 * adding them to corresponding string collections
 *
 * See also:
 *          ecma_op_function_try_lazy_instantiate_property
 */
void
ecma_op_function_list_lazy_property_names (bool separate_enumerable, /**< true - list enumerable properties into
                                                                      *          main collection and non-enumerable
                                                                      *          to collection of 'skipped
                                                                      *          non-enumerable' properties,
                                                                      *   false - list all properties into main
                                                                      *           collection.
                                                                      */
                                           ecma_collection_header_t *main_collection_p, /**< 'main' collection */
                                           ecma_collection_header_t *non_enum_collection_p) /**< skipped
                                                                                             *   'non-enumerable'
                                                                                             *   collection */
{
  ecma_collection_header_t *for_enumerable_p = main_collection_p;
  JERRY_UNUSED (for_enumerable_p);

  ecma_collection_header_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

  ecma_string_t *name_p;

  /* 'length' property is non-enumerable (ECMA-262 v5, 13.2.5) */
  name_p = ecma_new_ecma_length_string ();
  ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (name_p), true);
  ecma_deref_ecma_string (name_p);

  /* 'prototype' property is non-enumerable (ECMA-262 v5, 13.2.18) */
  name_p = ecma_get_magic_string (LIT_MAGIC_STRING_PROTOTYPE);
  ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (name_p), true);
  ecma_deref_ecma_string (name_p);
} /* ecma_op_function_list_lazy_property_names */

/**
 * Lazy instantation of non-builtin ecma function object's properties
 *
 * Warning:
 *         Only non-configurable properties could be instantiated lazily in this function,
 *         as configurable properties could be deleted and it would be incorrect
 *         to reinstantiate them in the function in second time.
 *
 * @return pointer to newly instantiated property, if a property was instantiated,
 *         NULL - otherwise
 */
ecma_property_t *
ecma_op_function_try_lazy_instantiate_property (ecma_object_t *object_p, /**< the function object */
                                                ecma_string_t *property_name_p) /**< property name */
{
  static const char prototype_str_p[] = "prototype";

  JERRY_ASSERT (!ecma_get_object_is_builtin (object_p));

  ecma_string_container_t container = ECMA_STRING_GET_CONTAINER (property_name_p);

  /* Check whether the property_name_p is prototype */
  if (container == ECMA_STRING_CONTAINER_MAGIC_STRING)
  {
    if (property_name_p->u.magic_string_id != LIT_MAGIC_STRING_PROTOTYPE)
    {
      return NULL;
    }
  }
  else if (container != ECMA_STRING_CONTAINER_HEAP_UTF8_STRING
           || property_name_p->u.utf8_string.size != (sizeof (prototype_str_p) - 1))
  {
    return NULL;
  }
  else
  {
    if (strncmp ((char *) (property_name_p + 1), prototype_str_p, (sizeof (prototype_str_p) - 1)) != 0)
    {
      return NULL;
    }
  }

  /* ECMA-262 v5, 13.2, 16-18 */

  /* 16. */
  ecma_object_t *proto_object_p = ecma_op_create_object_object_noarg ();

  /* 17. */
  ecma_string_t *magic_string_constructor_p = ecma_get_magic_string (LIT_MAGIC_STRING_CONSTRUCTOR);

  ecma_property_value_t *constructor_prop_value_p;
  constructor_prop_value_p = ecma_create_named_data_property (proto_object_p,
                                                              magic_string_constructor_p,
                                                              ECMA_PROPERTY_CONFIGURABLE_WRITABLE,
                                                              NULL);

  constructor_prop_value_p->value = ecma_make_object_value (object_p);

  ecma_deref_ecma_string (magic_string_constructor_p);

  /* 18. */
  ecma_property_t *prototype_prop_p;
  ecma_property_value_t *prototype_prop_value_p;
  prototype_prop_value_p = ecma_create_named_data_property (object_p,
                                                            property_name_p,
                                                            ECMA_PROPERTY_FLAG_WRITABLE,
                                                            &prototype_prop_p);

  prototype_prop_value_p->value = ecma_make_object_value (proto_object_p);

  ecma_deref_object (proto_object_p);

  return prototype_prop_p;
} /* ecma_op_function_try_lazy_instantiate_property */

/**
 * External function object creation operation.
 *
 * Note:
 *      external function object is implementation-defined object type
 *      that represent functions implemented in native code, using Embedding API
 *
 * @return pointer to newly created external function object
 */
ecma_object_t *
ecma_op_create_external_function_object (ecma_external_pointer_t code_p) /**< pointer to external native handler */
{
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);

  ecma_object_t *function_obj_p;
  function_obj_p = ecma_create_object (prototype_obj_p,
                                       sizeof (ecma_extended_object_t),
                                       ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);

  ecma_deref_object (prototype_obj_p);

  /*
   * [[Class]] property is not stored explicitly for objects of ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION type.
   *
   * See also: ecma_object_get_class_name
   */

  ecma_extended_object_t *ext_func_obj_p = (ecma_extended_object_t *) function_obj_p;
  ext_func_obj_p->u.external_function = code_p;

  ecma_string_t *magic_string_prototype_p = ecma_get_magic_string (LIT_MAGIC_STRING_PROTOTYPE);
  ecma_builtin_helper_def_prop (function_obj_p,
                                magic_string_prototype_p,
                                ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                                true, /* Writable */
                                false, /* Enumerable */
                                false, /* Configurable */
                                false); /* Failure handling */

  ecma_deref_ecma_string (magic_string_prototype_p);

  return function_obj_p;
} /* ecma_op_create_external_function_object */

/**
 * [[Call]] implementation for Function objects,
 * created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION)
 * or 15.3.4.5 (ECMA_OBJECT_TYPE_BOUND_FUNCTION),
 * and for built-in Function objects
 * from section 15 (ECMA_OBJECT_TYPE_FUNCTION).
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_function_has_instance (ecma_object_t *func_obj_p, /**< Function object */
                               ecma_value_t value) /**< argument 'V' */
{
  JERRY_ASSERT (func_obj_p != NULL
                && !ecma_is_lexical_environment (func_obj_p));

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION)
  {
    if (ecma_get_object_is_builtin (func_obj_p)
        && ecma_builtin_function_is_routine (func_obj_p))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Function parameter cannot be built-in a function."));
    }

    if (!ecma_is_value_object (value))
    {
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
    }

    ecma_object_t *v_obj_p = ecma_get_object_from_value (value);

    ecma_string_t *prototype_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_PROTOTYPE);

    ECMA_TRY_CATCH (prototype_obj_value,
                    ecma_op_object_get (func_obj_p, prototype_magic_string_p),
                    ret_value);

    if (!ecma_is_value_object (prototype_obj_value))
    {
      ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Expected an object."));
    }
    else
    {
      ecma_object_t *prototype_obj_p = ecma_get_object_from_value (prototype_obj_value);
      JERRY_ASSERT (prototype_obj_p != NULL);

      do
      {
        v_obj_p = ecma_get_object_prototype (v_obj_p);

        if (v_obj_p == NULL)
        {
          ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);

          break;
        }
        else if (v_obj_p == prototype_obj_p)
        {
          ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);

          break;
        }
      } while (true);
    }

    ECMA_FINALIZE (prototype_obj_value);

    ecma_deref_ecma_string (prototype_magic_string_p);
  }
  else if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION)
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Expected a function."));
  }
  else
  {
    JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    /* 1. */
    ecma_extended_object_t *ext_function_p = (ecma_extended_object_t *) func_obj_p;

    ecma_object_t *target_func_obj_p;
    target_func_obj_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                         ext_function_p->u.bound_function.target_function);

    /* 3. */
    ret_value = ecma_op_object_has_instance (target_func_obj_p, value);
  }

  return ret_value;
} /* ecma_op_function_has_instance */

/**
 * [[Call]] implementation for Function objects,
 * created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION)
 * or 15.3.4.5 (ECMA_OBJECT_TYPE_BOUND_FUNCTION),
 * and for built-in Function objects
 * from section 15 (ECMA_OBJECT_TYPE_FUNCTION).
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_function_call (ecma_object_t *func_obj_p, /**< Function object */
                       ecma_value_t this_arg_value, /**< 'this' argument's value */
                       const ecma_value_t *arguments_list_p, /**< arguments list */
                       ecma_length_t arguments_list_len) /**< length of arguments list */
{
  JERRY_ASSERT (func_obj_p != NULL
                && !ecma_is_lexical_environment (func_obj_p));
  JERRY_ASSERT (ecma_op_is_callable (ecma_make_object_value (func_obj_p)));

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION)
  {
    if (unlikely (ecma_get_object_is_builtin (func_obj_p)))
    {
      ret_value = ecma_builtin_dispatch_call (func_obj_p,
                                              this_arg_value,
                                              arguments_list_p,
                                              arguments_list_len);
    }
    else
    {
      /* Entering Function Code (ECMA-262 v5, 10.4.3) */
      ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) func_obj_p;

      ecma_object_t *scope_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                                ext_func_p->u.function.scope_cp);

      // 8.
      ecma_value_t this_binding;
      bool is_strict;
      bool is_no_lex_env;

      const ecma_compiled_code_t *bytecode_data_p;
      bytecode_data_p = ECMA_GET_INTERNAL_VALUE_POINTER (const ecma_compiled_code_t,
                                                         ext_func_p->u.function.bytecode_cp);

      is_strict = (bytecode_data_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE) ? true : false;
      is_no_lex_env = (bytecode_data_p->status_flags & CBC_CODE_FLAGS_LEXICAL_ENV_NOT_NEEDED) ? true : false;

      // 1.
      if (is_strict)
      {
        this_binding = ecma_copy_value (this_arg_value);
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
        this_binding = ecma_op_to_object (this_arg_value);

        JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (this_binding));
      }

      // 5.
      ecma_object_t *local_env_p;
      if (is_no_lex_env)
      {
        local_env_p = scope_p;
      }
      else
      {
        local_env_p = ecma_create_decl_lex_env (scope_p);
        if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_ARGUMENTS_NEEDED)
        {
          ecma_op_create_arguments_object (func_obj_p,
                                           local_env_p,
                                           arguments_list_p,
                                           arguments_list_len,
                                           bytecode_data_p);
        }
      }

      ret_value = vm_run (bytecode_data_p,
                          this_binding,
                          local_env_p,
                          false,
                          arguments_list_p,
                          arguments_list_len);

      if (!is_no_lex_env)
      {
        ecma_deref_object (local_env_p);
      }

      ecma_free_value (this_binding);
    }
  }
  else if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION)
  {
    ecma_extended_object_t *ext_func_obj_p = (ecma_extended_object_t *) func_obj_p;

    ret_value = jerry_dispatch_external_function (func_obj_p,
                                                  ext_func_obj_p->u.external_function,
                                                  this_arg_value,
                                                  arguments_list_p,
                                                  arguments_list_len);
  }
  else
  {
    JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);
    JERRY_CONTEXT (is_direct_eval_form_call) = false;

    /* 2-3. */
    ecma_extended_object_t *ext_function_p = (ecma_extended_object_t *) func_obj_p;

    ecma_object_t *target_func_obj_p;
    target_func_obj_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                         ext_function_p->u.bound_function.target_function);

    /* 4. */
    ecma_value_t *args_p = (ecma_value_t *) (ext_function_p + 1);

    ecma_value_t bound_this_value = *args_p;
    ecma_length_t args_length = ext_function_p->u.bound_function.args_length;

    JERRY_ASSERT (args_length > 0);

    if (args_length > 1)
    {
      args_length--;
      ecma_length_t merged_args_list_len = args_length + arguments_list_len;

      JMEM_DEFINE_LOCAL_ARRAY (merged_args_list_p, merged_args_list_len, ecma_value_t);

      memcpy (merged_args_list_p, args_p + 1, args_length * sizeof (ecma_value_t));
      memcpy (merged_args_list_p + args_length, arguments_list_p, arguments_list_len * sizeof (ecma_value_t));

      /* 5. */
      ret_value = ecma_op_function_call (target_func_obj_p,
                                         bound_this_value,
                                         merged_args_list_p,
                                         merged_args_list_len);

      JMEM_FINALIZE_LOCAL_ARRAY (merged_args_list_p);
    }
    else
    {
      /* 5. */
      ret_value = ecma_op_function_call (target_func_obj_p,
                                         bound_this_value,
                                         arguments_list_p,
                                         arguments_list_len);
    }
  }

  JERRY_ASSERT (!ecma_is_value_empty (ret_value));

  return ret_value;
} /* ecma_op_function_call */

/**
 * [[Construct]] implementation for Function objects (13.2.2),
 * created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION) and
 * externally defined (host) functions (ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION).
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
ecma_op_function_construct_simple_or_external (ecma_object_t *func_obj_p, /**< Function object */
                                               const ecma_value_t *arguments_list_p, /**< arguments list */
                                               ecma_length_t arguments_list_len) /**< length of arguments list */
{
  JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION
                || ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ecma_string_t *prototype_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_PROTOTYPE);

  // 5.
  ECMA_TRY_CATCH (func_obj_prototype_prop_value,
                  ecma_op_object_get (func_obj_p,
                                      prototype_magic_string_p),
                  ret_value);

  // 1., 2., 4.
  ecma_object_t *obj_p;
  if (ecma_is_value_object (func_obj_prototype_prop_value))
  {
    //  6.
    obj_p = ecma_create_object (ecma_get_object_from_value (func_obj_prototype_prop_value),
                                0,
                                ECMA_OBJECT_TYPE_GENERAL);
  }
  else
  {
    // 7.
    ecma_object_t *prototype_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);

    obj_p = ecma_create_object (prototype_p, 0, ECMA_OBJECT_TYPE_GENERAL);

    ecma_deref_object (prototype_p);
  }

  // 3.
  /*
   * [[Class]] property of ECMA_OBJECT_TYPE_GENERAL type objects
   * without ECMA_INTERNAL_PROPERTY_CLASS internal property
   * is "Object".
   *
   * See also: ecma_object_get_class_name.
   */

  // 8.
  ECMA_TRY_CATCH (call_completion,
                  ecma_op_function_call (func_obj_p,
                                         ecma_make_object_value (obj_p),
                                         arguments_list_p,
                                         arguments_list_len),
                  ret_value);

  // 9.
  if (ecma_is_value_object (call_completion))
  {
    ret_value = ecma_copy_value (call_completion);
  }
  else
  {
    // 10.
    ecma_ref_object (obj_p);
    ret_value = ecma_make_object_value (obj_p);
  }

  ECMA_FINALIZE (call_completion);

  ecma_deref_object (obj_p);

  ECMA_FINALIZE (func_obj_prototype_prop_value);

  ecma_deref_ecma_string (prototype_magic_string_p);

  return ret_value;
} /* ecma_op_function_construct_simple_or_external */

/**
 * [[Construct]] implementation:
 *   13.2.2 - for Function objects, created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION),
 *            and externally defined host functions (ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);
 *   15.3.4.5.1 - for Function objects, created through 15.3.4.5 (ECMA_OBJECT_TYPE_BOUND_FUNCTION).
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_function_construct (ecma_object_t *func_obj_p, /**< Function object */
                            const ecma_value_t *arguments_list_p, /**< arguments list */
                            ecma_length_t arguments_list_len) /**< length of arguments list */
{
  JERRY_ASSERT (func_obj_p != NULL
                && !ecma_is_lexical_environment (func_obj_p));
  JERRY_ASSERT (ecma_is_constructor (ecma_make_object_value (func_obj_p)));

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION)
  {
    if (unlikely (ecma_get_object_is_builtin (func_obj_p)
                  && !ecma_builtin_function_is_routine (func_obj_p)))
    {
      ret_value = ecma_builtin_dispatch_construct (func_obj_p,
                                                   arguments_list_p,
                                                   arguments_list_len);
    }
    else
    {
      ret_value = ecma_op_function_construct_simple_or_external (func_obj_p,
                                                                 arguments_list_p,
                                                                 arguments_list_len);
    }
  }
  else if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION)
  {
    ret_value = ecma_op_function_construct_simple_or_external (func_obj_p,
                                                               arguments_list_p,
                                                               arguments_list_len);
  }
  else
  {
    JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    /* 1. */
    ecma_extended_object_t *ext_function_p = (ecma_extended_object_t *) func_obj_p;

    ecma_object_t *target_func_obj_p;
    target_func_obj_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                         ext_function_p->u.bound_function.target_function);

    /* 2. */
    if (!ecma_is_constructor (ecma_make_object_value (target_func_obj_p)))
    {
      ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Expected a constructor."));
    }
    else
    {
      /* 4. */
      ecma_length_t args_length = ext_function_p->u.bound_function.args_length;

      JERRY_ASSERT (args_length > 0);

      if (args_length > 1)
      {
        ecma_value_t *args_p = (ecma_value_t *) (ext_function_p + 1);

        args_length--;
        ecma_length_t merged_args_list_len = args_length + arguments_list_len;

        JMEM_DEFINE_LOCAL_ARRAY (merged_args_list_p, merged_args_list_len, ecma_value_t);

        memcpy (merged_args_list_p, args_p + 1, args_length * sizeof (ecma_value_t));
        memcpy (merged_args_list_p + args_length, arguments_list_p, arguments_list_len * sizeof (ecma_value_t));

        /* 5. */
        ret_value = ecma_op_function_construct (target_func_obj_p,
                                                merged_args_list_p,
                                                merged_args_list_len);

        JMEM_FINALIZE_LOCAL_ARRAY (merged_args_list_p);
      }
      else
      {
        /* 5. */
        ret_value = ecma_op_function_construct (target_func_obj_p,
                                                arguments_list_p,
                                                arguments_list_len);
      }
    }
  }

  return ret_value;
} /* ecma_op_function_construct */

/**
 * @}
 * @}
 */

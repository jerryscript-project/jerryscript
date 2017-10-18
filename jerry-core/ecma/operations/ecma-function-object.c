/* Copyright JS Foundation and other contributors, http://js.foundation
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

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmafunctionobject ECMA Function object related routines
 * @{
 */

/**
 * Checks whether the type is a normal or arrow function.
 *
 * @return true - if the type is a normal or arrow function;
 *         false - otherwise
 */
inline bool __attr_always_inline___
ecma_is_normal_or_arrow_function (ecma_object_type_t type)
{
#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
  return (type == ECMA_OBJECT_TYPE_FUNCTION || type == ECMA_OBJECT_TYPE_ARROW_FUNCTION);
#else /* CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
  return (type == ECMA_OBJECT_TYPE_FUNCTION);
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
} /* ecma_is_normal_or_arrow_function */

/**
 * IsCallable operation.
 *
 * See also: ECMA-262 v5, 9.11
 *
 * @return true - if value is callable object;
 *         false - otherwise
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

  ecma_object_type_t type = ecma_get_object_type (obj_p);

  return (type == ECMA_OBJECT_TYPE_FUNCTION
#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
          || type == ECMA_OBJECT_TYPE_ARROW_FUNCTION
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
          || type == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION
          || type == ECMA_OBJECT_TYPE_BOUND_FUNCTION);
} /* ecma_op_is_callable */

/**
 * Checks whether the value is Object that implements [[Construct]].
 *
 * @return true - if value is constructor object;
 *         false - otherwise
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
                                const ecma_compiled_code_t *bytecode_data_p) /**< byte-code array */
{
  /* 1., 4., 13. */
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);

  ecma_object_t *func_p = ecma_create_object (prototype_obj_p,
                                              sizeof (ecma_extended_object_t),
                                              ECMA_OBJECT_TYPE_FUNCTION);

  ecma_deref_object (prototype_obj_p);

  /* 2., 6., 7., 8. */
  /*
   * We don't setup [[Get]], [[Call]], [[Construct]], [[HasInstance]] for each function object.
   * Instead we set the object's type to ECMA_OBJECT_TYPE_FUNCTION
   * that defines which version of the routine should be used on demand.
   */

  /* 3. */
  /*
   * [[Class]] property is not stored explicitly for objects of ECMA_OBJECT_TYPE_FUNCTION type.
   *
   * See also: ecma_object_get_class_name
   */

  ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) func_p;

  /* 9. */
  ECMA_SET_INTERNAL_VALUE_POINTER (ext_func_p->u.function.scope_cp, scope_p);

  /* 10., 11., 12. */
  ECMA_SET_INTERNAL_VALUE_POINTER (ext_func_p->u.function.bytecode_cp, bytecode_data_p);
  ecma_bytecode_ref ((ecma_compiled_code_t *) bytecode_data_p);

  /* 14., 15., 16., 17., 18. */
  /*
   * 'length' and 'prototype' properties are instantiated lazily
   *
   * See also: ecma_op_function_try_to_lazy_instantiate_property
   */

  return func_p;
} /* ecma_op_create_function_object */

#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION

/**
 * Arrow function object creation operation.
 *
 * See also: ES2015, 9.2.12
 *
 * @return pointer to newly created Function object
 */
ecma_object_t *
ecma_op_create_arrow_function_object (ecma_object_t *scope_p, /**< function's scope */
                                      const ecma_compiled_code_t *bytecode_data_p, /**< byte-code array */
                                      ecma_value_t this_binding) /**< value of 'this' binding */
{
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);

  ecma_object_t *func_p = ecma_create_object (prototype_obj_p,
                                              sizeof (ecma_arrow_function_t),
                                              ECMA_OBJECT_TYPE_ARROW_FUNCTION);

  ecma_deref_object (prototype_obj_p);


  ecma_arrow_function_t *arrow_func_p = (ecma_arrow_function_t *) func_p;

  ECMA_SET_NON_NULL_POINTER (arrow_func_p->scope_cp, scope_p);

  ECMA_SET_NON_NULL_POINTER (arrow_func_p->bytecode_cp, bytecode_data_p);
  ecma_bytecode_ref ((ecma_compiled_code_t *) bytecode_data_p);

  arrow_func_p->this_binding = ecma_copy_value_if_not_object (this_binding);
  return func_p;
} /* ecma_op_create_arrow_function_object */

#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */

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
ecma_op_create_external_function_object (ecma_external_handler_t handler_cb) /**< pointer to external native handler */
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
  ext_func_obj_p->u.external_handler_cb = handler_cb;

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

  if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION)
  {
    JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    /* 1. */
    ecma_extended_object_t *ext_function_p = (ecma_extended_object_t *) func_obj_p;

    ecma_object_t *target_func_obj_p;
    target_func_obj_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                         ext_function_p->u.bound_function.target_function);

    /* 3. */
    return ecma_op_object_has_instance (target_func_obj_p, value);
  }

  JERRY_ASSERT (ecma_is_normal_or_arrow_function (ecma_get_object_type (func_obj_p))
                || ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);

  if (!ecma_is_value_object (value))
  {
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
  }

  ecma_object_t *v_obj_p = ecma_get_object_from_value (value);

  ecma_string_t prototype_magic_string;
  ecma_init_ecma_magic_string (&prototype_magic_string, LIT_MAGIC_STRING_PROTOTYPE);

  ecma_value_t prototype_obj_value = ecma_op_object_get (func_obj_p, &prototype_magic_string);

  if (ECMA_IS_VALUE_ERROR (prototype_obj_value))
  {
    return prototype_obj_value;
  }

  if (!ecma_is_value_object (prototype_obj_value))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Object expected."));
  }

  ecma_object_t *prototype_obj_p = ecma_get_object_from_value (prototype_obj_value);
  JERRY_ASSERT (prototype_obj_p != NULL);

  bool result = false;

  while (true)
  {
    v_obj_p = ecma_get_object_prototype (v_obj_p);

    if (v_obj_p == NULL)
    {
      break;
    }

    if (v_obj_p == prototype_obj_p)
    {
      result = true;
      break;
    }
  }

  ecma_deref_object (prototype_obj_p);
  return ecma_make_boolean_value (result);
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

      /* 8. */
      ecma_value_t this_binding;
      bool is_strict;
      bool is_no_lex_env;

      const ecma_compiled_code_t *bytecode_data_p;
      bytecode_data_p = ECMA_GET_INTERNAL_VALUE_POINTER (const ecma_compiled_code_t,
                                                         ext_func_p->u.function.bytecode_cp);

      is_strict = (bytecode_data_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE) ? true : false;
      is_no_lex_env = (bytecode_data_p->status_flags & CBC_CODE_FLAGS_LEXICAL_ENV_NOT_NEEDED) ? true : false;

      /* 1. */
      if (is_strict)
      {
        this_binding = ecma_copy_value (this_arg_value);
      }
      else if (ecma_is_value_undefined (this_arg_value)
               || ecma_is_value_null (this_arg_value))
      {
        /* 2. */
        this_binding = ecma_make_object_value (ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL));
      }
      else
      {
        /* 3., 4. */
        this_binding = ecma_op_to_object (this_arg_value);

        JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (this_binding));
      }

      /* 5. */
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
#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
  else if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_ARROW_FUNCTION)
  {
    /* Entering Function Code (ES2015, 9.2.1) */
    ecma_arrow_function_t *arrow_func_p = (ecma_arrow_function_t *) func_obj_p;

    ecma_object_t *scope_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                        arrow_func_p->scope_cp);

    bool is_no_lex_env;

    const ecma_compiled_code_t *bytecode_data_p;
    bytecode_data_p = ECMA_GET_NON_NULL_POINTER (const ecma_compiled_code_t,
                                                 arrow_func_p->bytecode_cp);

    is_no_lex_env = (bytecode_data_p->status_flags & CBC_CODE_FLAGS_LEXICAL_ENV_NOT_NEEDED) ? true : false;

    ecma_object_t *local_env_p;
    if (is_no_lex_env)
    {
      local_env_p = scope_p;
    }
    else
    {
      local_env_p = ecma_create_decl_lex_env (scope_p);

      JERRY_ASSERT (!(bytecode_data_p->status_flags & CBC_CODE_FLAGS_ARGUMENTS_NEEDED));
    }

    ret_value = vm_run (bytecode_data_p,
                        arrow_func_p->this_binding,
                        local_env_p,
                        false,
                        arguments_list_p,
                        arguments_list_len);

    if (!is_no_lex_env)
    {
      ecma_deref_object (local_env_p);
    }
  }
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
  else if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION)
  {
    ecma_extended_object_t *ext_func_obj_p = (ecma_extended_object_t *) func_obj_p;

    ret_value = ext_func_obj_p->u.external_handler_cb (ecma_make_object_value (func_obj_p),
                                                       this_arg_value,
                                                       arguments_list_p,
                                                       arguments_list_len);

    if (unlikely (ecma_is_value_error_reference (ret_value)))
    {
      JERRY_CONTEXT (error_value) = ecma_clear_error_reference (ret_value);
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_ERROR);
    }
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
    ecma_value_t args_len_or_this = ext_function_p->u.bound_function.args_len_or_this;
    ecma_value_t bound_this_value;
    ecma_length_t args_length;

    if (!ecma_is_value_integer_number (args_len_or_this))
    {
      bound_this_value = args_len_or_this;
      args_length = 1;
    }
    else
    {
      bound_this_value = *(ecma_value_t *) (ext_function_p + 1);
      args_length = (ecma_length_t) ecma_get_integer_from_value (args_len_or_this);
    }

    JERRY_ASSERT (args_length > 0);

    if (args_length > 1)
    {
      args_length--;
      ecma_length_t merged_args_list_len = args_length + arguments_list_len;

      JMEM_DEFINE_LOCAL_ARRAY (merged_args_list_p, merged_args_list_len, ecma_value_t);

      ecma_value_t *args_p = (ecma_value_t *) (ext_function_p + 1);

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

  /* 5. */
  ECMA_TRY_CATCH (func_obj_prototype_prop_value,
                  ecma_op_object_get (func_obj_p,
                                      prototype_magic_string_p),
                  ret_value);

  /* 1., 2., 4. */
  ecma_object_t *obj_p;
  if (ecma_is_value_object (func_obj_prototype_prop_value))
  {
    /* 6. */
    obj_p = ecma_create_object (ecma_get_object_from_value (func_obj_prototype_prop_value),
                                0,
                                ECMA_OBJECT_TYPE_GENERAL);
  }
  else
  {
    /* 7. */
    ecma_object_t *prototype_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);

    obj_p = ecma_create_object (prototype_p, 0, ECMA_OBJECT_TYPE_GENERAL);

    ecma_deref_object (prototype_p);
  }

  /* 3. */
  /*
   * [[Class]] property of ECMA_OBJECT_TYPE_GENERAL type objects
   * without ECMA_INTERNAL_PROPERTY_CLASS internal property
   * is "Object".
   *
   * See also: ecma_object_get_class_name.
   */

  /* 8. */
  ECMA_TRY_CATCH (call_completion,
                  ecma_op_function_call (func_obj_p,
                                         ecma_make_object_value (obj_p),
                                         arguments_list_p,
                                         arguments_list_len),
                  ret_value);

  /* 9. */
  if (ecma_is_value_object (call_completion))
  {
    ret_value = ecma_copy_value (call_completion);
  }
  else
  {
    /* 10. */
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
      ecma_value_t args_len_or_this = ext_function_p->u.bound_function.args_len_or_this;

      ecma_length_t args_length = 1;

      if (ecma_is_value_integer_number (args_len_or_this))
      {
        args_length = (ecma_length_t) ecma_get_integer_from_value (args_len_or_this);
      }

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
ecma_op_function_try_to_lazy_instantiate_property (ecma_object_t *object_p, /**< the function object */
                                                   ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (!ecma_get_object_is_builtin (object_p));

  if (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_PROTOTYPE))
  {
    /* ECMA-262 v5, 13.2, 16-18 */

    /* 16. */
    ecma_object_t *proto_object_p = ecma_op_create_object_object_noarg ();

    /* 17. */
    ecma_string_t magic_string_constructor;
    ecma_init_ecma_magic_string (&magic_string_constructor, LIT_MAGIC_STRING_CONSTRUCTOR);

    ecma_property_value_t *constructor_prop_value_p;
    constructor_prop_value_p = ecma_create_named_data_property (proto_object_p,
                                                                &magic_string_constructor,
                                                                ECMA_PROPERTY_CONFIGURABLE_WRITABLE,
                                                                NULL);

    constructor_prop_value_p->value = ecma_make_object_value (object_p);

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
  }

  if (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_CALLER)
      || ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_ARGUMENTS))
  {
    ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) object_p;

    const ecma_compiled_code_t *bytecode_data_p;
    bytecode_data_p = ECMA_GET_INTERNAL_VALUE_POINTER (const ecma_compiled_code_t,
                                                       ext_func_p->u.function.bytecode_cp);

    if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE)
    {
      ecma_object_t *thrower_p = ecma_builtin_get (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER);

      ecma_property_t *caller_prop_p;
      /* The property_name_p argument contans the name. */
      ecma_create_named_accessor_property (object_p,
                                           property_name_p,
                                           thrower_p,
                                           thrower_p,
                                           ECMA_PROPERTY_FIXED,
                                           &caller_prop_p);

      ecma_deref_object (thrower_p);
      return caller_prop_p;
    }
  }

  return NULL;
} /* ecma_op_function_try_to_lazy_instantiate_property */

/**
 * Create specification defined non-configurable properties for external functions.
 *
 * See also:
 *          ECMA-262 v5, 15.3.4.5
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t *
ecma_op_external_function_try_to_lazy_instantiate_property (ecma_object_t *object_p, /**< object */
                                                            ecma_string_t *property_name_p) /**< property's name */
{
  JERRY_ASSERT (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);

  if (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_PROTOTYPE))
  {
    ecma_property_t *prototype_prop_p;
    ecma_property_value_t *prototype_prop_value_p;
    prototype_prop_value_p = ecma_create_named_data_property (object_p,
                                                              property_name_p,
                                                              ECMA_PROPERTY_FLAG_WRITABLE,
                                                              &prototype_prop_p);

    prototype_prop_value_p->value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    return prototype_prop_p;
  }

  return NULL;
} /* ecma_op_external_function_try_to_lazy_instantiate_property */

/**
 * Create specification defined non-configurable properties for bound functions.
 *
 * See also:
 *          ECMA-262 v5, 15.3.4.5
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t *
ecma_op_bound_function_try_to_lazy_instantiate_property (ecma_object_t *object_p, /**< object */
                                                         ecma_string_t *property_name_p) /**< property's name */
{
  JERRY_ASSERT (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

  if (ecma_string_is_length (property_name_p))
  {
    ecma_extended_object_t *ext_function_p = (ecma_extended_object_t *) object_p;
    ecma_object_t *target_func_obj_p;
    target_func_obj_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                         ext_function_p->u.bound_function.target_function);

    ecma_integer_value_t length = 0;

    if (ecma_object_get_class_name (target_func_obj_p) == LIT_MAGIC_STRING_FUNCTION_UL)
    {
      /* The property_name_p argument contans the 'length' string. */
      ecma_value_t get_len_value = ecma_op_object_get (target_func_obj_p, property_name_p);

      JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (get_len_value));
      JERRY_ASSERT (ecma_is_value_integer_number (get_len_value));

      ecma_value_t args_len_or_this = ext_function_p->u.bound_function.args_len_or_this;
      ecma_integer_value_t args_length = 1;

      if (ecma_is_value_integer_number (args_len_or_this))
      {
        args_length = ecma_get_integer_from_value (args_len_or_this);
      }

      length = ecma_get_integer_from_value (get_len_value) - (args_length - 1);

      if (length < 0)
      {
        length = 0;
      }
    }

    ecma_property_t *len_prop_p;
    ecma_property_value_t *len_prop_value_p = ecma_create_named_data_property (object_p,
                                                                               property_name_p,
                                                                               ECMA_PROPERTY_FIXED,
                                                                               &len_prop_p);

    len_prop_value_p->value = ecma_make_integer_value (length);
    return len_prop_p;
  }

  if (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_CALLER)
      || ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_ARGUMENTS))
  {
    ecma_object_t *thrower_p = ecma_builtin_get (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER);

    ecma_property_t *caller_prop_p;
    /* The string_p argument contans the name. */
    ecma_create_named_accessor_property (object_p,
                                         property_name_p,
                                         thrower_p,
                                         thrower_p,
                                         ECMA_PROPERTY_FIXED,
                                         &caller_prop_p);

    ecma_deref_object (thrower_p);
    return caller_prop_p;
  }

  return NULL;
} /* ecma_op_bound_function_try_to_lazy_instantiate_property */

/**
 * List names of a Function object's lazy instantiated properties,
 * adding them to corresponding string collections
 *
 * See also:
 *          ecma_op_function_try_to_lazy_instantiate_property
 */
void
ecma_op_function_list_lazy_property_names (ecma_object_t *object_p, /**< functionobject */
                                           bool separate_enumerable, /**< true - list enumerable properties into
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
  JERRY_UNUSED (main_collection_p);

  ecma_collection_header_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

  /* 'length' property is non-enumerable (ECMA-262 v5, 13.2.5) */
  ecma_string_t *name_p = ecma_new_ecma_length_string ();
  ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (name_p), true);
  ecma_deref_ecma_string (name_p);

  /* 'prototype' property is non-enumerable (ECMA-262 v5, 13.2.18) */
  name_p = ecma_get_magic_string (LIT_MAGIC_STRING_PROTOTYPE);
  ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (name_p), true);
  ecma_deref_ecma_string (name_p);

  ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) object_p;

  const ecma_compiled_code_t *bytecode_data_p;
  bytecode_data_p = ECMA_GET_INTERNAL_VALUE_POINTER (const ecma_compiled_code_t,
                                                     ext_func_p->u.function.bytecode_cp);

  if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE)
  {
    /* 'caller' property is non-enumerable (ECMA-262 v5, 13.2.5) */
    name_p = ecma_get_magic_string (LIT_MAGIC_STRING_CALLER);
    ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (name_p), true);
    ecma_deref_ecma_string (name_p);

    /* 'arguments' property is non-enumerable (ECMA-262 v5, 13.2.5) */
    name_p = ecma_get_magic_string (LIT_MAGIC_STRING_ARGUMENTS);
    ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (name_p), true);
    ecma_deref_ecma_string (name_p);
  }
} /* ecma_op_function_list_lazy_property_names */

/**
 * List names of an External Function object's lazy instantiated properties,
 * adding them to corresponding string collections
 *
 * See also:
 *          ecma_op_external_function_try_to_lazy_instantiate_property
 */
void
ecma_op_external_function_list_lazy_property_names (bool separate_enumerable, /**< true - list enumerable properties
                                                                               *          into main collection and
                                                                               *          non-enumerable to collection
                                                                               *          of 'skipped non-enumerable'
                                                                               *          properties,
                                                                               *   false - list all properties into
                                                                               *           main collection.
                                                                               */
                                                   ecma_collection_header_t *main_collection_p, /**< 'main'
                                                                                                 *    collection */
                                                   ecma_collection_header_t *non_enum_collection_p) /**< skipped
                                                                                                     *   collection */
{
  JERRY_UNUSED (main_collection_p);

  ecma_collection_header_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

  /* 'prototype' property is non-enumerable (ECMA-262 v5, 13.2.18) */
  ecma_string_t *name_p = ecma_get_magic_string (LIT_MAGIC_STRING_PROTOTYPE);
  ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (name_p), true);
  ecma_deref_ecma_string (name_p);
} /* ecma_op_external_function_list_lazy_property_names */

/**
 * List names of a Bound Function object's lazy instantiated properties,
 * adding them to corresponding string collections
 *
 * See also:
 *          ecma_op_bound_function_try_to_lazy_instantiate_property
 */
void
ecma_op_bound_function_list_lazy_property_names (bool separate_enumerable, /**< true - list enumerable properties
                                                                            *          into main collection and
                                                                            *          non-enumerable to collection
                                                                            *          of 'skipped non-enumerable'
                                                                            *          properties,
                                                                            *   false - list all properties into
                                                                            *           main collection.
                                                                            */
                                                 ecma_collection_header_t *main_collection_p, /**< 'main'
                                                                                               *    collection */
                                                 ecma_collection_header_t *non_enum_collection_p) /**< skipped
                                                                                                   *   'non-enumerable'
                                                                                                   *   collection */
{
  JERRY_UNUSED (main_collection_p);

  ecma_collection_header_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

  /* 'length' property is non-enumerable (ECMA-262 v5, 13.2.5) */
  ecma_string_t *name_p = ecma_new_ecma_length_string ();
  ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (name_p), true);
  ecma_deref_ecma_string (name_p);

  /* 'caller' property is non-enumerable (ECMA-262 v5, 13.2.5) */
  name_p = ecma_get_magic_string (LIT_MAGIC_STRING_CALLER);
  ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (name_p), true);
  ecma_deref_ecma_string (name_p);

  /* 'arguments' property is non-enumerable (ECMA-262 v5, 13.2.5) */
  name_p = ecma_get_magic_string (LIT_MAGIC_STRING_ARGUMENTS);
  ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (name_p), true);
  ecma_deref_ecma_string (name_p);
} /* ecma_op_bound_function_list_lazy_property_names */

/**
 * @}
 * @}
 */

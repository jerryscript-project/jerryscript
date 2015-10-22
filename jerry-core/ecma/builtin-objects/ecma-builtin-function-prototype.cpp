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

#include "ecma-alloc.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-function-object.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-function-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID function_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup functionprototype ECMA Function.prototype object built-in
 * @{
 */

/**
 * The Function.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.3.4.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_function_prototype_object_to_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (!ecma_op_is_callable (this_arg))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_string_t *function_to_string_p = ecma_get_magic_string (LIT_MAGIC_STRING__FUNCTION_TO_STRING);
    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (function_to_string_p));
  }
  return ret_value;
} /* ecma_builtin_function_prototype_object_to_string */

/**
 * The Function.prototype object's 'apply' routine
 *
 * See also:
 *          ECMA-262 v5, 15.3.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_function_prototype_object_apply (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t arg1, /**< first argument */
                                              ecma_value_t arg2) /**< second argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  if (!ecma_op_is_callable (this_arg))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_object_t *func_obj_p = ecma_get_object_from_value (this_arg);

    /* 2. */
    if (ecma_is_value_null (arg2) || ecma_is_value_undefined (arg2))
    {
      ret_value = ecma_op_function_call (func_obj_p, arg1, NULL);
    }
    else
    {
      /* 3. */
      if (!ecma_is_value_object (arg2))
      {
        ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
      }
      else
      {
        ecma_object_t *obj_p = ecma_get_object_from_value (arg2);
        ecma_string_t *length_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);

        /* 4. */
        ECMA_TRY_CATCH (length_value,
                        ecma_op_object_get (obj_p, length_magic_string_p),
                        ret_value);

        ECMA_OP_TO_NUMBER_TRY_CATCH (length_number,
                                     length_value,
                                     ret_value);

        /* 5. */
        const uint32_t length = ecma_number_to_uint32 (length_number);

        /* 6. */
        ecma_collection_header_t *arg_collection_p = ecma_new_values_collection (NULL, 0, true);

        /* 7. */
        for (uint32_t index = 0; index < length && ecma_is_completion_value_empty (ret_value); index++)
        {
          ecma_string_t *curr_idx_str_p = ecma_new_ecma_string_from_uint32 (index);

          ECMA_TRY_CATCH (get_value,
                          ecma_op_object_get (obj_p, curr_idx_str_p),
                          ret_value);

          ecma_append_to_values_collection (arg_collection_p, get_value, true);

          ECMA_FINALIZE (get_value);
          ecma_deref_ecma_string (curr_idx_str_p);
        }

        JERRY_ASSERT (arg_collection_p->unit_number == length || !ecma_is_completion_value_empty (ret_value));

        if (ecma_is_completion_value_empty (ret_value))
        {
          ret_value = ecma_op_function_call (func_obj_p,
                                             arg1,
                                             arg_collection_p);
        }

        ecma_free_values_collection (arg_collection_p, true);

        ECMA_OP_TO_NUMBER_FINALIZE (length_number);
        ECMA_FINALIZE (length_value);
        ecma_deref_ecma_string (length_magic_string_p);
      }
    }
  }

  return ret_value;
} /* ecma_builtin_function_prototype_object_apply */

/**
 * The Function.prototype object's 'call' routine
 *
 * See also:
 *          ECMA-262 v5, 15.3.4.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_function_prototype_object_call (ecma_value_t this_arg, /**< this argument */
                                             const ecma_value_t* arguments_list_p, /**< list of arguments */
                                             ecma_length_t arguments_number) /**< number of arguments */
{
  if (!ecma_op_is_callable (this_arg))
  {
    return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_object_t *func_obj_p = ecma_get_object_from_value (this_arg);

    if (arguments_number == 0)
    {
      return ecma_op_function_call (func_obj_p,
                                    ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                                    NULL);
    }
    else
    {
      return ecma_op_function_call_array_args (func_obj_p,
                                               arguments_list_p[0],
                                               (arguments_number == 1u) ? NULL : (arguments_list_p + 1),
                                               (ecma_length_t) (arguments_number - 1u));
    }
  }
} /* ecma_builtin_function_prototype_object_call */

/**
 * The Function.prototype object's 'bind' routine
 *
 * See also:
 *          ECMA-262 v5, 15.3.4.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_function_prototype_object_bind (ecma_value_t this_arg, /**< this argument */
                                             const ecma_value_t* arguments_list_p, /**< list of arguments */
                                             ecma_length_t arguments_number) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 2. */
  if (!ecma_op_is_callable (this_arg))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    /* 4. 11. 18. */
    ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);
    ecma_object_t *function_p = ecma_create_object (prototype_obj_p, true, ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    ecma_deref_object (prototype_obj_p);

    /* 7. */
    ecma_property_t *target_function_prop_p;
    target_function_prop_p = ecma_create_internal_property (function_p,
                                                            ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_TARGET_FUNCTION);

    ecma_object_t *this_arg_obj_p = ecma_get_object_from_value (this_arg);
    ECMA_SET_NON_NULL_POINTER (target_function_prop_p->u.internal_property.value, this_arg_obj_p);

    /* 8. */
    ecma_property_t *bound_this_prop_p;
    bound_this_prop_p = ecma_create_internal_property (function_p, ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_BOUND_THIS);
    const ecma_length_t arg_count = arguments_number;

    if (arg_count > 0)
    {
      bound_this_prop_p->u.internal_property.value = ecma_copy_value (arguments_list_p[0], false);
    }
    else
    {
      bound_this_prop_p->u.internal_property.value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }

    if (arg_count > 1)
    {
      ecma_collection_header_t *bound_args_collection_p;
      bound_args_collection_p = ecma_new_values_collection (&arguments_list_p[1], arg_count - 1, false);

      ecma_property_t *bound_args_prop_p;
      bound_args_prop_p = ecma_create_internal_property (function_p, ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_BOUND_ARGS);
      ECMA_SET_NON_NULL_POINTER (bound_args_prop_p->u.internal_property.value, bound_args_collection_p);
    }

    /*
     * [[Class]] property is not stored explicitly for objects of ECMA_OBJECT_TYPE_FUNCTION type.
     *
     * See also: ecma_object_get_class_name
     */

    ecma_number_t *length_p = ecma_alloc_number ();
    ecma_string_t *magic_string_length_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);

    /* 15. */
    if (ecma_object_get_class_name (this_arg_obj_p) == LIT_MAGIC_STRING_FUNCTION_UL)
    {
      ecma_completion_value_t get_len_completion = ecma_op_object_get (this_arg_obj_p,
                                                                       magic_string_length_p);
      JERRY_ASSERT (ecma_is_completion_value_normal (get_len_completion));

      ecma_value_t len_value = ecma_get_completion_value_value (get_len_completion);
      JERRY_ASSERT (ecma_is_value_number (len_value));

      const ecma_length_t bound_arg_count = arg_count > 1 ? arg_count - 1 : 0;

      /* 15.a */
      *length_p = *ecma_get_number_from_value (len_value) - ecma_uint32_to_number (bound_arg_count);
      ecma_free_completion_value (get_len_completion);

      /* 15.b */
      if (ecma_number_is_negative (*length_p))
      {
        *length_p = ECMA_NUMBER_ZERO;
      }
    }
    else
    {
      /* 16. */
      *length_p = ECMA_NUMBER_ZERO;
    }

    /* 17. */
    ecma_completion_value_t completion = ecma_builtin_helper_def_prop (function_p,
                                                                       magic_string_length_p,
                                                                       ecma_make_number_value (length_p),
                                                                       false, /* Writable */
                                                                       false, /* Enumerable */
                                                                       false, /* Configurable */
                                                                       false); /* Failure handling */

    JERRY_ASSERT (ecma_is_completion_value_normal_true (completion)
                  || ecma_is_completion_value_normal_false (completion));

    ecma_deref_ecma_string (magic_string_length_p);
    ecma_dealloc_number (length_p);

    /* 19-21. */
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
    completion = ecma_op_object_define_own_property (function_p,
                                                     magic_string_caller_p,
                                                     &prop_desc,
                                                     false);

    JERRY_ASSERT (ecma_is_completion_value_normal_true (completion)
                  || ecma_is_completion_value_normal_false (completion));

    ecma_deref_ecma_string (magic_string_caller_p);

    ecma_string_t *magic_string_arguments_p = ecma_get_magic_string (LIT_MAGIC_STRING_ARGUMENTS);
    completion = ecma_op_object_define_own_property (function_p,
                                                     magic_string_arguments_p,
                                                     &prop_desc,
                                                     false);

    JERRY_ASSERT (ecma_is_completion_value_normal_true (completion)
                  || ecma_is_completion_value_normal_false (completion));

    ecma_deref_ecma_string (magic_string_arguments_p);
    ecma_deref_object (thrower_p);

    /* 22. */
    ret_value = ecma_make_normal_completion_value (ecma_make_object_value (function_p));
  }

  return ret_value;
} /* ecma_builtin_function_prototype_object_bind */

/**
 * Handle calling [[Call]] of built-in Function.prototype object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_function_prototype_dispatch_call (const ecma_value_t* arguments_list_p, /**< arguments list */
                                               ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
} /* ecma_builtin_function_prototype_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Function.prototype object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_function_prototype_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                                    ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
} /* ecma_builtin_function_prototype_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

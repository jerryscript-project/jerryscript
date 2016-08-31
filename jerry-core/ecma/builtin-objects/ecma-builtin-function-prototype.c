/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged.
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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_function_prototype_object_to_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (!ecma_op_is_callable (this_arg))
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG (""));
  }
  else
  {
    ecma_string_t *function_to_string_p = ecma_get_magic_string (LIT_MAGIC_STRING__FUNCTION_TO_STRING);
    ret_value = ecma_make_string_value (function_to_string_p);
  }
  return ret_value;
} /* ecma_builtin_function_prototype_object_to_string */

/**
 * The Function.prototype object's 'apply' routine
 *
 * See also:
 *          ECMA-262 v5, 15.3.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_function_prototype_object_apply (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t arg1, /**< first argument */
                                              ecma_value_t arg2) /**< second argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  if (!ecma_op_is_callable (this_arg))
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG (""));
  }
  else
  {
    ecma_object_t *func_obj_p = ecma_get_object_from_value (this_arg);

    /* 2. */
    if (ecma_is_value_null (arg2) || ecma_is_value_undefined (arg2))
    {
      ret_value = ecma_op_function_call (func_obj_p, arg1, NULL, 0);
    }
    else
    {
      /* 3. */
      if (!ecma_is_value_object (arg2))
      {
        ret_value = ecma_raise_type_error (ECMA_ERR_MSG (""));
      }
      else
      {
        ecma_object_t *obj_p = ecma_get_object_from_value (arg2);
        ecma_string_t *length_magic_string_p = ecma_new_ecma_length_string ();

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
        JMEM_DEFINE_LOCAL_ARRAY (arguments_list_p, length, ecma_value_t);
        uint32_t last_index = 0;

        /* 7. */
        for (uint32_t index = 0;
             index < length && ecma_is_value_empty (ret_value);
             index++)
        {
          ecma_string_t *curr_idx_str_p = ecma_new_ecma_string_from_uint32 (index);

          ECMA_TRY_CATCH (get_value,
                          ecma_op_object_get (obj_p, curr_idx_str_p),
                          ret_value);

          arguments_list_p[index] = ecma_copy_value (get_value);
          last_index = index + 1;

          ECMA_FINALIZE (get_value);
          ecma_deref_ecma_string (curr_idx_str_p);
        }

        if (ecma_is_value_empty (ret_value))
        {
          JERRY_ASSERT (last_index == length);
          ret_value = ecma_op_function_call (func_obj_p,
                                             arg1,
                                             arguments_list_p,
                                             length);
        }

        for (uint32_t index = 0; index < last_index; index++)
        {
          ecma_free_value (arguments_list_p[index]);
        }

        JMEM_FINALIZE_LOCAL_ARRAY (arguments_list_p);

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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_function_prototype_object_call (ecma_value_t this_arg, /**< this argument */
                                             const ecma_value_t *arguments_list_p, /**< list of arguments */
                                             ecma_length_t arguments_number) /**< number of arguments */
{
  if (!ecma_op_is_callable (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (""));
  }
  else
  {
    ecma_object_t *func_obj_p = ecma_get_object_from_value (this_arg);

    if (arguments_number == 0)
    {
      /* Even a 'this' argument is missing. */
      return ecma_op_function_call (func_obj_p,
                                    ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                                    NULL,
                                    0);
    }
    else
    {
      return ecma_op_function_call (func_obj_p,
                                    arguments_list_p[0],
                                    arguments_list_p + 1,
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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_function_prototype_object_bind (ecma_value_t this_arg, /**< this argument */
                                             const ecma_value_t *arguments_list_p, /**< list of arguments */
                                             ecma_length_t arguments_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 2. */
  if (!ecma_op_is_callable (this_arg))
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG (""));
  }
  else
  {
    /* 4. 11. 18. */
    ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);
    ecma_object_t *function_p = ecma_create_object (prototype_obj_p,
                                                    false,
                                                    true,
                                                    ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    ecma_deref_object (prototype_obj_p);

    /* 7. */
    ecma_value_t *target_function_prop_p;
    target_function_prop_p = ecma_create_internal_property (function_p,
                                                            ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_TARGET_FUNCTION);

    ecma_object_t *this_arg_obj_p = ecma_get_object_from_value (this_arg);
    ECMA_SET_INTERNAL_VALUE_POINTER (*target_function_prop_p, this_arg_obj_p);

    /* 8. */
    ecma_value_t *bound_this_prop_p;
    bound_this_prop_p = ecma_create_internal_property (function_p, ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_BOUND_THIS);
    const ecma_length_t arg_count = arguments_number;

    if (arg_count > 0)
    {
      *bound_this_prop_p = ecma_copy_value_if_not_object (arguments_list_p[0]);
    }
    else
    {
      *bound_this_prop_p = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }

    if (arg_count > 1)
    {
      ecma_collection_header_t *bound_args_collection_p;
      bound_args_collection_p = ecma_new_values_collection (&arguments_list_p[1], arg_count - 1, false);

      ecma_value_t *bound_args_prop_p;
      bound_args_prop_p = ecma_create_internal_property (function_p, ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_BOUND_ARGS);
      ECMA_SET_INTERNAL_VALUE_POINTER (*bound_args_prop_p, bound_args_collection_p);
    }

    /*
     * [[Class]] property is not stored explicitly for objects of ECMA_OBJECT_TYPE_FUNCTION type.
     *
     * See also: ecma_object_get_class_name
     */

    /* 16. */
    ecma_number_t length = ECMA_NUMBER_ZERO;
    ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

    /* 15. */
    if (ecma_object_get_class_name (this_arg_obj_p) == LIT_MAGIC_STRING_FUNCTION_UL)
    {
      ecma_value_t get_len_value = ecma_op_object_get (this_arg_obj_p, magic_string_length_p);
      JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (get_len_value));
      JERRY_ASSERT (ecma_is_value_number (get_len_value));

      const ecma_length_t bound_arg_count = arg_count > 1 ? arg_count - 1 : 0;

      /* 15.a */
      length = ecma_get_number_from_value (get_len_value) - ((ecma_number_t) bound_arg_count);
      ecma_free_value (get_len_value);

      /* 15.b */
      if (ecma_number_is_negative (length))
      {
        length = ECMA_NUMBER_ZERO;
      }
    }

    /* 17. */
    ecma_value_t completion = ecma_builtin_helper_def_prop (function_p,
                                                            magic_string_length_p,
                                                            ecma_make_number_value (length),
                                                            false, /* Writable */
                                                            false, /* Enumerable */
                                                            false, /* Configurable */
                                                            false); /* Failure handling */

    JERRY_ASSERT (ecma_is_value_boolean (completion));

    ecma_deref_ecma_string (magic_string_length_p);

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

    JERRY_ASSERT (ecma_is_value_boolean (completion));

    ecma_deref_ecma_string (magic_string_caller_p);

    ecma_string_t *magic_string_arguments_p = ecma_get_magic_string (LIT_MAGIC_STRING_ARGUMENTS);
    completion = ecma_op_object_define_own_property (function_p,
                                                     magic_string_arguments_p,
                                                     &prop_desc,
                                                     false);

    JERRY_ASSERT (ecma_is_value_boolean (completion));

    ecma_deref_ecma_string (magic_string_arguments_p);
    ecma_deref_object (thrower_p);

    /* 22. */
    ret_value = ecma_make_object_value (function_p);
  }

  return ret_value;
} /* ecma_builtin_function_prototype_object_bind */

/**
 * Handle calling [[Call]] of built-in Function.prototype object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_function_prototype_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                               ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
} /* ecma_builtin_function_prototype_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Function.prototype object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_function_prototype_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                                    ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_raise_type_error (ECMA_ERR_MSG (""));
} /* ecma_builtin_function_prototype_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

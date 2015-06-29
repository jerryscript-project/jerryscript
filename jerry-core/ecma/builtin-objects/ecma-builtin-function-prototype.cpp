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
      ret_value = ecma_op_function_call (func_obj_p,
                                         arg1,
                                         NULL,
                                         0);
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
        MEM_DEFINE_LOCAL_ARRAY (arg_list, length, ecma_value_t);

        /* 7. */
        uint32_t appended_num = 0;
        for (uint32_t index = 0; index < length && ecma_is_completion_value_empty (ret_value); index++)
        {
          ecma_string_t *curr_idx_str_p = ecma_new_ecma_string_from_uint32 (index);

          ECMA_TRY_CATCH (get_value,
                          ecma_op_object_get (obj_p, curr_idx_str_p),
                          ret_value);

          arg_list[index] = ecma_copy_value (get_value, true);
          appended_num++;

          ECMA_FINALIZE (get_value);
          ecma_deref_ecma_string (curr_idx_str_p);
        }

        JERRY_ASSERT (appended_num == length || !ecma_is_completion_value_empty (ret_value));

        if (ecma_is_completion_value_empty (ret_value))
        {
          ret_value = ecma_op_function_call (func_obj_p,
                                             arg1,
                                             arg_list,
                                             (ecma_length_t) appended_num);
        }

        for (uint32_t index = 0; index < appended_num; index++)
        {
          ecma_free_value (arg_list[index], true);
        }

        MEM_FINALIZE_LOCAL_ARRAY (arg_list);

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
                                    NULL,
                                    0);
    }
    else
    {
      return ecma_op_function_call (func_obj_p,
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
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arguments_list_p, arguments_number);
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

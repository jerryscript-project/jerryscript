/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
#include "ecma-array-object.h"
#include "ecma-array-prototype.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-comparison.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-string-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-array-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID array_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup arrayprototype ECMA Array.prototype object built-in
 * @{
 */

/**
 * Helper function to set an object's length property
 *
 * @return completion value (return value of the [[Put]] method)
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_helper_set_length (ecma_object_t *object, /**< object*/
                                                uint32_t length) /**< new length */
{
  ecma_completion_value_t ret_value;
  ecma_string_t *magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  ecma_number_t *len_p = ecma_alloc_number ();
  *len_p = ecma_uint32_to_number (length);

  ret_value = ecma_op_object_put (object,
                                  magic_string_length_p,
                                  ecma_make_number_value (len_p),
                                  true),

  ecma_dealloc_number (len_p);
  ecma_deref_ecma_string (magic_string_length_p);

  return ret_value;
} /* ecma_builtin_array_prototype_helper_set_length */

/**
 * The Array.prototype object's 'concat' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_concat (ecma_value_t this_arg, /**< this argument */
                                            const ecma_value_t args[], /**< arguments list */
                                            ecma_length_t args_number) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  uint32_t len = ecma_number_to_uint32 (len_number);

  uint32_t new_array_index = 0;

  /* 2. */
  ecma_completion_value_t new_array = ecma_op_create_array_object (0, 0, false);
  ecma_object_t *new_array_p = ecma_get_object_from_completion_value (new_array);


  for (uint32_t index = 0;
       index < len && ecma_is_completion_value_empty (ret_value);
       index++, new_array_index++)
  {
    ecma_string_t *index_string_p = ecma_new_ecma_string_from_uint32 (index);
    ECMA_TRY_CATCH (get_value, ecma_op_object_get (obj_p, index_string_p), ret_value);

    /* Using [[Put]] is equvalent to [[DefineOwnProperty]] in this case, so we use it for simplicity. */
    ecma_completion_value_t put_comp = ecma_op_object_put (new_array_p,
                                                           index_string_p,
                                                           get_value,
                                                           false);
    JERRY_ASSERT (ecma_is_completion_value_normal (put_comp));
    ecma_free_completion_value (put_comp);

    ECMA_FINALIZE (get_value);
    ecma_deref_ecma_string (index_string_p);
  }

  for (uint32_t arg_index = 0;
       arg_index < args_number && ecma_is_completion_value_empty (ret_value);
       arg_index++)
  {
    /* 5.b */
    if (ecma_is_value_object (args[arg_index]) &&
        (ecma_get_object_type (ecma_get_object_from_value (args[arg_index])) == ECMA_OBJECT_TYPE_ARRAY))
    {
      /* 5.b.ii */
      ECMA_TRY_CATCH (arg_len_value,
                      ecma_op_object_get (ecma_get_object_from_value (args[arg_index]),
                                          magic_string_length_p),
                      ret_value);
      ECMA_OP_TO_NUMBER_TRY_CATCH (arg_len_number, len_value, ret_value);

      uint32_t arg_len = ecma_number_to_uint32 (arg_len_number);

      for (uint32_t array_index = 0;
           array_index < arg_len && ecma_is_completion_value_empty (ret_value);
           array_index++, new_array_index++)
      {
        ecma_string_t *array_index_string_p = ecma_new_ecma_string_from_uint32 (array_index);

        /* 5.b.iii.2 */
        if (ecma_op_object_get_property (ecma_get_object_from_value (args[arg_index]),
                                         array_index_string_p) != NULL)
        {
          ecma_string_t *new_array_index_string_p = ecma_new_ecma_string_from_uint32 (new_array_index);

          ECMA_TRY_CATCH (get_value,
                          ecma_op_object_get (ecma_get_object_from_value (args[arg_index]),
                                              array_index_string_p),
                          ret_value);

          /* Using [[Put]] is equvalent to [[DefineOwnProperty]] in this case, so we use it for simplicity. */
          ecma_completion_value_t put_comp = ecma_op_object_put (new_array_p,
                                                                 new_array_index_string_p,
                                                                 get_value,
                                                                 false);
          JERRY_ASSERT (ecma_is_completion_value_normal (put_comp));
          ecma_free_completion_value (put_comp);

          ECMA_FINALIZE (get_value);
          ecma_deref_ecma_string (new_array_index_string_p);
        }

        ecma_deref_ecma_string (array_index_string_p);
      }

      ECMA_OP_TO_NUMBER_FINALIZE (len_number);
      ECMA_FINALIZE (arg_len_value);
    }
    else
    {
      ecma_string_t *new_array_index_string_p = ecma_new_ecma_string_from_uint32 (new_array_index);

      /* Using [[Put]] is equvalent to [[DefineOwnProperty]] in this case, so we use it for simplicity. */
      ecma_completion_value_t put_comp = ecma_op_object_put (new_array_p,
                                                             new_array_index_string_p,
                                                             args[arg_index],
                                                             false);
      JERRY_ASSERT (ecma_is_completion_value_normal (put_comp));
      ecma_free_completion_value (put_comp);

      ecma_deref_ecma_string (new_array_index_string_p);
      new_array_index++;
    }
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    ret_value = new_array;
  }
  else
  {
    ecma_free_completion_value (new_array);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
}

/**
 * The Array.prototype object's 'forEach' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.18
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_for_each (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t arg1, /**< callbackfn */
                                              ecma_value_t arg2) /**< thisArg */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  /* 3. */
  uint32_t len = ecma_number_to_uint32 (len_number);

  /* 4. */
  if (!ecma_op_is_callable (arg1))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_value_t current_index;
    ecma_number_t *num_p = ecma_alloc_number ();
    ecma_object_t *func_object_p;

    /* We already checked that arg1 is callable, so it will always coerce to an object. */
    ecma_completion_value_t to_object_comp = ecma_op_to_object (arg1);
    JERRY_ASSERT (ecma_is_completion_value_normal (to_object_comp));

    func_object_p = ecma_get_object_from_completion_value (to_object_comp);

    /* Iterate over array and call callbackfn on every element */
    for (uint32_t index = 0; index < len && ecma_is_completion_value_empty (ret_value); index++)
    {
      /* 7.a */
      ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

      /* 7.b */
      if (ecma_op_object_get_property (obj_p, index_str_p) != NULL)
      {
        /* 7.c.i */
        ECMA_TRY_CATCH (current_value, ecma_op_object_get (obj_p, index_str_p), ret_value);

        *num_p = ecma_uint32_to_number (index);
        current_index = ecma_make_number_value (num_p);

        /* 7.c.ii */
        ecma_value_t call_args[] = {current_value, current_index, obj_this};
        ECMA_TRY_CATCH (call_value, ecma_op_function_call (func_object_p, arg2, call_args, 3), ret_value);

        ECMA_FINALIZE (call_value);
        ECMA_FINALIZE (current_value);
      }

      ecma_deref_ecma_string (index_str_p);
    }

    if (ecma_is_completion_value_empty (ret_value))
    {
      /* 8. */
      ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }

    ecma_free_completion_value (to_object_comp);
    ecma_dealloc_number (num_p);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_for_each */

/**
 * The Array.prototype object's 'join' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_join (const ecma_value_t this_arg, /**< this argument */
                                   const ecma_value_t separator_arg) /** < separator argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (obj_value,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);

  ecma_string_t *length_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  /* 2. */
  ECMA_TRY_CATCH (length_value,
                  ecma_op_object_get (obj_p, length_magic_string_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (length_number,
                               length_value,
                               ret_value);

  /* 3. */
  uint32_t length = ecma_number_to_uint32 (length_number);

  /* 4-5. */
  ECMA_TRY_CATCH (separator_value,
                  ecma_op_array_get_separator_string (separator_arg),
                  ret_value);

  if (length == 0)
  {
    /* 6. */
    ecma_string_t *empty_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING__EMPTY);
    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (empty_string_p));
  }
  else
  {
    ecma_string_t *separator_string_p = ecma_get_string_from_value (separator_value);

    /* 7-8. */
    ECMA_TRY_CATCH (first_value,
                    ecma_op_array_get_to_string_at_index (obj_p, 0),
                    ret_value);

    ecma_string_t *return_string_p = ecma_copy_or_ref_ecma_string (ecma_get_string_from_value (first_value));

    /* 9-10. */
    for (uint32_t k = 1; ecma_is_completion_value_empty (ret_value) && (k < length); ++k)
    {
      /* 10.a */
      ecma_string_t *part_string_p = ecma_concat_ecma_strings (return_string_p, separator_string_p);

      /* 10.b, 10.c */
      ECMA_TRY_CATCH (next_string_value,
                      ecma_op_array_get_to_string_at_index (obj_p, k),
                      ret_value);

      ecma_string_t *next_string_p = ecma_get_string_from_value (next_string_value);

      ecma_deref_ecma_string (return_string_p);

      /* 10.d */
      return_string_p = ecma_concat_ecma_strings (part_string_p, next_string_p);

      ECMA_FINALIZE (next_string_value);

      ecma_deref_ecma_string (part_string_p);
    }

    if (ecma_is_completion_value_empty (ret_value))
    {
      ret_value = ecma_make_normal_completion_value (ecma_make_string_value (return_string_p));
    }
    else
    {
      ecma_deref_ecma_string (return_string_p);
    }

    ECMA_FINALIZE (first_value);
  }

  ECMA_FINALIZE (separator_value);

  ECMA_OP_TO_NUMBER_FINALIZE (length_number);

  ECMA_FINALIZE (length_value);

  ecma_deref_ecma_string (length_magic_string_p);

  ECMA_FINALIZE (obj_value);

  return ret_value;
} /* ecma_builtin_array_prototype_join */

/**
 * The Array.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_to_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_completion_value_t return_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (obj_this_value,
                  ecma_op_to_object (this_arg),
                  return_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this_value);

  ecma_string_t *join_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_JOIN);

  /* 2. */
  ECMA_TRY_CATCH (join_value,
                  ecma_op_object_get (obj_p, join_magic_string_p),
                  return_value);

  if (!ecma_op_is_callable (join_value))
  {
    /* 3. */
    return_value = ecma_builtin_helper_object_to_string (this_arg);
  }
  else
  {
    /* 4. */
    ecma_object_t *join_func_obj_p = ecma_get_object_from_value (join_value);

    return_value = ecma_op_function_call (join_func_obj_p, this_arg, NULL, 0);
  }

  ECMA_FINALIZE (join_value);

  ecma_deref_ecma_string (join_magic_string_p);

  ECMA_FINALIZE (obj_this_value);

  return return_value;
} /* ecma_builtin_array_prototype_object_to_string */


/**
 * The Array.prototype object's 'toLocaleString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_to_locale_string (const ecma_value_t this_arg) /**< this argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (obj_value,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_completion_value (obj_value);

  ecma_string_t *length_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  /* 2. */
  ECMA_TRY_CATCH (length_value,
                  ecma_op_object_get (obj_p, length_magic_string_p),
                  ret_value);

  /* 3. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (length_number,
                               length_value,
                               ret_value);

  uint32_t length = ecma_number_to_uint32 (length_number);

  /* 4. Implementation-defined: set the separator to a single comma character */
  ecma_string_t *separator_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_COMMA_CHAR);

  /* 5. */
  if (length == 0)
  {
    ecma_string_t *empty_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING__EMPTY);
    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (empty_string_p));
  }
  else
  {
    /* 7-8. */
    ECMA_TRY_CATCH (first_value,
                    ecma_builtin_helper_get_to_locale_string_at_index (obj_p, 0),
                    ret_value);

    ecma_string_t *return_string_p = ecma_copy_or_ref_ecma_string (ecma_get_string_from_value (first_value));

    /* 9-10. */
    for (uint32_t k = 1; ecma_is_completion_value_empty (ret_value) && (k < length); ++k)
    {
      ecma_string_t *part_string_p = ecma_concat_ecma_strings (return_string_p, separator_string_p);

      ECMA_TRY_CATCH (next_string_value,
                      ecma_builtin_helper_get_to_locale_string_at_index (obj_p, k),
                      ret_value);

      ecma_string_t *next_string_p = ecma_get_string_from_completion_value (next_string_value);

      ecma_deref_ecma_string (return_string_p);

      return_string_p = ecma_concat_ecma_strings (part_string_p, next_string_p);

      ECMA_FINALIZE (next_string_value);

      ecma_deref_ecma_string (part_string_p);
    }

    if (ecma_is_completion_value_empty (ret_value))
    {
      ret_value = ecma_make_normal_completion_value (ecma_make_string_value (return_string_p));
    }
    else
    {
      ecma_deref_ecma_string (return_string_p);
    }

    ECMA_FINALIZE (first_value);
  }

  ecma_deref_ecma_string (separator_string_p);

  ECMA_OP_TO_NUMBER_FINALIZE (length_number);

  ECMA_FINALIZE (length_value);

  ecma_deref_ecma_string (length_magic_string_p);

  ECMA_FINALIZE (obj_value);

  return ret_value;
} /* ecma_builtin_array_prototype_object_to_locale_string */

/**
 * The Array.prototype object's 'pop' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_pop (ecma_value_t this_arg) /**< this argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  /* 3. */
  uint32_t len = ecma_number_to_uint32 (len_number);

  /* 4. */
  if (len == 0)
  {
    /* 4.a */
    ECMA_TRY_CATCH (set_length_value,
                    ecma_builtin_array_prototype_helper_set_length (obj_p, 0),
                    ret_value);

    /* 4.b */
    ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);

    ECMA_FINALIZE (set_length_value)
  }
  else
  {
    len--;
    /* 5.a */
    ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (len);

    /* 5.b */
    ECMA_TRY_CATCH (get_value, ecma_op_object_get (obj_p, index_str_p), ret_value);

    /* 5.c */
    ECMA_TRY_CATCH (del_value, ecma_op_object_delete (obj_p, index_str_p, true), ret_value);

    /* 5.d */
    ECMA_TRY_CATCH (set_length_value,
                    ecma_builtin_array_prototype_helper_set_length (obj_p, len),
                    ret_value);

    ret_value = ecma_make_normal_completion_value (ecma_copy_value (get_value, true));

    ECMA_FINALIZE (set_length_value);
    ECMA_FINALIZE (del_value);
    ECMA_FINALIZE (get_value);

    ecma_deref_ecma_string (index_str_p);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_pop */

/**
 * The Array.prototype object's 'push' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.7
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_push (ecma_value_t this_arg, /**< this argument */
                                          const ecma_value_t *argument_list_p, /**< arguments list */
                                          ecma_length_t arguments_number) /**< number of arguments */
{
  JERRY_ASSERT (argument_list_p == NULL || arguments_number > 0);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  // 1.
  ECMA_TRY_CATCH (obj_this_value, ecma_op_to_object (this_arg), ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this_value);

  // 2.
  ecma_string_t *length_str_p = ecma_new_ecma_string_from_magic_string_id (ECMA_MAGIC_STRING_LENGTH);

  ECMA_TRY_CATCH (length_value, ecma_op_object_get (obj_p, length_str_p), ret_value);

  // 3.
  ECMA_OP_TO_NUMBER_TRY_CATCH (length_var, length_value, ret_value);

  uint32_t n = ecma_number_to_uint32 (length_var);

  // 5.
  for (uint32_t index = 0;
       index < arguments_number;
       ++index, ++n)
  {
    // a.
    ecma_value_t e_value = argument_list_p[index];

    // b.
    ecma_string_t *n_str_p = ecma_new_ecma_string_from_uint32 (n);

    ecma_completion_value_t completion = ecma_op_object_put (obj_p, n_str_p, e_value, true);

    ecma_deref_ecma_string (n_str_p);

    if (unlikely (ecma_is_completion_value_throw (completion)
                  || ecma_is_completion_value_exit (completion)))
    {
      ret_value = completion;
      break;
    }
    else
    {
      JERRY_ASSERT (ecma_is_completion_value_normal (completion));
      ecma_free_completion_value (completion);
    }
  }

  // 6.
  if (ecma_is_completion_value_empty (ret_value))
  {
    ecma_number_t *num_length_p = ecma_alloc_number ();
    *num_length_p = ecma_uint32_to_number (n);

    ecma_value_t num_length_value = ecma_make_number_value (num_length_p);

    ecma_completion_value_t completion = ecma_op_object_put (obj_p,
                                                             length_str_p,
                                                             num_length_value,
                                                             true);

    if (unlikely (ecma_is_completion_value_throw (completion)
                  || ecma_is_completion_value_exit (completion)))
    {
      ret_value = completion;

      ecma_dealloc_number (num_length_p);
    }
    else
    {
      JERRY_ASSERT (ecma_is_completion_value_normal (completion));
      ecma_free_completion_value (completion);

      ret_value = ecma_make_normal_completion_value (num_length_value);
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (length_var);

  ECMA_FINALIZE (length_value);

  ecma_deref_ecma_string (length_str_p);

  ECMA_FINALIZE (obj_this_value);

  return ret_value;
} /* ecma_builtin_array_prototype_object_push */

/**
 * The Array.prototype object's 'reverse' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.8
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_reverse (ecma_value_t this_arg) /**< this argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  /* 3. */
  uint32_t len = ecma_number_to_uint32 (len_number);

  /* 4. */
  uint32_t middle = len / 2;

  /* 5. and 6. */
  for (uint32_t lower = 0; lower < middle && ecma_is_completion_value_empty (ret_value); lower++)
  {
    /* 6.a */
    uint32_t upper = len - lower - 1;
    /* 6.b and 6.c */
    ecma_string_t *upper_str_p = ecma_new_ecma_string_from_uint32 (upper);
    ecma_string_t *lower_str_p = ecma_new_ecma_string_from_uint32 (lower);

    /* 6.d and 6.e */
    ECMA_TRY_CATCH (lower_value, ecma_op_object_get (obj_p, lower_str_p), ret_value);
    ECMA_TRY_CATCH (upper_value, ecma_op_object_get (obj_p, upper_str_p), ret_value);

    /* 6.f and 6.g */
    bool lower_exist = (ecma_op_object_get_property (obj_p, lower_str_p) != NULL);
    bool upper_exist = (ecma_op_object_get_property (obj_p, upper_str_p) != NULL);

    /* 6.h */
    if (lower_exist && upper_exist)
    {
      ECMA_TRY_CATCH (outer_put_value, ecma_op_object_put (obj_p, lower_str_p, upper_value, true), ret_value);
      ECMA_TRY_CATCH (inner_put_value, ecma_op_object_put (obj_p, upper_str_p, lower_value, true), ret_value);
      ECMA_FINALIZE (inner_put_value);
      ECMA_FINALIZE (outer_put_value);
    }
    /* 6.i */
    else if (!lower_exist && upper_exist)
    {
      ECMA_TRY_CATCH (put_value, ecma_op_object_put (obj_p, lower_str_p, upper_value, true), ret_value);
      ECMA_TRY_CATCH (del_value, ecma_op_object_delete (obj_p, upper_str_p, true), ret_value);
      ECMA_FINALIZE (del_value);
      ECMA_FINALIZE (put_value);
    }
    /* 6.j */
    else if (lower_exist && !upper_exist)
    {
      ECMA_TRY_CATCH (del_value, ecma_op_object_delete (obj_p, lower_str_p, true), ret_value);
      ECMA_TRY_CATCH (put_value, ecma_op_object_put (obj_p, upper_str_p, lower_value, true), ret_value);
      ECMA_FINALIZE (put_value);
      ECMA_FINALIZE (del_value);
    }

    ECMA_FINALIZE (upper_value);
    ECMA_FINALIZE (lower_value);
    ecma_deref_ecma_string (lower_str_p);
    ecma_deref_ecma_string (upper_str_p);
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    /* 7. */
    ret_value = ecma_make_normal_completion_value (ecma_copy_value (obj_this, true));
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_reverse */

/**
 * The Array.prototype object's 'indexOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.14
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_index_of (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t arg1, /**< searchElement */
                                              ecma_value_t arg2) /**< fromIndex */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  /* 3. */
  uint32_t len = ecma_number_to_uint32 (len_number);

  ecma_number_t* num_p = ecma_alloc_number ();
  *num_p = ecma_int32_to_number (-1);

  /* 4. */
  if (len == 0)
  {
    ret_value = ecma_make_normal_completion_value (ecma_make_number_value (num_p));
  }
  else
  {
    /* 5. */
    ECMA_OP_TO_NUMBER_TRY_CATCH (arg_from_idx, arg2, ret_value);

    int32_t from_idx_int = ecma_number_to_int32 (arg_from_idx);

    /* 6. */
    if (from_idx_int > 0 && (uint32_t) from_idx_int >= len)
    {
      ret_value = ecma_make_normal_completion_value (ecma_make_number_value (num_p));
    }
    else
    {
      uint32_t k;

      /* 7 */
      if (from_idx_int >= 0)
      {
        k = (uint32_t) from_idx_int;
      }
      /* 8. */
      else
      {
        from_idx_int = -from_idx_int;

        /* As opposed to the standard, we prevent k from being negative, so that we can use an uint32 */
        if ((uint32_t) from_idx_int < len)
        {
          /* 8.a */
          k = len - (uint32_t) from_idx_int;
        }
        /* If k would've been negative */
        else
        {
          /* 8.b */
          k = 0;
        }

      }
      JERRY_ASSERT (k < len);

      for (; k < len && *num_p < 0 && ecma_is_completion_value_empty (ret_value); k++)
      {
        ecma_string_t *idx_str_p = ecma_new_ecma_string_from_uint32 (k);

        /* 9.a */
        if (ecma_op_object_get_property (obj_p, idx_str_p) != NULL)
        {
          /* 9.b.i */
          ECMA_TRY_CATCH (get_value, ecma_op_object_get (obj_p, idx_str_p), ret_value);

          /* 9.b.ii */
          if (ecma_op_strict_equality_compare (arg1, get_value))
          {
            *num_p = ecma_uint32_to_number (k);
          }

          ECMA_FINALIZE (get_value);
        }

        ecma_deref_ecma_string (idx_str_p);
      }

      if (ecma_is_completion_value_empty (ret_value))
      {
        ret_value = ecma_make_normal_completion_value (ecma_make_number_value (num_p));
      }
      else
      {
        ecma_dealloc_number (num_p);
      }
    }

    ECMA_OP_TO_NUMBER_FINALIZE (arg_from_idx);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);

  ECMA_FINALIZE (len_value);

  ecma_deref_ecma_string (magic_string_length_p);

  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_index_of */

/**
 * The Array.prototype object's 'lastIndexOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.15
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_last_index_of (ecma_value_t this_arg, /**< this argument */
                                                   ecma_value_t arg1, /**< searchElement */
                                                   ecma_value_t arg2) /**< fromIndex */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  /* 3. */
  uint32_t len = ecma_number_to_uint32 (len_number);

  ecma_number_t* num_p = ecma_alloc_number ();
  *num_p = ecma_int32_to_number (-1);

  /* 4. */
  if (len == 0)
  {
    ret_value = ecma_make_normal_completion_value (ecma_make_number_value (num_p));
  }
  else
  {
    uint32_t k = len - 1;

    /* 5. */
    if (!ecma_is_value_undefined (arg2))
    {
      ECMA_OP_TO_NUMBER_TRY_CATCH (arg_from_idx, arg2, ret_value);
      int32_t n = ecma_number_to_int32 (arg_from_idx);

      /* 6. */
      if (n >= 0)
      {
        /* min(n, len - 1)*/
        if ((uint32_t) n > len - 1)
        {
          k = len - 1;
        }
        else
        {
          k = (uint32_t) n;
        }
      }
      /* 7. */
      else
      {
        n = -n;

        /* We prevent k from being negative, so that we can use an uint32 */
        if ((uint32_t) n <= len)
        {
          k = len - (uint32_t) n;
        }
        else
        {
          /*
           * If k would be negative, we set it to UINT_MAX. See reasoning for this in the comment
           * at the for loop below.
           */
          k = (uint32_t) -1;
        }
      }

      ECMA_OP_TO_NUMBER_FINALIZE (arg_from_idx);
    }

    /* 8.
     * We should break from the loop when k < 0. We can still use an uint32_t for k, and check
     * for an underflow instead. This is safe, because k will always start in [0, len - 1],
     * and len is in [0, UINT_MAX], so k >= len means we've had an underflow, and should stop.
     */
    for (;k < len && *num_p < 0 && ecma_is_completion_value_empty (ret_value); k--)
    {
      /* 8.a */
      ecma_string_t *idx_str_p = ecma_new_ecma_string_from_uint32 (k);

      /* 8.a */
      if (ecma_op_object_get_property (obj_p, idx_str_p) != NULL)
      {
        /* 8.b.i */
        ECMA_TRY_CATCH (get_value, ecma_op_object_get (obj_p, idx_str_p), ret_value);

        /* 8.b.ii */
        if (ecma_op_strict_equality_compare (arg1, get_value))
        {
          *num_p = ecma_uint32_to_number (k);
        }

        ECMA_FINALIZE (get_value);
      }

      ecma_deref_ecma_string (idx_str_p);
    }

    if (ecma_is_completion_value_empty (ret_value))
    {
      ret_value = ecma_make_normal_completion_value (ecma_make_number_value (num_p));
    }
    else
    {
      ecma_dealloc_number (num_p);
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_last_index_of */

/**
 * The Array.prototype object's 'shift' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.9
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_shift (ecma_value_t this_arg) /**< this argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  /* 3. */
  uint32_t len = ecma_number_to_uint32 (len_number);

  /* 4. */
  if (len == 0)
  {
    ECMA_TRY_CATCH (set_length_value,
                    ecma_builtin_array_prototype_helper_set_length (obj_p, 0),
                    ret_value);

    ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);

    ECMA_FINALIZE (set_length_value);
  }
  else
  {
    ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (0);

    /* 5. */
    ECMA_TRY_CATCH (first_value, ecma_op_object_get (obj_p, index_str_p), ret_value);

    /* 6. and 7. */
    for (uint32_t k = 1; k < len && ecma_is_completion_value_empty (ret_value); k++)
    {
      /* 7.a */
      ecma_string_t *from_str_p = ecma_new_ecma_string_from_uint32 (k);
      /* 7.b */
      ecma_string_t *to_str_p = ecma_new_ecma_string_from_uint32 (k - 1);

      /* 7.c */
      if (ecma_op_object_get_property (obj_p, from_str_p) != NULL)
      {
        /* 7.d.i */
        ECMA_TRY_CATCH (curr_value, ecma_op_object_get (obj_p, from_str_p), ret_value);

        /* 7.d.ii*/
        ECMA_TRY_CATCH (put_value, ecma_op_object_put (obj_p, to_str_p, curr_value, true), ret_value);

        ECMA_FINALIZE (put_value);
        ECMA_FINALIZE (curr_value);
      }
      else
      {
        /* 7.e.i */
        ECMA_TRY_CATCH (del_value, ecma_op_object_delete (obj_p, to_str_p, true), ret_value);
        ECMA_FINALIZE (del_value);
      }

      ecma_deref_ecma_string (to_str_p);
      ecma_deref_ecma_string (from_str_p);
    }

    if (ecma_is_completion_value_empty (ret_value))
    {
      len--;
      ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (len);

      /* 8. */
      ECMA_TRY_CATCH (del_value, ecma_op_object_delete (obj_p, index_str_p, true), ret_value);

      /* 9. */
      ECMA_TRY_CATCH (set_length_value,
                      ecma_builtin_array_prototype_helper_set_length (obj_p, len),
                      ret_value);
      /* 10. */
      ret_value = ecma_make_normal_completion_value (ecma_copy_value (first_value, true));

      ECMA_FINALIZE (set_length_value);
      ECMA_FINALIZE (del_value);
      ecma_deref_ecma_string (index_str_p);
    }

    ECMA_FINALIZE (first_value);
    ecma_deref_ecma_string (index_str_p);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_shift */

/**
 * The Array.prototype object's 'unshift' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.13
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_unshift (ecma_value_t this_arg, /**< this argument */
                                             const ecma_value_t args[], /**< arguments list */
                                             ecma_length_t args_number) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  /* 3. */
  uint32_t len = ecma_number_to_uint32 (len_number);

  /* 5. and 6. */
  for (uint32_t k = len; k > 0 && ecma_is_completion_value_empty (ret_value); k--)
  {
    /* 6.a */
    ecma_string_t *from_str_p = ecma_new_ecma_string_from_uint32 (k - 1);
    /* 6.b */
    ecma_string_t *to_str_p = ecma_new_ecma_string_from_uint32 (k + args_number - 1);

    /* 6.c */
    if (ecma_op_object_get_property (obj_p, from_str_p) != NULL)
    {
      /* 6.d.i */
      ECMA_TRY_CATCH (get_value, ecma_op_object_get (obj_p, from_str_p), ret_value);
      /* 6.d.ii */
      ECMA_TRY_CATCH (put_value, ecma_op_object_put (obj_p, to_str_p, get_value, true), ret_value);

      ECMA_FINALIZE (put_value);
      ECMA_FINALIZE (get_value);
    }
    else
    {
      /* 6.e.i */
      ECMA_TRY_CATCH (del_value, ecma_op_object_delete (obj_p, to_str_p, true), ret_value);
      ECMA_FINALIZE (del_value);
    }

    ecma_deref_ecma_string (to_str_p);
    ecma_deref_ecma_string (from_str_p);
  }

  for (uint32_t arg_index = 0;
       arg_index < args_number && ecma_is_completion_value_empty (ret_value);
       arg_index++)
  {
    ecma_string_t *to_str_p = ecma_new_ecma_string_from_uint32 (arg_index);
    /* 9.b */
    ECMA_TRY_CATCH (put_value, ecma_op_object_put (obj_p, to_str_p, args[arg_index], true), ret_value);
    ECMA_FINALIZE (put_value);
    ecma_deref_ecma_string (to_str_p);
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    /* 10. */
    ECMA_TRY_CATCH (set_length_value,
                    ecma_builtin_array_prototype_helper_set_length (obj_p, len + args_number),
                    ret_value);

    ecma_number_t *num_p = ecma_alloc_number ();
    *num_p = ecma_uint32_to_number (len + args_number);
    ret_value = ecma_make_normal_completion_value (ecma_make_number_value (num_p));

    ECMA_FINALIZE (set_length_value);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_unshift */

/**
 * The Array.prototype object's 'every' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.16
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_every (ecma_value_t this_arg, /**< this argument */
                                           ecma_value_t arg1, /**< callbackfn */
                                           ecma_value_t arg2) /**< thisArg */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  /* 3. */
  uint32_t len = ecma_number_to_uint32 (len_number);

  /* 4. */
  if (!ecma_op_is_callable (arg1))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_value_t current_index;
    ecma_number_t *num_p = ecma_alloc_number ();
    ecma_object_t *func_object_p;

    /* We already checked that arg1 is callable, so it will always coerce to an object. */
    ecma_completion_value_t to_object_comp = ecma_op_to_object (arg1);
    JERRY_ASSERT (ecma_is_completion_value_normal (to_object_comp));

    func_object_p = ecma_get_object_from_completion_value (to_object_comp);

    /* 7. */
    for (uint32_t index = 0; index < len && ecma_is_completion_value_empty (ret_value); index++)
    {
      /* 7.a */
      ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

      /* 7.c */
      if (ecma_op_object_get_property (obj_p, index_str_p) != NULL)
      {
        /* 7.c.i */
        ECMA_TRY_CATCH (get_value, ecma_op_object_get (obj_p, index_str_p), ret_value);

        *num_p = ecma_uint32_to_number (index);
        current_index = ecma_make_number_value (num_p);

        ecma_value_t call_args[] = { get_value, current_index, obj_this };
        /* 7.c.ii */
        ECMA_TRY_CATCH (call_value, ecma_op_function_call (func_object_p, arg2, call_args, 3), ret_value);

        /* 7.c.iii, ecma_op_to_boolean always returns a simple value, so no need to free. */
        if (!ecma_is_value_true (ecma_op_to_boolean (call_value)))
        {
          ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
        }

        ECMA_FINALIZE (call_value);
        ECMA_FINALIZE (get_value);
      }

      ecma_deref_ecma_string (index_str_p);
    }

    ecma_free_completion_value (to_object_comp);
    ecma_dealloc_number (num_p);

    if (ecma_is_completion_value_empty (ret_value))
    {
      /* 8. */
      ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_every */

/**
 * The Array.prototype object's 'some' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.17
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_some (ecma_value_t this_arg, /**< this argument */
                                          ecma_value_t arg1, /**< callbackfn */
                                          ecma_value_t arg2) /**< thisArg */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  /* 3. */
  uint32_t len = ecma_number_to_uint32 (len_number);

  /* 4. */
  if (!ecma_op_is_callable (arg1))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_value_t current_index;
    ecma_number_t *num_p = ecma_alloc_number ();
    ecma_object_t *func_object_p;

    /* We already checked that arg1 is callable, so it will always coerce to an object. */
    ecma_completion_value_t to_object_comp = ecma_op_to_object (arg1);
    JERRY_ASSERT (ecma_is_completion_value_normal (to_object_comp));

    func_object_p = ecma_get_object_from_completion_value (to_object_comp);

    /* 7. */
    for (uint32_t index = 0; index < len && ecma_is_completion_value_empty (ret_value); index++)
    {
      /* 7.a */
      ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

      /* 7.c */
      if (ecma_op_object_get_property (obj_p, index_str_p) != NULL)
      {
        /* 7.c.i */
        ECMA_TRY_CATCH (get_value, ecma_op_object_get (obj_p, index_str_p), ret_value);

        *num_p = ecma_uint32_to_number (index);
        current_index = ecma_make_number_value (num_p);

        ecma_value_t call_args[] = { get_value, current_index, obj_this };
        /* 7.c.ii */
        ECMA_TRY_CATCH (call_value, ecma_op_function_call (func_object_p, arg2, call_args, 3), ret_value);

        /* 7.c.iii, ecma_op_to_boolean always returns a simple value, so no need to free. */
        if (ecma_is_value_true (ecma_op_to_boolean (call_value)))
        {
          ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
        }

        ECMA_FINALIZE (call_value);
        ECMA_FINALIZE (get_value);
      }

      ecma_deref_ecma_string (index_str_p);
    }

    ecma_free_completion_value (to_object_comp);
    ecma_dealloc_number (num_p);

    if (ecma_is_completion_value_empty (ret_value))
    {
      /* 8. */
      ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_some */

/**
 * The Array.prototype object's 'slice' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.10
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_slice (ecma_value_t this_arg, /**< 'this' argument */
                              ecma_value_t arg1, /**< start */
                              ecma_value_t arg2) /**< end */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t* obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t* length_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, length_magic_string_p),
                  ret_value);

  /* 3. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  /* 4. */
  const uint32_t len = ecma_number_to_uint32 (len_number);

  uint32_t start = 0, end = len;

  /* 5. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (start_num, arg1, ret_value);
  int32_t relative_start = ecma_number_to_int32 (start_num);

  /* 6. */
  if (relative_start < 0)
  {
    uint32_t start_abs = (uint32_t) -relative_start;

    if (start_abs > len)
    {
      start = 0;
    }
    else
    {
      start = len - start_abs;
    }
  }
  else
  {
    start = (uint32_t) relative_start;
    if (start > len)
    {
      start = len;
    }
  }

  /* 7. */
  if (ecma_is_value_undefined (arg2))
  {
    end = len;
  }
  else
  {
    /* 7. part 2*/
    ECMA_OP_TO_NUMBER_TRY_CATCH (end_num, arg2, ret_value)
    int32_t relative_end = ecma_number_to_int32 (end_num);

    if (relative_end < 0)
    {
      uint32_t end_abs = (uint32_t) -relative_end;

      if (end_abs > len)
      {
        end = 0;
      }
      else
      {
        end = len - end_abs;
      }
    }
    else
    {
      end = (uint32_t) relative_end;
      if (end > len)
      {
        end = len;
      }
    }

    ECMA_OP_TO_NUMBER_FINALIZE (end_num);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (start_num);

  JERRY_ASSERT (start <= len && end <= len);

  ecma_completion_value_t new_array = ecma_op_create_array_object (0, 0, false);
  ecma_object_t *new_array_p = ecma_get_object_from_completion_value (new_array);

  /* 9. */
  uint32_t n = 0;

  /* 10. */
  for (uint32_t k = start; k < end && ecma_is_completion_value_empty (ret_value); k++, n++)
  {
    /* 10.a */
    ecma_string_t *curr_idx_str_p = ecma_new_ecma_string_from_uint32 (k);

    /* 10.c */
    if (ecma_op_object_get_property (obj_p, curr_idx_str_p) != NULL)
    {
      /* 10.c.i */
      ECMA_TRY_CATCH (get_value, ecma_op_object_get (obj_p, curr_idx_str_p), ret_value);

      ecma_string_t *to_idx_str_p = ecma_new_ecma_string_from_uint32 (n);
      /*
       * 10.c.ii
       * Using [[Put]] is equivalent to using [[DefineOwnProperty]] as specified the standard,
       * so we use [[Put]] instead for simplicity. No need for a try-catch block since it is called
       * with is_throw = false.
       */
      ecma_completion_value_t put_comp_value = ecma_op_object_put (new_array_p, to_idx_str_p, get_value, false);
      JERRY_ASSERT (ecma_is_completion_value_normal (put_comp_value));
      ecma_free_completion_value (put_comp_value);
      ecma_deref_ecma_string (to_idx_str_p);

      ECMA_FINALIZE (get_value);
    }

    ecma_deref_ecma_string (curr_idx_str_p);
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    ret_value = new_array;
  }
  else
  {
    ecma_free_completion_value (new_array);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (length_magic_string_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_slice */

/**
 * The Array.prototype object's 'splice' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.12
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_splice (ecma_value_t this_arg, /**< this argument */
                                          const ecma_value_t args[], /**< arguments list */
                                          ecma_length_t args_number) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);

  /* 3. */
  ecma_string_t *length_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);

  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, length_magic_string_p),
                  ret_value);

  /* 4. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number,
                               len_value,
                               ret_value);

  const uint32_t len = ecma_number_to_uint32 (len_number);

  ecma_completion_value_t new_array = ecma_op_create_array_object (0, 0, false);
  ecma_object_t *new_array_p = ecma_get_object_from_completion_value (new_array);

  uint32_t start = 0;
  uint32_t delete_count = 0;

  if (args_number > 0)
  {
    /* 5. */
    ECMA_OP_TO_NUMBER_TRY_CATCH (start_num,
                                 args[0],
                                 ret_value);

    int32_t relative_start = ecma_number_to_int32 (start_num);

    /* 6. */
    if (relative_start < 0)
    {
      uint32_t start_abs = (uint32_t) - relative_start;
      if (start_abs > len)
      {
        start = 0;
      }
      else
      {
        start = len - start_abs;
      }
    }
    else
    {
      start = (uint32_t) relative_start;
      if (start > len)
      {
        start = len;
      }
    }

    /*
     * If there is only one argument, that will be the start argument,
     * and we must delete the additional elements.
     */
    if (args_number == 1)
    {
      delete_count = len - start;
    }
    else
    {
      /* 7. */
      ECMA_OP_TO_NUMBER_TRY_CATCH (delete_num,
                                   args[1],
                                   ret_value);

      int32_t delete_count_int = ecma_number_to_int32 (delete_num);

      if (delete_count_int > 0)
      {
        delete_count = (uint32_t) delete_count_int;
      }
      else
      {
        delete_count = 0;
      }

      if (len - start < delete_count)
      {
        delete_count = len - start;
      }

      ECMA_OP_TO_NUMBER_FINALIZE (delete_num);
    }

    ECMA_OP_TO_NUMBER_FINALIZE (start_num);
  }

  /* 8-9. */
  uint32_t k;

  for (uint32_t del_item_idx, k = 0;
       k < delete_count && ecma_is_completion_value_empty (ret_value);
       k++)
  {
    /* 9.a */
    del_item_idx = k + start;
    ecma_string_t *idx_str_p = ecma_new_ecma_string_from_uint32 (del_item_idx);

    /* 9.b */
    if (ecma_op_object_get_property (obj_p, idx_str_p) != NULL)
    {
      /* 9.c.i */
      ECMA_TRY_CATCH (get_value,
                      ecma_op_object_get (obj_p, idx_str_p),
                      ret_value);

      ecma_string_t *idx_str_new_p = ecma_new_ecma_string_from_uint32 (k);

      /* 9.c.ii
       * Using [[Put]] is equivalent to using [[DefineOwnProperty]] as specified the standard,
       * so we use [[Put]] instead for simplicity. No need for a try-catch block since it is called
       * with is_throw = false.
       */
      ECMA_TRY_CATCH (put_value,
                      ecma_op_object_put (new_array_p, idx_str_new_p, get_value, false),
                      ret_value);

      ECMA_FINALIZE (put_value);
      ecma_deref_ecma_string (idx_str_new_p);
      ECMA_FINALIZE (get_value);
    }

    ecma_deref_ecma_string (idx_str_p);
  }

  /* 11. */
  ecma_length_t item_count;

  if (args_number > 2)
  {
    item_count = (ecma_length_t) (args_number - 2);
  }
  else
  {
    item_count = 0;
  }

  const uint32_t new_len = len - delete_count + item_count;

  if (item_count != delete_count)
  {
    uint32_t from, to;

    /* 12. */
    if (item_count < delete_count)
    {
      /* 12.b */
      for (k = start; k < (len - delete_count) && ecma_is_completion_value_empty (ret_value); k++)
      {
        from = k + delete_count;
        ecma_string_t *from_str_p = ecma_new_ecma_string_from_uint32 (from);

        to = k + item_count;
        ecma_string_t *to_str_p = ecma_new_ecma_string_from_uint32 (to);

        /* 12.b.iii */
        if (ecma_op_object_get_property (obj_p, from_str_p) != NULL)
        {
          /* 12.b.iv */
          ECMA_TRY_CATCH (get_value,
                          ecma_op_object_get (obj_p, from_str_p),
                          ret_value);

          ECMA_TRY_CATCH (put_value,
                          ecma_op_object_put (obj_p, to_str_p, get_value, true),
                          ret_value);

          ECMA_FINALIZE (put_value);
          ECMA_FINALIZE (get_value);
        }
        else
        {
          /* 12.b.v */
          ECMA_TRY_CATCH (del_value,
                          ecma_op_object_delete (obj_p, to_str_p, true),
                          ret_value);

          ECMA_FINALIZE (del_value);
        }

        ecma_deref_ecma_string (to_str_p);
        ecma_deref_ecma_string (from_str_p);
      }

      /* 12.d */
      for (k = len; k > new_len && ecma_is_completion_value_empty (ret_value); k--)
      {
        ecma_string_t *str_idx_p = ecma_new_ecma_string_from_uint32 (k - 1);
        ECMA_TRY_CATCH (del_value,
                        ecma_op_object_delete (obj_p, str_idx_p, true),
                        ret_value);

        ECMA_FINALIZE (del_value);
        ecma_deref_ecma_string (str_idx_p);
      }
    }
    /* 13. */
    else if (item_count > delete_count)
    {
      /* 13.b */
      for (k = len - delete_count; k > start  && ecma_is_completion_value_empty (ret_value); k--)
      {
        from = k + delete_count - 1;
        ecma_string_t *from_str_p = ecma_new_ecma_string_from_uint32 (from);

        to = k + item_count - 1;
        ecma_string_t *to_str_p = ecma_new_ecma_string_from_uint32 (to);

        /* 13.b.iii */
        if (ecma_op_object_get_property (obj_p, from_str_p) != NULL)
        {
          /* 13.b.iv */
          ECMA_TRY_CATCH (get_value,
                          ecma_op_object_get (obj_p, from_str_p),
                          ret_value);

          ECMA_TRY_CATCH (put_value,
                          ecma_op_object_put (obj_p, to_str_p, get_value, true),
                          ret_value);

          ECMA_FINALIZE (put_value);
          ECMA_FINALIZE (get_value);
        }
        else
        {
          /* 13.b.v */
          ECMA_TRY_CATCH (del_value,
                          ecma_op_object_delete (obj_p, to_str_p, true),
                          ret_value);

          ECMA_FINALIZE (del_value);
        }

        ecma_deref_ecma_string (to_str_p);
        ecma_deref_ecma_string (from_str_p);
      }
    }
  }

  /* 15. */
  ecma_length_t idx = 0;
  for (ecma_length_t arg_index = 2;
       arg_index < args_number && ecma_is_completion_value_empty (ret_value);
       arg_index++, idx++)
  {
    ecma_string_t *str_idx_p = ecma_new_ecma_string_from_uint32 ((uint32_t) (start + idx));
    ECMA_TRY_CATCH (put_value,
                    ecma_op_object_put (obj_p, str_idx_p, args[arg_index], true),
                    ret_value);

    ECMA_FINALIZE (put_value);
    ecma_deref_ecma_string (str_idx_p);
  }

  /* 16. */
  if (ecma_is_completion_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (set_length_value,
                    ecma_builtin_array_prototype_helper_set_length (obj_p, new_len),
                    ret_value);

    ECMA_FINALIZE (set_length_value);
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    ret_value = new_array;
  }
  else
  {
    ecma_free_completion_value (new_array);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (length_magic_string_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_splice */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN */

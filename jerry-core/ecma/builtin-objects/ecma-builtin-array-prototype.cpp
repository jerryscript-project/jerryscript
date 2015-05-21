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

/* The Array.prototype object's 'every' routine
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
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN */

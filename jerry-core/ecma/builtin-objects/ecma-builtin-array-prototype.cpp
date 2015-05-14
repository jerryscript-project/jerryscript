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
#include "ecma-builtins.h"
#include "ecma-comparison.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
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
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
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
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN */

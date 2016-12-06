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

#ifndef CONFIG_DISABLE_ARRAY_BUILTIN

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
 * @return ecma value (return value of the [[Put]] method)
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_helper_set_length (ecma_object_t *object, /**< object*/
                                                ecma_number_t length) /**< new length */
{
  ecma_value_t ret_value;
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

  ecma_value_t length_value = ecma_make_number_value (length);
  ret_value = ecma_op_object_put (object,
                                  magic_string_length_p,
                                  length_value,
                                  true),

  ecma_free_value (length_value);
  ecma_deref_ecma_string (magic_string_length_p);

  return ret_value;
} /* ecma_builtin_array_prototype_helper_set_length */

/**
 * The Array.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_to_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_this_value,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this_value);

  ecma_string_t *join_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_JOIN);

  /* 2. */
  ECMA_TRY_CATCH (join_value,
                  ecma_op_object_get (obj_p, join_magic_string_p),
                  ret_value);

  if (!ecma_op_is_callable (join_value))
  {
    /* 3. */
    ret_value = ecma_builtin_helper_object_to_string (this_arg);
  }
  else
  {
    /* 4. */
    ecma_object_t *join_func_obj_p = ecma_get_object_from_value (join_value);

    ret_value = ecma_op_function_call (join_func_obj_p, this_arg, NULL, 0);
  }

  ECMA_FINALIZE (join_value);

  ecma_deref_ecma_string (join_magic_string_p);

  ECMA_FINALIZE (obj_this_value);

  return ret_value;
} /* ecma_builtin_array_prototype_object_to_string */

/**
 * The Array.prototype object's 'toLocaleString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_to_locale_string (const ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_value,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);

  ecma_string_t *length_magic_string_p = ecma_new_ecma_length_string ();

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
  ecma_string_t *separator_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_COMMA_CHAR);

  /* 5. */
  if (length == 0)
  {
    ecma_string_t *empty_string_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
    ret_value = ecma_make_string_value (empty_string_p);
  }
  else
  {
    /* 7-8. */
    ECMA_TRY_CATCH (first_value,
                    ecma_builtin_helper_get_to_locale_string_at_index (obj_p, 0),
                    ret_value);

    ecma_string_t *return_string_p = ecma_get_string_from_value (first_value);
    ecma_ref_ecma_string (return_string_p);

    /* 9-10. */
    for (uint32_t k = 1; ecma_is_value_empty (ret_value) && (k < length); k++)
    {
      ecma_string_t *part_string_p = ecma_concat_ecma_strings (return_string_p, separator_string_p);

      ECMA_TRY_CATCH (next_string_value,
                      ecma_builtin_helper_get_to_locale_string_at_index (obj_p, k),
                      ret_value);

      ecma_string_t *next_string_p = ecma_get_string_from_value (next_string_value);

      ecma_deref_ecma_string (return_string_p);

      return_string_p = ecma_concat_ecma_strings (part_string_p, next_string_p);

      ECMA_FINALIZE (next_string_value);

      ecma_deref_ecma_string (part_string_p);
    }

    if (ecma_is_value_empty (ret_value))
    {
      ret_value = ecma_make_string_value (return_string_p);
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
 * The Array.prototype object's 'concat' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_concat (ecma_value_t this_arg, /**< this argument */
                                            const ecma_value_t args[], /**< arguments list */
                                            ecma_length_t args_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  /* 2. */
  ecma_value_t new_array = ecma_op_create_array_object (0, 0, false);
  ecma_object_t *new_array_p = ecma_get_object_from_value (new_array);
  uint32_t new_length = 0;

  /* 5.b - 5.c for this_arg */
  ECMA_TRY_CATCH (concat_this_value,
                  ecma_builtin_helper_array_concat_value (new_array_p, &new_length, obj_this),
                  ret_value);

  /* 5. */
  for (uint32_t arg_index = 0;
       arg_index < args_number && ecma_is_value_empty (ret_value);
       arg_index++)
  {
    ECMA_TRY_CATCH (concat_value,
                    ecma_builtin_helper_array_concat_value (new_array_p, &new_length, args[arg_index]),
                    ret_value);
    ECMA_FINALIZE (concat_value);
  }

  ECMA_FINALIZE (concat_this_value);

  if (ecma_is_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (set_length_value,
                    ecma_builtin_array_prototype_helper_set_length (new_array_p, ((ecma_number_t) new_length)),
                    ret_value);
    ret_value = new_array;
    ECMA_FINALIZE (set_length_value);
  }
  else
  {
    ecma_free_value (new_array);
  }

  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_concat */

/**
 * The Array.prototype.toString's separator creation routine
 *
 * See also:
 *          ECMA-262 v5.1, 15.4.4.2 4th step
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */

static ecma_value_t
ecma_op_array_get_separator_string (ecma_value_t separator) /**< possible separator */
{
  if (ecma_is_value_undefined (separator))
  {
    ecma_string_t *comma_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_COMMA_CHAR);
    return ecma_make_string_value (comma_string_p);
  }
  else
  {
    return ecma_op_to_string (separator);
  }
} /* ecma_op_array_get_separator_string */

/**
 * The Array.prototype's 'toString' single element operation routine
 *
 * See also:
 *          ECMA-262 v5.1, 15.4.4.2
 *
 * @return ecma_value_t value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_op_array_get_to_string_at_index (ecma_object_t *obj_p, /**< this object */
                                      uint32_t index) /**< array index */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  ecma_string_t *index_string_p = ecma_new_ecma_string_from_uint32 (index);

  ECMA_TRY_CATCH (index_value,
                  ecma_op_object_get (obj_p, index_string_p),
                  ret_value);

  if (ecma_is_value_undefined (index_value)
      || ecma_is_value_null (index_value))
  {
    ecma_string_t *empty_string_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
    ret_value = ecma_make_string_value (empty_string_p);
  }
  else
  {
    ret_value = ecma_op_to_string (index_value);
  }

  ECMA_FINALIZE (index_value);

  ecma_deref_ecma_string (index_string_p);

  return ret_value;
} /* ecma_op_array_get_to_string_at_index */

/**
 * The Array.prototype object's 'join' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_join (const ecma_value_t this_arg, /**< this argument */
                                   const ecma_value_t separator_arg) /**< separator argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_value,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);

  ecma_string_t *length_magic_string_p = ecma_new_ecma_length_string ();

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
    ecma_string_t *empty_string_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
    ret_value = ecma_make_string_value (empty_string_p);
  }
  else
  {
    ecma_string_t *separator_string_p = ecma_get_string_from_value (separator_value);

    /* 7-8. */
    ECMA_TRY_CATCH (first_value,
                    ecma_op_array_get_to_string_at_index (obj_p, 0),
                    ret_value);

    ecma_string_t *return_string_p = ecma_get_string_from_value (first_value);
    ecma_ref_ecma_string (return_string_p);

    /* 9-10. */
    for (uint32_t k = 1; ecma_is_value_empty (ret_value) && (k < length); k++)
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

    if (ecma_is_value_empty (ret_value))
    {
      ret_value = ecma_make_string_value (return_string_p);
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
 * The Array.prototype object's 'pop' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_pop (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

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
                    ecma_builtin_array_prototype_helper_set_length (obj_p, ECMA_NUMBER_ZERO),
                    ret_value);

    /* 4.b */
    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

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
                    ecma_builtin_array_prototype_helper_set_length (obj_p, ((ecma_number_t) len)),
                    ret_value);

    ret_value = ecma_copy_value (get_value);

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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_push (ecma_value_t this_arg, /**< this argument */
                                          const ecma_value_t *argument_list_p, /**< arguments list */
                                          ecma_length_t arguments_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_this_value, ecma_op_to_object (this_arg), ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this_value);

  /* 2. */
  ecma_string_t *length_str_p = ecma_new_ecma_length_string ();

  ECMA_TRY_CATCH (length_value, ecma_op_object_get (obj_p, length_str_p), ret_value);

  /* 3. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (length_var, length_value, ret_value);

  ecma_number_t n = ((ecma_number_t) ecma_number_to_uint32 (length_var));

  /* 5. */
  for (uint32_t index = 0;
       index < arguments_number && ecma_is_value_empty (ret_value);
       index++, n++)
  {
    /* 5.a */
    ecma_value_t e_value = argument_list_p[index];

    /* 5.b */
    ecma_string_t *n_str_p = ecma_new_ecma_string_from_number (n);

    ECMA_TRY_CATCH (put_value, ecma_op_object_put (obj_p, n_str_p, e_value, true), ret_value);
    ECMA_FINALIZE (put_value);

    ecma_deref_ecma_string (n_str_p);
  }

  /* 6. */
  if (ecma_is_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (set_length_value,
                    ecma_builtin_array_prototype_helper_set_length (obj_p, n),
                    ret_value);

    ret_value = ecma_make_number_value (n);

    ECMA_FINALIZE (set_length_value)
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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_reverse (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  /* 3. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);
  uint32_t len = ecma_number_to_uint32 (len_number);

  /* 4. */
  uint32_t middle = len / 2;

  /* 5-6. */
  for (uint32_t lower = 0; lower < middle && ecma_is_value_empty (ret_value); lower++)
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
    bool lower_exist = ecma_op_object_has_property (obj_p, lower_str_p);
    bool upper_exist = ecma_op_object_has_property (obj_p, upper_str_p);

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

  if (ecma_is_value_empty (ret_value))
  {
    /* 7. */
    ret_value = ecma_copy_value (obj_this);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_reverse */

/**
 * The Array.prototype object's 'shift' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_shift (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

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
                    ecma_builtin_array_prototype_helper_set_length (obj_p, ECMA_NUMBER_ZERO),
                    ret_value);

    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

    ECMA_FINALIZE (set_length_value);
  }
  else
  {
    ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (0);

    /* 5. */
    ECMA_TRY_CATCH (first_value, ecma_op_object_get (obj_p, index_str_p), ret_value);

    /* 6. and 7. */
    for (uint32_t k = 1; k < len && ecma_is_value_empty (ret_value); k++)
    {
      /* 7.a */
      ecma_string_t *from_str_p = ecma_new_ecma_string_from_uint32 (k);
      /* 7.b */
      ecma_string_t *to_str_p = ecma_new_ecma_string_from_uint32 (k - 1);

      /* 7.c */
      ECMA_TRY_CATCH (curr_value, ecma_op_object_find (obj_p, from_str_p), ret_value);

      if (ecma_is_value_found (curr_value))
      {
        /* 7.d.i, 7.d.ii */
        ECMA_TRY_CATCH (put_value, ecma_op_object_put (obj_p, to_str_p, curr_value, true), ret_value);

        ECMA_FINALIZE (put_value);
      }
      else
      {
        /* 7.e.i */
        ECMA_TRY_CATCH (del_value, ecma_op_object_delete (obj_p, to_str_p, true), ret_value);
        ECMA_FINALIZE (del_value);
      }

      ECMA_FINALIZE (curr_value);

      ecma_deref_ecma_string (to_str_p);
      ecma_deref_ecma_string (from_str_p);
    }

    if (ecma_is_value_empty (ret_value))
    {
      len--;
      ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (len);

      /* 8. */
      ECMA_TRY_CATCH (del_value, ecma_op_object_delete (obj_p, index_str_p, true), ret_value);

      /* 9. */
      ECMA_TRY_CATCH (set_length_value,
                      ecma_builtin_array_prototype_helper_set_length (obj_p, ((ecma_number_t) len)),
                      ret_value);
      /* 10. */
      ret_value = ecma_copy_value (first_value);

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
 * The Array.prototype object's 'slice' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.10
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_slice (ecma_value_t this_arg, /**< 'this' argument */
                                           ecma_value_t arg1, /**< start */
                                           ecma_value_t arg2) /**< end */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *length_magic_string_p = ecma_new_ecma_length_string ();

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

  start = ecma_builtin_helper_array_index_normalize (start_num, len);

  /* 7. */
  if (ecma_is_value_undefined (arg2))
  {
    end = len;
  }
  else
  {
    /* 7. part 2 */
    ECMA_OP_TO_NUMBER_TRY_CATCH (end_num, arg2, ret_value);

    end = ecma_builtin_helper_array_index_normalize (end_num, len);

    ECMA_OP_TO_NUMBER_FINALIZE (end_num);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (start_num);

  JERRY_ASSERT (start <= len && end <= len);

  ecma_value_t new_array = ecma_op_create_array_object (0, 0, false);
  ecma_object_t *new_array_p = ecma_get_object_from_value (new_array);

  /* 9. */
  uint32_t n = 0;

  /* 10. */
  for (uint32_t k = start; k < end && ecma_is_value_empty (ret_value); k++, n++)
  {
    /* 10.a */
    ecma_string_t *curr_idx_str_p = ecma_new_ecma_string_from_uint32 (k);

    /* 10.c */
    ECMA_TRY_CATCH (get_value, ecma_op_object_find (obj_p, curr_idx_str_p), ret_value);

    if (ecma_is_value_found (get_value))
    {
      /* 10.c.i */
      ecma_string_t *to_idx_str_p = ecma_new_ecma_string_from_uint32 (n);

      /* 10.c.ii */
      /* This will always be a simple value since 'is_throw' is false, so no need to free. */
      ecma_value_t put_comp = ecma_builtin_helper_def_prop (new_array_p,
                                                            to_idx_str_p,
                                                            get_value,
                                                            true, /* Writable */
                                                            true, /* Enumerable */
                                                            true, /* Configurable */
                                                            false);
      JERRY_ASSERT (ecma_is_value_true (put_comp));

      ecma_deref_ecma_string (to_idx_str_p);
    }

    ECMA_FINALIZE (get_value);

    ecma_deref_ecma_string (curr_idx_str_p);
  }

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = new_array;
  }
  else
  {
    ecma_free_value (new_array);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (length_magic_string_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_slice */

/**
 * SortCompare abstract method
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.11
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_sort_compare_helper (ecma_value_t j, /**< left value */
                                                         ecma_value_t k, /**< right value */
                                                         ecma_value_t comparefn) /**< compare function */
{
  /*
   * ECMA-262 v5, 15.4.4.11 NOTE1: Because non-existent property values always
   * compare greater than undefined property values, and undefined always
   * compares greater than any other value, undefined property values always
   * sort to the end of the result, followed by non-existent property values.
   */
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  ecma_number_t result = ECMA_NUMBER_ZERO;

  bool j_is_undef = ecma_is_value_undefined (j);
  bool k_is_undef = ecma_is_value_undefined (k);

  if (j_is_undef)
  {
    if (k_is_undef)
    {
      result = ECMA_NUMBER_ZERO;
    }
    else
    {
      result = ECMA_NUMBER_ONE;
    }
  }
  else
  {
    if (k_is_undef)
    {
      result = ECMA_NUMBER_MINUS_ONE;
    }
    else
    {
      if (ecma_is_value_undefined (comparefn))
      {
        /* Default comparison when no comparefn is passed. */
        ECMA_TRY_CATCH (j_value, ecma_op_to_string (j), ret_value);
        ECMA_TRY_CATCH (k_value, ecma_op_to_string (k), ret_value);
        ecma_string_t *j_str_p = ecma_get_string_from_value (j_value);
        ecma_string_t *k_str_p = ecma_get_string_from_value (k_value);

        if (ecma_compare_ecma_strings_relational (j_str_p, k_str_p))
        {
          result = ECMA_NUMBER_MINUS_ONE;
        }
        else if (!ecma_compare_ecma_strings (j_str_p, k_str_p))
        {
          result = ECMA_NUMBER_ONE;
        }
        else
        {
          result = ECMA_NUMBER_ZERO;
        }

        ECMA_FINALIZE (k_value);
        ECMA_FINALIZE (j_value);
      }
      else
      {
        /*
         * comparefn, if not undefined, will always contain a callable function object.
         * We checked this previously, before this function was called.
         */
        JERRY_ASSERT (ecma_op_is_callable (comparefn));
        ecma_object_t *comparefn_obj_p = ecma_get_object_from_value (comparefn);

        ecma_value_t compare_args[] = {j, k};

        ECMA_TRY_CATCH (call_value,
                        ecma_op_function_call (comparefn_obj_p,
                                               ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                                               compare_args,
                                               2),
                        ret_value);

        if (!ecma_is_value_number (call_value))
        {
          ECMA_OP_TO_NUMBER_TRY_CATCH (ret_num, call_value, ret_value);
          result = ret_num;
          ECMA_OP_TO_NUMBER_FINALIZE (ret_num);
        }
        else
        {
          result = ecma_get_number_from_value (call_value);
        }

        ECMA_FINALIZE (call_value);
      }
    }
  }

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = ecma_make_number_value (result);
  }

  return ret_value;
} /* ecma_builtin_array_prototype_object_sort_compare_helper */

/**
 * Function used to reconstruct the ordered binary tree.
 * Shifts 'index' down in the tree until it is in the correct position.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_array_to_heap_helper (ecma_value_t array[], /**< heap data array */
                                                          int index, /**< current item index */
                                                          int right, /**< right index is a maximum index */
                                                          ecma_value_t comparefn) /**< compare function */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* Left child of the current index. */
  int child = index * 2 + 1;
  ecma_value_t swap = array[index];
  bool should_break = false;

  while (child <= right && ecma_is_value_empty (ret_value) && !should_break)
  {
    if (child < right)
    {
      /* Compare the two child nodes. */
      ECMA_TRY_CATCH (child_compare_value,
                      ecma_builtin_array_prototype_object_sort_compare_helper (array[child],
                                                                               array[child + 1],
                                                                               comparefn),
                      ret_value);

      JERRY_ASSERT (ecma_is_value_number (child_compare_value));

      /* Use the child that is greater. */
      if (ecma_get_number_from_value (child_compare_value) < ECMA_NUMBER_ZERO)
      {
        child++;
      }

      ECMA_FINALIZE (child_compare_value);
    }

    if (ecma_is_value_empty (ret_value))
    {
      JERRY_ASSERT (child <= right);

      /* Compare current child node with the swap (tree top). */
      ECMA_TRY_CATCH (swap_compare_value,
                      ecma_builtin_array_prototype_object_sort_compare_helper (array[child],
                                                                               swap,
                                                                               comparefn),
                      ret_value);
      JERRY_ASSERT (ecma_is_value_number (swap_compare_value));

      if (ecma_get_number_from_value (swap_compare_value) <= ECMA_NUMBER_ZERO)
      {
        /* Break from loop if current child is less than swap (tree top) */
        should_break = true;
      }
      else
      {
        /* We have to move 'swap' lower in the tree, so shift current child up in the hierarchy. */
        int parent = (child - 1) / 2;
        JERRY_ASSERT (parent >= 0 && parent <= right);
        array[parent] = array[child];

        /* Update child to be the left child of the current node. */
        child = child * 2 + 1;
      }

      ECMA_FINALIZE (swap_compare_value);
    }
  }

  if (ecma_is_value_empty (ret_value))
  {
    /*
     * Loop ended, either current child does not exist, or is less than swap.
     * This means that 'swap' should be placed in the parent node.
     */
    int parent = (child - 1) / 2;
    JERRY_ASSERT (parent >= 0 && parent <= right);
    array[parent] = swap;

    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  }

  return ret_value;
} /* ecma_builtin_array_prototype_object_array_to_heap_helper */

/**
 * Heapsort function
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_array_heap_sort_helper (ecma_value_t array[], /**< array to sort */
                                                            int right, /**< right index */
                                                            ecma_value_t comparefn) /**< compare function */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* First, construct the ordered binary tree from the array. */
  for (int i = right / 2; i >= 0 && ecma_is_value_empty (ret_value); i--)
  {
    ECMA_TRY_CATCH (value,
                    ecma_builtin_array_prototype_object_array_to_heap_helper (array,
                                                                              i,
                                                                              right,
                                                                              comparefn),
                    ret_value);
    ECMA_FINALIZE (value);
  }

  /* Sorting elements. */
  for (int i = right; i > 0 && ecma_is_value_empty (ret_value); i--)
  {
    /*
     * The top element will always contain the largest value.
     * Move top to the end, and remove it from the tree.
     */
    ecma_value_t swap = array[0];
    array[0] = array[i];
    array[i] = swap;

    /* Rebuild binary tree from the remaining elements. */
    ECMA_TRY_CATCH (value,
                    ecma_builtin_array_prototype_object_array_to_heap_helper (array,
                                                                              0,
                                                                              i - 1,
                                                                              comparefn),
                    ret_value);
    ECMA_FINALIZE (value);
  }

  return ret_value;
} /* ecma_builtin_array_prototype_object_array_heap_sort_helper */

/**
 * The Array.prototype object's 'sort' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.11
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_sort (ecma_value_t this_arg, /**< this argument */
                                          ecma_value_t arg1) /**< comparefn */
{
  /* Check if the provided compare function is callable. */
  if (!ecma_is_value_undefined (arg1) && !ecma_op_is_callable (arg1))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Compare function is not callable."));
  }

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  uint32_t len = ecma_number_to_uint32 (len_number);

  ecma_collection_header_t *array_index_props_p = ecma_op_object_get_property_names (obj_p, true, false, false);

  uint32_t defined_prop_count = 0;
  uint32_t copied_num = 0;

  ecma_collection_iterator_t iter;
  ecma_collection_iterator_init (&iter, array_index_props_p);

  /* Count properties with name that is array index less than len */
  while (ecma_collection_iterator_next (&iter)
         && ecma_is_value_empty (ret_value))
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (*iter.current_value_p);

    uint32_t index = ecma_string_get_array_index (property_name_p);
    JERRY_ASSERT (index != ECMA_STRING_NOT_ARRAY_INDEX);

    if (index < len)
    {
      defined_prop_count++;
    }
  }

  JMEM_DEFINE_LOCAL_ARRAY (values_buffer, defined_prop_count, ecma_value_t);

  ecma_collection_iterator_init (&iter, array_index_props_p);

  /* Copy unsorted array into a native c array. */
  while (ecma_collection_iterator_next (&iter)
         && ecma_is_value_empty (ret_value))
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (*iter.current_value_p);

    uint32_t index = ecma_string_get_array_index (property_name_p);
    JERRY_ASSERT (index != ECMA_STRING_NOT_ARRAY_INDEX);

    if (index >= len)
    {
      break;
    }

    ECMA_TRY_CATCH (index_value, ecma_op_object_get (obj_p, property_name_p), ret_value);

    values_buffer[copied_num++] = ecma_copy_value (index_value);

    ECMA_FINALIZE (index_value);
  }

  JERRY_ASSERT (copied_num == defined_prop_count
                || !ecma_is_value_empty (ret_value));

  /* Sorting. */
  if (copied_num > 1 && ecma_is_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (sort_value,
                    ecma_builtin_array_prototype_object_array_heap_sort_helper (values_buffer,
                                                                                (int)(copied_num - 1),
                                                                                arg1),
                    ret_value);
    ECMA_FINALIZE (sort_value);
  }

  /* Put sorted values to the front of the array. */
  for (uint32_t index = 0;
       index < copied_num && ecma_is_value_empty (ret_value);
       index++)
  {
    ecma_string_t *index_string_p = ecma_new_ecma_string_from_uint32 (index);
    ECMA_TRY_CATCH (put_value,
                    ecma_op_object_put (obj_p, index_string_p, values_buffer[index], true),
                    ret_value);
    ECMA_FINALIZE (put_value);
    ecma_deref_ecma_string (index_string_p);
  }

  /* Free values that were copied to the local array. */
  for (uint32_t index = 0; index < copied_num; index++)
  {
    ecma_free_value (values_buffer[index]);
  }

  JMEM_FINALIZE_LOCAL_ARRAY (values_buffer);

  /* Undefined properties should be in the back of the array. */

  ecma_collection_iterator_init (&iter, array_index_props_p);

  while (ecma_collection_iterator_next (&iter)
         && ecma_is_value_empty (ret_value))
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (*iter.current_value_p);

    uint32_t index = ecma_string_get_array_index (property_name_p);
    JERRY_ASSERT (index != ECMA_STRING_NOT_ARRAY_INDEX);

    if (index >= copied_num && index < len)
    {
      ECMA_TRY_CATCH (del_value, ecma_op_object_delete (obj_p, property_name_p, true), ret_value);
      ECMA_FINALIZE (del_value);
    }
  }

  ecma_free_values_collection (array_index_props_p, true);

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = ecma_copy_value (this_arg);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_sort */

/**
 * The Array.prototype object's 'splice' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.12
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_splice (ecma_value_t this_arg, /**< this argument */
                                            const ecma_value_t args[], /**< arguments list */
                                            ecma_length_t args_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);

  /* 3. */
  ecma_string_t *length_magic_string_p = ecma_new_ecma_length_string ();

  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, length_magic_string_p),
                  ret_value);

  /* 4. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number,
                               len_value,
                               ret_value);

  const uint32_t len = ecma_number_to_uint32 (len_number);

  ecma_value_t new_array = ecma_op_create_array_object (0, 0, false);
  ecma_object_t *new_array_p = ecma_get_object_from_value (new_array);

  uint32_t start = 0;
  uint32_t delete_count = 0;

  if (args_number > 0)
  {
    /* 5. */
    ECMA_OP_TO_NUMBER_TRY_CATCH (start_num,
                                 args[0],
                                 ret_value);

    start = ecma_builtin_helper_array_index_normalize (start_num, len);

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

      if (!ecma_number_is_nan (delete_num))
      {
        if (ecma_number_is_negative (delete_num))
        {
          delete_count = 0;
        }
        else
        {
          delete_count = ecma_number_is_infinity (delete_num) ? len : ecma_number_to_uint32 (delete_num);

          if (delete_count > len - start)
          {
            delete_count = len - start;
          }
        }
      }
      else
      {
        delete_count = 0;
      }

      ECMA_OP_TO_NUMBER_FINALIZE (delete_num);
    }

    ECMA_OP_TO_NUMBER_FINALIZE (start_num);
  }

  /* 8-9. */
  uint32_t k;

  for (uint32_t del_item_idx, k = 0;
       k < delete_count && ecma_is_value_empty (ret_value);
       k++)
  {
    /* 9.a */
    del_item_idx = k + start;
    ecma_string_t *idx_str_p = ecma_new_ecma_string_from_uint32 (del_item_idx);

    /* 9.b */
    ECMA_TRY_CATCH (get_value, ecma_op_object_find (obj_p, idx_str_p), ret_value);

    if (ecma_is_value_found (get_value))
    {
      /* 9.c.i */

      ecma_string_t *idx_str_new_p = ecma_new_ecma_string_from_uint32 (k);

      /* 9.c.ii */
      /* This will always be a simple value since 'is_throw' is false, so no need to free. */
      ecma_value_t put_comp = ecma_builtin_helper_def_prop (new_array_p,
                                                            idx_str_new_p,
                                                            get_value,
                                                            true, /* Writable */
                                                            true, /* Enumerable */
                                                            true, /* Configurable */
                                                            false);
      JERRY_ASSERT (ecma_is_value_true (put_comp));

      ecma_deref_ecma_string (idx_str_new_p);
    }

    ECMA_FINALIZE (get_value);

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
      for (k = start; k < (len - delete_count) && ecma_is_value_empty (ret_value); k++)
      {
        from = k + delete_count;
        ecma_string_t *from_str_p = ecma_new_ecma_string_from_uint32 (from);

        to = k + item_count;
        ecma_string_t *to_str_p = ecma_new_ecma_string_from_uint32 (to);

        /* 12.b.iii */
        ECMA_TRY_CATCH (get_value, ecma_op_object_find (obj_p, from_str_p), ret_value);

        if (ecma_is_value_found (get_value))
        {
          /* 12.b.iv */
          ECMA_TRY_CATCH (put_value,
                          ecma_op_object_put (obj_p, to_str_p, get_value, true),
                          ret_value);

          ECMA_FINALIZE (put_value);
        }
        else
        {
          /* 12.b.v */
          ECMA_TRY_CATCH (del_value,
                          ecma_op_object_delete (obj_p, to_str_p, true),
                          ret_value);

          ECMA_FINALIZE (del_value);
        }

        ECMA_FINALIZE (get_value);

        ecma_deref_ecma_string (to_str_p);
        ecma_deref_ecma_string (from_str_p);
      }

      /* 12.d */
      for (k = len; k > new_len && ecma_is_value_empty (ret_value); k--)
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
      for (k = len - delete_count; k > start  && ecma_is_value_empty (ret_value); k--)
      {
        from = k + delete_count - 1;
        ecma_string_t *from_str_p = ecma_new_ecma_string_from_uint32 (from);

        to = k + item_count - 1;
        ecma_string_t *to_str_p = ecma_new_ecma_string_from_uint32 (to);

        /* 13.b.iii */
        ECMA_TRY_CATCH (get_value, ecma_op_object_find (obj_p, from_str_p), ret_value);

        if (ecma_is_value_found (get_value))
        {
          /* 13.b.iv */
          ECMA_TRY_CATCH (put_value,
                          ecma_op_object_put (obj_p, to_str_p, get_value, true),
                          ret_value);

          ECMA_FINALIZE (put_value);
        }
        else
        {
          /* 13.b.v */
          ECMA_TRY_CATCH (del_value,
                          ecma_op_object_delete (obj_p, to_str_p, true),
                          ret_value);

          ECMA_FINALIZE (del_value);
        }

        ECMA_FINALIZE (get_value);

        ecma_deref_ecma_string (to_str_p);
        ecma_deref_ecma_string (from_str_p);
      }
    }
  }

  /* 15. */
  ecma_length_t idx = 0;
  for (ecma_length_t arg_index = 2;
       arg_index < args_number && ecma_is_value_empty (ret_value);
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
  if (ecma_is_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (set_length_value,
                    ecma_builtin_array_prototype_helper_set_length (obj_p, ((ecma_number_t) new_len)),
                    ret_value);

    ECMA_FINALIZE (set_length_value);
  }

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = new_array;
  }
  else
  {
    ecma_free_value (new_array);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (length_magic_string_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_splice */

/**
 * The Array.prototype object's 'unshift' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.13
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_unshift (ecma_value_t this_arg, /**< this argument */
                                             const ecma_value_t args[], /**< arguments list */
                                             ecma_length_t args_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  /* 3. */
  uint32_t len = ecma_number_to_uint32 (len_number);

  /* 5. and 6. */
  for (uint32_t k = len; k > 0 && ecma_is_value_empty (ret_value); k--)
  {
    /* 6.a */
    ecma_string_t *from_str_p = ecma_new_ecma_string_from_uint32 (k - 1);
    /* 6.b */
    ecma_number_t new_idx = ((ecma_number_t) k) + ((ecma_number_t) args_number) - 1;
    ecma_string_t *to_str_p = ecma_new_ecma_string_from_number (new_idx);

    /* 6.c */
    ECMA_TRY_CATCH (get_value, ecma_op_object_find (obj_p, from_str_p), ret_value);

    if (ecma_is_value_found (get_value))
    {
      /* 6.d.i, 6.d.ii */
      ECMA_TRY_CATCH (put_value, ecma_op_object_put (obj_p, to_str_p, get_value, true), ret_value);

      ECMA_FINALIZE (put_value);
    }
    else
    {
      /* 6.e.i */
      ECMA_TRY_CATCH (del_value, ecma_op_object_delete (obj_p, to_str_p, true), ret_value);
      ECMA_FINALIZE (del_value);
    }

    ECMA_FINALIZE (get_value);

    ecma_deref_ecma_string (to_str_p);
    ecma_deref_ecma_string (from_str_p);
  }

  for (uint32_t arg_index = 0;
       arg_index < args_number && ecma_is_value_empty (ret_value);
       arg_index++)
  {
    ecma_string_t *to_str_p = ecma_new_ecma_string_from_uint32 (arg_index);
    /* 9.b */
    ECMA_TRY_CATCH (put_value, ecma_op_object_put (obj_p, to_str_p, args[arg_index], true), ret_value);
    ECMA_FINALIZE (put_value);
    ecma_deref_ecma_string (to_str_p);
  }

  if (ecma_is_value_empty (ret_value))
  {
    ecma_number_t new_len = ((ecma_number_t) len) + ((ecma_number_t) args_number);
    /* 10. */
    ECMA_TRY_CATCH (set_length_value,
                    ecma_builtin_array_prototype_helper_set_length (obj_p, new_len),
                    ret_value);
    ret_value = ecma_make_number_value (new_len);

    ECMA_FINALIZE (set_length_value);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_unshift */

/**
 * The Array.prototype object's 'indexOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.14
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_index_of (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t arg1, /**< searchElement */
                                              ecma_value_t arg2) /**< fromIndex */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

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
    ret_value = ecma_make_integer_value (-1);
  }
  else
  {
    /* 5. */
    ECMA_OP_TO_NUMBER_TRY_CATCH (arg_from_idx, arg2, ret_value);

    ecma_number_t found_index = ECMA_NUMBER_MINUS_ONE;

    uint32_t from_idx = ecma_builtin_helper_array_index_normalize (arg_from_idx, len);

    /* 6. */
    if (from_idx < len)
    {
      JERRY_ASSERT (from_idx < len);

      for (; from_idx < len && found_index < 0 && ecma_is_value_empty (ret_value); from_idx++)
      {
        ecma_string_t *idx_str_p = ecma_new_ecma_string_from_uint32 (from_idx);

        /* 9.a */
        ECMA_TRY_CATCH (get_value, ecma_op_object_find (obj_p, idx_str_p), ret_value);

        if (ecma_is_value_found (get_value))
        {
          /* 9.b.i, 9.b.ii */
          if (ecma_op_strict_equality_compare (arg1, get_value))
          {
            found_index = ((ecma_number_t) from_idx);
          }
        }

        ECMA_FINALIZE (get_value);

        ecma_deref_ecma_string (idx_str_p);
      }
    }

    if (ecma_is_value_empty (ret_value))
    {
      ret_value = ecma_make_number_value (found_index);
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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_last_index_of (ecma_value_t this_arg, /**< this argument */
                                                   const ecma_value_t args[], /**< arguments list */
                                                   ecma_length_t args_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  ecma_value_t search_element = (args_number > 0) ? args[0] : ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  /* 3. */
  uint32_t len = ecma_number_to_uint32 (len_number);

  ecma_number_t num = ECMA_NUMBER_MINUS_ONE;

  /* 4. */
  if (len == 0)
  {
    ret_value = ecma_make_number_value (num);
  }
  else
  {
    uint32_t from_idx = len - 1;

    /* 5. */
    if (args_number > 1)
    {
      ECMA_OP_TO_NUMBER_TRY_CATCH (arg_from_idx, args[1], ret_value);

      if (!ecma_number_is_nan (arg_from_idx))
      {

        if (ecma_number_is_infinity (arg_from_idx))
        {
          from_idx = ecma_number_is_negative (arg_from_idx) ? (uint32_t) -1 : len - 1;
        }
        else
        {
          int32_t int_from_idx = ecma_number_to_int32 (arg_from_idx);

          /* 6. */
          if (int_from_idx >= 0)
          {
            /* min(int_from_idx, len - 1) */
            if ((uint32_t) int_from_idx > len - 1)
            {
              from_idx = len - 1;
            }
            else
            {
              from_idx = (uint32_t) int_from_idx;
            }
          }
          /* 7. */
          else
          {
            int_from_idx = -int_from_idx;

            /* We prevent from_idx from being negative, so that we can use an uint32. */
            if ((uint32_t) int_from_idx <= len)
            {
              from_idx = len - (uint32_t) int_from_idx;
            }
            else
            {
              /*
               * If from_idx would be negative, we set it to UINT_MAX. See reasoning for this in the comment
               * at the for loop below.
               */
              from_idx = (uint32_t) -1;
            }
          }
        }
      }
      else
      {
        from_idx = 0;
      }

      ECMA_OP_TO_NUMBER_FINALIZE (arg_from_idx);
    }

    /* 8.
     * We should break from the loop when from_idx < 0. We can still use an uint32_t for from_idx, and check
     * for an underflow instead. This is safe, because from_idx will always start in [0, len - 1],
     * and len is in [0, UINT_MAX], so from_idx >= len means we've had an underflow, and should stop.
     */
    for (; from_idx < len && num < 0 && ecma_is_value_empty (ret_value); from_idx--)
    {
      /* 8.a */
      ecma_string_t *idx_str_p = ecma_new_ecma_string_from_uint32 (from_idx);

      /* 8.a */
      ECMA_TRY_CATCH (get_value, ecma_op_object_find (obj_p, idx_str_p), ret_value);

      if (ecma_is_value_found (get_value))
      {
        /* 8.b.i, 8.b.ii */
        if (ecma_op_strict_equality_compare (search_element, get_value))
        {
          num = ((ecma_number_t) from_idx);
        }
      }

      ECMA_FINALIZE (get_value);

      ecma_deref_ecma_string (idx_str_p);
    }

    if (ecma_is_value_empty (ret_value))
    {
      ret_value = ecma_make_number_value (num);
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_last_index_of */

/**
 * The Array.prototype object's 'every' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.16
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_every (ecma_value_t this_arg, /**< this argument */
                                           ecma_value_t arg1, /**< callbackfn */
                                           ecma_value_t arg2) /**< thisArg */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

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
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }
  else
  {
    ecma_value_t current_index;
    ecma_object_t *func_object_p;

    /* We already checked that arg1 is callable, so it will always coerce to an object. */
    ecma_value_t to_object_comp = ecma_op_to_object (arg1);
    JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (to_object_comp));

    func_object_p = ecma_get_object_from_value (to_object_comp);

    /* 7. */
    for (uint32_t index = 0; index < len && ecma_is_value_empty (ret_value); index++)
    {
      /* 7.a */
      ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

      /* 7.c */
      ECMA_TRY_CATCH (get_value, ecma_op_object_find (obj_p, index_str_p), ret_value);

      if (ecma_is_value_found (get_value))
      {
        /* 7.c.i */
        current_index = ecma_make_uint32_value (index);

        ecma_value_t call_args[] = { get_value, current_index, obj_this };
        /* 7.c.ii */
        ECMA_TRY_CATCH (call_value, ecma_op_function_call (func_object_p, arg2, call_args, 3), ret_value);

        /* 7.c.iii */
        if (!ecma_op_to_boolean (call_value))
        {
          ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
        }

        ECMA_FINALIZE (call_value);
      }

      ECMA_FINALIZE (get_value);

      ecma_deref_ecma_string (index_str_p);
    }

    ecma_free_value (to_object_comp);

    if (ecma_is_value_empty (ret_value))
    {
      /* 8. */
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_some (ecma_value_t this_arg, /**< this argument */
                                          ecma_value_t arg1, /**< callbackfn */
                                          ecma_value_t arg2) /**< thisArg */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

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
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }
  else
  {
    ecma_value_t current_index;
    ecma_object_t *func_object_p;

    /* We already checked that arg1 is callable, so it will always coerce to an object. */
    ecma_value_t to_object_comp = ecma_op_to_object (arg1);
    JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (to_object_comp));

    func_object_p = ecma_get_object_from_value (to_object_comp);

    /* 7. */
    for (uint32_t index = 0; index < len && ecma_is_value_empty (ret_value); index++)
    {
      /* 7.a */
      ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

      /* 7.c */
      ECMA_TRY_CATCH (get_value, ecma_op_object_find (obj_p, index_str_p), ret_value);

      if (ecma_is_value_found (get_value))
      {
        /* 7.c.i */
        current_index = ecma_make_uint32_value (index);

        ecma_value_t call_args[] = { get_value, current_index, obj_this };

        /* 7.c.ii */
        ECMA_TRY_CATCH (call_value, ecma_op_function_call (func_object_p, arg2, call_args, 3), ret_value);

        /* 7.c.iii */
        if (ecma_op_to_boolean (call_value))
        {
          ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
        }

        ECMA_FINALIZE (call_value);
      }

      ECMA_FINALIZE (get_value);

      ecma_deref_ecma_string (index_str_p);
    }

    ecma_free_value (to_object_comp);

    if (ecma_is_value_empty (ret_value))
    {
      /* 8. */
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_some */

/**
 * The Array.prototype object's 'forEach' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.18
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_for_each (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t arg1, /**< callbackfn */
                                              ecma_value_t arg2) /**< thisArg */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

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
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }
  else
  {
    ecma_value_t current_index;
    ecma_object_t *func_object_p;

    /* We already checked that arg1 is callable, so it will always coerce to an object. */
    ecma_value_t to_object_comp = ecma_op_to_object (arg1);
    JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (to_object_comp));

    func_object_p = ecma_get_object_from_value (to_object_comp);

    /* Iterate over array and call callbackfn on every element */
    for (uint32_t index = 0; index < len && ecma_is_value_empty (ret_value); index++)
    {
      /* 7.a */
      ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

      /* 7.b */
      ECMA_TRY_CATCH (current_value, ecma_op_object_find (obj_p, index_str_p), ret_value);

      if (ecma_is_value_found (current_value))
      {
        /* 7.c.i */
        current_index = ecma_make_uint32_value (index);

        /* 7.c.ii */
        ecma_value_t call_args[] = {current_value, current_index, obj_this};
        ECMA_TRY_CATCH (call_value, ecma_op_function_call (func_object_p, arg2, call_args, 3), ret_value);

        ECMA_FINALIZE (call_value);
      }

      ECMA_FINALIZE (current_value);

      ecma_deref_ecma_string (index_str_p);
    }

    if (ecma_is_value_empty (ret_value))
    {
      /* 8. */
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }

    ecma_free_value (to_object_comp);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_for_each */

/**
 * The Array.prototype object's 'map' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.19
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_map (ecma_value_t this_arg, /**< this argument */
                                         ecma_value_t arg1, /**< callbackfn */
                                         ecma_value_t arg2) /**< thisArg */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

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
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }
  else
  {
    ecma_object_t *func_object_p;
    JERRY_ASSERT (ecma_is_value_object (arg1));
    func_object_p = ecma_get_object_from_value (arg1);

    /* 5. arg2 is simply used as T */

    /* 6. */
    ecma_value_t new_array = ecma_op_create_array_object (NULL, 0, false);
    JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (new_array));
    ecma_object_t *new_array_p = ecma_get_object_from_value (new_array);

    /* 7-8. */
    ecma_value_t current_index;

    for (uint32_t index = 0; index < len && ecma_is_value_empty (ret_value); index++)
    {
      /* 8.a */
      ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

      /* 8.b */
      ECMA_TRY_CATCH (current_value, ecma_op_object_find (obj_p, index_str_p), ret_value);

      if (ecma_is_value_found (current_value))
      {
        /* 8.c.i, 8.c.ii */
        current_index = ecma_make_uint32_value (index);
        ecma_value_t call_args[] = { current_value, current_index, obj_this };

        ECMA_TRY_CATCH (mapped_value, ecma_op_function_call (func_object_p, arg2, call_args, 3), ret_value);

        /* 8.c.iii */
        /* This will always be a simple value since 'is_throw' is false, so no need to free. */
        ecma_value_t put_comp = ecma_builtin_helper_def_prop (new_array_p,
                                                              index_str_p,
                                                              mapped_value,
                                                              true, /* Writable */
                                                              true, /* Enumerable */
                                                              true, /* Configurable */
                                                              false);
        JERRY_ASSERT (ecma_is_value_true (put_comp));

        ECMA_FINALIZE (mapped_value);
      }

      ECMA_FINALIZE (current_value);

      ecma_deref_ecma_string (index_str_p);
    }

    if (ecma_is_value_empty (ret_value))
    {
      ECMA_TRY_CATCH (set_length_value,
                      ecma_builtin_array_prototype_helper_set_length (new_array_p, ((ecma_number_t) len)),
                      ret_value);
      ret_value = new_array;
      ECMA_FINALIZE (set_length_value);
    }
    else
    {
      ecma_free_value (new_array);
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_map */

/**
 * The Array.prototype object's 'filter' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.20
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_filter (ecma_value_t this_arg, /**< this argument */
                                            ecma_value_t arg1, /**< callbackfn */
                                            ecma_value_t arg2) /**< thisArg */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  /* 3. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  uint32_t len = ecma_number_to_uint32 (len_number);

  /* 4. */
  if (!ecma_op_is_callable (arg1))
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }
  else
  {
    ecma_value_t current_index;
    ecma_object_t *func_object_p;

    /* 6. */
    ecma_value_t new_array = ecma_op_create_array_object (NULL, 0, false);
    JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (new_array));
    ecma_object_t *new_array_p = ecma_get_object_from_value (new_array);

    /* We already checked that arg1 is callable, so it will always be an object. */
    JERRY_ASSERT (ecma_is_value_object (arg1));
    func_object_p = ecma_get_object_from_value (arg1);

    /* 8. */
    uint32_t new_array_index = 0;

    /* 9. */
    for (uint32_t index = 0; index < len && ecma_is_value_empty (ret_value); index++)
    {
      /* 9.a */
      ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

      /* 9.c */
      ECMA_TRY_CATCH (get_value, ecma_op_object_find (obj_p, index_str_p), ret_value);

      if (ecma_is_value_found (get_value))
      {
        /* 9.c.i */
        current_index = ecma_make_uint32_value (index);

        ecma_value_t call_args[] = { get_value, current_index, obj_this };
        /* 9.c.ii */
        ECMA_TRY_CATCH (call_value, ecma_op_function_call (func_object_p, arg2, call_args, 3), ret_value);

        /* 9.c.iii */
        if (ecma_op_to_boolean (call_value))
        {
          ecma_string_t *to_index_string_p = ecma_new_ecma_string_from_uint32 (new_array_index);

          /* This will always be a simple value since 'is_throw' is false, so no need to free. */
          ecma_value_t put_comp = ecma_builtin_helper_def_prop (new_array_p,
                                                                to_index_string_p,
                                                                get_value,
                                                                true, /* Writable */
                                                                true, /* Enumerable */
                                                                true, /* Configurable */
                                                                false);
          JERRY_ASSERT (ecma_is_value_true (put_comp));

          ecma_deref_ecma_string (to_index_string_p);
          new_array_index++;
        }

        ECMA_FINALIZE (call_value);
      }

      ECMA_FINALIZE (get_value);

      ecma_deref_ecma_string (index_str_p);
    }

    if (ecma_is_value_empty (ret_value))
    {
      /* 10. */
      ret_value = new_array;
    }
    else
    {
      ecma_free_value (new_array);
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_filter */

/**
 * The Array.prototype object's 'reduce' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.21
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_reduce (ecma_value_t this_arg, /**< this argument */
                                            const ecma_value_t args[], /**< arguments list */
                                            ecma_length_t args_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  ecma_value_t callbackfn = (args_number > 0) ? args[0] : ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  ecma_value_t initial_value = (args_number > 1) ? args[1] : ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  /* 3. */
  uint32_t len = ecma_number_to_uint32 (len_number);

  /* 4. */
  if (!ecma_op_is_callable (callbackfn))
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }
  else
  {
    ecma_object_t *func_object_p;

    JERRY_ASSERT (ecma_is_value_object (callbackfn));
    func_object_p = ecma_get_object_from_value (callbackfn);
    ecma_value_t accumulator = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

    /* 5. */
    if (len_number == ECMA_NUMBER_ZERO && ecma_is_value_undefined (initial_value))
    {
      ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Initial value cannot be undefined."));
    }
    else
    {
      /* 6. */
      uint32_t index = 0;

      /* 7.a */
      if (args_number > 1)
      {
        accumulator = ecma_copy_value (initial_value);
      }
      else
      {
        /* 8.a */
        bool k_present = false;

        /* 8.b */
        while (!k_present && index < len && ecma_is_value_empty (ret_value))
        {
          /* 8.b.i */
          ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);
          k_present = true;

          /* 8.b.ii-iii */
          ECMA_TRY_CATCH (current_value, ecma_op_object_find (obj_p, index_str_p), ret_value);

          if (ecma_is_value_found (current_value))
          {
            accumulator = ecma_copy_value (current_value);
          }
          else
          {
            k_present = false;
          }

          ECMA_FINALIZE (current_value);

          /* 8.b.iv */
          index++;

          ecma_deref_ecma_string (index_str_p);
        }

        /* 8.c */
        if (!k_present)
        {
          ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Missing array element."));
        }
      }
      /* 9. */
      ecma_value_t current_index;

      for (; index < len && ecma_is_value_empty (ret_value); index++)
      {
        /* 9.a */
        ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

        /* 9.b */
        ECMA_TRY_CATCH (current_value, ecma_op_object_find (obj_p, index_str_p), ret_value);

        if (ecma_is_value_found (current_value))
        {
          /* 9.c.i, 9.c.ii */
          current_index = ecma_make_uint32_value (index);
          ecma_value_t call_args[] = {accumulator, current_value, current_index, obj_this};

          ECMA_TRY_CATCH (call_value,
                          ecma_op_function_call (func_object_p,
                                                 ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                                                 call_args,
                                                 4),
                          ret_value);

          ecma_free_value (accumulator);
          accumulator = ecma_copy_value (call_value);

          ECMA_FINALIZE (call_value);
        }

        ECMA_FINALIZE (current_value);

        ecma_deref_ecma_string (index_str_p);
        /* 9.d in for loop */
      }

      if (ecma_is_value_empty (ret_value))
      {
        ret_value = ecma_copy_value (accumulator);
      }
    }

    ecma_free_value (accumulator);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_reduce */

/**
 * The Array.prototype object's 'reduceRight' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.22
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_reduce_right (ecma_value_t this_arg, /**< this argument */
                                                  const ecma_value_t args[], /**< arguments list */
                                                  ecma_length_t args_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  ecma_value_t callbackfn = (args_number > 0) ? args[0] : ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  ecma_value_t initial_value = (args_number > 1) ? args[1] : ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

  /* 1. */
  ECMA_TRY_CATCH (obj_this,
                  ecma_op_to_object (this_arg),
                  ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

  /* 2. */
  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (obj_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  /* 3. */
  uint32_t len = ecma_number_to_uint32 (len_number);

  /* 4. */
  if (!ecma_op_is_callable (callbackfn))
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }
  else
  {
    ecma_object_t *func_object_p;

    JERRY_ASSERT (ecma_is_value_object (callbackfn));
    func_object_p = ecma_get_object_from_value (callbackfn);

    /* 5. */
    if (len_number == ECMA_NUMBER_ZERO && ecma_is_value_undefined (initial_value))
    {
      ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Initial value cannot be undefined."));
    }
    else
    {
      ecma_value_t accumulator = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

      /* 6. */
      int64_t index = (int64_t) len - 1;

      /* 7.a */
      if (args_number > 1)
      {
        accumulator = ecma_copy_value (initial_value);
      }
      else
      {
        /* 8.a */
        bool k_present = false;

        /* 8.b */
        while (!k_present && index >= 0 && ecma_is_value_empty (ret_value))
        {
          /* 8.b.i */
          ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 ((uint32_t) index);
          k_present = true;

          /* 8.b.ii-iii */
          ECMA_TRY_CATCH (current_value, ecma_op_object_find (obj_p, index_str_p), ret_value);

          if (ecma_is_value_found (current_value))
          {
            accumulator = ecma_copy_value (current_value);
          }
          else
          {
            k_present = false;
          }

          ECMA_FINALIZE (current_value);

          /* 8.b.iv */
          index--;

          ecma_deref_ecma_string (index_str_p);
        }

        /* 8.c */
        if (!k_present)
        {
          ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Missing array element."));
        }
      }
      /* 9. */
      ecma_value_t current_index;

      for (; index >= 0 && ecma_is_value_empty (ret_value); index--)
      {
        /* 9.a */
        ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 ((uint32_t) index);

        /* 9.b */
        ECMA_TRY_CATCH (current_value, ecma_op_object_find (obj_p, index_str_p), ret_value);

        if (ecma_is_value_found (current_value))
        {
          /* 9.c.i, 9.c.ii */
          current_index = ecma_make_uint32_value ((uint32_t) index);
          ecma_value_t call_args[] = {accumulator, current_value, current_index, obj_this};

          ECMA_TRY_CATCH (call_value,
                          ecma_op_function_call (func_object_p,
                                                 ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                                                 call_args,
                                                 4),
                          ret_value);

          ecma_free_value (accumulator);
          accumulator = ecma_copy_value (call_value);

          ECMA_FINALIZE (call_value);
        }

        ECMA_FINALIZE (current_value);

        ecma_deref_ecma_string (index_str_p);
        /* 9.d in for loop */
      }

      if (ecma_is_value_empty (ret_value))
      {
        ret_value = ecma_copy_value (accumulator);
      }

      ecma_free_value (accumulator);
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (obj_this);

  return ret_value;
} /* ecma_builtin_array_prototype_object_reduce_right */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_ARRAY_BUILTIN */

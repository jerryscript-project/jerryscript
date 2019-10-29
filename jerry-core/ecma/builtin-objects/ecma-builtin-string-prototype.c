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
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"
#include "ecma-objects.h"
#include "ecma-string-object.h"
#include "jcontext.h"
#include "jrt.h"
#include "jrt-libc-includes.h"
#include "lit-char-helpers.h"

#if ENABLED (JERRY_BUILTIN_REGEXP)
#include "ecma-regexp-object.h"
#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */

#if ENABLED (JERRY_BUILTIN_STRING)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/**
 * This object has a custom dispatch function.
 */
#define BUILTIN_CUSTOM_DISPATCH

/**
 * List of built-in routine identifiers.
 */
enum
{
  ECMA_STRING_PROTOTYPE_ROUTINE_START = ECMA_BUILTIN_ID__COUNT - 1,
  /* Note: These 4 routines MUST be in this order */
  ECMA_STRING_PROTOTYPE_TO_STRING,
  ECMA_STRING_PROTOTYPE_VALUE_OF,
  ECMA_STRING_PROTOTYPE_CHAR_AT,
  ECMA_STRING_PROTOTYPE_CHAR_CODE_AT,

  ECMA_STRING_PROTOTYPE_CONCAT,
  ECMA_STRING_PROTOTYPE_SLICE,

  ECMA_STRING_PROTOTYPE_LOCALE_COMPARE,

  ECMA_STRING_PROTOTYPE_MATCH,
  ECMA_STRING_PROTOTYPE_REPLACE,
  ECMA_STRING_PROTOTYPE_SEARCH,

  ECMA_STRING_PROTOTYPE_SPLIT,
  ECMA_STRING_PROTOTYPE_SUBSTRING,
  ECMA_STRING_PROTOTYPE_TO_LOWER_CASE,
  ECMA_STRING_PROTOTYPE_TO_LOCAL_LOWER_CASE,
  ECMA_STRING_PROTOTYPE_TO_UPPER_CASE,
  ECMA_STRING_PROTOTYPE_TO_LOCAL_UPPER_CASE,
  ECMA_STRING_PROTOTYPE_TRIM,

  ECMA_STRING_PROTOTYPE_SUBSTR,

  ECMA_STRING_PROTOTYPE_REPEAT,
  /* Note: These 5 routines MUST be in this order */
  ECMA_STRING_PROTOTYPE_INDEX_OF,
  ECMA_STRING_PROTOTYPE_LAST_INDEX_OF,
  ECMA_STRING_PROTOTYPE_STARTS_WITH,
  ECMA_STRING_PROTOTYPE_INCLUDES,
  ECMA_STRING_PROTOTYPE_ENDS_WITH,

  ECMA_STRING_PROTOTYPE_ITERATOR,
};

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-string-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID string_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup stringprototype ECMA String.prototype object built-in
 * @{
 */

/**
 * The String.prototype object's 'toString' and 'valueOf' routines
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.2
 *          ECMA-262 v5, 15.5.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_to_string (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_value_string (this_arg))
  {
    return ecma_copy_value (this_arg);
  }

  if (ecma_is_value_object (this_arg))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (this_arg);

    if (ecma_object_class_is (object_p, LIT_MAGIC_STRING_STRING_UL))
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      JERRY_ASSERT (ecma_is_value_string (ext_object_p->u.class_prop.u.value));

      return ecma_copy_value (ext_object_p->u.class_prop.u.value);
    }
  }

  return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a String or a String object."));
} /* ecma_builtin_string_prototype_object_to_string */

/**
 * Helper function for the String.prototype object's 'charAt' and charCodeAt' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_char_at_helper (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t arg, /**< routine's argument */
                                              bool charcode_mode) /**< routine mode */
{
  /* 3 */
  ecma_number_t index_num;
  ecma_value_t to_num_result = ecma_get_number (arg, &index_num);

  if (JERRY_UNLIKELY (!ecma_is_value_empty (to_num_result)))
  {
    return to_num_result;
  }

  /* 2 */
  ecma_string_t *original_string_p = ecma_op_to_string (this_arg);
  if (JERRY_UNLIKELY (original_string_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 4 */
  const ecma_length_t len = ecma_string_get_length (original_string_p);

  /* 5 */
  // When index_num is NaN, then the first two comparisons are false
  if (index_num < 0 || index_num >= len || (ecma_number_is_nan (index_num) && len == 0))
  {
    ecma_deref_ecma_string (original_string_p);
    return (charcode_mode ? ecma_make_nan_value ()
                          : ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY));
  }

  /* 6 */
  /*
   * String length is currently uint32_t, but index_num may be bigger,
   * ToInteger performs floor, while ToUInt32 performs modulo 2^32,
   * hence after the check 0 <= index_num < len we assume to_uint32 can be used.
   * We assume to_uint32 (NaN) is 0.
   */
  JERRY_ASSERT (ecma_number_is_nan (index_num) || ecma_number_to_uint32 (index_num) == ecma_number_trunc (index_num));

  ecma_char_t new_ecma_char = ecma_string_get_char_at_pos (original_string_p, ecma_number_to_uint32 (index_num));
  ecma_deref_ecma_string (original_string_p);

  return (charcode_mode ? ecma_make_uint32_value (new_ecma_char)
                        : ecma_make_string_value (ecma_new_ecma_string_from_code_unit (new_ecma_char)));
} /* ecma_builtin_string_prototype_char_at_helper */

/**
 * The String.prototype object's 'concat' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_concat (ecma_string_t *this_string_p, /**< this argument */
                                             const ecma_value_t *argument_list_p, /**< arguments list */
                                             ecma_length_t arguments_number) /**< number of arguments */
{
  ecma_ref_ecma_string (this_string_p);

  ecma_string_t *string_to_return = this_string_p;
  /* 5 */
  for (uint32_t arg_index = 0; arg_index < arguments_number; ++arg_index)
  {
    /* 5a, b */
    ecma_string_t *get_arg_string_p = ecma_op_to_string (argument_list_p[arg_index]);

    if (JERRY_UNLIKELY (get_arg_string_p == NULL))
    {
      ecma_deref_ecma_string (string_to_return);
      return ECMA_VALUE_ERROR;
    }

    string_to_return = ecma_concat_ecma_strings (string_to_return, get_arg_string_p);

    ecma_deref_ecma_string (get_arg_string_p);
  }

  /* 6 */
  return ecma_make_string_value (string_to_return);
} /* ecma_builtin_string_prototype_object_concat */

/**
 * The String.prototype object's 'localeCompare' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_locale_compare (ecma_string_t *this_string_p, /**< this argument */
                                                     ecma_value_t arg) /**< routine's argument */
{
  /* 3. */
  ecma_string_t *arg_string_p = ecma_op_to_string (arg);

  if (JERRY_UNLIKELY (arg_string_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_number_t result = ECMA_NUMBER_ZERO;

  if (ecma_compare_ecma_strings_relational (this_string_p, arg_string_p))
  {
    result = ECMA_NUMBER_MINUS_ONE;
  }
  else if (!ecma_compare_ecma_strings (this_string_p, arg_string_p))
  {
    result = ECMA_NUMBER_ONE;
  }
  else
  {
    result = ECMA_NUMBER_ZERO;
  }

  ecma_deref_ecma_string (arg_string_p);

  return ecma_make_number_value (result);
} /* ecma_builtin_string_prototype_object_locale_compare */

#if ENABLED (JERRY_BUILTIN_REGEXP)

/**
 * The common preparation code for 'search' and 'match' functions
 * of the String prototype.
 *
 * @return empty value on success, error value otherwise
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prepare_search (ecma_value_t regexp_arg, /**< regex argument */
                                    ecma_value_t *regexp_value) /**< [out] ptr to store the regexp object */
{
  /* 3. */
  if (ecma_is_value_object (regexp_arg)
      && ecma_object_class_is (ecma_get_object_from_value (regexp_arg), LIT_MAGIC_STRING_REGEXP_UL))
  {
    *regexp_value = ecma_copy_value (regexp_arg);
    return ECMA_VALUE_EMPTY;
  }

  /* 4. */
  ecma_value_t regexp_arguments[1] = { regexp_arg };
  ecma_value_t new_regexp_value = ecma_builtin_regexp_dispatch_construct (regexp_arguments, 1);

  if (!ECMA_IS_VALUE_ERROR (new_regexp_value))
  {
    *regexp_value = new_regexp_value;
  }

  return new_regexp_value;
} /* ecma_builtin_string_prepare_search */

/**
 * The String.prototype object's 'match' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.10
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_match (ecma_value_t this_to_string_value, /**< this argument */
                                            ecma_value_t regexp_arg) /**< routine's argument */
{
  ecma_value_t regexp_value = ECMA_VALUE_EMPTY;

  ecma_value_t ret_value = ecma_builtin_string_prepare_search (regexp_arg, &regexp_value);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  JERRY_ASSERT (!ecma_is_value_empty (regexp_value));
  ecma_object_t *regexp_obj_p = ecma_get_object_from_value (regexp_value);

  /* 5. */
  ecma_value_t global_value = ecma_op_object_get_by_magic_id (regexp_obj_p, LIT_MAGIC_STRING_GLOBAL);

  if (ECMA_IS_VALUE_ERROR (global_value))
  {
    ecma_deref_object (regexp_obj_p);
    return global_value;
  }

  ecma_string_t *this_string_p = ecma_get_string_from_value (this_to_string_value);
  ecma_ref_ecma_string (this_string_p);

  JERRY_ASSERT (ecma_is_value_boolean (global_value));

  if (ecma_is_value_false (global_value))
  {
    /* 7. */
    ret_value = ecma_regexp_exec_helper (regexp_value, this_to_string_value, false);
    ecma_deref_ecma_string (this_string_p);
    ecma_deref_object (regexp_obj_p);
    return ret_value;
  }

  /* 8.a. */
  ecma_string_t *index_zero_string_p = ecma_get_ecma_string_from_uint32 (0);

  ecma_value_t put_value = ecma_op_object_put (regexp_obj_p,
                                               ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL),
                                               ecma_make_integer_value (0),
                                               true);

  JERRY_ASSERT (ecma_is_value_boolean (put_value)
                || ecma_is_value_empty (put_value)
                || ECMA_IS_VALUE_ERROR (put_value));


  if (ECMA_IS_VALUE_ERROR (put_value))
  {
    ecma_deref_ecma_string (this_string_p);
    ecma_deref_object (regexp_obj_p);
    return put_value;
  }

  /* 8.b. */
  ecma_value_t new_array_value = ecma_op_create_array_object (NULL, 0, false);

  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (new_array_value));

  ecma_object_t *new_array_obj_p = ecma_get_object_from_value (new_array_value);

  /* 8.c. */
  ecma_number_t previous_last_index = 0;
  /* 8.d. */
  uint32_t n = 0;
  /* 8.e. */
  bool last_match = true;

  ret_value = ECMA_VALUE_ERROR;

  /* 8.f. */
  while (last_match)
  {
    /* 8.f.i. */
    ecma_value_t exec_value = ecma_regexp_exec_helper (regexp_value, this_to_string_value, false);

    if (ECMA_IS_VALUE_ERROR (exec_value))
    {
      break;
    }

    if (ecma_is_value_null (exec_value))
    {
      /* 8.f.ii. */
      break;
    }

    /* 8.f.iii. */
    ecma_value_t this_index_value = ecma_op_object_get_by_magic_id (regexp_obj_p, LIT_MAGIC_STRING_LASTINDEX_UL);

    if (ECMA_IS_VALUE_ERROR (this_index_value))
    {
      goto cleanup;
    }

    ecma_value_t this_index_number = ecma_op_to_number (this_index_value);

    if (ECMA_IS_VALUE_ERROR (this_index_value))
    {
      ecma_free_value (this_index_value);
      goto cleanup;
    }

    ecma_number_t this_index = ecma_get_number_from_value (this_index_number);

    /* 8.f.iii.2. */
    if (this_index == previous_last_index)
    {
      /* 8.f.iii.2.a. */
      ecma_value_t index_put_value = ecma_op_object_put (regexp_obj_p,
                                                         ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL),
                                                         ecma_make_number_value (this_index + 1),
                                                         true);
      if (ECMA_IS_VALUE_ERROR (index_put_value))
      {
        ecma_free_value (this_index_value);
        ecma_free_number (this_index_number);
        goto cleanup;
      }

      /* 8.f.iii.2.b. */
      previous_last_index = this_index + 1;
    }
    else
    {
      /* 8.f.iii.3. */
      previous_last_index = this_index;
    }

    /* 8.f.iii.4. */
    JERRY_ASSERT (ecma_is_value_object (exec_value));
    ecma_object_t *exec_obj_p = ecma_get_object_from_value (exec_value);

    ecma_value_t match_string_value = ecma_op_object_get (exec_obj_p, index_zero_string_p);
    JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (match_string_value));

    /* 8.f.iii.5. */
    ecma_value_t completion;
    completion = ecma_builtin_helper_def_prop_by_index (new_array_obj_p,
                                                        n,
                                                        match_string_value,
                                                        ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

    JERRY_ASSERT (ecma_is_value_true (completion));
    /* 8.f.iii.6. */
    n++;

    ecma_free_value (match_string_value);
    ecma_free_value (this_index_value);
    ecma_free_number (this_index_number);

    ecma_deref_object (exec_obj_p);
  }

  if (n == 0)
  {
    /* 8.g. */
    ret_value = ECMA_VALUE_NULL;
  }
  else
  {
    /* 8.h. */
    ecma_ref_object (new_array_obj_p);
    ret_value = new_array_value;
  }

cleanup:
  ecma_deref_object (new_array_obj_p);

  ecma_deref_object (regexp_obj_p);
  ecma_deref_ecma_string (this_string_p);

  return ret_value;
} /* ecma_builtin_string_prototype_object_match */

/**
 * This structure is the context which represents
 * the state of the ongoing string replace.
 */
typedef struct
{
  /* General part. */
  bool is_regexp; /**< whether we search a regexp or string */
  bool is_global; /**< global search or not */
  bool is_replace_callable; /**< replace part is callable or not */
  ecma_value_t input_string; /**< input string */
  ecma_length_t input_length; /**< input string length */
  ecma_value_t regexp_or_search_string; /**< regular expression or search string
                                         * depending on the value of is_regexp */
  ecma_length_t match_start; /**< starting position of the match */
  ecma_length_t match_end; /**< end position of the match */

  /* Replace value callable part. */
  ecma_object_t *replace_function_p; /**< replace function */

  /* Replace value string part. */
  ecma_string_t *replace_string_p; /**< replace string */
  lit_utf8_byte_t *replace_str_curr_p; /**< replace string iterator */
} ecma_builtin_replace_search_ctx_t;

/**
 * Generic helper function to append a substring at the end of a base string
 *
 * @return the constructed string
 */
static ecma_string_t *
ecma_builtin_string_prototype_object_replace_append_substr (ecma_string_t *base_string_p, /**< base string */
                                                            ecma_string_t *appended_string_p, /**< appended string */
                                                            ecma_length_t start, /**< start position */
                                                            ecma_length_t end) /**< end position */
{
  JERRY_ASSERT (start <= end);
  JERRY_ASSERT (end <= ecma_string_get_length (appended_string_p));

  if (start < end)
  {
    ecma_string_t *substring_p = ecma_string_substr (appended_string_p, start, end);

    base_string_p = ecma_concat_ecma_strings (base_string_p, substring_p);

    ecma_deref_ecma_string (substring_p);
  }

  return base_string_p;
} /* ecma_builtin_string_prototype_object_replace_append_substr */

/**
 * Generic helper function to perform the find the next match
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_replace_match (ecma_builtin_replace_search_ctx_t *context_p) /**< search
                                                                                                   * context */
{
  context_p->match_start = 0;
  context_p->match_end = 0;

  if (context_p->is_regexp)
  {
    ecma_value_t match_value = ecma_regexp_exec_helper (context_p->regexp_or_search_string,
                                                        context_p->input_string,
                                                        false);

    if (ECMA_IS_VALUE_ERROR (match_value))
    {
      return match_value;
    }

    if (ecma_is_value_null (match_value))
    {
      return match_value;
    }

    JERRY_ASSERT (ecma_is_value_object (match_value));

    ecma_object_t *match_object_p = ecma_get_object_from_value (match_value);

    ecma_value_t index_value = ecma_op_object_get_by_magic_id (match_object_p, LIT_MAGIC_STRING_INDEX);

    if (ECMA_IS_VALUE_ERROR (index_value))
    {
      ecma_deref_object (match_object_p);
      return index_value;
    }

    JERRY_ASSERT (ecma_is_value_number (index_value));
    ecma_value_t result_string_value = ecma_op_object_get_by_uint32_index (match_object_p, 0);

    if (ECMA_IS_VALUE_ERROR (result_string_value))
    {
      ecma_free_number (index_value);
      ecma_deref_object (match_object_p);
      return result_string_value;
    }

    /* We directly call the built-in exec, so
     * we can trust in the returned value. */

    JERRY_ASSERT (ecma_is_value_string (result_string_value));

    /* We use the length of the result string to determine the
     * match end. This works regardless the global flag is set. */
    ecma_string_t *result_string_p = ecma_get_string_from_value (result_string_value);
    ecma_number_t index_number = ecma_get_number_from_value (index_value);

    context_p->match_start = (ecma_length_t) (index_number);
    context_p->match_end = context_p->match_start + (ecma_length_t) ecma_string_get_length (result_string_p);

    JERRY_ASSERT ((ecma_length_t) ecma_number_to_uint32 (index_number) == context_p->match_start);

    ecma_deref_ecma_string (result_string_p);
    ecma_free_number (index_value);

    return match_value;
  }

  JERRY_ASSERT (!context_p->is_global);

  ecma_string_t *search_string_p = ecma_get_string_from_value (context_p->regexp_or_search_string);
  ecma_string_t *input_string_p = ecma_get_string_from_value (context_p->input_string);

  ecma_length_t index_of = 0;
  if (ecma_builtin_helper_string_find_index (input_string_p, search_string_p, true, 0, &index_of))
  {
    ecma_value_t arguments_list_p[1] = { context_p->regexp_or_search_string };

    context_p->match_start = index_of;
    context_p->match_end = index_of + ecma_string_get_length (search_string_p);

    return ecma_op_create_array_object (arguments_list_p, 1, false);
  }

  return ECMA_VALUE_NULL;
} /* ecma_builtin_string_prototype_object_replace_match */

/**
 * Generic helper function to construct the string which replaces the matched part
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_replace_get_string (ecma_builtin_replace_search_ctx_t *context_p, /**< search
                                                                                                        * context */
                                                         ecma_value_t match_value) /**< returned match value */
{
  ecma_object_t *match_object_p = ecma_get_object_from_value (match_value);
  JERRY_ASSERT (ecma_get_object_type (match_object_p) == ECMA_OBJECT_TYPE_ARRAY);

  ecma_length_t match_length = ecma_array_get_length (match_object_p);

  JERRY_ASSERT (match_length >= 1);

  if (context_p->is_replace_callable)
  {
    ecma_value_t ret_value = ECMA_VALUE_ERROR;
    JMEM_DEFINE_LOCAL_ARRAY (arguments_list,
                             match_length + 2,
                             ecma_value_t);

    /* An error might occure during the array copy and
     * uninitalized elements must not be freed. */
    ecma_length_t values_copied = 0;

    for (ecma_length_t i = 0; i < match_length ;i++)
    {
      ecma_value_t current_value = ecma_op_object_get_by_uint32_index (match_object_p, i);

      if (ECMA_IS_VALUE_ERROR (current_value))
      {
        goto cleanup;
      }

      arguments_list[i] = current_value;
      values_copied++;
    }

    arguments_list[match_length] = ecma_make_uint32_value (context_p->match_start);
    arguments_list[match_length + 1] = ecma_copy_value (context_p->input_string);

    ecma_value_t result_value = ecma_op_function_call (context_p->replace_function_p,
                                                       ECMA_VALUE_UNDEFINED,
                                                       arguments_list,
                                                       match_length + 2);

    ecma_free_value (arguments_list[match_length]);
    ecma_free_value (arguments_list[match_length + 1]);

    if (ECMA_IS_VALUE_ERROR (result_value))
    {
      goto cleanup;
    }

    ecma_string_t *to_string_p = ecma_op_to_string (result_value);
    ecma_free_value (result_value);

    if (JERRY_LIKELY (to_string_p != NULL))
    {
      ret_value = ecma_make_string_value (to_string_p);
    }

cleanup:
    for (ecma_length_t i = 0; i < values_copied; i++)
    {
      ecma_free_value (arguments_list[i]);
    }

    JMEM_FINALIZE_LOCAL_ARRAY (arguments_list);

    return ret_value;
  }

  /* Although the ECMA standard does not specify how $nn (n is a decimal
   * number) captures should be replaced if nn is greater than the maximum
   * capture index, we follow the test-262 expected behaviour:
   *
   * if maximum capture index is < 10
   *   we replace only those $n and $0n captures, where n < maximum capture index
   * otherwise
   *   we replace only those $nn captures, where nn < maximum capture index
   *
   * other $n $nn sequences left unchanged
   *
   * example: "<xy>".replace(/(x)y/, "$1,$2,$01,$12") === "<x,$2,x,x2>"
   */

  ecma_string_t *result_string_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);

  ecma_length_t previous_start = 0;
  ecma_length_t current_position = 0;

  lit_utf8_byte_t *replace_str_curr_p = context_p->replace_str_curr_p;
  lit_utf8_byte_t *replace_str_end_p = replace_str_curr_p + ecma_string_get_size (context_p->replace_string_p);

  while (replace_str_curr_p < replace_str_end_p)
  {
    ecma_char_t action = LIT_CHAR_NULL;

    if (*replace_str_curr_p != LIT_CHAR_DOLLAR_SIGN)
    {
      /* if not a continuation byte */
      if ((*replace_str_curr_p & LIT_UTF8_EXTRA_BYTE_MASK) != LIT_UTF8_EXTRA_BYTE_MARKER)
      {
        current_position++;
      }
      replace_str_curr_p++;
      continue;
    }

    replace_str_curr_p++;

    if (replace_str_curr_p < replace_str_end_p)
    {
      action = *replace_str_curr_p;

      if (action == LIT_CHAR_DOLLAR_SIGN)
      {
        current_position++;
      }
      else if (action >= LIT_CHAR_0 && action <= LIT_CHAR_9)
      {
        uint32_t index = 0;

        index = (uint32_t) (action - LIT_CHAR_0);

        if (index >= match_length)
        {
          action = LIT_CHAR_NULL;
        }
        else if (index == 0 || match_length > 10)
        {
          replace_str_curr_p++;

          if (replace_str_curr_p < replace_str_end_p)
          {
            ecma_char_t next_character = *replace_str_curr_p;

            if (next_character >= LIT_CHAR_0 && next_character <= LIT_CHAR_9)
            {
              uint32_t full_index = index * 10 + (uint32_t) (next_character - LIT_CHAR_0);
              if (full_index > 0 && full_index < match_length)
              {
                index = match_length;
              }
            }
          }

          replace_str_curr_p--;

          if (index == 0)
          {
            action = LIT_CHAR_NULL;
          }
        }
      }
      else if (action != LIT_CHAR_AMPERSAND
               && action != LIT_CHAR_GRAVE_ACCENT
               && action != LIT_CHAR_SINGLE_QUOTE)
      {
        action = LIT_CHAR_NULL;
      }
    }

    if (action != LIT_CHAR_NULL)
    {
      result_string_p = ecma_builtin_string_prototype_object_replace_append_substr (result_string_p,
                                                                                    context_p->replace_string_p,
                                                                                    previous_start,
                                                                                    current_position);
      replace_str_curr_p++;
      current_position++;

      if (action == LIT_CHAR_DOLLAR_SIGN)
      {
        current_position--;
      }
      else if (action == LIT_CHAR_GRAVE_ACCENT)
      {
        ecma_string_t *input_string_p = ecma_get_string_from_value (context_p->input_string);
        result_string_p = ecma_builtin_string_prototype_object_replace_append_substr (result_string_p,
                                                                                      input_string_p,
                                                                                      0,
                                                                                      context_p->match_start);
      }
      else if (action == LIT_CHAR_SINGLE_QUOTE)
      {
        ecma_string_t *input_string_p = ecma_get_string_from_value (context_p->input_string);
        result_string_p = ecma_builtin_string_prototype_object_replace_append_substr (result_string_p,
                                                                                      input_string_p,
                                                                                      context_p->match_end,
                                                                                      context_p->input_length);
      }
      else
      {
        /* Everything else is submatch reading. */
        uint32_t index = 0;

        JERRY_ASSERT (action == LIT_CHAR_AMPERSAND || (action >= LIT_CHAR_0 && action <= LIT_CHAR_9));

        if (action >= LIT_CHAR_0 && action <= LIT_CHAR_9)
        {
          index = (uint32_t) (action - LIT_CHAR_0);

          if ((match_length > 10 || index == 0)
              && replace_str_curr_p < replace_str_end_p)
          {
            action = *replace_str_curr_p;
            if (action >= LIT_CHAR_0 && action <= LIT_CHAR_9)
            {
              uint32_t full_index = index * 10 + (uint32_t) (action - LIT_CHAR_0);
              if (full_index < match_length)
              {
                index = full_index;
                replace_str_curr_p++;
                current_position++;
              }
            }
          }
          JERRY_ASSERT (index > 0 && index < match_length);
        }

        ecma_value_t submatch_value = ecma_op_object_get_by_uint32_index (match_object_p, index);

        if (ECMA_IS_VALUE_ERROR (submatch_value))
        {
          return submatch_value;
        }

        /* Undefined values are converted to empty string. */
        if (!ecma_is_value_undefined (submatch_value))
        {
          JERRY_ASSERT (ecma_is_value_string (submatch_value));
          ecma_string_t *submatch_string_p = ecma_get_string_from_value (submatch_value);

          result_string_p = ecma_concat_ecma_strings (result_string_p, submatch_string_p);
          ecma_free_value (submatch_value);
        }
      }

      current_position++;
      previous_start = current_position;
    }
    else
    {
      current_position++;
    }
  }

  result_string_p = ecma_builtin_string_prototype_object_replace_append_substr (result_string_p,
                                                                                context_p->replace_string_p,
                                                                                previous_start,
                                                                                current_position);

  return ecma_make_string_value (result_string_p);
} /* ecma_builtin_string_prototype_object_replace_get_string */

/**
 * Generic helper function to do the string replace
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_replace_loop (ecma_builtin_replace_search_ctx_t *context_p) /**< search
                                                                                                  * context */
{
  ecma_length_t previous_start = 0;

  ecma_string_t *result_string_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
  ecma_string_t *input_string_p = ecma_get_string_from_value (context_p->input_string);

  while (true)
  {
    ecma_value_t match_value = ecma_builtin_string_prototype_object_replace_match (context_p);

    if (ECMA_IS_VALUE_ERROR (match_value))
    {
      break;
    }

    if (!ecma_is_value_null (match_value))
    {
      result_string_p = ecma_builtin_string_prototype_object_replace_append_substr (result_string_p,
                                                                                    input_string_p,
                                                                                    previous_start,
                                                                                    context_p->match_start);

      ecma_value_t string_value = ecma_builtin_string_prototype_object_replace_get_string (context_p, match_value);

      if (ECMA_IS_VALUE_ERROR (string_value))
      {
        ecma_free_value (match_value);
        break;
      }

      JERRY_ASSERT (ecma_is_value_string (string_value));

      ecma_string_t *string_p = ecma_get_string_from_value (string_value);

      result_string_p = ecma_concat_ecma_strings (result_string_p, string_p);

      ecma_deref_ecma_string (string_p);

      previous_start = context_p->match_end;

      if (context_p->is_global
          && context_p->match_start == context_p->match_end)
      {
        JERRY_ASSERT (context_p->is_regexp);

        if (context_p->match_end == context_p->input_length)
        {
          /* Aborts the match. */
          context_p->is_global = false;
        }
        else
        {
          ecma_object_t *regexp_obj_p = ecma_get_object_from_value (context_p->regexp_or_search_string);

          ecma_value_t put_value = ecma_op_object_put (regexp_obj_p,
                                                       ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL),
                                                       ecma_make_uint32_value (context_p->match_end + 1),
                                                       true);

          JERRY_ASSERT (ecma_is_value_boolean (put_value)
                        || ecma_is_value_empty (put_value)
                        || ECMA_IS_VALUE_ERROR (put_value));

          if (ECMA_IS_VALUE_ERROR (put_value))
          {
            ecma_free_value (match_value);
            break;
          }
        }
      }
    }

    ecma_free_value (match_value);

    if (!context_p->is_global || ecma_is_value_null (match_value))
    {
      /* No more matches */
      ecma_string_t *appended_string_p;

      appended_string_p = ecma_builtin_string_prototype_object_replace_append_substr (result_string_p,
                                                                                      input_string_p,
                                                                                      previous_start,
                                                                                      context_p->input_length);
      return ecma_make_string_value (appended_string_p);
    }
  }

  ecma_deref_ecma_string (result_string_p);

  return ECMA_VALUE_ERROR;
} /* ecma_builtin_string_prototype_object_replace_loop */

/**
 * Generic helper function to check whether the search value is callable.
 * If it is not, the function converts the search value to string. The
 * appropriate fields of the context were filled as well and the search
 * loop is run afterwards.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_replace_main (ecma_builtin_replace_search_ctx_t *context_p, /**< search
                                                                                                  * context */
                                                   ecma_value_t replace_value) /**< replacement for a match */
{
  if (ecma_op_is_callable (replace_value))
  {
    context_p->is_replace_callable = true;
    context_p->replace_function_p = ecma_get_object_from_value (replace_value);

    return ecma_builtin_string_prototype_object_replace_loop (context_p);
  }


  context_p->is_replace_callable = false;

  ecma_string_t *replace_string_p = ecma_op_to_string (replace_value);

  if (JERRY_UNLIKELY (replace_string_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t ret_value;

  ECMA_STRING_TO_UTF8_STRING (replace_string_p, replace_start_p, replace_start_size);

  context_p->replace_string_p = replace_string_p;
  context_p->replace_str_curr_p = (lit_utf8_byte_t *) replace_start_p;

  ret_value = ecma_builtin_string_prototype_object_replace_loop (context_p);

  ECMA_FINALIZE_UTF8_STRING (replace_start_p, replace_start_size);
  ecma_deref_ecma_string (replace_string_p);

  return ret_value;
} /* ecma_builtin_string_prototype_object_replace_main */

/**
 * The String.prototype object's 'replace' routine
 *
 * The replace algorithm is splitted into several helper functions.
 *
 * To share data between these helper functions, we created a
 * structure called ecma_builtin_replace_search_ctx_t, which
 * represents the current state of the replace.
 *
 * The helper functions are called in the following order:
 *
 *  1) ecma_builtin_string_prototype_object_replace is called
 *       it initialise the context depending on search_value (regexp or string)
 *  2) ecma_builtin_string_prototype_object_replace_main is called
 *       it initialise the context depending on replace_value (callable or string)
 *  3) ecma_builtin_string_prototype_object_replace_loop is called
 *       this function has a loop which repeatedly calls
 *        - ecma_builtin_string_prototype_object_replace_match
 *          which performs a match
 *        - ecma_builtin_string_prototype_object_replace_get_string
 *          which computes the replacement string
 *
 *  The final string is created from several string fragments appended
 *  together by ecma_builtin_string_prototype_object_replace_append_substr.
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.11
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_replace (ecma_value_t to_string_value, /**< this argument */
                                              ecma_value_t search_value, /**< routine's first argument */
                                              ecma_value_t replace_value) /**< routine's second argument */
{
  ecma_builtin_replace_search_ctx_t context;

  if (ecma_is_value_object (search_value)
      && ecma_object_class_is (ecma_get_object_from_value (search_value), LIT_MAGIC_STRING_REGEXP_UL))
  {
    ecma_object_t *regexp_obj_p = ecma_get_object_from_value (search_value);

    ecma_value_t global_value = ecma_op_object_get_by_magic_id (regexp_obj_p, LIT_MAGIC_STRING_GLOBAL);

    if (ECMA_IS_VALUE_ERROR (global_value))
    {
      return global_value;
    }

    JERRY_ASSERT (ecma_is_value_boolean (global_value));

    context.is_regexp = true;
    context.is_global = ecma_is_value_true (global_value);
    context.input_string = to_string_value;
    context.input_length = ecma_string_get_length (ecma_get_string_from_value (to_string_value));
    context.regexp_or_search_string = search_value;

    if (context.is_global)
    {
      ecma_value_t put_value = ecma_op_object_put (regexp_obj_p,
                                                   ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL),
                                                   ecma_make_integer_value (0),
                                                   true);

      JERRY_ASSERT (ecma_is_value_boolean (put_value)
                    || ecma_is_value_empty (put_value)
                    || ECMA_IS_VALUE_ERROR (put_value));

      if (ECMA_IS_VALUE_ERROR (put_value))
      {
        return put_value;
      }
    }

    return ecma_builtin_string_prototype_object_replace_main (&context, replace_value);
  }

  ecma_string_t *to_string_search_p = ecma_op_to_string (search_value);

  if (JERRY_UNLIKELY (to_string_search_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  context.is_regexp = false;
  context.is_global = false;
  context.input_string = to_string_value;
  context.input_length = ecma_string_get_length (ecma_get_string_from_value (to_string_value));
  context.regexp_or_search_string = ecma_make_string_value (to_string_search_p);

  ecma_value_t ret_value = ecma_builtin_string_prototype_object_replace_main (&context, replace_value);

  ecma_deref_ecma_string (to_string_search_p);

  return ret_value;
} /* ecma_builtin_string_prototype_object_replace */

/**
 * The String.prototype object's 'search' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.12
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_search (ecma_value_t to_string_value, /**< this argument */
                                             ecma_value_t regexp_arg) /**< routine's argument */
{

  ecma_value_t regexp_value = ECMA_VALUE_EMPTY;

  if (ECMA_IS_VALUE_ERROR (ecma_builtin_string_prepare_search (regexp_arg, &regexp_value)))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 5. */
  ecma_string_t *this_string_p = ecma_get_string_from_value (to_string_value);
  ecma_ref_ecma_string (this_string_p);

  ecma_value_t match_result = ecma_regexp_exec_helper (regexp_value, to_string_value, true);

  ecma_value_t ret_value = ECMA_VALUE_ERROR;

  if (ECMA_IS_VALUE_ERROR (match_result))
  {
    goto cleanup;
  }

  ecma_number_t offset = -1;

  if (!ecma_is_value_null (match_result))
  {
    JERRY_ASSERT (ecma_is_value_object (match_result));

    ecma_object_t *match_object_p = ecma_get_object_from_value (match_result);

    ecma_value_t index_value = ecma_op_object_get_by_magic_id (match_object_p, LIT_MAGIC_STRING_INDEX);

    JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (index_value) && ecma_is_value_number (index_value));

    offset = ecma_get_number_from_value (index_value);

    ecma_free_number (index_value);
    ecma_free_value (match_result);
  }

  ret_value = ecma_make_number_value (offset);


cleanup:
  ecma_free_value (regexp_value);
  ecma_deref_ecma_string (this_string_p);

  /* 6. */
  return ret_value;
} /* ecma_builtin_string_prototype_object_search */

#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */

/**
 * The String.prototype object's 'slice' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.13
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_slice (ecma_string_t *get_string_val, /**< this argument */
                                            ecma_value_t arg1, /**< routine's first argument */
                                            ecma_value_t arg2) /**< routine's second argument */
{
  const ecma_length_t len = ecma_string_get_length (get_string_val);

  /* 4. */
  ecma_length_t start = 0, end = len;

  ecma_number_t start_num;

  if (ECMA_IS_VALUE_ERROR (ecma_get_number (arg1, &start_num)))
  {
    return ECMA_VALUE_ERROR;
  }

  start = ecma_builtin_helper_array_index_normalize (start_num, len, false);

  /* 5. 7. */
  if (ecma_is_value_undefined (arg2))
  {
    end = len;
  }
  else
  {
    ecma_number_t end_num;

    if (ECMA_IS_VALUE_ERROR (ecma_get_number (arg2, &end_num)))
    {
      return ECMA_VALUE_ERROR;
    }

    end = ecma_builtin_helper_array_index_normalize (end_num, len, false);
  }

  JERRY_ASSERT (start <= len && end <= len);

  /* 8-9. */
  ecma_string_t *new_str_p = ecma_string_substr (get_string_val, start, end);

  return ecma_make_string_value (new_str_p);
} /* ecma_builtin_string_prototype_object_slice */


/**
 * The String.prototype object's 'split' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.14
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_split (ecma_value_t this_to_string_val, /**< this argument */
                                            ecma_value_t arg1, /**< separator */
                                            ecma_value_t arg2) /**< limit */
{
  /* 5. */
  ecma_length_t limit = 0;

  if (ecma_is_value_undefined (arg2))
  {
    limit = (uint32_t) -1;
  }
  else
  {
    ecma_number_t limit_num;

    if (ECMA_IS_VALUE_ERROR (ecma_get_number (arg2, &limit_num)))
    {
      return ECMA_VALUE_ERROR;
    }

    limit = ecma_number_to_uint32 (limit_num);
  }

  /* 3. */
  ecma_value_t new_array = ecma_op_create_array_object (0, 0, false);

  if (limit == 0)
  {
    return new_array;
  }

  ecma_object_t *new_array_p = ecma_get_object_from_value (new_array);

  /* 10. */
  if (ecma_is_value_undefined (arg1))
  {
    ecma_value_t put_comp = ecma_builtin_helper_def_prop_by_index (new_array_p,
                                                                   0,
                                                                   this_to_string_val,
                                                                   ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

    JERRY_ASSERT (ecma_is_value_true (put_comp));

    return new_array;
  }

  /* 8. */
  ecma_value_t separator = ECMA_VALUE_EMPTY;

  bool separator_is_regexp = false;

  if (ecma_is_value_object (arg1)
      && ecma_object_class_is (ecma_get_object_from_value (arg1), LIT_MAGIC_STRING_REGEXP_UL))
  {
    separator_is_regexp = true;
    separator = ecma_copy_value (arg1);
  }
  else
  {
    ecma_string_t *separator_to_string_p = ecma_op_to_string (arg1);

    if (JERRY_UNLIKELY (separator_to_string_p == NULL))
    {
      ecma_deref_object (new_array_p);
      return ECMA_VALUE_ERROR;
    }

    separator = ecma_make_string_value (separator_to_string_p);
  }

  const ecma_string_t *this_to_string_p = ecma_get_string_from_value (this_to_string_val);

  /* 11. */
  if (ecma_string_is_empty (this_to_string_p))
  {
    bool should_return = false;

    if (separator_is_regexp)
    {
#if ENABLED (JERRY_BUILTIN_REGEXP)
      ecma_value_t regexp_value = ecma_copy_value_if_not_object (separator);
      ecma_value_t match_result;
      match_result = ecma_regexp_exec_helper (regexp_value,
                                              ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY),
                                              true);
      should_return = !ecma_is_value_null (match_result);

      if (ECMA_IS_VALUE_ERROR (match_result))
      {
        match_result = JERRY_CONTEXT (error_value);
      }

      ecma_free_value (match_result);
#else /* !ENABLED (JERRY_BUILTIN_REGEXP) */
      return ecma_raise_type_error (ECMA_ERR_MSG ("REGEXP separator is disabled in split method."));
#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */
    }
    else
    {
      ecma_string_t *separator_str_p = ecma_get_string_from_value (separator);

      if (ecma_string_get_length (separator_str_p) == 0)
      {
        should_return = true;
      }
    }

    if (!should_return)
    {
      /* 11.c */
      ecma_value_t put_comp = ecma_builtin_helper_def_prop_by_index (new_array_p,
                                                                     0,
                                                                     this_to_string_val,
                                                                     ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

      JERRY_ASSERT (ecma_is_value_true (put_comp));
    }
  }
  else
  {
    /* 4. */
    ecma_length_t new_array_length = 0;

    /* 7. */
    ecma_length_t start_pos = 0;

    /* 12. */
    ecma_length_t curr_pos = start_pos;

    bool separator_is_empty = false;
    bool should_return = false;

    /* 6. */
    const ecma_length_t string_length = ecma_string_get_length (this_to_string_p);

    while (curr_pos < string_length && !should_return)
    {
      ecma_value_t match_result = ECMA_VALUE_NULL;

      if (separator_is_regexp)
      {
#if ENABLED (JERRY_BUILTIN_REGEXP)
        ecma_value_t regexp_value = ecma_copy_value_if_not_object (separator);
        ecma_string_t *substr_str_p = ecma_string_substr (this_to_string_p, curr_pos, string_length);
        match_result = ecma_regexp_exec_helper (regexp_value, ecma_make_string_value (substr_str_p), true);
        ecma_deref_ecma_string (substr_str_p);
#else /* !ENABLED (JERRY_BUILTIN_REGEXP) */
        return ecma_raise_type_error (ECMA_ERR_MSG ("REGEXP separator is disabled in split method."));
#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */
      }
      else
      {
        ecma_string_t *separator_str_p = ecma_get_string_from_value (separator);
        ecma_length_t separator_length = ecma_string_get_length (separator_str_p);

        if (curr_pos + separator_length <= string_length)
        {
          bool is_different = false;
          for (ecma_length_t i = 0; i < separator_length && !is_different; i++)
          {
            ecma_char_t char_from_string = ecma_string_get_char_at_pos (this_to_string_p, curr_pos + i);
            ecma_char_t char_from_separator = ecma_string_get_char_at_pos (separator_str_p, i);
            is_different = (char_from_string != char_from_separator);
          }

          if (!is_different)
          {
            /* 6-7. */
            match_result = ecma_op_create_array_object (0, 0, false);
          }
        }
      }

      if (ecma_is_value_null (match_result) || ECMA_IS_VALUE_ERROR (match_result))
      {
        curr_pos++;
        if (ECMA_IS_VALUE_ERROR (match_result))
        {
          ecma_free_value (JERRY_CONTEXT (error_value));
        }
      }
      else
      {
        ecma_object_t *match_obj_p = ecma_get_object_from_value (match_result);
        ecma_string_t *zero_str_p = ecma_get_ecma_string_from_uint32 (0);
        ecma_string_t *magic_index_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INDEX);
        ecma_value_t index_prop_value;

        if (separator_is_regexp)
        {
          JERRY_ASSERT (!ecma_op_object_is_fast_array (match_obj_p));

          ecma_property_value_t *index_prop_value_p = ecma_get_named_data_property (match_obj_p, magic_index_str_p);
          ecma_number_t index_num = ecma_get_number_from_value (index_prop_value_p->value);
          ecma_value_assign_number (&index_prop_value_p->value, index_num + (ecma_number_t) curr_pos);
          index_prop_value = index_prop_value_p->value;
        }
        else
        {
          const uint32_t opts = ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE | ECMA_IS_THROW;
          ecma_string_t *separator_str_p = ecma_get_string_from_value (separator);

          ecma_value_t put_comp = ecma_builtin_helper_def_prop (match_obj_p,
                                                                zero_str_p,
                                                                ecma_make_string_value (separator_str_p),
                                                                opts);

          JERRY_ASSERT (ecma_is_value_true (put_comp));

          index_prop_value = ecma_make_uint32_value (curr_pos);

          put_comp = ecma_builtin_helper_def_prop (match_obj_p,
                                                   magic_index_str_p,
                                                   index_prop_value,
                                                   ECMA_PROPERTY_FLAG_WRITABLE);

          JERRY_ASSERT (ecma_is_value_true (put_comp));
        }

        ecma_value_t match_comp_value = ecma_op_object_get (match_obj_p, zero_str_p);
        JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (match_comp_value));

        ecma_string_t *match_str_p = ecma_get_string_from_value (match_comp_value);
        ecma_length_t match_str_length = ecma_string_get_length (match_str_p);

        separator_is_empty = ecma_string_is_empty (match_str_p);

        ecma_free_value (match_comp_value);

        ecma_number_t index_num = ecma_get_number_from_value (index_prop_value);
        JERRY_ASSERT (index_num >= 0);


        uint32_t end_pos = ecma_number_to_uint32 (index_num);

        if (separator_is_empty)
        {
          end_pos = curr_pos + 1;
        }

        /* 13.c.iii.1-2 */
        ecma_string_t *substr_str_p = ecma_string_substr (this_to_string_p,
                                                          start_pos,
                                                          end_pos);

        ecma_value_t put_comp;
        put_comp = ecma_builtin_helper_def_prop_by_index (new_array_p,
                                                          new_array_length,
                                                          ecma_make_string_value (substr_str_p),
                                                          ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

        JERRY_ASSERT (ecma_is_value_true (put_comp));

        /* 13.c.iii.3 */
        new_array_length++;

        /* 13.c.iii.4 */
        if (new_array_length == limit)
        {
          should_return = true;
        }

        /* 13.c.iii.5 */
        start_pos = end_pos + match_str_length;

        const uint32_t match_result_array_length = ecma_array_get_length (match_obj_p) - 1;

        /* 13.c.iii.6 */
        uint32_t i = 0;

        /* 13.c.iii.7 */
        while (i < match_result_array_length)
        {
          /* 13.c.iii.7.a */
          i++;
          match_comp_value = ecma_op_object_get_by_uint32_index (match_obj_p, i);

          JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (match_comp_value));

          /* 13.c.iii.7.b */
          put_comp = ecma_builtin_helper_def_prop_by_index (new_array_p,
                                                            new_array_length,
                                                            match_comp_value,
                                                            ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

          JERRY_ASSERT (ecma_is_value_true (put_comp));

          /* 13.c.iii.7.c */
          new_array_length++;

          /* 13.c.iii.7.d */
          if (new_array_length == limit)
          {
            should_return = true;
          }

          ecma_free_value (match_comp_value);
        }

        /* 13.c.iii.8 */
        curr_pos = start_pos;

        ecma_deref_ecma_string (substr_str_p);
      }

      ecma_free_value (match_result);

    }

    if (!should_return && !separator_is_empty)
    {
      /* 14. */
      ecma_string_t *substr_str_p;
      substr_str_p = ecma_string_substr (this_to_string_p,
                                         start_pos,
                                         string_length);

      /* 15. */
      ecma_value_t put_comp;
      put_comp = ecma_builtin_helper_def_prop_by_index (new_array_p,
                                                        new_array_length,
                                                        ecma_make_string_value (substr_str_p),
                                                        ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

      JERRY_ASSERT (ecma_is_value_true (put_comp));
      ecma_deref_ecma_string (substr_str_p);
    }
  }

  ecma_free_value (separator);

  return new_array;
} /* ecma_builtin_string_prototype_object_split */

/**
 * The String.prototype object's 'substring' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.15
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_substring (ecma_string_t *original_string_p, /**< this argument */
                                                ecma_value_t arg1, /**< routine's first argument */
                                                ecma_value_t arg2) /**< routine's second argument */
{
  const ecma_length_t len = ecma_string_get_length (original_string_p);

  /* 4, 6 */
  ecma_number_t start_num;

  if (ECMA_IS_VALUE_ERROR (ecma_get_number (arg1, &start_num)))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_length_t start = 0, end = len;

  start = ecma_builtin_helper_string_index_normalize (start_num, len, true);

  /* 5, 7 */
  if (ecma_is_value_undefined (arg2))
  {
    end = len;
  }
  else
  {
    ecma_number_t end_num;

    if (ECMA_IS_VALUE_ERROR (ecma_get_number (arg2, &end_num)))
    {
      return ECMA_VALUE_ERROR;
    }

    end = ecma_builtin_helper_string_index_normalize (end_num, len, true);
  }

  JERRY_ASSERT (start <= len && end <= len);

  /* 8 */
  uint32_t from = start < end ? start : end;

  /* 9 */
  uint32_t to = start > end ? start : end;

  /* 10 */
  ecma_string_t *new_str_p = ecma_string_substr (original_string_p, from, to);
  return ecma_make_string_value (new_str_p);
} /* ecma_builtin_string_prototype_object_substring */

/**
 * The common implementation of the String.prototype object's
 * 'toLowerCase', 'toLocaleLowerCase', 'toUpperCase', 'toLocalUpperCase' routines
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.16
 *          ECMA-262 v5, 15.5.4.17
 *          ECMA-262 v5, 15.5.4.18
 *          ECMA-262 v5, 15.5.4.19
 *
 * Helper function to convert a string to upper or lower case.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_conversion_helper (ecma_string_t *input_string_p, /**< this argument */
                                                        bool lower_case) /**< convert to lower (true)
                                                                          *   or upper (false) case */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* 3. */
  ECMA_STRING_TO_UTF8_STRING (input_string_p, input_start_p, input_start_size);

  /*
   * The URI encoding has two major phases: first we compute
   * the length of the lower case string, then we encode it.
   */

  lit_utf8_size_t output_length = 0;
  const lit_utf8_byte_t *input_str_curr_p = input_start_p;
  const lit_utf8_byte_t *input_str_end_p = input_start_p + input_start_size;

  while (input_str_curr_p < input_str_end_p)
  {
    ecma_char_t character = lit_utf8_read_next (&input_str_curr_p);
    ecma_char_t character_buffer[LIT_MAXIMUM_OTHER_CASE_LENGTH];
    ecma_length_t character_length;
    lit_utf8_byte_t utf8_byte_buffer[LIT_CESU8_MAX_BYTES_IN_CODE_POINT];

    if (lower_case)
    {
      character_length = lit_char_to_lower_case (character,
                                                 character_buffer,
                                                 LIT_MAXIMUM_OTHER_CASE_LENGTH);
    }
    else
    {
      character_length = lit_char_to_upper_case (character,
                                                 character_buffer,
                                                 LIT_MAXIMUM_OTHER_CASE_LENGTH);
    }

    JERRY_ASSERT (character_length >= 1 && character_length <= LIT_MAXIMUM_OTHER_CASE_LENGTH);

    for (ecma_length_t i = 0; i < character_length; i++)
    {
      output_length += lit_code_unit_to_utf8 (character_buffer[i], utf8_byte_buffer);
    }
  }

  /* Second phase. */

  JMEM_DEFINE_LOCAL_ARRAY (output_start_p,
                           output_length,
                           lit_utf8_byte_t);

  lit_utf8_byte_t *output_char_p = output_start_p;

  /* Encoding the output. */
  input_str_curr_p = input_start_p;

  while (input_str_curr_p < input_str_end_p)
  {
    ecma_char_t character = lit_utf8_read_next (&input_str_curr_p);
    ecma_char_t character_buffer[LIT_MAXIMUM_OTHER_CASE_LENGTH];
    ecma_length_t character_length;

    if (lower_case)
    {
      character_length = lit_char_to_lower_case (character,
                                                 character_buffer,
                                                 LIT_MAXIMUM_OTHER_CASE_LENGTH);
    }
    else
    {
      character_length = lit_char_to_upper_case (character,
                                                 character_buffer,
                                                 LIT_MAXIMUM_OTHER_CASE_LENGTH);
    }

    JERRY_ASSERT (character_length >= 1 && character_length <= LIT_MAXIMUM_OTHER_CASE_LENGTH);

    for (ecma_length_t i = 0; i < character_length; i++)
    {
      output_char_p += lit_code_unit_to_utf8 (character_buffer[i], output_char_p);
    }
  }

  JERRY_ASSERT (output_start_p + output_length == output_char_p);

  ecma_string_t *output_string_p = ecma_new_ecma_string_from_utf8 (output_start_p, output_length);

  ret_value = ecma_make_string_value (output_string_p);

  JMEM_FINALIZE_LOCAL_ARRAY (output_start_p);
  ECMA_FINALIZE_UTF8_STRING (input_start_p, input_start_size);

  return ret_value;
} /* ecma_builtin_string_prototype_object_conversion_helper */

/**
 * The String.prototype object's 'trim' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.20
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_trim (ecma_string_t *original_string_p) /**< this argument */
{
  ecma_string_t *trimmed_string_p = ecma_string_trim (original_string_p);

  return ecma_make_string_value (trimmed_string_p);
} /* ecma_builtin_string_prototype_object_trim */

#if ENABLED (JERRY_ES2015_BUILTIN)

/**
 * The String.prototype object's 'repeat' routine
 *
 * See also:
 *          ECMA-262 v6, 21.1.3.13
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_repeat (ecma_string_t *original_string_p, /**< this argument */
                                             ecma_value_t repeat) /**< times to repeat */
{
  ecma_string_t *ret_string_p;

  /* 4 */
  ecma_number_t count_number;
  ecma_value_t count_value = ecma_get_number (repeat, &count_number);

  /* 5 */
  if (ECMA_IS_VALUE_ERROR (count_value))
  {
    return count_value;
  }

  int32_t repeat_count = ecma_number_to_int32 (count_number);

  bool isNan = ecma_number_is_nan (count_number);

  /* 6, 7 */
  if (count_number < 0 || (!isNan && ecma_number_is_infinity (count_number)))
  {
    return ecma_raise_range_error (ECMA_ERR_MSG ("Invalid count value"));
  }

  lit_utf8_size_t size = ecma_string_get_size (original_string_p);

  if (repeat_count == 0 || size == 0 || isNan)
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  if ((uint32_t) repeat_count >= (ECMA_STRING_SIZE_LIMIT / size))
  {
    return ecma_raise_range_error (ECMA_ERR_MSG ("Invalid string length"));
  }

  lit_utf8_size_t total_size = size * (lit_utf8_size_t) repeat_count;

  JMEM_DEFINE_LOCAL_ARRAY (str_buffer, total_size, lit_utf8_byte_t);

  lit_utf8_byte_t *buffer_ptr = str_buffer;

  for (int32_t n = 0; n < repeat_count; n++)
  {
    buffer_ptr += ecma_string_copy_to_cesu8_buffer (original_string_p, buffer_ptr,
                                                    (lit_utf8_size_t) (size));
  }

  ret_string_p = ecma_new_ecma_string_from_utf8 (str_buffer, (lit_utf8_size_t) (buffer_ptr - str_buffer));
  JMEM_FINALIZE_LOCAL_ARRAY (str_buffer);

  return ecma_make_string_value (ret_string_p);
} /* ecma_builtin_string_prototype_object_repeat */

#endif /* ENABLED (JERRY_ES2015_BUILTIN) */

#if ENABLED (JERRY_BUILTIN_ANNEXB)

/**
 * The String.prototype object's 'substr' routine
 *
 * See also:
 *          ECMA-262 v5, B.2.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_substr (ecma_string_t *this_string_p, /**< this argument */
                                             ecma_value_t start, /**< routine's first argument */
                                             ecma_value_t length) /**< routine's second argument */
{
  /* 2. */
  ecma_number_t start_num;

  if (ECMA_IS_VALUE_ERROR (ecma_get_number (start, &start_num)))
  {
    return ECMA_VALUE_ERROR;
  }

  if (ecma_number_is_nan (start_num))
  {
    start_num = 0;
  }

  /* 3. */
  ecma_number_t length_num = ecma_number_make_infinity (false);

  if (!ecma_is_value_undefined (length))
  {
    ecma_number_t len;

    if (ECMA_IS_VALUE_ERROR (ecma_get_number (length, &len)))
    {
      return ECMA_VALUE_ERROR;
    }

    length_num = ecma_number_is_nan (len) ? 0 : len;
  }

  /* 4. */
  ecma_number_t this_len = (ecma_number_t) ecma_string_get_length (this_string_p);

  /* 5. */
  ecma_number_t from_num = (start_num < 0) ? JERRY_MAX (this_len + start_num, 0) : start_num;
  uint32_t from = ecma_builtin_helper_string_index_normalize (from_num, ecma_number_to_uint32 (this_len), true);

  /* 6-7. */
  ecma_number_t to_num = JERRY_MAX (JERRY_MIN (JERRY_MAX (length_num, 0), this_len - from_num), 0);
  uint32_t to = from + ecma_builtin_helper_string_index_normalize (to_num, ecma_number_to_uint32 (this_len), true);

  /* 8. */
  ecma_string_t *new_str_p = ecma_string_substr (this_string_p, from, to);
  return ecma_make_string_value (new_str_p);
} /* ecma_builtin_string_prototype_object_substr */

#endif /* ENABLED (JERRY_BUILTIN_ANNEXB) */

#if ENABLED (JERRY_ES2015_BUILTIN_ITERATOR)
/**
 * The String.prototype object's @@iterator routine
 *
 * See also:
 *          ECMA-262 v6, 21.1.3.27
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_iterator (ecma_value_t to_string) /**< this argument */
{
  return ecma_op_create_iterator_object (ecma_copy_value (to_string),
                                         ecma_builtin_get (ECMA_BUILTIN_ID_STRING_ITERATOR_PROTOTYPE),
                                         ECMA_PSEUDO_STRING_ITERATOR,
                                         0);
} /* ecma_builtin_string_prototype_object_iterator */
#endif /* ENABLED (JERRY_ES2015_BUILTIN_ITERATOR) */

/**
 * Dispatcher of the built-in's routines
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_string_prototype_dispatch_routine (uint16_t builtin_routine_id, /**< built-in wide routine
                                                                              *   identifier */
                                                ecma_value_t this_arg, /**< 'this' argument value */
                                                const ecma_value_t arguments_list_p[], /**< list of arguments
                                                                                      *   passed to routine */
                                                ecma_length_t arguments_number) /**< length of arguments' list */
{
  if (builtin_routine_id <= ECMA_STRING_PROTOTYPE_VALUE_OF)
  {
    return ecma_builtin_string_prototype_object_to_string (this_arg);
  }

  ecma_value_t coercible = ecma_op_check_object_coercible (this_arg);

  if (ECMA_IS_VALUE_ERROR (coercible))
  {
    return coercible;
  }

  ecma_value_t arg1 = arguments_list_p[0];
  ecma_value_t arg2 = arguments_list_p[1];

  if (builtin_routine_id <= ECMA_STRING_PROTOTYPE_CHAR_CODE_AT)
  {
    return ecma_builtin_string_prototype_char_at_helper (this_arg,
                                                         arg1,
                                                         builtin_routine_id == ECMA_STRING_PROTOTYPE_CHAR_CODE_AT);
  }

  ecma_string_t *string_p = ecma_op_to_string (this_arg);

  if (JERRY_UNLIKELY (string_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t to_string_val = ecma_make_string_value (string_p);
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  switch (builtin_routine_id)
  {
    case ECMA_STRING_PROTOTYPE_CONCAT:
    {
      ret_value = ecma_builtin_string_prototype_object_concat (string_p, arguments_list_p, arguments_number);
      break;
    }
    case ECMA_STRING_PROTOTYPE_SLICE:
    {
      ret_value = ecma_builtin_string_prototype_object_slice (string_p, arg1, arg2);
      break;
    }
    case ECMA_STRING_PROTOTYPE_INDEX_OF:
    case ECMA_STRING_PROTOTYPE_LAST_INDEX_OF:
#if ENABLED (JERRY_ES2015_BUILTIN)
    case ECMA_STRING_PROTOTYPE_STARTS_WITH:
    case ECMA_STRING_PROTOTYPE_INCLUDES:
    case ECMA_STRING_PROTOTYPE_ENDS_WITH:
#endif /* ENABLED (JERRY_ES2015_BUILTIN) */
    {
      ecma_string_index_of_mode_t mode;
      mode = (ecma_string_index_of_mode_t) (builtin_routine_id - ECMA_STRING_PROTOTYPE_INDEX_OF);
      ret_value = ecma_builtin_helper_string_prototype_object_index_of (string_p, arg1, arg2, mode);
      break;
    }
    case ECMA_STRING_PROTOTYPE_LOCALE_COMPARE:
    {
      ret_value = ecma_builtin_string_prototype_object_locale_compare (string_p, arg1);
      break;
    }
#if ENABLED (JERRY_BUILTIN_REGEXP)
    case ECMA_STRING_PROTOTYPE_MATCH:
    {
      ret_value = ecma_builtin_string_prototype_object_match (to_string_val, arg1);
      break;
    }
    case ECMA_STRING_PROTOTYPE_REPLACE:
    {
      ret_value = ecma_builtin_string_prototype_object_replace (to_string_val, arg1, arg2);
      break;
    }
    case ECMA_STRING_PROTOTYPE_SEARCH:
    {
      ret_value = ecma_builtin_string_prototype_object_search (to_string_val, arg1);
      break;
    }
#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */
    case ECMA_STRING_PROTOTYPE_SPLIT:
    {
      ret_value = ecma_builtin_string_prototype_object_split (to_string_val, arg1, arg2);
      break;
    }
    case ECMA_STRING_PROTOTYPE_SUBSTRING:
    {
      ret_value = ecma_builtin_string_prototype_object_substring (string_p, arg1, arg2);
      break;
    }
    case ECMA_STRING_PROTOTYPE_TO_LOWER_CASE:
    case ECMA_STRING_PROTOTYPE_TO_LOCAL_LOWER_CASE:
    case ECMA_STRING_PROTOTYPE_TO_UPPER_CASE:
    case ECMA_STRING_PROTOTYPE_TO_LOCAL_UPPER_CASE:
    {
      bool is_lower_case = builtin_routine_id <= ECMA_STRING_PROTOTYPE_TO_LOCAL_LOWER_CASE;
      ret_value = ecma_builtin_string_prototype_object_conversion_helper (string_p, is_lower_case);
      break;
    }
    case ECMA_STRING_PROTOTYPE_TRIM:
    {
      ret_value = ecma_builtin_string_prototype_object_trim (string_p);
      break;
    }
#if ENABLED (JERRY_BUILTIN_ANNEXB)
    case ECMA_STRING_PROTOTYPE_SUBSTR:
    {
      ret_value = ecma_builtin_string_prototype_object_substr (string_p, arg1, arg2);
      break;
    }
#endif /* ENABLED (JERRY_BUILTIN_ANNEXB) */
#if ENABLED (JERRY_ES2015_BUILTIN)
    case ECMA_STRING_PROTOTYPE_REPEAT:
    {
      ret_value = ecma_builtin_string_prototype_object_repeat (string_p, arg1);
      break;
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN) */
#if ENABLED (JERRY_ES2015_BUILTIN_ITERATOR)
    case ECMA_STRING_PROTOTYPE_ITERATOR:
    {
      ret_value = ecma_builtin_string_prototype_object_iterator (to_string_val);
      break;
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_ITERATOR) */
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  ecma_deref_ecma_string (string_p);

  return ret_value;
} /* ecma_builtin_string_prototype_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_STRING) */

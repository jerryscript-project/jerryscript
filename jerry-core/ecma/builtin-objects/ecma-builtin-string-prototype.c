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
#include "ecma-array-object.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
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
#include "jrt-libc-includes.h"
#include "lit-char-helpers.h"

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN
#include "ecma-regexp-object.h"
#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */

#ifndef CONFIG_DISABLE_STRING_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

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
 * The String.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.2
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
  else if (ecma_is_value_object (this_arg))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);

    if (ecma_object_get_class_name (obj_p) == LIT_MAGIC_STRING_STRING_UL)
    {
      ecma_value_t *prim_value_p = ecma_get_internal_property (obj_p,
                                                               ECMA_INTERNAL_PROPERTY_ECMA_VALUE);

      return ecma_copy_value (*prim_value_p);
    }
  }

  return ecma_raise_type_error (ECMA_ERR_MSG (""));
} /* ecma_builtin_string_prototype_object_to_string */

/**
 * The String.prototype object's 'valueOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_value_of (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_string_prototype_object_to_string (this_arg);
} /* ecma_builtin_string_prototype_object_value_of */

/**
 * The String.prototype object's 'charAt' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_char_at (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t arg) /**< routine's argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1 */
  ECMA_TRY_CATCH (check_coercible_val,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2 */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_to_string (this_arg),
                  ret_value);

  /* 3 */
  ECMA_OP_TO_NUMBER_TRY_CATCH (index_num,
                               arg,
                               ret_value);

  /* 4 */
  ecma_string_t *original_string_p = ecma_get_string_from_value (to_string_val);
  const ecma_length_t len = ecma_string_get_length (original_string_p);

  /* 5 */
  if (index_num < 0 || index_num >= len || !len)
  {
    ret_value = ecma_make_string_value (ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY));
  }
  else
  {
    /* 6 */
    ecma_char_t new_ecma_char = ecma_string_get_char_at_pos (original_string_p, ecma_number_to_uint32 (index_num));
    ret_value = ecma_make_string_value (ecma_new_ecma_string_from_code_unit (new_ecma_char));
  }

  ECMA_OP_TO_NUMBER_FINALIZE (index_num);

  ECMA_FINALIZE (to_string_val);
  ECMA_FINALIZE (check_coercible_val);

  return ret_value;
} /* ecma_builtin_string_prototype_object_char_at */

/**
 * The String.prototype object's 'charCodeAt' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_char_code_at (ecma_value_t this_arg, /**< this argument */
                                                   ecma_value_t arg) /**< routine's argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1 */
  ECMA_TRY_CATCH (check_coercible_val,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2 */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_to_string (this_arg),
                  ret_value);

  /* 3 */
  ECMA_OP_TO_NUMBER_TRY_CATCH (index_num,
                               arg,
                               ret_value);

  /* 4 */
  ecma_string_t *original_string_p = ecma_get_string_from_value (to_string_val);
  const ecma_length_t len = ecma_string_get_length (original_string_p);

  /* 5 */
  // When index_num is NaN, then the first two comparisons are false
  if (index_num < 0 || index_num >= len || (ecma_number_is_nan (index_num) && !len))
  {
    ret_value = ecma_make_nan_value ();
  }
  else
  {
    /* 6 */
    /*
     * String length is currently uit32_t, but index_num may be bigger,
     * ToInteger performs floor, while ToUInt32 performs modulo 2^32,
     * hence after the check 0 <= index_num < len we assume to_uint32 can be used.
     * We assume to_uint32 (NaN) is 0.
     */
    JERRY_ASSERT (ecma_number_is_nan (index_num) || ecma_number_to_uint32 (index_num) == ecma_number_trunc (index_num));

    ecma_char_t new_ecma_char = ecma_string_get_char_at_pos (original_string_p, ecma_number_to_uint32 (index_num));
    ret_value = ecma_make_uint32_value (new_ecma_char);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (index_num);

  ECMA_FINALIZE (to_string_val);
  ECMA_FINALIZE (check_coercible_val);

  return ret_value;
} /* ecma_builtin_string_prototype_object_char_code_at */

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
ecma_builtin_string_prototype_object_concat (ecma_value_t this_arg, /**< this argument */
                                             const ecma_value_t *argument_list_p, /**< arguments list */
                                             ecma_length_t arguments_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1 */
  ECMA_TRY_CATCH (check_coercible_val,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2 */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_to_string (this_arg),
                  ret_value);

  /* 3 */
  /* No copy performed */

  /* 4 */
  ecma_string_t *string_to_return = ecma_get_string_from_value (to_string_val);
  ecma_ref_ecma_string (string_to_return);

  /* 5 */
  for (uint32_t arg_index = 0;
       arg_index < arguments_number && ecma_is_value_empty (ret_value);
       ++arg_index)
  {
    /* 5a */
    /* 5b */
    ecma_string_t *string_temp = string_to_return;

    ECMA_TRY_CATCH (get_arg_string,
                    ecma_op_to_string (argument_list_p[arg_index]),
                    ret_value);

    string_to_return = ecma_concat_ecma_strings (string_to_return, ecma_get_string_from_value (get_arg_string));

    ecma_deref_ecma_string (string_temp);

    ECMA_FINALIZE (get_arg_string);
  }

  /* 6 */
  if (ecma_is_value_empty (ret_value))
  {
    ret_value = ecma_make_string_value (string_to_return);
  }
  else
  {
    ecma_deref_ecma_string (string_to_return);
  }

  ECMA_FINALIZE (to_string_val);
  ECMA_FINALIZE (check_coercible_val);

  return ret_value;
} /* ecma_builtin_string_prototype_object_concat */

/**
 * The String.prototype object's 'indexOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_index_of (ecma_value_t this_arg, /**< this argument */
                                               ecma_value_t arg1, /**< routine's first argument */
                                               ecma_value_t arg2) /**< routine's second argument */
{
  return ecma_builtin_helper_string_prototype_object_index_of (this_arg, arg1, arg2, true);
} /* ecma_builtin_string_prototype_object_index_of */

/**
 * The String.prototype object's 'lastIndexOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_last_index_of (ecma_value_t this_arg, /**< this argument */
                                                    ecma_value_t arg1, /**< routine's first argument */
                                                    ecma_value_t arg2) /**< routine's second argument */
{
  return ecma_builtin_helper_string_prototype_object_index_of (this_arg, arg1, arg2, false);
} /* ecma_builtin_string_prototype_object_last_index_of */

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
ecma_builtin_string_prototype_object_locale_compare (ecma_value_t this_arg, /**< this argument */
                                                     ecma_value_t arg) /**< routine's argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_check_coercible_val,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2. */
  ECMA_TRY_CATCH (this_to_string_val,
                  ecma_op_to_string (this_arg),
                  ret_value);

  /* 3. */
  ECMA_TRY_CATCH (arg_to_string_val,
                  ecma_op_to_string (arg),
                  ret_value);

  ecma_string_t *this_string_p = ecma_get_string_from_value (this_to_string_val);
  ecma_string_t *arg_string_p = ecma_get_string_from_value (arg_to_string_val);

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

  ret_value = ecma_make_number_value (result);

  ECMA_FINALIZE (arg_to_string_val);
  ECMA_FINALIZE (this_to_string_val);
  ECMA_FINALIZE (this_check_coercible_val);

  return ret_value;
} /* ecma_builtin_string_prototype_object_locale_compare */

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN

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
ecma_builtin_string_prototype_object_match (ecma_value_t this_arg, /**< this argument */
                                            ecma_value_t arg) /**< routine's argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_check_coercible_value,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2. */
  ECMA_TRY_CATCH (this_to_string_value,
                  ecma_op_to_string (this_arg),
                  ret_value);

  ecma_value_t regexp_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  /* 3. */
  if (ecma_is_value_object (arg)
      && ecma_object_get_class_name (ecma_get_object_from_value (arg)) == LIT_MAGIC_STRING_REGEXP_UL)
  {
    regexp_value = ecma_copy_value (arg);
  }
  else
  {
    /* 4. */
    ecma_value_t regexp_arguments[1] = { arg };
    ECMA_TRY_CATCH (new_regexp_value,
                    ecma_builtin_regexp_dispatch_construct (regexp_arguments, 1),
                    ret_value);

    regexp_value = ecma_copy_value (new_regexp_value);

    ECMA_FINALIZE (new_regexp_value);
  }

  if (ecma_is_value_empty (ret_value))
  {
    JERRY_ASSERT (!ecma_is_value_empty (regexp_value));
    ecma_object_t *regexp_obj_p = ecma_get_object_from_value (regexp_value);
    ecma_string_t *global_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_GLOBAL);

    /* 5. */
    ECMA_TRY_CATCH (global_value,
                    ecma_op_object_get (regexp_obj_p, global_string_p),
                    ret_value);

    JERRY_ASSERT (ecma_is_value_boolean (global_value));

    if (ecma_is_value_false (global_value))
    {
      /* 7. */
      ret_value = ecma_regexp_exec_helper (regexp_value, this_to_string_value, false);
    }
    else
    {
      /* 8.a. */
      ecma_string_t *index_zero_string_p = ecma_new_ecma_string_from_uint32 (0);

      ecma_string_t *last_index_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);

      ECMA_TRY_CATCH (put_value,
                      ecma_op_object_put (regexp_obj_p,
                                          last_index_string_p,
                                          ecma_make_integer_value (0),
                                          true),
                      ret_value);

      /* 8.b. */
      ECMA_TRY_CATCH (new_array_value,
                      ecma_op_create_array_object (NULL, 0, false),
                      ret_value);

      ecma_object_t *new_array_obj_p = ecma_get_object_from_value (new_array_value);

      /* 8.c. */
      ecma_number_t previous_last_index = 0;
      /* 8.d. */
      uint32_t n = 0;
      /* 8.e. */
      bool last_match = true;

      /* 8.f. */
      while (last_match && ecma_is_value_empty (ret_value))
      {
        /* 8.f.i. */
        ECMA_TRY_CATCH (exec_value,
                        ecma_regexp_exec_helper (regexp_value, this_to_string_value, false),
                        ret_value);

        if (ecma_is_value_null (exec_value))
        {
          /* 8.f.ii. */
          last_match = false;
        }
        else
        {
          /* 8.f.iii. */
          ECMA_TRY_CATCH (this_index_value,
                          ecma_op_object_get (regexp_obj_p, last_index_string_p),
                          ret_value);

          ECMA_TRY_CATCH (this_index_number,
                          ecma_op_to_number (this_index_value),
                          ret_value);

          ecma_number_t this_index = ecma_get_number_from_value (this_index_number);

          /* 8.f.iii.2. */
          if (this_index == previous_last_index)
          {
            /* 8.f.iii.2.a. */
            ECMA_TRY_CATCH (index_put_value,
                            ecma_op_object_put (regexp_obj_p,
                                                last_index_string_p,
                                                ecma_make_number_value (this_index + 1),
                                                true),
                            ret_value);

            /* 8.f.iii.2.b. */
            previous_last_index = this_index + 1;

            ECMA_FINALIZE (index_put_value);
          }
          else
          {
            /* 8.f.iii.3. */
            previous_last_index = this_index;
          }

          if (ecma_is_value_empty (ret_value))
          {
            /* 8.f.iii.4. */
            JERRY_ASSERT (ecma_is_value_object (exec_value));
            ecma_object_t *exec_obj_p = ecma_get_object_from_value (exec_value);

            ECMA_TRY_CATCH (match_string_value,
                            ecma_op_object_get (exec_obj_p, index_zero_string_p),
                            ret_value);

            ecma_string_t *current_index_str_p = ecma_new_ecma_string_from_uint32 (n);

            /* 8.f.iii.5. */
            ecma_value_t completion = ecma_builtin_helper_def_prop (new_array_obj_p,
                                                                    current_index_str_p,
                                                                    match_string_value,
                                                                    true, /* Writable */
                                                                    true, /* Enumerable */
                                                                    true, /* Configurable */
                                                                    false); /* Failure handling */

            JERRY_ASSERT (ecma_is_value_true (completion));

            ecma_deref_ecma_string (current_index_str_p);

            /* 8.f.iii.6. */
            n++;

            ECMA_FINALIZE (match_string_value);
          }

          ECMA_FINALIZE (this_index_number);

          ECMA_FINALIZE (this_index_value);
        }

        ECMA_FINALIZE (exec_value);
      }

      if (ecma_is_value_empty (ret_value))
      {
        if (n == 0)
        {
          /* 8.g. */
          ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
        }
        else
        {
          /* 8.h. */
          ret_value = ecma_copy_value (new_array_value);
        }
      }

      ECMA_FINALIZE (new_array_value);

      ECMA_FINALIZE (put_value);

      ecma_deref_ecma_string (last_index_string_p);
      ecma_deref_ecma_string (index_zero_string_p);
    }

    ECMA_FINALIZE (global_value);

    ecma_deref_ecma_string (global_string_p);

    ecma_free_value (regexp_value);
  }

  ECMA_FINALIZE (this_to_string_value);

  ECMA_FINALIZE (this_check_coercible_value);

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
  ecma_object_t *replace_function_p;

  /* Replace value string part. */
  ecma_string_t *replace_string_p; /**< replace string */
  lit_utf8_byte_t *replace_str_curr_p; /**< replace string iterator */
} ecma_builtin_replace_search_ctx_t;

/**
 * Generic helper function to append a substring at the end of a base string
 *
 * The base string can be kept or freed
 *
 * @return the constructed string
 */
static ecma_string_t *
ecma_builtin_string_prototype_object_replace_append_substr (ecma_string_t *base_string_p, /**< base string */
                                                            ecma_string_t *appended_string_p, /**< appended string */
                                                            ecma_length_t start, /**< start position */
                                                            ecma_length_t end, /**< end position */
                                                            bool free_base_string) /**< free base string or not */
{
  ecma_string_t *ret_string_p;

  JERRY_ASSERT (start <= end);
  JERRY_ASSERT (end <= ecma_string_get_length (appended_string_p));

  if (start < end)
  {
    ecma_string_t *substring_p = ecma_string_substr (appended_string_p, start, end);
    ret_string_p = ecma_concat_ecma_strings (base_string_p, substring_p);

    ecma_deref_ecma_string (substring_p);
    if (free_base_string)
    {
      ecma_deref_ecma_string (base_string_p);
    }
  }
  else if (free_base_string)
  {
    ret_string_p = base_string_p;
  }
  else
  {
    ret_string_p = base_string_p;
    ecma_ref_ecma_string (ret_string_p);
  }

  return ret_string_p;
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
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  context_p->match_start = 0;
  context_p->match_end = 0;

  if (context_p->is_regexp)
  {
    ECMA_TRY_CATCH (match_value,
                    ecma_regexp_exec_helper (context_p->regexp_or_search_string,
                                             context_p->input_string,
                                             false),
                    ret_value);

    if (!ecma_is_value_null (match_value))
    {
      JERRY_ASSERT (ecma_is_value_object (match_value));

      ecma_object_t *match_object_p = ecma_get_object_from_value (match_value);
      ecma_string_t *index_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_INDEX);
      ecma_string_t *zero_string_p = ecma_new_ecma_string_from_uint32 (0);

      ECMA_TRY_CATCH (index_value,
                      ecma_op_object_get (match_object_p, index_string_p),
                      ret_value);

      ECMA_TRY_CATCH (result_string_value,
                      ecma_op_object_get (match_object_p, zero_string_p),
                      ret_value);

      /* We directly call the built-in exec, so
       * we can trust in the returned value. */

      JERRY_ASSERT (ecma_is_value_number (index_value));
      JERRY_ASSERT (ecma_is_value_string (result_string_value));

      /* We use the length of the result string to determine the
       * match end. This works regardless the global flag is set. */
      ecma_string_t *result_string_p = ecma_get_string_from_value (result_string_value);
      ecma_number_t index_number = ecma_get_number_from_value (index_value);

      context_p->match_start = (ecma_length_t) (index_number);
      context_p->match_end = context_p->match_start + (ecma_length_t) ecma_string_get_length (result_string_p);

      JERRY_ASSERT ((ecma_length_t) ecma_number_to_uint32 (index_number) == context_p->match_start);

      ret_value = ecma_copy_value (match_value);

      ECMA_FINALIZE (result_string_value);
      ECMA_FINALIZE (index_value);
      ecma_deref_ecma_string (index_string_p);
      ecma_deref_ecma_string (zero_string_p);
    }
    else
    {
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
    }

    ECMA_FINALIZE (match_value);
  }
  else
  {
    JERRY_ASSERT (!context_p->is_global);

    ecma_string_t *search_string_p = ecma_get_string_from_value (context_p->regexp_or_search_string);
    ecma_string_t *input_string_p = ecma_get_string_from_value (context_p->input_string);

    ecma_length_t index_of = 0;
    if (ecma_builtin_helper_string_find_index (input_string_p, search_string_p, true, 0, &index_of))
    {
      ecma_value_t arguments_list_p[1] = { context_p->regexp_or_search_string };
      ECMA_TRY_CATCH (new_array_value,
                      ecma_op_create_array_object (arguments_list_p, 1, false),
                      ret_value);

      context_p->match_start = index_of;
      context_p->match_end = index_of + ecma_string_get_length (search_string_p);

      ret_value = ecma_copy_value (new_array_value);

      ECMA_FINALIZE (new_array_value);
    }
    else
    {
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
    }
  }

  return ret_value;
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
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  ecma_string_t *length_string_p = ecma_new_ecma_length_string ();
  ecma_object_t *match_object_p = ecma_get_object_from_value (match_value);

  ECMA_TRY_CATCH (match_length_value,
                  ecma_op_object_get (match_object_p, length_string_p),
                  ret_value);

  JERRY_ASSERT (ecma_is_value_number (match_length_value));

  ecma_number_t match_length_number = ecma_get_number_from_value (match_length_value);
  ecma_length_t match_length = (ecma_length_t) (match_length_number);

  JERRY_ASSERT ((ecma_length_t) ecma_number_to_uint32 (match_length_number) == match_length);
  JERRY_ASSERT (match_length >= 1);

  if (context_p->is_replace_callable)
  {
    JMEM_DEFINE_LOCAL_ARRAY (arguments_list,
                             match_length + 2,
                             ecma_value_t);

    /* An error might occure during the array copy and
     * uninitalized elements must not be freed. */
    ecma_length_t values_copied = 0;

    for (ecma_length_t i = 0;
         (i < match_length) && ecma_is_value_empty (ret_value);
         i++)
    {
      ecma_string_t *index_p = ecma_new_ecma_string_from_uint32 (i);
      ECMA_TRY_CATCH (current_value,
                      ecma_op_object_get (match_object_p, index_p),
                      ret_value);

      arguments_list[i] = ecma_copy_value (current_value);
      values_copied++;

      ECMA_FINALIZE (current_value);
      ecma_deref_ecma_string (index_p);
    }

    if (ecma_is_value_empty (ret_value))
    {
      arguments_list[match_length] = ecma_make_number_value (context_p->match_start);
      arguments_list[match_length + 1] = ecma_copy_value (context_p->input_string);

      ECMA_TRY_CATCH (result_value,
                      ecma_op_function_call (context_p->replace_function_p,
                                             ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                                             arguments_list,
                                             match_length + 2),
                      ret_value);

      ECMA_TRY_CATCH (to_string_value,
                      ecma_op_to_string (result_value),
                      ret_value);

      ret_value = ecma_copy_value (to_string_value);

      ECMA_FINALIZE (to_string_value);
      ECMA_FINALIZE (result_value);

      ecma_free_value (arguments_list[match_length + 1]);
    }

    for (ecma_length_t i = 0; i < values_copied; i++)
    {
      ecma_free_value (arguments_list[i]);
    }

    JMEM_FINALIZE_LOCAL_ARRAY (arguments_list);
  }
  else
  {
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

      if (*replace_str_curr_p++ == LIT_CHAR_DOLLAR_SIGN)
      {
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

              ecma_char_t next_character = *replace_str_curr_p;

              if (next_character >= LIT_CHAR_0 && next_character <= LIT_CHAR_9)
              {
                uint32_t full_index = index * 10 + (uint32_t) (next_character - LIT_CHAR_0);
                if (full_index > 0 && full_index < match_length)
                {
                  index = match_length;
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
      }

      if (action != LIT_CHAR_NULL)
      {
        result_string_p = ecma_builtin_string_prototype_object_replace_append_substr (result_string_p,
                                                                                      context_p->replace_string_p,
                                                                                      previous_start,
                                                                                      current_position,
                                                                                      true);
        replace_str_curr_p++;

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
                                                                                        context_p->match_start,
                                                                                        true);
        }
        else if (action == LIT_CHAR_SINGLE_QUOTE)
        {
          ecma_string_t *input_string_p = ecma_get_string_from_value (context_p->input_string);
          result_string_p = ecma_builtin_string_prototype_object_replace_append_substr (result_string_p,
                                                                                        input_string_p,
                                                                                        context_p->match_end,
                                                                                        context_p->input_length,
                                                                                        true);
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

          ecma_string_t *index_string_p = ecma_new_ecma_string_from_uint32 (index);
          ecma_object_t *match_object_p = ecma_get_object_from_value (match_value);

          ECMA_TRY_CATCH (submatch_value,
                          ecma_op_object_get (match_object_p, index_string_p),
                          ret_value);

          /* Undefined values are converted to empty string. */
          if (!ecma_is_value_undefined (submatch_value))
          {
            JERRY_ASSERT (ecma_is_value_string (submatch_value));
            ecma_string_t *submatch_string_p = ecma_get_string_from_value (submatch_value);

            ecma_string_t *appended_string_p = ecma_concat_ecma_strings (result_string_p, submatch_string_p);
            ecma_deref_ecma_string (result_string_p);
            result_string_p = appended_string_p;
          }

          ECMA_FINALIZE (submatch_value);
          ecma_deref_ecma_string (index_string_p);

          if (!ecma_is_value_empty (ret_value))
          {
            break;
          }
        }

        current_position++;
        previous_start = current_position + 1;
      }

      /* if not a continuation byte */
      if ((*replace_str_curr_p & LIT_UTF8_EXTRA_BYTE_MASK) != LIT_UTF8_EXTRA_BYTE_MARKER)
      {
        current_position++;
      }
    }

    if (ecma_is_value_empty (ret_value))
    {
      result_string_p = ecma_builtin_string_prototype_object_replace_append_substr (result_string_p,
                                                                                    context_p->replace_string_p,
                                                                                    previous_start,
                                                                                    current_position,
                                                                                    true);

      ret_value = ecma_make_string_value (result_string_p);
    }
  }

  ECMA_FINALIZE (match_length_value);
  ecma_deref_ecma_string (length_string_p);

  return ret_value;
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
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  ecma_length_t previous_start = 0;
  bool continue_match = true;

  ecma_string_t *result_string_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
  ecma_string_t *input_string_p = ecma_get_string_from_value (context_p->input_string);

  while (continue_match)
  {
    continue_match = false;
    ECMA_TRY_CATCH (match_value,
                    ecma_builtin_string_prototype_object_replace_match (context_p),
                    ret_value);

    if (!ecma_is_value_null (match_value))
    {
      result_string_p = ecma_builtin_string_prototype_object_replace_append_substr (result_string_p,
                                                                                    input_string_p,
                                                                                    previous_start,
                                                                                    context_p->match_start,
                                                                                    true);

      ECMA_TRY_CATCH (string_value,
                      ecma_builtin_string_prototype_object_replace_get_string (context_p, match_value),
                      ret_value);

      JERRY_ASSERT (ecma_is_value_string (string_value));

      ecma_string_t *appended_string_p = ecma_concat_ecma_strings (result_string_p,
                                                                   ecma_get_string_from_value (string_value));

      ecma_deref_ecma_string (result_string_p);
      result_string_p = appended_string_p;

      ECMA_FINALIZE (string_value);

      previous_start = context_p->match_end;

      if (context_p->is_global
          && ecma_is_value_empty (ret_value)
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
          ecma_string_t *last_index_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);
          ecma_object_t *regexp_obj_p = ecma_get_object_from_value (context_p->regexp_or_search_string);

          ECMA_TRY_CATCH (put_value,
                          ecma_op_object_put (regexp_obj_p,
                                              last_index_string_p,
                                              ecma_make_number_value (context_p->match_end + 1),
                                              true),
                          ret_value);

          ECMA_FINALIZE (put_value);
          ecma_deref_ecma_string (last_index_string_p);
        }
      }
    }

    if (ecma_is_value_empty (ret_value))
    {
      if (!context_p->is_global || ecma_is_value_null (match_value))
      {
        /* No more matches */
        ecma_string_t *appended_string_p;
        appended_string_p = ecma_builtin_string_prototype_object_replace_append_substr (result_string_p,
                                                                                        input_string_p,
                                                                                        previous_start,
                                                                                        context_p->input_length,
                                                                                        false);

        ret_value = ecma_make_string_value (appended_string_p);
      }
      else
      {
        continue_match = true;
      }
    }

    ECMA_FINALIZE (match_value);
  }

  ecma_deref_ecma_string (result_string_p);
  return ret_value;
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
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (ecma_op_is_callable (replace_value))
  {
    context_p->is_replace_callable = true;
    context_p->replace_function_p = ecma_get_object_from_value (replace_value);

    ret_value = ecma_builtin_string_prototype_object_replace_loop (context_p);
  }
  else
  {
    context_p->is_replace_callable = false;

    ECMA_TRY_CATCH (to_string_replace_val,
                    ecma_op_to_string (replace_value),
                    ret_value);

    ecma_string_t *replace_string_p = ecma_get_string_from_value (to_string_replace_val);

    ECMA_STRING_TO_UTF8_STRING (replace_string_p, replace_start_p, replace_start_size);

    context_p->replace_string_p = replace_string_p;
    context_p->replace_str_curr_p = (lit_utf8_byte_t *) replace_start_p;

    ret_value = ecma_builtin_string_prototype_object_replace_loop (context_p);

    ECMA_FINALIZE_UTF8_STRING (replace_start_p, replace_start_size);
    ECMA_FINALIZE (to_string_replace_val);
  }

  return ret_value;
} /* ecma_builtin_string_prototype_object_replace_main */

/**
 * The String.prototype object's 'replace' routine
 *
 * The replace algorithm is splitted into several helper functions.
 * This allows using ECMA_TRY_CATCH macros and avoiding early returns.
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
ecma_builtin_string_prototype_object_replace (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t search_value, /**< routine's first argument */
                                              ecma_value_t replace_value) /**< routine's second argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (check_coercible_val,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2. */
  ECMA_TRY_CATCH (to_string_value,
                  ecma_op_to_string (this_arg),
                  ret_value);

  ecma_builtin_replace_search_ctx_t context;

  if (ecma_is_value_object (search_value)
      && ecma_object_get_class_name (ecma_get_object_from_value (search_value)) == LIT_MAGIC_STRING_REGEXP_UL)
  {
    ecma_object_t *regexp_obj_p = ecma_get_object_from_value (search_value);
    ecma_string_t *global_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_GLOBAL);

    ECMA_TRY_CATCH (global_value,
                    ecma_op_object_get (regexp_obj_p, global_string_p),
                    ret_value);

    JERRY_ASSERT (ecma_is_value_boolean (global_value));

    context.is_regexp = true;
    context.is_global = ecma_is_value_true (global_value);
    context.input_string = to_string_value;
    context.input_length = ecma_string_get_length (ecma_get_string_from_value (to_string_value));
    context.regexp_or_search_string = search_value;

    if (context.is_global)
    {
      ecma_string_t *last_index_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);

      ECMA_TRY_CATCH (put_value,
                      ecma_op_object_put (regexp_obj_p,
                                          last_index_string_p,
                                          ecma_make_integer_value (0),
                                          true),
                      ret_value);

      ECMA_FINALIZE (put_value);
      ecma_deref_ecma_string (last_index_string_p);
    }

    if (ecma_is_value_empty (ret_value))
    {
      ret_value = ecma_builtin_string_prototype_object_replace_main (&context, replace_value);
    }

    ECMA_FINALIZE (global_value);
    ecma_deref_ecma_string (global_string_p);
  }
  else
  {
    ECMA_TRY_CATCH (to_string_search_val,
                    ecma_op_to_string (search_value),
                    ret_value);

    context.is_regexp = false;
    context.is_global = false;
    context.input_string = to_string_value;
    context.input_length = ecma_string_get_length (ecma_get_string_from_value (to_string_value));
    context.regexp_or_search_string = to_string_search_val;

    ret_value = ecma_builtin_string_prototype_object_replace_main (&context, replace_value);

    ECMA_FINALIZE (to_string_search_val);
  }

  ECMA_FINALIZE (to_string_value);
  ECMA_FINALIZE (check_coercible_val);

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
ecma_builtin_string_prototype_object_search (ecma_value_t this_arg, /**< this argument */
                                             ecma_value_t regexp_arg) /**< routine's argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (check_coercible_value,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2. */
  ECMA_TRY_CATCH (to_string_value,
                  ecma_op_to_string (this_arg),
                  ret_value);

  ecma_value_t regexp_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 3. */
  if (ecma_is_value_object (regexp_arg)
      && ecma_object_get_class_name (ecma_get_object_from_value (regexp_arg)) == LIT_MAGIC_STRING_REGEXP_UL)
  {
    regexp_value = ecma_copy_value (regexp_arg);
  }
  else
  {
    /* 4. */
    ecma_value_t regexp_arguments[1] = { regexp_arg };

    ECMA_TRY_CATCH (new_regexp_value,
                    ecma_builtin_regexp_dispatch_construct (regexp_arguments, 1),
                    ret_value);

    regexp_value = ecma_copy_value (new_regexp_value);

    ECMA_FINALIZE (new_regexp_value);
  }

  /* 5. */
  if (ecma_is_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (match_result,
                    ecma_regexp_exec_helper (regexp_value, to_string_value, true),
                    ret_value);

    ecma_number_t offset = -1;

    if (!ecma_is_value_null (match_result))
    {
      JERRY_ASSERT (ecma_is_value_object (match_result));

      ecma_object_t *match_object_p = ecma_get_object_from_value (match_result);
      ecma_string_t *index_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_INDEX);

      ECMA_TRY_CATCH (index_value,
                      ecma_op_object_get (match_object_p, index_string_p),
                      ret_value);

      JERRY_ASSERT (ecma_is_value_number (index_value));

      offset = ecma_get_number_from_value (index_value);

      ECMA_FINALIZE (index_value);
      ecma_deref_ecma_string (index_string_p);
    }

    if (ecma_is_value_empty (ret_value))
    {
      ret_value = ecma_make_number_value (offset);
    }

    ECMA_FINALIZE (match_result);
    ecma_free_value (regexp_value);
  }

  ECMA_FINALIZE (to_string_value);
  ECMA_FINALIZE (check_coercible_value);

  /* 6. */
  return ret_value;
} /* ecma_builtin_string_prototype_object_search */

#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */

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
ecma_builtin_string_prototype_object_slice (ecma_value_t this_arg, /**< this argument */
                                            ecma_value_t arg1, /**< routine's first argument */
                                            ecma_value_t arg2) /**< routine's second argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (check_coercible_val,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2. */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_to_string (this_arg),
                  ret_value);

  /* 3. */
  ecma_string_t *get_string_val = ecma_get_string_from_value (to_string_val);

  const ecma_length_t len = ecma_string_get_length (get_string_val);

  /* 4. */
  ecma_length_t start = 0, end = len;

  ECMA_OP_TO_NUMBER_TRY_CATCH (start_num,
                               arg1,
                               ret_value);

  start = ecma_builtin_helper_array_index_normalize (start_num, len);

  /* 5. 7. */
  if (ecma_is_value_undefined (arg2))
  {
    end = len;
  }
  else
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (end_num,
                                 arg2,
                                 ret_value);

    end = ecma_builtin_helper_array_index_normalize (end_num, len);

    ECMA_OP_TO_NUMBER_FINALIZE (end_num);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (start_num);

  JERRY_ASSERT (start <= len && end <= len);

  if (ecma_is_value_empty (ret_value))
  {
    /* 8-9. */
    ecma_string_t *new_str_p = ecma_string_substr (get_string_val, start, end);
    ret_value = ecma_make_string_value (new_str_p);
  }

  ECMA_FINALIZE (to_string_val);
  ECMA_FINALIZE (check_coercible_val);

  return ret_value;
} /* ecma_builtin_string_prototype_object_slice */

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN

/**
 * The abstract SplitMatch routine for String.prototype.split()
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.14
 *
 * Used by:
 *        - The String.prototype.split routine.
 *
 * @return ecma value - contains the value of the match
 *                          - the index property of the completion value indicates the position of the
 *                            first character in the input_string that matched
 *
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_helper_split_match (ecma_value_t input_string, /**< first argument */
                                 ecma_length_t start_idx, /**< second argument */
                                 ecma_value_t separator) /**< third argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  if (ecma_is_value_object (separator)
      && ecma_object_get_class_name (ecma_get_object_from_value (separator)) == LIT_MAGIC_STRING_REGEXP_UL)
  {
    ecma_value_t regexp_value = ecma_copy_value_if_not_object (separator);

    ECMA_TRY_CATCH (to_string_val,
                    ecma_op_to_string (input_string),
                    ret_value);

    ecma_string_t *input_str_p = ecma_get_string_from_value (to_string_val);
    ecma_string_t *substr_str_p = ecma_string_substr (input_str_p, start_idx, ecma_string_get_length (input_str_p));

    ret_value = ecma_regexp_exec_helper (regexp_value, ecma_make_string_value (substr_str_p), true);

    if (!ecma_is_value_null (ret_value))
    {
      ecma_object_t *obj_p = ecma_get_object_from_value (ret_value);
      ecma_string_t *magic_index_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INDEX);
      ecma_property_value_t *index_prop_value_p = ecma_get_named_data_property (obj_p, magic_index_str_p);

      ecma_number_t index_num = ecma_get_number_from_value (index_prop_value_p->value);
      ecma_value_assign_number (&index_prop_value_p->value, index_num + start_idx);

      ecma_deref_ecma_string (magic_index_str_p);
    }

    ecma_deref_ecma_string (substr_str_p);
    ECMA_FINALIZE (to_string_val);
  }
  else
  {
    /* 2. */
    JERRY_ASSERT (ecma_is_value_string (input_string) && ecma_is_value_string (separator));

    ecma_string_t *string_str_p = ecma_get_string_from_value (input_string);
    ecma_string_t *separator_str_p = ecma_get_string_from_value (separator);

    /* 3. */
    ecma_length_t string_length = ecma_string_get_length (string_str_p);
    ecma_length_t separator_length = ecma_string_get_length (separator_str_p);

    /* 4. */
    if (start_idx + separator_length > string_length)
    {
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
    }
    else
    {
      bool is_different = false;

      /* 5. */
      for (ecma_length_t i = 0; i < separator_length && !is_different; i++)
      {
        ecma_char_t char_from_string = ecma_string_get_char_at_pos (string_str_p, start_idx + i);
        ecma_char_t char_from_separator = ecma_string_get_char_at_pos (separator_str_p, i);

        if (char_from_string != char_from_separator)
        {
          is_different = true;
        }
      }

      if (!is_different)
      {
        /* 6-7. */
        ecma_value_t match_array = ecma_op_create_array_object (0, 0, false);
        ecma_object_t *match_array_p = ecma_get_object_from_value (match_array);
        ecma_string_t *zero_str_p = ecma_new_ecma_string_from_number (ECMA_NUMBER_ZERO);

        ecma_value_t put_comp = ecma_builtin_helper_def_prop (match_array_p,
                                                              zero_str_p,
                                                              ecma_make_string_value (separator_str_p),
                                                              true,
                                                              true,
                                                              true,
                                                              true);
        JERRY_ASSERT (ecma_is_value_true (put_comp));

        ecma_string_t *magic_index_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INDEX);
        ecma_property_value_t *index_prop_value_p;
        index_prop_value_p = ecma_create_named_data_property (match_array_p,
                                                              magic_index_str_p,
                                                              ECMA_PROPERTY_FLAG_WRITABLE,
                                                              NULL);
        ecma_deref_ecma_string (magic_index_str_p);

        ecma_named_data_property_assign_value (match_array_p,
                                               index_prop_value_p,
                                               ecma_make_uint32_value (start_idx));

        ret_value = match_array;

        ecma_deref_ecma_string (zero_str_p);
      }
      else
      {
        ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
      }
    }
  }

  return ret_value;
} /* ecma_builtin_helper_split_match */

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
ecma_builtin_string_prototype_object_split (ecma_value_t this_arg, /**< this argument */
                                            ecma_value_t arg1, /**< separator */
                                            ecma_value_t arg2) /**< limit */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_check_coercible_val,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2. */
  ECMA_TRY_CATCH (this_to_string_val,
                  ecma_op_to_string (this_arg),
                  ret_value);

  /* 3. */
  ecma_value_t new_array = ecma_op_create_array_object (0, 0, false);

  /* 5. */
  ecma_length_t limit = 0;

  if (ecma_is_value_undefined (arg2))
  {
    limit = (uint32_t) -1;
  }
  else
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (limit_num, arg2, ret_value);

    limit = ecma_number_to_uint32 (limit_num);

    ECMA_OP_TO_NUMBER_FINALIZE (limit_num);
  }

  if (ecma_is_value_empty (ret_value))
  {
    /* This variable indicates that we should return with the current array, to avoid another operation. */
    bool should_return = false;

    /* 9. */
    if (limit == 0)
    {
      should_return = true;
    }
    else /* if (limit != 0) */
    {
      ecma_object_t *new_array_p = ecma_get_object_from_value (new_array);

      /* 10. */
      if (ecma_is_value_undefined (arg1))
      {
        ecma_string_t *zero_str_p = ecma_new_ecma_string_from_number (ECMA_NUMBER_ZERO);

        ecma_value_t put_comp = ecma_builtin_helper_def_prop (new_array_p,
                                                              zero_str_p,
                                                              this_to_string_val,
                                                              true,
                                                              true,
                                                              true,
                                                              false);

        JERRY_ASSERT (ecma_is_value_true (put_comp));

        should_return = true;

        ecma_deref_ecma_string (zero_str_p);
      }
      else /* if (!ecma_is_value_undefined (arg1)) */
      {
        /* 8. */
        ecma_value_t separator = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

        if (ecma_is_value_object (arg1)
            && ecma_object_get_class_name (ecma_get_object_from_value (arg1)) == LIT_MAGIC_STRING_REGEXP_UL)
        {
          separator = ecma_copy_value (arg1);
        }
        else
        {
          ECMA_TRY_CATCH (separator_to_string_val,
                          ecma_op_to_string (arg1),
                          ret_value);

          separator = ecma_copy_value (separator_to_string_val);

          ECMA_FINALIZE (separator_to_string_val);
        }

        const ecma_string_t *this_to_string_p = ecma_get_string_from_value (this_to_string_val);

        /* 11. */
        if (ecma_string_is_empty (this_to_string_p)
            && ecma_is_value_empty (ret_value))
        {
          /* 11.a */
          ecma_value_t match_result = ecma_builtin_helper_split_match (this_to_string_val,
                                                                       0,
                                                                       separator);

          /* 11.b */
          if (!ecma_is_value_null (match_result))
          {
            should_return = true;
          }
          else
          {
            /* 11.c */
            ecma_string_t *zero_str_p = ecma_new_ecma_string_from_number (ECMA_NUMBER_ZERO);

            ecma_value_t put_comp = ecma_builtin_helper_def_prop (new_array_p,
                                                                  zero_str_p,
                                                                  this_to_string_val,
                                                                  true,
                                                                  true,
                                                                  true,
                                                                  false);

            JERRY_ASSERT (ecma_is_value_true (put_comp));

            /* 11.d */
            should_return = true;

            ecma_deref_ecma_string (zero_str_p);
          }

          ecma_free_value (match_result);
        }
        else /* if (string_length != 0) || !ecma_is_value_empty (ret_value) */
        {
          /* 4. */
          ecma_length_t new_array_length = 0;

          /* 7. */
          ecma_length_t start_pos = 0;

          /* 12. */
          ecma_length_t curr_pos = start_pos;

          bool separator_is_empty = false;

          /* 6. */
          const ecma_length_t string_length = ecma_string_get_length (this_to_string_p);

          /* 13. */
          while (curr_pos < string_length && !should_return && ecma_is_value_empty (ret_value))
          {
            ecma_value_t match_result = ecma_builtin_helper_split_match (this_to_string_val,
                                                                         curr_pos,
                                                                         separator);

            /* 13.b */
            if (ecma_is_value_null (match_result))
            {
              curr_pos++;
            }
            else /* if (!ecma_is_value_null (match_result)) */
            {
              ecma_object_t *match_array_obj_p = ecma_get_object_from_value (match_result);

              ecma_string_t *zero_str_p = ecma_new_ecma_string_from_number (ECMA_NUMBER_ZERO);
              ecma_value_t match_comp_value = ecma_op_object_get (match_array_obj_p, zero_str_p);

              JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (match_comp_value));

              ecma_string_t *match_str_p = ecma_get_string_from_value (match_comp_value);
              ecma_length_t match_str_length = ecma_string_get_length (match_str_p);

              ecma_string_t *magic_empty_str_p = ecma_new_ecma_string_from_magic_string_id (LIT_MAGIC_STRING__EMPTY);
              separator_is_empty = ecma_compare_ecma_strings (magic_empty_str_p, match_str_p);

              ecma_deref_ecma_string (magic_empty_str_p);
              ecma_free_value (match_comp_value);
              ecma_deref_ecma_string (zero_str_p);

              ecma_string_t *magic_index_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INDEX);
              ecma_property_value_t *index_prop_value_p = ecma_get_named_data_property (match_array_obj_p,
                                                                                        magic_index_str_p);

              ecma_number_t index_num = ecma_get_number_from_value (index_prop_value_p->value);
              JERRY_ASSERT (index_num >= 0);

              uint32_t end_pos = ecma_number_to_uint32 (index_num);

              if (separator_is_empty)
              {
                end_pos = curr_pos + 1;
              }

              /* 13.c.iii.1-2 */
              ecma_string_t *substr_str_p = ecma_string_substr (ecma_get_string_from_value (this_to_string_val),
                                                                start_pos,
                                                                end_pos);

              ecma_string_t *array_length_str_p = ecma_new_ecma_string_from_uint32 (new_array_length);

              ecma_value_t put_comp = ecma_builtin_helper_def_prop (new_array_p,
                                                                    array_length_str_p,
                                                                    ecma_make_string_value (substr_str_p),
                                                                    true,
                                                                    true,
                                                                    true,
                                                                    false);

              JERRY_ASSERT (ecma_is_value_true (put_comp));

              /* 13.c.iii.3 */
              new_array_length++;

              /* 13.c.iii.4 */
              if (new_array_length == limit && ecma_is_value_empty (ret_value))
              {
                should_return = true;
              }

              /* 13.c.iii.5 */
              start_pos = end_pos + match_str_length;

              ecma_string_t *magic_length_str_p = ecma_new_ecma_length_string ();

              ECMA_TRY_CATCH (array_length_val,
                              ecma_op_object_get (match_array_obj_p, magic_length_str_p),
                              ret_value);

              ECMA_OP_TO_NUMBER_TRY_CATCH (array_length_num, array_length_val, ret_value);

              /* The first item is the match object, thus we should skip it. */
              const uint32_t match_result_array_length = ecma_number_to_uint32 (array_length_num) - 1;

              /* 13.c.iii.6 */
              uint32_t i = 0;

              /* 13.c.iii.7 */
              while (i < match_result_array_length && ecma_is_value_empty (ret_value))
              {
                /* 13.c.iii.7.a */
                i++;
                ecma_string_t *idx_str_p = ecma_new_ecma_string_from_uint32 (i);
                ecma_string_t *new_array_idx_str_p = ecma_new_ecma_string_from_uint32 (new_array_length);

                ecma_value_t match_comp_value = ecma_op_object_get (match_array_obj_p, idx_str_p);

                JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (match_comp_value));

                /* 13.c.iii.7.b */
                ecma_value_t put_comp = ecma_builtin_helper_def_prop (new_array_p,
                                                                      new_array_idx_str_p,
                                                                      match_comp_value,
                                                                      true,
                                                                      true,
                                                                      true,
                                                                      false);

                JERRY_ASSERT (ecma_is_value_true (put_comp));

                /* 13.c.iii.7.c */
                new_array_length++;

                /* 13.c.iii.7.d */
                if (new_array_length == limit && ecma_is_value_empty (ret_value))
                {
                  should_return = true;
                }

                ecma_free_value (match_comp_value);
                ecma_deref_ecma_string (new_array_idx_str_p);
                ecma_deref_ecma_string (idx_str_p);
              }

              /* 13.c.iii.8 */
              curr_pos = start_pos;

              ECMA_OP_TO_NUMBER_FINALIZE (array_length_num);
              ECMA_FINALIZE (array_length_val);
              ecma_deref_ecma_string (magic_length_str_p);
              ecma_deref_ecma_string (array_length_str_p);
              ecma_deref_ecma_string (substr_str_p);
              ecma_deref_ecma_string (magic_index_str_p);
            } /* if (!ecma_is_value_null (match_result)) */

            ecma_free_value (match_result);

          } /* while (curr_pos < string_length && !should_return && ecma_is_value_empty (ret_value)) */

          if (!should_return && !separator_is_empty && ecma_is_value_empty (ret_value))
          {
            /* 14. */
            ecma_string_t *substr_str_p;
            substr_str_p = ecma_string_substr (ecma_get_string_from_value (this_to_string_val),
                                               start_pos,
                                               string_length);

            /* 15. */
            ecma_string_t *array_length_string_p = ecma_new_ecma_string_from_uint32 (new_array_length);

            ecma_value_t put_comp = ecma_builtin_helper_def_prop (new_array_p,
                                                                  array_length_string_p,
                                                                  ecma_make_string_value (substr_str_p),
                                                                  true,
                                                                  true,
                                                                  true,
                                                                  false);

            JERRY_ASSERT (ecma_is_value_true (put_comp));

            ecma_deref_ecma_string (array_length_string_p);
            ecma_deref_ecma_string (substr_str_p);
          }
        } /* if (string_length != 0) || !ecma_is_value_empty (ret_value) */

        ecma_free_value (separator);
      } /* if (!ecma_is_value_undefined (arg1)) */
    } /* if (limit != 0) */
  } /* if (ecma_is_value_empty (ret_value)) */

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = new_array;
  }
  else
  {
    ecma_free_value (new_array);
  }

  ECMA_FINALIZE (this_to_string_val);
  ECMA_FINALIZE (this_check_coercible_val);

  return ret_value;
} /* ecma_builtin_string_prototype_object_split */

#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */

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
ecma_builtin_string_prototype_object_substring (ecma_value_t this_arg, /**< this argument */
                                                ecma_value_t arg1, /**< routine's first argument */
                                                ecma_value_t arg2) /**< routine's second argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1 */
  ECMA_TRY_CATCH (check_coercible_val,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2 */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_to_string (this_arg),
                  ret_value);

  /* 3 */
  ecma_string_t *original_string_p = ecma_get_string_from_value (to_string_val);

  const ecma_length_t len = ecma_string_get_length (original_string_p);

  /* 4, 6 */
  ECMA_OP_TO_NUMBER_TRY_CATCH (start_num,
                               arg1,
                               ret_value);

  ecma_length_t start = 0, end = len;

  start = ecma_builtin_helper_string_index_normalize (start_num, len, true);

  /* 5, 7 */
  if (ecma_is_value_undefined (arg2))
  {
    end = len;
  }
  else
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (end_num,
                                 arg2,
                                 ret_value);

    end = ecma_builtin_helper_string_index_normalize (end_num, len, true);

    ECMA_OP_TO_NUMBER_FINALIZE (end_num);
  }

  if (ecma_is_value_empty (ret_value))
  {
    JERRY_ASSERT (start <= len && end <= len);

    /* 8 */
    uint32_t from = start < end ? start : end;

    /* 9 */
    uint32_t to = start > end ? start : end;

    /* 10 */
    ecma_string_t *new_str_p = ecma_string_substr (original_string_p, from, to);
    ret_value = ecma_make_string_value (new_str_p);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (start_num);

  ECMA_FINALIZE (to_string_val);
  ECMA_FINALIZE (check_coercible_val);

  return ret_value;
} /* ecma_builtin_string_prototype_object_substring */

/**
 * Helper function to convert a string to upper or lower case.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_conversion_helper (ecma_value_t this_arg, /**< this argument */
                                                        bool lower_case) /**< convert to lower (true)
                                                                          *   or upper (false) case */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (check_coercible_val,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2. */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_to_string (this_arg),
                  ret_value);

  /* 3. */
  ecma_string_t *input_string_p = ecma_get_string_from_value (to_string_val);

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

  ECMA_FINALIZE (to_string_val);
  ECMA_FINALIZE (check_coercible_val);

  return ret_value;
} /* ecma_builtin_string_prototype_object_conversion_helper */

/**
 * The String.prototype object's 'toLowerCase' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.16
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_to_lower_case (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_string_prototype_object_conversion_helper (this_arg, true);
} /* ecma_builtin_string_prototype_object_to_lower_case */

/**
 * The String.prototype object's 'toLocaleLowerCase' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.17
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_to_locale_lower_case (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_string_prototype_object_conversion_helper (this_arg, true);
} /* ecma_builtin_string_prototype_object_to_locale_lower_case */

/**
 * The String.prototype object's 'toUpperCase' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.18
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_to_upper_case (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_string_prototype_object_conversion_helper (this_arg, false);
} /* ecma_builtin_string_prototype_object_to_upper_case */

/**
 * The String.prototype object's 'toLocaleUpperCase' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.19
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_to_locale_upper_case (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_string_prototype_object_conversion_helper (this_arg, false);
} /* ecma_builtin_string_prototype_object_to_locale_upper_case */

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
ecma_builtin_string_prototype_object_trim (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1 */
  ECMA_TRY_CATCH (check_coercible_val,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2 */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_to_string (this_arg),
                  ret_value);

  ecma_string_t *original_string_p = ecma_get_string_from_value (to_string_val);

  ecma_string_t *trimmed_string_p = ecma_string_trim (original_string_p);
  ret_value = ecma_make_string_value (trimmed_string_p);

  ECMA_FINALIZE (to_string_val);
  ECMA_FINALIZE (check_coercible_val);

  return ret_value;
} /* ecma_builtin_string_prototype_object_trim */

#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN

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
ecma_builtin_string_prototype_object_substr (ecma_value_t this_arg, /**< this argument */
                                             ecma_value_t start, /**< routine's first argument */
                                             ecma_value_t length) /**< routine's second argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (check_coercible_val,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 1. */
  ECMA_TRY_CATCH (to_string_val, ecma_op_to_string (this_arg), ret_value);
  ecma_string_t *this_string_p = ecma_get_string_from_value (to_string_val);

  /* 2. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (start_num, start, ret_value);
  if (ecma_number_is_nan (start_num))
  {
    start_num = 0;
  }

  /* 3. */
  ecma_number_t length_num = ecma_number_make_infinity (false);
  if (!ecma_is_value_undefined (length))
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (len, length, ret_value);
    length_num = ecma_number_is_nan (len) ? 0 : len;

    ECMA_OP_TO_NUMBER_FINALIZE (len);
  }

  if (ecma_is_value_empty (ret_value))
  {
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
    ret_value = ecma_make_string_value (new_str_p);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (start_num);
  ECMA_FINALIZE (to_string_val);
  ECMA_FINALIZE (check_coercible_val);

  return ret_value;
} /* ecma_builtin_string_prototype_object_substr */

#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_STRING_BUILTIN */

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
#include "ecma-objects.h"
#include "ecma-string-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"
#include "jrt-libc-includes.h"
#include "lit-char-helpers.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN

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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_to_string (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_value_string (this_arg))
  {
    return ecma_make_normal_completion_value (ecma_copy_value (this_arg, true));
  }
  else if (ecma_is_value_object (this_arg))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);

    if (ecma_object_get_class_name (obj_p) == LIT_MAGIC_STRING_STRING_UL)
    {
      ecma_property_t *prim_value_prop_p = ecma_get_internal_property (obj_p,
                                                                       ECMA_INTERNAL_PROPERTY_PRIMITIVE_STRING_VALUE);

      ecma_string_t *prim_value_str_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                   prim_value_prop_p->u.internal_property.value);

      prim_value_str_p = ecma_copy_or_ref_ecma_string (prim_value_str_p);

      return ecma_make_normal_completion_value (ecma_make_string_value (prim_value_str_p));
    }
  }

  return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
} /* ecma_builtin_string_prototype_object_to_string */

/**
 * The String.prototype object's 'valueOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_char_at (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t arg) /**< routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

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
    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (
                                                   ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY)));
  }
  else
  {
    /* 6 */
    ecma_char_t new_ecma_char = ecma_string_get_char_at_pos (original_string_p, ecma_number_to_uint32 (index_num));
    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (
                                                   ecma_new_ecma_string_from_code_unit (new_ecma_char)));
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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_char_code_at (ecma_value_t this_arg, /**< this argument */
                                                   ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_string_prototype_object_char_code_at */

/**
 * The String.prototype object's 'concat' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_concat (ecma_value_t this_arg, /**< this argument */
                                             const ecma_value_t* argument_list_p, /**< arguments list */
                                             ecma_length_t arguments_number) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1 */
  ECMA_TRY_CATCH (check_coercible_val,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2 */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_to_string (this_arg),
                  ret_value);

  /* 3 */
  // No copy performed

  /* 4 */
  ecma_string_t *string_to_return = ecma_copy_or_ref_ecma_string (ecma_get_string_from_value (to_string_val));

  /* 5 */
  for (uint32_t arg_index = 0;
       arg_index < arguments_number && ecma_is_completion_value_empty (ret_value);
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
  if (ecma_is_completion_value_empty (ret_value))
  {
    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (string_to_return));
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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_index_of (ecma_value_t this_arg, /**< this argument */
                                               ecma_value_t arg1, /**< routine's first argument */
                                               ecma_value_t arg2) /**< routine's second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg1, arg2);
} /* ecma_builtin_string_prototype_object_index_of */

/**
 * The String.prototype object's 'lastIndexOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.8
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_last_index_of (ecma_value_t this_arg, /**< this argument */
                                                    ecma_value_t arg1, /**< routine's first argument */
                                                    ecma_value_t arg2) /**< routine's second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg1, arg2);
} /* ecma_builtin_string_prototype_object_last_index_of */

/**
 * The String.prototype object's 'localeCompare' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.9
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_locale_compare (ecma_value_t this_arg, /**< this argument */
                                                     ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_string_prototype_object_locale_compare */

/**
 * The String.prototype object's 'match' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.10
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_match (ecma_value_t this_arg, /**< this argument */
                                            ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_string_prototype_object_match */

/**
 * The String.prototype object's 'replace' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.11
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_replace (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t arg1, /**< routine's first argument */
                                              ecma_value_t arg2) /**< routine's second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg1, arg2);
} /* ecma_builtin_string_prototype_object_replace */

/**
 * The String.prototype object's 'search' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.12
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_search (ecma_value_t this_arg, /**< this argument */
                                             ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_string_prototype_object_search */

/**
 * The String.prototype object's 'slice' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.13
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_slice (ecma_value_t this_arg, /**< this argument */
                                            ecma_value_t arg1, /**< routine's first argument */
                                            ecma_value_t arg2) /**< routine's second argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

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

  if (ecma_is_completion_value_empty (ret_value))
  {
    /* 8-9. */
    ecma_string_t *new_str_p = ecma_string_substr (get_string_val, start, end);
    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (new_str_p));
  }

  ECMA_FINALIZE (to_string_val);
  ECMA_FINALIZE (check_coercible_val);

  return ret_value;
} /* ecma_builtin_string_prototype_object_slice */

/**
 * The String.prototype object's 'split' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.14
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_split (ecma_value_t this_arg, /**< this argument */
                                            ecma_value_t arg1, /**< routine's first argument */
                                            ecma_value_t arg2) /**< routine's second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg1, arg2);
} /* ecma_builtin_string_prototype_object_split */

/**
 * The String.prototype object's 'substring' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.15
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_substring (ecma_value_t this_arg, /**< this argument */
                                                ecma_value_t arg1, /**< routine's first argument */
                                                ecma_value_t arg2) /**< routine's second argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

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

  start = ecma_builtin_helper_string_index_normalize (start_num, len);

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

    end = ecma_builtin_helper_string_index_normalize (end_num, len);

    ECMA_OP_TO_NUMBER_FINALIZE (end_num);
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    JERRY_ASSERT (start <= len && end <= len);

    /* 8 */
    uint32_t from = start < end ? start : end;

    /* 9 */
    uint32_t to = start > end ? start : end;

    /* 10 */
    ecma_string_t *new_str_p = ecma_string_substr (original_string_p, from, to);
    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (new_str_p));
  }

  ECMA_OP_TO_NUMBER_FINALIZE (start_num);

  ECMA_FINALIZE (to_string_val);
  ECMA_FINALIZE (check_coercible_val);

  return ret_value;
} /* ecma_builtin_string_prototype_object_substring */

/**
 * Helper function to convert a string to upper or lower case.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_conversion_helper (ecma_value_t this_arg, /**< this argument */
                                                        bool lower_case) /**< convert to lower (true)
                                                                          *   or upper (false) case */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

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
  lit_utf8_size_t input_size = ecma_string_get_size (input_string_p);

  MEM_DEFINE_LOCAL_ARRAY (input_start_p,
                          input_size,
                          lit_utf8_byte_t);

  ecma_string_to_utf8_string (input_string_p,
                              input_start_p,
                              (ssize_t) (input_size));

  /*
   * The URI encoding has two major phases: first we compute
   * the length of the lower case string, then we encode it.
   */

  lit_utf8_size_t output_length = 0;
  lit_utf8_iterator_t input_iterator = lit_utf8_iterator_create (input_start_p, input_size);

  while (!lit_utf8_iterator_is_eos (&input_iterator))
  {
    ecma_char_t character = lit_utf8_iterator_read_next (&input_iterator);
    ecma_char_t character_buffer[LIT_MAXIMUM_OTHER_CASE_LENGTH];
    lit_utf8_byte_t utf8_byte_buffer[LIT_UTF8_MAX_BYTES_IN_CODE_POINT];
    lit_utf8_size_t character_length;

    /*
     * We need to keep surrogate pairs. Surrogates are never converted,
     * regardless they form a valid pair or not.
     */
    if (lit_is_code_unit_high_surrogate (character))
    {
      ecma_char_t next_character = lit_utf8_iterator_peek_next (&input_iterator);

      if (lit_is_code_unit_low_surrogate (next_character))
      {
        lit_code_point_t surrogate_code_point = lit_convert_surrogate_pair_to_code_point (character, next_character);
        output_length += lit_code_point_to_utf8 (surrogate_code_point, utf8_byte_buffer);
        lit_utf8_iterator_incr (&input_iterator);
        continue;
      }
    }

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

    for (lit_utf8_size_t i = 0; i < character_length; i++)
    {
      output_length += lit_code_unit_to_utf8 (character_buffer[i], utf8_byte_buffer);
    }
  }

  /* Second phase. */

  MEM_DEFINE_LOCAL_ARRAY (output_start_p,
                          output_length,
                          lit_utf8_byte_t);

  lit_utf8_byte_t *output_char_p = output_start_p;

  /* Encoding the output. */
  lit_utf8_iterator_seek_bos (&input_iterator);

  while (!lit_utf8_iterator_is_eos (&input_iterator))
  {
    ecma_char_t character = lit_utf8_iterator_read_next (&input_iterator);
    ecma_char_t character_buffer[LIT_MAXIMUM_OTHER_CASE_LENGTH];
    lit_utf8_size_t character_length;

    /*
     * We need to keep surrogate pairs. Surrogates are never converted,
     * regardless they form a valid pair or not.
     */
    if (lit_is_code_unit_high_surrogate (character))
    {
      ecma_char_t next_character = lit_utf8_iterator_peek_next (&input_iterator);

      if (lit_is_code_unit_low_surrogate (next_character))
      {
        lit_code_point_t surrogate_code_point = lit_convert_surrogate_pair_to_code_point (character, next_character);
        output_char_p += lit_code_point_to_utf8 (surrogate_code_point, output_char_p);
        lit_utf8_iterator_incr (&input_iterator);
        continue;
      }
    }

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

    for (lit_utf8_size_t i = 0; i < character_length; i++)
    {
      output_char_p += lit_code_point_to_utf8 (character_buffer[i], output_char_p);
    }
  }

  JERRY_ASSERT (output_start_p + output_length == output_char_p);

  ecma_string_t *output_string_p = ecma_new_ecma_string_from_utf8 (output_start_p, output_length);

  ret_value = ecma_make_normal_completion_value (ecma_make_string_value (output_string_p));

  MEM_FINALIZE_LOCAL_ARRAY (output_start_p);
  MEM_FINALIZE_LOCAL_ARRAY (input_start_p);

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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_trim (ecma_value_t this_arg) /**< this argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1 */
  ECMA_TRY_CATCH (check_coercible_val,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2 */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_to_string (this_arg),
                  ret_value);

  ecma_string_t *original_string_p = ecma_get_string_from_value (to_string_val);

  /* 3 */
  const lit_utf8_size_t size = ecma_string_get_size (original_string_p);
  const ecma_length_t length = ecma_string_get_size (original_string_p);

  /* Workaround: avoid repeated call of ecma_string_get_char_at_pos() because its overhead */
  lit_utf8_byte_t *original_utf8_str_p = (lit_utf8_byte_t *) mem_heap_alloc_block (size + 1,
                                                                                   MEM_HEAP_ALLOC_SHORT_TERM);
  ecma_string_to_utf8_string (original_string_p, original_utf8_str_p, (ssize_t) size);

  uint32_t prefix = 0, postfix = 0;
  uint32_t new_len = 0;

  while (prefix < length && isspace (lit_utf8_string_code_unit_at (original_utf8_str_p, size, prefix)))
  {
    prefix++;
  }

  while (postfix < length - prefix && isspace (lit_utf8_string_code_unit_at (original_utf8_str_p,
                                                                             size,
                                                                             length - postfix - 1)))
  {
    postfix++;
  }

  new_len = prefix < size ? size - prefix - postfix : 0;

  ecma_string_t *new_str_p = ecma_string_substr (original_string_p, prefix, prefix + new_len);

  /* 4 */
  ret_value = ecma_make_normal_completion_value (ecma_make_string_value (new_str_p));

  mem_heap_free_block (original_utf8_str_p);

  ECMA_FINALIZE (to_string_val);
  ECMA_FINALIZE (check_coercible_val);

  return ret_value;
} /* ecma_builtin_string_prototype_object_trim */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN */

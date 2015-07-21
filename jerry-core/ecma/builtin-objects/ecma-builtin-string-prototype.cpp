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
#include "ecma-array-object.h"
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

  ecma_number_t *ret_num_p = ecma_alloc_number ();

  /* 5 */
  // When index_num is NaN, then the first two comparisons are false
  if (index_num < 0 || index_num >= len || (ecma_number_is_nan (index_num) && !len))
  {
    *ret_num_p = ecma_number_make_nan ();
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
    *ret_num_p = ecma_uint32_to_number (new_ecma_char);
  }

  ecma_value_t new_value = ecma_make_number_value (ret_num_p);
  ret_value = ecma_make_normal_completion_value (new_value);

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
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1 */
  ECMA_TRY_CATCH (check_coercible_val,
                  ecma_op_check_object_coercible (this_arg),
                  ret_value);

  /* 2 */
  ECMA_TRY_CATCH (to_str_val,
                  ecma_op_to_string (this_arg),
                  ret_value);

  /* 3 */
  ECMA_TRY_CATCH (search_str_val,
                  ecma_op_to_string (arg1),
                  ret_value);

  /* 4 */
  ECMA_OP_TO_NUMBER_TRY_CATCH (pos_num,
                               arg2,
                               ret_value);

  /* 5 */
  ecma_string_t *original_str_p = ecma_get_string_from_value (to_str_val);
  const ecma_length_t original_len = ecma_string_get_length (original_str_p);
  const lit_utf8_size_t original_size = ecma_string_get_size (original_str_p);

  /* 4b, 6 */
  ecma_length_t start = ecma_builtin_helper_string_index_normalize (pos_num, original_len);

  /* 7 */
  ecma_string_t *search_str_p = ecma_get_string_from_value (search_str_val);
  const ecma_length_t search_len = ecma_string_get_length (search_str_p);
  const lit_utf8_size_t search_size = ecma_string_get_size (search_str_p);

  ecma_number_t *ret_num_p = ecma_alloc_number ();
  *ret_num_p = ecma_int32_to_number (-1);

  /* 8 */
  if (search_len <= original_len)
  {
    if (!search_len)
    {
      *ret_num_p = ecma_uint32_to_number (0);
    }
    else
    {
      /* create utf8 string from original string and advance to start position */
      MEM_DEFINE_LOCAL_ARRAY (original_str_utf8_p,
                              original_size,
                              lit_utf8_byte_t);

      ecma_string_to_utf8_string (original_str_p,
                                  original_str_utf8_p,
                                  (ssize_t) (original_size));

      lit_utf8_iterator_t original_it = lit_utf8_iterator_create (original_str_utf8_p, original_size);

      ecma_length_t index = start;
      lit_utf8_iterator_advance (&original_it, index);

      /* create utf8 string from search string */
      MEM_DEFINE_LOCAL_ARRAY (search_str_utf8_p,
                              search_size,
                              lit_utf8_byte_t);

      ecma_string_to_utf8_string (search_str_p,
                                  search_str_utf8_p,
                                  (ssize_t) (search_size));

      lit_utf8_iterator_t search_it = lit_utf8_iterator_create (search_str_utf8_p, search_size);

      /* iterate original string and try to match at each position */
      bool found = false;

      while (!found && index <= original_len - search_len)
      {
        ecma_length_t match_len = 0;
        lit_utf8_iterator_pos_t stored_original_pos = lit_utf8_iterator_get_pos (&original_it);

        while (match_len < search_len &&
               lit_utf8_iterator_read_next (&original_it) == lit_utf8_iterator_read_next (&search_it))
        {
          match_len++;
        }

        /* Check for match */
        if (match_len == search_len)
        {
          *ret_num_p = ecma_uint32_to_number (index);
          found = true;
        }
        else
        {
          /* reset iterators */
          lit_utf8_iterator_seek_bos (&search_it);
          lit_utf8_iterator_seek (&original_it, stored_original_pos);
          lit_utf8_iterator_incr (&original_it);
        }
        index++;
      }

      MEM_FINALIZE_LOCAL_ARRAY (search_str_utf8_p);
      MEM_FINALIZE_LOCAL_ARRAY (original_str_utf8_p);
    }
  }

  ecma_value_t new_value = ecma_make_number_value (ret_num_p);
  ret_value = ecma_make_normal_completion_value (new_value);

  ECMA_OP_TO_NUMBER_FINALIZE (pos_num);
  ECMA_FINALIZE (search_str_val);
  ECMA_FINALIZE (to_str_val);
  ECMA_FINALIZE (check_coercible_val);

  return ret_value;
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
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

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

  ecma_number_t *result_p = ecma_alloc_number ();

  if (ecma_compare_ecma_strings_relational (this_string_p, arg_string_p))
  {
    *result_p = ecma_int32_to_number (-1);
  }
  else if (!ecma_compare_ecma_strings (this_string_p, arg_string_p))
  {
    *result_p = ecma_int32_to_number (1);
  }
  else
  {
    *result_p = ecma_int32_to_number (0);
  }

  ret_value = ecma_make_normal_completion_value (ecma_make_number_value (result_p));

  ECMA_FINALIZE (arg_to_string_val);
  ECMA_FINALIZE (this_to_string_val);
  ECMA_FINALIZE (this_check_coercible_val);

  return ret_value;
} /* ecma_builtin_string_prototype_object_locale_compare */

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN

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
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

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
    regexp_value = ecma_copy_value (arg, true);
  }
  else
  {
    /* 4. */
    ecma_value_t regexp_arguments[1] = { arg };
    ECMA_TRY_CATCH (new_regexp_value,
                    ecma_builtin_regexp_dispatch_construct (regexp_arguments, 1),
                    ret_value);

    regexp_value = ecma_copy_value (new_regexp_value, true);

    ECMA_FINALIZE (new_regexp_value);
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    JERRY_ASSERT (!ecma_is_value_empty (regexp_value));
    ecma_object_t *regexp_obj_p = ecma_get_object_from_value (regexp_value);
    ecma_string_t *global_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_GLOBAL);

    /* 5. */
    ECMA_TRY_CATCH (global_value,
                    ecma_op_object_get (regexp_obj_p, global_string_p),
                    ret_value);

    JERRY_ASSERT (ecma_is_value_boolean (global_value));

    ecma_value_t exec_arguments[1] = { this_to_string_value };

    if (!ecma_is_value_true (global_value))
    {
      /* 7. */
      ret_value = ecma_builtin_regexp_prototype_dispatch_routine (LIT_MAGIC_STRING_EXEC,
                                                                  regexp_value,
                                                                  exec_arguments,
                                                                  1);
    }
    else
    {
      /* 8.a. */
      ecma_number_t *zero_number_p = ecma_alloc_number ();
      *zero_number_p = 0;

      ecma_string_t *index_zero_string_p = ecma_new_ecma_string_from_uint32 (0);

      ecma_string_t *last_index_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);

      ECMA_TRY_CATCH (put_value,
                      ecma_op_object_put (regexp_obj_p,
                                          last_index_string_p,
                                          ecma_make_number_value (zero_number_p),
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

      //ecma_completion_value_t exec_result = ecma_make_empty_completion_value ();

      /* 8.f. */
      while (last_match && ecma_is_completion_value_empty (ret_value))
      {
        /* 8.f.i. */
        ECMA_TRY_CATCH (exec_value,
                        ecma_builtin_regexp_prototype_dispatch_routine (LIT_MAGIC_STRING_EXEC,
                                                                        regexp_value,
                                                                        exec_arguments,
                                                                        1),
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

          ecma_number_t this_index = *ecma_get_number_from_value (this_index_number);

          /* 8.f.iii.2. */
          if (this_index == previous_last_index)
          {
            ecma_number_t *new_last_index_p = ecma_alloc_number ();
            *new_last_index_p = this_index + 1;
            /* 8.f.iii.2.a. */
            ECMA_TRY_CATCH (index_put_value,
                            ecma_op_object_put (regexp_obj_p,
                                                last_index_string_p,
                                                ecma_make_number_value (new_last_index_p),
                                                true),
                            ret_value);

            /* 8.f.iii.2.b. */
            previous_last_index = this_index + 1;

            ECMA_FINALIZE (index_put_value);

            ecma_dealloc_number (new_last_index_p);
          }
          else
          {
            /* 8.f.iii.3. */
            previous_last_index = this_index;
          }

          if (ecma_is_completion_value_empty (ret_value))
          {
            /* 8.f.iii.4. */
            JERRY_ASSERT (ecma_is_value_object (exec_value));
            ecma_object_t *exec_obj_p = ecma_get_object_from_value (exec_value);

            ECMA_TRY_CATCH (match_string_value,
                            ecma_op_object_get (exec_obj_p, index_zero_string_p),
                            ret_value);

            /* 8.f.iii.5. */
            ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();
            {
              prop_desc.is_value_defined = true;
              prop_desc.value = match_string_value;

              prop_desc.is_writable_defined = true;
              prop_desc.is_writable = true;

              prop_desc.is_enumerable_defined = true;
              prop_desc.is_enumerable = true;

              prop_desc.is_configurable_defined = true;
              prop_desc.is_configurable = true;
            }

            ecma_string_t *current_index_str_p = ecma_new_ecma_string_from_uint32 (n);

            ecma_completion_value_t completion = ecma_op_object_define_own_property (new_array_obj_p,
                                                                                     current_index_str_p,
                                                                                     &prop_desc,
                                                                                     false);
            JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));

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

      if (ecma_is_completion_value_empty (ret_value))
      {
        if (n == 0)
        {
          /* 8.g. */
          ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_NULL);
        }
        else
        {
          /* 8.h. */
          ret_value = ecma_make_normal_completion_value (ecma_copy_value (new_array_value, true));
        }
      }

      ECMA_FINALIZE (new_array_value);

      ECMA_FINALIZE (put_value);

      ecma_deref_ecma_string (last_index_string_p);
      ecma_deref_ecma_string (index_zero_string_p);
      ecma_dealloc_number (zero_number_p);
    }

    ECMA_FINALIZE (global_value);

    ecma_deref_ecma_string (global_string_p);

    ecma_free_value (regexp_value, true);
  }

  ECMA_FINALIZE (this_to_string_value);

  ECMA_FINALIZE (this_check_coercible_value);

  return ret_value;
} /* ecma_builtin_string_prototype_object_match */
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */

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

  while (prefix < length)
  {
    ecma_char_t next_char = lit_utf8_string_code_unit_at (original_utf8_str_p,
                                                          size,
                                                          prefix);

    if (lit_char_is_white_space (next_char)
        || lit_char_is_line_terminator (next_char))
    {
      prefix++;
    }
    else
    {
      break;
    }
  }

  while (postfix < length - prefix)
  {
    ecma_char_t next_char = lit_utf8_string_code_unit_at (original_utf8_str_p,
                                                          size,
                                                          length - postfix - 1);
    if (lit_char_is_white_space (next_char)
        || lit_char_is_line_terminator (next_char))
    {
      postfix++;
    }
    else
    {
      break;
    }
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

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

#include "ecma-builtin-helpers.h"

#include "ecma-alloc.h"
#include "ecma-array-object.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-function-object.h"
#include "ecma-exceptions.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "lit-magic-strings.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltinhelpers ECMA builtin helper operations
 * @{
 */

/**
 * Common implementation of the Object.prototype.toString routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.2
 *
 * Used by:
 *         - The Object.prototype.toString routine.
 *         - The Array.prototype.toString routine as fallback.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */

ecma_value_t
ecma_builtin_helper_object_to_string (const ecma_value_t this_arg) /**< this argument */
{
  lit_magic_string_id_t type_string;

  if (ecma_is_value_undefined (this_arg))
  {
    type_string = LIT_MAGIC_STRING_UNDEFINED_UL;
  }
  else if (ecma_is_value_null (this_arg))
  {
    type_string = LIT_MAGIC_STRING_NULL_UL;
  }
  else
  {
    ecma_value_t obj_this = ecma_op_to_object (this_arg);

    if (ECMA_IS_VALUE_ERROR (obj_this))
    {
      return obj_this;
    }

    JERRY_ASSERT (ecma_is_value_object (obj_this));

    ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);

    type_string = ecma_object_get_class_name (obj_p);

    ecma_free_value (obj_this);
  }

  ecma_string_t *ret_string_p;

  /* Building string "[object #type#]" where type is 'Undefined',
     'Null' or one of possible object's classes.
     The string with null character is maximum 27 characters long. */
  const lit_utf8_size_t buffer_size = 27;
  lit_utf8_byte_t str_buffer[buffer_size];

  lit_utf8_byte_t *buffer_ptr = str_buffer;

  const lit_magic_string_id_t magic_string_ids[] =
  {
    LIT_MAGIC_STRING_LEFT_SQUARE_CHAR,
    LIT_MAGIC_STRING_OBJECT,
    LIT_MAGIC_STRING_SPACE_CHAR,
    type_string,
    LIT_MAGIC_STRING_RIGHT_SQUARE_CHAR
  };

  for (uint32_t i = 0; i < sizeof (magic_string_ids) / sizeof (lit_magic_string_id_t); ++i)
  {
    buffer_ptr = lit_copy_magic_string_to_buffer (magic_string_ids[i], buffer_ptr,
                                                  (lit_utf8_size_t) ((str_buffer + buffer_size) - buffer_ptr));
    JERRY_ASSERT (buffer_ptr <= str_buffer + buffer_size);
  }

  ret_string_p = ecma_new_ecma_string_from_utf8 (str_buffer, (lit_utf8_size_t) (buffer_ptr - str_buffer));

  return ecma_make_string_value (ret_string_p);
} /* ecma_builtin_helper_object_to_string */

/**
 * The Array.prototype's 'toLocaleString' single element operation routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.3 steps 6-8 and 10.b-d
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_helper_get_to_locale_string_at_index (ecma_object_t *obj_p, /**< this object */
                                                   uint32_t index) /**< array index */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;
  ecma_string_t *index_string_p = ecma_new_ecma_string_from_uint32 (index);

  ECMA_TRY_CATCH (index_value,
                  ecma_op_object_get (obj_p, index_string_p),
                  ret_value);

  if (ecma_is_value_undefined (index_value) || ecma_is_value_null (index_value))
  {
    ecma_string_t *return_string_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
    ret_value = ecma_make_string_value (return_string_p);
  }
  else
  {
    ECMA_TRY_CATCH (index_obj_value,
                    ecma_op_to_object (index_value),
                    ret_value);

    ecma_object_t *index_obj_p = ecma_get_object_from_value (index_obj_value);

    ECMA_TRY_CATCH (to_locale_value,
                    ecma_op_object_get_by_magic_id (index_obj_p, LIT_MAGIC_STRING_TO_LOCALE_STRING_UL),
                    ret_value);

    if (ecma_op_is_callable (to_locale_value))
    {
      ecma_object_t *locale_func_obj_p = ecma_get_object_from_value (to_locale_value);
      ECMA_TRY_CATCH (call_value,
                      ecma_op_function_call (locale_func_obj_p,
                                             ecma_make_object_value (index_obj_p),
                                             NULL,
                                             0),
                      ret_value);
      ret_value = ecma_op_to_string (call_value);
      ECMA_FINALIZE (call_value);

    }
    else
    {
      ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("'toLocaleString' is missing or not a function."));
    }

    ECMA_FINALIZE (to_locale_value);
    ECMA_FINALIZE (index_obj_value);
  }

  ECMA_FINALIZE (index_value);

  ecma_deref_ecma_string (index_string_p);

  return ret_value;
} /* ecma_builtin_helper_get_to_locale_string_at_index */


/**
 * The Object.keys and Object.getOwnPropertyNames routine's common part.
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.4 steps 2-5
 *          ECMA-262 v5, 15.2.3.14 steps 3-6
 *
 * @return ecma value - Array of property names.
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_helper_object_get_properties (ecma_object_t *obj_p, /**< object */
                                           bool only_enumerable_properties) /**< list enumerable properties? */
{
  JERRY_ASSERT (obj_p != NULL);

  ecma_value_t new_array = ecma_op_create_array_object (NULL, 0, false);
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (new_array));
  ecma_object_t *new_array_p = ecma_get_object_from_value (new_array);

  uint32_t index = 0;

  ecma_collection_header_t *props_p = ecma_op_object_get_property_names (obj_p,
                                                                         false,
                                                                         only_enumerable_properties,
                                                                         false);

  ecma_collection_iterator_t iter;
  ecma_collection_iterator_init (&iter, props_p);

  while (ecma_collection_iterator_next (&iter))
  {
    ecma_string_t *index_string_p = ecma_new_ecma_string_from_uint32 (index);

    ecma_value_t completion = ecma_builtin_helper_def_prop (new_array_p,
                                                            index_string_p,
                                                            *iter.current_value_p,
                                                            true, /* Writable */
                                                            true, /* Enumerable */
                                                            true, /* Configurable */
                                                            false); /* Failure handling */

    JERRY_ASSERT (ecma_is_value_true (completion));

    ecma_deref_ecma_string (index_string_p);

    index++;
  }

  ecma_free_values_collection (props_p, true);

  return new_array;
} /* ecma_builtin_helper_object_get_properties */

/**
 * Helper function to normalizing an array index
 *
 * This function clamps the given index to the [0, length] range.
 * If the index is negative, it is used as the offset from the end of the array,
 * to compute normalized index.
 * If the index is greater than the length of the array, the normalized index will be the length of the array.
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.10 steps 5-6, 7 (part 2) and 8
 *          ECMA-262 v5, 15.4.4.12 steps 5-6
 *          ECMA-262 v5, 15.4.4.14 steps 5
 *          ECMA-262 v5, 15.5.4.13 steps 4, 5 (part 2) and 6-7
 *
 * Used by:
 *         - The Array.prototype.slice routine.
 *         - The Array.prototype.splice routine.
 *         - The Array.prototype.indexOf routine.
 *         - The String.prototype.slice routine.
 *
 * @return uint32_t - the normalized value of the index
 */
uint32_t
ecma_builtin_helper_array_index_normalize (ecma_number_t index, /**< index */
                                           uint32_t length) /**< array's length */
{
  uint32_t norm_index;

  if (!ecma_number_is_nan (index))
  {

    if (ecma_number_is_zero (index))
    {
      norm_index = 0;
    }
    else if (ecma_number_is_infinity (index))
    {
      norm_index = ecma_number_is_negative (index) ? 0 : length;
    }
    else
    {
      if (ecma_number_is_negative (index))
      {
        ecma_number_t index_neg = -index;

        if (index_neg > length)
        {
          norm_index = 0;
        }
        else
        {
          norm_index = length - ecma_number_to_uint32 (index_neg);
        }
      }
      else
      {
        if (index > length)
        {
          norm_index = length;
        }
        else
        {
          norm_index = ecma_number_to_uint32 (index);
        }
      }
    }
  }
  else
  {
    norm_index = 0;
  }

  return norm_index;
} /* ecma_builtin_helper_array_index_normalize */

/**
 * Helper function for concatenating an ecma_value_t to an Array.
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.4 steps 5.b - 5.c
 *
 * Used by:
 *         - The Array.prototype.concat routine.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_helper_array_concat_value (ecma_object_t *obj_p, /**< array */
                                        uint32_t *length_p, /**< [in,out] array's length */
                                        ecma_value_t value) /**< value to concat */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* 5.b */
  if (ecma_is_value_object (value)
      && (ecma_object_get_class_name (ecma_get_object_from_value (value)) == LIT_MAGIC_STRING_ARRAY_UL))
  {
    /* 5.b.ii */
    ECMA_TRY_CATCH (arg_len_value,
                    ecma_op_object_get_by_magic_id (ecma_get_object_from_value (value),
                                                    LIT_MAGIC_STRING_LENGTH),
                    ret_value);
    ECMA_OP_TO_NUMBER_TRY_CATCH (arg_len_number, arg_len_value, ret_value);

    uint32_t arg_len = ecma_number_to_uint32 (arg_len_number);

    /* 5.b.iii */
    for (uint32_t array_index = 0;
         array_index < arg_len && ecma_is_value_empty (ret_value);
         array_index++)
    {
      ecma_string_t *array_index_string_p = ecma_new_ecma_string_from_uint32 (array_index);

      /* 5.b.iii.2 */
      ECMA_TRY_CATCH (get_value,
                      ecma_op_object_find (ecma_get_object_from_value (value),
                                           array_index_string_p),
                      ret_value);

      if (ecma_is_value_found (get_value))
      {
        /* 5.b.iii.3.a */
        ecma_string_t *new_array_index_string_p = ecma_new_ecma_string_from_uint32 (*length_p + array_index);

        /* 5.b.iii.3.b */
        /* This will always be a simple value since 'is_throw' is false, so no need to free. */
        ecma_value_t put_comp = ecma_builtin_helper_def_prop (obj_p,
                                                              new_array_index_string_p,
                                                              get_value,
                                                              true, /* Writable */
                                                              true, /* Enumerable */
                                                              true, /* Configurable */
                                                              false); /* Failure handling */

        JERRY_ASSERT (ecma_is_value_true (put_comp));
        ecma_deref_ecma_string (new_array_index_string_p);
      }

      ECMA_FINALIZE (get_value);

      ecma_deref_ecma_string (array_index_string_p);
    }

    *length_p += arg_len;

    ECMA_OP_TO_NUMBER_FINALIZE (arg_len_number);
    ECMA_FINALIZE (arg_len_value);
  }
  else
  {
    ecma_string_t *new_array_index_string_p = ecma_new_ecma_string_from_uint32 ((*length_p)++);

    /* 5.c.i */
    /* This will always be a simple value since 'is_throw' is false, so no need to free. */
    ecma_value_t put_comp = ecma_builtin_helper_def_prop (obj_p,
                                                          new_array_index_string_p,
                                                          value,
                                                          true, /* Writable */
                                                          true, /* Enumerable */
                                                          true, /* Configurable */
                                                          false); /* Failure handling */

    JERRY_ASSERT (ecma_is_value_true (put_comp));

    ecma_deref_ecma_string (new_array_index_string_p);
  }

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = ECMA_VALUE_TRUE;
  }

  return ret_value;
} /* ecma_builtin_helper_array_concat_value */

/**
 * Helper function to normalizing a string index
 *
 * This function clamps the given index to the [0, length] range.
 * If the index is negative, 0 value is used.
 * If the index is greater than the length of the string, the normalized index will be the length of the string.
 * NaN is mapped to zero or length depending on the nan_to_zero parameter.
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.15
 *
 * Used by:
 *         - The String.prototype.substring routine.
 *         - The ecma_builtin_helper_string_prototype_object_index_of helper routine.
 *
 * @return uint32_t - the normalized value of the index
 */
uint32_t
ecma_builtin_helper_string_index_normalize (ecma_number_t index, /**< index */
                                            uint32_t length, /**< string's length */
                                            bool nan_to_zero) /**< whether NaN is mapped to zero (t) or length (f) */
{
  uint32_t norm_index = 0;

  if (ecma_number_is_nan (index))
  {
    if (!nan_to_zero)
    {
      norm_index = length;
    }
  }
  else if (!ecma_number_is_negative (index))
  {
    if (ecma_number_is_infinity (index))
    {
      norm_index = length;
    }
    else
    {
      norm_index = ecma_number_to_uint32 (index);

      if (norm_index > length)
      {
        norm_index = length;
      }
    }
  }

  return norm_index;
} /* ecma_builtin_helper_string_index_normalize */

/**
 * Helper function for string indexOf and lastIndexOf functions
 *
 * This function implements string indexOf and lastIndexOf with required checks and conversions.
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.7
 *          ECMA-262 v5, 15.5.4.8
 *
 * Used by:
 *         - The String.prototype.indexOf routine.
 *         - The String.prototype.lastIndexOf routine.
 *
 * @return uint32_t - (last) index of search string
 */
ecma_value_t
ecma_builtin_helper_string_prototype_object_index_of (ecma_value_t this_arg, /**< this argument */
                                                      ecma_value_t arg1, /**< routine's first argument */
                                                      ecma_value_t arg2, /**< routine's second argument */
                                                      bool first_index) /**< routine's third argument */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

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

  /* 5 (indexOf) -- 6 (lastIndexOf) */
  ecma_string_t *original_str_p = ecma_get_string_from_value (to_str_val);
  const ecma_length_t original_len = ecma_string_get_length (original_str_p);

  /* 4b, 6 (indexOf) - 4b, 5, 7 (lastIndexOf) */
  ecma_length_t start = ecma_builtin_helper_string_index_normalize (pos_num, original_len, first_index);

  /* 7 (indexOf) -- 8 (lastIndexOf) */
  ecma_string_t *search_str_p = ecma_get_string_from_value (search_str_val);

  ecma_number_t ret_num = ECMA_NUMBER_MINUS_ONE;

  /* 8 (indexOf) -- 9 (lastIndexOf) */
  ecma_length_t index_of = 0;
  if (ecma_builtin_helper_string_find_index (original_str_p, search_str_p, first_index, start, &index_of))
  {
    ret_num = ((ecma_number_t) index_of);
  }

  ret_value = ecma_make_number_value (ret_num);

  ECMA_OP_TO_NUMBER_FINALIZE (pos_num);
  ECMA_FINALIZE (search_str_val);
  ECMA_FINALIZE (to_str_val);
  ECMA_FINALIZE (check_coercible_val);

  return ret_value;
} /* ecma_builtin_helper_string_prototype_object_index_of */

/**
 * Helper function for finding index of a search string
 *
 * This function clamps the given index to the [0, length] range.
 * If the index is negative, 0 value is used.
 * If the index is greater than the length of the string, the normalized index will be the length of the string.
 * NaN is mapped to zero or length depending on the nan_to_zero parameter.
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.7,8,11
 *
 * Used by:
 *         - The ecma_builtin_helper_string_prototype_object_index_of helper routine.
 *         - The ecma_builtin_string_prototype_object_replace_match helper routine.
 *
 * @return uint32_t - the normalized value of the index
 */
bool
ecma_builtin_helper_string_find_index (ecma_string_t *original_str_p, /**< index */
                                       ecma_string_t *search_str_p, /**< string's length */
                                       bool first_index, /**< whether search for first (t) or last (f) index */
                                       ecma_length_t start_pos, /**< start position */
                                       ecma_length_t *ret_index_p) /**< position found in original string */
{
  bool match_found = false;
  const ecma_length_t original_len = ecma_string_get_length (original_str_p);
  const ecma_length_t search_len = ecma_string_get_length (search_str_p);

  if (search_len <= original_len)
  {
    if (!search_len)
    {
      match_found = true;
      *ret_index_p = first_index ? 0 : original_len;
    }
    else
    {
      /* create utf8 string from original string and advance to position */
      ECMA_STRING_TO_UTF8_STRING (original_str_p, original_str_utf8_p, original_str_size);

      ecma_length_t index = start_pos;

      const lit_utf8_byte_t *original_str_curr_p = original_str_utf8_p;
      for (ecma_length_t idx = 0; idx < index; idx++)
      {
        lit_utf8_incr (&original_str_curr_p);
      }

      /* create utf8 string from search string */
      ECMA_STRING_TO_UTF8_STRING (search_str_p, search_str_utf8_p, search_str_size);

      const lit_utf8_byte_t *search_str_curr_p = search_str_utf8_p;

      /* iterate original string and try to match at each position */
      bool searching = true;
      ecma_char_t first_char = lit_utf8_read_next (&search_str_curr_p);
      while (searching)
      {
        /* match as long as possible */
        ecma_length_t match_len = 0;
        const lit_utf8_byte_t *stored_original_str_curr_p = original_str_curr_p;

        if (match_len < search_len &&
            index + match_len < original_len &&
            lit_utf8_read_next (&original_str_curr_p) == first_char)
        {
          const lit_utf8_byte_t *nested_search_str_curr_p = search_str_curr_p;
          match_len++;

          while (match_len < search_len &&
                 index + match_len < original_len &&
                 lit_utf8_read_next (&original_str_curr_p) == lit_utf8_read_next (&nested_search_str_curr_p))
          {
            match_len++;
          }
        }

        /* check for match */
        if (match_len == search_len)
        {
          match_found = true;
          *ret_index_p = index;

          break;
        }
        else
        {
          /* inc/dec index and update iterators and search condition */
          original_str_curr_p = stored_original_str_curr_p;

          if (first_index)
          {
            if ((searching = (index <= original_len - search_len)))
            {
              lit_utf8_incr (&original_str_curr_p);
              index++;
            }
          }
          else
          {
            if ((searching = (index > 0)))
            {
              lit_utf8_decr (&original_str_curr_p);
              index--;
            }
          }
        }
      }

      ECMA_FINALIZE_UTF8_STRING (search_str_utf8_p, search_str_size);
      ECMA_FINALIZE_UTF8_STRING (original_str_utf8_p, original_str_size);
    }
  }

  return match_found;
} /* ecma_builtin_helper_string_find_index */

/**
 * Helper function for using [[DefineOwnProperty]].
 *
 * See also:
 *          ECMA-262 v5, 8.12.9
 *          ECMA-262 v5, 15.4.5.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_helper_def_prop (ecma_object_t *obj_p, /**< object */
                              ecma_string_t *index_p, /**< index string */
                              ecma_value_t value, /**< value */
                              bool writable, /**< writable */
                              bool enumerable, /**< enumerable */
                              bool configurable, /**< configurable */
                              bool is_throw) /**< is_throw */
{
  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

  prop_desc.is_value_defined = true;
  prop_desc.value = value;

  prop_desc.is_writable_defined = true;
  prop_desc.is_writable = ECMA_BOOL_TO_BITFIELD (writable);

  prop_desc.is_enumerable_defined = true;
  prop_desc.is_enumerable = ECMA_BOOL_TO_BITFIELD (enumerable);

  prop_desc.is_configurable_defined = true;
  prop_desc.is_configurable = ECMA_BOOL_TO_BITFIELD (configurable);

  return ecma_op_object_define_own_property (obj_p,
                                             index_p,
                                             &prop_desc,
                                             is_throw);
} /* ecma_builtin_helper_def_prop */

/**
 * @}
 * @}
 */

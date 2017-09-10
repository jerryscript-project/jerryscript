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
#include "ecma-helpers.h"
#include "ecma-builtin-helpers.h"
#include "lit-char-helpers.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltinhelpers ECMA builtin helper operations
 * @{
 */

/**
 * Check the object value existance in the collection.
 *
 * Used by:
 *         - ecma_builtin_json_object step 1
 *         - ecma_builtin_json_array step 1
 *
 * @return true, if the object is already in the collection.
 */
bool
ecma_has_object_value_in_collection (ecma_collection_header_t *collection_p, /**< collection */
                                     ecma_value_t object_value) /**< object value */
{
  JERRY_ASSERT (ecma_is_value_object (object_value));

  ecma_object_t *obj_p = ecma_get_object_from_value (object_value);

  ecma_collection_iterator_t iterator;
  ecma_collection_iterator_init (&iterator, collection_p);

  while (ecma_collection_iterator_next (&iterator))
  {
    ecma_value_t value = *iterator.current_value_p;
    ecma_object_t *current_p = ecma_get_object_from_value (value);

    if (current_p == obj_p)
    {
      return true;
    }
  }

  return false;
} /* ecma_has_object_value_in_collection */

/**
 * Check the string value existance in the collection.
 *
 * Used by:
 *         - ecma_builtin_json_stringify step 4.b.ii.5
 *
 * @return true, if the string is already in the collection.
 */
bool
ecma_has_string_value_in_collection (ecma_collection_header_t *collection_p, /**< collection */
                                     ecma_value_t string_value) /**< string value */
{
  JERRY_ASSERT (ecma_is_value_string (string_value));

  ecma_string_t *string_p = ecma_get_string_from_value (string_value);

  ecma_collection_iterator_t iterator;
  ecma_collection_iterator_init (&iterator, collection_p);

  while (ecma_collection_iterator_next (&iterator))
  {
    ecma_value_t value = *iterator.current_value_p;
    ecma_string_t *current_p = ecma_get_string_from_value (value);

    if (ecma_compare_ecma_strings (current_p, string_p))
    {
      return true;
    }
  }

  return false;
} /* ecma_has_string_value_in_collection*/

/**
 * Common function to concatenate key-value pairs into an ecma-string.
 *
 * See also:
 *          ECMA-262 v5, 15.12.3
 *
 * Used by:
 *         - ecma_builtin_helper_json_create_formatted_json step 10.b.ii
 *         - ecma_builtin_helper_json_create_non_formatted_json step 10.a.i
 *
 * @return pointer to ecma-string
 *         Returned value must be freed with ecma_deref_ecma_string.
 */
ecma_string_t *
ecma_builtin_helper_json_create_separated_properties (ecma_collection_header_t *partial_p, /**< key-value pairs*/
                                                      ecma_string_t *separator_p) /**< separator*/
{
  ecma_string_t *properties_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
  ecma_string_t *tmp_str_p;

  ecma_collection_iterator_t iterator;
  ecma_collection_iterator_init (&iterator, partial_p);

  uint32_t index = 0;

  while (ecma_collection_iterator_next (&iterator))
  {
    ecma_value_t name_value = *iterator.current_value_p;
    ecma_string_t *current_p = ecma_get_string_from_value (name_value);

    if (index == 0)
    {
      index++;

      tmp_str_p = ecma_concat_ecma_strings (properties_str_p, current_p);
      ecma_deref_ecma_string (properties_str_p);
      properties_str_p = tmp_str_p;
      continue;
    }

    tmp_str_p = ecma_concat_ecma_strings (properties_str_p, separator_p);
    ecma_deref_ecma_string (properties_str_p);
    properties_str_p = tmp_str_p;

    tmp_str_p = ecma_concat_ecma_strings (properties_str_p, current_p);
    ecma_deref_ecma_string (properties_str_p);
    properties_str_p = tmp_str_p;
  }

  return properties_str_p;
} /* ecma_builtin_helper_json_create_separated_properties */

/**
 * Common function to create a formatted JSON string.
 *
 * See also:
 *          ECMA-262 v5, 15.12.3
 *
 * Used by:
 *         - ecma_builtin_json_object step 10.b
 *         - ecma_builtin_json_array step 10.b
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_helper_json_create_formatted_json (ecma_string_t *left_bracket_p, /**< left bracket*/
                                                ecma_string_t *right_bracket_p, /**< right bracket*/
                                                ecma_string_t *stepback_p, /**< stepback*/
                                                ecma_collection_header_t *partial_p, /**< key-value pairs*/
                                                ecma_json_stringify_context_t *context_p) /**< context*/
{
  /* 10.b */
  ecma_string_t *comma_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_COMMA_CHAR);
  ecma_string_t *line_feed_p = ecma_get_magic_string (LIT_MAGIC_STRING_NEW_LINE_CHAR);
  ecma_string_t *properties_str_p;
  ecma_string_t *separator_p;

  /* 10.b.i */
  ecma_string_t *tmp_str_p = ecma_concat_ecma_strings (comma_str_p, line_feed_p);
  ecma_deref_ecma_string (comma_str_p);
  separator_p = tmp_str_p;

  tmp_str_p = ecma_concat_ecma_strings (separator_p, context_p->indent_str_p);
  ecma_deref_ecma_string (separator_p);
  separator_p = tmp_str_p;

  /* 10.b.ii */
  properties_str_p = ecma_builtin_helper_json_create_separated_properties (partial_p, separator_p);
  ecma_deref_ecma_string (separator_p);

  /* 10.b.iii */
  ecma_string_t *final_str_p;

  tmp_str_p = ecma_concat_ecma_strings (left_bracket_p, line_feed_p);
  final_str_p = tmp_str_p;

  tmp_str_p = ecma_concat_ecma_strings (final_str_p, context_p->indent_str_p);
  ecma_deref_ecma_string (final_str_p);
  final_str_p = tmp_str_p;

  tmp_str_p = ecma_concat_ecma_strings (final_str_p, properties_str_p);
  ecma_deref_ecma_string (final_str_p);
  ecma_deref_ecma_string (properties_str_p);
  final_str_p = tmp_str_p;

  tmp_str_p = ecma_concat_ecma_strings (final_str_p, line_feed_p);
  ecma_deref_ecma_string (line_feed_p);
  ecma_deref_ecma_string (final_str_p);
  final_str_p = tmp_str_p;

  tmp_str_p = ecma_concat_ecma_strings (final_str_p, stepback_p);
  ecma_deref_ecma_string (final_str_p);
  final_str_p = tmp_str_p;

  tmp_str_p = ecma_concat_ecma_strings (final_str_p, right_bracket_p);
  ecma_deref_ecma_string (final_str_p);
  final_str_p = tmp_str_p;

  return ecma_make_string_value (final_str_p);
} /* ecma_builtin_helper_json_create_formatted_json */

/**
 * Common function to create a non-formatted JSON string.
 *
 * See also:
 *          ECMA-262 v5, 15.12.3
 *
 * Used by:
 *         - ecma_builtin_json_object step 10.a
 *         - ecma_builtin_json_array step 10.a
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_helper_json_create_non_formatted_json (ecma_string_t *left_bracket_p, /**< left bracket*/
                                                    ecma_string_t *right_bracket_p, /**< right bracket*/
                                                    ecma_collection_header_t *partial_p) /**< key-value pairs*/
{
  /* 10.a */
  ecma_string_t *comma_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_COMMA_CHAR);
  ecma_string_t *properties_str_p;
  ecma_string_t *tmp_str_p;

  /* 10.a.i */
  properties_str_p = ecma_builtin_helper_json_create_separated_properties (partial_p, comma_str_p);
  ecma_deref_ecma_string (comma_str_p);

  /* 10.a.ii */
  tmp_str_p = ecma_concat_ecma_strings (left_bracket_p, properties_str_p);
  ecma_deref_ecma_string (properties_str_p);
  properties_str_p = tmp_str_p;

  tmp_str_p = ecma_concat_ecma_strings (properties_str_p, right_bracket_p);
  ecma_deref_ecma_string (properties_str_p);
  properties_str_p = tmp_str_p;

  return ecma_make_string_value (properties_str_p);
} /* ecma_builtin_helper_json_create_non_formatted_json */

/**
 * Convert decimal value to 4 digit hexadecimal string value.
 *
 * See also:
 *          ECMA-262 v5, 15.12.3
 *
 * Used by:
 *         - ecma_builtin_json_quote step 2.c.iii
 *
 * @return pointer to ecma-string
 *         Returned value must be freed with ecma_deref_ecma_string.
 */
ecma_string_t *
ecma_builtin_helper_json_create_hex_digit_ecma_string (uint8_t value) /**< value in decimal*/
{
  /* 2.c.iii */
  ecma_string_t *hex_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);

  JMEM_DEFINE_LOCAL_ARRAY (hex_buff, 4, lit_utf8_byte_t);

  for (uint32_t i = 0; i < 4; i++)
  {
    uint8_t remainder = value % 16;
    lit_utf8_byte_t ch = ' ';

    if (remainder < 10)
    {
      ch = (lit_utf8_byte_t) (LIT_CHAR_0 + remainder);
    }
    else
    {
      uint8_t a = (uint8_t) (remainder - 10);
      ch = (lit_utf8_byte_t) (LIT_CHAR_LOWERCASE_A + a);
    }

    hex_buff[3 - i] = ch;

    value = value / 16;
  }

  ecma_deref_ecma_string (hex_str_p);
  hex_str_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) hex_buff, 4);

  JMEM_FINALIZE_LOCAL_ARRAY (hex_buff);

  JERRY_ASSERT (ecma_string_get_length (hex_str_p));

  return hex_str_p;
} /* ecma_builtin_helper_json_create_hex_digit_ecma_string */

/**
 * @}
 * @}
 */

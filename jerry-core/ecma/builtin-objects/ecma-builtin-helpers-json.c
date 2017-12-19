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

  ecma_collection_iterator_t iterator;
  ecma_collection_iterator_init (&iterator, partial_p);

  bool first = true;

  while (ecma_collection_iterator_next (&iterator))
  {
    ecma_value_t name_value = *iterator.current_value_p;
    ecma_string_t *current_p = ecma_get_string_from_value (name_value);

    if (likely (!first))
    {
      properties_str_p = ecma_concat_ecma_strings (properties_str_p, separator_p);
    }

    properties_str_p = ecma_concat_ecma_strings (properties_str_p, current_p);
    first = false;
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
ecma_builtin_helper_json_create_formatted_json (lit_utf8_byte_t left_bracket, /**< left bracket character */
                                                lit_utf8_byte_t right_bracket, /**< right bracket character */
                                                ecma_string_t *stepback_p, /**< stepback*/
                                                ecma_collection_header_t *partial_p, /**< key-value pairs*/
                                                ecma_json_stringify_context_t *context_p) /**< context*/
{
  JERRY_ASSERT (left_bracket < LIT_UTF8_1_BYTE_CODE_POINT_MAX
                && right_bracket < LIT_UTF8_1_BYTE_CODE_POINT_MAX);

  /* 10.b.i */
  lit_utf8_byte_t chars[2] = { LIT_CHAR_COMMA, LIT_CHAR_LF };

  ecma_string_t *separator_p = ecma_new_ecma_string_from_utf8 (chars, 2);

  separator_p = ecma_concat_ecma_strings (separator_p, context_p->indent_str_p);

  /* 10.b.ii */
  ecma_string_t *properties_str_p = ecma_builtin_helper_json_create_separated_properties (partial_p, separator_p);
  ecma_deref_ecma_string (separator_p);

  /* 10.b.iii */
  chars[0] = left_bracket;

  ecma_string_t *final_str_p = ecma_new_ecma_string_from_utf8 (chars, 2);

  final_str_p = ecma_concat_ecma_strings (final_str_p, context_p->indent_str_p);

  final_str_p = ecma_concat_ecma_strings (final_str_p, properties_str_p);
  ecma_deref_ecma_string (properties_str_p);

  chars[0] = LIT_CHAR_LF;
  final_str_p = ecma_append_chars_to_string (final_str_p, chars, 1, 1);

  final_str_p = ecma_concat_ecma_strings (final_str_p, stepback_p);

  chars[0] = right_bracket;
  final_str_p = ecma_append_chars_to_string (final_str_p, chars, 1, 1);

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
ecma_builtin_helper_json_create_non_formatted_json (lit_utf8_byte_t left_bracket, /**< left bracket character */
                                                    lit_utf8_byte_t right_bracket, /**< right bracket character */
                                                    ecma_collection_header_t *partial_p) /**< key-value pairs */
{
  JERRY_ASSERT (left_bracket < LIT_UTF8_1_BYTE_CODE_POINT_MAX
                && right_bracket < LIT_UTF8_1_BYTE_CODE_POINT_MAX);

  /* 10.a */
  ecma_string_t *comma_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_COMMA_CHAR);
  ecma_string_t *properties_str_p;

  /* 10.a.i */
  properties_str_p = ecma_builtin_helper_json_create_separated_properties (partial_p, comma_str_p);
  ecma_deref_ecma_string (comma_str_p);

  /* 10.a.ii */
  ecma_string_t *result_str_p = ecma_new_ecma_string_from_code_unit (left_bracket);

  result_str_p = ecma_concat_ecma_strings (result_str_p, properties_str_p);
  ecma_deref_ecma_string (properties_str_p);

  lit_utf8_byte_t chars[1] = { right_bracket };

  result_str_p = ecma_append_chars_to_string (result_str_p, chars, 1, 1);

  return ecma_make_string_value (result_str_p);
} /* ecma_builtin_helper_json_create_non_formatted_json */

/**
 * @}
 * @}
 */

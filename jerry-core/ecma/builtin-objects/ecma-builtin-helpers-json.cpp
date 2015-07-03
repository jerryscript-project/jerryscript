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

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

#include "ecma-alloc.h"
#include "ecma-helpers.h"
#include "ecma-builtin-helpers.h"
#include "lit-char-helpers.h"

#define LIST_BLOCK_SIZE 256UL

/**
 * Allocate memory for elements.
 *
 * @return pointer to current position of the allocated memory.
 */
void **
realloc_list (list_ctx_t *ctx_p) /**< list context */
{
  size_t old_block_size = static_cast<size_t> (ctx_p->block_end_p - ctx_p->block_start_p);
  size_t new_block_size = old_block_size + LIST_BLOCK_SIZE;

  size_t current_ptr_offset = static_cast<size_t> (ctx_p->current_p - ctx_p->block_start_p);
  void **new_block_start_p = (void **) mem_heap_alloc_block (new_block_size, MEM_HEAP_ALLOC_SHORT_TERM);

  if (ctx_p->current_p)
  {
    memcpy (new_block_start_p, ctx_p->block_start_p, static_cast<size_t> (current_ptr_offset));
    mem_heap_free_block (ctx_p->block_start_p);
  }

  ctx_p->block_start_p = new_block_start_p;
  ctx_p->block_end_p = new_block_start_p + new_block_size;
  ctx_p->current_p = new_block_start_p + current_ptr_offset;

  return ctx_p->current_p;
} /* realloc_list */

/**
 * Append element to the list.
 */
void
list_append (list_ctx_t *ctx_p, /**< list context */
             void *element_p) /**< element that should be stored */
{
  void **current_p = ctx_p->current_p;

  if (current_p + 1 > ctx_p->block_end_p)
  {
    current_p = realloc_list (ctx_p);
  }

  *current_p = element_p;

  ctx_p->current_p++;
} /* list_append */

/**
 * Check the element existance in the list.
 *
 * @return true if the list already has the value.
 */
bool
list_has_element (list_ctx_t *ctx_p, /**< list context */
                  void *element_p) /**< element that should be checked */
{
  for (void **current_p = ctx_p->block_start_p;
       current_p < ctx_p->current_p;
       current_p++)
  {
    if (*current_p == element_p)
    {
      return true;
    }
  }

  return false;
} /* list_has_element */

/**
 * Check the element existance in the list for ecma strings.
 *
 * @return true if the list already has the value.
 */
bool
list_has_ecma_string_element (list_ctx_t *ctx_p, /**< list context */
                              ecma_string_t *string_p) /**< element that should be checked */
{
  for (void **current_p = ctx_p->block_start_p;
       current_p < ctx_p->current_p;
       current_p++)
  {
    if (ecma_compare_ecma_strings ((ecma_string_t *) *current_p, string_p))
    {
      return true;
    }
  }

  return false;
} /* list_has_element */

/**
 * Check the state of the list allocation.
 *
 * @return true if the list is already allocated.
 */
static bool
list_is_allocated (list_ctx_t *ctx_p) /**< list context */
{
  return ctx_p->block_start_p < ctx_p->block_end_p;
} /* list_is_allocated */

/**
 * Check if the list is empty.
 *
 * @return true if the list is empty.
 */
bool
list_is_empty (list_ctx_t *ctx_p)
{
  return ctx_p->block_start_p == ctx_p->current_p;
} /* list_is_empty */

/**
 * Set the current memory position to the previous element.
 */
void
list_remove_last_element (list_ctx_t *ctx_p) /**< list context */
{
  if (list_is_empty (ctx_p))
  {
    return;
  }
  ctx_p->current_p = NULL;

  ctx_p->current_p--;
} /* list_remove_last_element */

/**
 * Free the allocated list.
 */
void
free_list (list_ctx_t *ctx_p) /**< list context */
{
  if (!list_is_allocated (ctx_p))
  {
    return;
  }

  mem_heap_free_block (ctx_p->block_start_p);

  ctx_p->block_start_p = NULL;
  ctx_p->block_end_p = NULL;
  ctx_p->current_p = NULL;
} /* free_list */

/**
 * Free the stored ecma string elements and the list.
 */
void
free_list_with_ecma_string_content (list_ctx_t *ctx_p) /**< list context */
{
  if (!list_is_allocated (ctx_p))
  {
    return;
  }

  for (void **current_p = ctx_p->block_start_p;
       current_p < ctx_p->current_p;
       current_p++)
  {
    ecma_deref_ecma_string ((ecma_string_t *) *current_p);
  }

  free_list (ctx_p);
} /* free_list_with_ecma_string_content */

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
ecma_builtin_helper_json_create_separated_properties (list_ctx_t *partial_p, /**< list of key-value pairs*/
                                                      ecma_string_t *separator_p) /**< separator*/
{
  ecma_string_t *properties_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
  ecma_string_t *tmp_str_p;

  uint32_t index = 0;

  for (void **current_p = partial_p->block_start_p;
       current_p < partial_p->current_p;
       current_p++, index++)
  {
    if (index == 0)
    {
      tmp_str_p = ecma_concat_ecma_strings (properties_str_p, (ecma_string_t *) *current_p);
      ecma_deref_ecma_string (properties_str_p);
      properties_str_p = tmp_str_p;
      continue;
    }

    tmp_str_p = ecma_concat_ecma_strings (properties_str_p, separator_p);
    ecma_deref_ecma_string (properties_str_p);
    properties_str_p = tmp_str_p;

    tmp_str_p = ecma_concat_ecma_strings (properties_str_p, (ecma_string_t *) *current_p);
    ecma_deref_ecma_string (properties_str_p);
    properties_str_p = tmp_str_p;
  }

  return properties_str_p;
} /* ecma_builtin_json_helper_create_separated_properties */

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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_helper_json_create_formatted_json (ecma_string_t *left_bracket_p, /**< left bracket*/
                                                ecma_string_t *right_bracket_p, /**< right bracket*/
                                                ecma_string_t *stepback_p, /**< stepback*/
                                                list_ctx_t *partial_p, /**< list of key-value pairs*/
                                                stringify_context_t *context_p) /**< context*/
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

  return ecma_make_normal_completion_value (ecma_make_string_value (final_str_p));
} /* ecma_builtin_json_helper_create_formatted_json */

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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_helper_json_create_non_formatted_json (ecma_string_t *left_bracket_p, /**< left bracket*/
                                                    ecma_string_t *right_bracket_p, /**< right bracket*/
                                                    list_ctx_t *partial_p) /**< list of key-value pairs*/
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

  return ecma_make_normal_completion_value (ecma_make_string_value (properties_str_p));
} /* ecma_builtin_json_helper_create_non_formatted_json */

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

  MEM_DEFINE_LOCAL_ARRAY (hex_buff, 4, lit_utf8_byte_t);

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

  MEM_FINALIZE_LOCAL_ARRAY (hex_buff);

  JERRY_ASSERT (ecma_string_get_length (hex_str_p));

  return hex_str_p;
} /* ecma_builtin_helper_json_create_hex_digit_ecma_string */

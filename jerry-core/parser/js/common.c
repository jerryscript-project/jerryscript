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

#include "common.h"
#include "ecma-helpers.h"
#include "ecma-big-uint.h"
#include "ecma-bigint.h"
#include "lit-char-helpers.h"

#if ENABLED (JERRY_PARSER)

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup jsparser JavaScript
 * @{
 *
 * \addtogroup jsparser_utils Utility
 * @{
 */

/**
 * Free literal.
 */
void
util_free_literal (lexer_literal_t *literal_p) /**< literal */
{
  if (literal_p->type == LEXER_IDENT_LITERAL
      || literal_p->type == LEXER_STRING_LITERAL)
  {
    if (!(literal_p->status_flags & LEXER_FLAG_SOURCE_PTR))
    {
      jmem_heap_free_block ((void *) literal_p->u.char_p, literal_p->prop.length);
    }
  }
  else if ((literal_p->type == LEXER_FUNCTION_LITERAL)
           || (literal_p->type == LEXER_REGEXP_LITERAL))
  {
    ecma_bytecode_deref (literal_p->u.bytecode_p);
  }
} /* util_free_literal */

#if ENABLED (JERRY_PARSER_DUMP_BYTE_CODE)

/**
 * Debug utility to print a character sequence.
 */
static void
util_print_chars (const uint8_t *char_p, /**< character pointer */
                  size_t size) /**< size */
{
  while (size > 0)
  {
    JERRY_DEBUG_MSG ("%c", *char_p++);
    size--;
  }
} /* util_print_chars */

/**
 * Debug utility to print a number.
 */
static void
util_print_number (ecma_number_t num_p) /**< number to print */
{
  lit_utf8_byte_t str_buf[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
  lit_utf8_size_t str_size = ecma_number_to_utf8_string (num_p, str_buf, sizeof (str_buf));
  str_buf[str_size] = 0;
  JERRY_DEBUG_MSG ("%s", str_buf);
} /* util_print_number */

#if ENABLED (JERRY_BUILTIN_BIGINT)

/**
 * Debug utility to print a bigint.
 */
static void
util_print_bigint (ecma_value_t bigint) /**< bigint to print */
{
  if (bigint == ECMA_BIGINT_ZERO)
  {
    JERRY_DEBUG_MSG ("0");
    return;
  }

  ecma_extended_primitive_t *bigint_p = ecma_get_extended_primitive_from_value (bigint);
  uint32_t char_start_p, char_size_p;
  lit_utf8_byte_t *string_buffer_p = ecma_big_uint_to_string (bigint_p, 10, &char_start_p, &char_size_p);

  if (JERRY_UNLIKELY (string_buffer_p == NULL))
  {
    JERRY_DEBUG_MSG ("<out-of-memory>");
    return;
  }

  JERRY_ASSERT (char_start_p > 0);

  if (bigint_p->u.bigint_sign_and_size & ECMA_BIGINT_SIGN)
  {
    string_buffer_p[--char_start_p] = LIT_CHAR_MINUS;
  }

  util_print_chars (string_buffer_p + char_start_p, char_size_p - char_start_p);
  jmem_heap_free_block (string_buffer_p, char_size_p);
} /* util_print_bigint */

#endif /* ENABLED (JERRY_BUILTIN_BIGINT) */

/**
 * Print literal.
 */
void
util_print_literal (lexer_literal_t *literal_p) /**< literal */
{
  switch (literal_p->type)
  {
    case LEXER_IDENT_LITERAL:
    {
      JERRY_DEBUG_MSG ("ident(");
      util_print_chars (literal_p->u.char_p, literal_p->prop.length);
      break;
    }
    case LEXER_FUNCTION_LITERAL:
    {
      JERRY_DEBUG_MSG ("function");
      return;
    }
    case LEXER_STRING_LITERAL:
    {
      JERRY_DEBUG_MSG ("string(");
      util_print_chars (literal_p->u.char_p, literal_p->prop.length);
      break;
    }
    case LEXER_NUMBER_LITERAL:
    {
#if ENABLED (JERRY_BUILTIN_BIGINT)
      if (ecma_is_value_bigint (literal_p->u.value))
      {
        JERRY_DEBUG_MSG ("bigint(");
        util_print_bigint (literal_p->u.value);
        break;
      }
#endif /* ENABLED (JERRY_BUILTIN_BIGINT) */
      JERRY_DEBUG_MSG ("number(");
      util_print_number (ecma_get_number_from_value (literal_p->u.value));
      break;
    }
    case LEXER_REGEXP_LITERAL:
    {
      JERRY_DEBUG_MSG ("regexp");
      return;
    }
    default:
    {
      JERRY_DEBUG_MSG ("unknown");
      return;
    }
  }

  JERRY_DEBUG_MSG (")");
} /* util_print_literal */

#endif /* ENABLED (JERRY_PARSER_DUMP_BYTE_CODE) */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_PARSER) */

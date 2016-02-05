/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
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

#include "common.h"
#include "ecma-helpers.h"
#include "lit-char-helpers.h"

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
 * Checks whether the next UTF8 character is a valid identifier start.
 *
 * @return non-zero if it is.
 */
int
util_is_identifier_start (const uint8_t *src_p) /* pointer to a vaild UTF8 character */
{
  if (*src_p <= 127)
  {
    return util_is_identifier_start_character (*src_p);
  }

  return util_is_identifier_start_character (lit_utf8_peek_next (src_p));
} /* util_is_identifier_start */

/**
 * Checks whether the next UTF8 character is a valid identifier part.
 *
 * @return non-zero if it is.
 */
int
util_is_identifier_part (const uint8_t *src_p) /* pointer to a vaild UTF8 character */
{
  if (*src_p <= 127)
  {
    return util_is_identifier_part_character (*src_p);
  }

  return util_is_identifier_part_character (lit_utf8_peek_next (src_p));
} /* util_is_identifier_part */

/**
 * Checks whether the character is a valid identifier start.
 *
 * @return non-zero if it is.
 */
int
util_is_identifier_start_character (uint16_t chr) /**< EcmaScript character */
{
  if (chr <= 127)
  {
    return (((chr | 0x20) >= LIT_CHAR_LOWERCASE_A && (chr | 0x20) <= LIT_CHAR_LOWERCASE_Z)
            || chr == LIT_CHAR_DOLLAR_SIGN
            || chr == LIT_CHAR_UNDERSCORE);
  }

  return lit_char_is_unicode_letter (chr);

} /* util_is_identifier_start_character */

/**
 * Checks whether the character is a valid identifier part.
 *
 * @return non-zero if it is.
 */
int
util_is_identifier_part_character (uint16_t chr) /**< EcmaScript character */
{
  if (chr <= 127)
  {
    return (((chr | 0x20) >= LIT_CHAR_LOWERCASE_A && (chr | 0x20) <= LIT_CHAR_LOWERCASE_Z)
            || (chr >= LIT_CHAR_0 && chr <= LIT_CHAR_9)
            || chr == LIT_CHAR_DOLLAR_SIGN
            || chr == LIT_CHAR_UNDERSCORE);
  }

  return (lit_char_is_unicode_letter (chr)
          || lit_char_is_unicode_combining_mark (chr)
          || lit_char_is_unicode_digit (chr)
          || lit_char_is_unicode_connector_punctuation (chr));
} /* util_is_identifier_part_character */

/**
 * Converts a character to UTF8 bytes.
 *
 * @return length of the UTF8 representation.
 */
size_t
util_to_utf8_bytes (uint8_t *dst_p, /**< destination buffer */
                    uint16_t chr) /**< EcmaScript character */
{
  if (!(chr & ~0x007f))
  {
    /* 00000000 0xxxxxxx -> 0xxxxxxx */
    *dst_p = (uint8_t) chr;
    return 1;
  }

  if (!(chr & ~0x07ff))
  {
    /* 00000yyy yyxxxxxx -> 110yyyyy 10xxxxxx */
    *(dst_p++) = (uint8_t) (0xc0 | ((chr >> 6) & 0x1f));
    *dst_p = (uint8_t) (0x80 | (chr & 0x3f));
    return 2;
  }

  /* zzzzyyyy yyxxxxxx -> 1110zzzz 10yyyyyy 10xxxxxx */
  *(dst_p++) = (uint8_t) (0xe0 | ((chr >> 12) & 0x0f));
  *(dst_p++) = (uint8_t) (0x80 | ((chr >> 6) & 0x3f));
  *dst_p = (uint8_t) (0x80 | (chr & 0x3f));
  return 3;
} /* util_to_utf8_bytes */

/**
 * Returns the length of the UTF8 representation of a character.
 *
 * @return length of the UTF8 representation.
 */
size_t
util_get_utf8_length (uint16_t chr) /**< EcmaScript character */
{
  if (!(chr & ~0x007f))
  {
    /* 00000000 0xxxxxxx */
    return 1;
  }

  if (!(chr & ~0x07ff))
  {
    /* 00000yyy yyxxxxxx */
    return 2;
  }

  /* zzzzyyyy yyxxxxxx */
  return 3;
} /* util_get_utf8_length */

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
      PARSER_FREE ((uint8_t *) literal_p->u.char_p);
    }
  }
  else if ((literal_p->type == LEXER_FUNCTION_LITERAL)
           || (literal_p->type == LEXER_REGEXP_LITERAL))
  {
    ecma_bytecode_deref (literal_p->u.bytecode_p);
  }
} /* util_free_literal */

#ifdef PARSER_DUMP_BYTE_CODE

/**
 * Debug utility to print a character sequence.
 */
static void
util_print_chars (const uint8_t *char_p, /**< character pointer */
                  size_t size) /**< size */
{
  while (size > 0)
  {
    printf ("%c", *char_p++);
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
  printf ("%s", str_buf);
} /* util_print_number */

/**
 * Print literal.
 */
void
util_print_literal (lexer_literal_t *literal_p) /**< literal */
{
  if (literal_p->type == LEXER_IDENT_LITERAL)
  {
    if (literal_p->status_flags & LEXER_FLAG_VAR)
    {
      printf ("var_ident(");
    }
    else
    {
      printf ("ident(");
    }
    util_print_chars (literal_p->u.char_p, literal_p->prop.length);
  }
  else if (literal_p->type == LEXER_FUNCTION_LITERAL)
  {
    printf ("function");
    return;
  }
  else if (literal_p->type == LEXER_STRING_LITERAL)
  {
    printf ("string(");
    util_print_chars (literal_p->u.char_p, literal_p->prop.length);
  }
  else if (literal_p->type == LEXER_NUMBER_LITERAL)
  {
    lit_literal_t literal = lit_get_literal_by_cp (literal_p->u.value);
    printf ("number(");
    util_print_number (lit_number_literal_get_number (literal));
  }
  else if (literal_p->type == LEXER_REGEXP_LITERAL)
  {
    printf ("regexp");
    return;
  }
  else
  {
    printf ("unknown");
    return;
  }

  printf (")");
} /* util_print_literal */

#endif /* PARSER_DUMP_BYTE_CODE */

/**
 * @}
 * @}
 * @}
 */

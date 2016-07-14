/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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

#include "lit-char-helpers.h"
#include "lit/lit-unicode-ranges.inc.h"
#include "lit-strings.h"

#define NUM_OF_ELEMENTS(array) (sizeof (array) / sizeof ((array)[0]))

/**
 * Binary search algorithm that searches the a
 * character in the given char array.
 *
 * @return true - if the character is in the given array
 *         false - otherwise
 */
static bool
search_char_in_char_array (ecma_char_t c,               /**< code unit */
                           const ecma_char_t *array,    /**< array */
                           int size_of_array)           /**< length of the array */
{
  int bottom = 0;
  int top = size_of_array - 1;

  while (bottom <= top)
  {
    int middle = (bottom + top) / 2;
    ecma_char_t current = array[middle];

    if (current == c)
    {
      return true;
    }

    if (c < current)
    {
      top = middle - 1;
    }
    else
    {
      bottom = middle + 1;
    }
  }

  return false;
} /* search_char_in_char_array */

/**
 * Binary search algorithm that searches a character in the given intervals.
 * Intervals specifed by two arrays. The first one contains the starting points
 * of the intervals, the second one contains the length of them.
 *
 * @return true - if the the character is included (inclusively) in one of the intervals in the given array
 *         false - otherwise
 */
static bool
search_char_in_interval_array (ecma_char_t c,               /**< code unit */
                               const ecma_char_t *array_sp, /**< array of interval starting points */
                               const uint8_t *lengths,      /**< array of interval lengths */
                               int size_of_array)           /**< length of the array */
{
  int bottom = 0;
  int top = size_of_array - 1;

  while (bottom <= top)
  {
    int middle = (bottom + top) / 2;
    ecma_char_t current_sp = array_sp[middle];

    if (current_sp <= c && c <= current_sp + lengths[middle])
    {
      return true;
    }

    if (c > current_sp)
    {
      bottom = middle + 1;
    }
    else
    {
      top = middle - 1;
    }
  }

  return false;
} /* search_char_in_interval_array */

/**
 * Check if specified character is one of the Format-Control characters
 *
 * @return true - if the character is one of characters, listed in ECMA-262 v5, Table 1,
 *         false - otherwise.
 */
bool
lit_char_is_format_control (ecma_char_t c) /**< code unit */
{
  return (c == LIT_CHAR_ZWNJ
          || c == LIT_CHAR_ZWJ
          || c == LIT_CHAR_BOM);
} /* lit_char_is_format_control */

/**
 * Check if specified character is one of the Whitespace characters including those
 * that fall into "Space, Separator" ("Zs") Unicode character category.
 *
 * @return true - if the character is one of characters, listed in ECMA-262 v5, Table 2,
 *         false - otherwise.
 */
bool
lit_char_is_white_space (ecma_char_t c) /**< code unit */
{
  if (c <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
  {
    return (c == LIT_CHAR_TAB
            || c == LIT_CHAR_VTAB
            || c == LIT_CHAR_FF
            || c == LIT_CHAR_SP);
  }
  else
  {
    return (c == LIT_CHAR_NBSP
            || c == LIT_CHAR_BOM
            || (c >= unicode_separator_char_interv_sps[0]
                && c <= unicode_separator_char_interv_sps[0] + unicode_separator_char_interv_lens[0])
            || search_char_in_char_array (c, unicode_separator_chars, NUM_OF_ELEMENTS (unicode_separator_chars)));
  }
} /* lit_char_is_white_space */

/**
 * Check if specified character is one of LineTerminator characters
 *
 * @return true - if the character is one of characters, listed in ECMA-262 v5, Table 3,
 *         false - otherwise.
 */
bool
lit_char_is_line_terminator (ecma_char_t c) /**< code unit */
{
  return (c == LIT_CHAR_LF
          || c == LIT_CHAR_CR
          || c == LIT_CHAR_LS
          || c == LIT_CHAR_PS);
} /* lit_char_is_line_terminator */

/**
 * Check if specified character is a unicode letter
 *
 * Note:
 *      Unicode letter is a character, included into one of the following categories:
 *       - Uppercase letter (Lu);
 *       - Lowercase letter (Ll);
 *       - Titlecase letter (Lt);
 *       - Modifier letter (Lm);
 *       - Other letter (Lo);
 *       - Letter number (Nl).
 *
 * See also:
 *          ECMA-262 v5, 7.6
 *
 * @return true - if specified character falls into one of the listed categories,
 *         false - otherwise.
 */
static bool
lit_char_is_unicode_letter (ecma_char_t c) /**< code unit */
{
  return (search_char_in_interval_array (c, unicode_letter_interv_sps, unicode_letter_interv_lens,
                                         NUM_OF_ELEMENTS (unicode_letter_interv_sps))
          || search_char_in_char_array (c, unicode_letter_chars, NUM_OF_ELEMENTS (unicode_letter_chars)));
} /* lit_char_is_unicode_letter */

/**
 * Check if specified character is a non-letter character and can be used as a
 * non-first character of an identifier.
 * These characters coverd by the following unicode categories:
 *  - digit (Nd)
 *  - punctuation mark (Mn, Mc)
 *  - connector punctuation (Pc)
 *
 * See also:
 *          ECMA-262 v5, 7.6
 *
 * @return true - if specified character falls into one of the listed categories,
 *         false - otherwise.
 */
static bool
lit_char_is_unicode_non_letter_ident_part (ecma_char_t c) /**< code unit */
{
  return (search_char_in_interval_array (c, unicode_non_letter_ident_part_interv_sps,
                                         unicode_non_letter_ident_part_interv_lens,
                                         NUM_OF_ELEMENTS (unicode_non_letter_ident_part_interv_sps))
          || search_char_in_char_array (c, unicode_non_letter_ident_part_chars,
                                        NUM_OF_ELEMENTS (unicode_non_letter_ident_part_chars)));
} /* lit_char_is_unicode_non_letter_ident_part */

/**
 * Checks whether the next UTF8 character is a valid identifier start.
 *
 * @return true if it is.
 */
bool
lit_char_is_identifier_start (const uint8_t *src_p) /**< pointer to a vaild UTF8 character */
{
  if (*src_p <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
  {
    return lit_char_is_identifier_start_character (*src_p);
  }

  return lit_char_is_identifier_start_character (lit_utf8_peek_next (src_p));
} /* lit_char_is_identifier_start */

/**
 * Checks whether the character is a valid identifier start.
 *
 * @return true if it is.
 */
bool
lit_char_is_identifier_start_character (uint16_t chr) /**< EcmaScript character */
{
  // Fast path for ASCII-defined letters
  if (chr <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
  {
    return ((LEXER_TO_ASCII_LOWERCASE (chr) >= LIT_CHAR_LOWERCASE_A
             && LEXER_TO_ASCII_LOWERCASE (chr) <= LIT_CHAR_LOWERCASE_Z)
            || chr == LIT_CHAR_DOLLAR_SIGN
            || chr == LIT_CHAR_UNDERSCORE);
  }

  return lit_char_is_unicode_letter (chr);
} /* lit_char_is_identifier_start_character */

/**
 * Checks whether the next UTF8 character is a valid identifier part.
 *
 * @return true if it is.
 */
bool
lit_char_is_identifier_part (const uint8_t *src_p) /**< pointer to a vaild UTF8 character */
{
  if (*src_p <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
  {
    return lit_char_is_identifier_part_character (*src_p);
  }

  return lit_char_is_identifier_part_character (lit_utf8_peek_next (src_p));
} /* lit_char_is_identifier_part */

/**
 * Checks whether the character is a valid identifier part.
 *
 * @return true if it is.
 */
bool
lit_char_is_identifier_part_character (uint16_t chr) /**< EcmaScript character */
{
  // Fast path for ASCII-defined letters
  if (chr <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
  {
    return ((LEXER_TO_ASCII_LOWERCASE (chr) >= LIT_CHAR_LOWERCASE_A
             && LEXER_TO_ASCII_LOWERCASE (chr) <= LIT_CHAR_LOWERCASE_Z)
            || (chr >= LIT_CHAR_0 && chr <= LIT_CHAR_9)
            || chr == LIT_CHAR_DOLLAR_SIGN
            || chr == LIT_CHAR_UNDERSCORE);
  }

  return (lit_char_is_unicode_letter (chr)
          || lit_char_is_unicode_non_letter_ident_part (chr));
} /* lit_char_is_identifier_part_character */

/**
 * Check if specified character is one of OctalDigit characters (ECMA-262 v5, B.1.2)
 *
 * @return true / false
 */
bool
lit_char_is_octal_digit (ecma_char_t c) /**< code unit */
{
  return (c >= LIT_CHAR_ASCII_OCTAL_DIGITS_BEGIN && c <= LIT_CHAR_ASCII_OCTAL_DIGITS_END);
} /* lit_char_is_octal_digit */

/**
 * Check if specified character is one of DecimalDigit characters (ECMA-262 v5, 7.8.3)
 *
 * @return true / false
 */
bool
lit_char_is_decimal_digit (ecma_char_t c) /**< code unit */
{
  return (c >= LIT_CHAR_ASCII_DIGITS_BEGIN && c <= LIT_CHAR_ASCII_DIGITS_END);
} /* lit_char_is_decimal_digit */

/**
 * Check if specified character is one of HexDigit characters (ECMA-262 v5, 7.8.3)
 *
 * @return true / false
 */
bool
lit_char_is_hex_digit (ecma_char_t c) /**< code unit */
{
  return ((c >= LIT_CHAR_ASCII_DIGITS_BEGIN && c <= LIT_CHAR_ASCII_DIGITS_END)
          || (LEXER_TO_ASCII_LOWERCASE (c) >= LIT_CHAR_ASCII_LOWERCASE_LETTERS_HEX_BEGIN
              && LEXER_TO_ASCII_LOWERCASE (c) <= LIT_CHAR_ASCII_LOWERCASE_LETTERS_HEX_END));
} /* lit_char_is_hex_digit */

/**
 * Convert a HexDigit character to its numeric value, as defined in ECMA-262 v5, 7.8.3
 *
 * @return digit value, corresponding to the hex char
 */
uint32_t
lit_char_hex_to_int (ecma_char_t c) /**< code unit, corresponding to
                                     *    one of HexDigit characters */
{
  JERRY_ASSERT (lit_char_is_hex_digit (c));

  if (c >= LIT_CHAR_ASCII_DIGITS_BEGIN && c <= LIT_CHAR_ASCII_DIGITS_END)
  {
    return (uint32_t) (c - LIT_CHAR_ASCII_DIGITS_BEGIN);
  }
  else if (c >= LIT_CHAR_ASCII_LOWERCASE_LETTERS_HEX_BEGIN && c <= LIT_CHAR_ASCII_LOWERCASE_LETTERS_HEX_END)
  {
    return (uint32_t) (c - LIT_CHAR_ASCII_LOWERCASE_LETTERS_HEX_BEGIN + 10);
  }
  else
  {
    return (uint32_t) (c - LIT_CHAR_ASCII_UPPERCASE_LETTERS_HEX_BEGIN + 10);
  }
} /* lit_char_hex_to_int */

/**
 * Converts a character to UTF8 bytes.
 *
 * @return length of the UTF8 representation.
 */
size_t
lit_char_to_utf8_bytes (uint8_t *dst_p, /**< destination buffer */
                        ecma_char_t chr) /**< EcmaScript character */
{
  if (!(chr & ~LIT_UTF8_1_BYTE_CODE_POINT_MAX))
  {
    /* 00000000 0xxxxxxx -> 0xxxxxxx */
    *dst_p = (uint8_t) chr;
    return 1;
  }

  if (!(chr & ~LIT_UTF8_2_BYTE_CODE_POINT_MAX))
  {
    /* 00000yyy yyxxxxxx -> 110yyyyy 10xxxxxx */
    *(dst_p++) = (uint8_t) (LIT_UTF8_2_BYTE_MARKER | ((chr >> 6) & LIT_UTF8_LAST_5_BITS_MASK));
    *dst_p = (uint8_t) (LIT_UTF8_EXTRA_BYTE_MARKER | (chr & LIT_UTF8_LAST_6_BITS_MASK));
    return 2;
  }

  JERRY_ASSERT (!(chr & ~LIT_UTF8_3_BYTE_CODE_POINT_MAX));
  /* zzzzyyyy yyxxxxxx -> 1110zzzz 10yyyyyy 10xxxxxx */
  *(dst_p++) = (uint8_t) (LIT_UTF8_3_BYTE_MARKER | ((chr >> 12) & LIT_UTF8_LAST_4_BITS_MASK));
  *(dst_p++) = (uint8_t) (LIT_UTF8_EXTRA_BYTE_MARKER | ((chr >> 6) & LIT_UTF8_LAST_6_BITS_MASK));
  *dst_p = (uint8_t) (LIT_UTF8_EXTRA_BYTE_MARKER | (chr & LIT_UTF8_LAST_6_BITS_MASK));
  return 3;
} /* lit_char_to_utf8_bytes */

/**
 * Returns the length of the UTF8 representation of a character.
 *
 * @return length of the UTF8 representation.
 */
size_t
lit_char_get_utf8_length (ecma_char_t chr) /**< EcmaScript character */
{
  if (!(chr & ~LIT_UTF8_1_BYTE_CODE_POINT_MAX))
  {
    /* 00000000 0xxxxxxx */
    return 1;
  }

  if (!(chr & ~LIT_UTF8_2_BYTE_CODE_POINT_MAX))
  {
    /* 00000yyy yyxxxxxx */
    return 2;
  }

  /* zzzzyyyy yyxxxxxx */
  JERRY_ASSERT (!(chr & ~LIT_UTF8_3_BYTE_CODE_POINT_MAX));
  return 3;
} /* lit_char_get_utf8_length */

/**
 * Parse the next number_of_characters hexadecimal character,
 * and construct a code unit from them. The buffer must
 * be zero terminated.
 *
 * @return true if decoding was successful, false otherwise
 */
bool
lit_read_code_unit_from_hex (const lit_utf8_byte_t *buf_p, /**< buffer with characters */
                             lit_utf8_size_t number_of_characters, /**< number of characters to be read */
                             ecma_char_ptr_t out_code_unit_p) /**< [out] decoded result */
{
  ecma_char_t code_unit = LIT_CHAR_NULL;

  JERRY_ASSERT (number_of_characters >= 2 && number_of_characters <= 4);

  for (lit_utf8_size_t i = 0; i < number_of_characters; i++)
  {
    code_unit = (ecma_char_t) (code_unit << 4u);

    if (*buf_p >= LIT_CHAR_ASCII_DIGITS_BEGIN
        && *buf_p <= LIT_CHAR_ASCII_DIGITS_END)
    {
      code_unit |= (ecma_char_t) (*buf_p - LIT_CHAR_ASCII_DIGITS_BEGIN);
    }
    else if (*buf_p >= LIT_CHAR_ASCII_LOWERCASE_LETTERS_HEX_BEGIN
             && *buf_p <= LIT_CHAR_ASCII_LOWERCASE_LETTERS_HEX_END)
    {
      code_unit |= (ecma_char_t) (*buf_p - (LIT_CHAR_ASCII_LOWERCASE_LETTERS_HEX_BEGIN - 10));
    }
    else if (*buf_p >= LIT_CHAR_ASCII_UPPERCASE_LETTERS_HEX_BEGIN
             && *buf_p <= LIT_CHAR_ASCII_UPPERCASE_LETTERS_HEX_END)
    {
      code_unit |= (ecma_char_t) (*buf_p - (LIT_CHAR_ASCII_UPPERCASE_LETTERS_HEX_BEGIN - 10));
    }
    else
    {
      return false;
    }

    buf_p++;
  }

  *out_code_unit_p = code_unit;
  return true;
} /* lit_read_code_unit_from_hex */

/**
 * Check if specified character is a word character (part of IsWordChar abstract operation)
 *
 * See also: ECMA-262 v5, 15.10.2.6 (IsWordChar)
 *
 * @return true - if the character is a word character
 *         false - otherwise.
 */
bool
lit_char_is_word_char (ecma_char_t c) /**< code unit */
{
  return ((c >= LIT_CHAR_ASCII_LOWERCASE_LETTERS_BEGIN && c <= LIT_CHAR_ASCII_LOWERCASE_LETTERS_END)
          || (c >= LIT_CHAR_ASCII_UPPERCASE_LETTERS_BEGIN && c <= LIT_CHAR_ASCII_UPPERCASE_LETTERS_END)
          || (c >= LIT_CHAR_ASCII_DIGITS_BEGIN && c <= LIT_CHAR_ASCII_DIGITS_END)
          || c == LIT_CHAR_UNDERSCORE);
} /* lit_char_is_word_char */

/**
 * Returns the lowercase character sequence of an ecma character.
 *
 * Note: output_buffer_p must be able to hold at least LIT_MAXIMUM_OTHER_CASE_LENGTH characters.
 *
 * @return the length of the lowercase character sequence
 *         which is always between 1 and LIT_MAXIMUM_OTHER_CASE_LENGTH.
 */
ecma_length_t
lit_char_to_lower_case (ecma_char_t character, /**< input character value */
                        ecma_char_t *output_buffer_p, /**< [out] buffer for the result characters */
                        ecma_length_t buffer_size) /**< buffer size */
{
  /* TODO: Needs a proper lower case implementation. See issue #323. */

  JERRY_ASSERT (buffer_size >= LIT_MAXIMUM_OTHER_CASE_LENGTH);

  if (character >= LIT_CHAR_UPPERCASE_A && character <= LIT_CHAR_UPPERCASE_Z)
  {
    output_buffer_p[0] = (ecma_char_t) (character + (LIT_CHAR_LOWERCASE_A - LIT_CHAR_UPPERCASE_A));
    return 1;
  }

  output_buffer_p[0] = character;
  return 1;
} /* lit_char_to_lower_case */

/**
 * Returns the uppercase character sequence of an ecma character.
 *
 * Note: output_buffer_p must be able to hold at least LIT_MAXIMUM_OTHER_CASE_LENGTH characters.
 *
 * @return the length of the uppercase character sequence
 *         which is always between 1 and LIT_MAXIMUM_OTHER_CASE_LENGTH.
 */
ecma_length_t
lit_char_to_upper_case (ecma_char_t character, /**< input character value */
                        ecma_char_t *output_buffer_p, /**< buffer for the result characters */
                        ecma_length_t buffer_size) /**< buffer size */
{
  /* TODO: Needs a proper upper case implementation. See issue #323. */

  JERRY_ASSERT (buffer_size >= LIT_MAXIMUM_OTHER_CASE_LENGTH);

  if (character >= LIT_CHAR_LOWERCASE_A && character <= LIT_CHAR_LOWERCASE_Z)
  {
    output_buffer_p[0] = (ecma_char_t) (character - (LIT_CHAR_LOWERCASE_A - LIT_CHAR_UPPERCASE_A));
    return 1;
  }

  output_buffer_p[0] = character;
  return 1;
} /* lit_char_to_upper_case */

/* Copyright 2015 Samsung Electronics Co., Ltd.
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
 * Check if specified character is the Space Separator character
 *
 * See also:
 *          ECMA-262 v5, Table 2
 *
 * @return true - if the character falls into "Space, Separator" ("Zs") character category,
 *         false - otherwise.
 */
bool
lit_char_is_space_separator (ecma_char_t c) /**< code unit */
{
  /* Zs */
#define LIT_UNICODE_RANGE_ZS(range_begin, range_end) \
  if (c >= (range_begin) && c <= (range_end)) \
  { \
    return true; \
  }
#include "lit-unicode-ranges.inc.h"

  return false;
} /* lit_char_is_space_separator */

/**
 * Check if specified character is one of the Whitespace characters
 *
 * @return true - if the character is one of characters, listed in ECMA-262 v5, Table 2,
 *         false - otherwise.
 */
bool
lit_char_is_white_space (ecma_char_t c) /**< code unit */
{
  return (c == LIT_CHAR_TAB
          || c == LIT_CHAR_VTAB
          || c == LIT_CHAR_FF
          || c == LIT_CHAR_SP
          || c == LIT_CHAR_NBSP
          || c == LIT_CHAR_BOM
          || lit_char_is_space_separator (c));
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
bool
lit_char_is_unicode_letter (ecma_char_t c) /**< code unit */
{
  /* Fast path for ASCII-defined letters */
  if ((c >= LIT_CHAR_ASCII_LOWERCASE_LETTERS_BEGIN && c <= LIT_CHAR_ASCII_LOWERCASE_LETTERS_END)
      || (c >= LIT_CHAR_ASCII_UPPERCASE_LETTERS_BEGIN && c <= LIT_CHAR_ASCII_UPPERCASE_LETTERS_END))
  {
    return true;
  }

  /* Lu */
#define LIT_UNICODE_RANGE_LU(range_begin, range_end) \
  if (c >= (range_begin) && c <= (range_end)) \
  { \
    return true; \
  }

  /* Ll */
#define LIT_UNICODE_RANGE_LL(range_begin, range_end) \
  if (c >= (range_begin) && c <= (range_end)) \
  { \
    return true; \
  }

  /* Lt */
#define LIT_UNICODE_RANGE_LT(range_begin, range_end) \
  if (c >= (range_begin) && c <= (range_end)) \
  { \
    return true; \
  }

  /* Lm */
#define LIT_UNICODE_RANGE_LM(range_begin, range_end) \
  if (c >= (range_begin) && c <= (range_end)) \
  { \
    return true; \
  }

  /* Lo */
#define LIT_UNICODE_RANGE_LO(range_begin, range_end) \
  if (c >= (range_begin) && c <= (range_end)) \
  { \
    return true; \
  }

  /* Nl */
#define LIT_UNICODE_RANGE_NL(range_begin, range_end) \
  if (c >= (range_begin) && c <= (range_end)) \
  { \
    return true; \
  }

#include "lit-unicode-ranges.inc.h"

  return false;
} /* lit_char_is_unicode_letter */

/**
 * Check if specified character is a unicode combining mark
 *
 * Note:
 *      Unicode combining mark is a character, included into one of the following categories:
 *       - Non-spacing mark (Mn);
 *       - Combining spacing mark (Mc).
 *
 * See also:
 *          ECMA-262 v5, 7.6
 *
 * @return true - if specified character falls into one of the listed categories,
 *         false - otherwise.
 */
bool
lit_char_is_unicode_combining_mark (ecma_char_t c) /**< code unit */
{
  /* Mn */
#define LIT_UNICODE_RANGE_MN(range_begin, range_end) \
  if (c >= (range_begin) && c <= (range_end)) \
  { \
    return true; \
  }

  /* Mc */
#define LIT_UNICODE_RANGE_MC(range_begin, range_end) \
  if (c >= (range_begin) && c <= (range_end)) \
  { \
    return true; \
  }

#include "lit-unicode-ranges.inc.h"

  return false;
} /* lit_char_is_unicode_combining_mark */

/**
 * Check if specified character is a unicode digit
 *
 * Note:
 *      Unicode digit is a character, included into the following category:
 *       - Decimal number (Nd).
 *
 * See also:
 *          ECMA-262 v5, 7.6
 *
 * @return true - if specified character falls into the specified category,
 *         false - otherwise.
 */
bool
lit_char_is_unicode_digit (ecma_char_t c) /**< code unit */
{
  /* Nd */
#define LIT_UNICODE_RANGE_ND(range_begin, range_end) \
  if (c >= (range_begin) && c <= (range_end)) \
  { \
    return true; \
  }

#include "lit-unicode-ranges.inc.h"

  return false;
} /* lit_char_is_unicode_digit */

/**
 * Check if specified character is a unicode connector punctuation
 *
 * Note:
 *      Unicode connector punctuation is a character, included into the following category:
 *       - Connector punctuation (Pc).
 *
 * See also:
 *          ECMA-262 v5, 7.6
 *
 * @return true - if specified character falls into the specified category,
 *         false - otherwise.
 */
bool
lit_char_is_unicode_connector_punctuation (ecma_char_t c) /**< code unit */
{
  /* Pc */
#define LIT_UNICODE_RANGE_PC(range_begin, range_end) \
  if (c >= (range_begin) && c <= (range_end)) \
  { \
    return true; \
  }

#include "lit-unicode-ranges.inc.h"

  return false;
} /* lit_char_is_unicode_connector_punctuation */

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
  if ((c >= LIT_CHAR_ASCII_LOWERCASE_LETTERS_BEGIN && c <= LIT_CHAR_ASCII_LOWERCASE_LETTERS_END)
      || (c >= LIT_CHAR_ASCII_UPPERCASE_LETTERS_BEGIN && c <= LIT_CHAR_ASCII_UPPERCASE_LETTERS_END)
      || (c >= LIT_CHAR_ASCII_DIGITS_BEGIN && c <= LIT_CHAR_ASCII_DIGITS_END)
      || c == LIT_CHAR_UNDERSCORE)
  {
    return true;
  }

  return false;
} /* lit_char_is_word_char */

/**
 * Convert a hex character to an unsigned integer
 *
 * @return digit value, corresponding to the hex char
 */
uint32_t
lit_char_hex_to_int (ecma_char_t c) /**< code unit, corresponding to
                                     *    one of [0-9A-Fa-f] characters */
{
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
    JERRY_ASSERT (c >= LIT_CHAR_ASCII_UPPERCASE_LETTERS_HEX_BEGIN && c <= LIT_CHAR_ASCII_UPPERCASE_LETTERS_HEX_END);

    return (uint32_t) (c - LIT_CHAR_ASCII_UPPERCASE_LETTERS_HEX_BEGIN + 10);
  }
} /* lit_char_hex_to_int */

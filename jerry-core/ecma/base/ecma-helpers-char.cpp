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

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA characters
 * @{
 */

#include "ecma-globals.h"
#include "ecma-helpers.h"

/**
 * Check if specified character is the newline character
 *
 * @return true - if the character is "<LF>" character according to ECMA-262 v5, Table 3,
 *         false - otherwise.
 */
bool
ecma_char_is_new_line (ecma_char_t c) /**< character value */
{
  return (c == '\x0A');
} /* ecma_char_is_new_line */

/**
 * Check if specified character the carriage return character
 *
 * @return true - if the character is "<CR>" character according to ECMA-262 v5, Table 3,
 *         false - otherwise.
 */
bool
ecma_char_is_carriage_return (ecma_char_t c) /**< character value */
{
  return (c == '\x0D');
} /* ecma_char_is_carriage_return */

/**
 * Check if specified character is one of LineTerminator (ECMA-262 v5, Table 3) characters
 *
 * @return true - if the character is one of LineTerminator characters,
 *         false - otherwise.
 */
bool
ecma_char_is_line_terminator (ecma_char_t c) /**< character value */
{
  /* FIXME: Handle <LS> and <PS> (ECMA-262 v5, 7.3, Table 3) when Unicode would be supported */

  return (ecma_char_is_carriage_return (c)
          || ecma_char_is_new_line (c));
} /* ecma_char_is_line_terminator */

/**
 * Check if specified character is a word character (part of IsWordChar abstract operation)
 *
 * See also: ECMA-262 v5, 15.10.2.6 (IsWordChar)
 *
 * @return true - if the character is a word character
 *         false - otherwise.
 */
bool
ecma_char_is_word_char (ecma_char_t c) /**< character value */
{
  if ((c >= 'a' && c <= 'z')
      || (c >= 'A' && c <= 'Z')
      || (c >= '0' && c <= '9')
      || c == '_')
  {
    return true;
  }

  return false;
} /* ecma_char_is_word_char */

/**
 * Convert a hex character to an unsigned integer
 *
 * @return digit value, corresponding to the hex char
 */
uint32_t
ecma_char_hex_to_int (ecma_char_t hex) /**< [0-9A-Fa-f] character value */
{
  switch (hex)
  {
    case '0': return 0x0;
    case '1': return 0x1;
    case '2': return 0x2;
    case '3': return 0x3;
    case '4': return 0x4;
    case '5': return 0x5;
    case '6': return 0x6;
    case '7': return 0x7;
    case '8': return 0x8;
    case '9': return 0x9;
    case 'a':
    case 'A': return 0xA;
    case 'b':
    case 'B': return 0xB;
    case 'c':
    case 'C': return 0xC;
    case 'd':
    case 'D': return 0xD;
    case 'e':
    case 'E': return 0xE;
    case 'f':
    case 'F': return 0xF;
    default: JERRY_UNREACHABLE ();
  }
} /* ecma_char_hex_to_int */

/**
 * @}
 * @}
 */

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
  return (c == '\x0D');
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
  return (c == '\x0A');
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
 * @}
 * @}
 */

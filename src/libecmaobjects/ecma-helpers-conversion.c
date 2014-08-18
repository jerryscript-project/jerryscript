/* Copyright 2014 Samsung Electronics Co., Ltd.
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

/** \addtogroup ecma ---TODO---
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "jerry-libc.h"

/**
 * ECMA-defined conversion of string (zero-terminated) to Number.
 *
 * See also:
 *          ECMA-262 v5, 9.6
 *
 * @return ecma-number
 */
ecma_number_t
ecma_zt_string_to_number (const ecma_char_t *str_p) /**< zero-terminated string */
{
  FIXME (Implement according to ECMA);

  ecma_number_t ret_value = 0;
  while (*str_p != '\0')
  {
    if (*str_p >= '0' && *str_p <= '9')
    {
      ret_value *= 10;
      ret_value += (ecma_number_t) (*str_p - '0');
    }
    else
    {
      JERRY_UNIMPLEMENTED();
    }
  }

  return ret_value;
} /* ecma_zt_string_to_number */

/**
 * ECMA-defined conversion of UInt32 to String (zero-terminated).
 */
void
ecma_uint32_to_string (uint32_t value, /**< value to convert */
                       ecma_char_t *out_buffer_p, /**< buffer for zero-terminated string */
                       size_t buffer_size) /**< size of buffer */
{
  FIXME (Implement according to ECMA);

  ecma_char_t *p = (ecma_char_t*) ((uint8_t*) out_buffer_p + buffer_size) - 1;
  *p-- = '\0';

  do
  {
    JERRY_ASSERT (p != out_buffer_p);

    *p-- = (ecma_char_t) ("0123456789"[value % 10]);
    value /= 10;
  }
  while (value != 0);

  if (likely (p != out_buffer_p))
  {
    ssize_t bytes_to_move = ((uint8_t*) out_buffer_p + buffer_size) - (uint8_t*) p;
    __memmove (out_buffer_p, p, (size_t) bytes_to_move);
  }
} /* ecma_uint32_to_string */

/**
 * ECMA-defined conversion of UInt32 value to Number value
 *
 * @return number - result of conversion.
 */
ecma_number_t
ecma_uint32_to_number (uint32_t value) /**< unsigned 32-bit integer value */
{
  TODO(Implement according to ECMA);

  return (ecma_number_t) value;
} /* ecma_uint32_to_number */

/**
 * ECMA-defined conversion of Number value to Uint32 value
 *
 * See also:
 *          ECMA-262 v5, 9.6
 *
 * @return number - result of conversion.
 */
uint32_t
ecma_number_to_uint32 (ecma_number_t value) /**< unsigned 32-bit integer value */
{
  TODO(Implement according to ECMA);

  return (uint32_t) value;
} /* ecma_number_to_uint32 */

/**
 * ECMA-defined conversion of Number value to Int32 value
 *
 * See also:
 *          ECMA-262 v5, 9.5
 *
 * @return number - result of conversion.
 */
int32_t
ecma_number_to_int32 (ecma_number_t value) /**< unsigned 32-bit integer value */
{
  TODO(Implement according to ECMA);

  return (int32_t) value;
} /* ecma_number_to_int32 */

/**
 * @}
 * @}
 */

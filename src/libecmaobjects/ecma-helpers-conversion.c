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
#include "ecma-magic-strings.h"
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
 * ECMA-defined conversion of Int32 value to Number value
 *
 * @return number - result of conversion.
 */
ecma_number_t
ecma_int32_to_number (int32_t value) /**< signed 32-bit integer value */
{
  TODO(Implement according to ECMA);

  return (ecma_number_t) value;
} /* ecma_int32_to_number */

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
 * Convert ecma-number to zero-terminated string
 *
 * See also:
 *          ECMA-262 v5, 9.8.1
 *
 * @return length of zt-string
 */
ecma_length_t
ecma_number_to_zt_string (ecma_number_t num, /**< ecma-number */
                          ecma_char_t *buffer_p, /**< buffer for zt-string */
                          ssize_t buffer_size) /**< size of buffer */
{
  TODO (Support UTF-16);

  ecma_char_t digits_chars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
  ecma_char_t plus_char = '+';
  ecma_char_t minus_char = '-';
  ecma_char_t dot_char = '.';
  ecma_char_t e_char = 'e';
  ecma_char_t null_char = '\0';

  if (ecma_number_is_nan (num))
  {
    // 1.
    FIXME (/* Assert that buffer's size is enough */);
    __strncpy ((char*) buffer_p, (char*) ecma_get_magic_string_zt (ECMA_MAGIC_STRING_NAN), (size_t) buffer_size);
  }
  else
  {
    ecma_char_t *dst_p = buffer_p;

    if (ecma_number_is_zero (num))
    {
      // 2.
      *dst_p++ = digits_chars[0];
      *dst_p++ = null_char;

      JERRY_ASSERT ((uint8_t*)dst_p - (uint8_t*)buffer_p <= (ssize_t) buffer_size);
    }
    else if (ecma_number_is_negative (num))
    {
      // 3.
      *dst_p++ = minus_char;
      ssize_t new_buffer_size = (buffer_size - ((uint8_t*)dst_p - (uint8_t*)buffer_p));
      ecma_number_to_zt_string (ecma_number_negate (num), dst_p, new_buffer_size);
    }
    else if (ecma_number_is_infinity (num))
    {
      // 4.
      FIXME (/* Assert that buffer's size is enough */);
      __strncpy ((char*) buffer_p, (char*) ecma_get_magic_string_zt (ECMA_MAGIC_STRING_INFINITY), (size_t) buffer_size);
    }
    else
    {
      // 5.
      uint64_t fraction;
      int32_t exponent;
      int32_t dot_shift;
      dot_shift = ecma_number_get_fraction_and_exponent (num, &fraction, &exponent);

      FIXME (Decimal representation);

      uint64_t s = fraction;
      int32_t n = exponent + 1;
      int32_t k = dot_shift;

      JERRY_ASSERT (s != 0);
      while (!(s & 1))
      {
        s >>= 1;
        k--;
      }

      // 6.
      if (k <= n && n <= 21)
      {
        for (int32_t i = 0; i < k; i++)
        {
          uint64_t bit_mask = 1ul << (k - i - 1);
          *dst_p++ = digits_chars [(s & bit_mask) ? 1 : 0];
        }

        for (int32_t i = 0; i < n - k; i++)
        {
          *dst_p++ = digits_chars[0];
        }
      }
      else if (0 < n && n <= 21)
      {
        // 7.
        for (int32_t i = 0; i < n; i++)
        {
          uint64_t bit_mask = 1ul << (k - i - 1);
          *dst_p++ = digits_chars [(s & bit_mask) ? 1 : 0];
        }

        *dst_p++ = dot_char;

        for (int32_t i = n; i < k; i++)
        {
          uint64_t bit_mask = 1ul << (k - i - 1);
          *dst_p++ = digits_chars [(s & bit_mask) ? 1 : 0];
        }
      }
      else if (-6 <= n && n <= 0)
      {
        // 8.
        *dst_p++ = digits_chars[0];
        *dst_p++ = '.';

        for (int32_t i = 0; i < k; i++)
        {
          uint64_t bit_mask = 1ul << (k - i - 1);
          *dst_p++ = digits_chars [(s & bit_mask) ? 1 : 0];
        }
      }
      else
      {
        if (k == 1)
        {
          // 9.
          *dst_p++ = digits_chars [s ? 1 : 0];
        }
        else
        {
          // 10.
          uint64_t bit_mask = 1ul << (k - 1);
          *dst_p++ = digits_chars [(s & bit_mask) ? 1 : 0];
          *dst_p++ = dot_char;
          for (int32_t i = 1; i < k; i++)
          {
            bit_mask = 1ul << (k - i - 1);
            *dst_p++ = digits_chars [(s & bit_mask) ? 1 : 0];
          }
        }

        // 9., 10.
        *dst_p++ = e_char;
        *dst_p++ = (n >= 1) ? plus_char : minus_char;
        int32_t t = (n >= 1) ? (n - 1) : -(n - 1);

        if (t == 0)
        {
          *dst_p++ = digits_chars[0];
        }
        else
        {
          uint32_t t_bit = (1u << 31);

          while ((t & (int32_t) t_bit) == 0)
          {
            t_bit >>= 1;

            JERRY_ASSERT (t_bit != 0);
          }

          while (t_bit != 0)
          {
            *dst_p++ = digits_chars [(t & (int32_t) t_bit) ? 1 : 0];

            t_bit >>= 1;
          }
        }
      }

      *dst_p++ = null_char;

      JERRY_ASSERT ((uint8_t*)dst_p - (uint8_t*)buffer_p <= (ssize_t) buffer_size);
    }
  }

  ecma_length_t length = (ecma_length_t) __strlen ((char*) buffer_p);

  return length;
} /* ecma_number_to_zt_string */

/**
 * @}
 * @}
 */

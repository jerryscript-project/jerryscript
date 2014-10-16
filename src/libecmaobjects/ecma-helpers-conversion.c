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

/** \addtogroup ecma ECMA
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
 *          ECMA-262 v5, 9.3.1
 *
 * Warning:
 *         the conversion routine may be not precise for some cases
 *
 * @return ecma-number
 */
ecma_number_t
ecma_zt_string_to_number (const ecma_char_t *str_p) /**< zero-terminated string */
{
  TODO (Check conversion precision);

  const ecma_char_t dec_digits_range[10] = { '0', '9' };
  const ecma_char_t hex_lower_digits_range[10] = { 'a', 'f' };
  const ecma_char_t hex_upper_digits_range[10] = { 'A', 'F' };
  const ecma_char_t hex_x_chars[2] = { 'x', 'X' };
  const ecma_char_t white_space[2] = { ' ', '\n' };
  const ecma_char_t e_chars [2] = { 'e', 'E' };
  const ecma_char_t plus_char = '+';
  const ecma_char_t minus_char = '-';
  const ecma_char_t dot_char = '.';

  const ecma_char_t *begin_p = str_p;
  const ecma_char_t *end_p = begin_p;
  while (*end_p != ECMA_CHAR_NULL)
  {
    end_p++;
  }
  end_p--;

  while (begin_p <= end_p
         && (*begin_p == white_space[0]
             || *begin_p == white_space[1]))
  {
    begin_p++;
  }

  while (begin_p <= end_p
         && (*end_p == white_space[0]
             || *end_p == white_space[1]))
  {
    end_p--;
  }

  if (begin_p > end_p)
  {
    return ECMA_NUMBER_ZERO;
  }

  const ssize_t literal_len = end_p - begin_p + 1;

  bool is_hex_literal = false;

  if (literal_len > 2
      && *begin_p == dec_digits_range[0])
  {
    begin_p++;

    if (*begin_p == hex_x_chars[0]
        || *begin_p == hex_x_chars[1])
    {
      is_hex_literal = true;

      begin_p++;
    }
  }

  ecma_number_t num = 0;

  if (is_hex_literal)
  {
    for (const ecma_char_t* iter_p = begin_p;
         iter_p <= end_p;
         iter_p++)
    {
      int32_t digit_value;

      if (*iter_p >= dec_digits_range [0]
          && *iter_p <= dec_digits_range [1])
      {
        digit_value = (*iter_p - dec_digits_range[0]);
      }
      else if (*iter_p >= hex_lower_digits_range[0]
               && *iter_p <= hex_lower_digits_range[1])
      {
        digit_value = 10 + (*iter_p - hex_lower_digits_range[0]);
      }
      else if (*iter_p >= hex_upper_digits_range[0]
               && *iter_p <= hex_upper_digits_range[1])
      {
        digit_value = 10 + (*iter_p - hex_upper_digits_range[0]);
      }
      else
      {
        return ecma_number_make_nan ();
      }

      num = num * 16 + (ecma_number_t) digit_value;
    }

    return num;
  }

  bool sign = false; /* positive */

  if (*begin_p == plus_char)
  {
    begin_p++;
  }
  else if (*begin_p == minus_char)
  {
    sign = true; /* negative */

    begin_p++;
  }

  if (begin_p > end_p)
  {
    return ecma_number_make_nan ();
  }

  const ecma_char_t *infinity_zt_str_p = ecma_get_magic_string_zt (ECMA_MAGIC_STRING_INFINITY_UL);

  for (const ecma_char_t *iter_p = begin_p, *iter_infinity_p = infinity_zt_str_p;
       ;
       iter_infinity_p++, iter_p++)
  {
    if (*iter_p != *iter_infinity_p)
    {
      break;
    }

    if (iter_p == end_p)
    {
      return ecma_number_make_infinity (sign);
    }
  }

  while (begin_p <= end_p)
  {
    int32_t digit_value;

    if (*begin_p >= dec_digits_range [0]
        && *begin_p <= dec_digits_range [1])
    {
      digit_value = (*begin_p - dec_digits_range[0]);
    }
    else
    {
      break;
    }

    num = num * 10 + (ecma_number_t) digit_value;

    begin_p++;
  }

  if (begin_p > end_p)
  {
    if (sign)
    {
      return -num;
    }
    else
    {
      return num;
    }
  }

  int32_t e = 0;

  if (*begin_p == dot_char)
  {
    begin_p++;

    if (begin_p > end_p)
    {
      if (sign)
      {
        return -num;
      }
      else
      {
        return num;
      }
    }

    while (begin_p <= end_p)
    {
      int32_t digit_value;

      if (*begin_p >= dec_digits_range [0]
          && *begin_p <= dec_digits_range [1])
      {
        digit_value = (*begin_p - dec_digits_range[0]);
      }
      else
      {
        break;
      }

      num = num * 10 + (ecma_number_t) digit_value;
      e--;

      begin_p++;
    }
  }

  if (sign)
  {
    num = -num;
  }

  int e_in_lit = 0;
  bool e_in_lit_sign = false;

  if (*begin_p == e_chars[0]
      || *begin_p == e_chars[1])
  {
    begin_p++;

    if (*begin_p == plus_char)
    {
      begin_p++;
    }
    else if (*begin_p == minus_char)
    {
      e_in_lit_sign = true;
      begin_p++;
    }

    if (begin_p > end_p)
    {
      return ecma_number_make_nan ();
    }

    while (begin_p <= end_p)
    {
      int32_t digit_value;

      if (*begin_p >= dec_digits_range [0]
          && *begin_p <= dec_digits_range [1])
      {
        digit_value = (*begin_p - dec_digits_range[0]);
      }
      else
      {
        break;
      }

      e_in_lit = e_in_lit * 10 + digit_value;

      if (e_in_lit > 10000)
      {
        if (e_in_lit_sign)
        {
          return 0;
        }
        else
        {
          return ecma_number_make_infinity (sign);
        }
      }

      begin_p++;
    }
  }

  if (e_in_lit_sign)
  {
    e_in_lit -= e;
  }
  else
  {
    e_in_lit += e;
  }

  if (e_in_lit < 0)
  {
    JERRY_ASSERT (!e_in_lit_sign);
    e_in_lit_sign = true;
    e_in_lit = -e_in_lit;
  }

  ecma_number_t m = e_in_lit_sign ? 0.1f : 10.0f;

  while (e_in_lit)
  {
    if (e_in_lit % 2)
    {
      num *= m;
    }

    m *= m;
    e_in_lit /= 2;
  }

  if (begin_p > end_p)
  {
    return num;
  }
  else
  {
    return ecma_number_make_nan ();
  }
} /* ecma_zt_string_to_number */

/**
 * ECMA-defined conversion of UInt32 to String (zero-terminated).
 *
 * See also:
 *          ECMA-262 v5, 9.8.1
 *
 * @return number of bytes copied to buffer
 */
ssize_t
ecma_uint32_to_string (uint32_t value, /**< value to convert */
                       ecma_char_t *out_buffer_p, /**< buffer for zero-terminated string */
                       ssize_t buffer_size) /**< size of buffer */
{
  const ecma_char_t digits[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

  ecma_char_t *p = (ecma_char_t*) ((uint8_t*) out_buffer_p + buffer_size) - 1;
  *p-- = ECMA_CHAR_NULL;

  size_t bytes_copied = sizeof (ecma_char_t);

  do
  {
    JERRY_ASSERT (p >= out_buffer_p);

    *p-- = digits[value % 10];
    value /= 10;

    bytes_copied += sizeof (ecma_char_t);
  }
  while (value != 0);

  p++;

  JERRY_ASSERT (p >= out_buffer_p);

  if (likely (p != out_buffer_p))
  {
    ssize_t bytes_to_move = ((uint8_t*) out_buffer_p + buffer_size) - (uint8_t*) p;
    __memmove (out_buffer_p, p, (size_t) bytes_to_move);
  }

  return (ssize_t) bytes_copied;
} /* ecma_uint32_to_string */

/**
 * ECMA-defined conversion of UInt32 value to Number value
 *
 * @return number - result of conversion.
 */
ecma_number_t
ecma_uint32_to_number (uint32_t value) /**< unsigned 32-bit integer value */
{
  ecma_number_t num_value = (ecma_number_t) value;

  return num_value;
} /* ecma_uint32_to_number */

/**
 * ECMA-defined conversion of Int32 value to Number value
 *
 * @return number - result of conversion.
 */
ecma_number_t
ecma_int32_to_number (int32_t value) /**< signed 32-bit integer value */
{
  ecma_number_t num_value = (ecma_number_t) value;

  return num_value;
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
  if (ecma_number_is_nan (value)
      || ecma_number_is_zero (value)
      || ecma_number_is_infinity (value))
  {
    return 0;
  }

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
  if (ecma_number_is_nan (value)
      || ecma_number_is_zero (value)
      || ecma_number_is_infinity (value))
  {
    return 0;
  }

  return (int32_t) value;
} /* ecma_number_to_int32 */

/**
 * Convert ecma-number to zero-terminated string
 *
 * See also:
 *          ECMA-262 v5, 9.8.1
 *
 * Warning:
 *         the conversion is not precise for all cases
 *         For example, 12345.123f converts to "12345.12209".
 *
 * @return length of zt-string
 */
ecma_length_t
ecma_number_to_zt_string (ecma_number_t num, /**< ecma-number */
                          ecma_char_t *buffer_p, /**< buffer for zt-string */
                          ssize_t buffer_size) /**< size of buffer */
{
  FIXME (Conversion precision);

  const ecma_char_t digits[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
  const ecma_char_t e_chars [2] = { 'e', 'E' };
  const ecma_char_t plus_char = '+';
  const ecma_char_t minus_char = '-';
  const ecma_char_t dot_char = '.';

  if (ecma_number_is_nan (num))
  {
    // 1.
    ecma_copy_zt_string_to_buffer (ecma_get_magic_string_zt (ECMA_MAGIC_STRING_NAN),
                                   buffer_p,
                                   buffer_size);
  }
  else
  {
    ecma_char_t *dst_p = buffer_p;

    if (ecma_number_is_zero (num))
    {
      // 2.
      *dst_p++ = digits[0];
      *dst_p++ = ECMA_CHAR_NULL;

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
      ecma_copy_zt_string_to_buffer (ecma_get_magic_string_zt (ECMA_MAGIC_STRING_INFINITY_UL),
                                     buffer_p,
                                     buffer_size);
    }
    else
    {
      // 5.
#ifdef CONFIG_ECMA_NUMBER_FLOAT32
#define LL_T uint32_t
#define LL_MAX_DIGITS 10
#elif defined (CONFIG_ECMA_NUMBER_FLOAT64)
#define LL_T uint64_t
#define LL_MAX_DIGITS 18
#else /* !CONFIG_ECMA_NUMBER_FLOAT32 && !CONFIG_ECMA_NUMBER_FLOAT64 */
# error "!CONFIG_ECMA_NUMBER_FLOAT32 && !CONFIG_ECMA_NUMBER_FLOAT64"
#endif /* !CONFIG_ECMA_NUMBER_FLOAT32 && !CONFIG_ECMA_NUMBER_FLOAT64 */

      uint32_t num_uint32 = ecma_number_to_uint32 (num);
      if (ecma_uint32_to_number (num_uint32) == num)
      {
        ecma_uint32_to_string (num_uint32, dst_p, buffer_size);
      }
      else
      {
        uint64_t fraction_uint64;
        LL_T fraction;
        int32_t exponent;
        int32_t dot_shift;
        int32_t decimal_exp = 0;

        dot_shift = ecma_number_get_fraction_and_exponent (num, &fraction_uint64, &exponent);

        fraction = (LL_T) fraction_uint64;
        JERRY_ASSERT (fraction == fraction_uint64);

        if (exponent != 0)
        {
          ecma_number_t t = 1.0f;
          bool do_divide;

          if (exponent < 0)
          {
            do_divide = true;

            while (exponent <= 0)
            {
              t *= 2.0f;
              exponent++;

              if (t >= 10.0f)
              {
                t /= 10.0f;
                decimal_exp--;
              }

              JERRY_ASSERT (t < 10.0f);
            }

            while (t > 1.0f)
            {
              exponent--;
              t /= 2.0f;
            }
          }
          else
          {
            do_divide = false;

            while (exponent >= 0)
            {
              t *= 2.0f;
              exponent--;

              if (t >= 10.0f)
              {
                t /= 10.0f;
                decimal_exp++;
              }

              JERRY_ASSERT (t < 10.0f);
            }

            while (t > 2.0f)
            {
              exponent++;
              t /= 2.0f;
            }
          }

          if (do_divide)
          {
            fraction = (LL_T) ((ecma_number_t) fraction / t);
          }
          else
          {
            fraction = (LL_T) ((ecma_number_t) fraction * t);
          }
        }

        LL_T s;
        int32_t n;
        int32_t k;

        if (exponent > 0)
        {
          fraction <<= exponent;
        }
        else
        {
          fraction >>= -exponent;
        }

        const int32_t int_part_shift = dot_shift;
        const LL_T frac_part_mask = ((((LL_T)1) << int_part_shift) - 1);

        LL_T int_part = fraction >> int_part_shift;
        LL_T frac_part = fraction & frac_part_mask;

        s = int_part;
        k = 1;
        n = decimal_exp + 1;

        JERRY_ASSERT (int_part < 10);

        while (k < LL_MAX_DIGITS
               && frac_part != 0)
        {
          frac_part *= 10;

          LL_T new_frac_part = frac_part & frac_part_mask;
          LL_T digit = (frac_part - new_frac_part) >> int_part_shift;
          s = s * 10 + digit;
          k++;
          frac_part = new_frac_part;
        }

        // 6.
        if (k <= n && n <= 21)
        {
          dst_p += n;
          JERRY_ASSERT ((ssize_t) sizeof (ecma_char_t) * ((dst_p - buffer_p) + 1) <= buffer_size);

          *dst_p = ECMA_CHAR_NULL;

          for (int32_t i = 0; i < n - k; i++)
          {
            *--dst_p = digits [0];
          }

          for (int32_t i = 0; i < k; i++)
          {
            *--dst_p = digits [s % 10];
            s /= 10;
          }
        }
        else if (0 < n && n <= 21)
        {
          // 7.
          dst_p += k + 1;
          JERRY_ASSERT ((ssize_t) sizeof (ecma_char_t) * ((dst_p - buffer_p) + 1) <= buffer_size);

          *dst_p = ECMA_CHAR_NULL;

          for (int32_t i = 0; i < k - n; i++)
          {
            *--dst_p = digits [s % 10];
            s /= 10;
          }

          *--dst_p = dot_char;

          for (int32_t i = 0; i < n; i++)
          {
            *--dst_p = digits [s % 10];
            s /= 10;
          }
        }
        else if (-6 <= n && n <= 0)
        {
          // 8.
          dst_p += k - n + 1 + 1;
          JERRY_ASSERT ((ssize_t) sizeof (ecma_char_t) * ((dst_p - buffer_p) + 1) <= buffer_size);

          *dst_p = ECMA_CHAR_NULL;

          for (int32_t i = 0; i < k; i++)
          {
            *--dst_p = digits [s % 10];
            s /= 10;
          }

          for (int32_t i = 0; i < -n; i++)
          {
            *--dst_p = digits [0];
          }

          *--dst_p = dot_char;
          *--dst_p = digits[0];
        }
        else
        {
          if (k == 1)
          {
            // 9.
            JERRY_ASSERT ((ssize_t) sizeof (ecma_char_t) <= buffer_size);

            *dst_p++ = digits [s % 10];
            s /= 10;
          }
          else
          {
            // 10.
            dst_p += k + 1;
            JERRY_ASSERT ((ssize_t) sizeof (ecma_char_t) * (dst_p - buffer_p) <= buffer_size);

            for (int32_t i = 0; i < k - 1; i++)
            {
              *--dst_p = digits [s % 10];
              s /= 10;
            }

            *--dst_p = dot_char;
            *--dst_p = digits[s % 10];
            s /= 10;

            dst_p += k + 1;
          }

          // 9., 10.
          JERRY_ASSERT ((ssize_t) sizeof (ecma_char_t) * (dst_p - buffer_p + 2) <= buffer_size);
          *dst_p++ = e_chars[0];
          *dst_p++ = (n >= 1) ? plus_char : minus_char;
          int32_t t = (n >= 1) ? (n - 1) : -(n - 1);

          if (t == 0)
          {
            JERRY_ASSERT ((ssize_t) sizeof (ecma_char_t) * (dst_p - buffer_p + 1) <= buffer_size);
            *dst_p++ = digits [0];
          }
          else
          {
            int32_t t_mod = 1000000000u;

            while ((t / t_mod) == 0)
            {
              t_mod /= 10;

              JERRY_ASSERT (t_mod != 0);
            }

            while (t_mod != 0)
            {
              JERRY_ASSERT ((ssize_t) sizeof (ecma_char_t) * (dst_p - buffer_p + 1) <= buffer_size);
              *dst_p++ = digits [t / t_mod];

              t -= (t / t_mod) * t_mod;
              t_mod /= 10;
            }
          }

          JERRY_ASSERT ((ssize_t) sizeof (ecma_char_t) * (dst_p - buffer_p + 1) <= buffer_size);
          *dst_p++ = ECMA_CHAR_NULL;
        }

        JERRY_ASSERT (s == 0);
      }
    }
  }

  ecma_length_t length = ecma_zt_string_length (buffer_p);

  return length;
} /* ecma_number_to_zt_string */

/**
 * @}
 * @}
 */

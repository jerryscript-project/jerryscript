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

#include <math.h>

#include "ecma-globals.h"
#include "ecma-helpers-number.h"
#include "ecma-helpers.h"

#include "jrt-libc-includes.h"
#include "lit-char-helpers.h"
#include "lit-magic-strings.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

#if JERRY_NUMBER_TYPE_FLOAT64

/**
 * \addtogroup ecmahelpersbigintegers Helpers for operations intermediate 128-bit integers
 * @{
 */

/**
 * 128-bit integer type
 */
typedef struct
{
  uint64_t hi; /**< high 64 bits */
  uint64_t lo; /**< low 64 bits */
} ecma_uint128_t;

/**
 * Round high part of 128-bit integer to uint64_t
 *
 * @return rounded high to uint64_t
 */
static uint64_t
ecma_round_high_to_uint64 (ecma_uint128_t *num_p) /**< 128-bit unsigned integer */
{
  uint64_t masked_lo = num_p->lo & ~(1ULL << 63u);
  uint64_t masked_hi = num_p->hi & 0x1;

  if ((num_p->lo >> 63u != 0) && (masked_lo > 0 || masked_hi != 0))
  {
    return (num_p->hi + 1);
  }

  return num_p->hi;
} /* ecma_round_high_to_uint64 */

/**
 * Left shift 128-bit integer by max 63 bits.
 */
static void JERRY_ATTR_ALWAYS_INLINE
ecma_uint128_shift_left (ecma_uint128_t *num_p, /**< 128-bit unsigned integer */
                         int32_t shift) /**< left shift count */
{
  num_p->hi = (num_p->hi << shift) | (num_p->lo >> (64 - shift));
  num_p->lo <<= shift;
} /* ecma_uint128_shift_left */

/**
 * Right shift 128-bit integer by max 63 bits.
 */
static void JERRY_ATTR_ALWAYS_INLINE
ecma_uint128_shift_right (ecma_uint128_t *num_p, /**< 128-bit unsigned integer */
                          int32_t shift) /**< right shift count */
{
  num_p->lo = (num_p->lo >> shift) | (num_p->hi << (64 - shift));
  num_p->hi >>= shift;
} /* ecma_uint128_shift_right */

/**
 * Add two 128-bit integer values and assign the result to the left one.
 */
static void
ecma_uint128_add (ecma_uint128_t *left_p, /**< left 128-bit unsigned integer */
                  ecma_uint128_t *right_p) /**< right 128-bit unsigned integer */
{
  left_p->hi += right_p->hi;
  left_p->lo += right_p->lo;

  if (left_p->lo < right_p->lo)
  {
    left_p->hi++;
  }
} /* ecma_uint128_add */

/**
 * Multiply 128-bit integer by 10
 */
static void
ecma_uint128_mul10 (ecma_uint128_t *num_p) /**< 128-bit unsigned integer */
{
  ecma_uint128_shift_left (num_p, 1u);

  ecma_uint128_t tmp = { .hi = num_p->hi, .lo = num_p->lo };
  ecma_uint128_shift_left (&tmp, 2u);

  ecma_uint128_add (num_p, &tmp);
} /* ecma_uint128_mul10 */

/**
 * Divide 128-bit integer by 10
 *
 * N = N3 *2^96  + N2 *2^64  + N1 *2^32  + N0 *2^0     // 128-bit dividend
 * T = T3 *2^-32 + T2 *2^-64 + T1 *2^-96 + T0 *2^-128  // 128-bit divisor reciprocal, 1/10 * 2^-128
 *
 * N * T    = N3*T3 *2^64 + N2*T3 *2^32 + N1*T3 *2^0 + N0*T3 *2^-32
 *          +               N3*T2 *2^32 + N2*T2 *2^0 + N1*T2 *2^-32 + N0*T2 *2^-64
 *          +                             N3*T1 *2^0 + N2*T1 *2^-32 + N1*T1 *2^-64 + N0*T1 *2^-96
 *          +                                          N3*T0 *2^-32 + N2*T0 *2^-64 + N1*T0 *2^-96 + N0*T0 *2^-128
 *
 *  Q3=carry  Q2=^+carry    Q1=^+carry    Q0=^+carry   fraction=^...
 *
 * Q = Q3 *2^96  + Q2 *2^64  + Q1 *2^32  + Q0 *2^0     // 128-bit quotient
 */
static void
ecma_uint128_div10 (ecma_uint128_t *num_p) /**< 128-bit unsigned integer */
{
  /* estimation of reciprocal of 10, 128 bits right of the binary point (T1 == T2) */
  const uint64_t tenth_l = 0x9999999aul;
  const uint64_t tenth_m = 0x99999999ul;
  const uint64_t tenth_h = 0x19999999ul;

  const uint64_t l0 = ((uint32_t) num_p->lo) * tenth_l;
  const uint64_t l1 = (num_p->lo >> 32u) * tenth_l;
  const uint64_t l2 = ((uint32_t) num_p->hi) * tenth_l;
  const uint64_t l3 = (num_p->hi >> 32u) * tenth_l;
  const uint64_t m0 = ((uint32_t) num_p->lo) * tenth_m;
  const uint64_t m1 = (num_p->lo >> 32u) * tenth_m;
  const uint64_t m2 = ((uint32_t) num_p->hi) * tenth_m;
  const uint64_t m3 = (num_p->hi >> 32u) * tenth_m;
  const uint64_t h0 = ((uint32_t) num_p->lo) * tenth_h;
  const uint64_t h1 = (num_p->lo >> 32u) * tenth_h;
  const uint64_t h2 = ((uint32_t) num_p->hi) * tenth_h;
  const uint64_t h3 = (num_p->hi >> 32u) * tenth_h;

  uint64_t q0 = l0 >> 32u;
  q0 += (uint32_t) l1;
  q0 += (uint32_t) m0;

  q0 >>= 32u;
  q0 += l1 >> 32u;
  q0 += m0 >> 32u;
  q0 += (uint32_t) l2;
  q0 += (uint32_t) m1;
  q0 += (uint32_t) m0;

  q0 >>= 32u;
  q0 += l2 >> 32u;
  q0 += m1 >> 32u;
  q0 += m0 >> 32u;
  q0 += (uint32_t) l3;
  q0 += (uint32_t) m2;
  q0 += (uint32_t) m1;
  q0 += (uint32_t) h0;

  q0 >>= 32u;
  q0 += l3 >> 32u;
  q0 += m2 >> 32u;
  q0 += m1 >> 32u;
  q0 += h0 >> 32u;
  q0 += (uint32_t) m3;
  q0 += (uint32_t) m2;
  q0 += (uint32_t) h1;

  uint64_t q1 = q0 >> 32u;
  q1 += m3 >> 32u;
  q1 += m2 >> 32u;
  q1 += h1 >> 32u;
  q1 += (uint32_t) m3;
  q1 += (uint32_t) h2;

  uint64_t q32 = q1 >> 32u;
  q32 += m3 >> 32u;
  q32 += h2 >> 32u;
  q32 += h3;

  num_p->lo = (q1 << 32u) | ((uint32_t) q0);
  num_p->hi = q32;
} /* ecma_uint128_div10 */

/**
 * Count leading zeros in a 64-bit integer. The behaviour is undefined for 0.
 *
 * @return number of leading zeros.
 */
inline static int JERRY_ATTR_CONST
ecma_uint64_clz (uint64_t n) /**< integer to count leading zeros in */
{
#if defined(__GNUC__) || defined(__clang__)
  return __builtin_clzll (n);
#else /* !defined (__GNUC__) && !defined (__clang__) */
  JERRY_ASSERT (n != 0);

  int cnt = 0;
  uint64_t one = 0x8000000000000000ull;

  while ((n & one) == 0)
  {
    cnt++;
    one >>= 1;
  }

  return cnt;
#endif /* !defined (__GNUC__) && !defined (__clang__) */
} /* ecma_uint64_clz */

/**
 * Count leading zeros in the top 4 bits of a 64-bit integer.
 *
 * @return number of leading zeros in top 4 bits.
 */
inline static int JERRY_ATTR_ALWAYS_INLINE JERRY_ATTR_CONST
ecma_uint64_clz_top4 (uint64_t n) /**< integer to count leading zeros in */
{
  static const uint8_t ecma_uint4_clz[] = { 4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
  return ecma_uint4_clz[n >> 60];
} /* ecma_uint64_clz */

/**
 * Shift required to clear 4 bits of a 64-bit integer.
 *
 * @return 0-4
 */
inline static int JERRY_ATTR_ALWAYS_INLINE JERRY_ATTR_CONST
ecma_uint64_normalize_shift (uint64_t n) /**< integer to count leading zeros in */
{
  static const uint8_t ecma_uint4_shift[] = { 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 };

  return ecma_uint4_shift[n >> 60];
} /* ecma_uint64_normalize_shift */

/**
 * @}
 */

/**
 * Number.MAX_VALUE exponent part when using 64 bit float representation.
 */
#define NUMBER_MAX_DECIMAL_EXPONENT 308
/**
 * Number.MIN_VALUE exponent part when using 64 bit float representation.
 */
#define NUMBER_MIN_DECIMAL_EXPONENT -324

#elif !JERRY_NUMBER_TYPE_FLOAT64

/**
 * Number.MAX_VALUE exponent part when using 32 bit float representation.
 */
#define NUMBER_MAX_DECIMAL_EXPONENT 38
/**
 * Number.MIN_VALUE exponent part when using 32 bit float representation.
 */
#define NUMBER_MIN_DECIMAL_EXPONENT -45

#endif /* JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Value of epsilon
 */
#define EPSILON 0.0000001

/**
 * ECMA-defined conversion from string to number for different radixes (2, 8, 16).
 *
 * See also:
 *          ECMA-262 v5 9.3.1
 *          ECMA-262 v6 7.1.3.1
 *
 * @return NaN - if the conversion fails
 *         converted number - otherwise
 */
ecma_number_t
ecma_utf8_string_to_number_by_radix (const lit_utf8_byte_t *str_p, /**< utf-8 string */
                                     const lit_utf8_size_t string_size, /**< end of utf-8 string  */
                                     uint32_t radix, /**< radix */
                                     uint32_t options) /**< option flags */
{
  JERRY_ASSERT (radix == 2 || radix == 8 || radix == 16);
  JERRY_ASSERT (*str_p == LIT_CHAR_0);

  const lit_utf8_byte_t *end_p = str_p + string_size;

  /* Skip leading zero */
  str_p++;

  if (radix != 8 || LEXER_TO_ASCII_LOWERCASE (*str_p) == LIT_CHAR_LOWERCASE_O)
  {
    /* Skip radix specifier */
    str_p++;
  }

  ecma_number_t num = ECMA_NUMBER_ZERO;

  while (str_p < end_p)
  {
    lit_utf8_byte_t digit = *str_p++;

    if (digit == LIT_CHAR_UNDERSCORE && (options & ECMA_CONVERSION_ALLOW_UNDERSCORE))
    {
      continue;
    }

    if (!lit_char_is_hex_digit (digit))
    {
      return ecma_number_make_nan ();
    }

    uint32_t value = lit_char_hex_to_int (digit);

    if (value >= radix)
    {
      return ecma_number_make_nan ();
    }

    num = num * radix + value;
  }

  return num;
} /* ecma_utf8_string_to_number_by_radix */

/**
 * ECMA-defined conversion of string to Number.
 *
 * See also:
 *          ECMA-262 v5, 9.3.1
 *
 * @return NaN - if the conversion fails
 *         converted number - otherwise
 */
ecma_number_t
ecma_utf8_string_to_number (const lit_utf8_byte_t *str_p, /**< utf-8 string */
                            lit_utf8_size_t str_size, /**< string size */
                            uint32_t options) /**< allowing underscore option bit */
{
  ecma_string_trim_helper (&str_p, &str_size);
  const lit_utf8_byte_t *end_p = str_p + str_size;

  if (str_size == 0)
  {
    return ECMA_NUMBER_ZERO;
  }

  bool sign = false;

  if (str_p + 2 < end_p && str_p[0] == LIT_CHAR_0)
  {
    uint8_t radix = lit_char_to_radix (str_p[1]);

    if (radix != 10)
    {
      return ecma_utf8_string_to_number_by_radix (str_p, str_size, radix, options);
    }
  }

  if (*str_p == LIT_CHAR_PLUS)
  {
    str_p++;
  }
  else if (*str_p == LIT_CHAR_MINUS)
  {
    sign = true;
    str_p++;
  }

  /* Check if string is equal to "Infinity". */
  const lit_utf8_byte_t *infinity_str_p = lit_get_magic_string_utf8 (LIT_MAGIC_STRING_INFINITY_UL);
  const lit_utf8_size_t infinity_length = lit_get_magic_string_size (LIT_MAGIC_STRING_INFINITY_UL);

  if ((lit_utf8_size_t) (end_p - str_p) == infinity_length && memcmp (infinity_str_p, str_p, infinity_length) == 0)
  {
    return ecma_number_make_infinity (sign);
  }

  uint64_t significand = 0;
  uint32_t digit_count = 0;
  int32_t decimal_exponent = 0;
  bool has_significand = false;

  /* Parsing integer part */
  while (str_p < end_p)
  {
    if (*str_p == LIT_CHAR_UNDERSCORE && (options & ECMA_CONVERSION_ALLOW_UNDERSCORE))
    {
      str_p++;
      continue;
    }

    if (!lit_char_is_decimal_digit (*str_p))
    {
      break;
    }

    has_significand = true;
    uint32_t digit_value = (uint32_t) (*str_p++ - LIT_CHAR_0);

    if (digit_count == 0 && digit_value == 0)
    {
      /* Leading zeros are omitted. */
      continue;
    }

    if (digit_count >= ECMA_NUMBER_MAX_DIGITS)
    {
      decimal_exponent++;
      continue;
    }

    significand = significand * 10 + digit_value;
    digit_count++;
  }

  /* Parse fraction part */
  if (str_p < end_p && *str_p == LIT_CHAR_DOT)
  {
    str_p++;

    while (str_p < end_p)
    {
      if (*str_p == LIT_CHAR_UNDERSCORE && (options & ECMA_CONVERSION_ALLOW_UNDERSCORE))
      {
        str_p++;
        continue;
      }

      if (!lit_char_is_decimal_digit (*str_p))
      {
        break;
      }

      has_significand = true;
      uint32_t digit_value = (uint32_t) (*str_p++ - LIT_CHAR_0);

      if (digit_count == 0 && digit_value == 0)
      {
        /* Leading zeros are omitted. */
        decimal_exponent--;
        continue;
      }

      if (digit_count < ECMA_NUMBER_MAX_DIGITS)
      {
        significand = significand * 10 + digit_value;
        digit_count++;
        decimal_exponent--;
      }
    }
  }

  /* Parsing exponent */
  if (str_p < end_p && LEXER_TO_ASCII_LOWERCASE (*str_p) == LIT_CHAR_LOWERCASE_E)
  {
    str_p++;

    int32_t exponent = 0;
    int32_t exponent_sign = 1;

    if (str_p >= end_p)
    {
      return ecma_number_make_nan ();
    }

    if (*str_p == LIT_CHAR_PLUS)
    {
      str_p++;
    }
    else if (*str_p == LIT_CHAR_MINUS)
    {
      exponent_sign = -1;
      str_p++;
    }

    if (str_p >= end_p || !lit_char_is_decimal_digit (*str_p))
    {
      return ecma_number_make_nan ();
    }

    while (str_p < end_p)
    {
      if (*str_p == LIT_CHAR_UNDERSCORE && (options & ECMA_CONVERSION_ALLOW_UNDERSCORE))
      {
        str_p++;
        continue;
      }

      if (!lit_char_is_decimal_digit (*str_p))
      {
        break;
      }

      int32_t digit_value = (*str_p++ - LIT_CHAR_0);
      exponent = exponent * 10 + digit_value;

      if (exponent_sign * exponent > NUMBER_MAX_DECIMAL_EXPONENT)
      {
        return ecma_number_make_infinity (sign);
      }

      if (exponent_sign * exponent < NUMBER_MIN_DECIMAL_EXPONENT)
      {
        return sign ? -ECMA_NUMBER_ZERO : ECMA_NUMBER_ZERO;
      }
    }

    decimal_exponent += exponent_sign * exponent;
  }

  if (!has_significand || str_p < end_p)
  {
    return ecma_number_make_nan ();
  }

  if (significand == 0)
  {
    return sign ? -ECMA_NUMBER_ZERO : ECMA_NUMBER_ZERO;
  }

#if JERRY_NUMBER_TYPE_FLOAT64
  /*
   * 128-bit mantissa storage
   *
   * Normalized: |4 bits zero|124-bit mantissa with highest bit set to 1|
   */
  ecma_uint128_t significand_uint128 = { .hi = significand, .lo = 0 };

  /* Normalizing mantissa */
  int shift = 4 - ecma_uint64_clz (significand_uint128.hi);

  if (shift < 0)
  {
    ecma_uint128_shift_left (&significand_uint128, -shift);
  }
  else
  {
    ecma_uint128_shift_right (&significand_uint128, shift);
  }

  int32_t binary_exponent = ECMA_NUMBER_FRACTION_WIDTH + shift;

  while (decimal_exponent > 0)
  {
    JERRY_ASSERT (ecma_uint64_clz (significand_uint128.hi) == 4);

    ecma_uint128_mul10 (&significand_uint128);
    decimal_exponent--;

    /* Re-normalizing mantissa */
    shift = ecma_uint64_normalize_shift (significand_uint128.hi);
    JERRY_ASSERT (shift >= 0 && shift <= 4);

    ecma_uint128_shift_right (&significand_uint128, shift);
    binary_exponent += shift;
  }

  while (decimal_exponent < 0)
  {
    /* Denormalizing mantissa, moving highest 1 to bit 127 */
    JERRY_ASSERT (ecma_uint64_clz (significand_uint128.hi) <= 4);
    shift = ecma_uint64_clz_top4 (significand_uint128.hi);
    JERRY_ASSERT (shift >= 0 && shift <= 4);

    ecma_uint128_shift_left (&significand_uint128, shift);
    binary_exponent -= shift;

    ecma_uint128_div10 (&significand_uint128);
    decimal_exponent++;
  }

  /*
   * Preparing mantissa for conversion to 52-bit representation, converting it to:
   *
   * |11 zero bits|1|116 mantissa bits|
   */
  JERRY_ASSERT (ecma_uint64_clz (significand_uint128.hi) <= 4);
  shift = 11 - ecma_uint64_clz_top4 (significand_uint128.hi);
  ecma_uint128_shift_right (&significand_uint128, shift);
  binary_exponent += shift;

  JERRY_ASSERT (ecma_uint64_clz (significand_uint128.hi) == 11);

  binary_exponent += ECMA_NUMBER_EXPONENT_BIAS;

  /* Handle denormal numbers */
  if (binary_exponent < 1)
  {
    ecma_uint128_shift_right (&significand_uint128, -binary_exponent + 1);
    binary_exponent = 0;
  }

  significand = ecma_round_high_to_uint64 (&significand_uint128);

  if (significand >= 1ull << (ECMA_NUMBER_FRACTION_WIDTH + 1))
  {
    /* Rounding carried over to the most significant bit, re-normalize.
     * No need to shift mantissa right, as the low 52 bits will be 0 regardless. */
    binary_exponent++;
  }

  if (binary_exponent >= ((1 << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1))
  {
    return ecma_number_make_infinity (sign);
  }

  /* Mask low 52 bits. */
  significand &= ((1ull << ECMA_NUMBER_FRACTION_WIDTH) - 1);

  JERRY_ASSERT (binary_exponent < (1 << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1);
  JERRY_ASSERT (significand < (1ull << ECMA_NUMBER_FRACTION_WIDTH));

  return ecma_number_create (sign, (uint32_t) binary_exponent, significand);
#elif !JERRY_NUMBER_TYPE_FLOAT64
  /* Less precise conversion */
  ecma_number_t num = (ecma_number_t) (uint32_t) fraction_uint64;

  ecma_number_t m = e_sign ? (ecma_number_t) 0.1 : (ecma_number_t) 10.0;

  while (e)
  {
    if (e % 2)
    {
      num *= m;
    }

    m *= m;
    e /= 2;
  }

  return num;
#endif /* JERRY_NUMBER_TYPE_FLOAT64 */
} /* ecma_utf8_string_to_number */

/**
 * ECMA-defined conversion of UInt32 to String (zero-terminated).
 *
 * See also:
 *          ECMA-262 v5, 9.8.1
 *
 * @return number of bytes copied to buffer
 */
lit_utf8_size_t
ecma_uint32_to_utf8_string (uint32_t value, /**< value to convert */
                            lit_utf8_byte_t *out_buffer_p, /**< buffer for string */
                            lit_utf8_size_t buffer_size) /**< size of buffer */
{
  lit_utf8_byte_t *buf_p = out_buffer_p + buffer_size;

  do
  {
    JERRY_ASSERT (buf_p >= out_buffer_p);

    buf_p--;
    *buf_p = (lit_utf8_byte_t) ((value % 10) + LIT_CHAR_0);
    value /= 10;
  } while (value != 0);

  JERRY_ASSERT (buf_p >= out_buffer_p);

  lit_utf8_size_t bytes_copied = (lit_utf8_size_t) (out_buffer_p + buffer_size - buf_p);

  if (JERRY_LIKELY (buf_p != out_buffer_p))
  {
    memmove (out_buffer_p, buf_p, bytes_copied);
  }

  return bytes_copied;
} /* ecma_uint32_to_utf8_string */

/**
 * ECMA-defined conversion of Number value to UInt32 value
 *
 * See also:
 *          ECMA-262 v5, 9.6
 *
 * @return 32-bit unsigned integer - result of conversion.
 */
uint32_t
ecma_number_to_uint32 (ecma_number_t num) /**< ecma-number */
{
  if (JERRY_UNLIKELY (ecma_number_is_zero (num) || !ecma_number_is_finite (num)))
  {
    return 0;
  }

  const bool sign = ecma_number_is_negative (num);
  const ecma_number_t abs_num = sign ? -num : num;

  /* 2 ^ 32 */
  const uint64_t uint64_2_pow_32 = (1ull << 32);

  const ecma_number_t num_2_pow_32 = (float) uint64_2_pow_32;

  ecma_number_t num_in_uint32_range;

  if (abs_num >= num_2_pow_32)
  {
    num_in_uint32_range = ecma_number_remainder (abs_num, num_2_pow_32);
  }
  else
  {
    num_in_uint32_range = abs_num;
  }

  /* Check that the floating point value can be represented with uint32_t. */
  JERRY_ASSERT (num_in_uint32_range < uint64_2_pow_32);
  uint32_t uint32_num = (uint32_t) num_in_uint32_range;

  const uint32_t ret = sign ? -uint32_num : uint32_num;

#ifndef JERRY_NDEBUG
  if (sign && uint32_num != 0)
  {
    JERRY_ASSERT (ret == uint64_2_pow_32 - uint32_num);
  }
  else
  {
    JERRY_ASSERT (ret == uint32_num);
  }
#endif /* !JERRY_NDEBUG */

  return ret;
} /* ecma_number_to_uint32 */

/**
 * ECMA-defined conversion of Number value to Int32 value
 *
 * See also:
 *          ECMA-262 v5, 9.5
 *
 * @return 32-bit signed integer - result of conversion.
 */
int32_t
ecma_number_to_int32 (ecma_number_t num) /**< ecma-number */
{
  uint32_t uint32_num = ecma_number_to_uint32 (num);

  /* 2 ^ 32 */
  const int64_t int64_2_pow_32 = (1ll << 32);

  /* 2 ^ 31 */
  const uint32_t uint32_2_pow_31 = (1ull << 31);

  int32_t ret;

  if (uint32_num >= uint32_2_pow_31)
  {
    ret = (int32_t) (uint32_num - int64_2_pow_32);
  }
  else
  {
    ret = (int32_t) uint32_num;
  }

#ifndef JERRY_NDEBUG
  int64_t int64_num = uint32_num;

  JERRY_ASSERT (int64_num >= 0);

  if (int64_num >= uint32_2_pow_31)
  {
    JERRY_ASSERT (ret == int64_num - int64_2_pow_32);
  }
  else
  {
    JERRY_ASSERT (ret == int64_num);
  }
#endif /* !JERRY_NDEBUG */

  return ret;
} /* ecma_number_to_int32 */

/**
 * Perform conversion of ecma-number to decimal representation with decimal exponent.
 *
 * Note:
 *      The calculated values correspond to s, n, k parameters in ECMA-262 v5, 9.8.1, item 5:
 *         - parameter out_digits_p corresponds to s, the digits of the number;
 *         - parameter out_decimal_exp_p corresponds to n, the decimal exponent;
 *         - return value corresponds to k, the number of digits.
 *
 * @return the number of digits
 */
lit_utf8_size_t
ecma_number_to_decimal (ecma_number_t num, /**< ecma-number */
                        lit_utf8_byte_t *out_digits_p, /**< [out] buffer to fill with digits */
                        int32_t *out_decimal_exp_p) /**< [out] decimal exponent */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));
  JERRY_ASSERT (!ecma_number_is_zero (num));
  JERRY_ASSERT (!ecma_number_is_infinity (num));
  JERRY_ASSERT (!ecma_number_is_negative (num));

  return ecma_errol0_dtoa ((double) num, out_digits_p, out_decimal_exp_p);
} /* ecma_number_to_decimal */

/**
 * Convert ecma-number to zero-terminated string
 *
 * See also:
 *          ECMA-262 v5, 9.8.1
 *
 *
 * @return size of utf-8 string
 */
lit_utf8_size_t
ecma_number_to_utf8_string (ecma_number_t num, /**< ecma-number */
                            lit_utf8_byte_t *buffer_p, /**< buffer for utf-8 string */
                            lit_utf8_size_t buffer_size) /**< size of buffer */
{
  lit_utf8_byte_t *dst_p;

  if (ecma_number_is_nan (num))
  {
    /* 1. */
    dst_p = lit_copy_magic_string_to_buffer (LIT_MAGIC_STRING_NAN, buffer_p, buffer_size);
    return (lit_utf8_size_t) (dst_p - buffer_p);
  }

  if (ecma_number_is_zero (num))
  {
    /* 2. */
    *buffer_p = LIT_CHAR_0;
    JERRY_ASSERT (1 <= buffer_size);
    return 1;
  }

  dst_p = buffer_p;

  if (ecma_number_is_negative (num))
  {
    /* 3. */
    *dst_p++ = LIT_CHAR_MINUS;
    num = -num;
  }

  if (ecma_number_is_infinity (num))
  {
    /* 4. */
    dst_p = lit_copy_magic_string_to_buffer (LIT_MAGIC_STRING_INFINITY_UL,
                                             dst_p,
                                             (lit_utf8_size_t) (buffer_p + buffer_size - dst_p));
    JERRY_ASSERT (dst_p <= buffer_p + buffer_size);
    return (lit_utf8_size_t) (dst_p - buffer_p);
  }

  /* 5. */
  uint32_t num_uint32 = ecma_number_to_uint32 (num);

  if (((ecma_number_t) num_uint32) == num)
  {
    dst_p += ecma_uint32_to_utf8_string (num_uint32, dst_p, (lit_utf8_size_t) (buffer_p + buffer_size - dst_p));
    JERRY_ASSERT (dst_p <= buffer_p + buffer_size);
    return (lit_utf8_size_t) (dst_p - buffer_p);
  }

  /* decimal exponent */
  int32_t n;
  /* number of digits in mantissa */
  int32_t k;

  k = (int32_t) ecma_number_to_decimal (num, dst_p, &n);

  if (k <= n && n <= 21)
  {
    /* 6. */
    dst_p += k;

    memset (dst_p, LIT_CHAR_0, (size_t) (n - k));
    dst_p += n - k;

    JERRY_ASSERT (dst_p <= buffer_p + buffer_size);
    return (lit_utf8_size_t) (dst_p - buffer_p);
  }

  if (0 < n && n <= 21)
  {
    /* 7. */
    memmove (dst_p + n + 1, dst_p + n, (size_t) (k - n));
    *(dst_p + n) = LIT_CHAR_DOT;
    dst_p += k + 1;

    JERRY_ASSERT (dst_p <= buffer_p + buffer_size);
    return (lit_utf8_size_t) (dst_p - buffer_p);
  }

  if (-6 < n && n <= 0)
  {
    /* 8. */
    memmove (dst_p + 2 - n, dst_p, (size_t) k);
    memset (dst_p + 2, LIT_CHAR_0, (size_t) -n);
    *dst_p = LIT_CHAR_0;
    *(dst_p + 1) = LIT_CHAR_DOT;
    dst_p += k - n + 2;

    JERRY_ASSERT (dst_p <= buffer_p + buffer_size);
    return (lit_utf8_size_t) (dst_p - buffer_p);
  }

  if (k == 1)
  {
    /* 9. */
    dst_p++;
  }
  else
  {
    /* 10. */
    memmove (dst_p + 2, dst_p + 1, (size_t) (k - 1));
    *(dst_p + 1) = LIT_CHAR_DOT;
    dst_p += k + 1;
  }

  /* 9., 10. */
  *dst_p++ = LIT_CHAR_LOWERCASE_E;
  *dst_p++ = (n >= 1) ? LIT_CHAR_PLUS : LIT_CHAR_MINUS;
  uint32_t t = (uint32_t) (n >= 1 ? (n - 1) : -(n - 1));

  dst_p += ecma_uint32_to_utf8_string (t, dst_p, (lit_utf8_size_t) (buffer_p + buffer_size - dst_p));

  JERRY_ASSERT (dst_p <= buffer_p + buffer_size);

  return (lit_utf8_size_t) (dst_p - buffer_p);
} /* ecma_number_to_utf8_string */

/**
 * @}
 * @}
 */

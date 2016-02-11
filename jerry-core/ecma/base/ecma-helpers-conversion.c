/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
#include "jrt-libc-includes.h"
#include "lit-char-helpers.h"
#include "lit-magic-strings.h"

/*
 * \addtogroup ecmahelpersbigintegers Helpers for operations intermediate 128-bit integers
 * @{
 */

/**
 * Check that parts of 128-bit integer are 32-bit.
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT(name) \
{ \
  JERRY_ASSERT (name[0] == (uint32_t) name[0]); \
  JERRY_ASSERT (name[1] == (uint32_t) name[1]); \
  JERRY_ASSERT (name[2] == (uint32_t) name[2]); \
  JERRY_ASSERT (name[3] == (uint32_t) name[3]); \
}

/**
 * Declare 128-bit integer.
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER(name) uint64_t name[4] = { 0, 0, 0, 0 }

/**
 * Declare 128-bit in-out argument integer.
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_ARG(name) uint64_t name[4]

/**
 * Initialize 128-bit integer with given 32-bit parts
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_INIT(name, high, mid_high, mid_low, low) \
{ \
  name[3] = high; \
  name[2] = mid_high; \
  name[1] = mid_low; \
  name[0] = low; \
 \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name); \
}

/**
 * Copy specified 128-bit integer
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_COPY(name_copy_to, name_copy_from) \
{ \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name_copy_to); \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name_copy_from); \
  \
  name_copy_to[0] = name_copy_from[0]; \
  name_copy_to[1] = name_copy_from[1]; \
  name_copy_to[2] = name_copy_from[2]; \
  name_copy_to[3] = name_copy_from[3]; \
}

/**
 * Copy high and middle parts of 128-bit integer to specified uint64_t variable
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_ROUND_HIGH_AND_MIDDLE_TO_UINT64(name, uint64_var) \
{ \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name); \
  uint64_var = ((name[3] << 32u) | (name[2])) + (((name[1] >> 31u) != 0 ? 1 : 0)); \
}

/**
 * Copy middle and low parts of 128-bit integer to specified uint64_t variable
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_ROUND_MIDDLE_AND_LOW_TO_UINT64(name, uint64_var) \
{ \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name); \
  uint64_var = (name[1] << 32u) | (name[0]); \
}

/**
 * Check if specified 128-bit integers are equal
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_ARE_EQUAL(name1, name2) \
  ((name1)[0] == (name2[0]) \
   && (name1)[1] == (name2[1]) \
   && (name1)[2] == (name2[2]) \
   && (name1)[3] == (name2[3]))

/**
 * Check if bits [lowest_bit, 128) are zero
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO(name, lowest_bit) \
  ((lowest_bit) >= 96 ? ((name[3] >> ((lowest_bit) - 96)) == 0) : \
   ((lowest_bit) >= 64 ? (name[3] == 0 \
                          && ((name[2] >> ((lowest_bit) - 64)) == 0)) : \
   ((lowest_bit) >= 32 ? (name[3] == 0 \
                          && name[2] == 0 && ((name[1] >> ((lowest_bit) - 32)) == 0)) : \
   (name[3] == 0 && name[2] == 0 && name[1] == 0 && ((name[0] >> (lowest_bit)) == 0)))))

/**
 * Check if bits [0, highest_bit] are zero
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_LOW_BIT_MASK_ZERO(name, highest_bit) \
  ((highest_bit >= 96) ? (name[2] == 0 && name[1] == 0 && name[0] == 0 \
                          && (((uint32_t) name[3] << (127 - (highest_bit))) == 0)) : \
   ((highest_bit >= 64) ? (name[1] == 0 && name[0] == 0 \
                           && (((uint32_t) name[2] << (95 - (highest_bit))) == 0)) : \
   ((highest_bit >= 32) ? (name[0] == 0 \
                           && (((uint32_t) name[1] << (63 - (highest_bit))) == 0)) : \
    (((uint32_t) name[0] << (31 - (highest_bit))) == 0))))

/**
 * Check if 128-bit integer is zero
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_ZERO(name) \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (name, 0)

/**
 * Shift 128-bit integer one bit left
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_LEFT_SHIFT(name) \
{ \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name); \
  \
  name[3] = (uint32_t) (name[3] << 1u); \
  name[3] |= name[2] >> 31u; \
  name[2] = (uint32_t) (name[2] << 1u); \
  name[2] |= name[1] >> 31u; \
  name[1] = (uint32_t) (name[1] << 1u); \
  name[1] |= name[0] >> 31u; \
  name[0] = (uint32_t) (name[0] << 1u); \
  \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name); \
}

/**
 * Shift 128-bit integer one bit right
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_RIGHT_SHIFT(name) \
{ \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name); \
  \
  name[0] >>= 1u; \
  name[0] |= (uint32_t) (name[1] << 31u); \
  name[1] >>= 1u; \
  name[1] |= (uint32_t) (name[2] << 31u); \
  name[2] >>= 1u; \
  name[2] |= (uint32_t) (name[3] << 31u); \
  name[3] >>= 1u; \
  \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name); \
}

/**
 * Increment 128-bit integer
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_INC(name) \
{ \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name); \
  \
  name[0] += 1ull; \
  name[1] += (name[0] >> 32u); \
  name[0] = (uint32_t) name[0]; \
  name[2] += (name[1] >> 32u); \
  name[1] = (uint32_t) name[1]; \
  name[3] += (name[2] >> 32u); \
  name[2] = (uint32_t) name[2]; \
  \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name); \
}

/**
 * Add 128-bit integer
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_ADD(name_add_to, name_to_add) \
{ \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name_add_to); \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name_to_add); \
  \
  name_add_to[0] += name_to_add[0]; \
  name_add_to[1] += name_to_add[1]; \
  name_add_to[2] += name_to_add[2]; \
  name_add_to[3] += name_to_add[3]; \
  \
  name_add_to[1] += (name_add_to[0] >> 32u); \
  name_add_to[0] = (uint32_t) name_add_to[0]; \
  name_add_to[2] += (name_add_to[1] >> 32u); \
  name_add_to[1] = (uint32_t) name_add_to[1]; \
  name_add_to[3] += (name_add_to[2] >> 32u); \
  name_add_to[2] = (uint32_t) name_add_to[2]; \
  \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name_add_to); \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name_to_add); \
}

/**
 * Multiply 128-bit integer by 10
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_MUL_10(name) \
{ \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name); \
  \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_LEFT_SHIFT (name); \
  \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER (name ## _tmp); \
  \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_COPY (name ## _tmp, name); \
  \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_LEFT_SHIFT (name ## _tmp); \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_LEFT_SHIFT (name ## _tmp); \
  \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_ADD (name, name ## _tmp); \
  \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name); \
}

/**
 * Divide 128-bit integer by 10
 */
#define ECMA_NUMBER_CONVERSION_128BIT_INTEGER_DIV_10(name) \
{ \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name); \
  \
  /* estimation of reciprocal of 10 */ \
  const uint64_t div10_p_low = 0x9999999aul; \
  const uint64_t div10_p_mid = 0x99999999ul; \
  const uint64_t div10_p_high = 0x19999999ul; \
  \
  uint64_t intermediate[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; \
  uint64_t l0, l1, l2, l3, m0, m1, m2, m3, h0, h1, h2, h3; \
  l0 = name[0] * div10_p_low; \
  l1 = name[1] * div10_p_low; \
  l2 = name[2] * div10_p_low; \
  l3 = name[3] * div10_p_low; \
  m0 = name[0] * div10_p_mid; \
  m1 = name[1] * div10_p_mid; \
  m2 = name[2] * div10_p_mid; \
  m3 = name[3] * div10_p_mid; \
  h0 = name[0] * div10_p_high; \
  h1 = name[1] * div10_p_high; \
  h2 = name[2] * div10_p_high; \
  h3 = name[3] * div10_p_high; \
  intermediate[0] += (uint32_t) l0; \
  intermediate[1] += l0 >> 32u; \
  \
  intermediate[1] += (uint32_t) l1; \
  intermediate[2] += l1 >> 32u; \
  intermediate[1] += (uint32_t) m0; \
  intermediate[2] += m0 >> 32u; \
  \
  intermediate[2] += (uint32_t) l2; \
  intermediate[3] += l2 >> 32u; \
  intermediate[2] += (uint32_t) m1; \
  intermediate[3] += m1 >> 32u; \
  intermediate[2] += (uint32_t) m0; \
  intermediate[3] += m0 >> 32u; \
  \
  intermediate[3] += (uint32_t) l3; \
  intermediate[4] += l3 >> 32u; \
  intermediate[3] += (uint32_t) m2; \
  intermediate[4] += m2 >> 32u; \
  intermediate[3] += (uint32_t) m1; \
  intermediate[4] += m1 >> 32u; \
  intermediate[3] += (uint32_t) h0; \
  intermediate[4] += h0 >> 32u; \
  \
  intermediate[4] += (uint32_t) m3; \
  intermediate[5] += m3 >> 32u; \
  intermediate[4] += (uint32_t) m2; \
  intermediate[5] += m2 >> 32u; \
  intermediate[4] += (uint32_t) h1; \
  intermediate[5] += h1 >> 32u; \
  \
  intermediate[5] += (uint32_t) m3; \
  intermediate[6] += m3 >> 32u; \
  intermediate[5] += (uint32_t) h2; \
  intermediate[6] += h2 >> 32u; \
  \
  intermediate[6] += (uint32_t) h3; \
  intermediate[7] += h3 >> 32u; \
  \
  intermediate[1] += intermediate[0] >> 32u; \
  intermediate[0] = (uint32_t) intermediate[0]; \
  intermediate[2] += intermediate[1] >> 32u; \
  intermediate[1] = (uint32_t) intermediate[1]; \
  intermediate[3] += intermediate[2] >> 32u; \
  intermediate[2] = (uint32_t) intermediate[2]; \
  intermediate[4] += intermediate[3] >> 32u; \
  intermediate[3] = (uint32_t) intermediate[3]; \
  intermediate[5] += intermediate[4] >> 32u; \
  intermediate[4] = (uint32_t) intermediate[4]; \
  intermediate[6] += intermediate[5] >> 32u; \
  intermediate[5] = (uint32_t) intermediate[5]; \
  intermediate[7] += intermediate[6] >> 32u; \
  intermediate[6] = (uint32_t) intermediate[6]; \
  \
  name[0] = intermediate[4]; \
  name[1] = intermediate[5]; \
  name[2] = intermediate[6]; \
  name[3] = intermediate[7]; \
  \
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_CHECK_PARTS_ARE_32BIT (name); \
}

/**
 * @}
 */

/**
 * ECMA-defined conversion of string to Number.
 *
 * See also:
 *          ECMA-262 v5, 9.3.1
 *
 * @return ecma-number
 */
ecma_number_t
ecma_utf8_string_to_number (const lit_utf8_byte_t *str_p, /**< utf-8 string */
                            lit_utf8_size_t str_size) /**< string size */
{
  TODO (Check license issues);

  const lit_utf8_byte_t dec_digits_range[10] = { '0', '9' };
  const lit_utf8_byte_t hex_lower_digits_range[10] = { 'a', 'f' };
  const lit_utf8_byte_t hex_upper_digits_range[10] = { 'A', 'F' };
  const lit_utf8_byte_t hex_x_chars[2] = { 'x', 'X' };
  const lit_utf8_byte_t e_chars[2] = { 'e', 'E' };
  const lit_utf8_byte_t plus_char = '+';
  const lit_utf8_byte_t minus_char = '-';
  const lit_utf8_byte_t dot_char = '.';

  if (str_size == 0)
  {
    return ECMA_NUMBER_ZERO;
  }

  lit_utf8_byte_t *str_curr_p = (lit_utf8_byte_t *) str_p;
  const lit_utf8_byte_t *str_end_p = str_p + str_size;
  ecma_char_t code_unit;

  while (str_curr_p < str_end_p)
  {
    code_unit = lit_utf8_peek_next (str_curr_p);
    if (lit_char_is_white_space (code_unit) || lit_char_is_line_terminator (code_unit))
    {
      lit_utf8_incr (&str_curr_p);
    }
    else
    {
      break;
    }
  }

  const lit_utf8_byte_t *begin_p = str_curr_p;
  str_curr_p = (lit_utf8_byte_t *) str_end_p;

  while (str_curr_p > str_p)
  {
    code_unit = lit_utf8_peek_prev (str_curr_p);
    if (lit_char_is_white_space (code_unit) || lit_char_is_line_terminator (code_unit))
    {
      lit_utf8_decr (&str_curr_p);
    }
    else
    {
      break;
    }
  }

  const lit_utf8_byte_t *end_p = str_curr_p - 1;

  if (begin_p > end_p)
  {
    return ECMA_NUMBER_ZERO;
  }

  const ssize_t literal_len = end_p - begin_p + 1;

  if (literal_len > 2
      && begin_p[0] == dec_digits_range[0]
      && (begin_p[1] == hex_x_chars[0]
          || begin_p[1] == hex_x_chars[1]))
  {
    /* Hex literal handling */
    begin_p += 2;

    ecma_number_t num = 0;

    for (const lit_utf8_byte_t * iter_p = begin_p;
         iter_p <= end_p;
         iter_p++)
    {
      int32_t digit_value;

      if (*iter_p >= dec_digits_range[0]
          && *iter_p <= dec_digits_range[1])
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

  /* Checking if significant part of parse string is equal to "Infinity" */
  const lit_utf8_byte_t *infinity_zt_str_p = lit_get_magic_string_utf8 (LIT_MAGIC_STRING_INFINITY_UL);

  for (const lit_utf8_byte_t *iter_p = begin_p, *iter_infinity_p = infinity_zt_str_p;
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

  uint64_t fraction_uint64 = 0;
  uint32_t digits = 0;
  int32_t e = 0;

  /* Parsing digits before dot (or before end of digits part if there is no dot in number) */
  while (begin_p <= end_p)
  {
    int32_t digit_value;

    if (*begin_p >= dec_digits_range[0]
        && *begin_p <= dec_digits_range[1])
    {
      digit_value = (*begin_p - dec_digits_range[0]);
    }
    else
    {
      break;
    }

    if (digits != 0 || digit_value != 0)
    {
      if (digits < ECMA_NUMBER_MAX_DIGITS)
      {
        fraction_uint64 = fraction_uint64 * 10 + (uint32_t) digit_value;
        digits++;
      }
      else if (e <= 100000) /* Some limit to not overflow exponent value
                               (so big exponent anyway will make number
                               rounded to infinity) */
      {
        e++;
      }
    }

    begin_p++;
  }

  if (begin_p <= end_p
      && *begin_p == dot_char)
  {
    begin_p++;

    /* Parsing number's part that is placed after dot */
    while (begin_p <= end_p)
    {
      int32_t digit_value;

      if (*begin_p >= dec_digits_range[0]
          && *begin_p <= dec_digits_range[1])
      {
        digit_value = (*begin_p - dec_digits_range[0]);
      }
      else
      {
        break;
      }

      if (digits < ECMA_NUMBER_MAX_DIGITS)
      {
        if (digits != 0 || digit_value != 0)
        {
          fraction_uint64 = fraction_uint64 * 10 + (uint32_t) digit_value;
          digits++;
        }

        e--;
      }

      begin_p++;
    }
  }

  /* Parsing exponent literal */
  int32_t e_in_lit = 0;
  bool e_in_lit_sign = false;

  if (begin_p <= end_p
      && (*begin_p == e_chars[0]
          || *begin_p == e_chars[1]))
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

      if (*begin_p >= dec_digits_range[0]
          && *begin_p <= dec_digits_range[1])
      {
        digit_value = (*begin_p - dec_digits_range[0]);
      }
      else
      {
        return ecma_number_make_nan ();
      }

      e_in_lit = e_in_lit * 10 + digit_value;

      begin_p++;
    }
  }

  /* Adding value of exponent literal to exponent value */
  if (e_in_lit_sign)
  {
    e -= e_in_lit;
  }
  else
  {
    e += e_in_lit;
  }

  bool e_sign;

  if (e < 0)
  {
    e_sign = true;
    e = -e;
  }
  else
  {
    e_sign = false;
  }

  if (begin_p <= end_p)
  {
    return ecma_number_make_nan ();
  }

  JERRY_ASSERT (begin_p == end_p + 1);

  if (fraction_uint64 == 0)
  {
    return sign ? -ECMA_NUMBER_ZERO : ECMA_NUMBER_ZERO;
  }

#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
  int32_t binary_exponent = 33;

  /*
   * 128-bit mantissa storage
   *
   * Normalized: |4 bits zero|124-bit mantissa with highest bit set to 1 if mantissa is non-zero|
   */
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER (fraction_uint128);
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_INIT (fraction_uint128,
                                             0ull,
                                             fraction_uint64 >> 32u,
                                             (uint32_t) fraction_uint64,
                                             0ull);

  /* Normalizing mantissa */
  JERRY_ASSERT (ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 124));

  while (ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 123))
  {
    ECMA_NUMBER_CONVERSION_128BIT_INTEGER_LEFT_SHIFT (fraction_uint128);
    binary_exponent--;

    JERRY_ASSERT (!ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_ZERO (fraction_uint128));
  }

  if (!e_sign)
  {
    /* positive or zero decimal exponent */
    JERRY_ASSERT (e >= 0);

    while (e > 0)
    {
      JERRY_ASSERT (ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 124));

      ECMA_NUMBER_CONVERSION_128BIT_INTEGER_MUL_10 (fraction_uint128);

      e--;

      /* Normalizing mantissa */
      while (!ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 124))
      {
        ECMA_NUMBER_CONVERSION_128BIT_INTEGER_RIGHT_SHIFT (fraction_uint128);

        binary_exponent++;
      }
      while (ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 123))
      {
        ECMA_NUMBER_CONVERSION_128BIT_INTEGER_LEFT_SHIFT (fraction_uint128);

        binary_exponent--;

        JERRY_ASSERT (!ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_ZERO (fraction_uint128));
      }
    }
  }
  else
  {
    /* negative decimal exponent */
    JERRY_ASSERT (e != 0);

    while (e > 0)
    {
      /* Denormalizing mantissa, moving highest 1 to 95-bit */
      while (ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 127))
      {
        ECMA_NUMBER_CONVERSION_128BIT_INTEGER_LEFT_SHIFT (fraction_uint128);

        binary_exponent--;

        JERRY_ASSERT (!ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_ZERO (fraction_uint128));
      }

      ECMA_NUMBER_CONVERSION_128BIT_INTEGER_DIV_10 (fraction_uint128);

      e--;
    }

    /* Normalizing mantissa */
    while (!ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 124))
    {
      ECMA_NUMBER_CONVERSION_128BIT_INTEGER_RIGHT_SHIFT (fraction_uint128);

      binary_exponent++;
    }
    while (ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 123))
    {
      ECMA_NUMBER_CONVERSION_128BIT_INTEGER_LEFT_SHIFT (fraction_uint128);

      binary_exponent--;

      JERRY_ASSERT (!ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_ZERO (fraction_uint128));
    }
  }

  JERRY_ASSERT (!ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_ZERO (fraction_uint128));
  JERRY_ASSERT (ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 124));

  /*
   * Preparing mantissa for conversion to 52-bit representation, converting it to:
   *
   * |12 zero bits|116 mantissa bits|
   */
  while (!ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 116 + 1))
  {
    ECMA_NUMBER_CONVERSION_128BIT_INTEGER_RIGHT_SHIFT (fraction_uint128);

    binary_exponent++;
  }
  while (ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 116))
  {
    ECMA_NUMBER_CONVERSION_128BIT_INTEGER_LEFT_SHIFT (fraction_uint128);

    binary_exponent--;

    JERRY_ASSERT (!ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_ZERO (fraction_uint128));
  }

  JERRY_ASSERT (ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 116 + 1));

  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_ROUND_HIGH_AND_MIDDLE_TO_UINT64 (fraction_uint128, fraction_uint64);

  return ecma_number_make_from_sign_mantissa_and_exponent (sign,
                                                           fraction_uint64,
                                                           binary_exponent);
#elif CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
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
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32 */
} /* ecma_utf8_string_to_number */

/**
 * ECMA-defined conversion of UInt32 to String (zero-terminated).
 *
 * See also:
 *          ECMA-262 v5, 9.8.1
 *
 * @return number of bytes copied to buffer
 */
ssize_t
ecma_uint32_to_utf8_string (uint32_t value, /**< value to convert */
                            lit_utf8_byte_t *out_buffer_p, /**< buffer for string */
                            ssize_t buffer_size) /**< size of buffer */
{
  const lit_utf8_byte_t digits[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

  lit_utf8_byte_t *p = out_buffer_p + buffer_size - 1;
  size_t bytes_copied = 0;

  do
  {
    JERRY_ASSERT (p >= out_buffer_p);

    *p-- = digits[value % 10];
    value /= 10;

    bytes_copied ++;
  }
  while (value != 0);

  p++;

  JERRY_ASSERT (p >= out_buffer_p);

  if (likely (p != out_buffer_p))
  {
    ssize_t bytes_to_move = out_buffer_p + buffer_size - p;
    memmove (out_buffer_p, p, (size_t) bytes_to_move);
  }

  return (ssize_t) bytes_copied;
} /* ecma_uint32_to_utf8_string */

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
  if (ecma_number_is_nan (num)
      || ecma_number_is_zero (num)
      || ecma_number_is_infinity (num))
  {
    return 0;
  }

  bool sign = ecma_number_is_negative (num);
  ecma_number_t abs_num;

  if (sign)
  {
    abs_num = ecma_number_negate (num);
  }
  else
  {
    abs_num = num;
  }

  // 2 ^ 32
  const uint64_t uint64_2_pow_32 = (1ull << 32);

  const ecma_number_t num_2_pow_32 = (float) uint64_2_pow_32;

  ecma_number_t num_in_uint32_range;

  if (abs_num >= num_2_pow_32)
  {
    num_in_uint32_range = ecma_number_calc_remainder (abs_num,
                                                      num_2_pow_32);
  }
  else
  {
    num_in_uint32_range = abs_num;
  }

  // Check that the floating point value can be represented with uint32_t
  JERRY_ASSERT (num_in_uint32_range < uint64_2_pow_32);
  uint32_t uint32_num = (uint32_t) num_in_uint32_range;

  uint32_t ret;

  if (sign)
  {
    ret = -uint32_num;
  }
  else
  {
    ret = uint32_num;
  }

#ifndef JERRY_NDEBUG
  if (sign
      && uint32_num != 0)
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

  // 2 ^ 32
  const int64_t int64_2_pow_32 = (1ll << 32);

  // 2 ^ 31
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

#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
/**
  * Perform conversion of 128-bit binary representation of number
  * to decimal representation with decimal exponent.
  */
static void
ecma_number_helper_binary_to_decimal (ECMA_NUMBER_CONVERSION_128BIT_INTEGER_ARG (fraction_uint128), /**< mantissa */
                                      int32_t binary_exponent, /**< binary exponent */
                                      int32_t *out_decimal_exp_p) /**< out: decimal exponent */
{
  int32_t decimal_exp = 0;

  if (binary_exponent > 0)
  {
    while (binary_exponent > 0)
    {
      if (!ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 124))
      {
        ECMA_NUMBER_CONVERSION_128BIT_INTEGER_INC (fraction_uint128);
        ECMA_NUMBER_CONVERSION_128BIT_INTEGER_RIGHT_SHIFT (fraction_uint128);
        binary_exponent++;
      }
      else
      {
        ECMA_NUMBER_CONVERSION_128BIT_INTEGER (fraction_uint128_tmp);
        ECMA_NUMBER_CONVERSION_128BIT_INTEGER_COPY (fraction_uint128_tmp, fraction_uint128);
        ECMA_NUMBER_CONVERSION_128BIT_INTEGER_DIV_10 (fraction_uint128_tmp);
        ECMA_NUMBER_CONVERSION_128BIT_INTEGER_MUL_10 (fraction_uint128_tmp);

        if (!ECMA_NUMBER_CONVERSION_128BIT_INTEGER_ARE_EQUAL (fraction_uint128, fraction_uint128_tmp)
            && ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 123))
        {
          ECMA_NUMBER_CONVERSION_128BIT_INTEGER_LEFT_SHIFT (fraction_uint128);
          binary_exponent--;
        }
        else
        {
          ECMA_NUMBER_CONVERSION_128BIT_INTEGER_DIV_10 (fraction_uint128);
          decimal_exp++;
        }
      }
    }
  }
  else if (binary_exponent < 0)
  {
    while (binary_exponent < 0)
    {
      if (ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_LOW_BIT_MASK_ZERO (fraction_uint128, 0)
          || !ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128, 124))
      {
        ECMA_NUMBER_CONVERSION_128BIT_INTEGER_RIGHT_SHIFT (fraction_uint128);

        binary_exponent++;
      }
      else
      {
        ECMA_NUMBER_CONVERSION_128BIT_INTEGER_MUL_10 (fraction_uint128);

        decimal_exp--;
      }
    }
  }

  *out_decimal_exp_p = decimal_exp;
} /* ecma_number_helper_binary_to_decimal */

#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */

/**
  * Perform conversion of ecma-number to decimal representation with decimal exponent
  *
  * Note:
  *      The calculated values correspond to s, n, k parameters in ECMA-262 v5, 9.8.1, item 5:
  *         - s represents digits of the number;
  *         - k is the number of digits;
  *         - n is the decimal exponent.
  */
void
ecma_number_to_decimal (ecma_number_t num, /**< ecma-number */
                        uint64_t *out_digits_p, /**< out: digits */
                        int32_t *out_digits_num_p, /**< out: number of digits */
                        int32_t *out_decimal_exp_p) /**< out: decimal exponent */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));
  JERRY_ASSERT (!ecma_number_is_zero (num));
  JERRY_ASSERT (!ecma_number_is_infinity (num));

#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
  ecma_number_t num_m1 = ecma_number_get_prev (num);
  ecma_number_t num_p1 = ecma_number_get_next (num);

  ECMA_NUMBER_CONVERSION_128BIT_INTEGER (fraction_uint128);
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER (fraction_uint128_m1);
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER (fraction_uint128_p1);

  uint64_t fraction_uint64, fraction_uint64_m1, fraction_uint64_p1;
  int32_t binary_exponent, binary_exponent_m1, binary_exponent_p1;
  int32_t decimal_exp, decimal_exp_m1, decimal_exp_p1;
  int32_t dot_shift, dot_shift_m1, dot_shift_p1;

  dot_shift_m1 = ecma_number_get_fraction_and_exponent (num_m1, &fraction_uint64_m1, &binary_exponent_m1);
  dot_shift = ecma_number_get_fraction_and_exponent (num, &fraction_uint64, &binary_exponent);
  dot_shift_p1 = ecma_number_get_fraction_and_exponent (num_p1, &fraction_uint64_p1, &binary_exponent_p1);

  binary_exponent_m1 -= dot_shift_m1;
  binary_exponent -= dot_shift;
  binary_exponent_p1 -= dot_shift_p1;

  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_INIT (fraction_uint128,
                                              0ull,
                                              0ull,
                                              (fraction_uint64) >> 32u,
                                              ((fraction_uint64) << 32u) >> 32u);
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_INIT (fraction_uint128_m1,
                                              0ull,
                                              0ull,
                                              (fraction_uint64_m1) >> 32u,
                                              ((fraction_uint64_m1) << 32u) >> 32u);
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_INIT (fraction_uint128_p1,
                                              0ull,
                                              0ull,
                                              (fraction_uint64_p1) >> 32u,
                                              ((fraction_uint64_p1) << 32u) >> 32u);

  ecma_number_helper_binary_to_decimal (fraction_uint128, binary_exponent, &decimal_exp);
  ecma_number_helper_binary_to_decimal (fraction_uint128_m1, binary_exponent_m1, &decimal_exp_m1);
  ecma_number_helper_binary_to_decimal (fraction_uint128_p1, binary_exponent_p1, &decimal_exp_p1);

  if (ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_ZERO (fraction_uint128_m1))
  {
    decimal_exp_m1 = decimal_exp;
  }

  while (decimal_exp != decimal_exp_m1
         || decimal_exp != decimal_exp_p1)
  {
    while (decimal_exp > decimal_exp_m1
           || decimal_exp > decimal_exp_p1)
    {
      ECMA_NUMBER_CONVERSION_128BIT_INTEGER_MUL_10 (fraction_uint128);
      decimal_exp--;
    }
    while (decimal_exp_m1 > decimal_exp
           || decimal_exp_m1 > decimal_exp_p1)
    {
      ECMA_NUMBER_CONVERSION_128BIT_INTEGER_MUL_10 (fraction_uint128_m1);
      decimal_exp_m1--;
    }
    while (decimal_exp_p1 > decimal_exp
           || decimal_exp_p1 > decimal_exp_m1)
    {
      ECMA_NUMBER_CONVERSION_128BIT_INTEGER_MUL_10 (fraction_uint128_p1);
      decimal_exp_p1--;
    }
  }

  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_ADD (fraction_uint128_m1, fraction_uint128);
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_RIGHT_SHIFT (fraction_uint128_m1);

  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_ADD (fraction_uint128_p1, fraction_uint128);
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_RIGHT_SHIFT (fraction_uint128_p1);

  /* While fraction doesn't fit to integer, divide it by 10
       and simultaneously increment decimal exponent */
  uint64_t digits_min, digits_max;

  while (!ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128_m1, 63))
  {
    ECMA_NUMBER_CONVERSION_128BIT_INTEGER_DIV_10 (fraction_uint128_m1);
    decimal_exp_m1++;
  }
  while (!ECMA_NUMBER_CONVERSION_128BIT_INTEGER_IS_HIGH_BIT_MASK_ZERO (fraction_uint128_p1, 63))
  {
    ECMA_NUMBER_CONVERSION_128BIT_INTEGER_DIV_10 (fraction_uint128_p1);
    decimal_exp_p1++;
  }

  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_ROUND_MIDDLE_AND_LOW_TO_UINT64 (fraction_uint128_m1, digits_min);
  ECMA_NUMBER_CONVERSION_128BIT_INTEGER_ROUND_MIDDLE_AND_LOW_TO_UINT64 (fraction_uint128_p1, digits_max);

  digits_min++;

  if (decimal_exp_m1 < decimal_exp_p1)
  {
    JERRY_ASSERT (decimal_exp_m1 == decimal_exp_p1 - 1);

    digits_min /= 10;
    decimal_exp_m1++;
  }
  else if (decimal_exp_m1 > decimal_exp_p1)
  {
    JERRY_ASSERT (decimal_exp_m1 == decimal_exp_p1 + 1);

    digits_max /= 10;
    decimal_exp_p1++;
  }

  JERRY_ASSERT (digits_max >= digits_min);

  while (digits_min / 10 != digits_max / 10)
  {
    digits_min /= 10;
    digits_max /= 10;
    decimal_exp_m1++;
    decimal_exp_p1++;
  }

  uint64_t digits = (digits_min + digits_max + 1) / 2;
  int32_t digits_num = 0;
  uint64_t t = digits;

  while (t != 0)
  {
    t /= 10;
    digits_num++;
  }

  JERRY_ASSERT (digits_num > 0);

  *out_digits_p = digits;
  *out_digits_num_p = digits_num;
  *out_decimal_exp_p = decimal_exp_p1 + digits_num;

#elif CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
  /* Less precise conversion */
  uint64_t fraction_uint64;
  uint32_t fraction;
  int32_t exponent;
  int32_t dot_shift;
  int32_t decimal_exp = 0;

  dot_shift = ecma_number_get_fraction_and_exponent (num, &fraction_uint64, &exponent);

  fraction = (uint32_t) fraction_uint64;
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
      fraction = (uint32_t) ((ecma_number_t) fraction / t);
    }
    else
    {
      fraction = (uint32_t) ((ecma_number_t) fraction * t);
    }
  }

  uint32_t s;
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
  const uint32_t frac_part_mask = ((((uint32_t)1) << int_part_shift) - 1);

  uint32_t int_part = fraction >> int_part_shift;
  uint32_t frac_part = fraction & frac_part_mask;

  s = int_part;
  k = 1;
  n = decimal_exp + 1;

  JERRY_ASSERT (int_part < 10);

  while (k < ECMA_NUMBER_MAX_DIGITS
         && frac_part != 0)
  {
    frac_part *= 10;

    uint32_t new_frac_part = frac_part & frac_part_mask;
    uint32_t digit = (frac_part - new_frac_part) >> int_part_shift;
    s = s * 10 + digit;
    k++;
    frac_part = new_frac_part;
  }

  JERRY_ASSERT (k > 0);

  *out_digits_p = s;
  *out_digits_num_p = k;
  *out_decimal_exp_p = n;
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32 */
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
                            ssize_t buffer_size) /**< size of buffer */
{
  const lit_utf8_byte_t digits[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
  const lit_utf8_byte_t e_chars[2] = { 'e', 'E' };
  const lit_utf8_byte_t plus_char = '+';
  const lit_utf8_byte_t minus_char = '-';
  const lit_utf8_byte_t dot_char = '.';
  lit_utf8_size_t size;

  if (ecma_number_is_nan (num))
  {
    // 1.
    lit_copy_magic_string_to_buffer (LIT_MAGIC_STRING_NAN, buffer_p, buffer_size);
    size = lit_get_magic_string_size (LIT_MAGIC_STRING_NAN);
  }
  else
  {
    lit_utf8_byte_t *dst_p = buffer_p;

    if (ecma_number_is_zero (num))
    {
      // 2.
      *dst_p++ = digits[0];

      JERRY_ASSERT (dst_p - buffer_p <= (ssize_t) buffer_size);
      size = (lit_utf8_size_t) (dst_p - buffer_p);
    }
    else if (ecma_number_is_negative (num))
    {
      // 3.
      *dst_p++ = minus_char;
      ssize_t new_buffer_size = (buffer_size - (dst_p - buffer_p));
      size = 1 + ecma_number_to_utf8_string (ecma_number_negate (num), dst_p, new_buffer_size);
    }
    else if (ecma_number_is_infinity (num))
    {
      // 4.
      dst_p = lit_copy_magic_string_to_buffer (LIT_MAGIC_STRING_INFINITY_UL, buffer_p, buffer_size);
      size = (lit_utf8_size_t) (dst_p - buffer_p);
    }
    else
    {
      ecma_number_t p = ecma_number_get_prev (num);
      ecma_number_t q = ecma_number_get_next (p);
      JERRY_ASSERT (q == num);

      // 5.
      uint32_t num_uint32 = ecma_number_to_uint32 (num);
      if (ecma_uint32_to_number (num_uint32) == num)
      {
        size = (lit_utf8_size_t) ecma_uint32_to_utf8_string (num_uint32, dst_p, buffer_size);
      }
      else
      {
        /* mantissa */
        uint64_t s;
        /* decimal exponent */
        int32_t n;
        /* number of digits in k */
        int32_t k;

        ecma_number_to_decimal (num, &s, &k, &n);

        // 6.
        if (k <= n && n <= 21)
        {
          dst_p += n;
          JERRY_ASSERT ((ssize_t) (dst_p - buffer_p) <= buffer_size);

          size = (lit_utf8_size_t) (dst_p - buffer_p);

          for (int32_t i = 0; i < n - k; i++)
          {
            *--dst_p = digits[0];
          }

          for (int32_t i = 0; i < k; i++)
          {
            *--dst_p = digits[s % 10];
            s /= 10;
          }
        }
        else if (0 < n && n <= 21)
        {
          // 7.
          dst_p += k + 1;
          JERRY_ASSERT ((ssize_t) (dst_p - buffer_p) <= buffer_size);

          size = (lit_utf8_size_t) (dst_p - buffer_p);

          for (int32_t i = 0; i < k - n; i++)
          {
            *--dst_p = digits[s % 10];
            s /= 10;
          }

          *--dst_p = dot_char;

          for (int32_t i = 0; i < n; i++)
          {
            *--dst_p = digits[s % 10];
            s /= 10;
          }
        }
        else if (-6 < n && n <= 0)
        {
          // 8.
          dst_p += k - n + 1 + 1;
          JERRY_ASSERT ((ssize_t) (dst_p - buffer_p) <= buffer_size);

          size = (lit_utf8_size_t) (dst_p - buffer_p);

          for (int32_t i = 0; i < k; i++)
          {
            *--dst_p = digits[s % 10];
            s /= 10;
          }

          for (int32_t i = 0; i < -n; i++)
          {
            *--dst_p = digits[0];
          }

          *--dst_p = dot_char;
          *--dst_p = digits[0];
        }
        else
        {
          if (k == 1)
          {
            // 9.
            JERRY_ASSERT (1 <= buffer_size);

            size = 1;

            *dst_p++ = digits[s % 10];
            s /= 10;
          }
          else
          {
            // 10.
            dst_p += k + 1;
            JERRY_ASSERT ((ssize_t) (dst_p - buffer_p) <= buffer_size);

            for (int32_t i = 0; i < k - 1; i++)
            {
              *--dst_p = digits[s % 10];
              s /= 10;
            }

            *--dst_p = dot_char;
            *--dst_p = digits[s % 10];
            s /= 10;

            dst_p += k + 1;
          }

          // 9., 10.
          JERRY_ASSERT ((ssize_t) (dst_p - buffer_p + 2) <= buffer_size);
          *dst_p++ = e_chars[0];
          *dst_p++ = (n >= 1) ? plus_char : minus_char;
          int32_t t = (n >= 1) ? (n - 1) : -(n - 1);

          if (t == 0)
          {
            JERRY_ASSERT ((ssize_t) (dst_p - buffer_p) <= buffer_size);
            *dst_p++ = digits[0];
          }
          else
          {
            int32_t t_mod = 1000000000u;

            while ((t / t_mod) == 0)
            {
              t_mod /= 10;

              JERRY_ASSERT (t != 0);
            }

            while (t_mod != 0)
            {
              JERRY_ASSERT ((ssize_t) (dst_p - buffer_p + 1) <= buffer_size);
              *dst_p++ = digits[t / t_mod];

              t -= (t / t_mod) * t_mod;
              t_mod /= 10;
            }
          }

          JERRY_ASSERT ((ssize_t) (dst_p - buffer_p) <= buffer_size);
          size = (lit_utf8_size_t) (dst_p - buffer_p);
        }

        JERRY_ASSERT (s == 0);
      }
    }
  }

  return size;
} /* ecma_number_to_utf8_string */

/**
 * @}
 * @}
 */

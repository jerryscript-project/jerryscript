/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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

#include <math.h>

#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "jrt-libc-includes.h"
#include "lit-char-helpers.h"
#include "lit-magic-strings.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 *
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
  /* TODO: Check license issues */

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

  const lit_utf8_byte_t *str_curr_p = str_p;
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

  if ((end_p >= begin_p + 2)
      && begin_p[0] == dec_digits_range[0]
      && (begin_p[1] == hex_x_chars[0]
          || begin_p[1] == hex_x_chars[1]))
  {
    /* Hex literal handling */
    begin_p += 2;

    ecma_number_t num = ECMA_NUMBER_ZERO;

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
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */
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
  const lit_utf8_byte_t digits[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

  lit_utf8_byte_t *p = out_buffer_p + buffer_size - 1;
  lit_utf8_size_t bytes_copied = 0;

  do
  {
    JERRY_ASSERT (p >= out_buffer_p);

    *p-- = digits[value % 10];
    value /= 10;

    bytes_copied++;
  }
  while (value != 0);

  p++;

  JERRY_ASSERT (p >= out_buffer_p);

  if (likely (p != out_buffer_p))
  {
    memmove (out_buffer_p, p, bytes_copied);
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
  if (ecma_number_is_nan (num)
      || ecma_number_is_zero (num)
      || ecma_number_is_infinity (num))
  {
    return 0;
  }

  const bool sign = ecma_number_is_negative (num);
  const ecma_number_t abs_num = sign ? ecma_number_negate (num) : num;

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

  const uint32_t ret = sign ? -uint32_num : uint32_num;

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

#define GRISU3_SIGN_BIT                 0x8000000000000000ULL
#define GRISU3_EXPONENT_MASK            0x7FF0000000000000ULL
#define GRISU3_FRACTION_MASK            0x000FFFFFFFFFFFFFULL
#define GRISU3_IMPLICIT_ONE             0x0010000000000000ULL
#define GRISU3_EXPONENT_POSITION        52
#define GRISU3_EXPONENT_BIAS            1075
#define GRISU3_1_LOG2_10                0.30102999566398114 /* 1 / lg(10) */
#define GRISU3_MIN_TARGET_EXP           -60
#define GRISU3_32_BIT_MASK              0xFFFFFFFFULL
#define GRISU3_ENCODED_FLOAT_FRACT_SIZE 64
#define GRISU3_MIN_CACHED_EXP           -348
#define GRISU3_CACHED_EXP_STEP          8

/**
 * Encoded floating point type
 *
 * Note:
 *      Florian Loitsch: Printing Floating-point Numbers Quickly and Accurately with Integers.
 *      In Proceedings of the 31st ACM SIGPLAN Conference on Programming Language Design and
 *      Implementation (PLDI '10), pages 233-243.
 *      Toronto, Ontario, Canada, June 5-10, 2010.
 *      ACM (ISBN:978-1-4503-0019-3).
 *      (Henceforth: Florian Loitsh)
 *
 *      Florian Loitsh, Section 3.
 */
typedef struct
{
  uint64_t fract; /**< fraction */
  int exp; /**< exponent */
} encoded_float_t;

/**
 * Power type
 *
 * Note:
 *      Florian Loitsh, Section 4.
 */
typedef struct
{
  uint64_t fract; /**< fraction */
  int16_t bin_exp; /**< binary exponent */
  int16_t dec_exp; /**< decimal exponent */
} power_t;

#define POWER_CACHE_ITEM(fract, bin_exp, dec_exp) \
{ \
  fract, \
  bin_exp, \
  dec_exp \
}

static const power_t power_cache[] =
{
  POWER_CACHE_ITEM (0xfa8fd5a0081c0288ULL, -1220, -348),
  POWER_CACHE_ITEM (0xbaaee17fa23ebf76ULL, -1193, -340),
  POWER_CACHE_ITEM (0x8b16fb203055ac76ULL, -1166, -332),
  POWER_CACHE_ITEM (0xcf42894a5dce35eaULL, -1140, -324),
  POWER_CACHE_ITEM (0x9a6bb0aa55653b2dULL, -1113, -316),
  POWER_CACHE_ITEM (0xe61acf033d1a45dfULL, -1087, -308),
  POWER_CACHE_ITEM (0xab70fe17c79ac6caULL, -1060, -300),
  POWER_CACHE_ITEM (0xff77b1fcbebcdc4fULL, -1034, -292),
  POWER_CACHE_ITEM (0xbe5691ef416bd60cULL, -1007, -284),
  POWER_CACHE_ITEM (0x8dd01fad907ffc3cULL,  -980, -276),
  POWER_CACHE_ITEM (0xd3515c2831559a83ULL,  -954, -268),
  POWER_CACHE_ITEM (0x9d71ac8fada6c9b5ULL,  -927, -260),
  POWER_CACHE_ITEM (0xea9c227723ee8bcbULL,  -901, -252),
  POWER_CACHE_ITEM (0xaecc49914078536dULL,  -874, -244),
  POWER_CACHE_ITEM (0x823c12795db6ce57ULL,  -847, -236),
  POWER_CACHE_ITEM (0xc21094364dfb5637ULL,  -821, -228),
  POWER_CACHE_ITEM (0x9096ea6f3848984fULL,  -794, -220),
  POWER_CACHE_ITEM (0xd77485cb25823ac7ULL,  -768, -212),
  POWER_CACHE_ITEM (0xa086cfcd97bf97f4ULL,  -741, -204),
  POWER_CACHE_ITEM (0xef340a98172aace5ULL,  -715, -196),
  POWER_CACHE_ITEM (0xb23867fb2a35b28eULL,  -688, -188),
  POWER_CACHE_ITEM (0x84c8d4dfd2c63f3bULL,  -661, -180),
  POWER_CACHE_ITEM (0xc5dd44271ad3cdbaULL,  -635, -172),
  POWER_CACHE_ITEM (0x936b9fcebb25c996ULL,  -608, -164),
  POWER_CACHE_ITEM (0xdbac6c247d62a584ULL,  -582, -156),
  POWER_CACHE_ITEM (0xa3ab66580d5fdaf6ULL,  -555, -148),
  POWER_CACHE_ITEM (0xf3e2f893dec3f126ULL,  -529, -140),
  POWER_CACHE_ITEM (0xb5b5ada8aaff80b8ULL,  -502, -132),
  POWER_CACHE_ITEM (0x87625f056c7c4a8bULL,  -475, -124),
  POWER_CACHE_ITEM (0xc9bcff6034c13053ULL,  -449, -116),
  POWER_CACHE_ITEM (0x964e858c91ba2655ULL,  -422, -108),
  POWER_CACHE_ITEM (0xdff9772470297ebdULL,  -396, -100),
  POWER_CACHE_ITEM (0xa6dfbd9fb8e5b88fULL,  -369,  -92),
  POWER_CACHE_ITEM (0xf8a95fcf88747d94ULL,  -343,  -84),
  POWER_CACHE_ITEM (0xb94470938fa89bcfULL,  -316,  -76),
  POWER_CACHE_ITEM (0x8a08f0f8bf0f156bULL,  -289,  -68),
  POWER_CACHE_ITEM (0xcdb02555653131b6ULL,  -263,  -60),
  POWER_CACHE_ITEM (0x993fe2c6d07b7facULL,  -236,  -52),
  POWER_CACHE_ITEM (0xe45c10c42a2b3b06ULL,  -210,  -44),
  POWER_CACHE_ITEM (0xaa242499697392d3ULL,  -183,  -36),
  POWER_CACHE_ITEM (0xfd87b5f28300ca0eULL,  -157,  -28),
  POWER_CACHE_ITEM (0xbce5086492111aebULL,  -130,  -20),
  POWER_CACHE_ITEM (0x8cbccc096f5088ccULL,  -103,  -12),
  POWER_CACHE_ITEM (0xd1b71758e219652cULL,   -77,   -4),
  POWER_CACHE_ITEM (0x9c40000000000000ULL,   -50,    4),
  POWER_CACHE_ITEM (0xe8d4a51000000000ULL,   -24,   12),
  POWER_CACHE_ITEM (0xad78ebc5ac620000ULL,     3,   20),
  POWER_CACHE_ITEM (0x813f3978f8940984ULL,    30,   28),
  POWER_CACHE_ITEM (0xc097ce7bc90715b3ULL,    56,   36),
  POWER_CACHE_ITEM (0x8f7e32ce7bea5c70ULL,    83,   44),
  POWER_CACHE_ITEM (0xd5d238a4abe98068ULL,   109,   52),
  POWER_CACHE_ITEM (0x9f4f2726179a2245ULL,   136,   60),
  POWER_CACHE_ITEM (0xed63a231d4c4fb27ULL,   162,   68),
  POWER_CACHE_ITEM (0xb0de65388cc8ada8ULL,   189,   76),
  POWER_CACHE_ITEM (0x83c7088e1aab65dbULL,   216,   84),
  POWER_CACHE_ITEM (0xc45d1df942711d9aULL,   242,   92),
  POWER_CACHE_ITEM (0x924d692ca61be758ULL,   269,  100),
  POWER_CACHE_ITEM (0xda01ee641a708deaULL,   295,  108),
  POWER_CACHE_ITEM (0xa26da3999aef774aULL,   322,  116),
  POWER_CACHE_ITEM (0xf209787bb47d6b85ULL,   348,  124),
  POWER_CACHE_ITEM (0xb454e4a179dd1877ULL,   375,  132),
  POWER_CACHE_ITEM (0x865b86925b9bc5c2ULL,   402,  140),
  POWER_CACHE_ITEM (0xc83553c5c8965d3dULL,   428,  148),
  POWER_CACHE_ITEM (0x952ab45cfa97a0b3ULL,   455,  156),
  POWER_CACHE_ITEM (0xde469fbd99a05fe3ULL,   481,  164),
  POWER_CACHE_ITEM (0xa59bc234db398c25ULL,   508,  172),
  POWER_CACHE_ITEM (0xf6c69a72a3989f5cULL,   534,  180),
  POWER_CACHE_ITEM (0xb7dcbf5354e9beceULL,   561,  188),
  POWER_CACHE_ITEM (0x88fcf317f22241e2ULL,   588,  196),
  POWER_CACHE_ITEM (0xcc20ce9bd35c78a5ULL,   614,  204),
  POWER_CACHE_ITEM (0x98165af37b2153dfULL,   641,  212),
  POWER_CACHE_ITEM (0xe2a0b5dc971f303aULL,   667,  220),
  POWER_CACHE_ITEM (0xa8d9d1535ce3b396ULL,   694,  228),
  POWER_CACHE_ITEM (0xfb9b7cd9a4a7443cULL,   720,  236),
  POWER_CACHE_ITEM (0xbb764c4ca7a44410ULL,   747,  244),
  POWER_CACHE_ITEM (0x8bab8eefb6409c1aULL,   774,  252),
  POWER_CACHE_ITEM (0xd01fef10a657842cULL,   800,  260),
  POWER_CACHE_ITEM (0x9b10a4e5e9913129ULL,   827,  268),
  POWER_CACHE_ITEM (0xe7109bfba19c0c9dULL,   853,  276),
  POWER_CACHE_ITEM (0xac2820d9623bf429ULL,   880,  284),
  POWER_CACHE_ITEM (0x80444b5e7aa7cf85ULL,   907,  292),
  POWER_CACHE_ITEM (0xbf21e44003acdd2dULL,   933,  300),
  POWER_CACHE_ITEM (0x8e679c2f5e44ff8fULL,   960,  308),
  POWER_CACHE_ITEM (0xd433179d9c8cb841ULL,   986,  316),
  POWER_CACHE_ITEM (0x9e19db92b4e31ba9ULL,  1013,  324),
  POWER_CACHE_ITEM (0xeb96bf6ebadf77d9ULL,  1039,  332),
  POWER_CACHE_ITEM (0xaf87023b9bf0ee6bULL,  1066,  340)
};

/**
 * Get power from the chache
 *
 * Note:
 *      Florian Loitsh, Section 4.
 *
 * @return chached power
 */
static int
ecma_find_power_in_cache (int exp, /**< exponent */
                          encoded_float_t *fp_value) /**< float value */
{
  int k = (int) ceil ((exp + GRISU3_ENCODED_FLOAT_FRACT_SIZE - 1) * GRISU3_1_LOG2_10); /* k_computation */
  int i = (k - GRISU3_MIN_CACHED_EXP - 1) / GRISU3_CACHED_EXP_STEP + 1;
  fp_value->fract = power_cache[i].fract;
  fp_value->exp = power_cache[i].bin_exp;
  return power_cache[i].dec_exp;
} /* ecma_find_power_in_cache */

/**
 * Subtract two floating point values
 *
 * Note:
 *      Florian Loitsh, Figure 2/a.
 *
 * @return result of subtraction
 */
static encoded_float_t
ecma_subtract_encoded_fp (encoded_float_t fp_value_1, /**< first float value */
                          encoded_float_t fp_value_2) /**< second float value */
{
  JERRY_ASSERT (fp_value_1.exp == fp_value_2.exp && fp_value_1.fract >= fp_value_2.fract);

  encoded_float_t result;

  result.fract = fp_value_1.fract - fp_value_2.fract;
  result.exp = fp_value_1.exp;

  return result;
} /* ecma_subtract_encoded_fp */

/**
 * Multiply two floating point numbers
 *
 * Note:
 *      Florian Loitsh, Figure 2/b.
 *
 * @return product arithmetical
 */
static encoded_float_t
ecma_multiply_encoded_fp (encoded_float_t fp_value_1, /**< first float value */
                          encoded_float_t fp_value_2) /**< second float value */
{
  uint64_t a = fp_value_1.fract >> 32;
  uint64_t b = fp_value_1.fract & GRISU3_32_BIT_MASK;
  uint64_t c = fp_value_2.fract >> 32;
  uint64_t d = fp_value_2.fract & GRISU3_32_BIT_MASK;

  uint64_t ac = a * c;
  uint64_t bc = b * c;
  uint64_t ad = a * d;
  uint64_t bd = b * d;

  uint64_t tmp = (bd >> 32) + (ad & GRISU3_32_BIT_MASK) + (bc & GRISU3_32_BIT_MASK);

  tmp += 1U << 31; /* round */

  encoded_float_t result;

  result.fract = ac + (ad >> 32) + (bc >> 32) + (tmp >> 32);
  result.exp = fp_value_1.exp + fp_value_2.exp + 64;

  return result;
} /* ecma_multiply_encoded_fp */

/**
 * Normalize a floating point value
 *
 * @return normalized encoded_fp
 */
static encoded_float_t
ecma_normalize_encoded_fp (encoded_float_t fp_value) /**< float value */
{
  JERRY_ASSERT (fp_value.fract != 0);

  while (!(fp_value.fract & 0xFFC0000000000000ULL))
  {
    fp_value.fract <<= 10;
    fp_value.exp -= 10;
  }

  while (!(fp_value.fract & GRISU3_SIGN_BIT))
  {
    fp_value.fract <<= 1;
    --fp_value.exp;
  }

  return fp_value;
} /* ecma_normalize_encoded_fp */

/**
 * Convert ecma number to encoded_fp
 *
 * @return encoded_fp
 */
static inline encoded_float_t __attr_always_inline___
ecma_number_to_encoded_fp (ecma_number_t value) /**< ecma number */
{
  encoded_float_t fp;
  uint64_t *tmp = (uint64_t *) &value;
  uint64_t u64 = *tmp;

  if (!(u64 & GRISU3_EXPONENT_MASK))
  {
    fp.fract = u64 & GRISU3_FRACTION_MASK;
    fp.exp = 1 - GRISU3_EXPONENT_BIAS;
  }
  else
  {
    fp.fract = (u64 & GRISU3_FRACTION_MASK) + GRISU3_IMPLICIT_ONE;
    fp.exp = (int) ((u64 & GRISU3_EXPONENT_MASK) >> GRISU3_EXPONENT_POSITION) - GRISU3_EXPONENT_BIAS;
  }

  return fp;
} /* ecma_number_to_encoded_fp */

/**
 * Powers of 10
 */
static const uint32_t power_10[] =
{
  0u,
  1u,
  10u,
  100u,
  1000u,
  10000u,
  100000u,
  1000000u,
  10000000u,
  100000000u,
  1000000000u
};

/**
 * Get the largest possoble power
 *
 * @return power of 10
 */
static inline int __attr_always_inline___
ecma_largest_power (uint32_t n, /**< number */
                    int n_bits, /**< n_bits */
                    uint32_t *power) /**< [out] power of 10 */
{
  int guess = (((n_bits + 1) * 1233) >> 12) + 1;

  if (n < power_10[guess])
  {
    --guess; /* We don't have any guarantees that 2^n_bits <= n. */
  }

  *power = power_10[guess];

  return guess;
} /* ecma_largest_power */

/**
 * Try all possible numbers with the same leading length
 * and pick the one that is the closest.
 *
 * Note:
 *      Florian Loitsh, Figure 8.
 *
 * @return true - if success
 *         false - otherwise
 */
static inline bool __attr_always_inline___
ecma_grisu_round_weed (char *buffer, /**< [in, out] digits buffer */
                       int len, /**< number of digits */
                       uint64_t wp_w, /**< difference from positive neighbor */
                       uint64_t delta, /**< delta */
                       uint64_t rest, /**< positive neighbor minus one ulp */
                       uint64_t ten_kappa, /**< 10^K */
                       uint64_t ulp) /**< unit in the last place */
{
  uint64_t wp_w_up = wp_w - ulp;
  uint64_t wp_w_down = wp_w + ulp;

  while (rest < wp_w_up && delta - rest >= ten_kappa
         && (rest + ten_kappa < wp_w_up || wp_w_up - rest >= rest + ten_kappa - wp_w_up))
  {
    buffer[len - 1]--;
    rest += ten_kappa;
  }

  if (rest < wp_w_down && delta - rest >= ten_kappa
      && (rest + ten_kappa < wp_w_down || wp_w_down - rest > rest + ten_kappa - wp_w_down))
  {
    return false; /* failure */
  }

  return 2 * ulp <= rest && rest <= delta - 4 * ulp;
} /* ecma_grisu_round_weed */

/**
 * Digit generation routine of Grisu3
 *
 * @return true - if success
 *         false - otherwise
 */
static inline bool __attr_always_inline___
ecma_generate_digits (encoded_float_t low, /**< lower neighbor*/
                      encoded_float_t w, /**< number */
                      encoded_float_t high, /**< higher neighbor*/
                      char *buffer, /**< [out] digits buffer */
                      int *length, /**< [out] number of digits */
                      int *kappa) /**< [out] kappa */
{
  uint64_t unit = 1;
  encoded_float_t too_low = { low.fract - unit, low.exp };
  encoded_float_t too_high = { high.fract + unit, high.exp };
  encoded_float_t unsafe_interval = ecma_subtract_encoded_fp (too_high, too_low);
  encoded_float_t one = { 1ULL << -w.exp, w.exp };
  uint32_t part1 = (uint32_t) (too_high.fract >> -one.exp);
  uint64_t part2 = too_high.fract & (one.fract - 1);
  uint32_t div;
  *kappa = ecma_largest_power (part1, GRISU3_ENCODED_FLOAT_FRACT_SIZE + one.exp, &div);
  *length = 0;

  while (*kappa > 0)
  {
    uint64_t rest;
    uint8_t digit = (uint8_t) (part1 / div);
    buffer[(*length)++] = (char) ('0' + digit);
    part1 %= div;
    (*kappa)--;
    rest = ((uint64_t) part1 << -one.exp) + part2;

    if (rest < unsafe_interval.fract)
    {
      return ecma_grisu_round_weed (buffer,
                                    *length,
                                    ecma_subtract_encoded_fp (too_high, w).fract,
                                    unsafe_interval.fract,
                                    rest,
                                    (uint64_t) div << -one.exp,
                                    unit);
    }

    div /= 10;
  }

  while (true)
  {
    uint8_t digit;
    part2 *= 10;
    unit *= 10;
    unsafe_interval.fract *= 10;
    digit = (uint8_t) (part2 >> -one.exp); /* Integer division by one. */
    buffer[(*length)++] = (char) ('0' + digit);
    part2 &= one.fract - 1; /* Modulo by one. */
    (*kappa)--;

    if (part2 < unsafe_interval.fract)
    {
      return ecma_grisu_round_weed (buffer,
                                    *length,
                                    ecma_subtract_encoded_fp (too_high, w).fract * unit,
                                    unsafe_interval.fract,
                                    part2,
                                    one.fract,
                                    unit);
    }
  }
} /* ecma_generate_digits */

/**
 * Implementation of the "grisu3" double to string
 * conversion algorithm described in the research paper
 *
 * "Printing Floating-Point Numbers Quickly And Accurately with Integers"
 * by Florian Loitsch, available at
 * http://www.cs.tufts.edu/~nr/cs257/archive/florian-loitsch/printf.pdf
 *
 * @return true - if success
 *         false - otherwise
 */
static bool
ecma_grisu3 (ecma_number_t num, /**< input ecma number */
             char *buffer, /**< [out] digits buffer */
             int *length, /**< [out] number of digits */
             int *exp) /**< [out] exponent */
{
  int mk, kappa, success;
  encoded_float_t dfp = ecma_number_to_encoded_fp (num);
  encoded_float_t w = ecma_normalize_encoded_fp (dfp);

  /* normalize boundaries */
  encoded_float_t t = { (dfp.fract << 1) + 1, dfp.exp - 1 };
  encoded_float_t b_plus = ecma_normalize_encoded_fp (t);
  encoded_float_t b_minus;
  encoded_float_t c_mk; /* Cached power of ten: 10^-k */
  uint64_t *tmp_u64_p = (uint64_t *) &num;
  uint64_t u64 = *tmp_u64_p;

  /* Grisu only handles strictly positive finite numbers. */
  JERRY_ASSERT (num > 0 && num <= 1.7976931348623157e308);

  /* Is lower boundary closer? */
  if (!(u64 & GRISU3_FRACTION_MASK) && (u64 & GRISU3_EXPONENT_MASK) != 0)
  {
    b_minus.fract = (dfp.fract << 2) - 1;
    b_minus.exp =  dfp.exp - 2;
  }
  else
  {
    b_minus.fract = (dfp.fract << 1) - 1;
    b_minus.exp = dfp.exp - 1;
  }

  b_minus.fract = b_minus.fract << (b_minus.exp - b_plus.exp);
  b_minus.exp = b_plus.exp;

  mk = ecma_find_power_in_cache (GRISU3_MIN_TARGET_EXP - GRISU3_ENCODED_FLOAT_FRACT_SIZE - w.exp, &c_mk);

  w = ecma_multiply_encoded_fp (w, c_mk);
  b_minus = ecma_multiply_encoded_fp (b_minus, c_mk);
  b_plus = ecma_multiply_encoded_fp (b_plus,  c_mk);

  success = ecma_generate_digits (b_minus, w, b_plus, buffer, length, &kappa);
  *exp = kappa - mk;

  return success;
} /* ecma_grisu3 */

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
                        uint64_t *out_digits_p, /**< [out] digits */
                        int32_t *out_digits_num_p, /**< [out] number of digits */
                        int32_t *out_decimal_exp_p) /**< [out] decimal exponent */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));
  JERRY_ASSERT (!ecma_number_is_zero (num));
  JERRY_ASSERT (!ecma_number_is_infinity (num));
  JERRY_ASSERT (!ecma_number_is_negative (num));

#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
  char str[32];
  int len;
  int exp;

  /*
   * TODO:
   *      Add fallback mechanism, when grisu3 fails. There is no more accurate algorithm
   *      in JerryScript yet.
   */
  ecma_grisu3 (num, str, &len, &exp);

  *out_digits_p = 0;

  /*
   * TODO:
   *      Change return value to 'const char*' and update the call sites. (Number.prototype.toString,
   *      Number.prototype.toFixed, Number.prototype.toExponential, and Number.prototype.toPrecision).
   */
  for (int i = 0; i < len; i++)
  {
    *out_digits_p *= 10;
    *out_digits_p += (uint64_t) (str[i] - '0');
  }

  *out_digits_num_p = len;
  *out_decimal_exp_p = len + exp;

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
  const uint32_t frac_part_mask = ((((uint32_t) 1) << int_part_shift) - 1);

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
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */
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

      JERRY_ASSERT (dst_p <= buffer_p + buffer_size);
      size = (lit_utf8_size_t) (dst_p - buffer_p);
    }
    else if (ecma_number_is_negative (num))
    {
      // 3.
      *dst_p++ = minus_char;
      lit_utf8_size_t new_buffer_size = (lit_utf8_size_t) ((buffer_p + buffer_size) - dst_p);
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

      if (((ecma_number_t) num_uint32) == num)
      {
        size = ecma_uint32_to_utf8_string (num_uint32, dst_p, buffer_size);
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
          JERRY_ASSERT (dst_p <= buffer_p + buffer_size);

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
          JERRY_ASSERT (dst_p <= buffer_p + buffer_size);

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
          JERRY_ASSERT (dst_p <= buffer_p + buffer_size);

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
            JERRY_ASSERT (dst_p <= buffer_p + buffer_size);

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
          JERRY_ASSERT (dst_p + 2 <= buffer_p + buffer_size);
          *dst_p++ = e_chars[0];
          *dst_p++ = (n >= 1) ? plus_char : minus_char;
          int32_t t = (n >= 1) ? (n - 1) : -(n - 1);

          if (t == 0)
          {
            JERRY_ASSERT (dst_p <= buffer_p + buffer_size);
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
              JERRY_ASSERT (dst_p + 1 <= buffer_p + buffer_size);
              *dst_p++ = digits[t / t_mod];

              t -= (t / t_mod) * t_mod;
              t_mod /= 10;
            }
          }

          JERRY_ASSERT (dst_p <= buffer_p + buffer_size);
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

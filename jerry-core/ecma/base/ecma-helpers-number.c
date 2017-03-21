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

#include "ecma-globals.h"
#include "ecma-helpers.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

JERRY_STATIC_ASSERT (sizeof (ecma_value_t) == sizeof (ecma_integer_value_t),
                     size_of_ecma_value_t_must_be_equal_to_the_size_of_ecma_integer_value_t);

JERRY_STATIC_ASSERT (ECMA_DIRECT_SHIFT == ECMA_VALUE_SHIFT + 1,
                     currently_directly_encoded_values_has_one_extra_flag);

JERRY_STATIC_ASSERT (((1 << (ECMA_DIRECT_SHIFT - 1)) | ECMA_TYPE_DIRECT) == ECMA_DIRECT_TYPE_SIMPLE_VALUE,
                     currently_directly_encoded_values_start_after_direct_type_simple_value);

#define ECMA_NUMBER_SIGN_POS (ECMA_NUMBER_FRACTION_WIDTH + \
                              ECMA_NUMBER_BIASED_EXP_WIDTH)

#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
JERRY_STATIC_ASSERT (sizeof (ecma_number_t) == sizeof (uint32_t),
                     size_of_ecma_number_t_must_be_equal_to_4_bytes);

/**
 * Packing sign, fraction and biased exponent to ecma-number
 *
 * @return ecma-number with specified sign, biased_exponent and fraction
 */
static ecma_number_t
ecma_number_pack (bool sign, /**< sign */
                  uint32_t biased_exp, /**< biased exponent */
                  uint64_t fraction) /**< fraction */
{
  JERRY_ASSERT ((biased_exp & ~((1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1)) == 0);
  JERRY_ASSERT ((fraction & ~((1ull << ECMA_NUMBER_FRACTION_WIDTH) - 1)) == 0);

  uint32_t packed_value = (((sign ? 1u : 0u) << ECMA_NUMBER_SIGN_POS) |
                           (biased_exp << ECMA_NUMBER_FRACTION_WIDTH) |
                           ((uint32_t) fraction));

  union
  {
    uint32_t u32_value;
    ecma_number_t float_value;
  } u;

  u.u32_value = packed_value;

  return u.float_value;
} /* ecma_number_pack */

/**
 * Unpacking sign, fraction and biased exponent from ecma-number
 */
static void
ecma_number_unpack (ecma_number_t num, /**< ecma-number */
                    bool *sign_p, /**< [out] sign (optional) */
                    uint32_t *biased_exp_p, /**< [out] biased exponent (optional) */
                    uint64_t *fraction_p) /**< [out] fraction (optional) */
{
  union
  {
    uint32_t u32_value;
    ecma_number_t float_value;
  } u;

  u.float_value = num;

  uint32_t packed_value = u.u32_value;

  if (sign_p != NULL)
  {
    *sign_p = ((packed_value >> ECMA_NUMBER_SIGN_POS) != 0);
  }

  if (biased_exp_p != NULL)
  {
    *biased_exp_p = (((packed_value) & ~(1u << ECMA_NUMBER_SIGN_POS)) >> ECMA_NUMBER_FRACTION_WIDTH);
  }

  if (fraction_p != NULL)
  {
    *fraction_p = (packed_value & ((1u << ECMA_NUMBER_FRACTION_WIDTH) - 1));
  }
} /* ecma_number_unpack */

/**
 * Value used to calculate exponent from biased exponent
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
const int32_t ecma_number_exponent_bias = 127;

#elif CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
JERRY_STATIC_ASSERT (sizeof (ecma_number_t) == sizeof (uint64_t),
                     size_of_ecma_number_t_must_be_equal_to_8_bytes);

/**
 * Packing sign, fraction and biased exponent to ecma-number
 *
 * @return ecma-number with specified sign, biased_exponent and fraction
 */
static ecma_number_t
ecma_number_pack (bool sign, /**< sign */
                  uint32_t biased_exp, /**< biased exponent */
                  uint64_t fraction) /**< fraction */
{
  uint64_t packed_value = (((sign ? 1ull : 0ull) << ECMA_NUMBER_SIGN_POS) |
                           (((uint64_t) biased_exp) << ECMA_NUMBER_FRACTION_WIDTH) |
                           fraction);

  JERRY_ASSERT ((biased_exp & ~((1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1)) == 0);
  JERRY_ASSERT ((fraction & ~((1ull << ECMA_NUMBER_FRACTION_WIDTH) - 1)) == 0);

  union
  {
    uint64_t u64_value;
    ecma_number_t float_value;
  } u;

  u.u64_value = packed_value;

  return u.float_value;
} /* ecma_number_pack */

/**
 * Unpacking sign, fraction and biased exponent from ecma-number
 */
static void
ecma_number_unpack (ecma_number_t num, /**< ecma-number */
                    bool *sign_p, /**< [out] sign (optional) */
                    uint32_t *biased_exp_p, /**< [out] biased exponent (optional) */
                    uint64_t *fraction_p) /**< [out] fraction (optional) */
{
  union
  {
    uint64_t u64_value;
    ecma_number_t float_value;
  } u;
  u.float_value = num;

  uint64_t packed_value = u.u64_value;

  if (sign_p != NULL)
  {
    *sign_p = ((packed_value >> ECMA_NUMBER_SIGN_POS) != 0);
  }

  if (biased_exp_p != NULL)
  {
    *biased_exp_p = (uint32_t) (((packed_value) & ~(1ull << ECMA_NUMBER_SIGN_POS)) >> ECMA_NUMBER_FRACTION_WIDTH);
  }

  if (fraction_p != NULL)
  {
    *fraction_p = (packed_value & ((1ull << ECMA_NUMBER_FRACTION_WIDTH) - 1));
  }
} /* ecma_number_unpack */

/**
 * Value used to calculate exponent from biased exponent
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
const int32_t ecma_number_exponent_bias = 1023;

#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32 */

/**
 * Get fraction of number
 *
 * @return normalized fraction field of number
 */
static uint64_t
ecma_number_get_fraction_field (ecma_number_t num) /**< ecma-number */
{
  uint64_t fraction;

  ecma_number_unpack (num, NULL, NULL, &fraction);

  return fraction;
} /* ecma_number_get_fraction_field */

/**
 * Get exponent of number
 *
 * @return exponent corresponding to normalized fraction of number
 */
static uint32_t
ecma_number_get_biased_exponent_field (ecma_number_t num) /**< ecma-number */
{
  uint32_t biased_exp;

  ecma_number_unpack (num, NULL, &biased_exp, NULL);

  return biased_exp;
} /* ecma_number_get_biased_exponent_field */

/**
 * Get sign bit of number
 *
 * @return 0 or 1 - value of sign bit
 */
static uint32_t
ecma_number_get_sign_field (ecma_number_t num) /**< ecma-number */
{
  bool sign;

  ecma_number_unpack (num, &sign, NULL, NULL);

  return sign;
} /* ecma_number_get_sign_field */

/**
 * Check if ecma-number is NaN
 *
 * @return true - if biased exponent is filled with 1 bits and
                  fraction is filled with anything but not all zero bits,
 *         false - otherwise
 */
inline bool __attr_always_inline___
ecma_number_is_nan (ecma_number_t num) /**< ecma-number */
{
  bool is_nan = (num != num);

#ifndef JERRY_NDEBUG
  uint32_t biased_exp = ecma_number_get_biased_exponent_field (num);
  uint64_t fraction = ecma_number_get_fraction_field (num);

   /* IEEE-754 2008, 3.4, a */
  bool is_nan_ieee754 = ((biased_exp == (1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1)
                         && (fraction != 0));

  JERRY_ASSERT (is_nan == is_nan_ieee754);
#endif /* !JERRY_NDEBUG */

  return is_nan;
} /* ecma_number_is_nan */

/**
 * Make a NaN.
 *
 * @return NaN value
 */
ecma_number_t
ecma_number_make_nan (void)
{
  return ecma_number_pack (false,
                           (1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1u,
                           1u);
} /* ecma_number_make_nan */

/**
 * Make an Infinity.
 *
 * @return if !sign - +Infinity value,
 *         else - -Infinity value.
 */
ecma_number_t
ecma_number_make_infinity (bool sign) /**< true - for negative Infinity,
                                           false - for positive Infinity */
{
  return ecma_number_pack (sign,
                           (1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1u,
                           0u);
} /* ecma_number_make_infinity */

/**
 * Check if ecma-number is negative
 *
 * @return true - if sign bit of ecma-number is set
 *         false - otherwise
 */
inline bool __attr_always_inline___
ecma_number_is_negative (ecma_number_t num) /**< ecma-number */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));

  /* IEEE-754 2008, 3.4 */
  return (ecma_number_get_sign_field (num) != 0);
} /* ecma_number_is_negative */

/**
 * Check if ecma-number is zero
 *
 * @return true - if fraction is zero and biased exponent is zero,
 *         false - otherwise
 */
bool
ecma_number_is_zero (ecma_number_t num) /**< ecma-number */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));

  bool is_zero = (num == ECMA_NUMBER_ZERO);

#ifndef JERRY_NDEBUG
  /* IEEE-754 2008, 3.4, e */
  bool is_zero_ieee754 = (ecma_number_get_fraction_field (num) == 0
                          && ecma_number_get_biased_exponent_field (num) == 0);

  JERRY_ASSERT (is_zero == is_zero_ieee754);
#endif /* !JERRY_NDEBUG */

  return is_zero;
} /* ecma_number_is_zero */

/**
 * Check if number is infinity
 *
 * @return true - if biased exponent is filled with 1 bits and
 *                fraction is filled with zero bits,
 *         false - otherwise
 */
bool
ecma_number_is_infinity (ecma_number_t num) /**< ecma-number */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));

  uint32_t biased_exp = ecma_number_get_biased_exponent_field (num);
  uint64_t fraction = ecma_number_get_fraction_field (num);

  /* IEEE-754 2008, 3.4, b */
  return ((biased_exp  == (1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1)
          && (fraction == 0));
} /* ecma_number_is_infinity */

/**
 * Get fraction and exponent of the number
 *
 * @return shift of dot in the fraction
 */
int32_t
ecma_number_get_fraction_and_exponent (ecma_number_t num, /**< ecma-number */
                                       uint64_t *out_fraction_p, /**< [out] fraction of the number */
                                       int32_t *out_exponent_p) /**< [out] exponent of the number */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));

  uint32_t biased_exp = ecma_number_get_biased_exponent_field (num);
  uint64_t fraction = ecma_number_get_fraction_field (num);
  int32_t exponent;

  if (unlikely (biased_exp == 0))
  {
    /* IEEE-754 2008, 3.4, d */
    if (ecma_number_is_zero (num))
    {
      exponent = -ecma_number_exponent_bias;
    }
    else
    {
      exponent = 1 - ecma_number_exponent_bias;

      while (!(fraction & (1ull << ECMA_NUMBER_FRACTION_WIDTH)))
      {
        JERRY_ASSERT (fraction != 0);

        fraction <<= 1;
        exponent--;
      }
    }
  }
  else if (ecma_number_is_infinity (num))
  {
    /* The fraction and exponent should round to infinity */
    exponent = (int32_t) biased_exp - ecma_number_exponent_bias;

    JERRY_ASSERT ((fraction & (1ull << ECMA_NUMBER_FRACTION_WIDTH)) == 0);
    fraction |= 1ull << ECMA_NUMBER_FRACTION_WIDTH;
  }
  else
  {
    /* IEEE-754 2008, 3.4, c */
    exponent = (int32_t) biased_exp - ecma_number_exponent_bias;

    JERRY_ASSERT (biased_exp > 0 && biased_exp < (1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1);
    JERRY_ASSERT ((fraction & (1ull << ECMA_NUMBER_FRACTION_WIDTH)) == 0);
    fraction |= 1ull << ECMA_NUMBER_FRACTION_WIDTH;
  }

  *out_fraction_p = fraction;
  *out_exponent_p = exponent;
  return ECMA_NUMBER_FRACTION_WIDTH;
} /* ecma_number_get_fraction_and_exponent */

/**
 * Make normalised positive Number from given fraction and exponent
 *
 * @return ecma-number
 */
ecma_number_t
ecma_number_make_normal_positive_from_fraction_and_exponent (uint64_t fraction, /**< fraction */
                                                             int32_t exponent) /**< exponent */
{
  uint32_t biased_exp = (uint32_t) (exponent + ecma_number_exponent_bias);
  JERRY_ASSERT (biased_exp > 0 && biased_exp < (1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1);
  JERRY_ASSERT ((fraction & ~((1ull << (ECMA_NUMBER_FRACTION_WIDTH + 1)) - 1)) == 0);
  JERRY_ASSERT ((fraction & (1ull << ECMA_NUMBER_FRACTION_WIDTH)) != 0);

  return ecma_number_pack (false,
                           biased_exp,
                           fraction & ~(1ull << ECMA_NUMBER_FRACTION_WIDTH));
} /* ecma_number_make_normal_positive_from_fraction_and_exponent */

/**
 * Make Number of given sign from given mantissa value and binary exponent
 *
 * @return ecma-number (possibly Infinity of specified sign)
 */
ecma_number_t
ecma_number_make_from_sign_mantissa_and_exponent (bool sign, /**< true - for negative sign,
                                                                  false - for positive sign */
                                                  uint64_t mantissa, /**< mantissa */
                                                  int32_t exponent) /**< binary exponent */
{
  /* Rounding mantissa to fit into fraction field width */
  if (mantissa & ~((1ull << (ECMA_NUMBER_FRACTION_WIDTH + 1)) - 1))
  {
    /* Rounded mantissa looks like the following: |00...0|1|fraction_width mantissa bits| */
    while ((mantissa & ~((1ull << (ECMA_NUMBER_FRACTION_WIDTH + 1)) - 1)) != 0)
    {
      uint64_t rightmost_bit = (mantissa & 1);

      exponent++;
      mantissa >>= 1;

      if ((mantissa & ~((1ull << (ECMA_NUMBER_FRACTION_WIDTH + 1)) - 1)) == 0)
      {
        /* Rounding to nearest value */
        mantissa += rightmost_bit;

        /* In the first case loop is finished,
           and in the second - just one shift follows and then loop finishes */
        JERRY_ASSERT (((mantissa & ~((1ull << (ECMA_NUMBER_FRACTION_WIDTH + 1)) - 1)) == 0)
                      || (mantissa == (1ull << (ECMA_NUMBER_FRACTION_WIDTH + 1))));
      }
    }
  }

  /* Normalizing mantissa */
  while (mantissa != 0
         && ((mantissa & (1ull << ECMA_NUMBER_FRACTION_WIDTH)) == 0))
  {
    exponent--;
    mantissa <<= 1;
  }

  /* Moving floating point */
  exponent += ECMA_NUMBER_FRACTION_WIDTH - 1;

  int32_t biased_exp_signed = exponent + ecma_number_exponent_bias;

  if (biased_exp_signed < 1)
  {
    /* Denormalizing mantissa if biased_exponent is less than zero */
    while (biased_exp_signed < 0)
    {
      biased_exp_signed++;
      mantissa >>= 1;
    }

    /* Rounding to nearest value */
    mantissa += 1;
    mantissa >>= 1;

    /* Encoding denormalized exponent */
    biased_exp_signed = 0;
  }
  else
  {
    /* Clearing highest mantissa bit that should have been non-zero if mantissa is non-zero */
    mantissa &= ~(1ull << ECMA_NUMBER_FRACTION_WIDTH);
  }

  uint32_t biased_exp = (uint32_t) biased_exp_signed;

  if (biased_exp >= ((1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1))
  {
    return ecma_number_make_infinity (sign);
  }

  JERRY_ASSERT (biased_exp < (1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1);
  JERRY_ASSERT ((mantissa & ~((1ull << ECMA_NUMBER_FRACTION_WIDTH) - 1)) == 0);

  return ecma_number_pack (sign,
                           biased_exp,
                           mantissa);
} /* ecma_number_make_from_sign_mantissa_and_exponent */

/**
 * Get previous representable ecma-number
 *
 * @return maximum ecma-number that is less compared to passed argument
 */
ecma_number_t
ecma_number_get_prev (ecma_number_t num) /**< ecma-number */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));
  JERRY_ASSERT (!ecma_number_is_zero (num));

  if (ecma_number_is_negative (num))
  {
    return ecma_number_negate (ecma_number_get_next (num));
  }

  uint32_t biased_exp = ecma_number_get_biased_exponent_field (num);
  uint64_t fraction = ecma_number_get_fraction_field (num);

  if (fraction == 0 && biased_exp != 0)
  {
    fraction = (1ull << ECMA_NUMBER_FRACTION_WIDTH) - 1;

    biased_exp--;
  }
  else
  {
    fraction--;
  }

  return ecma_number_pack (false,
                           biased_exp,
                           fraction);
} /* ecma_number_get_prev */

/**
 * Get next representable ecma-number
 *
 * @return minimum ecma-number that is greater compared to passed argument
 */
ecma_number_t
ecma_number_get_next (ecma_number_t num) /**< ecma-number */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));
  JERRY_ASSERT (!ecma_number_is_infinity (num));

  if (ecma_number_is_negative (num))
  {
    return ecma_number_negate (ecma_number_get_prev (num));
  }

  uint32_t biased_exp = ecma_number_get_biased_exponent_field (num);
  uint64_t fraction = ecma_number_get_fraction_field (num);

  fraction |= (1ull << ECMA_NUMBER_FRACTION_WIDTH);

  fraction++;

  if ((fraction & (1ull << ECMA_NUMBER_FRACTION_WIDTH)) == 0)
  {
    fraction >>= 1;

    biased_exp++;
  }

  JERRY_ASSERT (fraction & (1ull << ECMA_NUMBER_FRACTION_WIDTH));

  fraction &= ~(1ull << ECMA_NUMBER_FRACTION_WIDTH);

  return ecma_number_pack (false,
                           biased_exp,
                           fraction);
} /* ecma_number_get_next */

/**
 * Negate ecma-number
 *
 * @return negated number
 */
ecma_number_t
ecma_number_negate (ecma_number_t num) /**< ecma-number */
{
  ecma_number_t negated = -num;

#ifndef JERRY_NDEBUG
  bool sign;
  uint32_t biased_exp;
  uint64_t fraction;

  ecma_number_unpack (num, &sign, &biased_exp, &fraction);

  sign = !sign;

  ecma_number_t negated_ieee754 = ecma_number_pack (sign, biased_exp, fraction);

  JERRY_ASSERT (negated == negated_ieee754
                || (ecma_number_is_nan (negated)
                    && ecma_number_is_nan (negated_ieee754)));
#endif /* !JERRY_NDEBUG */

  return negated;
} /* ecma_number_negate */

/**
 * Truncate fractional part of the number
 *
 * @return integer part of the number
 */
ecma_number_t
ecma_number_trunc (ecma_number_t num) /**< ecma-number */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));

  uint64_t fraction;
  int32_t exponent;
  const int32_t dot_shift = ecma_number_get_fraction_and_exponent (num, &fraction, &exponent);
  const bool sign = ecma_number_is_negative (num);

  if (exponent < 0)
  {
    return ECMA_NUMBER_ZERO;
  }
  else if (exponent < dot_shift)
  {
    fraction &= ~((1ull << (dot_shift - exponent)) - 1);

    ecma_number_t tmp = ecma_number_make_normal_positive_from_fraction_and_exponent (fraction,
                                                                                     exponent);
    if (sign)
    {
      return ecma_number_negate (tmp);
    }
    else
    {
      return tmp;
    }
  }
  else
  {
    return num;
  }
} /* ecma_number_trunc */

/**
 * Calculate remainder of division of two numbers,
 * as specified in ECMA-262 v5, 11.5.3, item 6.
 *
 * Note:
 *      operands shouldn't contain NaN, Infinity, or zero.
 *
 * @return number - calculated remainder.
 */
ecma_number_t
ecma_number_calc_remainder (ecma_number_t left_num, /**< left operand */
                            ecma_number_t right_num) /**< right operand */
{
  JERRY_ASSERT (!ecma_number_is_nan (left_num)
                && !ecma_number_is_zero (left_num)
                && !ecma_number_is_infinity (left_num));
  JERRY_ASSERT (!ecma_number_is_nan (right_num)
                && !ecma_number_is_zero (right_num)
                && !ecma_number_is_infinity (right_num));

  const ecma_number_t q = ecma_number_trunc (ecma_number_divide (left_num, right_num));
  ecma_number_t r = ecma_number_substract (left_num, ecma_number_multiply (right_num, q));

  if (ecma_number_is_zero (r)
      && ecma_number_is_negative (left_num))
  {
    r = ecma_number_negate (r);
  }

  return r;
} /* ecma_number_calc_remainder */

/**
 * ECMA-number addition.
 *
 * @return number - result of addition.
 */
ecma_number_t
ecma_number_add (ecma_number_t left_num, /**< left operand */
                 ecma_number_t right_num) /**< right operand */
{
  return left_num + right_num;
} /* ecma_number_add */

/**
 * ECMA-number substraction.
 *
 * @return number - result of substraction.
 */
ecma_number_t
ecma_number_substract (ecma_number_t left_num, /**< left operand */
                       ecma_number_t right_num) /**< right operand */
{
  return ecma_number_add (left_num, ecma_number_negate (right_num));
} /* ecma_number_substract */

/**
 * ECMA-number multiplication.
 *
 * @return number - result of multiplication.
 */
ecma_number_t
ecma_number_multiply (ecma_number_t left_num, /**< left operand */
                      ecma_number_t right_num) /**< right operand */
{
  return left_num * right_num;
} /* ecma_number_multiply */

/**
 * ECMA-number division.
 *
 * @return number - result of division.
 */
ecma_number_t
ecma_number_divide (ecma_number_t left_num, /**< left operand */
                    ecma_number_t right_num) /**< right operand */
{
  return left_num / right_num;
} /* ecma_number_divide */

/**
 * @}
 * @}
 */

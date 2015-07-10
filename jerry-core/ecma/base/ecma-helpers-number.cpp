/* Copyright 2014 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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

#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
JERRY_STATIC_ASSERT (sizeof (ecma_number_t) == sizeof (uint32_t));

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
  const uint32_t fraction_pos = 0;
  const uint32_t biased_exp_pos = fraction_pos + ECMA_NUMBER_FRACTION_WIDTH;
  const uint32_t sign_pos = biased_exp_pos + ECMA_NUMBER_BIASED_EXP_WIDTH;

  JERRY_ASSERT ((biased_exp & ~((1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1)) == 0);
  JERRY_ASSERT ((fraction & ~((1ull << ECMA_NUMBER_FRACTION_WIDTH) - 1)) == 0);

  uint32_t packed_value = (((sign ? 1u : 0u) << sign_pos) |
                           (biased_exp << biased_exp_pos) |
                           (((uint32_t) fraction) << fraction_pos));

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
                    bool *sign_p, /**< optional out: sign */
                    uint32_t *biased_exp_p, /**< optional out: biased exponent */
                    uint64_t *fraction_p) /**< optional out: fraction */
{
  const uint32_t fraction_pos = 0;
  const uint32_t biased_exp_pos = fraction_pos + ECMA_NUMBER_FRACTION_WIDTH;
  const uint32_t sign_pos = biased_exp_pos + ECMA_NUMBER_BIASED_EXP_WIDTH;

  union
  {
    uint32_t u32_value;
    ecma_number_t float_value;
  } u;

  u.float_value = num;

  uint32_t packed_value = u.u32_value;

  if (sign_p != NULL)
  {
    *sign_p = ((packed_value >> sign_pos) != 0);
  }

  if (biased_exp_p != NULL)
  {
    *biased_exp_p = (((packed_value) & ~(1u << sign_pos)) >> biased_exp_pos);
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

/**
 * Relative precision used in calculation with ecma-numbers
 */
const ecma_number_t ecma_number_relative_eps = 1.0e-10f;
#elif CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
JERRY_STATIC_ASSERT (sizeof (ecma_number_t) == sizeof (uint64_t));

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
  const uint32_t fraction_pos = 0;
  const uint32_t biased_exp_pos = fraction_pos + ECMA_NUMBER_FRACTION_WIDTH;
  const uint32_t sign_pos = biased_exp_pos + ECMA_NUMBER_BIASED_EXP_WIDTH;

  uint64_t packed_value = (((sign ? 1ull : 0ull) << sign_pos) |
                           (((uint64_t) biased_exp) << biased_exp_pos) |
                           (fraction << fraction_pos));

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
                    bool *sign_p, /**< optional out: sign */
                    uint32_t *biased_exp_p, /**< optional out: biased exponent */
                    uint64_t *fraction_p) /**< optional out: fraction */
{
  const uint32_t fraction_pos = 0;
  const uint32_t biased_exp_pos = fraction_pos + ECMA_NUMBER_FRACTION_WIDTH;
  const uint32_t sign_pos = biased_exp_pos + ECMA_NUMBER_BIASED_EXP_WIDTH;

  union
  {
    uint64_t u64_value;
    ecma_number_t float_value;
  } u;
  u.float_value = num;

  uint64_t packed_value = u.u64_value;

  if (sign_p != NULL)
  {
    *sign_p = ((packed_value >> sign_pos) != 0);
  }

  if (biased_exp_p != NULL)
  {
    *biased_exp_p = (uint32_t) (((packed_value) & ~(1ull << sign_pos)) >> biased_exp_pos);
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

/**
 * Relative precision used in calculation with ecma-numbers
 */
const ecma_number_t ecma_number_relative_eps = 1.0e-16;
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */

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
bool
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
bool
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
 *         false - otherwise.
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
                                       uint64_t *out_fraction_p, /**< out: fraction of the number */
                                       int32_t *out_exponent_p) /**< out: exponent of the number */
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
  uint64_t fraction;
  int32_t exponent;
  const int32_t dot_shift = ecma_number_get_fraction_and_exponent (num, &fraction, &exponent);
  const bool sign = ecma_number_is_negative (num);

  if (exponent < 0)
  {
    return 0;
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
  ecma_number_t n = left_num, d = right_num;

  JERRY_ASSERT (!ecma_number_is_nan (n)
                && !ecma_number_is_zero (n)
                && !ecma_number_is_infinity (n));
  JERRY_ASSERT (!ecma_number_is_nan (d)
                && !ecma_number_is_zero (d)
                && !ecma_number_is_infinity (d));

  ecma_number_t q = ecma_number_trunc (ecma_number_divide (n, d));

  ecma_number_t r = ecma_number_substract (n, ecma_number_multiply (d, q));

  if (ecma_number_is_zero (r)
      && ecma_number_is_negative (n))
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
 * Helper for calculating absolute value
 *
 * Warning:
 *         argument should be valid number
 *
 * @return absolute value of the argument
 */
ecma_number_t
ecma_number_abs (ecma_number_t num) /**< valid number */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));

  if (num < 0)
  {
    return ecma_number_negate (num);
  }
  else
  {
    return num;
  }
} /* ecma_number_abs */

/**
 * Helper for calculating square root using Newton's method.
 *
 * @return square root of specified number
 */
ecma_number_t
ecma_number_sqrt (ecma_number_t num) /**< valid finite
                                          positive number */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));
  JERRY_ASSERT (!ecma_number_is_infinity (num));
  JERRY_ASSERT (!ecma_number_is_negative (num));

  ecma_number_t x = ECMA_NUMBER_ONE;
  ecma_number_t diff = ecma_number_make_infinity (false);

  while (ecma_number_divide (diff, x) > ecma_number_relative_eps)
  {
    ecma_number_t x_next = ecma_number_multiply (ECMA_NUMBER_HALF,
                                                 (ecma_number_add (x,
                                                                   ecma_number_divide (num, x))));

    diff = ecma_number_substract (x, x_next);
    if (diff < 0)
    {
      diff = ecma_number_negate (diff);
    }

    x = x_next;
  }

  return x;
} /* ecma_number_sqrt */

/**
 * Helper for calculating natural logarithm.
 *
 * @return natural logarithm of specified number
 */
ecma_number_t
ecma_number_ln (ecma_number_t num) /**< valid finite
                                        positive number */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));
  JERRY_ASSERT (!ecma_number_is_infinity (num));
  JERRY_ASSERT (!ecma_number_is_negative (num));

  if (num == ECMA_NUMBER_ONE)
  {
    return ECMA_NUMBER_ZERO;
  }

  /* Taylor series of ln (1 + x) around x = 0 is x - x^2/2 + x^3/3 - x^4/4 + ... */

  ecma_number_t x = num;
  ecma_number_t multiplier = ECMA_NUMBER_ONE;

  while (ecma_number_abs (ecma_number_substract (x,
                                                 ECMA_NUMBER_ONE)) > ECMA_NUMBER_HALF)
  {
    x = ecma_number_sqrt (x);
    multiplier = ecma_number_multiply (multiplier, ECMA_NUMBER_TWO);
  }

  x = ecma_number_substract (x, ECMA_NUMBER_ONE);

  ecma_number_t sum = ECMA_NUMBER_ZERO;
  ecma_number_t next_power = x;
  ecma_number_t next_divisor = ECMA_NUMBER_ONE;

  ecma_number_t diff;

  do
  {
    ecma_number_t next_sum = ecma_number_add (sum,
                                              ecma_number_divide (next_power,
                                                                  next_divisor));

    next_divisor = ecma_number_add (next_divisor, ECMA_NUMBER_ONE);
    next_power = ecma_number_multiply (next_power, x);
    next_power = ecma_number_negate (next_power);

    diff = ecma_number_abs (ecma_number_substract (sum, next_sum));

    sum = next_sum;
  }
  while (ecma_number_abs (ecma_number_divide (diff,
                                              sum)) > ecma_number_relative_eps);

  sum = ecma_number_multiply (sum, multiplier);

  return sum;
} /* ecma_number_ln */

/**
 * Helper for calculating exponent of a number
 *
 * @return exponent of specified number
 */
ecma_number_t
ecma_number_exp (ecma_number_t num) /**< valid finite number */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));
  JERRY_ASSERT (!ecma_number_is_infinity (num));

  bool invert = false;
  ecma_number_t pow_e;

  if (ecma_number_is_negative (num))
  {
    invert = true;
    pow_e = ecma_number_negate (num);
  }
  else
  {
    pow_e = num;
  }

  /* Taylor series of e^x is 1 + x/1! + x^2/2! + x^3/3! + ... + x^n/n! + ... */

  ecma_number_t sum = ECMA_NUMBER_ONE;
  ecma_number_t next_addendum = ecma_number_divide (pow_e, ECMA_NUMBER_ONE);
  ecma_number_t next_factorial_factor = ECMA_NUMBER_ONE;

  ecma_number_t diff = ecma_number_make_infinity (false);

  while (ecma_number_divide (diff, sum) > ecma_number_relative_eps)
  {
    ecma_number_t next_sum = ecma_number_add (sum, next_addendum);

    next_factorial_factor = ecma_number_add (next_factorial_factor, ECMA_NUMBER_ONE);
    next_addendum = ecma_number_multiply (next_addendum, pow_e);
    next_addendum = ecma_number_divide (next_addendum, next_factorial_factor);

    diff = ecma_number_substract (sum, next_sum);
    if (diff < 0)
    {
      diff = ecma_number_negate (diff);
    }

    sum = next_sum;
  }

  if (invert)
  {
    sum = ecma_number_divide (ECMA_NUMBER_ONE, sum);
  }

  return sum;
} /* ecma_number_exp */

/**
 * @}
 * @}
 */

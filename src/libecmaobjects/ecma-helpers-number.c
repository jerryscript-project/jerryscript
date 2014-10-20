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

#ifdef CONFIG_ECMA_NUMBER_FLOAT32
JERRY_STATIC_ASSERT (sizeof (ecma_number_t) == sizeof (uint32_t));

/**
 * Width of sign field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#define ECMA_NUMBER_SIGN_WIDTH       (1)

/**
 * Width of biased exponent field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#define ECMA_NUMBER_BIASED_EXP_WIDTH (8)

/**
 * Width of fraction field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#define ECMA_NUMBER_FRACTION_WIDTH   (23)

/**
 * Field of ecma-number
 *
 * See also:
 *          IEEE-754 2008, 3.4
 */
typedef struct
{
  /** fraction field */
  unsigned int fraction : ECMA_NUMBER_FRACTION_WIDTH;

  /** biased exponent field */
  unsigned int biased_exp : ECMA_NUMBER_BIASED_EXP_WIDTH;

  /** sign bit */
  unsigned int sign : ECMA_NUMBER_SIGN_WIDTH;
} ecma_number_fields_t;

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
#elif defined (CONFIG_ECMA_NUMBER_FLOAT64)
JERRY_STATIC_ASSERT (sizeof (ecma_number_t) == sizeof (uint64_t));

/**
 * Width of sign field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#define ECMA_NUMBER_SIGN_WIDTH       (1)

/**
 * Width of biased exponent field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#define ECMA_NUMBER_BIASED_EXP_WIDTH (11)

/**
 * Width of fraction field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#define ECMA_NUMBER_FRACTION_WIDTH   (52)

/**
 * Field of ecma-number
 *
 * See also:
 *          IEEE-754 2008, 3.4
 */
typedef struct
{
  /** fraction field */
  unsigned long int fraction : ECMA_NUMBER_FRACTION_WIDTH;

  /** biased exponent field */
  unsigned long int biased_exp : ECMA_NUMBER_BIASED_EXP_WIDTH;

  /** sign bit */
  unsigned long int sign : ECMA_NUMBER_SIGN_WIDTH;
} ecma_number_fields_t;

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
#else /* !CONFIG_ECMA_NUMBER_FLOAT32 && !CONFIG_ECMA_NUMBER_FLOAT64 */
# error "!CONFIG_ECMA_NUMBER_FLOAT32 && !CONFIG_ECMA_NUMBER_FLOAT64"
#endif /* !CONFIG_ECMA_NUMBER_FLOAT32 && !CONFIG_ECMA_NUMBER_FLOAT64 */

/**
 * Get fraction of number
 *
 * @return normalized fraction field of number
 */
static uint64_t
ecma_number_get_fraction_field (ecma_number_t num) /**< ecma-number */
{
  union
  {
    ecma_number_fields_t fields;
    ecma_number_t value;
  } u;

  u.value = num;

  return u.fields.fraction;
} /* ecma_number_get_fraction_field */

/**
 * Get exponent of number
 *
 * @return exponent corresponding to normalized fraction of number
 */
static uint32_t
ecma_number_get_biased_exponent_field (ecma_number_t num) /**< ecma-number */
{
  union
  {
    ecma_number_fields_t fields;
    ecma_number_t value;
  } u;

  u.value = num;

  return u.fields.biased_exp;
} /* ecma_number_get_biased_exponent_field */

/**
 * Get sign bit of number
 *
 * @return 0 or 1 - value of sign bit
 */
static uint32_t
ecma_number_get_sign_field (ecma_number_t num) /**< ecma-number */
{
  union
  {
    ecma_number_fields_t fields;
    ecma_number_t value;
  } u;

  u.value = num;

  return u.fields.sign;
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
  uint32_t biased_exp = ecma_number_get_biased_exponent_field (num);
  uint64_t fraction = ecma_number_get_fraction_field (num);

   /* IEEE-754 2008, 3.4, a */

  return ((biased_exp == (1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1)
          && (fraction != 0));
} /* ecma_number_is_nan */

/**
 * Make a NaN.
 *
 * @return NaN value
 */
ecma_number_t
ecma_number_make_nan (void)
{
  union
  {
    ecma_number_fields_t fields;
    ecma_number_t value;
  } u;

  u.fields.biased_exp = (1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1;
  u.fields.fraction = 1;
  u.fields.sign = 0;

  return u.value;
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
  union
  {
    ecma_number_fields_t fields;
    ecma_number_t value;
  } u;

  u.fields.biased_exp = (1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1;
  u.fields.fraction = 0;
  u.fields.sign = sign;

  return u.value;
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

  /* IEEE-754 2008, 3.4, e */

  return (ecma_number_get_fraction_field (num) == 0
          && ecma_number_get_biased_exponent_field (num) == 0);
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
  JERRY_ASSERT (!ecma_number_is_zero (num));
  JERRY_ASSERT (!ecma_number_is_infinity (num));

  uint32_t biased_exp = ecma_number_get_biased_exponent_field (num);
  uint64_t fraction = ecma_number_get_fraction_field (num);
  int32_t exponent;

  if (unlikely (biased_exp == 0))
  {
    /* IEEE-754 2008, 3.4, d */
    exponent = 1 - ecma_number_exponent_bias;

    while (!(fraction & (1ul << ECMA_NUMBER_FRACTION_WIDTH)))
    {
      JERRY_ASSERT (fraction != 0);

      fraction <<= 1;
      exponent--;
    }
  }
  else
  {
    /* IEEE-754 2008, 3.4, c */
    exponent = (int32_t) biased_exp - ecma_number_exponent_bias;

    JERRY_ASSERT (biased_exp > 0 && biased_exp < (1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1);
    JERRY_ASSERT ((fraction & (1ul << ECMA_NUMBER_FRACTION_WIDTH)) == 0);
    fraction |= 1ul << ECMA_NUMBER_FRACTION_WIDTH;
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
  union
  {
    ecma_number_fields_t fields;
    ecma_number_t value;
  } u;

  uint32_t biased_exp = (uint32_t) (exponent + ecma_number_exponent_bias);
  JERRY_ASSERT (biased_exp > 0 && biased_exp < (1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1);

  u.fields.biased_exp = biased_exp & ((1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1);
  u.fields.fraction = fraction & ((1ul << ECMA_NUMBER_FRACTION_WIDTH) - 1);
  u.fields.sign = 0;

  return u.value;
} /* ecma_number_make_normal_positive_from_fraction_and_exponent */

/**
 * Negate ecma-number
 *
 * @return negated number
 */
ecma_number_t
ecma_number_negate (ecma_number_t num) /**< ecma-number */
{
  JERRY_ASSERT (!ecma_number_is_nan (num));

  union
  {
    ecma_number_fields_t fields;
    ecma_number_t value;
  } u;

  u.value = num;

  u.fields.sign = !u.fields.sign;

  return u.value;
} /* ecma_number_negate */

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
  return ecma_number_add (left_num, -right_num);
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

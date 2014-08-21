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

  return ((biased_exp  == (1u << ECMA_NUMBER_BIASED_EXP_WIDTH) - 1)
          && (fraction != 0));
} /* ecma_number_is_nan */

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

    while (!(fraction & (1u << ECMA_NUMBER_FRACTION_WIDTH)))
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
    JERRY_ASSERT ((fraction & (1u << ECMA_NUMBER_FRACTION_WIDTH)) == 0);
    fraction |= 1u << ECMA_NUMBER_FRACTION_WIDTH;
  }

  *out_fraction_p = fraction;
  *out_exponent_p = exponent;
  return ECMA_NUMBER_FRACTION_WIDTH + 1;
} /* ecma_number_get_fraction_and_exponent */

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
 * @}
 * @}
 */

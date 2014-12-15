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

#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-number-arithmetic.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-try-catch-macro.h"
#include "globals.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_MATH_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-math.inc.h"
#define BUILTIN_UNDERSCORED_ID math
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup object ECMA Object object built-in
 * @{
 */

/**
 * The Math object's 'abs' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_abs (ecma_value_t this_arg __unused, /**< 'this' argument */
                              ecma_value_t arg) /**< routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ecma_number_t *num_p = ecma_alloc_number ();

  if (ecma_number_is_nan (arg_num))
  {
    *num_p = arg_num;
  }
  else
  {
    *num_p = ecma_number_abs (arg_num);
  }

  ret_value = ecma_make_normal_completion_value (ecma_make_number_value (num_p));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

  return ret_value;
} /* ecma_builtin_math_object_abs */

/**
 * The Math object's 'acos' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_acos (ecma_value_t this_arg, /**< 'this' argument */
                               ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_math_object_acos */

/**
 * The Math object's 'asin' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_asin (ecma_value_t this_arg, /**< 'this' argument */
                               ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_math_object_asin */

/**
 * The Math object's 'atan' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_atan (ecma_value_t this_arg, /**< 'this' argument */
                               ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_math_object_atan */

/**
 * The Math object's 'atan2' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_atan2 (ecma_value_t this_arg, /**< 'this' argument */
                                ecma_value_t arg1, /**< first routine's argument */
                                ecma_value_t arg2) /**< second routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg1, arg2);
} /* ecma_builtin_math_object_atan2 */

/**
 * The Math object's 'ceil' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_ceil (ecma_value_t this_arg, /**< 'this' argument */
                               ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_math_object_ceil */

/**
 * The Math object's 'cos' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.7
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_cos (ecma_value_t this_arg __unused, /**< 'this' argument */
                              ecma_value_t arg) /**< routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ecma_number_t *num_p = ecma_alloc_number ();

  if (ecma_number_is_nan (arg_num)
      || ecma_number_is_infinity (arg_num))
  {
    *num_p = ecma_number_make_nan ();
  }
  else if (ecma_number_is_zero (arg_num))
  {
    *num_p = ECMA_NUMBER_ONE;
  }
  else
  {
    /* Taylor series of cos (x) around x = 0 is 1 - x^2/2! + x^4/4! - x^6/6! + ... */

    ecma_number_t x = ecma_op_number_remainder (arg_num, 2 * ECMA_NUMBER_PI);
    ecma_number_t neg_sqr_x = ecma_number_negate (ecma_number_multiply (x, x));

    ecma_number_t sum = ECMA_NUMBER_ZERO;
    ecma_number_t next_addendum = ECMA_NUMBER_ONE;
    ecma_number_t next_factorial_factor = ECMA_NUMBER_ZERO;

    ecma_number_t diff = ecma_number_make_infinity (false);

    while ((ecma_number_is_zero (sum) && !ecma_number_is_zero (diff))
           || (!ecma_number_is_zero (sum)
               && ecma_number_abs (ecma_number_divide (diff, sum)) > ecma_number_relative_eps))
    {
      ecma_number_t next_sum = ecma_number_add (sum, next_addendum);

      next_addendum = ecma_number_multiply (next_addendum, neg_sqr_x);
      next_factorial_factor = ecma_number_add (next_factorial_factor, ECMA_NUMBER_ONE);
      next_addendum = ecma_number_divide (next_addendum, next_factorial_factor);
      next_factorial_factor = ecma_number_add (next_factorial_factor, ECMA_NUMBER_ONE);
      next_addendum = ecma_number_divide (next_addendum, next_factorial_factor);

      diff = ecma_number_abs (ecma_number_substract (sum, next_sum));

      sum = next_sum;
    }

    *num_p = sum;
  }

  ret_value = ecma_make_normal_completion_value (ecma_make_number_value (num_p));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

  return ret_value;
} /* ecma_builtin_math_object_cos */

/**
 * The Math object's 'exp' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.8
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_exp (ecma_value_t this_arg __unused, /**< 'this' argument */
                              ecma_value_t arg) /**< routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ecma_number_t *num_p = ecma_alloc_number ();

  if (ecma_number_is_nan (arg_num))
  {
    *num_p = arg_num;
  }
  else if (ecma_number_is_zero (arg_num))
  {
    *num_p = ECMA_NUMBER_ONE;
  }
  else if (ecma_number_is_infinity (arg_num))
  {
    if (ecma_number_is_negative (arg_num))
    {
      *num_p = ECMA_NUMBER_ZERO;
    }
    else
    {
      *num_p = arg_num;
    }
  }
  else
  {
    *num_p = ecma_number_exp (arg_num);
  }

  ret_value = ecma_make_normal_completion_value (ecma_make_number_value (num_p));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

  return ret_value;
} /* ecma_builtin_math_object_exp */

/**
 * The Math object's 'floor' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.9
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_floor (ecma_value_t this_arg, /**< 'this' argument */
                                ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_math_object_floor */

/**
 * The Math object's 'log' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.10
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_log (ecma_value_t this_arg __unused, /**< 'this' argument */
                              ecma_value_t arg) /**< routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ecma_number_t *num_p = ecma_alloc_number ();

  if (ecma_number_is_nan (arg_num))
  {
    *num_p = arg_num;
  }
  else if (ecma_number_is_zero (arg_num))
  {
    *num_p = ecma_number_make_infinity (true);
  }
  else if (ecma_number_is_negative (arg_num))
  {
    *num_p = ecma_number_make_nan ();
  }
  else if (ecma_number_is_infinity (arg_num))
  {
    *num_p = arg_num;
  }
  else
  {
    *num_p = ecma_number_ln (arg_num);
  }

  ret_value = ecma_make_normal_completion_value (ecma_make_number_value (num_p));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

  return ret_value;
} /* ecma_builtin_math_object_log */

/**
 * The Math object's 'max' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.11
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_max (ecma_value_t this_arg __unused, /**< 'this' argument */
                              ecma_value_t args[], /**< arguments list */
                              ecma_length_t args_number) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ecma_number_t ret_num = ecma_number_make_infinity (true);

  bool is_just_convert = false;

  for (ecma_length_t arg_index = 0;
       arg_index < args_number;
       arg_index++)
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, args[arg_index], ret_value);

    if (!is_just_convert)
    {
      if (unlikely (ecma_number_is_nan (arg_num)))
      {
        ret_num = arg_num;
        is_just_convert = true;
      }
      else if (ecma_number_is_zero (arg_num) /* both numbers are zeroes */
               && ecma_number_is_zero (ret_num))
      {
        if (!ecma_number_is_negative (arg_num))
        {
          ret_num = arg_num;
        }
      }
      else if (ecma_number_is_infinity (arg_num))
      {
        if (!ecma_number_is_negative (arg_num))
        {
          ret_num = arg_num;
          is_just_convert = true;
        }
      }
      else if (ecma_number_is_infinity (ret_num)) /* ret_num is negative infinity */
      {
        JERRY_ASSERT (ecma_number_is_negative (ret_num));

        ret_num = arg_num;
      }
      else
      {
        JERRY_ASSERT (!ecma_number_is_nan (arg_num)
                      && !ecma_number_is_infinity (arg_num));
        JERRY_ASSERT (!ecma_number_is_nan (ret_num)
                      && !ecma_number_is_infinity (ret_num));

        if (arg_num > ret_num)
        {
          ret_num = arg_num;
        }
      }
    }

    ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

    if (ecma_is_completion_value_throw (ret_value))
    {
      return ret_value;
    }

    JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));
  }

  JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));

  ecma_number_t *num_p = ecma_alloc_number ();
  *num_p = ret_num;

  return ecma_make_normal_completion_value (ecma_make_number_value (num_p));
} /* ecma_builtin_math_object_max */

/**
 * The Math object's 'min' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.12
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_min (ecma_value_t this_arg __unused, /**< 'this' argument */
                              ecma_value_t args[], /**< arguments list */
                              ecma_length_t args_number) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ecma_number_t ret_num = ecma_number_make_infinity (false);

  bool is_just_convert = false;

  for (ecma_length_t arg_index = 0;
       arg_index < args_number;
       arg_index++)
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, args[arg_index], ret_value);

    if (!is_just_convert)
    {
      if (unlikely (ecma_number_is_nan (arg_num)))
      {
        ret_num = arg_num;
        is_just_convert = true;
      }
      else if (ecma_number_is_zero (arg_num) /* both numbers are zeroes */
               && ecma_number_is_zero (ret_num))
      {
        if (ecma_number_is_negative (arg_num))
        {
          ret_num = arg_num;
        }
      }
      else if (ecma_number_is_infinity (arg_num))
      {
        if (ecma_number_is_negative (arg_num))
        {
          ret_num = arg_num;
          is_just_convert = true;
        }
      }
      else if (ecma_number_is_infinity (ret_num)) /* ret_num is positive infinity */
      {
        JERRY_ASSERT (!ecma_number_is_negative (ret_num));

        ret_num = arg_num;
      }
      else
      {
        JERRY_ASSERT (!ecma_number_is_nan (arg_num)
                      && !ecma_number_is_infinity (arg_num));
        JERRY_ASSERT (!ecma_number_is_nan (ret_num)
                      && !ecma_number_is_infinity (ret_num));

        if (arg_num < ret_num)
        {
          ret_num = arg_num;
        }
      }
    }

    ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

    if (ecma_is_completion_value_throw (ret_value))
    {
      return ret_value;
    }

    JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));
  }

  JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));

  ecma_number_t *num_p = ecma_alloc_number ();
  *num_p = ret_num;

  return ecma_make_normal_completion_value (ecma_make_number_value (num_p));
} /* ecma_builtin_math_object_min */

/**
 * The Math object's 'pow' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.13
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_pow (ecma_value_t this_arg __unused, /**< 'this' argument */
                              ecma_value_t arg1, /**< first routine's argument */
                              ecma_value_t arg2) /**< second routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_OP_TO_NUMBER_TRY_CATCH (x, arg1, ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (y, arg2, ret_value);

  ecma_number_t *num_p = ecma_alloc_number ();

  if (ecma_number_is_nan (y)
      || (ecma_number_is_nan (x)
          && !ecma_number_is_zero (y)))
  {
    *num_p = ecma_number_make_nan ();
  }
  else if (ecma_number_is_zero (y))
  {
    *num_p = ECMA_NUMBER_ONE;
  }
  else if (ecma_number_is_infinity (y))
  {
    const ecma_number_t x_abs = ecma_number_abs (x);

    if (x_abs == ECMA_NUMBER_ONE)
    {
      *num_p = ecma_number_make_nan ();
    }
    else if ((ecma_number_is_negative (y) && x_abs < ECMA_NUMBER_ONE)
             || (!ecma_number_is_negative (y) && x_abs > ECMA_NUMBER_ONE))
    {
      *num_p = ecma_number_make_infinity (false);
    }
    else
    {
      JERRY_ASSERT ((ecma_number_is_negative (y) && x_abs > ECMA_NUMBER_ONE)
                    || (!ecma_number_is_negative (y) && x_abs < ECMA_NUMBER_ONE));

      *num_p = ECMA_NUMBER_ZERO;
    }
  }
  else
  {
    const ecma_number_t diff_is_int = ecma_op_number_remainder (y, ECMA_NUMBER_ONE);
    const ecma_number_t rel_diff_is_int = ecma_number_abs (ecma_number_divide (diff_is_int,
                                                                               y));
    const ecma_number_t y_int = ecma_number_substract (y, diff_is_int);

    const ecma_number_t y_int_half = ecma_number_multiply (y_int, ECMA_NUMBER_HALF);
    const ecma_number_t diff_is_odd = ecma_op_number_remainder (y_int_half, ECMA_NUMBER_ONE);
    const ecma_number_t rel_diff_is_odd = ecma_number_abs (ecma_number_divide (diff_is_odd,
                                                                               y_int_half));

    const bool is_y_int = (rel_diff_is_int < ecma_number_relative_eps);
    const bool is_y_odd = (is_y_int && rel_diff_is_odd > ecma_number_relative_eps);

    if (ecma_number_is_infinity (x))
    {
      if (!ecma_number_is_negative (x))
      {
        if (y > ECMA_NUMBER_ZERO)
        {
          *num_p = ecma_number_make_infinity (false);
        }
        else
        {
          JERRY_ASSERT (y < ECMA_NUMBER_ZERO);

          *num_p = ECMA_NUMBER_ZERO;
        }
      }
      else
      {
        if (y > ECMA_NUMBER_ZERO)
        {
          *num_p = ecma_number_make_infinity (is_y_odd);
        }
        else
        {
          JERRY_ASSERT (y < ECMA_NUMBER_ZERO);

          if (is_y_odd)
          {
            *num_p = ecma_number_negate (ECMA_NUMBER_ZERO);
          }
          else
          {
            *num_p = ECMA_NUMBER_ZERO;
          }
        }
      }
    }
    else if (ecma_number_is_zero (x))
    {
      if (!ecma_number_is_negative (x))
      {
        if (y > ECMA_NUMBER_ZERO)
        {
          *num_p = ECMA_NUMBER_ZERO;
        }
        else
        {
          JERRY_ASSERT (y < ECMA_NUMBER_ZERO);

          *num_p = ecma_number_make_infinity (false);
        }
      }
      else
      {
        if (y > ECMA_NUMBER_ZERO)
        {
          if (is_y_odd)
          {
            *num_p = ecma_number_negate (ECMA_NUMBER_ZERO);
          }
          else
          {
            *num_p = ECMA_NUMBER_ZERO;
          }
        }
        else
        {
          *num_p = ecma_number_make_infinity (is_y_odd);
        }
      }
    }
    else if (!ecma_number_is_infinity (x)
             && x < ECMA_NUMBER_ZERO
             && !ecma_number_is_infinity (y)
             && !is_y_int)
    {
      *num_p = ecma_number_make_nan ();
    }
    else
    {
      JERRY_ASSERT (!ecma_number_is_infinity (x)
                    && !ecma_number_is_zero (x));
      JERRY_ASSERT (!ecma_number_is_infinity (y)
                    && !ecma_number_is_zero (y));

      const bool sign = (x < ECMA_NUMBER_ZERO && is_y_odd);
      const bool invert = (y < ECMA_NUMBER_ZERO);

      JERRY_ASSERT (is_y_int || !sign);

      ecma_number_t positive_x;
      ecma_number_t positive_y;

      if (x < ECMA_NUMBER_ZERO)
      {
        JERRY_ASSERT (x < ECMA_NUMBER_ZERO);

        positive_x = ecma_number_negate (x);
      }
      else
      {
        positive_x = x;
      }

      if (invert)
      {
        positive_y = ecma_number_negate (y);
      }
      else
      {
        positive_y = y;
      }

      ecma_number_t ret_num;

      if (is_y_int
          && ecma_uint32_to_number (ecma_number_to_uint32 (positive_y)) == positive_y)
      {
        TODO (/* Check for license issues */);

        uint32_t power_uint32 = ecma_number_to_uint32 (positive_y);

        ret_num = ECMA_NUMBER_ONE;
        ecma_number_t power_accumulator = positive_x;

        while (power_uint32 != 0)
        {
          if (power_uint32 % 2)
          {
            ret_num = ecma_number_multiply (ret_num, power_accumulator);

            power_uint32--;
          }

          power_accumulator = ecma_number_multiply (power_accumulator, power_accumulator);
          power_uint32 /= 2;
        }
      }
      else
      {
        /* pow (x, y) = exp (y * ln (x)) */
        ecma_number_t ln_x = ecma_number_ln (positive_x);
        ecma_number_t y_m_ln_x = ecma_number_multiply (positive_y, ln_x);
        ret_num = ecma_number_exp (y_m_ln_x);
      }

      if (sign)
      {
        ret_num = ecma_number_negate (ret_num);
      }

      if (invert)
      {
        ret_num = ecma_number_divide (ECMA_NUMBER_ONE, ret_num);
      }

      *num_p = ret_num;
    }
  }

  ret_value = ecma_make_normal_completion_value (ecma_make_number_value (num_p));

  ECMA_OP_TO_NUMBER_FINALIZE (y);
  ECMA_OP_TO_NUMBER_FINALIZE (x);

  return ret_value;
} /* ecma_builtin_math_object_pow */

/**
 * The Math object's 'random' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.14
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_random (ecma_value_t this_arg __unused) /**< 'this' argument */
{
  /* Implementation of George Marsaglia's XorShift random number generator */
  TODO (/* Check for license issues */);

  static uint32_t word1 = 1455997910;
  static uint32_t word2 = 1999515274;
  static uint32_t word3 = 1234451287;
  static uint32_t word4 = 1949149569;

  uint32_t intermediate = word1 ^ (word1 << 11);
  intermediate ^= intermediate >> 8;

  word1 = word2;
  word2 = word3;
  word3 = word4;

  word4 ^= word4 >> 19;
  word4 ^= intermediate;

  const uint32_t max_uint32 = (uint32_t) -1;
  ecma_number_t rand = (ecma_number_t) word4;
  rand /= (ecma_number_t) max_uint32;
  rand *= (ecma_number_t) (max_uint32 - 1) / (ecma_number_t) max_uint32;

  ecma_number_t *rand_p = ecma_alloc_number ();
  *rand_p = rand;

  return ecma_make_normal_completion_value (ecma_make_number_value (rand_p));
} /* ecma_builtin_math_object_random */

/**
 * The Math object's 'round' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.15
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_round (ecma_value_t this_arg __unused, /**< 'this' argument */
                                ecma_value_t arg) /**< routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ecma_number_t *num_p = ecma_alloc_number ();

  if (ecma_number_is_nan (arg_num)
      || ecma_number_is_zero (arg_num)
      || ecma_number_is_infinity (arg_num))
  {
    *num_p = arg_num;
  }
  else if (ecma_number_is_negative (arg_num)
           && arg_num >= -0.5f)
  {
    *num_p = ecma_number_negate (0.0f);
  }
  else
  {
    const ecma_number_t up_half = arg_num + 0.5f;
    const ecma_number_t down_half = arg_num - 0.5f;
    const ecma_number_t up_rounded = up_half - ecma_op_number_remainder (up_half, 1);
    const ecma_number_t down_rounded = down_half - ecma_op_number_remainder (down_half, 1);

    if (up_rounded - arg_num <= arg_num - down_rounded)
    {
      *num_p = up_rounded;
    }
    else
    {
      *num_p = down_rounded;
    }
  }

  ret_value = ecma_make_normal_completion_value (ecma_make_number_value (num_p));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

  return ret_value;
} /* ecma_builtin_math_object_round */

/**
 * The Math object's 'sin' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.16
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_sin (ecma_value_t this_arg __unused, /**< 'this' argument */
                              ecma_value_t arg) /**< routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ecma_number_t *num_p = ecma_alloc_number ();

  if (ecma_number_is_nan (arg_num)
      || ecma_number_is_infinity (arg_num))
  {
    *num_p = ecma_number_make_nan ();
  }
  else if (ecma_number_is_zero (arg_num))
  {
    *num_p = arg_num;
  }
  else
  {
    /* Taylor series of sin (x) around x = 0 is x - x^3/3! + x^5/5! - x^7/7! + ... */

    ecma_number_t x = ecma_op_number_remainder (arg_num, 2 * ECMA_NUMBER_PI);
    ecma_number_t neg_sqr_x = ecma_number_negate (ecma_number_multiply (x, x));

    ecma_number_t sum = ECMA_NUMBER_ZERO;
    ecma_number_t next_addendum = ecma_number_divide (x, ECMA_NUMBER_ONE);
    ecma_number_t next_factorial_factor = ECMA_NUMBER_ONE;

    ecma_number_t diff = ecma_number_make_infinity (false);

    while ((ecma_number_is_zero (sum) && !ecma_number_is_zero (diff))
           || (!ecma_number_is_zero (sum)
               && ecma_number_abs (ecma_number_divide (diff, sum)) > ecma_number_relative_eps))
    {
      ecma_number_t next_sum = ecma_number_add (sum, next_addendum);

      next_addendum = ecma_number_multiply (next_addendum, neg_sqr_x);
      next_factorial_factor = ecma_number_add (next_factorial_factor, ECMA_NUMBER_ONE);
      next_addendum = ecma_number_divide (next_addendum, next_factorial_factor);
      next_factorial_factor = ecma_number_add (next_factorial_factor, ECMA_NUMBER_ONE);
      next_addendum = ecma_number_divide (next_addendum, next_factorial_factor);

      diff = ecma_number_abs (ecma_number_substract (sum, next_sum));

      sum = next_sum;
    }

    *num_p = sum;
  }

  ret_value = ecma_make_normal_completion_value (ecma_make_number_value (num_p));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

  return ret_value;
} /* ecma_builtin_math_object_sin */

/**
 * The Math object's 'sqrt' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.17
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_sqrt (ecma_value_t this_arg __unused, /**< 'this' argument */
                               ecma_value_t arg) /**< routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ecma_number_t ret_num;

  if (ecma_number_is_nan (arg_num)
      || (!ecma_number_is_zero (arg_num)
          && ecma_number_is_negative (arg_num)))
  {
    ret_num = ecma_number_make_nan ();
  }
  else if (ecma_number_is_zero (arg_num))
  {
    ret_num = arg_num;
  }
  else if (ecma_number_is_infinity (arg_num))
  {
    JERRY_ASSERT (!ecma_number_is_negative (arg_num));

    ret_num = arg_num;
  }
  else
  {
    ret_num = ecma_number_sqrt (arg_num);
  }

  ecma_number_t *num_p = ecma_alloc_number ();
  *num_p = ret_num;

  ret_value = ecma_make_normal_completion_value (ecma_make_number_value (num_p));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

  return ret_value;
} /* ecma_builtin_math_object_sqrt */

/**
 * The Math object's 'tan' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.18
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_math_object_tan (ecma_value_t this_arg, /**< 'this' argument */
                              ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_math_object_tan */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_MATH_BUILTIN */

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
#include "jrt.h"
#include "jrt-libc-includes.h"

#ifndef CONFIG_DISABLE_MATH_BUILTIN

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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_abs (ecma_value_t this_arg, /**< 'this' argument */
                              ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ret_value = ecma_make_number_value (DOUBLE_TO_ECMA_NUMBER_T (fabs (arg_num)));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

  return ret_value;
} /* ecma_builtin_math_object_abs */

/**
 * The Math object's 'acos' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_acos (ecma_value_t this_arg, /**< 'this' argument */
                               ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ret_value = ecma_make_number_value (DOUBLE_TO_ECMA_NUMBER_T (acos (arg_num)));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  return ret_value;
} /* ecma_builtin_math_object_acos */

/**
 * The Math object's 'asin' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_asin (ecma_value_t this_arg, /**< 'this' argument */
                               ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ret_value = ecma_make_number_value (DOUBLE_TO_ECMA_NUMBER_T (asin (arg_num)));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  return ret_value;
} /* ecma_builtin_math_object_asin */

/**
 * The Math object's 'atan' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_atan (ecma_value_t this_arg, /**< 'this' argument */
                               ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ret_value = ecma_make_number_value (DOUBLE_TO_ECMA_NUMBER_T (atan (arg_num)));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  return ret_value;
} /* ecma_builtin_math_object_atan */

/**
 * The Math object's 'atan2' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_atan2 (ecma_value_t this_arg, /**< 'this' argument */
                                ecma_value_t arg1, /**< first routine's argument */
                                ecma_value_t arg2) /**< second routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (x, arg1, ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (y, arg2, ret_value);

  ret_value = ecma_make_number_value (DOUBLE_TO_ECMA_NUMBER_T (atan2 (x, y)));

  ECMA_OP_TO_NUMBER_FINALIZE (y);
  ECMA_OP_TO_NUMBER_FINALIZE (x);
  return ret_value;
} /* ecma_builtin_math_object_atan2 */

/**
 * The Math object's 'ceil' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_ceil (ecma_value_t this_arg, /**< 'this' argument */
                               ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ret_value = ecma_make_number_value (DOUBLE_TO_ECMA_NUMBER_T (ceil (arg_num)));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  return ret_value;
} /* ecma_builtin_math_object_ceil */

/**
 * The Math object's 'cos' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_cos (ecma_value_t this_arg, /**< 'this' argument */
                              ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ret_value = ecma_make_number_value (DOUBLE_TO_ECMA_NUMBER_T (cos (arg_num)));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  return ret_value;
} /* ecma_builtin_math_object_cos */

/**
 * The Math object's 'exp' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_exp (ecma_value_t this_arg, /**< 'this' argument */
                              ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ret_value = ecma_make_number_value (DOUBLE_TO_ECMA_NUMBER_T (exp (arg_num)));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

  return ret_value;
} /* ecma_builtin_math_object_exp */

/**
 * The Math object's 'floor' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_floor (ecma_value_t this_arg, /**< 'this' argument */
                                ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ret_value = ecma_make_number_value (DOUBLE_TO_ECMA_NUMBER_T (floor (arg_num)));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  return ret_value;
} /* ecma_builtin_math_object_floor */

/**
 * The Math object's 'log' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.10
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_log (ecma_value_t this_arg, /**< 'this' argument */
                              ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ret_value = ecma_make_number_value (DOUBLE_TO_ECMA_NUMBER_T (log (arg_num)));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

  return ret_value;
} /* ecma_builtin_math_object_log */

/**
 * The Math object's 'max' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.11
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_max (ecma_value_t this_arg, /**< 'this' argument */
                              const ecma_value_t args[], /**< arguments list */
                              ecma_length_t args_number) /**< number of arguments */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ecma_number_t ret_num = ecma_number_make_infinity (true);

  bool is_NaN = false;

  for (ecma_length_t arg_index = 0;
       arg_index < args_number && ecma_is_value_empty (ret_value);
       arg_index++)
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, args[arg_index], ret_value);

    if (!is_NaN)
    {
      if (unlikely (ecma_number_is_nan (arg_num)))
      {
        ret_num = arg_num;
        is_NaN = true;
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
        }
      }
      else if (ecma_number_is_infinity (ret_num))
      {
        if (ecma_number_is_negative (ret_num))
        {
          ret_num = arg_num;
        }
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
  }

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = ecma_make_number_value (ret_num);
  }

  return ret_value;
} /* ecma_builtin_math_object_max */

/**
 * The Math object's 'min' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.12
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_min (ecma_value_t this_arg, /**< 'this' argument */
                              const ecma_value_t args[], /**< arguments list */
                              ecma_length_t args_number) /**< number of arguments */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ecma_number_t ret_num = ecma_number_make_infinity (false);

  bool is_NaN = false;

  for (ecma_length_t arg_index = 0;
       arg_index < args_number && ecma_is_value_empty (ret_value);
       arg_index++)
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, args[arg_index], ret_value);

    if (!is_NaN)
    {
      if (unlikely (ecma_number_is_nan (arg_num)))
      {
        ret_num = arg_num;
        is_NaN = true;
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
        }
      }
      else if (ecma_number_is_infinity (ret_num))
      {
        if (!ecma_number_is_negative (ret_num))
        {
          ret_num = arg_num;
        }
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
  }

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = ecma_make_number_value (ret_num);
  }

  return ret_value;
} /* ecma_builtin_math_object_min */

/**
 * The Math object's 'pow' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.13
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_pow (ecma_value_t this_arg, /**< 'this' argument */
                              ecma_value_t arg1, /**< first routine's argument */
                              ecma_value_t arg2) /**< second routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (x, arg1, ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (y, arg2, ret_value);

  if (ecma_number_is_nan (y) ||
      (ecma_number_is_infinity (y) && (x == 1.0 || x == -1.0)))
  {
    /* Handle differences between ES5.1 and ISO C standards for pow. */
    ret_value = ecma_make_number_value (ecma_number_make_nan ());
  }
  else
  {
    ret_value = ecma_make_number_value (DOUBLE_TO_ECMA_NUMBER_T (pow (x, y)));
  }

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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_random (ecma_value_t this_arg) /**< 'this' argument */
{
  JERRY_UNUSED (this_arg);

  const ecma_number_t rand_max = (ecma_number_t) RAND_MAX;
  const ecma_number_t rand_max_min_1 = (ecma_number_t) (RAND_MAX - 1);

  return ecma_make_number_value (((ecma_number_t) rand ()) / rand_max * rand_max_min_1 / rand_max);
} /* ecma_builtin_math_object_random */

/**
 * The Math object's 'round' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.15
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_round (ecma_value_t this_arg, /**< 'this' argument */
                                ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ecma_number_t num = ECMA_NUMBER_ZERO;

  if (ecma_number_is_nan (arg_num)
      || ecma_number_is_zero (arg_num)
      || ecma_number_is_infinity (arg_num))
  {
    num = arg_num;
  }
  else if (ecma_number_is_negative (arg_num)
           && arg_num >= -ECMA_NUMBER_HALF)
  {
    num = ecma_number_negate (ECMA_NUMBER_ZERO);
  }
  else
  {
    const ecma_number_t up_half = arg_num + ECMA_NUMBER_HALF;
    const ecma_number_t down_half = arg_num - ECMA_NUMBER_HALF;
    const ecma_number_t up_rounded = up_half - ecma_op_number_remainder (up_half, ECMA_NUMBER_ONE);
    const ecma_number_t down_rounded = down_half - ecma_op_number_remainder (down_half, ECMA_NUMBER_ONE);

    if (up_rounded - arg_num <= arg_num - down_rounded)
    {
      num = up_rounded;
    }
    else
    {
      num = down_rounded;
    }
  }

  ret_value = ecma_make_number_value (num);

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

  return ret_value;
} /* ecma_builtin_math_object_round */

/**
 * The Math object's 'sin' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.16
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_sin (ecma_value_t this_arg, /**< 'this' argument */
                              ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ret_value = ecma_make_number_value (DOUBLE_TO_ECMA_NUMBER_T (sin (arg_num)));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  return ret_value;
} /* ecma_builtin_math_object_sin */

/**
 * The Math object's 'sqrt' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.17
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_sqrt (ecma_value_t this_arg, /**< 'this' argument */
                               ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ret_value = ecma_make_number_value (DOUBLE_TO_ECMA_NUMBER_T (sqrt (arg_num)));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  return ret_value;
} /* ecma_builtin_math_object_sqrt */

/**
 * The Math object's 'tan' routine
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.18
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_tan (ecma_value_t this_arg, /**< 'this' argument */
                              ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  ret_value = ecma_make_number_value (DOUBLE_TO_ECMA_NUMBER_T (tan (arg_num)));

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  return ret_value;
} /* ecma_builtin_math_object_tan */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_MATH_BUILTIN */

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

#if ENABLED (JERRY_BUILTIN_MATH)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/**
 * This object has a custom dispatch function.
 */
#define BUILTIN_CUSTOM_DISPATCH

/**
 * List of built-in routine identifiers.
 */
enum
{
  ECMA_MATH_OBJECT_ROUTINE_START = ECMA_BUILTIN_ID__COUNT - 1,

  ECMA_MATH_OBJECT_ABS, /* ECMA-262 v5, 15.8.2.1 */
  ECMA_MATH_OBJECT_ACOS, /* ECMA-262 v5, 15.8.2.2 */
  ECMA_MATH_OBJECT_ASIN, /* ECMA-262 v5, 15.8.2.3 */
  ECMA_MATH_OBJECT_ATAN, /* ECMA-262 v5, 15.8.2.4 */
  ECMA_MATH_OBJECT_CEIL, /* ECMA-262 v5, 15.8.2.6 */
  ECMA_MATH_OBJECT_COS, /* ECMA-262 v5, 15.8.2.7 */
  ECMA_MATH_OBJECT_EXP, /* ECMA-262 v5, 15.8.2.8 */
  ECMA_MATH_OBJECT_FLOOR, /* ECMA-262 v5, 15.8.2.9 */
  ECMA_MATH_OBJECT_LOG, /* ECMA-262 v5, 15.8.2.10 */
#if ENABLED (JERRY_ES2015_BUILTIN)
  ECMA_MATH_OBJECT_TRUNC, /* ECMA-262 v6, 20.2.2.35  */
  ECMA_MATH_OBJECT_SIGN, /* ECMA-262 v6, 20.2.2.29 */
#endif /* ENABLED (JERRY_ES2015_BUILTIN) */
  ECMA_MATH_OBJECT_ROUND, /* ECMA-262 v5, 15.8.2.15 */
  ECMA_MATH_OBJECT_SIN, /* ECMA-262 v5, 15.8.2.16 */
  ECMA_MATH_OBJECT_SQRT, /* ECMA-262 v5, 15.8.2.17 */
  ECMA_MATH_OBJECT_TAN, /* ECMA-262 v5, 15.8.2.18 */

  ECMA_MATH_OBJECT_ATAN2, /* ECMA-262 v5, 15.8.2.5 */
  ECMA_MATH_OBJECT_POW, /* ECMA-262 v5, 15.8.2.13 */

  ECMA_MATH_OBJECT_MAX, /* ECMA-262 v5, 15.8.2.11 */
  ECMA_MATH_OBJECT_MIN, /* ECMA-262 v5, 15.8.2.12 */

  ECMA_MATH_OBJECT_RANDOM, /* ECMA-262 v5, 15.8.2.14 */
};

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
 * The Math object's 'max' 'min' routines.
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.11
 *          ECMA-262 v5, 15.8.2.12
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_max_min (bool is_max, /**< 'max' or 'min' operation */
                                  const ecma_value_t *arg, /**< arguments list */
                                  ecma_length_t args_number) /**< number of arguments */
{
  ecma_number_t result_num = ecma_number_make_infinity (is_max);

  while (args_number > 0)
  {
    ecma_number_t arg_num;

    if (ecma_is_value_number (*arg))
    {
      arg_num = ecma_get_number_from_value (*arg);
    }
    else
    {
      ecma_value_t value = ecma_op_to_number (*arg);

      if (ECMA_IS_VALUE_ERROR (value))
      {
        return value;
      }

      arg_num = ecma_get_number_from_value (value);

      ecma_fast_free_value (value);
    }

    if (JERRY_UNLIKELY (ecma_number_is_nan (arg_num)))
    {
      result_num = arg_num;
      break;
    }

    if (ecma_number_is_zero (arg_num)
        && ecma_number_is_zero (result_num))
    {
      bool is_negative = ecma_number_is_negative (arg_num);

      if (is_max ? !is_negative : is_negative)
      {
        result_num = arg_num;
      }
    }
    else
    {
      if (is_max ? (arg_num > result_num) : (arg_num < result_num))
      {
        result_num = arg_num;
      }
    }

    arg++;
    args_number--;
  }

  return ecma_make_number_value (result_num);
} /* ecma_builtin_math_object_max_min */

#if ENABLED (JERRY_ES2015_BUILTIN)
/**
 * The Math object's 'trunc' routine
 *
 * See also:
 *          ECMA-262 v6, 20.2.2.35
 *
 * @return ecma number
 */
static ecma_number_t
ecma_builtin_math_object_trunc (ecma_number_t arg)
{
  if (ecma_number_is_nan (arg) || ecma_number_is_infinity (arg) || ecma_number_is_zero (arg))
  {
    return arg;
  }

  if ((arg > 0) && (arg < 1))
  {
    return (ecma_number_t) 0;
  }

  if ((arg < 0) && (arg > -1))
  {
    return (ecma_number_t) -0;
  }

  return (ecma_number_t) arg - fmod (arg, 1);
} /* ecma_builtin_math_object_trunc */

/**
 * The Math object's 'sign' routine
 *
 * See also:
 *          ECMA-262 v6, 20.2.2.29
 *
 * @return ecma number
 */
static ecma_number_t
ecma_builtin_math_object_sign (ecma_number_t arg)
{
  if (ecma_number_is_nan (arg) || ecma_number_is_zero (arg))
  {
    return arg;
  }

  if (ecma_number_is_negative (arg))
  {
    return (ecma_number_t) -1.0;
  }

  return (ecma_number_t) 1.0;
} /* ecma_builtin_math_object_sign */
#endif /* ENABLED (JERRY_ES2015_BUILTIN) */

/**
 * The Math object's 'random' routine.
 *
 * See also:
 *          ECMA-262 v5, 15.8.2.14
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_math_object_random (void)
{
  const ecma_number_t rand_max = (ecma_number_t) RAND_MAX;
  const ecma_number_t rand_max_min_1 = (ecma_number_t) (RAND_MAX - 1);

  return ecma_make_number_value (((ecma_number_t) rand ()) / rand_max * rand_max_min_1 / rand_max);
} /* ecma_builtin_math_object_random */

/**
 * Dispatcher for the built-in's routines.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_math_dispatch_routine (uint16_t builtin_routine_id, /**< built-in wide routine
                                                                  *   identifier */
                                    ecma_value_t this_arg, /**< 'this' argument value */
                                    const ecma_value_t arguments_list[], /**< list of arguments
                                                                          *   passed to routine */
                                    ecma_length_t arguments_number) /**< length of arguments' list */
{
  JERRY_UNUSED (this_arg);

  if (builtin_routine_id <= ECMA_MATH_OBJECT_POW)
  {
    ecma_number_t x = ecma_number_make_nan ();
    ecma_number_t y = ecma_number_make_nan ();

    if (arguments_number >= 1)
    {
      if (ecma_is_value_number (arguments_list[0]))
      {
        x = ecma_get_number_from_value (arguments_list[0]);
      }
      else
      {
        ecma_value_t value = ecma_op_to_number (arguments_list[0]);

        if (ECMA_IS_VALUE_ERROR (value))
        {
          return value;
        }

        x = ecma_get_number_from_value (value);

        ecma_fast_free_value (value);
      }
    }

    if (builtin_routine_id >= ECMA_MATH_OBJECT_ATAN2
        && arguments_number >= 2)
    {
      if (ecma_is_value_number (arguments_list[1]))
      {
        y = ecma_get_number_from_value (arguments_list[1]);
      }
      else
      {
        ecma_value_t value = ecma_op_to_number (arguments_list[1]);

        if (ECMA_IS_VALUE_ERROR (value))
        {
          return value;
        }

        y = ecma_get_number_from_value (value);

        ecma_fast_free_value (value);
      }
    }

    switch (builtin_routine_id)
    {
      case ECMA_MATH_OBJECT_ABS:
      {
        x = DOUBLE_TO_ECMA_NUMBER_T (fabs (x));
        break;
      }
      case ECMA_MATH_OBJECT_ACOS:
      {
        x = DOUBLE_TO_ECMA_NUMBER_T (acos (x));
        break;
      }
      case ECMA_MATH_OBJECT_ASIN:
      {
        x = DOUBLE_TO_ECMA_NUMBER_T (asin (x));
        break;
      }
      case ECMA_MATH_OBJECT_ATAN:
      {
        x = DOUBLE_TO_ECMA_NUMBER_T (atan (x));
        break;
      }
      case ECMA_MATH_OBJECT_CEIL:
      {
        x = DOUBLE_TO_ECMA_NUMBER_T (ceil (x));
        break;
      }
      case ECMA_MATH_OBJECT_COS:
      {
        x = DOUBLE_TO_ECMA_NUMBER_T (cos (x));
        break;
      }
      case ECMA_MATH_OBJECT_EXP:
      {
        x = DOUBLE_TO_ECMA_NUMBER_T (exp (x));
        break;
      }
      case ECMA_MATH_OBJECT_FLOOR:
      {
        x = DOUBLE_TO_ECMA_NUMBER_T (floor (x));
        break;
      }
      case ECMA_MATH_OBJECT_LOG:
      {
        x = DOUBLE_TO_ECMA_NUMBER_T (log (x));
        break;
      }
#if ENABLED (JERRY_ES2015_BUILTIN)
      case ECMA_MATH_OBJECT_TRUNC:
      {
        x = ecma_builtin_math_object_trunc (x);
        break;
      }
      case ECMA_MATH_OBJECT_SIGN:
      {
        x = ecma_builtin_math_object_sign (x);
        break;
      }
#endif /* ENABLED (JERRY_ES2015_BUILTIN) */
      case ECMA_MATH_OBJECT_ROUND:
      {
        if (ecma_number_is_nan (x)
            || ecma_number_is_zero (x)
            || ecma_number_is_infinity (x))
        {
          /* Do nothing. */
        }
        else if (ecma_number_is_negative (x)
                 && x >= -ECMA_NUMBER_HALF)
        {
          x = -ECMA_NUMBER_ZERO;
        }
        else
        {
          const ecma_number_t up_half = x + ECMA_NUMBER_HALF;
          const ecma_number_t down_half = x - ECMA_NUMBER_HALF;
          const ecma_number_t up_rounded = up_half - ecma_op_number_remainder (up_half, ECMA_NUMBER_ONE);
          const ecma_number_t down_rounded = down_half - ecma_op_number_remainder (down_half, ECMA_NUMBER_ONE);

          if (up_rounded - x <= x - down_rounded)
          {
            x = up_rounded;
          }
          else
          {
            x = down_rounded;
          }
        }
        break;
      }
      case ECMA_MATH_OBJECT_SIN:
      {
        x = DOUBLE_TO_ECMA_NUMBER_T (sin (x));
        break;
      }
      case ECMA_MATH_OBJECT_SQRT:
      {
        x = DOUBLE_TO_ECMA_NUMBER_T (sqrt (x));
        break;
      }
      case ECMA_MATH_OBJECT_TAN:
      {
        x = DOUBLE_TO_ECMA_NUMBER_T (tan (x));
        break;
      }
      case ECMA_MATH_OBJECT_ATAN2:
      {
        x = DOUBLE_TO_ECMA_NUMBER_T (atan2 (x, y));
        break;
      }
      case ECMA_MATH_OBJECT_POW:
      {
        if (ecma_number_is_nan (y) ||
            (ecma_number_is_infinity (y) && (x == 1.0 || x == -1.0)))
        {
          /* Handle differences between ES5.1 and ISO C standards for pow. */
          x = ecma_number_make_nan ();
        }
        else if (ecma_number_is_zero (y))
        {
          /* Handle differences between ES5.1 and ISO C standards for pow. */
          x = (ecma_number_t) 1.0;
        }
        else
        {
          x = DOUBLE_TO_ECMA_NUMBER_T (pow (x, y));
        }
        break;
      }
    }

    return ecma_make_number_value (x);
  }

  if (builtin_routine_id <= ECMA_MATH_OBJECT_MIN)
  {
    return ecma_builtin_math_object_max_min (builtin_routine_id == ECMA_MATH_OBJECT_MAX,
                                             arguments_list,
                                             arguments_number);
  }


  JERRY_ASSERT (builtin_routine_id == ECMA_MATH_OBJECT_RANDOM);

  return ecma_builtin_math_object_random ();
} /* ecma_builtin_math_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_MATH) */

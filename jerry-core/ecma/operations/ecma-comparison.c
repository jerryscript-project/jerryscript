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

#include "ecma-comparison.h"
#include "ecma-conversion.h"
#include "ecma-globals.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmacomparison ECMA comparison
 * @{
 */

/**
 * ECMA abstract equality comparison routine.
 *
 * See also: ECMA-262 v5, 11.9.3
 *
 * @return true - if values are equal,
 *         false - otherwise.
 */
ecma_value_t
ecma_op_abstract_equality_compare (ecma_value_t x, /**< first operand */
                                   ecma_value_t y) /**< second operand */
{
  if (x == y)
  {
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  if (ecma_are_values_integer_numbers (x, y))
  {
    /* Note: the (x == y) comparison captures the true case. */
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
  }

  if (ecma_is_value_number (x))
  {
    if (ecma_is_value_number (y))
    {
      /* 1.c */
      ecma_number_t x_num = ecma_get_number_from_value (x);
      ecma_number_t y_num = ecma_get_number_from_value (y);

      bool is_x_equal_to_y = (x_num == y_num);

#ifndef JERRY_NDEBUG
      bool is_x_equal_to_y_check;

      if (ecma_number_is_nan (x_num)
          || ecma_number_is_nan (y_num))
      {
        is_x_equal_to_y_check = false;
      }
      else if (x_num == y_num
               || (ecma_number_is_zero (x_num)
                   && ecma_number_is_zero (y_num)))
      {
        is_x_equal_to_y_check = true;
      }
      else
      {
        is_x_equal_to_y_check = false;
      }

      JERRY_ASSERT (is_x_equal_to_y == is_x_equal_to_y_check);
#endif /* !JERRY_NDEBUG */

      return ecma_make_boolean_value (is_x_equal_to_y);
    }

    /* Swap values. */
    ecma_value_t tmp = x;
    x = y;
    y = tmp;
  }

  if (ecma_is_value_string (x))
  {
    if (ecma_is_value_string (y))
    {
      /* 1., d. */
      ecma_string_t *x_str_p = ecma_get_string_from_value (x);
      ecma_string_t *y_str_p = ecma_get_string_from_value (y);

      bool is_equal = ecma_compare_ecma_strings (x_str_p, y_str_p);

      return ecma_make_boolean_value (is_equal);
    }

    if (ecma_is_value_number (y))
    {
      /* 4. */
      ecma_value_t x_num_value = ecma_op_to_number (x);

      if (ECMA_IS_VALUE_ERROR (x_num_value))
      {
        return x_num_value;
      }

      ecma_value_t compare_result = ecma_op_abstract_equality_compare (x_num_value, y);

      ecma_free_value (x_num_value);
      return compare_result;
    }

    /* Swap values. */
    ecma_value_t tmp = x;
    x = y;
    y = tmp;
  }

  if (ecma_is_value_boolean (y))
  {
    if (ecma_is_value_boolean (x))
    {
      /* 1., e. */
      /* Note: the (x == y) comparison captures the true case. */
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
    }

    /* 7. */
    return ecma_op_abstract_equality_compare (x, ecma_make_integer_value (ecma_is_value_true (y) ? 1 : 0));
  }

  if (ecma_is_value_object (x))
  {
    if (ecma_is_value_string (y)
        || ecma_is_value_number (y))
    {
      /* 9. */
      ecma_value_t x_prim_value = ecma_op_to_primitive (x, ECMA_PREFERRED_TYPE_NO);

      if (ECMA_IS_VALUE_ERROR (x_prim_value))
      {
        return x_prim_value;
      }

      ecma_value_t compare_result = ecma_op_abstract_equality_compare (x_prim_value, y);

      ecma_free_value (x_prim_value);
      return compare_result;
    }

    /* 1., f. */
    /* Note: the (x == y) comparison captures the true case. */
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
  }

  if (ecma_is_value_boolean (x))
  {
    /* 6. */
    return ecma_op_abstract_equality_compare (ecma_make_integer_value (ecma_is_value_true (x) ? 1 : 0), y);
  }

  if (ecma_is_value_undefined (x)
      || ecma_is_value_null (x))
  {
    /* 1. a., b. */
    /* 2., 3. */
    bool is_equal = ecma_is_value_undefined (y) || ecma_is_value_null (y);

    return ecma_make_boolean_value (is_equal);
  }

  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
} /* ecma_op_abstract_equality_compare */

/**
 * ECMA strict equality comparison routine.
 *
 * See also: ECMA-262 v5, 11.9.6
 *
 * @return true - if values are strict equal,
 *         false - otherwise.
 */
bool
ecma_op_strict_equality_compare (ecma_value_t x, /**< first operand */
                                 ecma_value_t y) /**< second operand */
{
  if (ecma_is_value_direct (x)
      || ecma_is_value_direct (y)
      || ecma_is_value_object (x)
      || ecma_is_value_object (y))
  {
    JERRY_ASSERT (!ecma_is_value_direct (x)
                  || ecma_is_value_undefined (x)
                  || ecma_is_value_null (x)
                  || ecma_is_value_boolean (x)
                  || ecma_is_value_integer_number (x));

    JERRY_ASSERT (!ecma_is_value_direct (y)
                  || ecma_is_value_undefined (y)
                  || ecma_is_value_null (y)
                  || ecma_is_value_boolean (y)
                  || ecma_is_value_integer_number (y));

    if ((x != ecma_make_integer_value (0) || !ecma_is_value_float_number (y))
        && (y != ecma_make_integer_value (0) || !ecma_is_value_float_number (x)))
    {
      return (x == y);
    }

    /* The +0 === -0 case handled below. */
  }

  JERRY_ASSERT (ecma_is_value_number (x) || ecma_is_value_string (x));
  JERRY_ASSERT (ecma_is_value_number (y) || ecma_is_value_string (y));

  if (ecma_is_value_string (x))
  {
    if (!ecma_is_value_string (y))
    {
      return false;
    }

    ecma_string_t *x_str_p = ecma_get_string_from_value (x);
    ecma_string_t *y_str_p = ecma_get_string_from_value (y);

    return ecma_compare_ecma_strings (x_str_p, y_str_p);
  }

  if (!ecma_is_value_number (y))
  {
    return false;
  }

  ecma_number_t x_num = ecma_get_number_from_value (x);
  ecma_number_t y_num = ecma_get_number_from_value (y);

  bool is_x_equal_to_y = (x_num == y_num);

#ifndef JERRY_NDEBUG
  bool is_x_equal_to_y_check;

  if (ecma_number_is_nan (x_num)
      || ecma_number_is_nan (y_num))
  {
    is_x_equal_to_y_check = false;
  }
  else if (x_num == y_num
           || (ecma_number_is_zero (x_num)
               && ecma_number_is_zero (y_num)))
  {
    is_x_equal_to_y_check = true;
  }
  else
  {
    is_x_equal_to_y_check = false;
  }

  JERRY_ASSERT (is_x_equal_to_y == is_x_equal_to_y_check);
#endif /* !JERRY_NDEBUG */

  return is_x_equal_to_y;
} /* ecma_op_strict_equality_compare */

/**
 * ECMA abstract relational comparison routine.
 *
 * See also: ECMA-262 v5, 11.8.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_abstract_relational_compare (ecma_value_t x, /**< first operand */
                                     ecma_value_t y, /**< second operand */
                                     bool left_first) /**< 'LeftFirst' flag */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1., 2. */
  ECMA_TRY_CATCH (prim_first_converted_value,
                  ecma_op_to_primitive (x, ECMA_PREFERRED_TYPE_NUMBER),
                  ret_value);
  ECMA_TRY_CATCH (prim_second_converted_value,
                  ecma_op_to_primitive (y, ECMA_PREFERRED_TYPE_NUMBER),
                  ret_value);

  const ecma_value_t px = left_first ? prim_first_converted_value : prim_second_converted_value;
  const ecma_value_t py = left_first ? prim_second_converted_value : prim_first_converted_value;

  const bool is_px_string = ecma_is_value_string (px);
  const bool is_py_string = ecma_is_value_string (py);

  if (!(is_px_string && is_py_string))
  {
    /* 3. */

    /* a. */
    ECMA_OP_TO_NUMBER_TRY_CATCH (nx, px, ret_value);
    ECMA_OP_TO_NUMBER_TRY_CATCH (ny, py, ret_value);

    /* b. */
    if (ecma_number_is_nan (nx)
        || ecma_number_is_nan (ny))
    {
      /* c., d. */
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else
    {
      bool is_x_less_than_y = (nx < ny);

#ifndef JERRY_NDEBUG
      bool is_x_less_than_y_check;

      if (nx == ny
          || (ecma_number_is_zero (nx)
              && ecma_number_is_zero (ny)))
      {
        /* e., f., g. */
        is_x_less_than_y_check = false;
      }
      else if (ecma_number_is_infinity (nx)
               && !ecma_number_is_negative (nx))
      {
        /* h. */
        is_x_less_than_y_check = false;
      }
      else if (ecma_number_is_infinity (ny)
               && !ecma_number_is_negative (ny))
      {
        /* i. */
        is_x_less_than_y_check = true;
      }
      else if (ecma_number_is_infinity (ny)
               && ecma_number_is_negative (ny))
      {
        /* j. */
        is_x_less_than_y_check = false;
      }
      else if (ecma_number_is_infinity (nx)
               && ecma_number_is_negative (nx))
      {
        /* k. */
        is_x_less_than_y_check = true;
      }
      else
      {
        /* l. */
        JERRY_ASSERT (!ecma_number_is_nan (nx)
                      && !ecma_number_is_infinity (nx));
        JERRY_ASSERT (!ecma_number_is_nan (ny)
                      && !ecma_number_is_infinity (ny));
        JERRY_ASSERT (!(ecma_number_is_zero (nx)
                        && ecma_number_is_zero (ny)));

        if (nx < ny)
        {
          is_x_less_than_y_check = true;
        }
        else
        {
          is_x_less_than_y_check = false;
        }
      }

      JERRY_ASSERT (is_x_less_than_y_check == is_x_less_than_y);
#endif /* !JERRY_NDEBUG */

      ret_value = ecma_make_boolean_value (is_x_less_than_y);
    }

    ECMA_OP_TO_NUMBER_FINALIZE (ny);
    ECMA_OP_TO_NUMBER_FINALIZE (nx);
  }
  else
  { /* 4. */
    JERRY_ASSERT (is_px_string && is_py_string);

    ecma_string_t *str_x_p = ecma_get_string_from_value (px);
    ecma_string_t *str_y_p = ecma_get_string_from_value (py);

    bool is_px_less = ecma_compare_ecma_strings_relational (str_x_p, str_y_p);

    ret_value = ecma_make_boolean_value (is_px_less);
  }

  ECMA_FINALIZE (prim_second_converted_value);
  ECMA_FINALIZE (prim_first_converted_value);

  return ret_value;
} /* ecma_op_abstract_relational_compare */

/**
 * @}
 * @}
 */

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
ecma_completion_value_t
ecma_op_abstract_equality_compare (ecma_value_t x, /**< first operand */
                                   ecma_value_t y) /**< second operand */
{
  const bool is_x_undefined = ecma_is_value_undefined (x);
  const bool is_x_null = ecma_is_value_null (x);
  const bool is_x_boolean = ecma_is_value_boolean (x);
  const bool is_x_number = ecma_is_value_number (x);
  const bool is_x_string = ecma_is_value_string (x);
  const bool is_x_object = ecma_is_value_object (x);

  const bool is_y_undefined = ecma_is_value_undefined (y);
  const bool is_y_null = ecma_is_value_null (y);
  const bool is_y_boolean = ecma_is_value_boolean (y);
  const bool is_y_number = ecma_is_value_number (y);
  const bool is_y_string = ecma_is_value_string (y);
  const bool is_y_object = ecma_is_value_object (y);

  const bool is_types_equal = ((is_x_undefined && is_y_undefined)
                               || (is_x_null && is_y_null)
                               || (is_x_boolean && is_y_boolean)
                               || (is_x_number && is_y_number)
                               || (is_x_string && is_y_string)
                               || (is_x_object && is_y_object));

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (is_types_equal)
  {
    // 1.

    if (is_x_undefined
        || is_x_null)
    {
      // a., b.
      ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
    }
    else if (is_x_number)
    { // c.
      ecma_number_t x_num = *ecma_get_number_from_value (x);
      ecma_number_t y_num = *ecma_get_number_from_value (y);

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

      ret_value = ecma_make_simple_completion_value (is_x_equal_to_y ?
                                                     ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
    }
    else if (is_x_string)
    { // d.
      ecma_string_t* x_str_p = ecma_get_string_from_value (x);
      ecma_string_t* y_str_p = ecma_get_string_from_value (y);

      bool is_equal = ecma_compare_ecma_strings (x_str_p, y_str_p);

      ret_value = ecma_make_simple_completion_value (is_equal ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
    }
    else if (is_x_boolean)
    { // e.
      bool is_equal = (ecma_is_value_true (x) == ecma_is_value_true (y));

      ret_value = ecma_make_simple_completion_value (is_equal ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
    }
    else
    { // f.
      JERRY_ASSERT (is_x_object);

      bool is_equal = (ecma_get_object_from_value (x) == ecma_get_object_from_value (y));

      ret_value = ecma_make_simple_completion_value (is_equal ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
    }
  }
  else if ((is_x_null && is_y_undefined)
           || (is_x_undefined && is_y_null))
  { // 2., 3.
    ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }
  else if (is_x_number && is_y_string)
  {
    // 4.
    ECMA_TRY_CATCH (y_num_value,
                    ecma_op_to_number (y),
                    ret_value);

    ret_value = ecma_op_abstract_equality_compare (x, y_num_value);

    ECMA_FINALIZE (y_num_value);
  }
  else if (is_x_string && is_y_number)
  {
    // 5.
    ECMA_TRY_CATCH (x_num_value,
                    ecma_op_to_number (x),
                    ret_value);

    ret_value = ecma_op_abstract_equality_compare (x_num_value, y);

    ECMA_FINALIZE (x_num_value);
  }
  else if (is_x_boolean)
  {
    // 6.
    ECMA_TRY_CATCH (x_num_value,
                    ecma_op_to_number (x),
                    ret_value);

    ret_value = ecma_op_abstract_equality_compare (x_num_value, y);

    ECMA_FINALIZE (x_num_value);
  }
  else if (is_y_boolean)
  {
    // 7.
    ECMA_TRY_CATCH (y_num_value,
                    ecma_op_to_number (y),
                    ret_value);

    ret_value = ecma_op_abstract_equality_compare (x, y_num_value);

    ECMA_FINALIZE (y_num_value);
  }
  else if (is_y_object
           && (is_x_number || is_x_string))
  {
    // 8.
    ECMA_TRY_CATCH (y_prim_value,
                    ecma_op_to_primitive (y, ECMA_PREFERRED_TYPE_NO),
                    ret_value);

    ret_value = ecma_op_abstract_equality_compare (x, y_prim_value);

    ECMA_FINALIZE (y_prim_value);
  }
  else if (is_x_object
           && (is_y_number || is_y_string))
  {
    // 9.
    ECMA_TRY_CATCH (x_prim_value,
                    ecma_op_to_primitive (x, ECMA_PREFERRED_TYPE_NO),
                    ret_value);

    ret_value = ecma_op_abstract_equality_compare (x_prim_value, y);

    ECMA_FINALIZE (x_prim_value);
  }
  else
  {
    ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
  }

  return ret_value;
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
  const bool is_x_undefined = ecma_is_value_undefined (x);
  const bool is_x_null = ecma_is_value_null (x);
  const bool is_x_boolean = ecma_is_value_boolean (x);
  const bool is_x_number = ecma_is_value_number (x);
  const bool is_x_string = ecma_is_value_string (x);
  const bool is_x_object = ecma_is_value_object (x);

  const bool is_y_undefined = ecma_is_value_undefined (y);
  const bool is_y_null = ecma_is_value_null (y);
  const bool is_y_boolean = ecma_is_value_boolean (y);
  const bool is_y_number = ecma_is_value_number (y);
  const bool is_y_string = ecma_is_value_string (y);
  const bool is_y_object = ecma_is_value_object (y);

  const bool is_types_equal = ((is_x_undefined && is_y_undefined)
                               || (is_x_null && is_y_null)
                               || (is_x_boolean && is_y_boolean)
                               || (is_x_number && is_y_number)
                               || (is_x_string && is_y_string)
                               || (is_x_object && is_y_object));

  // 1. If Type (x) is different from Type (y), return false.
  if (!is_types_equal)
  {
    return false;
  }

  // 2. If Type (x) is Undefined, return true.
  if (is_x_undefined)
  {
    return true;
  }

  // 3. If Type (x) is Null, return true.
  if (is_x_null)
  {
    return true;
  }

  // 4. If Type (x) is Number, then
  if (is_x_number)
  {
    // a. If x is NaN, return false.
    // b. If y is NaN, return false.
    // c. If x is the same Number value as y, return true.
    // d. If x is +0 and y is -0, return true.
    // e. If x is -0 and y is +0, return true.

    ecma_number_t x_num = *ecma_get_number_from_value (x);
    ecma_number_t y_num = *ecma_get_number_from_value (y);

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
  }

  // 5. If Type (x) is String, then return true if x and y are exactly the same sequence of characters
  // (same length and same characters in corresponding positions); otherwise, return false.
  if (is_x_string)
  {
    ecma_string_t* x_str_p = ecma_get_string_from_value (x);
    ecma_string_t* y_str_p = ecma_get_string_from_value (y);

    return ecma_compare_ecma_strings (x_str_p, y_str_p);
  }

  // 6. If Type (x) is Boolean, return true if x and y are both true or both false; otherwise, return false.
  if (is_x_boolean)
  {
    return (ecma_is_value_true (x) == ecma_is_value_true (y));
  }

  // 7. Return true if x and y refer to the same object. Otherwise, return false.
  JERRY_ASSERT (is_x_object);

  return (ecma_get_object_from_value (x) == ecma_get_object_from_value (y));
} /* ecma_op_strict_equality_compare */

/**
 * ECMA abstract relational comparison routine.
 *
 * See also: ECMA-262 v5, 11.8.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_abstract_relational_compare (ecma_value_t x, /**< first operand */
                                     ecma_value_t y, /**< second operand */
                                     bool left_first) /**< 'LeftFirst' flag */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ecma_value_t first_converted_value = left_first ? x : y;
  ecma_value_t second_converted_value = left_first ? y : x;

  // 1., 2.
  ECMA_TRY_CATCH (prim_first_converted_value,
                  ecma_op_to_primitive (first_converted_value, ECMA_PREFERRED_TYPE_NUMBER),
                  ret_value);
  ECMA_TRY_CATCH (prim_second_converted_value,
                  ecma_op_to_primitive (second_converted_value, ECMA_PREFERRED_TYPE_NUMBER),
                  ret_value);

  const ecma_value_t &px = left_first ? prim_first_converted_value : prim_second_converted_value;
  const ecma_value_t &py = left_first ? prim_second_converted_value : prim_first_converted_value;

  const bool is_px_string = ecma_is_value_string (px);
  const bool is_py_string = ecma_is_value_string (py);

  if (!(is_px_string && is_py_string))
  {
    // 3.

    // a.
    ECMA_OP_TO_NUMBER_TRY_CATCH (nx, px, ret_value);
    ECMA_OP_TO_NUMBER_TRY_CATCH (ny, py, ret_value);

    // b.
    if (ecma_number_is_nan (nx)
        || ecma_number_is_nan (ny))
    {
      // c., d.
      ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
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
        // e., f., g.
        is_x_less_than_y_check = false;
      }
      else if (ecma_number_is_infinity (nx)
               && !ecma_number_is_negative (nx))
      {
        // h.
        is_x_less_than_y_check = false;
      }
      else if (ecma_number_is_infinity (ny)
               && !ecma_number_is_negative (ny))
      {
        // i.
        is_x_less_than_y_check = true;
      }
      else if (ecma_number_is_infinity (ny)
               && ecma_number_is_negative (ny))
      {
        // j.
        is_x_less_than_y_check = false;
      }
      else if (ecma_number_is_infinity (nx)
               && ecma_number_is_negative (nx))
      {
        // k.
        is_x_less_than_y_check = true;
      }
      else
      {
        // l.
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

      ret_value = ecma_make_simple_completion_value (is_x_less_than_y ?
                                                     ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
    }

    ECMA_OP_TO_NUMBER_FINALIZE (ny);
    ECMA_OP_TO_NUMBER_FINALIZE (nx);
  }
  else
  { // 4.
    JERRY_ASSERT (is_px_string && is_py_string);

    ecma_string_t *str_x_p = ecma_get_string_from_value (px);
    ecma_string_t *str_y_p = ecma_get_string_from_value (py);

    bool is_px_less = ecma_compare_ecma_strings_relational (str_x_p, str_y_p);

    ret_value = ecma_make_simple_completion_value (is_px_less ? ECMA_SIMPLE_VALUE_TRUE
                                                              : ECMA_SIMPLE_VALUE_FALSE);
  }

  ECMA_FINALIZE (prim_second_converted_value);
  ECMA_FINALIZE (prim_first_converted_value);

  return ret_value;
} /* ecma_op_abstract_relational_compare */

/**
 * @}
 * @}
 */

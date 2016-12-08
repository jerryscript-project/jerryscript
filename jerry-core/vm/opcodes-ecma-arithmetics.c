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

#include "ecma-alloc.h"
#include "ecma-conversion.h"
#include "ecma-helpers.h"
#include "ecma-number-arithmetic.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "opcodes.h"
#include "jrt-libc-includes.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup vm_opcodes Opcodes
 * @{
 */

/**
 * Perform ECMA number arithmetic operation.
 *
 * The algorithm of the operation is following:
 *   leftNum = ToNumber (leftValue);
 *   rightNum = ToNumber (rightValue);
 *   result = leftNum ArithmeticOp rightNum;
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
do_number_arithmetic (number_arithmetic_op op, /**< number arithmetic operation */
                      ecma_value_t left_value, /**< left value */
                      ecma_value_t right_value) /**< right value */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (num_left, left_value, ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (num_right, right_value, ret_value);

  ecma_number_t result = ECMA_NUMBER_ZERO;

  switch (op)
  {
    case NUMBER_ARITHMETIC_SUBSTRACTION:
    {
      result = ecma_number_substract (num_left, num_right);
      break;
    }
    case NUMBER_ARITHMETIC_MULTIPLICATION:
    {
      result = ecma_number_multiply (num_left, num_right);
      break;
    }
    case NUMBER_ARITHMETIC_DIVISION:
    {
      result = ecma_number_divide (num_left, num_right);
      break;
    }
    case NUMBER_ARITHMETIC_REMAINDER:
    {
      result = ecma_op_number_remainder (num_left, num_right);
      break;
    }
  }

  ret_value = ecma_make_number_value (result);

  ECMA_OP_TO_NUMBER_FINALIZE (num_right);
  ECMA_OP_TO_NUMBER_FINALIZE (num_left);

  return ret_value;
} /* do_number_arithmetic */

/**
 * 'Addition' opcode handler.
 *
 * See also: ECMA-262 v5, 11.6.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_addition (ecma_value_t left_value, /**< left value */
                 ecma_value_t right_value) /**< right value */
{
  bool free_left_value = false;
  bool free_right_value = false;

  if (ecma_is_value_object (left_value))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (left_value);
    left_value = ecma_op_object_default_value (obj_p, ECMA_PREFERRED_TYPE_NO);
    free_left_value = true;

    if (ECMA_IS_VALUE_ERROR (left_value))
    {
      return left_value;
    }
  }

  if (ecma_is_value_object (right_value))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (right_value);
    right_value = ecma_op_object_default_value (obj_p, ECMA_PREFERRED_TYPE_NO);
    free_right_value = true;

    if (ECMA_IS_VALUE_ERROR (right_value))
    {
      if (free_left_value)
      {
        ecma_free_value (left_value);
      }
      return right_value;
    }
  }

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (ecma_is_value_string (left_value)
      || ecma_is_value_string (right_value))
  {
    ECMA_TRY_CATCH (str_left_value, ecma_op_to_string (left_value), ret_value);
    ECMA_TRY_CATCH (str_right_value, ecma_op_to_string (right_value), ret_value);

    ecma_string_t *string1_p = ecma_get_string_from_value (str_left_value);
    ecma_string_t *string2_p = ecma_get_string_from_value (str_right_value);

    ecma_string_t *concat_str_p = ecma_concat_ecma_strings (string1_p, string2_p);

    ret_value = ecma_make_string_value (concat_str_p);

    ECMA_FINALIZE (str_right_value);
    ECMA_FINALIZE (str_left_value);
  }
  else
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (num_left, left_value, ret_value);
    ECMA_OP_TO_NUMBER_TRY_CATCH (num_right, right_value, ret_value);

    ret_value = ecma_make_number_value (ecma_number_add (num_left, num_right));

    ECMA_OP_TO_NUMBER_FINALIZE (num_right);
    ECMA_OP_TO_NUMBER_FINALIZE (num_left);
  }

  if (free_left_value)
  {
    ecma_free_value (left_value);
  }

  if (free_right_value)
  {
    ecma_free_value (right_value);
  }

  return ret_value;
} /* opfunc_addition */

/**
 * 'Unary "+"' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4, 11.4.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_unary_plus (ecma_value_t left_value) /**< left value */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (num_var_value,
                               left_value,
                               ret_value);

  ret_value = ecma_make_number_value (num_var_value);

  ECMA_OP_TO_NUMBER_FINALIZE (num_var_value);

  return ret_value;
} /* opfunc_unary_plus */

/**
 * 'Unary "-"' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4, 11.4.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_unary_minus (ecma_value_t left_value) /**< left value */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (num_var_value,
                               left_value,
                               ret_value);

  ret_value = ecma_make_number_value (ecma_number_negate (num_var_value));

  ECMA_OP_TO_NUMBER_FINALIZE (num_var_value);

  return ret_value;
} /* opfunc_unary_minus */

/**
 * @}
 * @}
 */

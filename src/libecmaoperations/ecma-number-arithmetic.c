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

#include "ecma-globals.h"
#include "ecma-number-arithmetic.h"

/** \addtogroup ecma ---TODO---
 * @{
 */

/**
 * \addtogroup numberarithmetic ECMA number arithmetic operations
 * @{
 */

/**
 * ECMA-defined number addition.
 *
 * See also:
 *          ECMA-262 v5, 11.6.3
 *
 * @return number - result of addition.
 */
ecma_number_t
ecma_op_number_add (ecma_number_t left_num, /**< left operand */
                   ecma_number_t right_num) /**< right operand */
{
  TODO(Implement according to ECMA);

  return left_num + right_num;
} /* ecma_op_number_add */

/**
 * ECMA-defined number substraction.
 *
 * See also:
 *          ECMA-262 v5, 11.6.3
 *
 * @return number - result of substraction.
 */
ecma_number_t
ecma_op_number_substract (ecma_number_t left_num, /**< left operand */
                         ecma_number_t right_num) /**< right operand */
{
  return ecma_op_number_add (left_num, ecma_op_number_negate (right_num));
} /* ecma_op_number_substract */

/**
 * ECMA-defined number multiplication.
 *
 * See also:
 *          ECMA-262 v5, 11.5.1
 *
 * @return number - result of multiplication.
 */
ecma_number_t
ecma_op_number_multiply (ecma_number_t left_num, /**< left operand */
                        ecma_number_t right_num) /**< right operand */
{
  TODO(Implement according to ECMA);

  return left_num * right_num;
} /* ecma_op_number_multiply */

/**
 * ECMA-defined number division.
 *
 * See also:
 *          ECMA-262 v5, 11.5.2
 *
 * @return number - result of division.
 */
ecma_number_t
ecma_op_number_divide (ecma_number_t left_num, /**< left operand */
                      ecma_number_t right_num) /**< right operand */
{
  TODO(Implement according to ECMA);

  return left_num / right_num;
} /* ecma_op_number_divide */

/**
 * ECMA-defined number remainder calculation.
 *
 * See also:
 *          ECMA-262 v5, 11.5.3
 *
 * @return number - calculated remainder.
 */
ecma_number_t
ecma_op_number_remainder (ecma_number_t left_num, /**< left operand */
                         ecma_number_t right_num) /**< right operand */
{
  TODO(Implement according to ECMA);

  ecma_number_t n = left_num, d = right_num;

  return (n - d * (ecma_number_t)((int32_t)(n / d)));
} /* ecma_op_number_remainder */

/**
 * ECMA-defined number negation.
 *
 * See also:
 *          ECMA-262 v5, 11.4.7
 *
 * @return number - result of negation.
 */
ecma_number_t
ecma_op_number_negate (ecma_number_t num) /**< operand */
{
  TODO(Implement according to ECMA);

  return -num;
} /* ecma_op_number_negate */

/**
 * @}
 * @}
 */

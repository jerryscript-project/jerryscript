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
 * ECMA-defined number multiplication.
 *
 * See also:
 *          ECMA-262 v5, 11.5.1
 *
 * @return number - result of multiplication.
 */
ecma_Number_t
ecma_op_number_multiply(ecma_Number_t left_num, /**< left operand */
                        ecma_Number_t right_num) /**< right operand */
{
  TODO( Implement according to ECMA );

  return left_num * right_num;
} /* ecma_op_number_multiply */

/**
 * @}
 * @}
 */

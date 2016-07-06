/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged.
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

#ifndef OPCODES_H
#define OPCODES_H

#include "ecma-globals.h"
#include "vm-defines.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup vm_opcodes Opcodes
 * @{
 */

/**
 * Number arithmetic operations.
 */
typedef enum
{
  NUMBER_ARITHMETIC_SUBSTRACTION, /**< substraction */
  NUMBER_ARITHMETIC_MULTIPLICATION, /**< multiplication */
  NUMBER_ARITHMETIC_DIVISION, /**< division */
  NUMBER_ARITHMETIC_REMAINDER, /**< remainder calculation */
} number_arithmetic_op;

/**
 * Number bitwise logic operations.
 */
typedef enum
{
  NUMBER_BITWISE_LOGIC_AND, /**< bitwise AND calculation */
  NUMBER_BITWISE_LOGIC_OR, /**< bitwise OR calculation */
  NUMBER_BITWISE_LOGIC_XOR, /**< bitwise XOR calculation */
  NUMBER_BITWISE_SHIFT_LEFT, /**< bitwise LEFT SHIFT calculation */
  NUMBER_BITWISE_SHIFT_RIGHT, /**< bitwise RIGHT_SHIFT calculation */
  NUMBER_BITWISE_SHIFT_URIGHT, /**< bitwise UNSIGNED RIGHT SHIFT calculation */
  NUMBER_BITWISE_NOT, /**< bitwise NOT calculation */
} number_bitwise_logic_op;

ecma_value_t
vm_var_decl (vm_frame_ctx_t *, ecma_string_t *);

ecma_value_t
opfunc_equal_value (ecma_value_t, ecma_value_t);

ecma_value_t
opfunc_not_equal_value (ecma_value_t, ecma_value_t);

ecma_value_t
do_number_arithmetic (number_arithmetic_op, ecma_value_t, ecma_value_t);

ecma_value_t
opfunc_unary_plus (ecma_value_t);

ecma_value_t
opfunc_unary_minus (ecma_value_t);

ecma_value_t
do_number_bitwise_logic (number_bitwise_logic_op, ecma_value_t, ecma_value_t);

ecma_value_t
opfunc_addition (ecma_value_t, ecma_value_t);

ecma_value_t
opfunc_less_than (ecma_value_t, ecma_value_t);

ecma_value_t
opfunc_greater_than (ecma_value_t, ecma_value_t);

ecma_value_t
opfunc_less_or_equal_than (ecma_value_t, ecma_value_t);

ecma_value_t
opfunc_greater_or_equal_than (ecma_value_t, ecma_value_t);

ecma_value_t
opfunc_in (ecma_value_t, ecma_value_t);

ecma_value_t
opfunc_instanceof (ecma_value_t, ecma_value_t);

ecma_value_t
opfunc_logical_not (ecma_value_t);

ecma_value_t
opfunc_typeof (ecma_value_t);

void
opfunc_set_accessor (bool, ecma_value_t, ecma_value_t, ecma_value_t);

ecma_value_t
vm_op_delete_prop (ecma_value_t, ecma_value_t, bool);

ecma_value_t
vm_op_delete_var (jmem_cpointer_t, ecma_object_t *);

ecma_collection_header_t *
opfunc_for_in (ecma_value_t, ecma_value_t *);

/**
 * @}
 * @}
 */

#endif /* !OPCODES_H */

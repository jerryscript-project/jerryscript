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

#ifndef JERRY_ECMA_CONVERSION_H
#define JERRY_ECMA_CONVERSION_H

#include "ecma-globals.h"
#include "ecma-helpers.h"

/** \addtogroup ecma ---TODO---
 * @{
 *
 * \addtogroup ecmaconversion ECMA conversion
 * @{
 */

/**
 * Second argument of 'ToPrimitive' operation that is a hint,
 * specifying the preferred type of conversion result.
 */
typedef enum
{
  ECMA_PREFERRED_TYPE_NO, /**< no preferred type is specified */
  ECMA_PREFERRED_TYPE_UNDEFINED, /**< Undefined */
  ECMA_PREFERRED_TYPE_NULL, /**< Null */
  ECMA_PREFERRED_TYPE_BOOLEAN, /**< Boolean */
  ECMA_PREFERRED_TYPE_NUMBER, /**< Number */
  ECMA_PREFERRED_TYPE_STRING /**< String */
} ecma_preferred_type_hint_t;

extern ecma_completion_value_t ecma_op_check_object_coercible( ecma_value_t value);
extern bool ecma_op_same_value( ecma_value_t x, ecma_value_t y);
extern ecma_completion_value_t ecma_op_to_primitive( ecma_value_t value, ecma_preferred_type_hint_t preferred_type);
extern ecma_completion_value_t ecma_op_to_boolean( ecma_value_t value);
extern ecma_completion_value_t ecma_op_to_number( ecma_value_t value);
extern ecma_completion_value_t ecma_op_to_object( ecma_value_t value);

/**
 * @}
 * @}
 */

#endif /* !JERRY_ECMA_CONVERSION_H */

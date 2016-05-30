/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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

#ifndef ECMA_CONVERSION_H
#define ECMA_CONVERSION_H

#include "ecma-globals.h"
#include "ecma-helpers.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaconversion ECMA conversion routines
 * @{
 */

/**
 * Second argument of 'ToPrimitive' operation that is a hint,
 * specifying the preferred type of conversion result.
 */
typedef enum
{
  ECMA_PREFERRED_TYPE_NO, /**< no preferred type is specified */
  ECMA_PREFERRED_TYPE_NUMBER, /**< Number */
  ECMA_PREFERRED_TYPE_STRING /**< String */
} ecma_preferred_type_hint_t;

extern ecma_value_t ecma_op_check_object_coercible (ecma_value_t);
extern bool ecma_op_same_value (ecma_value_t, ecma_value_t);
extern ecma_value_t ecma_op_to_primitive (ecma_value_t, ecma_preferred_type_hint_t);
extern bool ecma_op_to_boolean (ecma_value_t);
extern ecma_value_t ecma_op_to_number (ecma_value_t);
extern ecma_value_t ecma_op_to_string (ecma_value_t);
extern ecma_value_t ecma_op_to_object (ecma_value_t);

extern ecma_object_t *ecma_op_from_property_descriptor (const ecma_property_descriptor_t *);
extern ecma_value_t ecma_op_to_property_descriptor (ecma_value_t, ecma_property_descriptor_t *);

/**
 * @}
 * @}
 */

#endif /* !ECMA_CONVERSION_H */

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

#ifndef ECMA_ITERATOR_OBJECT_H
#define ECMA_ITERATOR_OBJECT_H

#include "ecma-globals.h"

#if ENABLED (JERRY_ES2015_BUILTIN_ITERATOR)

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaiteratorobject ECMA iterator object related routines
 * @{
 */

/**
 * Maximum value of [[%Iterator%NextIndex]] until it can be stored
 * in an ecma pseudo array object structure element.
 */
#define ECMA_ITERATOR_INDEX_LIMIT UINT16_MAX

ecma_value_t
ecma_op_create_iterator_object (ecma_value_t iterated_value, ecma_object_t *prototype_obj_p,
                                uint8_t iterator_type, uint8_t extra_info);

ecma_value_t
ecma_create_iter_result_object (ecma_value_t value, ecma_value_t done);

ecma_value_t
ecma_create_array_from_iter_element (ecma_value_t value, ecma_value_t index_value);

ecma_value_t
ecma_op_get_iterator (ecma_value_t value, ecma_value_t method);

ecma_value_t
ecma_op_iterator_value (ecma_value_t iter_result);

ecma_value_t
ecma_op_iterator_step (ecma_value_t iterator);

#endif /* ENABLED (JERRY_ES2015_BUILTIN_ITERATOR) */

/**
 * @}
 * @}
 */

#endif /* !ECMA_ITERATOR_OBJECT_H */

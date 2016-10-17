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

#ifndef ECMA_ARRAY_OBJECT_H
#define ECMA_ARRAY_OBJECT_H

#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaarrayobject ECMA Array object related routines
 * @{
 */

/**
 * Flags for ecma_op_array_object_set_length
 */
typedef enum
{
  ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_IS_THROW = 1u << 0, /**< is_throw flag is set */
  ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_REJECT = 1u << 1, /**< reject later because the descriptor flags
                                                       *   contains an unallowed combination */
  ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE_DEFINED = 1u << 2, /**< writable flag defined
                                                                 *   in the property descriptor */
  ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE = 1u << 3, /**< writable flag enabled
                                                         *   in the property descriptor */
} ecma_array_object_set_length_flags_t;

extern ecma_value_t
ecma_op_create_array_object (const ecma_value_t *, ecma_length_t, bool);

extern ecma_value_t
ecma_op_array_object_set_length (ecma_object_t *, ecma_value_t, uint32_t);

extern ecma_value_t
ecma_op_array_object_define_own_property (ecma_object_t *, ecma_string_t *, const ecma_property_descriptor_t *, bool);

/**
 * @}
 * @}
 */

#endif /* !ECMA_ARRAY_OBJECT_H */

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

ecma_value_t
ecma_op_create_array_object (const ecma_value_t *arguments_list_p, ecma_length_t arguments_list_len,
                             bool is_treat_single_arg_as_length);

ecma_value_t
ecma_op_array_object_set_length (ecma_object_t *object_p, ecma_value_t new_value, uint32_t flags);

ecma_value_t
ecma_op_array_object_define_own_property (ecma_object_t *object_p, ecma_string_t *property_name_p,
                                          const ecma_property_descriptor_t *property_desc_p, bool is_throw);

void
ecma_op_array_list_lazy_property_names (ecma_object_t *obj_p, bool separate_enumerable,
                                        ecma_collection_header_t *main_collection_p,
                                        ecma_collection_header_t *non_enum_collection_p);

/**
 * @}
 * @}
 */

#endif /* !ECMA_ARRAY_OBJECT_H */

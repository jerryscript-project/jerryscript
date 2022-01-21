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

#ifndef ECMA_STRING_OBJECT_H
#define ECMA_STRING_OBJECT_H

#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmastringobject ECMA String object related routines
 * @{
 */

ecma_value_t ecma_op_create_string_object (const ecma_value_t *arguments_list_p, uint32_t arguments_list_len);

ecma_property_descriptor_t ecma_string_object_get_own_property (ecma_object_t *obj_p, ecma_string_t *property_name_p);
void ecma_string_object_list_lazy_property_keys (ecma_object_t *obj_p,
                                                 ecma_collection_t *prop_names_p,
                                                 ecma_property_counter_t *prop_counter_p,
                                                 jerry_property_filter_t filter);

/**
 * Virtual function table for String object's internal methods
 */
#define ECMA_STRING_OBJ_VTABLE                                               \
  [ECMA_OBJECT_CLASS_STRING] = { NULL,                                       \
                                 NULL,                                       \
                                 NULL,                                       \
                                 NULL,                                       \
                                 ecma_string_object_get_own_property,        \
                                 ecma_ordinary_object_define_own_property,   \
                                 NULL,                                       \
                                 ecma_ordinary_object_get,                   \
                                 ecma_ordinary_object_set,                   \
                                 ecma_ordinary_object_delete,                \
                                 NULL,                                       \
                                 NULL,                                       \
                                 NULL,                                       \
                                 ecma_string_object_list_lazy_property_keys, \
                                 ecma_ordinary_object_delete_lazy_property }

/**
 * @}
 * @}
 */

#endif /* !ECMA_STRING_OBJECT_H */

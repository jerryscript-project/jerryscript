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

#ifndef ECMA_CLASS_OBJECT_H
#define ECMA_CLASS_OBJECT_H

#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaclassobject ECMA Class object related routines
 * @{
 */

ecma_property_descriptor_t ecma_class_object_get_own_property (ecma_object_t *obj_p, ecma_string_t *property_name_p);
ecma_value_t ecma_class_object_define_own_property (ecma_object_t *obj_p,
                                                    ecma_string_t *property_name_p,
                                                    const ecma_property_descriptor_t *property_desc_p);
ecma_value_t ecma_class_object_get (ecma_object_t *obj_p, ecma_string_t *property_name_p, ecma_value_t receiver);
ecma_value_t ecma_class_object_set (ecma_object_t *obj_p,
                                    ecma_string_t *property_name_p,
                                    ecma_value_t value,
                                    ecma_value_t receiver,
                                    bool is_throw);
ecma_value_t ecma_class_object_delete_property (ecma_object_t *obj_p, ecma_string_t *property_name_p, bool is_strict);
void ecma_class_object_list_lazy_property_keys (ecma_object_t *object_p,
                                                ecma_collection_t *prop_names_p,
                                                ecma_property_counter_t *prop_counter_p,
                                                jerry_property_filter_t filter);
void ecma_class_object_delete_lazy_property (ecma_object_t *obj_p, ecma_string_t *property_name_p);

/**
 * Virtual function table for Class object's internal methods
 */
#define ECMA_CLASS_OBJ_VTABLE                                             \
  [ECMA_OBJECT_TYPE_CLASS] = { ecma_ordinary_object_get_prototype_of,     \
                               ecma_ordinary_object_set_prototype_of,     \
                               ecma_ordinary_object_is_extensible,        \
                               ecma_ordinary_object_prevent_extensions,   \
                               ecma_class_object_get_own_property,        \
                               ecma_class_object_define_own_property,     \
                               ecma_ordinary_object_has_property,         \
                               ecma_class_object_get,                     \
                               ecma_class_object_set,                     \
                               ecma_class_object_delete_property,         \
                               ecma_ordinary_object_own_property_keys,    \
                               ecma_ordinary_object_call,                 \
                               ecma_ordinary_object_construct,            \
                               ecma_class_object_list_lazy_property_keys, \
                               ecma_class_object_delete_lazy_property }

/**
 * @}
 * @}
 */

#endif /* ECMA_CLASS_OBJECT_H */

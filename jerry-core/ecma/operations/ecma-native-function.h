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

#ifndef ECMA_OP_NATIVE_FUNCTION_H
#define ECMA_OP_NATIVE_FUNCTION_H

#include "ecma-builtin-handlers.h"
#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmanativefunction ECMA native function related routines
 * @{
 */

#if JERRY_ESNEXT
ecma_object_t *ecma_native_function_create (ecma_native_handler_id_t id, size_t object_size);
#endif /* JERRY_ESNEXT */

ecma_property_descriptor_t ecma_native_function_get_own_property (ecma_object_t *obj_p, ecma_string_t *property_name_p);
void ecma_native_object_list_lazy_property_keys (ecma_object_t *object_p,
                                                 ecma_collection_t *prop_names_p,
                                                 ecma_property_counter_t *prop_counter_p,
                                                 jerry_property_filter_t filter);
ecma_value_t ecma_native_function_call (ecma_object_t *func_obj_p,
                                        ecma_value_t this_arg_value,
                                        const ecma_value_t *arguments_list_p,
                                        uint32_t arguments_list_len);
ecma_value_t ecma_native_function_construct (ecma_object_t *func_obj_p,
                                             ecma_object_t *new_target_p,
                                             const ecma_value_t *arguments_list_p,
                                             uint32_t arguments_list_len);

/**
 * Virtual function table for native function object's internal methods
 */
#define ECMA_NATIVE_FUNCTION_OBJ_VTABLE                                              \
  [ECMA_OBJECT_TYPE_NATIVE_FUNCTION] = { ecma_ordinary_object_get_prototype_of,      \
                                         ecma_ordinary_object_set_prototype_of,      \
                                         ecma_ordinary_object_is_extensible,         \
                                         ecma_ordinary_object_prevent_extensions,    \
                                         ecma_native_function_get_own_property,      \
                                         ecma_ordinary_object_define_own_property,   \
                                         ecma_ordinary_object_has_property,          \
                                         ecma_ordinary_object_get,                   \
                                         ecma_ordinary_object_set,                   \
                                         ecma_ordinary_object_delete,                \
                                         ecma_ordinary_object_own_property_keys,     \
                                         ecma_native_function_call,                  \
                                         ecma_native_function_construct,             \
                                         ecma_native_object_list_lazy_property_keys, \
                                         ecma_ordinary_object_delete_lazy_property }

/**
 * @}
 * @}
 */

#endif /* !ECMA_OP_NATIVE_FUNCTION_H */

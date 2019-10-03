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

#ifndef ECMA_TYPEDARRAY_OBJECT_H
#define ECMA_TYPEDARRAY_OBJECT_H

#include "ecma-globals.h"
#include "ecma-builtins.h"

#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmatypedarrayobject ECMA TypedArray object related routines
 * @{
 */

uint8_t ecma_typedarray_helper_get_shift_size (ecma_typedarray_type_t typedarray_id);
ecma_typedarray_getter_fn_t ecma_get_typedarray_getter_fn (ecma_typedarray_type_t typedarray_id);
ecma_typedarray_setter_fn_t ecma_get_typedarray_setter_fn (ecma_typedarray_type_t typedarray_id);
ecma_number_t ecma_get_typedarray_element (lit_utf8_byte_t *src_p,
                                           ecma_typedarray_type_t typedarray_id);
void ecma_set_typedarray_element (lit_utf8_byte_t *dst_p,
                                  ecma_number_t value,
                                  ecma_typedarray_type_t typedarray_id);
bool ecma_typedarray_helper_is_typedarray (ecma_builtin_id_t builtin_id);
ecma_typedarray_type_t ecma_get_typedarray_id (ecma_object_t *obj_p);
ecma_builtin_id_t ecma_typedarray_helper_get_prototype_id (ecma_typedarray_type_t typedarray_id);
ecma_typedarray_type_t ecma_typedarray_helper_builtin_to_typedarray_id (ecma_builtin_id_t builtin_id);

ecma_value_t ecma_op_typedarray_from (ecma_value_t items_val,
                                      ecma_value_t map_fn_val,
                                      ecma_value_t this_val,
                                      ecma_object_t *proto_p,
                                      uint8_t element_size_shift,
                                      ecma_typedarray_type_t typedarray_id);
ecma_length_t ecma_typedarray_get_length (ecma_object_t *typedarray_p);
ecma_length_t ecma_typedarray_get_offset (ecma_object_t *typedarray_p);
lit_utf8_byte_t *ecma_typedarray_get_buffer (ecma_object_t *typedarray_p);
uint8_t ecma_typedarray_get_element_size_shift (ecma_object_t *typedarray_p);
ecma_object_t *ecma_typedarray_get_arraybuffer (ecma_object_t *typedarray_p);
ecma_value_t ecma_op_create_typedarray (const ecma_value_t *arguments_list_p,
                                        ecma_length_t arguments_list_len,
                                        ecma_object_t *proto_p,
                                        uint8_t element_size_shift,
                                        ecma_typedarray_type_t typedarray_id);
bool ecma_object_is_typedarray (ecma_object_t *obj_p);
bool ecma_is_typedarray (ecma_value_t target);
void ecma_op_typedarray_list_lazy_property_names (ecma_object_t *obj_p,
                                                  ecma_collection_t *main_collection_p);
bool ecma_op_typedarray_define_index_prop (ecma_object_t *obj_p,
                                           uint32_t index,
                                           const ecma_property_descriptor_t *property_desc_p);
ecma_value_t ecma_op_create_typedarray_with_type_and_length (ecma_object_t *obj_p,
                                                             ecma_length_t array_length);
ecma_typedarray_info_t ecma_typedarray_get_info (ecma_object_t *typedarray_p);
ecma_value_t ecma_typedarray_create_object_with_length (ecma_length_t array_length,
                                                        ecma_object_t *proto_p,
                                                        uint8_t element_size_shift,
                                                        ecma_typedarray_type_t typedarray_id);

/**
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
#endif /* !ECMA_TYPEDARRAY_OBJECT_H */

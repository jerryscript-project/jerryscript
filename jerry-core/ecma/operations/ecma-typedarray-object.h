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
#ifndef CONFIG_DISABLE_TYPEDARRAY_BUILTIN

#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmatypedarrayobject ECMA TypedArray object related routines
 * @{
 */

ecma_value_t ecma_op_typedarray_from (ecma_value_t items_val,
                                      ecma_value_t map_fn_val,
                                      ecma_value_t this_val,
                                      ecma_object_t *proto_p,
                                      uint8_t element_size_shift,
                                      lit_magic_string_id_t class_id);
ecma_length_t ecma_typedarray_get_length (ecma_object_t *typedarray_p);
ecma_length_t ecma_typedarray_get_offset (ecma_object_t *typedarray_p);
uint8_t ecma_typedarray_get_element_size_shift (ecma_object_t *typedarray_p);
ecma_object_t *ecma_typedarray_get_arraybuffer (ecma_object_t *typedarray_p);
ecma_value_t ecma_op_create_typedarray (const ecma_value_t *arguments_list_p,
                                        ecma_length_t arguments_list_len,
                                        ecma_object_t *proto_p,
                                        uint8_t element_size_shift,
                                        lit_magic_string_id_t class_id);
bool ecma_is_typedarray (ecma_value_t target);
void ecma_op_typedarray_list_lazy_property_names (ecma_object_t *obj_p,
                                                  ecma_collection_header_t *main_collection_p);
ecma_value_t ecma_op_typedarray_get_index_prop (ecma_object_t *obj_p, uint32_t index);
bool ecma_op_typedarray_define_index_prop (ecma_object_t *obj_p,
                                           uint32_t index,
                                           const ecma_property_descriptor_t *property_desc_p);
bool ecma_op_typedarray_set_index_prop (ecma_object_t *obj_p, uint32_t index, ecma_value_t value);
ecma_value_t ecma_op_create_typedarray_with_type_and_length (ecma_object_t *obj_p,
                                                             ecma_length_t array_length);

/**
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_TYPEDARRAY_BUILTIN */
#endif /* !ECMA_TYPEDARRAY_OBJECT_H */

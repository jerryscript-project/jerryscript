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

/** \addtogroup ecma ---TODO---
 * @{
 * 
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

#ifndef JERRY_ECMA_HELPERS_H
#define JERRY_ECMA_HELPERS_H

#include "ecma-globals.h"

extern uintptr_t ecma_compress_pointer(void *pointer);
extern void* ecma_decompress_pointer(uintptr_t compressed_pointer);

/**
 * Get value of pointer from specified compressed pointer field.
 */
#define ecma_get_pointer( field) \
    ecma_decompress_pointer( field)

/**
 * Set value of compressed pointer field so that it will correspond
 * to specified non_compressed_pointer.
 */
#define ecma_set_pointer( field, non_compressed_pointer) \
    (field) = ecma_compress_pointer( non_compressed_pointer) & ( ( 1u << ECMA_POINTER_FIELD_WIDTH ) - 1)

/* ecma-helpers-value.c */
extern bool ecma_is_value_empty( ecma_value_t value);
extern bool ecma_is_value_undefined( ecma_value_t value);
extern bool ecma_is_value_null( ecma_value_t value);
extern bool ecma_is_value_boolean( ecma_value_t value);
extern bool ecma_is_value_true( ecma_value_t value);

extern ecma_value_t ecma_make_simple_value( ecma_simple_value_t value);
extern ecma_value_t ecma_make_number_value( ecma_number_t* num_p);
extern ecma_value_t ecma_make_string_value( ecma_array_first_chunk_t* ecma_string_p);
extern ecma_value_t ecma_make_object_value( ecma_object_t* object_p);
extern ecma_value_t ecma_copy_value( const ecma_value_t value);
extern void ecma_free_value( const ecma_value_t value);

extern ecma_completion_value_t ecma_make_completion_value( ecma_completion_type_t type, ecma_value_t value, uint8_t target);
extern ecma_completion_value_t ecma_make_simple_completion_value( ecma_simple_value_t simple_value);
extern ecma_completion_value_t ecma_make_throw_value( ecma_object_t *exception_p);
extern ecma_completion_value_t ecma_make_empty_completion_value( void);
extern ecma_completion_value_t ecma_copy_completion_value( ecma_completion_value_t value);
extern void ecma_free_completion_value( ecma_completion_value_t completion_value);

extern bool ecma_is_completion_value_normal( ecma_completion_value_t value);
extern bool ecma_is_completion_value_throw( ecma_completion_value_t value);
extern bool ecma_is_completion_value_normal_simple_value( ecma_completion_value_t value, ecma_simple_value_t simple_value);
extern bool ecma_is_completion_value_normal_true( ecma_completion_value_t value);
extern bool ecma_is_completion_value_normal_false( ecma_completion_value_t value);
extern bool ecma_is_empty_completion_value( ecma_completion_value_t value);

extern ecma_object_t* ecma_create_object( ecma_object_t *prototype_object_p, bool is_extensible);
extern ecma_object_t* ecma_create_lexical_environment( ecma_object_t *outer_lexical_environment_p, ecma_lexical_environment_type_t type);

/* ecma-helpers.c */
extern ecma_property_t* ecma_create_internal_property(ecma_object_t *object_p, ecma_internal_property_id_t property_id);
extern ecma_property_t* ecma_find_internal_property(ecma_object_t *object_p, ecma_internal_property_id_t property_id);
extern ecma_property_t* ecma_get_internal_property(ecma_object_t *object_p, ecma_internal_property_id_t property_id);

extern ecma_property_t *ecma_create_named_data_property(ecma_object_t *obj_p, ecma_char_t *name_p, ecma_property_writable_value_t writable, ecma_property_enumerable_value_t enumerable, ecma_property_configurable_value_t configurable);
extern ecma_property_t *ecma_create_named_accessor_property(ecma_object_t *obj_p, ecma_char_t *name_p, ecma_object_t *get_p, ecma_object_t *set_p, ecma_property_enumerable_value_t enumerable, ecma_property_configurable_value_t configurable);
extern ecma_property_t *ecma_find_named_property(ecma_object_t *obj_p, ecma_char_t *name_p);
extern ecma_property_t *ecma_get_named_property(ecma_object_t *obj_p, ecma_char_t *name_p);
extern ecma_property_t *ecma_get_named_data_property(ecma_object_t *obj_p, ecma_char_t *name_p);

extern void ecma_free_internal_property(ecma_property_t *prop_p);
extern void ecma_free_named_data_property(ecma_property_t *prop_p);
extern void ecma_free_named_accessor_property(ecma_property_t *prop_p);
extern void ecma_free_property(ecma_property_t *prop_p);

extern void ecma_delete_property( ecma_object_t *obj_p, ecma_property_t *prop_p);

extern ecma_array_first_chunk_t* ecma_new_ecma_string( const ecma_char_t *string_p);
extern ssize_t ecma_copy_ecma_string_chars_to_buffer( ecma_array_first_chunk_t *first_chunk_p, uint8_t *buffer_p, size_t buffer_size);
extern ecma_array_first_chunk_t* ecma_duplicate_ecma_string( ecma_array_first_chunk_t *first_chunk_p);
extern bool ecma_compare_zt_string_to_ecma_string( const ecma_char_t *string_p, const ecma_array_first_chunk_t *ecma_string_p);
extern bool ecma_compare_ecma_string_to_ecma_string(const ecma_array_first_chunk_t *string1_p, const ecma_array_first_chunk_t *string2_p);
extern void ecma_free_array( ecma_array_first_chunk_t *first_chunk_p);

#endif /* !JERRY_ECMA_HELPERS_H */

/**
 * @}
 * @}
 */

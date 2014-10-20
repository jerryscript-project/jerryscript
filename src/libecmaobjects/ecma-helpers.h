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

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

#ifndef JERRY_ECMA_HELPERS_H
#define JERRY_ECMA_HELPERS_H

#include "ecma-globals.h"
#include "mem-allocator.h"

/**
 * Get value of pointer from specified compressed pointer field.
 */
#define ECMA_GET_POINTER(field) \
  ((unlikely (field == ECMA_NULL_POINTER)) ? NULL : mem_decompress_pointer (field))

/**
 * Set value of compressed pointer field so that it will correspond
 * to specified non_compressed_pointer.
 */
#define ECMA_SET_POINTER(field, non_compressed_pointer) \
  do \
  { \
    void *__temp_pointer = non_compressed_pointer; \
    non_compressed_pointer = __temp_pointer; \
  } while (0); \
  \
  (field) = (unlikely ((non_compressed_pointer) == NULL) ? ECMA_NULL_POINTER \
                                                         : (mem_compress_pointer (non_compressed_pointer) \
                                                            & ((1u << ECMA_POINTER_FIELD_WIDTH) - 1)))

/**
 * Set value of non-null compressed pointer field so that it will correspond
 * to specified non_compressed_pointer.
 */
#define ECMA_SET_NON_NULL_POINTER(field, non_compressed_pointer) \
  (field) = (mem_compress_pointer (non_compressed_pointer) & ((1u << ECMA_POINTER_FIELD_WIDTH) - 1))

/* ecma-helpers-value.c */
extern bool ecma_is_value_empty (ecma_value_t value);
extern bool ecma_is_value_undefined (ecma_value_t value);
extern bool ecma_is_value_null (ecma_value_t value);
extern bool ecma_is_value_boolean (ecma_value_t value);
extern bool ecma_is_value_true (ecma_value_t value);

extern ecma_value_t ecma_make_simple_value (ecma_simple_value_t value);
extern ecma_value_t ecma_make_number_value (ecma_number_t* num_p);
extern ecma_value_t ecma_make_string_value (ecma_string_t* ecma_string_p);
extern ecma_value_t ecma_make_object_value (ecma_object_t* object_p);
extern ecma_value_t ecma_copy_value (const ecma_value_t value, bool do_ref_if_object);
extern void ecma_free_value (const ecma_value_t value, bool do_deref_if_object);

extern ecma_completion_value_t ecma_make_completion_value (ecma_completion_type_t type,
                                                           ecma_value_t value);
extern ecma_completion_value_t ecma_make_label_completion_value (ecma_completion_type_t type,
                                                                 uint8_t depth_level,
                                                                 uint16_t offset);
extern ecma_completion_value_t ecma_make_simple_completion_value (ecma_simple_value_t simple_value);
extern ecma_completion_value_t ecma_make_normal_completion_value (ecma_value_t value);
extern ecma_completion_value_t ecma_make_throw_completion_value (ecma_value_t value);
extern ecma_completion_value_t ecma_make_throw_obj_completion_value (ecma_object_t *exception_p);
extern ecma_completion_value_t ecma_make_empty_completion_value (void);
extern ecma_completion_value_t ecma_make_return_completion_value (ecma_value_t value);
extern ecma_completion_value_t ecma_make_exit_completion_value (bool is_successful);
extern ecma_completion_value_t ecma_make_meta_completion_value (void);
extern ecma_completion_value_t ecma_copy_completion_value (ecma_completion_value_t value);
extern void ecma_free_completion_value (ecma_completion_value_t completion_value);

extern bool ecma_is_completion_value_normal (ecma_completion_value_t value);
extern bool ecma_is_completion_value_throw (ecma_completion_value_t value);
extern bool ecma_is_completion_value_return (ecma_completion_value_t value);
extern bool ecma_is_completion_value_exit (ecma_completion_value_t value);
extern bool ecma_is_completion_value_meta (ecma_completion_value_t value);
extern bool ecma_is_completion_value_normal_simple_value (ecma_completion_value_t value,
                                                          ecma_simple_value_t simple_value);
extern bool ecma_is_completion_value_normal_true (ecma_completion_value_t value);
extern bool ecma_is_completion_value_normal_false (ecma_completion_value_t value);
extern bool ecma_is_completion_value_empty (ecma_completion_value_t value);

/* ecma-helpers-string.c */
extern ecma_string_t* ecma_new_ecma_string (const ecma_char_t *string_p);
extern ecma_string_t* ecma_new_ecma_string_from_uint32 (uint32_t uint_number);
extern ecma_string_t* ecma_new_ecma_string_from_number (ecma_number_t number);
extern ecma_string_t* ecma_new_ecma_string_from_lit_index (literal_index_t lit_index);
extern ecma_string_t* ecma_new_ecma_string_from_magic_string_id (ecma_magic_string_id_t id);
extern ecma_string_t* ecma_concat_ecma_strings (ecma_string_t *string1_p, ecma_string_t *string2_p);
extern ecma_string_t* ecma_copy_or_ref_ecma_string (ecma_string_t *string_desc_p);
extern void ecma_deref_ecma_string (ecma_string_t *string_p);
extern ecma_number_t ecma_string_to_number (const ecma_string_t *str_p);
extern ssize_t ecma_string_to_zt_string (const ecma_string_t *string_desc_p,
                                         ecma_char_t *buffer_p,
                                         ssize_t buffer_size);
extern bool ecma_compare_ecma_strings (const ecma_string_t *string1_p,
                                       const ecma_string_t *string2_p);
extern bool ecma_compare_ecma_strings_relational (const ecma_string_t *string1_p,
                                                  const ecma_string_t *string2_p);
extern int32_t ecma_string_get_length (const ecma_string_t *string_p);
extern ecma_char_t ecma_string_get_char_at_pos (const ecma_string_t *string_p, uint32_t index);
extern bool ecma_compare_zt_strings (const ecma_char_t *string1_p, const ecma_char_t *string2_p);
extern bool ecma_compare_zt_strings_relational (const ecma_char_t *string1_p, const ecma_char_t *string2_p);
extern ecma_char_t*
ecma_copy_zt_string_to_buffer (const ecma_char_t *string_p,
                               ecma_char_t *buffer_p,
                               ssize_t buffer_size);
extern ecma_length_t ecma_zt_string_length (const ecma_char_t *string_p);

extern void ecma_strings_init (void);
extern const ecma_char_t* ecma_get_magic_string_zt (ecma_magic_string_id_t id);
extern ecma_string_t* ecma_get_magic_string (ecma_magic_string_id_t id);
extern bool ecma_is_string_magic (ecma_string_t *string_p, ecma_magic_string_id_t *out_id_p);
extern bool ecma_is_zt_string_magic (ecma_char_t *zt_string_p, ecma_magic_string_id_t *out_id_p);

/* ecma-helpers-number.c */
extern const ecma_number_t ecma_number_relative_eps;

extern ecma_number_t ecma_number_make_nan (void);
extern ecma_number_t ecma_number_make_infinity (bool sign);
extern bool ecma_number_is_nan (ecma_number_t num);
extern bool ecma_number_is_negative (ecma_number_t num);
extern bool ecma_number_is_zero (ecma_number_t num);
extern bool ecma_number_is_infinity (ecma_number_t num);
extern int32_t ecma_number_get_fraction_and_exponent (ecma_number_t num,
                                                      uint64_t *out_fraction_p,
                                                      int32_t *out_exponent_p);
extern ecma_number_t ecma_number_make_normal_positive_from_fraction_and_exponent (uint64_t fraction,
                                                                                  int32_t exponent);
extern ecma_number_t ecma_number_negate (ecma_number_t num);
extern ecma_number_t ecma_number_add (ecma_number_t left_num, ecma_number_t right_num);
extern ecma_number_t ecma_number_substract (ecma_number_t left_num, ecma_number_t right_num);
extern ecma_number_t ecma_number_multiply (ecma_number_t left_num, ecma_number_t right_num);
extern ecma_number_t ecma_number_divide (ecma_number_t left_num, ecma_number_t right_num);
extern ecma_number_t ecma_number_sqrt (ecma_number_t num);
extern ecma_number_t ecma_number_abs (ecma_number_t num);
extern ecma_number_t ecma_number_ln (ecma_number_t num);
extern ecma_number_t ecma_number_exp (ecma_number_t num);

/* ecma-helpers-values-collection.c */

extern ecma_collection_header_t *ecma_new_values_collection (ecma_value_t values_buffer[],
                                                             ecma_length_t values_number,
                                                             bool do_ref_if_object);
extern void ecma_free_values_collection (ecma_collection_header_t* header_p, bool do_deref_if_object);
extern ecma_collection_header_t *ecma_new_strings_collection (ecma_string_t* string_ptrs_buffer[],
                                                              ecma_length_t strings_number);

/**
 * Context of ecma-values' collection iterator
 */
typedef struct
{
  ecma_collection_header_t *header_p; /**< collection header */
  uint16_t next_chunk_cp; /**< compressed pointer to next chunk */
  ecma_length_t current_index; /**< index of current element */
  ecma_value_t *current_value_p; /**< pointer to current element */
  ecma_value_t *current_chunk_beg_p; /**< pointer to beginning of current chunk's data */
  ecma_value_t *current_chunk_end_p; /**< pointer to place right after the end of current chunk's data */
} ecma_collection_iterator_t;

extern void
ecma_collection_iterator_init (ecma_collection_iterator_t *iterator_p,
                               ecma_collection_header_t *collection_p);
extern bool
ecma_collection_iterator_next (ecma_collection_iterator_t *iterator_p);

/* ecma-helpers.c */
extern ecma_object_t* ecma_create_object (ecma_object_t *prototype_object_p,
                                          bool is_extensible,
                                          ecma_object_type_t type);
extern ecma_object_t* ecma_create_decl_lex_env (ecma_object_t *outer_lexical_environment_p);
extern ecma_object_t* ecma_create_object_lex_env (ecma_object_t *outer_lexical_environment_p,
                                                  ecma_object_t *binding_obj_p,
                                                  bool provide_this);
extern bool ecma_is_lexical_environment (ecma_object_t *object_p);
extern bool ecma_get_object_extensible (ecma_object_t *object_p);
extern void ecma_set_object_extensible (ecma_object_t *object_p, bool is_extensible);
extern ecma_object_type_t ecma_get_object_type (ecma_object_t *object_p);
extern void ecma_set_object_type (ecma_object_t *object_p, ecma_object_type_t type);
extern ecma_object_t* ecma_get_object_prototype (ecma_object_t *object_p);
extern bool ecma_get_object_is_builtin (ecma_object_t *object_p);
extern void ecma_set_object_is_builtin (ecma_object_t *object_p,
                                        bool is_builtin);
extern ecma_lexical_environment_type_t ecma_get_lex_env_type (ecma_object_t *object_p);
extern ecma_object_t *ecma_get_lex_env_outer_reference (ecma_object_t *object_p);
extern ecma_property_t *ecma_get_property_list (ecma_object_t *object_p);

extern ecma_property_t* ecma_create_internal_property (ecma_object_t *object_p,
                                                       ecma_internal_property_id_t property_id);
extern ecma_property_t* ecma_find_internal_property (ecma_object_t *object_p,
                                                     ecma_internal_property_id_t property_id);
extern ecma_property_t* ecma_get_internal_property (ecma_object_t *object_p,
                                                    ecma_internal_property_id_t property_id);

extern ecma_property_t *ecma_create_named_data_property (ecma_object_t *obj_p,
                                                         ecma_string_t *name_p,
                                                         ecma_property_writable_value_t writable,
                                                         ecma_property_enumerable_value_t enumerable,
                                                         ecma_property_configurable_value_t configurable);
extern ecma_property_t *ecma_create_named_accessor_property (ecma_object_t *obj_p,
                                                             ecma_string_t *name_p,
                                                             ecma_object_t *get_p,
                                                             ecma_object_t *set_p,
                                                             ecma_property_enumerable_value_t enumerable,
                                                             ecma_property_configurable_value_t configurable);
extern ecma_property_t *ecma_find_named_property (ecma_object_t *obj_p,
                                                  const ecma_string_t *name_p);
extern ecma_property_t *ecma_get_named_property (ecma_object_t *obj_p,
                                                 const ecma_string_t *name_p);
extern ecma_property_t *ecma_get_named_data_property (ecma_object_t *obj_p,
                                                      const ecma_string_t *name_p);

extern void ecma_free_internal_property (ecma_property_t *prop_p);
extern void ecma_free_named_data_property (ecma_property_t *prop_p);
extern void ecma_free_named_accessor_property (ecma_property_t *prop_p);
extern void ecma_free_property (ecma_property_t *prop_p);

extern void ecma_delete_property (ecma_object_t *obj_p, ecma_property_t *prop_p);

extern bool ecma_is_property_enumerable (ecma_property_t* prop_p);
extern bool ecma_is_property_configurable (ecma_property_t* prop_p);

extern ecma_property_descriptor_t ecma_make_empty_property_descriptor (void);
extern void ecma_free_property_descriptor (ecma_property_descriptor_t *prop_desc_p);

/* ecma-helpers-conversion.c */
extern ecma_number_t ecma_zt_string_to_number (const ecma_char_t *str_p);
extern ssize_t ecma_uint32_to_string (uint32_t value, ecma_char_t *out_buffer_p, ssize_t buffer_size);
extern uint32_t ecma_number_to_uint32 (ecma_number_t value);
extern int32_t ecma_number_to_int32 (ecma_number_t value);
extern ecma_number_t ecma_int32_to_number (int32_t value);
extern ecma_number_t ecma_uint32_to_number (uint32_t value);
extern ecma_length_t ecma_number_to_zt_string (ecma_number_t num, ecma_char_t *buffer_p, ssize_t buffer_size);

#endif /* !JERRY_ECMA_HELPERS_H */

/**
 * @}
 * @}
 */

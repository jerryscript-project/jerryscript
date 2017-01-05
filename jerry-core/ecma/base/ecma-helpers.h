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

#ifndef ECMA_HELPERS_H
#define ECMA_HELPERS_H

#include "ecma-globals.h"
#include "jmem-allocator.h"
#include "lit-strings.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

/**
 * Get value of pointer from specified non-null compressed pointer.
 */
#define ECMA_GET_NON_NULL_POINTER(type, field) JMEM_CP_GET_NON_NULL_POINTER (type, field)

/**
 * Get value of pointer from specified compressed pointer.
 */
#define ECMA_GET_POINTER(type, field) JMEM_CP_GET_POINTER (type, field)

/**
 * Set value of non-null compressed pointer so that it will correspond
 * to specified non_compressed_pointer.
 */
#define ECMA_SET_NON_NULL_POINTER(field, non_compressed_pointer) JMEM_CP_SET_NON_NULL_POINTER (field, \
                                                                                               non_compressed_pointer)

/**
 * Set value of compressed pointer so that it will correspond
 * to specified non_compressed_pointer.
 */
#define ECMA_SET_POINTER(field, non_compressed_pointer) JMEM_CP_SET_POINTER (field, non_compressed_pointer)

/**
 * Convert ecma-string's contents to a cesu-8 string and put it into a buffer.
 */
#define ECMA_STRING_TO_UTF8_STRING(ecma_str_ptr, /**< ecma string pointer */ \
                                   utf8_ptr, /**< [out] output buffer pointer */ \
                                   utf8_str_size) /**< [out] output buffer size */ \
  lit_utf8_size_t utf8_str_size; \
  bool utf8_ptr ## must_be_freed; \
  const lit_utf8_byte_t *utf8_ptr = ecma_string_raw_chars (ecma_str_ptr, &utf8_str_size, &utf8_ptr ## must_be_freed); \
  utf8_ptr ## must_be_freed = false; /* it was used as 'is_ascii' in  'ecma_string_raw_chars', so we must reset it */ \
  \
  if (utf8_ptr == NULL) \
  { \
    utf8_ptr = (const lit_utf8_byte_t *) jmem_heap_alloc_block (utf8_str_size); \
    ecma_string_to_utf8_bytes (ecma_str_ptr, (lit_utf8_byte_t *) utf8_ptr, utf8_str_size); \
    utf8_ptr ## must_be_freed = true; \
  }

#ifdef ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY

/**
 * Set an internal property value of pointer
 */
#define ECMA_SET_INTERNAL_VALUE_POINTER(field, pointer) \
  (field) = ((ecma_value_t) pointer)

/**
 * Get an internal property value of pointer
 */
#define ECMA_GET_INTERNAL_VALUE_POINTER(type, field) \
  ((type *) field)

#else /* !ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

/**
 * Set an internal property value of non-null pointer so that it will correspond
 * to specified non_compressed_pointer.
 */
#define ECMA_SET_INTERNAL_VALUE_POINTER(field, non_compressed_pointer) \
  ECMA_SET_NON_NULL_POINTER (field, non_compressed_pointer)

/**
 * Get an internal property value of pointer from specified compressed pointer.
 */
#define ECMA_GET_INTERNAL_VALUE_POINTER(type, field) \
  ECMA_GET_POINTER (type, field)

#endif /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

/**
 * Free the cesu-8 string buffer allocated by 'ECMA_STRING_TO_UTF8_STRING'
 */
#define ECMA_FINALIZE_UTF8_STRING(utf8_ptr, /**< pointer to character buffer */ \
                                  utf8_str_size) /**< buffer size */ \
  if (utf8_ptr ## must_be_freed) \
  { \
    JERRY_ASSERT (utf8_ptr != NULL); \
    jmem_heap_free_block ((void *) utf8_ptr, utf8_str_size); \
  }

/* ecma-helpers-value.c */
bool ecma_is_value_direct (ecma_value_t value) __attr_pure___;
bool ecma_is_value_simple (ecma_value_t value) __attr_pure___;
bool ecma_is_value_empty (ecma_value_t value) __attr_pure___;
bool ecma_is_value_undefined (ecma_value_t value) __attr_pure___;
bool ecma_is_value_null (ecma_value_t value) __attr_pure___;
bool ecma_is_value_boolean (ecma_value_t value) __attr_pure___;
bool ecma_is_value_true (ecma_value_t value) __attr_pure___;
bool ecma_is_value_false (ecma_value_t value) __attr_pure___;
bool ecma_is_value_found (ecma_value_t value) __attr_pure___;
bool ecma_is_value_array_hole (ecma_value_t value) __attr_pure___;

bool ecma_is_value_integer_number (ecma_value_t value) __attr_pure___;
bool ecma_are_values_integer_numbers (ecma_value_t first_value, ecma_value_t second_value) __attr_pure___;
bool ecma_is_value_float_number (ecma_value_t value) __attr_pure___;
bool ecma_is_value_number (ecma_value_t value) __attr_pure___;
bool ecma_is_value_string (ecma_value_t value) __attr_pure___;
bool ecma_is_value_object (ecma_value_t value) __attr_pure___;

void ecma_check_value_type_is_spec_defined (ecma_value_t value);

ecma_value_t ecma_make_simple_value (const ecma_simple_value_t value) __attr_const___;
ecma_value_t ecma_make_boolean_value (bool boolean_value) __attr_const___;
ecma_value_t ecma_make_integer_value (ecma_integer_value_t integer_value) __attr_const___;
ecma_value_t ecma_make_nan_value (void);
ecma_value_t ecma_make_number_value (ecma_number_t ecma_number);
ecma_value_t ecma_make_int32_value (int32_t int32_number);
ecma_value_t ecma_make_uint32_value (uint32_t uint32_number);
ecma_value_t ecma_make_string_value (const ecma_string_t *ecma_string_p);
ecma_value_t ecma_make_object_value (const ecma_object_t *object_p);
ecma_value_t ecma_make_error_value (ecma_value_t value);
ecma_value_t ecma_make_error_obj_value (const ecma_object_t *object_p);
ecma_integer_value_t ecma_get_integer_from_value (ecma_value_t value) __attr_pure___;
ecma_number_t ecma_get_float_from_value (ecma_value_t value) __attr_pure___;
ecma_number_t ecma_get_number_from_value (ecma_value_t value) __attr_pure___;
uint32_t ecma_get_uint32_from_value (ecma_value_t value) __attr_pure___;
ecma_string_t *ecma_get_string_from_value (ecma_value_t value) __attr_pure___;
ecma_object_t *ecma_get_object_from_value (ecma_value_t value) __attr_pure___;
ecma_value_t ecma_get_value_from_error_value (ecma_value_t value) __attr_pure___;
ecma_value_t ecma_invert_boolean_value (ecma_value_t value) __attr_pure___;
ecma_value_t ecma_copy_value (ecma_value_t value);
ecma_value_t ecma_fast_copy_value (ecma_value_t value);
ecma_value_t ecma_copy_value_if_not_object (ecma_value_t value);
ecma_value_t ecma_update_float_number (ecma_value_t float_value, ecma_number_t new_number);
void ecma_value_assign_value (ecma_value_t *value_p, ecma_value_t ecma_value);
void ecma_value_assign_number (ecma_value_t *value_p, ecma_number_t ecma_number);
void ecma_value_assign_uint32 (ecma_value_t *value_p, uint32_t uint32_number);
void ecma_free_value (ecma_value_t value);
void ecma_fast_free_value (ecma_value_t value);
void ecma_free_value_if_not_object (ecma_value_t value);

/* ecma-helpers-string.c */
ecma_string_t *ecma_new_ecma_string_from_utf8 (const lit_utf8_byte_t *string_p, lit_utf8_size_t string_size);
ecma_string_t *ecma_new_ecma_string_from_utf8_converted_to_cesu8 (const lit_utf8_byte_t *string_p,
                                                                  lit_utf8_size_t string_size);
ecma_string_t *ecma_new_ecma_string_from_code_unit (ecma_char_t code_unit);
ecma_string_t *ecma_new_ecma_string_from_uint32 (uint32_t uint32_number);
ecma_string_t *ecma_new_ecma_string_from_number (ecma_number_t num);
ecma_string_t *ecma_new_ecma_string_from_magic_string_id (lit_magic_string_id_t id);
ecma_string_t *ecma_new_ecma_string_from_magic_string_ex_id (lit_magic_string_ex_id_t id);
ecma_string_t *ecma_new_ecma_length_string ();
ecma_string_t *ecma_concat_ecma_strings (ecma_string_t *string1_p, ecma_string_t *string2_p);
void ecma_ref_ecma_string (ecma_string_t *string_p);
void ecma_deref_ecma_string (ecma_string_t *string_p);
ecma_number_t ecma_string_to_number (const ecma_string_t *str_p);
uint32_t ecma_string_get_array_index (const ecma_string_t *str_p);

lit_utf8_size_t __attr_return_value_should_be_checked___
ecma_string_copy_to_cesu8_buffer (const ecma_string_t *string_desc_p,
                                  lit_utf8_byte_t *buffer_p,
                                  lit_utf8_size_t buffer_size);
lit_utf8_size_t __attr_return_value_should_be_checked___
ecma_string_copy_to_utf8_buffer (const ecma_string_t *string_desc_p,
                                 lit_utf8_byte_t *buffer_p,
                                 lit_utf8_size_t buffer_size);
void ecma_string_to_utf8_bytes (const ecma_string_t *string_desc_p, lit_utf8_byte_t *buffer_p,
                                lit_utf8_size_t buffer_size);
const lit_utf8_byte_t *ecma_string_raw_chars (const ecma_string_t *string_p, lit_utf8_size_t *size_p, bool *is_ascii_p);
void ecma_init_ecma_string_from_uint32 (ecma_string_t *string_desc_p, uint32_t uint32_number);
void ecma_init_ecma_length_string (ecma_string_t *string_desc_p);
bool ecma_string_is_empty (const ecma_string_t *str_p);
bool ecma_string_is_length (const ecma_string_t *string_p);

jmem_cpointer_t ecma_string_to_property_name (ecma_string_t *prop_name_p, ecma_property_t *name_type_p);
ecma_string_t *ecma_string_from_property_name (ecma_property_t property, jmem_cpointer_t prop_name_cp);
lit_string_hash_t ecma_string_get_property_name_hash (ecma_property_t property, jmem_cpointer_t prop_name_cp);
uint32_t ecma_string_get_property_index (ecma_property_t property, jmem_cpointer_t prop_name_cp);
bool ecma_string_compare_to_property_name (ecma_property_t property, jmem_cpointer_t prop_name_cp,
                                           const ecma_string_t *string_p);

bool ecma_compare_ecma_strings (const ecma_string_t *string1_p, const ecma_string_t *string2_p);
bool ecma_compare_ecma_strings_relational (const ecma_string_t *string1_p, const ecma_string_t *string2_p);
ecma_length_t ecma_string_get_length (const ecma_string_t *string_p);
ecma_length_t ecma_string_get_utf8_length (const ecma_string_t *string_p);
lit_utf8_size_t ecma_string_get_size (const ecma_string_t *string_p);
lit_utf8_size_t ecma_string_get_utf8_size (const ecma_string_t *string_p);
ecma_char_t ecma_string_get_char_at_pos (const ecma_string_t *string_p, ecma_length_t index);

ecma_string_t *ecma_get_magic_string (lit_magic_string_id_t id);
ecma_string_t *ecma_get_magic_string_ex (lit_magic_string_ex_id_t id);
lit_magic_string_id_t ecma_get_string_magic (const ecma_string_t *string_p);

lit_string_hash_t ecma_string_hash (const ecma_string_t *string_p);
ecma_string_t *ecma_string_substr (const ecma_string_t *string_p, ecma_length_t start_pos, ecma_length_t end_pos);
ecma_string_t *ecma_string_trim (const ecma_string_t *string_p);

/* ecma-helpers-number.c */
ecma_number_t ecma_number_make_nan (void);
ecma_number_t ecma_number_make_infinity (bool sign);
bool ecma_number_is_nan (ecma_number_t num);
bool ecma_number_is_negative (ecma_number_t num);
bool ecma_number_is_zero (ecma_number_t num);
bool ecma_number_is_infinity (ecma_number_t num);
int32_t
ecma_number_get_fraction_and_exponent (ecma_number_t num, uint64_t *out_fraction_p, int32_t *out_exponent_p);
ecma_number_t
ecma_number_make_normal_positive_from_fraction_and_exponent (uint64_t fraction, int32_t exponent);
ecma_number_t
ecma_number_make_from_sign_mantissa_and_exponent (bool sign, uint64_t mantissa, int32_t exponent);
ecma_number_t ecma_number_get_prev (ecma_number_t num);
ecma_number_t ecma_number_get_next (ecma_number_t num);
ecma_number_t ecma_number_negate (ecma_number_t num);
ecma_number_t ecma_number_trunc (ecma_number_t num);
ecma_number_t ecma_number_calc_remainder (ecma_number_t left_num, ecma_number_t right_num);
ecma_number_t ecma_number_add (ecma_number_t left_num, ecma_number_t right_num);
ecma_number_t ecma_number_substract (ecma_number_t left_num, ecma_number_t right_num);
ecma_number_t ecma_number_multiply (ecma_number_t left_num, ecma_number_t right_num);
ecma_number_t ecma_number_divide (ecma_number_t left_num, ecma_number_t right_num);
lit_utf8_size_t ecma_number_to_decimal (ecma_number_t num, lit_utf8_byte_t *out_digits_p, int32_t *out_decimal_exp_p);

/* ecma-helpers-values-collection.c */
ecma_collection_header_t *ecma_new_values_collection (const ecma_value_t values_buffer[], ecma_length_t values_number,
                                                      bool do_ref_if_object);
void ecma_free_values_collection (ecma_collection_header_t *header_p, bool do_deref_if_object);
void ecma_append_to_values_collection (ecma_collection_header_t *header_p, ecma_value_t v, bool do_ref_if_object);
void ecma_remove_last_value_from_values_collection (ecma_collection_header_t *header_p);
ecma_collection_header_t *ecma_new_strings_collection (ecma_string_t *string_ptrs_buffer[],
                                                       ecma_length_t strings_number);

/**
 * Context of ecma values' collection iterator
 */
typedef struct
{
  ecma_collection_header_t *header_p; /**< collection header */
  jmem_cpointer_t next_chunk_cp; /**< compressed pointer to next chunk */
  ecma_length_t current_index; /**< index of current element */
  const ecma_value_t *current_value_p; /**< pointer to current element */
  const ecma_value_t *current_chunk_beg_p; /**< pointer to beginning of current chunk's data */
  const ecma_value_t *current_chunk_end_p; /**< pointer to place right after the end of current chunk's data */
} ecma_collection_iterator_t;

void
ecma_collection_iterator_init (ecma_collection_iterator_t *iterator_p, ecma_collection_header_t *collection_p);
bool
ecma_collection_iterator_next (ecma_collection_iterator_t *iterator_p);

/* ecma-helpers.c */
ecma_object_t *ecma_create_object (ecma_object_t *prototype_object_p, size_t ext_object_size, ecma_object_type_t type);
ecma_object_t *ecma_create_decl_lex_env (ecma_object_t *outer_lexical_environment_p);
ecma_object_t *ecma_create_object_lex_env (ecma_object_t *outer_lexical_environment_p, ecma_object_t *binding_obj_p,
                                           bool provide_this);
bool ecma_is_lexical_environment (const ecma_object_t *object_p) __attr_pure___;
bool ecma_get_object_extensible (const ecma_object_t *object_p) __attr_pure___;
void ecma_set_object_extensible (ecma_object_t *object_p, bool is_extensible);
ecma_object_type_t ecma_get_object_type (const ecma_object_t *object_p) __attr_pure___;
void ecma_set_object_type (ecma_object_t *object_p, ecma_object_type_t type);
ecma_object_t *ecma_get_object_prototype (const ecma_object_t *object_p) __attr_pure___;
bool ecma_get_object_is_builtin (const ecma_object_t *object_p) __attr_pure___;
void ecma_set_object_is_builtin (ecma_object_t *object_p);
ecma_lexical_environment_type_t ecma_get_lex_env_type (const ecma_object_t *object_p) __attr_pure___;
ecma_object_t *ecma_get_lex_env_outer_reference (const ecma_object_t *object_p) __attr_pure___;
ecma_property_header_t *ecma_get_property_list (const ecma_object_t *object_p) __attr_pure___;
ecma_object_t *ecma_get_lex_env_binding_object (const ecma_object_t *object_p) __attr_pure___;
bool ecma_get_lex_env_provide_this (const ecma_object_t *object_p) __attr_pure___;

ecma_value_t *ecma_create_internal_property (ecma_object_t *object_p, ecma_internal_property_id_t property_id);
ecma_value_t *ecma_find_internal_property (ecma_object_t *object_p, ecma_internal_property_id_t property_id);
ecma_value_t *ecma_get_internal_property (ecma_object_t *object_p, ecma_internal_property_id_t property_id);

ecma_property_value_t *
ecma_create_named_data_property (ecma_object_t *object_p, ecma_string_t *name_p, uint8_t prop_attributes,
                                 ecma_property_t **out_prop_p);
ecma_property_value_t *
ecma_create_named_accessor_property (ecma_object_t *object_p, ecma_string_t *name_p, ecma_object_t *get_p,
                                     ecma_object_t *set_p, uint8_t prop_attributes, ecma_property_t **out_prop_p);
ecma_property_t *
ecma_find_named_property (ecma_object_t *obj_p, ecma_string_t *name_p);
ecma_property_value_t *
ecma_get_named_data_property (ecma_object_t *obj_p, ecma_string_t *name_p);

void ecma_free_property (ecma_object_t *object_p, jmem_cpointer_t name_cp, ecma_property_t *property_p);

void ecma_delete_property (ecma_object_t *object_p, ecma_property_value_t *prop_value_p);
uint32_t ecma_delete_array_properties (ecma_object_t *object_p, uint32_t new_length, uint32_t old_length);

void ecma_named_data_property_assign_value (ecma_object_t *obj_p, ecma_property_value_t *prop_value_p,
                                            ecma_value_t value);

ecma_object_t *ecma_get_named_accessor_property_getter (const ecma_property_value_t *prop_value_p);
ecma_object_t *ecma_get_named_accessor_property_setter (const ecma_property_value_t *prop_value_p);
void ecma_set_named_accessor_property_getter (ecma_object_t *object_p, ecma_property_value_t *prop_value_p,
                                              ecma_object_t *getter_p);
void ecma_set_named_accessor_property_setter (ecma_object_t *object_p, ecma_property_value_t *prop_value_p,
                                              ecma_object_t *setter_p);
bool ecma_is_property_writable (ecma_property_t property);
void ecma_set_property_writable_attr (ecma_property_t *property_p, bool is_writable);
bool ecma_is_property_enumerable (ecma_property_t property);
void ecma_set_property_enumerable_attr (ecma_property_t *property_p, bool is_enumerable);
bool ecma_is_property_configurable (ecma_property_t property);
void ecma_set_property_configurable_attr (ecma_property_t *property_p, bool is_configurable);

bool ecma_is_property_lcached (ecma_property_t *property_p);
void ecma_set_property_lcached (ecma_property_t *property_p, bool is_lcached);

ecma_property_descriptor_t ecma_make_empty_property_descriptor (void);
void ecma_free_property_descriptor (ecma_property_descriptor_t *prop_desc_p);

ecma_property_t *ecma_get_next_property_pair (ecma_property_pair_t *);

void ecma_bytecode_ref (ecma_compiled_code_t *bytecode_p);
void ecma_bytecode_deref (ecma_compiled_code_t *bytecode_p);

/* ecma-helpers-external-pointers.c */
bool
ecma_create_external_pointer_property (ecma_object_t *obj_p, ecma_internal_property_id_t id,
                                       ecma_external_pointer_t ptr_value);
bool
ecma_get_external_pointer_value (ecma_object_t *obj_p, ecma_internal_property_id_t id,
                                 ecma_external_pointer_t *out_pointer_p);
void
ecma_free_external_pointer_in_property (ecma_property_t *prop_p);

/* ecma-helpers-conversion.c */
ecma_number_t ecma_utf8_string_to_number (const lit_utf8_byte_t *str_p, lit_utf8_size_t str_size);
lit_utf8_size_t ecma_uint32_to_utf8_string (uint32_t value, lit_utf8_byte_t *out_buffer_p, lit_utf8_size_t buffer_size);
uint32_t ecma_number_to_uint32 (ecma_number_t num);
int32_t ecma_number_to_int32 (ecma_number_t num);
lit_utf8_size_t ecma_number_to_utf8_string (ecma_number_t num, lit_utf8_byte_t *buffer_p, lit_utf8_size_t buffer_size);

/* ecma-helpers-errol.c */
lit_utf8_size_t ecma_errol0_dtoa (double val, lit_utf8_byte_t *buffer_p, int32_t *exp_p);

/**
 * @}
 * @}
 */

#endif /* !ECMA_HELPERS_H */

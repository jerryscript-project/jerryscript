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
#include "jmem.h"
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
 * Status flags for ecma_string_get_chars function
 */
typedef enum
{
  ECMA_STRING_FLAG_EMPTY = 0,                /**< No options are provided. */
  ECMA_STRING_FLAG_IS_ASCII = (1 << 0),      /**< The string contains only ASCII characters. */
  ECMA_STRING_FLAG_REHASH_NEEDED = (1 << 1), /**< The hash of the string must be recalculated.
                                              *   For more details see ecma_append_chars_to_string */
  ECMA_STRING_FLAG_IS_UINT32 = (1 << 2),     /**< The string repesents an UINT32 number */
  ECMA_STRING_FLAG_MUST_BE_FREED = (1 << 3), /**< The returned buffer must be freed */
} ecma_string_flag_t;

/**
 * Convert ecma-string's contents to a cesu-8 string and put it into a buffer.
 */
#define ECMA_STRING_TO_UTF8_STRING(ecma_str_ptr, /**< ecma string pointer */ \
                                   utf8_ptr, /**< [out] output buffer pointer */ \
                                   utf8_str_size) /**< [out] output buffer size */ \
  lit_utf8_size_t utf8_str_size; \
  uint8_t utf8_ptr ## flags = ECMA_STRING_FLAG_EMPTY; \
  const lit_utf8_byte_t *utf8_ptr = ecma_string_get_chars (ecma_str_ptr, \
                                                           &utf8_str_size, \
                                                           NULL, \
                                                           NULL, \
                                                           &utf8_ptr ## flags);

/**
 * Free the cesu-8 string buffer allocated by 'ECMA_STRING_TO_UTF8_STRING'
 */
#define ECMA_FINALIZE_UTF8_STRING(utf8_ptr, /**< pointer to character buffer */ \
                                  utf8_str_size) /**< buffer size */ \
  if (utf8_ptr ## flags & ECMA_STRING_FLAG_MUST_BE_FREED) \
  { \
    JERRY_ASSERT (utf8_ptr != NULL); \
    jmem_heap_free_block ((void *) utf8_ptr, utf8_str_size); \
  }

#ifdef ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY

/**
 * Set an internal property value from pointer.
 */
#define ECMA_SET_INTERNAL_VALUE_POINTER(field, pointer) \
  (field) = ((ecma_value_t) pointer)

/**
 * Set an internal property value from pointer. Pointer can be NULL.
 */
#define ECMA_SET_INTERNAL_VALUE_ANY_POINTER(field, pointer) \
  (field) = ((ecma_value_t) pointer)

/**
 * Convert an internal property value to pointer.
 */
#define ECMA_GET_INTERNAL_VALUE_POINTER(type, field) \
  ((type *) field)

/**
 * Convert an internal property value to pointer. Result can be NULL.
 */
#define ECMA_GET_INTERNAL_VALUE_ANY_POINTER(type, field) \
  ((type *) field)

#else /* !ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

/**
 * Set an internal property value from pointer.
 */
#define ECMA_SET_INTERNAL_VALUE_POINTER(field, pointer) \
  ECMA_SET_NON_NULL_POINTER (field, pointer)

/**
 * Set an internal property value from pointer. Pointer can be NULL.
 */
#define ECMA_SET_INTERNAL_VALUE_ANY_POINTER(field, pointer) \
  ECMA_SET_POINTER (field, pointer)

/**
 * Convert an internal property value to pointer.
 */
#define ECMA_GET_INTERNAL_VALUE_POINTER(type, field) \
  ECMA_GET_NON_NULL_POINTER (type, field)

/**
 * Convert an internal property value to pointer. Result can be NULL.
 */
#define ECMA_GET_INTERNAL_VALUE_ANY_POINTER(type, field) \
  ECMA_GET_POINTER (type, field)

#endif /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

/**
 * Convert boolean to bitfield value.
 */
#define ECMA_BOOL_TO_BITFIELD(x) ((x) ? 1 : 0)

#if ENABLED (JERRY_ES2015)
/**
 * JERRY_ASSERT compatible macro for checking whether the given ecma-value is symbol
 */
#define ECMA_ASSERT_VALUE_IS_SYMBOL(value) (ecma_is_value_symbol ((value)))
#else /* !ENABLED (JERRY_ES2015) */
/**
 * JERRY_ASSERT compatible macro for checking whether the given ecma-value is symbol
 */
#define ECMA_ASSERT_VALUE_IS_SYMBOL(value) (false)
#endif /* ENABLED (JERRY_ES2015) */

/* ecma-helpers-value.c */
bool JERRY_ATTR_CONST ecma_is_value_direct (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_simple (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_empty (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_undefined (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_null (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_boolean (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_true (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_false (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_found (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_array_hole (ecma_value_t value);

bool JERRY_ATTR_CONST ecma_is_value_integer_number (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_are_values_integer_numbers (ecma_value_t first_value, ecma_value_t second_value);
bool JERRY_ATTR_CONST ecma_is_value_float_number (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_number (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_string (ecma_value_t value);
#if ENABLED (JERRY_ES2015)
bool JERRY_ATTR_CONST ecma_is_value_symbol (ecma_value_t value);
#endif /* ENABLED (JERRY_ES2015) */
bool JERRY_ATTR_CONST ecma_is_value_prop_name (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_direct_string (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_non_direct_string (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_object (ecma_value_t value);
bool JERRY_ATTR_CONST ecma_is_value_error_reference (ecma_value_t value);

void ecma_check_value_type_is_spec_defined (ecma_value_t value);

ecma_value_t JERRY_ATTR_CONST ecma_make_boolean_value (bool boolean_value);
ecma_value_t JERRY_ATTR_CONST ecma_make_integer_value (ecma_integer_value_t integer_value);
ecma_value_t ecma_make_nan_value (void);
ecma_value_t ecma_make_float_value (ecma_number_t *ecma_num_p);
ecma_value_t ecma_make_number_value (ecma_number_t ecma_number);
ecma_value_t ecma_make_int32_value (int32_t int32_number);
ecma_value_t ecma_make_uint32_value (uint32_t uint32_number);
ecma_value_t JERRY_ATTR_PURE ecma_make_string_value (const ecma_string_t *ecma_string_p);
#if ENABLED (JERRY_ES2015)
ecma_value_t JERRY_ATTR_PURE ecma_make_symbol_value (const ecma_string_t *ecma_symbol_p);
#endif /* ENABLED (JERRY_ES2015) */
ecma_value_t JERRY_ATTR_PURE ecma_make_prop_name_value (const ecma_string_t *ecma_prop_name_p);
ecma_value_t JERRY_ATTR_PURE ecma_make_magic_string_value (lit_magic_string_id_t id);
ecma_value_t JERRY_ATTR_PURE ecma_make_object_value (const ecma_object_t *object_p);
ecma_value_t JERRY_ATTR_PURE ecma_make_error_reference_value (const ecma_error_reference_t *error_ref_p);
ecma_integer_value_t JERRY_ATTR_CONST ecma_get_integer_from_value (ecma_value_t value);
ecma_number_t JERRY_ATTR_PURE ecma_get_float_from_value (ecma_value_t value);
ecma_number_t * ecma_get_pointer_from_float_value (ecma_value_t value);
ecma_number_t JERRY_ATTR_PURE ecma_get_number_from_value (ecma_value_t value);
ecma_string_t JERRY_ATTR_PURE *ecma_get_string_from_value (ecma_value_t value);
#if ENABLED (JERRY_ES2015)
ecma_string_t JERRY_ATTR_PURE *ecma_get_symbol_from_value (ecma_value_t value);
#endif /* ENABLED (JERRY_ES2015) */
ecma_string_t JERRY_ATTR_PURE *ecma_get_prop_name_from_value (ecma_value_t value);
ecma_object_t JERRY_ATTR_PURE *ecma_get_object_from_value (ecma_value_t value);
ecma_error_reference_t JERRY_ATTR_PURE *ecma_get_error_reference_from_value (ecma_value_t value);
ecma_value_t JERRY_ATTR_CONST ecma_invert_boolean_value (ecma_value_t value);
ecma_value_t ecma_copy_value (ecma_value_t value);
ecma_value_t ecma_fast_copy_value (ecma_value_t value);
ecma_value_t ecma_copy_value_if_not_object (ecma_value_t value);
ecma_value_t ecma_update_float_number (ecma_value_t float_value, ecma_number_t new_number);
void ecma_value_assign_value (ecma_value_t *value_p, ecma_value_t ecma_value);
void ecma_value_assign_number (ecma_value_t *value_p, ecma_number_t ecma_number);
void ecma_free_value (ecma_value_t value);
void ecma_fast_free_value (ecma_value_t value);
void ecma_free_value_if_not_object (ecma_value_t value);
void ecma_free_number (ecma_value_t value);
lit_magic_string_id_t ecma_get_typeof_lit_id (ecma_value_t value);

/* ecma-helpers-string.c */
#if ENABLED (JERRY_ES2015)
ecma_string_t *ecma_new_symbol_from_descriptor_string (ecma_value_t string_desc);
bool ecma_prop_name_is_symbol (ecma_string_t *string_p);
#endif /* ENABLED (JERRY_ES2015) */
#if ENABLED (JERRY_ES2015_BUILTIN_MAP) || ENABLED (JERRY_ES2015_BUILTIN_SET)
ecma_string_t *ecma_new_map_key_string (ecma_value_t value);
bool ecma_prop_name_is_map_key (ecma_string_t *string_p);
#endif /* ENABLED (JERRY_ES2015_BUILTIN_MAP) || ENABLED (JERRY_ES2015_BUILTIN_SET) */
ecma_string_t *ecma_new_ecma_string_from_utf8 (const lit_utf8_byte_t *string_p, lit_utf8_size_t string_size);
ecma_string_t *ecma_new_ecma_string_from_utf8_converted_to_cesu8 (const lit_utf8_byte_t *string_p,
                                                                  lit_utf8_size_t string_size);
ecma_string_t *ecma_new_ecma_string_from_code_unit (ecma_char_t code_unit);
#if ENABLED (JERRY_ES2015_BUILTIN_ITERATOR)
ecma_string_t *ecma_new_ecma_string_from_code_units (ecma_char_t first_code_unit, ecma_char_t second_code_unit);
#endif /* ENABLED (JERRY_ES2015_BUILTIN_ITERATOR) */
ecma_string_t *ecma_new_ecma_string_from_uint32 (uint32_t uint32_number);
ecma_string_t *ecma_new_non_direct_string_from_uint32 (uint32_t uint32_number);
ecma_string_t *ecma_get_ecma_string_from_uint32 (uint32_t uint32_number);
ecma_string_t *ecma_new_ecma_string_from_number (ecma_number_t num);
ecma_string_t *ecma_get_magic_string (lit_magic_string_id_t id);
ecma_string_t *ecma_append_chars_to_string (ecma_string_t *string1_p,
                                            const lit_utf8_byte_t *cesu8_string2_p,
                                            lit_utf8_size_t cesu8_string2_size,
                                            lit_utf8_size_t cesu8_string2_length);
ecma_string_t *ecma_concat_ecma_strings (ecma_string_t *string1_p, ecma_string_t *string2_p);
ecma_string_t *ecma_append_magic_string_to_string (ecma_string_t *string1_p, lit_magic_string_id_t string2_id);
void ecma_ref_ecma_string (ecma_string_t *string_p);
void ecma_deref_ecma_string (ecma_string_t *string_p);
void ecma_destroy_ecma_string (ecma_string_t *string_p);
ecma_number_t ecma_string_to_number (const ecma_string_t *str_p);
uint32_t ecma_string_get_array_index (const ecma_string_t *str_p);

lit_utf8_size_t JERRY_ATTR_WARN_UNUSED_RESULT
ecma_string_copy_to_cesu8_buffer (const ecma_string_t *string_desc_p,
                                  lit_utf8_byte_t *buffer_p,
                                  lit_utf8_size_t buffer_size);
lit_utf8_size_t JERRY_ATTR_WARN_UNUSED_RESULT
ecma_string_copy_to_utf8_buffer (const ecma_string_t *string_desc_p,
                                 lit_utf8_byte_t *buffer_p,
                                 lit_utf8_size_t buffer_size);
lit_utf8_size_t
ecma_substring_copy_to_cesu8_buffer (const ecma_string_t *string_desc_p,
                                     ecma_length_t start_pos,
                                     ecma_length_t end_pos,
                                     lit_utf8_byte_t *buffer_p,
                                     lit_utf8_size_t buffer_size);
lit_utf8_size_t
ecma_substring_copy_to_utf8_buffer (const ecma_string_t *string_desc_p,
                                    ecma_length_t start_pos,
                                    ecma_length_t end_pos,
                                    lit_utf8_byte_t *buffer_p,
                                    lit_utf8_size_t buffer_size);
void ecma_string_to_utf8_bytes (const ecma_string_t *string_desc_p, lit_utf8_byte_t *buffer_p,
                                lit_utf8_size_t buffer_size);
const lit_utf8_byte_t *ecma_string_get_chars (const ecma_string_t *string_p,
                                              lit_utf8_size_t *size_p,
                                              lit_utf8_size_t *length_p,
                                              lit_utf8_byte_t *uint32_buff_p,
                                              uint8_t *flags_p);
bool ecma_compare_ecma_string_to_magic_id (const ecma_string_t *string_p, lit_magic_string_id_t id);
bool ecma_string_is_empty (const ecma_string_t *string_p);
bool ecma_string_is_length (const ecma_string_t *string_p);

jmem_cpointer_t ecma_string_to_property_name (ecma_string_t *prop_name_p, ecma_property_t *name_type_p);
ecma_string_t *ecma_string_from_property_name (ecma_property_t property, jmem_cpointer_t prop_name_cp);
lit_string_hash_t ecma_string_get_property_name_hash (ecma_property_t property, jmem_cpointer_t prop_name_cp);
uint32_t ecma_string_get_property_index (ecma_property_t property, jmem_cpointer_t prop_name_cp);
bool ecma_string_compare_to_property_name (ecma_property_t property, jmem_cpointer_t prop_name_cp,
                                           const ecma_string_t *string_p);

bool ecma_compare_ecma_strings (const ecma_string_t *string1_p, const ecma_string_t *string2_p);
bool ecma_compare_ecma_non_direct_strings (const ecma_string_t *string1_p, const ecma_string_t *string2_p);
bool ecma_compare_ecma_strings_relational (const ecma_string_t *string1_p, const ecma_string_t *string2_p);
ecma_length_t ecma_string_get_length (const ecma_string_t *string_p);
ecma_length_t ecma_string_get_utf8_length (const ecma_string_t *string_p);
lit_utf8_size_t ecma_string_get_size (const ecma_string_t *string_p);
lit_utf8_size_t ecma_string_get_utf8_size (const ecma_string_t *string_p);
ecma_char_t ecma_string_get_char_at_pos (const ecma_string_t *string_p, ecma_length_t index);

lit_magic_string_id_t ecma_get_string_magic (const ecma_string_t *string_p);

lit_string_hash_t ecma_string_hash (const ecma_string_t *string_p);
ecma_string_t *ecma_string_substr (const ecma_string_t *string_p, ecma_length_t start_pos, ecma_length_t end_pos);
void ecma_string_trim_helper (const lit_utf8_byte_t **utf8_str_p,
                              lit_utf8_size_t *utf8_str_size);
ecma_string_t *ecma_string_trim (const ecma_string_t *string_p);

ecma_stringbuilder_t ecma_stringbuilder_create (void);
ecma_stringbuilder_t ecma_stringbuilder_create_from (ecma_string_t *string_p);
lit_utf8_size_t ecma_stringbuilder_get_size (ecma_stringbuilder_t *builder_p);
lit_utf8_byte_t *ecma_stringbuilder_get_data (ecma_stringbuilder_t *builder_p);
void ecma_stringbuilder_revert (ecma_stringbuilder_t *builder_p, const lit_utf8_size_t size);
void ecma_stringbuilder_append (ecma_stringbuilder_t *builder_p, const ecma_string_t *string_p);
void ecma_stringbuilder_append_magic (ecma_stringbuilder_t *builder_p, const lit_magic_string_id_t id);
void ecma_stringbuilder_append_raw (ecma_stringbuilder_t *builder_p,
                                    const lit_utf8_byte_t *data_p,
                                    const lit_utf8_size_t data_size);
void ecma_stringbuilder_append_char (ecma_stringbuilder_t *builder_p, const ecma_char_t c);
void ecma_stringbuilder_append_byte (ecma_stringbuilder_t *builder_p, const lit_utf8_byte_t);
ecma_string_t *ecma_stringbuilder_finalize (ecma_stringbuilder_t *builder_p);
void ecma_stringbuilder_destroy (ecma_stringbuilder_t *builder_p);

/* ecma-helpers-number.c */
ecma_number_t ecma_number_make_nan (void);
ecma_number_t ecma_number_make_infinity (bool sign);
bool ecma_number_is_nan (ecma_number_t num);
bool ecma_number_is_negative (ecma_number_t num);
bool ecma_number_is_zero (ecma_number_t num);
bool ecma_number_is_infinity (ecma_number_t num);
ecma_number_t
ecma_number_make_from_sign_mantissa_and_exponent (bool sign, uint64_t mantissa, int32_t exponent);
ecma_number_t ecma_number_get_prev (ecma_number_t num);
ecma_number_t ecma_number_get_next (ecma_number_t num);
ecma_number_t ecma_number_trunc (ecma_number_t num);
ecma_number_t ecma_number_calc_remainder (ecma_number_t left_num, ecma_number_t right_num);
ecma_value_t ecma_integer_multiply (ecma_integer_value_t left_integer, ecma_integer_value_t right_integer);
lit_utf8_size_t ecma_number_to_decimal (ecma_number_t num, lit_utf8_byte_t *out_digits_p, int32_t *out_decimal_exp_p);
lit_utf8_size_t ecma_number_to_binary_floating_point_number (ecma_number_t num,
                                                             lit_utf8_byte_t *out_digits_p,
                                                             int32_t *out_decimal_exp_p);

/* ecma-helpers-collection.c */
ecma_collection_t *ecma_new_collection (void);
void ecma_collection_push_back (ecma_collection_t *collection_p, ecma_value_t value);
void ecma_collection_destroy (ecma_collection_t *collection_p);
void ecma_collection_free (ecma_collection_t *collection_p);
void ecma_collection_free_if_not_object (ecma_collection_t *collection_p);
void ecma_collection_free_objects (ecma_collection_t *collection_p);

/* ecma-helpers.c */
ecma_object_t *ecma_create_object (ecma_object_t *prototype_object_p, size_t ext_object_size, ecma_object_type_t type);
ecma_object_t *ecma_create_decl_lex_env (ecma_object_t *outer_lexical_environment_p);
ecma_object_t *ecma_create_object_lex_env (ecma_object_t *outer_lexical_environment_p, ecma_object_t *binding_obj_p,
                                           ecma_lexical_environment_type_t type);
bool JERRY_ATTR_PURE ecma_is_lexical_environment (const ecma_object_t *object_p);
bool JERRY_ATTR_PURE ecma_get_object_extensible (const ecma_object_t *object_p);
void ecma_set_object_extensible (ecma_object_t *object_p, bool is_extensible);
ecma_object_type_t JERRY_ATTR_PURE ecma_get_object_type (const ecma_object_t *object_p);
bool JERRY_ATTR_PURE ecma_get_object_is_builtin (const ecma_object_t *object_p);
void ecma_set_object_is_builtin (ecma_object_t *object_p);
uint8_t ecma_get_object_builtin_id (ecma_object_t *object_p);
ecma_lexical_environment_type_t JERRY_ATTR_PURE ecma_get_lex_env_type (const ecma_object_t *object_p);
ecma_object_t JERRY_ATTR_PURE *ecma_get_lex_env_outer_reference (const ecma_object_t *object_p);
ecma_object_t JERRY_ATTR_PURE *ecma_get_lex_env_binding_object (const ecma_object_t *object_p);

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

void ecma_named_data_property_assign_value (ecma_object_t *obj_p, ecma_property_value_t *prop_value_p,
                                            ecma_value_t value);

ecma_getter_setter_pointers_t *
ecma_get_named_accessor_property (const ecma_property_value_t *prop_value_p);
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

#if ENABLED (JERRY_LCACHE)
bool ecma_is_property_lcached (ecma_property_t *property_p);
void ecma_set_property_lcached (ecma_property_t *property_p, bool is_lcached);
#endif /* ENABLED (JERRY_LCACHE) */

ecma_property_descriptor_t ecma_make_empty_property_descriptor (void);
void ecma_free_property_descriptor (ecma_property_descriptor_t *prop_desc_p);

ecma_value_t ecma_create_error_reference (ecma_value_t value, bool is_exception);
ecma_value_t ecma_create_error_reference_from_context (void);
ecma_value_t ecma_create_error_object_reference (ecma_object_t *object_p);
void ecma_ref_error_reference (ecma_error_reference_t *error_ref_p);
void ecma_deref_error_reference (ecma_error_reference_t *error_ref_p);
ecma_value_t ecma_clear_error_reference (ecma_value_t value, bool set_abort_flag);

void ecma_bytecode_ref (ecma_compiled_code_t *bytecode_p);
void ecma_bytecode_deref (ecma_compiled_code_t *bytecode_p);
#if (JERRY_STACK_LIMIT != 0)
uintptr_t ecma_get_current_stack_usage (void);
#endif /* (JERRY_STACK_LIMIT != 0) */

/* ecma-helpers-external-pointers.c */
bool ecma_create_native_pointer_property (ecma_object_t *obj_p, void *native_p, void *info_p);
ecma_native_pointer_t *ecma_get_native_pointer_value (ecma_object_t *obj_p, void *info_p);
bool ecma_delete_native_pointer_property (ecma_object_t *obj_p, void *info_p);

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

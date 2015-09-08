/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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
#include "lit-strings.h"
#include "mem-allocator.h"
#include "opcodes.h"

/**
 * Get value of pointer from specified non-null compressed pointer.
 */
#define ECMA_GET_NON_NULL_POINTER(type, field) MEM_CP_GET_NON_NULL_POINTER (type, field)

/**
 * Get value of pointer from specified compressed pointer.
 */
#define ECMA_GET_POINTER(type, field) MEM_CP_GET_POINTER (type, field)

/**
 * Set value of non-null compressed pointer so that it will correspond
 * to specified non_compressed_pointer.
 */
#define ECMA_SET_NON_NULL_POINTER(field, non_compressed_pointer) MEM_CP_SET_NON_NULL_POINTER (field, \
                                                                                              non_compressed_pointer)

/**
 * Set value of compressed pointer so that it will correspond
 * to specified non_compressed_pointer.
 */
#define ECMA_SET_POINTER(field, non_compressed_pointer) MEM_CP_SET_POINTER (field, non_compressed_pointer)

/* ecma-helpers-value.cpp */
extern bool ecma_is_value_empty (ecma_value_t);
extern bool ecma_is_value_undefined (ecma_value_t);
extern bool ecma_is_value_null (ecma_value_t);
extern bool ecma_is_value_boolean (ecma_value_t);
extern bool ecma_is_value_true (ecma_value_t);
extern bool ecma_is_value_array_hole (ecma_value_t);

extern bool ecma_is_value_number (ecma_value_t);
extern bool ecma_is_value_string (ecma_value_t);
extern bool ecma_is_value_object (ecma_value_t);

extern void ecma_check_value_type_is_spec_defined (ecma_value_t);

extern ecma_value_t ecma_make_simple_value (const ecma_simple_value_t value);
extern ecma_value_t ecma_make_number_value (const ecma_number_t *);
extern ecma_value_t ecma_make_string_value (const ecma_string_t *);
extern ecma_value_t ecma_make_object_value (const ecma_object_t *);
extern ecma_number_t *ecma_get_number_from_value (ecma_value_t) __attr_pure___;
extern ecma_string_t *ecma_get_string_from_value (ecma_value_t) __attr_pure___;
extern ecma_object_t *ecma_get_object_from_value (ecma_value_t) __attr_pure___;
extern ecma_value_t ecma_copy_value (ecma_value_t, bool);
extern void ecma_free_value (ecma_value_t, bool);

extern ecma_completion_value_t ecma_make_completion_value (ecma_completion_type_t, ecma_value_t);
extern ecma_completion_value_t ecma_make_simple_completion_value (ecma_simple_value_t);
extern ecma_completion_value_t ecma_make_normal_completion_value (ecma_value_t);
extern ecma_completion_value_t ecma_make_throw_completion_value (ecma_value_t);
extern ecma_completion_value_t ecma_make_throw_obj_completion_value (ecma_object_t *);
extern ecma_completion_value_t ecma_make_empty_completion_value (void);
extern ecma_completion_value_t ecma_make_return_completion_value (ecma_value_t);
extern ecma_completion_value_t ecma_make_meta_completion_value (void);
extern ecma_completion_value_t ecma_make_jump_completion_value (vm_instr_counter_t);
extern ecma_value_t ecma_get_completion_value_value (ecma_completion_value_t);
extern ecma_number_t *ecma_get_number_from_completion_value (ecma_completion_value_t) __attr_const___;
extern ecma_string_t *ecma_get_string_from_completion_value (ecma_completion_value_t) __attr_const___;
extern ecma_object_t *ecma_get_object_from_completion_value (ecma_completion_value_t) __attr_const___;
extern vm_instr_counter_t ecma_get_jump_target_from_completion_value (ecma_completion_value_t);
extern ecma_completion_value_t ecma_copy_completion_value (ecma_completion_value_t);
extern void ecma_free_completion_value (ecma_completion_value_t);

extern bool ecma_is_completion_value_normal (ecma_completion_value_t);
extern bool ecma_is_completion_value_throw (ecma_completion_value_t);
extern bool ecma_is_completion_value_return (ecma_completion_value_t);
extern bool ecma_is_completion_value_meta (ecma_completion_value_t);
extern bool ecma_is_completion_value_jump (ecma_completion_value_t);
extern bool ecma_is_completion_value_normal_simple_value (ecma_completion_value_t, ecma_simple_value_t);
extern bool ecma_is_completion_value_normal_true (ecma_completion_value_t);
extern bool ecma_is_completion_value_normal_false (ecma_completion_value_t);
extern bool ecma_is_completion_value_empty (ecma_completion_value_t);

/* ecma-helpers-string.c */
extern ecma_string_t *ecma_new_ecma_string_from_utf8 (const lit_utf8_byte_t *, lit_utf8_size_t);
extern ecma_string_t *ecma_new_ecma_string_from_code_unit (ecma_char_t);
extern ecma_string_t *ecma_new_ecma_string_from_uint32 (uint32_t);
extern ecma_string_t *ecma_new_ecma_string_from_number (ecma_number_t);
extern void ecma_new_ecma_string_on_stack_from_lit_cp (ecma_string_t *, lit_cpointer_t);
extern ecma_string_t *ecma_new_ecma_string_from_lit_cp (lit_cpointer_t);
extern void ecma_new_ecma_string_on_stack_from_magic_string_id (ecma_string_t *, lit_magic_string_id_t);
extern ecma_string_t *ecma_new_ecma_string_from_magic_string_id (lit_magic_string_id_t);
extern ecma_string_t *ecma_new_ecma_string_from_magic_string_ex_id (lit_magic_string_ex_id_t);
extern ecma_string_t *ecma_concat_ecma_strings (ecma_string_t *, ecma_string_t *);
extern ecma_string_t *ecma_copy_or_ref_ecma_string (ecma_string_t *);
extern void ecma_deref_ecma_string (ecma_string_t *);
extern void ecma_check_that_ecma_string_need_not_be_freed (const ecma_string_t *);
extern ecma_number_t ecma_string_to_number (const ecma_string_t *);
extern bool ecma_string_get_array_index (const ecma_string_t *, uint32_t *);

extern ssize_t __attr_return_value_should_be_checked___
ecma_string_to_utf8_string (const ecma_string_t *, lit_utf8_byte_t *, ssize_t);

extern bool ecma_compare_ecma_strings_equal_hashes (const ecma_string_t *, const ecma_string_t *);
extern bool ecma_compare_ecma_strings (const ecma_string_t *, const ecma_string_t *);
extern bool ecma_compare_ecma_strings_relational (const ecma_string_t *, const ecma_string_t *);
extern ecma_length_t ecma_string_get_length (const ecma_string_t *);
extern lit_utf8_size_t ecma_string_get_size (const ecma_string_t *);
extern ecma_char_t ecma_string_get_char_at_pos (const ecma_string_t *, ecma_length_t);
extern lit_utf8_byte_t ecma_string_get_byte_at_pos (const ecma_string_t *, lit_utf8_size_t);

extern ecma_string_t *ecma_get_magic_string (lit_magic_string_id_t);
extern ecma_string_t *ecma_get_magic_string_ex (lit_magic_string_ex_id_t);
extern bool ecma_is_string_magic (const ecma_string_t *, lit_magic_string_id_t *);
extern bool ecma_is_ex_string_magic (const ecma_string_t *, lit_magic_string_ex_id_t *);

extern lit_string_hash_t ecma_string_hash (const ecma_string_t *);
extern ecma_string_t *ecma_string_substr (const ecma_string_t *, ecma_length_t, ecma_length_t);
extern ecma_string_t *ecma_string_trim (const ecma_string_t *);

/* ecma-helpers-number.cpp */
extern const ecma_number_t ecma_number_relative_eps;

extern ecma_number_t ecma_number_make_nan (void);
extern ecma_number_t ecma_number_make_infinity (bool);
extern bool ecma_number_is_nan (ecma_number_t);
extern bool ecma_number_is_negative (ecma_number_t);
extern bool ecma_number_is_zero (ecma_number_t);
extern bool ecma_number_is_infinity (ecma_number_t);
extern int32_t
ecma_number_get_fraction_and_exponent (ecma_number_t, uint64_t *, int32_t *);
extern ecma_number_t
ecma_number_make_normal_positive_from_fraction_and_exponent (uint64_t, int32_t);
extern ecma_number_t
ecma_number_make_from_sign_mantissa_and_exponent (bool, uint64_t, int32_t);
extern ecma_number_t ecma_number_get_prev (ecma_number_t);
extern ecma_number_t ecma_number_get_next (ecma_number_t);
extern ecma_number_t ecma_number_negate (ecma_number_t);
extern ecma_number_t ecma_number_trunc (ecma_number_t);
extern ecma_number_t ecma_number_calc_remainder (ecma_number_t, ecma_number_t);
extern ecma_number_t ecma_number_add (ecma_number_t, ecma_number_t);
extern ecma_number_t ecma_number_substract (ecma_number_t, ecma_number_t);
extern ecma_number_t ecma_number_multiply (ecma_number_t, ecma_number_t);
extern ecma_number_t ecma_number_divide (ecma_number_t, ecma_number_t);
extern ecma_number_t ecma_number_sqrt (ecma_number_t);
extern ecma_number_t ecma_number_abs (ecma_number_t);
extern ecma_number_t ecma_number_ln (ecma_number_t);
extern ecma_number_t ecma_number_exp (ecma_number_t);
extern void ecma_number_to_decimal (ecma_number_t, uint64_t *, int32_t *, int32_t *);

/* ecma-helpers-values-collection.cpp */
extern ecma_collection_header_t *ecma_new_values_collection (const ecma_value_t[], ecma_length_t, bool);
extern void ecma_free_values_collection (ecma_collection_header_t *, bool);
extern void ecma_append_to_values_collection (ecma_collection_header_t *, ecma_value_t, bool);
extern void ecma_remove_last_value_from_values_collection (ecma_collection_header_t *, bool);
extern ecma_collection_header_t *ecma_new_strings_collection (ecma_string_t *[], ecma_length_t);

/**
 * Context of ecma-values' collection iterator
 */
typedef struct
{
  ecma_collection_header_t *header_p; /**< collection header */
  mem_cpointer_t next_chunk_cp; /**< compressed pointer to next chunk */
  ecma_length_t current_index; /**< index of current element */
  const ecma_value_t *current_value_p; /**< pointer to current element */
  const ecma_value_t *current_chunk_beg_p; /**< pointer to beginning of current chunk's data */
  const ecma_value_t *current_chunk_end_p; /**< pointer to place right after the end of current chunk's data */
} ecma_collection_iterator_t;

extern void
ecma_collection_iterator_init (ecma_collection_iterator_t *, ecma_collection_header_t *);
extern bool
ecma_collection_iterator_next (ecma_collection_iterator_t *);

/* ecma-helpers.cpp */
extern ecma_object_t *ecma_create_object (ecma_object_t *, bool, ecma_object_type_t);
extern ecma_object_t *ecma_create_decl_lex_env (ecma_object_t *);
extern ecma_object_t *ecma_create_object_lex_env (ecma_object_t *, ecma_object_t *, bool);
extern bool ecma_is_lexical_environment (const ecma_object_t *) __attr_pure___;
extern bool ecma_get_object_extensible (const ecma_object_t *) __attr_pure___;
extern void ecma_set_object_extensible (ecma_object_t *, bool);
extern ecma_object_type_t ecma_get_object_type (const ecma_object_t *) __attr_pure___;
extern void ecma_set_object_type (ecma_object_t *, ecma_object_type_t);
extern ecma_object_t *ecma_get_object_prototype (const ecma_object_t *) __attr_pure___;
extern bool ecma_get_object_is_builtin (const ecma_object_t *) __attr_pure___;
extern void ecma_set_object_is_builtin (ecma_object_t *, bool);
extern ecma_lexical_environment_type_t ecma_get_lex_env_type (const ecma_object_t *) __attr_pure___;
extern ecma_object_t *ecma_get_lex_env_outer_reference (const ecma_object_t *) __attr_pure___;
extern ecma_property_t *ecma_get_property_list (const ecma_object_t *) __attr_pure___;
extern ecma_object_t *ecma_get_lex_env_binding_object (const ecma_object_t *) __attr_pure___;
extern bool ecma_get_lex_env_provide_this (const ecma_object_t *) __attr_pure___;

extern ecma_property_t *ecma_create_internal_property (ecma_object_t *, ecma_internal_property_id_t);
extern ecma_property_t *ecma_find_internal_property (ecma_object_t *, ecma_internal_property_id_t);
extern ecma_property_t *ecma_get_internal_property (ecma_object_t *, ecma_internal_property_id_t);

extern ecma_property_t *
ecma_create_named_data_property (ecma_object_t *, ecma_string_t *, bool, bool, bool);
extern ecma_property_t *
ecma_create_named_accessor_property (ecma_object_t *, ecma_string_t *, ecma_object_t *, ecma_object_t *, bool, bool);
extern ecma_property_t *
ecma_find_named_property (ecma_object_t *, ecma_string_t *);
extern ecma_property_t *
ecma_get_named_property (ecma_object_t *, ecma_string_t *);
extern ecma_property_t *
ecma_get_named_data_property (ecma_object_t *, ecma_string_t *);

extern void ecma_free_property (ecma_object_t *, ecma_property_t *);

extern void ecma_delete_property (ecma_object_t *, ecma_property_t *);

extern ecma_value_t ecma_get_named_data_property_value (const ecma_property_t *);
extern void ecma_set_named_data_property_value (ecma_property_t *, ecma_value_t);
extern void ecma_named_data_property_assign_value (ecma_object_t *, ecma_property_t *, ecma_value_t);

extern ecma_object_t *ecma_get_named_accessor_property_getter (const ecma_property_t *);
extern ecma_object_t *ecma_get_named_accessor_property_setter (const ecma_property_t *);
extern void ecma_set_named_accessor_property_getter (ecma_object_t *, ecma_property_t *, ecma_object_t *);
extern void ecma_set_named_accessor_property_setter (ecma_object_t *, ecma_property_t *, ecma_object_t *);
extern bool ecma_is_property_writable (ecma_property_t *);
extern void ecma_set_property_writable_attr (ecma_property_t *, bool);
extern bool ecma_is_property_enumerable (ecma_property_t *);
extern void ecma_set_property_enumerable_attr (ecma_property_t *, bool);
extern bool ecma_is_property_configurable (ecma_property_t *);
extern void ecma_set_property_configurable_attr (ecma_property_t *, bool);

extern bool ecma_is_property_lcached (ecma_property_t *);
extern void ecma_set_property_lcached (ecma_property_t *, bool);

extern ecma_property_descriptor_t ecma_make_empty_property_descriptor (void);
extern void ecma_free_property_descriptor (ecma_property_descriptor_t *);
extern ecma_property_descriptor_t ecma_get_property_descriptor_from_property (ecma_property_t *);

/* ecma-helpers-external-pointers.cpp */
extern bool
ecma_create_external_pointer_property (ecma_object_t *, ecma_internal_property_id_t, ecma_external_pointer_t);
extern bool
ecma_get_external_pointer_value (ecma_object_t *, ecma_internal_property_id_t, ecma_external_pointer_t *);
extern void
ecma_free_external_pointer_in_property (ecma_property_t *);

/* ecma-helpers-conversion.cpp */
extern ecma_number_t ecma_utf8_string_to_number (const lit_utf8_byte_t *, lit_utf8_size_t);
extern ssize_t ecma_uint32_to_utf8_string (uint32_t, lit_utf8_byte_t *, ssize_t);
extern uint32_t ecma_number_to_uint32 (ecma_number_t);
extern int32_t ecma_number_to_int32 (ecma_number_t);
extern ecma_number_t ecma_int32_to_number (int32_t);
extern ecma_number_t ecma_uint32_to_number (uint32_t);
extern lit_utf8_size_t ecma_number_to_utf8_string (ecma_number_t, lit_utf8_byte_t *, ssize_t);

#endif /* !JERRY_ECMA_HELPERS_H */

/**
 * @}
 * @}
 */

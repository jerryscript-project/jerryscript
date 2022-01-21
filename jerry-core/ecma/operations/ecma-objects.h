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

#ifndef ECMA_OBJECTS_H
#define ECMA_OBJECTS_H

#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaobjectsinternalops ECMA objects' operations
 * @{
 */

#if JERRY_ERROR_MESSAGES
/**
 * Reject with TypeError depending on 'is_throw' with the given format
 */
#define ECMA_REJECT_WITH_FORMAT(is_throw, msg, ...) \
  ((is_throw) ? ecma_raise_standard_error_with_format (JERRY_ERROR_TYPE, (msg), __VA_ARGS__) : ECMA_VALUE_FALSE)

/**
 * Reject with TypeError depending on 'is_throw' with the given message
 */
#define ECMA_REJECT(is_throw, msg) ((is_throw) ? ecma_raise_type_error (msg) : ECMA_VALUE_FALSE)
#else /* !JERRY_ERROR_MESSAGES */
/**
 * Reject with TypeError depending on is_throw flags wit the given format
 */
#define ECMA_REJECT_WITH_FORMAT(is_throw, msg, ...) ECMA_REJECT ((is_throw), (msg))

/**
 * Reject with TypeError depending on is_throw flags wit the given message
 */
#define ECMA_REJECT(is_throw, msg)                  ((is_throw) ? ecma_raise_type_error (ECMA_ERR_EMPTY) : ECMA_VALUE_FALSE)
#endif /* JERRY_ERROR_MESSAGES */

ecma_value_t ecma_raise_property_redefinition (ecma_string_t *property_name_p, uint32_t flags);
ecma_value_t ecma_raise_readonly_assignment (ecma_string_t *property_name_p, bool is_throw);
ecma_value_t ecma_raise_non_configurable_property (ecma_string_t *property_name_p, bool is_throw);

ecma_value_t ecma_op_object_find (ecma_object_t *obj_p, ecma_string_t *property_name_p, ecma_value_t base_value);
ecma_value_t ecma_op_object_find_by_index (ecma_object_t *object_p, ecma_length_t index);
ecma_value_t ecma_op_object_get_length (ecma_object_t *object_p, ecma_length_t *length_p);
ecma_value_t ecma_op_object_get_by_index (ecma_object_t *object_p, ecma_length_t index);
ecma_value_t ecma_op_object_get_by_magic_id (ecma_object_t *object_p, lit_magic_string_id_t property_id);
#if JERRY_ESNEXT
ecma_string_t *ecma_op_get_global_symbol (lit_magic_string_id_t property_id);
bool ecma_op_compare_string_to_global_symbol (ecma_string_t *string_p, lit_magic_string_id_t property_id);
ecma_value_t ecma_op_object_get_by_symbol_id (ecma_object_t *object_p, lit_magic_string_id_t property_id);
ecma_value_t ecma_op_get_method_by_symbol_id (ecma_value_t value, lit_magic_string_id_t symbol_id);
ecma_value_t ecma_op_get_method_by_magic_id (ecma_value_t value, lit_magic_string_id_t magic_id);
#endif /* JERRY_ESNEXT */
ecma_value_t
ecma_op_object_put_by_index (ecma_object_t *object_p, ecma_length_t index, ecma_value_t value, bool is_throw);
ecma_value_t ecma_op_object_delete_by_index (ecma_object_t *obj_p, ecma_length_t index, bool is_throw);
ecma_value_t ecma_op_object_default_value (ecma_object_t *obj_p, ecma_preferred_type_hint_t hint);
ecma_value_t ecma_op_object_has_instance (ecma_object_t *obj_p, ecma_value_t value);

ecma_value_t ecma_op_object_is_prototype_of (ecma_object_t *base_p, ecma_object_t *target_p);
ecma_collection_t *ecma_op_object_get_enumerable_property_names (ecma_object_t *obj_p,
                                                                 ecma_enumerable_property_names_options_t option);
ecma_collection_t *ecma_op_object_enumerate (ecma_object_t *obj_p);

lit_magic_string_id_t ecma_object_get_class_name (ecma_object_t *obj_p);
#if JERRY_BUILTIN_REGEXP
bool ecma_object_is_regexp_object (ecma_value_t arg);
#endif /* JERRY_BUILTIN_REGEXP */
#if JERRY_ESNEXT
ecma_value_t ecma_op_is_concat_spreadable (ecma_value_t arg);
ecma_value_t ecma_op_is_regexp (ecma_value_t arg);
ecma_value_t ecma_op_species_constructor (ecma_object_t *this_value, ecma_builtin_id_t default_constructor_id);
ecma_value_t ecma_op_invoke_by_symbol_id (ecma_value_t object,
                                          lit_magic_string_id_t magic_string_id,
                                          ecma_value_t *args_p,
                                          uint32_t args_len);
#endif /* JERRY_ESNEXT */
#if JERRY_BUILTIN_WEAKREF || JERRY_BUILTIN_CONTAINER
void ecma_op_object_set_weak (ecma_object_t *object_p, ecma_object_t *target_p);
void ecma_op_object_unref_weak (ecma_object_t *object_p, ecma_value_t ref_holder);
#endif /* JERRY_BUILTIN_WEAKREF || JERRY_BUILTIN_CONTAINER */
ecma_value_t ecma_op_object_has_own_property (ecma_object_t *object_p, ecma_string_t *property_name_p);
ecma_value_t
ecma_op_invoke (ecma_value_t object, ecma_string_t *property_name_p, ecma_value_t *args_p, uint32_t args_len);
ecma_value_t ecma_op_invoke_by_magic_id (ecma_value_t object,
                                         lit_magic_string_id_t magic_string_id,
                                         ecma_value_t *args_p,
                                         uint32_t args_len);

ecma_object_t *ecma_internal_method_get_prototype_of (ecma_object_t *obj_p);
ecma_value_t ecma_internal_method_set_prototype_of (ecma_object_t *obj_p, ecma_value_t proto);
ecma_value_t ecma_internal_method_is_extensible (ecma_object_t *obj_p);
ecma_value_t ecma_internal_method_prevent_extensions (ecma_object_t *obj_p);
ecma_property_descriptor_t ecma_internal_method_get_own_property (ecma_object_t *obj_p, ecma_string_t *property_name_p);
ecma_value_t ecma_internal_method_define_own_property (ecma_object_t *obj_p,
                                                       ecma_string_t *property_name_p,
                                                       const ecma_property_descriptor_t *property_desc_p);
ecma_value_t ecma_internal_method_has_property (ecma_object_t *obj_p, ecma_string_t *property_name_p);
ecma_value_t ecma_internal_method_get (ecma_object_t *obj_p, ecma_string_t *property_name_p, ecma_value_t receiver);
ecma_value_t ecma_internal_method_set (ecma_object_t *obj_p,
                                       ecma_string_t *property_name_p,
                                       ecma_value_t value,
                                       ecma_value_t receiver,
                                       bool is_throw);
ecma_value_t ecma_internal_method_delete (ecma_object_t *obj_p, ecma_string_t *property_name_p, bool is_strict);
ecma_collection_t *ecma_internal_method_own_property_keys (ecma_object_t *obj_p, jerry_property_filter_t filter);

ecma_value_t ecma_internal_method_call (ecma_object_t *obj_p,
                                        ecma_value_t this_value,
                                        const ecma_value_t *arguments_list_p,
                                        uint32_t arguments_list_len);

ecma_value_t ecma_internal_method_validated_call (ecma_value_t callee,
                                                  ecma_value_t this_value,
                                                  const ecma_value_t *arguments_list_p,
                                                  uint32_t arguments_list_len);

ecma_value_t ecma_internal_method_construct (ecma_object_t *obj_p,
                                             ecma_object_t *new_target_p,
                                             const ecma_value_t *arguments_list_p,
                                             uint32_t arguments_list_len);

void ecma_internal_method_list_lazy_property_keys (ecma_object_t *obj_p,
                                                   ecma_collection_t *prop_names_p,
                                                   ecma_property_counter_t *prop_counter_p,
                                                   jerry_property_filter_t filter);

void ecma_internal_method_delete_lazy_property (ecma_object_t *obj_p, ecma_string_t *property_name_p);

bool ecma_op_canonical_numeric_string (ecma_string_t *property_name_p);

/**
 * @}
 * @}
 */

#endif /* !ECMA_OBJECTS_H */

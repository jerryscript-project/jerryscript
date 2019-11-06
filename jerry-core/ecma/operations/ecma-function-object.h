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

#ifndef ECMA_FUNCTION_OBJECT_H
#define ECMA_FUNCTION_OBJECT_H

#include "ecma-globals.h"
#include "vm.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmafunctionobject ECMA Function object related routines
 * @{
 */

bool ecma_is_normal_or_arrow_function (ecma_object_type_t type);

#if ENABLED (JERRY_LINE_INFO) || ENABLED (JERRY_ES2015_MODULE_SYSTEM)
ecma_value_t ecma_op_resource_name (const ecma_compiled_code_t *bytecode_header_p);
#endif /* ENABLED (JERRY_LINE_INFO) || ENABLED (JERRY_ES2015_MODULE_SYSTEM) */

bool ecma_op_is_callable (ecma_value_t value);
bool ecma_op_object_is_callable (ecma_object_t *obj_p);
bool ecma_is_constructor (ecma_value_t value);
bool ecma_object_is_constructor (ecma_object_t *obj_p);

ecma_object_t *
ecma_op_create_function_object (ecma_object_t *scope_p, const ecma_compiled_code_t *bytecode_data_p);


ecma_object_t *
ecma_op_create_external_function_object (ecma_external_handler_t handler_cb);

const ecma_compiled_code_t *
ecma_op_function_get_compiled_code (ecma_extended_object_t *function_p);

#if ENABLED (JERRY_ES2015)
void
ecma_op_set_super_called (ecma_object_t *lex_env_p);

bool
ecma_op_is_super_called (ecma_object_t *lex_env_p);

void
ecma_op_set_class_this_binding (ecma_object_t *lex_env_p, ecma_value_t this_binding);

ecma_value_t
ecma_op_get_class_this_binding (ecma_object_t *lex_env_p);

ecma_value_t
ecma_op_function_implicit_constructor_handler_cb (const ecma_value_t function_obj,
                                                  const ecma_value_t this_val,
                                                  const ecma_value_t args_p[],
                                                  const ecma_length_t args_count);

void
ecma_op_set_class_prototype (ecma_value_t completion_value, ecma_value_t this_arg);

ecma_object_t *
ecma_op_create_arrow_function_object (ecma_object_t *scope_p, const ecma_compiled_code_t *bytecode_data_p,
                                      ecma_value_t this_binding);

const ecma_compiled_code_t *
ecma_op_arrow_function_get_compiled_code (ecma_arrow_function_t *arrow_function_p);
#endif /* ENABLED (JERRY_ES2015) */

ecma_value_t
ecma_op_function_has_instance (ecma_object_t *func_obj_p, ecma_value_t value);

ecma_value_t
ecma_op_function_call (ecma_object_t *func_obj_p, ecma_value_t this_arg_value,
                       const ecma_value_t *arguments_list_p, ecma_length_t arguments_list_len);

ecma_value_t
ecma_op_function_construct (ecma_object_t *func_obj_p, ecma_value_t this_arg_value,
                            const ecma_value_t *arguments_list_p, ecma_length_t arguments_list_len);

ecma_property_t *
ecma_op_function_try_to_lazy_instantiate_property (ecma_object_t *object_p, ecma_string_t *property_name_p);

ecma_property_t *
ecma_op_external_function_try_to_lazy_instantiate_property (ecma_object_t *object_p, ecma_string_t *property_name_p);

ecma_property_t *
ecma_op_bound_function_try_to_lazy_instantiate_property (ecma_object_t *object_p, ecma_string_t *property_name_p);

void
ecma_op_function_list_lazy_property_names (ecma_object_t *object_p,
                                           bool separate_enumerable,
                                           ecma_collection_t *main_collection_p,
                                           ecma_collection_t *non_enum_collection_p);

void
ecma_op_external_function_list_lazy_property_names (bool separate_enumerable,
                                                    ecma_collection_t *main_collection_p,
                                                    ecma_collection_t *non_enum_collection_p);

void
ecma_op_bound_function_list_lazy_property_names (bool separate_enumerable,
                                                 ecma_collection_t *main_collection_p,
                                                 ecma_collection_t *non_enum_collection_p);

/**
 * @}
 * @}
 */

#endif /* !ECMA_FUNCTION_OBJECT_H */

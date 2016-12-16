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

bool ecma_op_is_callable (ecma_value_t value);
bool ecma_is_constructor (ecma_value_t value);

ecma_object_t *
ecma_op_create_function_object (ecma_object_t *scope_p, bool is_decl_in_strict_mode,
                                const ecma_compiled_code_t *bytecode_data_p);

void
ecma_op_function_list_lazy_property_names (bool separate_enumerable,
                                           ecma_collection_header_t *main_collection_p,
                                           ecma_collection_header_t *non_enum_collection_p);

ecma_property_t *
ecma_op_function_try_lazy_instantiate_property (ecma_object_t *object_p, ecma_string_t *property_name_p);

ecma_object_t *
ecma_op_create_external_function_object (ecma_external_pointer_t code_p);

ecma_value_t
ecma_op_function_call (ecma_object_t *func_obj_p, ecma_value_t this_arg_value,
                       const ecma_value_t *arguments_list_p, ecma_length_t arguments_list_len);

ecma_value_t
ecma_op_function_construct (ecma_object_t *func_obj_p, const ecma_value_t *arguments_list_p,
                            ecma_length_t arguments_list_len);

ecma_value_t
ecma_op_function_has_instance (ecma_object_t *func_obj_p, ecma_value_t value);

/**
 * @}
 * @}
 */

#endif /* !ECMA_FUNCTION_OBJECT_H */

/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

extern bool ecma_op_is_callable (ecma_value_t value);
extern bool ecma_is_constructor (ecma_value_t value);

extern ecma_object_t*
ecma_op_create_function_object (ecma_string_t* formal_parameter_list_p[],
                                ecma_length_t formal_parameters_number,
                                ecma_object_t *scope_p,
                                bool is_strict,
                                bool do_instantiate_arguments_object,
                                const opcode_t *opcodes_p,
                                opcode_counter_t first_opcode_idx);
extern ecma_object_t*
ecma_op_create_external_function_object (ecma_external_pointer_t code_p);

extern ecma_completion_value_t
ecma_op_function_call (ecma_object_t *func_obj_p,
                       ecma_value_t this_arg_value,
                       const ecma_value_t* arguments_list_p,
                       ecma_length_t arguments_list_len);

extern ecma_completion_value_t
ecma_op_function_construct (ecma_object_t *func_obj_p,
                            const ecma_value_t* arguments_list_p,
                            ecma_length_t arguments_list_len);

extern ecma_completion_value_t
ecma_op_function_has_instance (ecma_object_t *func_obj_p,
                               ecma_value_t value);

extern ecma_completion_value_t
ecma_op_function_declaration (ecma_object_t *lex_env_p,
                              ecma_string_t *function_name_p,
                              const opcode_t *opcodes_p,
                              opcode_counter_t function_code_opcode_idx,
                              ecma_string_t* formal_parameter_list_p[],
                              ecma_length_t formal_parameter_list_length,
                              bool is_strict,
                              bool do_instantiate_arguments_object,
                              bool is_configurable_bindings);

/**
 * @}
 * @}
 */

#endif /* !ECMA_FUNCTION_OBJECT_H */

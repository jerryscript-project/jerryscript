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

#ifndef ECMA_OBJECTS_ARGUMENTS_H
#define ECMA_OBJECTS_ARGUMENTS_H

#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-value.h"

extern ecma_object_t*
ecma_create_arguments_object (ecma_object_t *func_obj_p,
                              ecma_object_t *lex_env_p,
                              ecma_collection_iterator_t *formal_params_iter_p,
                              const ecma_value_t *arguments_list_p,
                              ecma_length_t arguments_list_length,
                              bool is_strict);

extern ecma_completion_value_t ecma_op_arguments_object_get (ecma_object_t *obj_p,
                                                             ecma_string_t *property_name_p);
extern ecma_property_t *ecma_op_arguments_object_get_own_property (ecma_object_t *obj_p,
                                                                   ecma_string_t *property_name_p);
extern ecma_completion_value_t ecma_op_arguments_object_delete (ecma_object_t *obj_p,
                                                                ecma_string_t *property_name_p,
                                                                bool is_throw);
extern ecma_completion_value_t
ecma_op_arguments_object_define_own_property (ecma_object_t *obj_p,
                                              ecma_string_t *property_name_p,
                                              const ecma_property_descriptor_t* property_desc_p,
                                              bool is_throw);

#endif /* !ECMA_OBJECTS_ARGUMENTS_H */

/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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

extern void
ecma_op_create_arguments_object (ecma_object_t *, ecma_object_t *, const ecma_value_t *,
                                 ecma_length_t, const ecma_compiled_code_t *);

extern ecma_value_t
ecma_op_arguments_object_delete (ecma_object_t *, ecma_string_t *, bool);
extern ecma_value_t
ecma_op_arguments_object_define_own_property (ecma_object_t *, ecma_string_t *,
                                              const ecma_property_descriptor_t *, bool);

#endif /* !ECMA_OBJECTS_ARGUMENTS_H */

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

extern bool ecma_op_is_callable (ecma_value_t);
extern bool ecma_is_constructor (ecma_value_t);

extern ecma_object_t *
ecma_op_create_function_object (ecma_object_t *, bool, const ecma_compiled_code_t *);

extern void
ecma_op_function_list_lazy_property_names (bool,
                                           ecma_collection_header_t *,
                                           ecma_collection_header_t *);

extern ecma_property_t *
ecma_op_function_try_lazy_instantiate_property (ecma_object_t *, ecma_string_t *);

extern ecma_object_t *
ecma_op_create_external_function_object (ecma_external_pointer_t);

extern ecma_value_t
ecma_op_function_call (ecma_object_t *, ecma_value_t,
                       const ecma_value_t *, ecma_length_t);

extern ecma_value_t
ecma_op_function_construct (ecma_object_t *, const ecma_value_t *, ecma_length_t);

extern ecma_value_t
ecma_op_function_has_instance (ecma_object_t *, ecma_value_t);

/**
 * @}
 * @}
 */

#endif /* !ECMA_FUNCTION_OBJECT_H */

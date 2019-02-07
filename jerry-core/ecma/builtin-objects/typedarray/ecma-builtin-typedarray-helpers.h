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

#ifndef ECMA_TYPEDARRAY_HELPERS_H
#define ECMA_TYPEDARRAY_HELPERS_H
#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN

#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltinhelpers ECMA builtin helper operations
 * @{
 */

bool ecma_typedarray_helper_is_typedarray (uint8_t builtin_id);
uint8_t ecma_typedarray_helper_get_shift_size (uint8_t builtin_id);
uint8_t ecma_typedarray_helper_get_builtin_id (ecma_object_t *obj_p);
uint8_t ecma_typedarray_helper_get_magic_string (uint8_t builtin_id);
uint8_t ecma_typedarray_helper_get_prototype_id (uint8_t builtin_id);

ecma_value_t
ecma_typedarray_helper_dispatch_construct (const ecma_value_t *arguments_list_p,
                                           ecma_length_t arguments_list_len,
                                           uint8_t builtin_id);

/**
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
#endif /* !ECMA_TYPEDARRAY_HELPERS_H */

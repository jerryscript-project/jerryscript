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

#ifndef ECMA_SET_OBJECT_H
#define ECMA_SET_OBJECT_H

#include "ecma-globals.h"

#ifndef CONFIG_DISABLE_ES2015_SET_BUILTIN

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmasethelpers ECMA builtin set helper functions
 * @{
 */

ecma_value_t
ecma_op_set_create (const ecma_value_t *arguments_list_p, ecma_length_t arguments_list_len);

ecma_value_t
ecma_op_set_size (ecma_value_t this_arg);

ecma_value_t
ecma_op_set_has (ecma_value_t this_arg, ecma_value_t value);

ecma_value_t
ecma_op_set_add (ecma_value_t this_arg, ecma_value_t value);

ecma_value_t
ecma_op_set_clear (ecma_value_t this_arg);

ecma_value_t
ecma_op_set_delete (ecma_value_t this_arg, ecma_value_t value);

void
ecma_op_set_clear_set (ecma_set_object_t *set_object_p);

ecma_set_object_t *
ecma_op_set_get_object (ecma_value_t this_arg);

/**
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_ES2015_SET_BUILTIN */

#endif /* !ECMA_SET_OBJECT_H */

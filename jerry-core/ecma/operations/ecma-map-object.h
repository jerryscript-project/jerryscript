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

#ifndef ECMA_MAP_OBJECT_H
#define ECMA_MAP_OBJECT_H

#include "ecma-globals.h"

#ifndef CONFIG_DISABLE_ES2015_MAP_BUILTIN

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmamaphelpers ECMA builtin map helper functions
 * @{
 */

ecma_value_t ecma_op_map_create (const ecma_value_t *arguments_list_p, ecma_length_t arguments_list_len);
ecma_value_t ecma_op_map_size (ecma_value_t this_arg);
ecma_value_t ecma_op_map_get (ecma_value_t this_arg, ecma_value_t key_arg);
ecma_value_t ecma_op_map_has (ecma_value_t this_arg, ecma_value_t key_arg);
ecma_value_t ecma_op_map_set (ecma_value_t this_arg, ecma_value_t key_arg, ecma_value_t value_arg);
void ecma_op_map_clear_map (ecma_map_object_t *map_object_p);
ecma_value_t ecma_op_map_clear (ecma_value_t this_arg);
ecma_value_t ecma_op_map_delete (ecma_value_t this_arg, ecma_value_t key_arg);

/**
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_ES2015_MAP_BUILTIN */

#endif /* !ECMA_MAP_OBJECT_H */

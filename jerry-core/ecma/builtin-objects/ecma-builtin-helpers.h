/* Copyright 2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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

#ifndef ECMA_OBJECT_PROTOTYPE_H
#define ECMA_OBJECT_PROTOTYPE_H

#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltinhelpers ECMA builtin helper operations
 * @{
 */

extern ecma_completion_value_t ecma_builtin_helper_object_to_string (const ecma_value_t this_arg);
extern ecma_completion_value_t ecma_builtin_helper_get_to_locale_string_at_index (ecma_object_t *obj_p, uint32_t index);
extern ecma_completion_value_t ecma_builtin_helper_object_get_properties (ecma_object_t *obj,
                                                                          bool only_enumerable_properties);
extern uint32_t ecma_builtin_helper_array_index_normalize (ecma_number_t index, uint32_t length);

/**
 * @}
 * @}
 */

#endif /* !ECMA_OBJECT_PROPERTY_H */

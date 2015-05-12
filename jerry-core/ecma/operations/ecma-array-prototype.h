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

#ifndef ECMA_ARRAY_PROTOTYPE_H
#define ECMA_ARRAY_PROTOTYPE_H

#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaarrayprototypeops ECMA array prototype's operations
 * @{
 */

extern ecma_completion_value_t ecma_op_array_get_separator_string (ecma_value_t separator);
extern ecma_completion_value_t ecma_op_array_get_to_string_at_index (ecma_object_t *obj_p, uint32_t index);

/**
 * @}
 * @}
 */

#endif /* !ECMA_ARRAY_PROPERTY_H */

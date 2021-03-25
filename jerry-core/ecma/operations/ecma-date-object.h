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

#ifndef ECMA_DATE_OBJECT_H
#define ECMA_DATE_OBJECT_H

#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmadateobject ECMA Date object related routines
 * @{
 */

ecma_value_t
ecma_date_create (ecma_number_t tv);

ecma_number_t
ecma_date_now (void);

ecma_number_t
ecma_date_parse (ecma_string_t *str_p);

ecma_value_t
ecma_date_get_number_value (ecma_value_t date_value);

/**
 * @}
 * @}
 */

#endif /* !ECMA_DATE_OBJECT_H */

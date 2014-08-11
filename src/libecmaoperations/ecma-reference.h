/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#ifndef ECMA_REFERENCE_H
#define ECMA_REFERENCE_H

#include "ecma-globals.h"
#include "globals.h"

/** \addtogroup ecma ---TODO---
 * @{
 */

/**
 * \addtogroup references ECMA-Reference
 * @{
 */

extern ecma_reference_t ecma_op_get_identifier_reference (ecma_object_t *lex_env_p, const ecma_char_t *name_p, bool is_strict);
extern ecma_reference_t ecma_make_reference (ecma_value_t base, const ecma_char_t *name_p, bool is_strict);
extern void ecma_free_reference (const ecma_reference_t ref);

/**
 * @}
 * @}
 */

#endif /* !ECMA_REFERENCE_H */

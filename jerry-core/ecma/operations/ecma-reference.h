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

#ifndef ECMA_REFERENCE_H
#define ECMA_REFERENCE_H

#include "ecma-globals.h"
#include "jrt.h"

/** \addtogroup ecma ECMA
 * @{
 */

/**
 * \addtogroup references ECMA-Reference
 * @{
 */

/**
 * ECMA-reference (see also: ECMA-262 v5, 8.7).
 */
typedef struct
{
  /** base value */
  ecma_value_t base;

  /** referenced name */
  mem_cpointer_t referenced_name_cp : ECMA_POINTER_FIELD_WIDTH;

  /** strict reference flag */
  unsigned int is_strict : 1;
} ecma_reference_t;

extern ecma_object_t *ecma_op_resolve_reference_base (ecma_object_t *, ecma_string_t *);

extern ecma_reference_t ecma_op_get_identifier_reference (ecma_object_t *, ecma_string_t *, bool);
extern ecma_reference_t ecma_make_reference (ecma_value_t, ecma_string_t *, bool);
extern void ecma_free_reference (ecma_reference_t);

/**
 * @}
 * @}
 */

#endif /* !ECMA_REFERENCE_H */

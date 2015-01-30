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
#include "ecma-value.h"
#include "globals.h"

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
typedef struct ecma_reference_t
{
  ecma_reference_t () : base (), referenced_name_cp (ECMA_NULL_POINTER), is_strict (false)
  {
  }

  /** base value */
  ecma_value_t base;

  /** referenced name */
  unsigned int referenced_name_cp : ECMA_POINTER_FIELD_WIDTH;

  /** strict reference flag */
  unsigned int is_strict : 1;
} ecma_reference_t;

extern ecma_object_t* ecma_op_resolve_reference_base (ecma_object_t *lex_env_p,
                                                      ecma_string_t *name_p);

extern void ecma_op_get_identifier_reference (ecma_reference_t &ret,
                                              ecma_object_t *lex_env_p,
                                              ecma_string_t *name_p,
                                              bool is_strict);
extern void ecma_make_reference (ecma_reference_t &ret,
                                 const ecma_value_t& base,
                                 ecma_string_t *name_p,
                                 bool is_strict);
extern void ecma_free_reference (ecma_reference_t& ref);

/**
 * @}
 * @}
 */

#endif /* !ECMA_REFERENCE_H */

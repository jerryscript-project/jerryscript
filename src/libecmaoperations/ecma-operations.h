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

#ifndef JERRY_ECMA_OPERATIONS_H
#define JERRY_ECMA_OPERATIONS_H

#include "ecma-globals.h"
#include "ecma-lex-env.h"
#include "ecma-reference.h"

/** \addtogroup ecma ---TODO---
 * @{
 * 
 * \addtogroup ecmaoperations ECMA-defined operations
 * @{
 */

extern ecma_Reference_t ecma_OpGetIdentifierReference( ecma_Object_t *lex_env_p, ecma_Char_t *name_p, bool is_strict);

extern ecma_CompletionValue_t ecma_op_get_value( ecma_Reference_t *ref_p);
extern ecma_CompletionValue_t ecma_op_put_value( ecma_Reference_t *ref_p, ecma_Value_t value);

/**
 * @}
 * @}
 */

#endif /* JERRY_ECMA_OPERATIONS_H */

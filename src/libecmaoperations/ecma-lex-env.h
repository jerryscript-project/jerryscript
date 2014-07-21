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

#ifndef ECMA_LEX_ENV_H
#define ECMA_LEX_ENV_H

#include "ecma-globals.h"
#include "globals.h"

/** \addtogroup ecma ---TODO---
 * @{
 */

/**
 * \addtogroup lexicalenvironment Lexical environment
 * @{
 */

/* ECMA-262 v5, Table 17. Abstract methods of Environment Records */
extern ecma_CompletionValue_t ecma_OpHasBinding( ecma_Object_t *lex_env_p, ecma_Char_t *name_p);
extern ecma_CompletionValue_t ecma_OpCreateMutableBinding( ecma_Object_t *lex_env_p, ecma_Char_t *name_p, bool is_deletable);
extern ecma_CompletionValue_t ecma_OpSetMutableBinding( ecma_Object_t *lex_env_p, ecma_Char_t *name_p, ecma_Value_t value, bool is_strict);
extern ecma_CompletionValue_t ecma_OpGetBindingValue( ecma_Object_t *lex_env_p, ecma_Char_t *name_p, bool is_strict);
extern ecma_CompletionValue_t ecma_OpDeleteBinding( ecma_Object_t *lex_env_p, ecma_Char_t *name_p);
extern ecma_CompletionValue_t ecma_OpImplicitThisValue( ecma_Object_t *lex_env_p);

/* ECMA-262 v5, Table 18. Additional methods of Declarative Environment Records */
extern void ecma_OpCreateImmutableBinding( ecma_Object_t *lex_env_p, ecma_Char_t *name_p);
extern void ecma_OpInitializeImmutableBinding( ecma_Object_t *lex_env_p, ecma_Char_t *name_p, ecma_Value_t value);

/**
 * @}
 * @}
 */

#endif /* !ECMA_LEX_ENV_H */

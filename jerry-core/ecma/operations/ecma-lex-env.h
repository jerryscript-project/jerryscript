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

#ifndef ECMA_LEX_ENV_H
#define ECMA_LEX_ENV_H

#include "ecma-globals.h"
#include "ecma-reference.h"
#include "jrt.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup lexicalenvironment Lexical environment
 * @{
 *
 * \addtogroup globallexicalenvironment Global lexical environment
 * @{
 */

extern void ecma_init_global_lex_env (void);
extern void ecma_finalize_global_lex_env (void);
extern ecma_object_t *ecma_get_global_environment (void);

/**
 * @}
 */

/* ECMA-262 v5, 8.7.1 and 8.7.2 */
extern ecma_value_t ecma_op_get_value_lex_env_base (ecma_object_t *, ecma_string_t *, bool);
extern ecma_value_t ecma_op_get_value_object_base (ecma_value_t, ecma_string_t *);
extern ecma_value_t ecma_op_put_value_lex_env_base (ecma_object_t *, ecma_string_t *, bool, ecma_value_t);

/* ECMA-262 v5, Table 17. Abstract methods of Environment Records */
extern bool ecma_op_has_binding (ecma_object_t *, ecma_string_t *);
extern ecma_value_t ecma_op_create_mutable_binding (ecma_object_t *, ecma_string_t *, bool);
extern ecma_value_t ecma_op_set_mutable_binding (ecma_object_t *, ecma_string_t *, ecma_value_t, bool);
extern ecma_value_t ecma_op_get_binding_value (ecma_object_t *, ecma_string_t *, bool);
extern ecma_value_t ecma_op_delete_binding (ecma_object_t *, ecma_string_t *);
extern ecma_value_t ecma_op_implicit_this_value (ecma_object_t *);

/* ECMA-262 v5, Table 18. Additional methods of Declarative Environment Records */
extern void ecma_op_create_immutable_binding (ecma_object_t *, ecma_string_t *, ecma_value_t);

extern ecma_object_t *ecma_op_create_global_environment (ecma_object_t *);

/**
 * @}
 * @}
 */

#endif /* !ECMA_LEX_ENV_H */

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

/** \addtogroup ecma ECMA
 * @{
 */

/**
 * \addtogroup lexicalenvironment Lexical environment
 * @{
 */

/* ECMA-262 v5, 8.7.1 and 8.7.2 */
extern ecma_completion_value_t ecma_op_get_value_lex_env_base (ecma_reference_t ref);
extern ecma_completion_value_t ecma_op_get_value_object_base (ecma_reference_t ref);
extern ecma_completion_value_t ecma_op_put_value_lex_env_base (ecma_reference_t ref,
                                                               ecma_value_t value);
extern ecma_completion_value_t ecma_op_put_value_object_base (ecma_reference_t ref,
                                                              ecma_value_t value);

/* ECMA-262 v5, Table 17. Abstract methods of Environment Records */
extern ecma_completion_value_t ecma_op_has_binding (ecma_object_t *lex_env_p,
                                                    ecma_string_t *name_p);
extern ecma_completion_value_t ecma_op_create_mutable_binding (ecma_object_t *lex_env_p,
                                                               ecma_string_t *name_p,
                                                               bool is_deletable);
extern ecma_completion_value_t ecma_op_set_mutable_binding (ecma_object_t *lex_env_p,
                                                            ecma_string_t *name_p,
                                                            ecma_value_t value,
                                                            bool is_strict);
extern ecma_completion_value_t ecma_op_get_binding_value (ecma_object_t *lex_env_p,
                                                          ecma_string_t *name_p,
                                                          bool is_strict);
extern ecma_completion_value_t ecma_op_delete_binding (ecma_object_t *lex_env_p,
                                                       ecma_string_t *name_p);
extern ecma_completion_value_t ecma_op_implicit_this_value (ecma_object_t *lex_env_p);

/* ECMA-262 v5, Table 18. Additional methods of Declarative Environment Records */
extern void ecma_op_create_immutable_binding (ecma_object_t *lex_env_p,
                                              ecma_string_t *name_p);
extern void ecma_op_initialize_immutable_binding (ecma_object_t *lex_env_p,
                                                  ecma_string_t *name_p,
                                                  ecma_value_t value);

extern ecma_object_t* ecma_op_create_global_environment (ecma_object_t *glob_obj_p);
extern bool ecma_is_lexical_environment_global (ecma_object_t *lex_env_p);

/**
 * @}
 * @}
 */

#endif /* !ECMA_LEX_ENV_H */

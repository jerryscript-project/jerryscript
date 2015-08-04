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

#ifndef ECMA_BUILTINS_H
#define ECMA_BUILTINS_H

#include "ecma-globals.h"

/**
 * A built-in object's identifier
 */
typedef enum
{
#define BUILTIN(builtin_id, \
                object_type, \
                object_prototype_builtin_id, \
                is_extensible, \
                is_static, \
                lowercase_name) \
  builtin_id,
#include "ecma-builtins.inc.h"
  ECMA_BUILTIN_ID__COUNT /**< number of built-in objects */
} ecma_builtin_id_t;

/* ecma-builtins.c */
extern void ecma_init_builtins (void);
extern void ecma_finalize_builtins (void);

extern ecma_completion_value_t
ecma_builtin_dispatch_call (ecma_object_t *obj_p,
                            ecma_value_t this_arg,
                            ecma_collection_header_t *arg_collection_p);
extern ecma_completion_value_t
ecma_builtin_dispatch_construct (ecma_object_t *obj_p,
                                 ecma_collection_header_t *arg_collection_p);
extern ecma_property_t*
ecma_builtin_try_to_instantiate_property (ecma_object_t *object_p,
                                          ecma_string_t *string_p);
extern bool
ecma_builtin_is (ecma_object_t *obj_p,
                 ecma_builtin_id_t builtin_id);
extern ecma_object_t*
ecma_builtin_get (ecma_builtin_id_t builtin_id);
#endif /* !ECMA_BUILTINS_H */

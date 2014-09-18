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

#ifndef ECMA_BUILTINS_INTERNAL_H
#define ECMA_BUILTINS_INTERNAL_H

#ifndef ECMA_BUILTINS_INTERNAL
# error "!ECMA_BUILTINS_INTERNAL"
#endif /* !ECMA_BUILTINS_INTERNAL */

#include "ecma-globals.h"

/* ecma-builtin-global-object.c */
extern void ecma_builtin_init_global_object (void);
extern void ecma_builtin_finalize_global_object (void);

/* ecma-builtin-object-object.c */
extern void ecma_builtin_init_object_object (void);
extern void ecma_builtin_finalize_object_object (void);

extern void ecma_builtin_init_object_prototype_object (void);
extern void ecma_builtin_finalize_object_prototype_object (void);

/* ecma-builtin-array-object.c */
extern void ecma_builtin_init_array_object (void);
extern void ecma_builtin_finalize_array_object (void);

extern void ecma_builtin_init_array_prototype_object (void);
extern void ecma_builtin_finalize_array_prototype_object (void);

#endif /* !ECMA_BUILTINS_INTERNAL_H */

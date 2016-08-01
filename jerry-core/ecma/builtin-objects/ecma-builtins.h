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

/**
 * Construct a routine value
 */
#define ECMA_ROUTINE_VALUE(id, length) (((id) << 4) | length)

/**
 * Get routine length
 */
#define ECMA_GET_ROUTINE_LENGTH(value) ((uint8_t) ((value) & 0xf))

/**
 * Get routine ID
 */
#define ECMA_GET_ROUTINE_ID(value) ((uint16_t) ((value) >> 4))

/* ecma-builtins.c */
extern void ecma_finalize_builtins (void);

extern ecma_value_t
ecma_builtin_dispatch_call (ecma_object_t *, ecma_value_t,
                            const ecma_value_t *, ecma_length_t);
extern ecma_value_t
ecma_builtin_dispatch_construct (ecma_object_t *,
                                 const ecma_value_t *, ecma_length_t);
extern ecma_property_t *
ecma_builtin_try_to_instantiate_property (ecma_object_t *, ecma_string_t *);
extern void
ecma_builtin_list_lazy_property_names (ecma_object_t *,
                                       bool,
                                       ecma_collection_header_t *,
                                       ecma_collection_header_t *);
extern bool
ecma_builtin_is (ecma_object_t *, ecma_builtin_id_t);
extern ecma_object_t *
ecma_builtin_get (ecma_builtin_id_t);
extern bool
ecma_builtin_function_is_routine (ecma_object_t *);

#endif /* !ECMA_BUILTINS_H */

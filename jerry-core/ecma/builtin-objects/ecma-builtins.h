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

/**
 * Construct a fully accessor value
 */
#define ECMA_ACCESSOR_READ_WRITE(getter, setter) (((getter) << 8) | (setter))

/**
 * Get accessor setter ID
 */
#define ECMA_ACCESSOR_READ_WRITE_GET_SETTER_ID(value) ((uint16_t) ((value) & 0xff))

/**
 * Get accessor getter ID
 */
#define ECMA_ACCESSOR_READ_WRITE_GET_GETTER_ID(value) ((uint16_t) ((value) >> 8))

/* ecma-builtins.c */
void ecma_finalize_builtins (void);

ecma_value_t
ecma_builtin_dispatch_call (ecma_object_t *obj_p, ecma_value_t this_arg_value,
                            const ecma_value_t *arguments_list_p, ecma_length_t arguments_list_len);
ecma_value_t
ecma_builtin_dispatch_construct (ecma_object_t *obj_p,
                                 const ecma_value_t *arguments_list_p, ecma_length_t arguments_list_len);
ecma_property_t *
ecma_builtin_try_to_instantiate_property (ecma_object_t *object_p, ecma_string_t *string_p);
void
ecma_builtin_list_lazy_property_names (ecma_object_t *object_p,
                                       bool separate_enumerable,
                                       ecma_collection_header_t *main_collection_p,
                                       ecma_collection_header_t *non_enum_collection_p);
bool
ecma_builtin_is (ecma_object_t *obj_p, ecma_builtin_id_t builtin_id);
ecma_object_t *
ecma_builtin_get (ecma_builtin_id_t builtin_id);
bool
ecma_builtin_function_is_routine (ecma_object_t *func_obj_p);

#endif /* !ECMA_BUILTINS_H */

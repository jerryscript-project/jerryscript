/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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

#include "ecma-builtins.h"
#include "ecma-globals.h"

/**
 * Position of built-in object's id field in [[Built-in routine's description]] internal property
 */
#define ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_OBJECT_ID_POS   (0)

/**
 * Width of built-in object's id field in [[Built-in routine's description]] internal property
 */
#define ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_OBJECT_ID_WIDTH (8)

/**
 * Position of built-in routine's id field in [[Built-in routine's description]] internal property
 */
#define ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_ROUTINE_ID_POS \
  (ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_OBJECT_ID_POS + \
   ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_OBJECT_ID_WIDTH)

/**
 * Width of built-in routine's id field in [[Built-in routine's description]] internal property
 */
#define ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_ROUTINE_ID_WIDTH (16)

/**
 * Position of built-in routine's length field in [[Built-in routine's description]] internal property
 */
#define ECMA_BUILTIN_ROUTINE_ID_LENGTH_VALUE_POS \
  (ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_ROUTINE_ID_POS + \
   ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_ROUTINE_ID_WIDTH)

/**
 * Width of built-in routine's id field in [[Built-in routine's description]] internal property
 */
#define ECMA_BUILTIN_ROUTINE_ID_LENGTH_VALUE_WIDTH (8)

/* ecma-builtins.c */
extern ecma_object_t *
ecma_builtin_make_function_object_for_routine (ecma_builtin_id_t, uint16_t, uint8_t);

#define BUILTIN(builtin_id, \
                object_type, \
                object_prototype_builtin_id, \
                is_extensible, \
                is_static, \
                lowercase_name) \
extern ecma_value_t \
ecma_builtin_ ## lowercase_name ## _dispatch_call (const ecma_value_t *, \
                                                   ecma_length_t); \
extern ecma_value_t \
ecma_builtin_ ## lowercase_name ## _dispatch_construct (const ecma_value_t *, \
                                                        ecma_length_t); \
extern ecma_value_t \
ecma_builtin_ ## lowercase_name ## _dispatch_routine (uint16_t builtin_routine_id, \
                                                      ecma_value_t this_arg_value, \
                                                      const ecma_value_t [], \
                                                      ecma_length_t); \
extern ecma_property_t * \
ecma_builtin_ ## lowercase_name ## _try_to_instantiate_property (ecma_object_t *, \
                                                                 ecma_string_t *); \
extern void \
ecma_builtin_ ## lowercase_name ## _list_lazy_property_names (ecma_object_t *, \
                                                              bool, \
                                                              ecma_collection_header_t *, \
                                                              ecma_collection_header_t *);
#include "ecma-builtins.inc.h"

#endif /* !ECMA_BUILTINS_INTERNAL_H */

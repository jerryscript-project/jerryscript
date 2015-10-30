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
extern int32_t
ecma_builtin_bin_search_for_magic_string_id_in_array (const lit_magic_string_id_t[],
                                                      ecma_length_t, lit_magic_string_id_t);

#define BUILTIN(builtin_id, \
                object_type, \
                object_prototype_builtin_id, \
                is_extensible, \
                is_static, \
                lowercase_name) \
extern ecma_completion_value_t \
ecma_builtin_ ## lowercase_name ## _dispatch_call (const ecma_value_t *, \
                                                   ecma_length_t); \
extern ecma_completion_value_t \
ecma_builtin_ ## lowercase_name ## _dispatch_construct (const ecma_value_t *, \
                                                        ecma_length_t); \
extern ecma_completion_value_t \
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
                                                              ecma_collection_header_t *); \
extern void \
ecma_builtin_ ## lowercase_name ## _sort_property_names (void);
#include "ecma-builtins.inc.h"


#ifndef CONFIG_ECMA_COMPACT_PROFILE
# define ECMA_BUILTIN_CP_UNIMPLEMENTED(...) \
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS ("Built-in is not implemented.", __VA_ARGS__)
#else /* !CONFIG_ECMA_COMPACT_PROFILE */
# define ECMA_BUILTIN_CP_UNIMPLEMENTED(...) \
{ \
  if (false) \
  { \
    jerry_ref_unused_variables (0, __VA_ARGS__); \
  } \
  ecma_object_t *cp_error_p = ecma_builtin_get (ECMA_BUILTIN_ID_COMPACT_PROFILE_ERROR); \
  return ecma_make_throw_obj_completion_value (cp_error_p); \
}
#endif /* CONFIG_ECMA_COMPACT_PROFILE */

#endif /* !ECMA_BUILTINS_INTERNAL_H */

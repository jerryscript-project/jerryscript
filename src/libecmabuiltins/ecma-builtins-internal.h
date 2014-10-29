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

#include "ecma-builtins.h"
#include "ecma-globals.h"

/**
 * Position of built-in object's id field in [[Built-in routine ID]] internal property
 */
#define ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_OBJECT_ID_POS   (0)

/**
 * Width of built-in object's id field in [[Built-in routine ID]] internal property
 */
#define ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_OBJECT_ID_WIDTH (16)

/**
 * Position of built-in routine's id field in [[Built-in routine ID]] internal property
 */
#define ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_ROUTINE_ID_POS \
  (ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_OBJECT_ID_POS + \
   ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_OBJECT_ID_WIDTH)

/**
 * Width of built-in routine's id field in [[Built-in routine ID]] internal property
 */
#define ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_ROUTINE_ID_WIDTH (16)

/* ecma-builtins.c */
extern ecma_object_t*
ecma_builtin_make_function_object_for_routine (ecma_builtin_id_t builtin_id,
                                               ecma_magic_string_id_t routine_id,
                                               ecma_number_t length_prop_num_value);
extern int32_t
ecma_builtin_bin_search_for_magic_string_id_in_array (const ecma_magic_string_id_t ids[],
                                                      ecma_length_t array_length,
                                                      ecma_magic_string_id_t key);

/**
 * List of built-in objects in format
 * 'macro (builtin_id, object_type, object_class, object_prototype_builtin_id, lowercase_name)'
 */
#define ECMA_BUILTIN_LIST(macro) \
  macro (OBJECT_PROTOTYPE, \
         TYPE_GENERAL, \
         OBJECT_UL, \
         ECMA_BUILTIN_ID__COUNT /* no prototype */, \
         object_prototype) \
  macro (FUNCTION_PROTOTYPE, \
         TYPE_FUNCTION, \
         FUNCTION_UL, \
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE, \
         function_prototype) \
  macro (ARRAY_PROTOTYPE, \
         TYPE_ARRAY, \
         ARRAY_UL, \
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE, \
         array_prototype) \
  macro (STRING_PROTOTYPE, \
         TYPE_GENERAL, \
         STRING_UL, \
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE, \
         string_prototype) \
  macro (BOOLEAN_PROTOTYPE, \
         TYPE_GENERAL, \
         BOOLEAN_UL, \
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE, \
         boolean_prototype) \
  macro (NUMBER_PROTOTYPE, \
         TYPE_GENERAL, \
         NUMBER_UL, \
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE, \
         number_prototype) \
  macro (OBJECT, \
         TYPE_FUNCTION, \
         OBJECT_UL, \
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE, \
         object) \
  macro (MATH, \
         TYPE_GENERAL, \
         MATH_UL, \
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE, \
         math) \
  macro (ARRAY, \
         TYPE_FUNCTION, \
         FUNCTION_UL, \
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE, \
         array) \
  macro (STRING, \
         TYPE_FUNCTION, \
         FUNCTION_UL, \
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE, \
         string) \
  macro (BOOLEAN, \
         TYPE_FUNCTION, \
         FUNCTION_UL, \
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE, \
         boolean) \
  macro (NUMBER, \
         TYPE_FUNCTION, \
         FUNCTION_UL, \
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE, \
         number) \
  macro (FUNCTION, \
         TYPE_FUNCTION, \
         FUNCTION_UL, \
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE, \
         function) \
  macro (COMPACT_PROFILE_ERROR, \
         TYPE_FUNCTION, \
         COMPACT_PROFILE_ERROR_UL, \
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE, \
         compact_profile_error) \
  macro (GLOBAL, \
         TYPE_GENERAL, \
         OBJECT_UL, \
         ECMA_BUILTIN_ID__COUNT /* no prototype */, \
         global)

#define DECLARE_DISPATCH_ROUTINES(builtin_id, \
                                  object_type, \
                                  object_class, \
                                  object_prototype_builtin_id, \
                                  lowercase_name) \
extern ecma_completion_value_t \
ecma_builtin_ ## lowercase_name ## _dispatch_call (ecma_value_t *arguments_list_p, \
                                                   ecma_length_t arguments_list_len); \
extern ecma_completion_value_t \
ecma_builtin_ ## lowercase_name ## _dispatch_construct (ecma_value_t *arguments_list_p, \
                                                        ecma_length_t arguments_list_len); \
extern ecma_completion_value_t \
ecma_builtin_ ## lowercase_name ## _dispatch_routine (ecma_magic_string_id_t builtin_routine_id, \
                                                      ecma_value_t this_arg_value, \
                                                      ecma_value_t arguments_list [], \
                                                      ecma_length_t arguments_number); \
extern ecma_property_t* \
ecma_builtin_ ## lowercase_name ## _try_to_instantiate_property (ecma_object_t *obj_p, \
                                                                 ecma_string_t *prop_name_p); \
extern void \
ecma_builtin_ ## lowercase_name ## _sort_property_names (void);

ECMA_BUILTIN_LIST (DECLARE_DISPATCH_ROUTINES)

#undef DECLARE_PROPERTY_NUMBER_VARIABLES
#undef DECLARE_DISPATCH_ROUTINES

#ifndef CONFIG_ECMA_COMPACT_PROFILE
# define ECMA_BUILTIN_CP_UNIMPLEMENTED(...) \
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS ("Compact Profile optional built-in.", __VA_ARGS__)
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

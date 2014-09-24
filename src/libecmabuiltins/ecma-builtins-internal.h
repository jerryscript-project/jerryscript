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
                                               ecma_magic_string_id_t routine_id);

/* ecma-builtin-global-object.c */
extern ecma_length_t
ecma_builtin_global_get_routine_parameters_number (ecma_magic_string_id_t routine_id);
extern ecma_completion_value_t
ecma_builtin_global_dispatch_routine (ecma_magic_string_id_t builtin_routine_id,
                                      ecma_value_t this_arg_value,
                                      ecma_value_t arguments_list [],
                                      ecma_length_t arguments_number);
extern ecma_property_t*
ecma_builtin_global_try_to_instantiate_property (ecma_object_t *obj_p,
                                                 ecma_string_t *prop_name_p);

extern const ecma_length_t ecma_builtin_global_property_number;

/* ecma-builtin-object-object.c */
extern ecma_length_t
ecma_builtin_object_get_routine_parameters_number (ecma_magic_string_id_t routine_id);
extern ecma_completion_value_t
ecma_builtin_object_dispatch_routine (ecma_magic_string_id_t builtin_routine_id,
                                      ecma_value_t this_arg_value,
                                      ecma_value_t arguments_list [],
                                      ecma_length_t arguments_number);
extern ecma_property_t*
ecma_builtin_object_try_to_instantiate_property (ecma_object_t *obj_p,
                                                 ecma_string_t *prop_name_p);
extern ecma_completion_value_t
ecma_builtin_object_dispatch_call (ecma_value_t *arguments_list_p,
                                   ecma_length_t arguments_list_len);
extern ecma_completion_value_t
ecma_builtin_object_dispatch_construct (ecma_value_t *arguments_list_p,
                                        ecma_length_t arguments_list_len);

extern const ecma_length_t ecma_builtin_object_property_number;

extern void ecma_builtin_init_object_prototype_object (void);
extern void ecma_builtin_finalize_object_prototype_object (void);
extern ecma_property_t*
ecma_builtin_object_prototype_try_to_instantiate_property (ecma_object_t *obj_p,
                                                           ecma_string_t *prop_name_p);

/* ecma-builtin-math-object.c */
extern const ecma_length_t ecma_builtin_math_property_number;
extern ecma_length_t
ecma_builtin_math_get_routine_parameters_number (ecma_magic_string_id_t routine_id);
extern ecma_completion_value_t
ecma_builtin_math_dispatch_routine (ecma_magic_string_id_t builtin_routine_id,
                                    ecma_value_t this_arg_value,
                                    ecma_value_t arguments_list [],
                                    ecma_length_t arguments_number);
extern ecma_property_t*
ecma_builtin_math_try_to_instantiate_property (ecma_object_t *obj_p, ecma_string_t *prop_name_p);

/* ecma-builtin-string-object.c */
extern const ecma_length_t ecma_builtin_string_property_number;
extern ecma_length_t
ecma_builtin_string_get_routine_parameters_number (ecma_magic_string_id_t routine_id);
extern ecma_completion_value_t
ecma_builtin_string_dispatch_routine (ecma_magic_string_id_t builtin_routine_id,
                                      ecma_value_t this_arg_value,
                                      ecma_value_t arguments_list [],
                                      ecma_length_t arguments_number);
extern ecma_property_t*
ecma_builtin_string_try_to_instantiate_property (ecma_object_t *obj_p, ecma_string_t *prop_name_p);
extern ecma_completion_value_t
ecma_builtin_string_dispatch_call (ecma_value_t *arguments_list_p,
                                   ecma_length_t arguments_list_len);
extern ecma_completion_value_t
ecma_builtin_string_dispatch_construct (ecma_value_t *arguments_list_p,
                                        ecma_length_t arguments_list_len);

/* ecma-builtin-array-object.c */
extern void ecma_builtin_init_array_object (void);
extern void ecma_builtin_finalize_array_object (void);
extern ecma_property_t*
ecma_builtin_array_try_to_instantiate_property (ecma_object_t *obj_p,
                                                ecma_string_t *prop_name_p);


extern void ecma_builtin_init_array_prototype_object (void);
extern void ecma_builtin_finalize_array_prototype_object (void);
extern ecma_property_t*
ecma_builtin_array_prototype_try_to_instantiate_property (ecma_object_t *obj_p,
                                                          ecma_string_t *prop_name_p);

extern int32_t
ecma_builtin_bin_search_for_magic_string_id_in_array (const ecma_magic_string_id_t ids[],
                                                      ecma_length_t array_length,
                                                      ecma_magic_string_id_t key);
#endif /* !ECMA_BUILTINS_INTERNAL_H */

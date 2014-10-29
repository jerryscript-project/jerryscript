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

#ifndef ECMA_BUILTINS_H
#define ECMA_BUILTINS_H

#include "ecma-globals.h"

/**
 * A built-in object's identifier
 */
typedef enum
{
  ECMA_BUILTIN_ID_GLOBAL, /**< the Global object (15.1) */
  ECMA_BUILTIN_ID_OBJECT, /**< the Object object (15.2.1) */
  ECMA_BUILTIN_ID_OBJECT_PROTOTYPE, /**< the Object object (15.2.4) */
  ECMA_BUILTIN_ID_FUNCTION, /**< the Function object (15.3.1) */
  ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE, /**< the Function object (15.3.4) */
  ECMA_BUILTIN_ID_ARRAY, /**< the Array object (15.4.1) */
  ECMA_BUILTIN_ID_ARRAY_PROTOTYPE, /**< the Array object (15.4.4) */
  ECMA_BUILTIN_ID_STRING, /**< the String object (15.5.1) */
  ECMA_BUILTIN_ID_STRING_PROTOTYPE, /**< the String object (15.5.4) */
  ECMA_BUILTIN_ID_BOOLEAN, /**< the Boolean object (15.6.1) */
  ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE, /**< the Boolean object (15.6.4) */
  ECMA_BUILTIN_ID_NUMBER, /**< the Number object (15.7.1) */
  ECMA_BUILTIN_ID_NUMBER_PROTOTYPE, /**< the Number object (15.7.4) */
  ECMA_BUILTIN_ID_DATE, /**< the Date object (15.9.2) */
  ECMA_BUILTIN_ID_REGEXP, /**< the RegExp object (15.10.3) */
  ECMA_BUILTIN_ID_REGEXP_PROTOTYPE, /**< the RegExp object (15.10.6) */
  ECMA_BUILTIN_ID_ERROR, /**< the Error object (15.11.1) */
  ECMA_BUILTIN_ID_ERROR_PROTOTYPE, /**< the Error object (15.11.4) */
  ECMA_BUILTIN_ID_EVAL_ERROR, /**< the EvalError object (15.11.6.1) */
  ECMA_BUILTIN_ID_RANGE_ERROR, /**< the RangeError object (15.11.6.2) */
  ECMA_BUILTIN_ID_REFERENCE_ERROR, /**< the ReferenceError object (15.11.6.3) */
  ECMA_BUILTIN_ID_SYNTAX_ERROR, /**< the SyntaxError object (15.11.6.4) */
  ECMA_BUILTIN_ID_TYPE_ERROR, /**< the SyntaxError object (15.11.6.5) */
  ECMA_BUILTIN_ID_URI_ERROR, /**< the URIError object (15.11.6.6) */
  ECMA_BUILTIN_ID_MATH, /**< the Math object (15.8) */
  ECMA_BUILTIN_ID_JSON, /**< the JSON object (15.12) */
#ifdef CONFIG_ECMA_COMPACT_PROFILE
  ECMA_BUILTIN_ID_COMPACT_PROFILE_ERROR, /**< CompactProfileError object defined in the Compact Profile */
#endif /* CONFIG_ECMA_COMPACT_PROFILE */
  ECMA_BUILTIN_ID__COUNT /**< number of built-in objects */
} ecma_builtin_id_t;

/* ecma-builtins.c */
extern void ecma_init_builtins (void);
extern void ecma_finalize_builtins (void);

extern ecma_completion_value_t
ecma_builtin_dispatch_call (ecma_object_t *obj_p,
                            ecma_value_t this_arg,
                            ecma_value_t *arguments_list_p,
                            ecma_length_t arguments_list_len);
extern ecma_completion_value_t
ecma_builtin_dispatch_construct (ecma_object_t *obj_p,
                                 ecma_value_t *arguments_list_p,
                                 ecma_length_t arguments_list_len);
extern ecma_property_t*
ecma_builtin_try_to_instantiate_property (ecma_object_t *object_p,
                                          ecma_string_t *string_p);
extern bool
ecma_builtin_is (ecma_object_t *obj_p,
                 ecma_builtin_id_t builtin_id);
extern ecma_object_t*
ecma_builtin_get (ecma_builtin_id_t builtin_id);
#endif /* !ECMA_BUILTINS_H */

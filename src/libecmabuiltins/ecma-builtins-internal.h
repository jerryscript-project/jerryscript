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
  ECMA_BUILTIN_ID_SYNTAX_URI_ERROR, /**< the URIError object (15.11.6.6) */
  ECMA_BUILTIN_ID_MATH, /**< the Math object (15.8) */
  ECMA_BUILTIN_ID_JSON /**< the JSON object (15.12) */
} ecma_builtin_id_t;

/**
 * Identifier of an Global object's property
 */
typedef enum
{
  /* Non-object value properties */
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_NAN,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_INFINITY,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_UNDEFINED,

  /* Object value properties */
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_OBJECT,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_FUNCTION,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_ARRAY,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_STRING,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_BOOLEAN,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_NUMBER,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_DATE,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_REGEXP,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_ERROR,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_RANGE_ERROR,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_REFERENCE_ERROR,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_SYNTAX_ERROR,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_SYNTAX_URI_ERROR,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_MATH,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_JSON,

  /* Routine properties */
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_EVAL,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_PARSE_INT,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_PARSE_FLOAT,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_IS_NAN,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_IS_FINITE,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_DECODE_URI,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_DECODE_URI_COMPONENT,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_ENCODE_URI,
  ECMA_BUILTIN_GLOBAL_PROPERTY_ID_ENCODE_URI_COMPONENT,

  ECMA_BUILTIN_GLOBAL_PROPERTY_ID__COUNT
} ecma_builtin_global_property_id_t;

/* ecma-builtin-routines-dispatcher.c */
extern ecma_completion_value_t
ecma_builtin_dispatch_routine (ecma_builtin_id_t builtin_object_id,
                               uint32_t builtin_routine_id,
                               ecma_value_t arguments_list[],
                               ecma_length_t arguments_number);

/* ecma-builtin-global-object.c */
extern void ecma_builtin_init_global_object (void);
extern void ecma_builtin_finalize_global_object (void);

extern ecma_completion_value_t
ecma_builtin_global_dispatch_routine (ecma_builtin_global_property_id_t builtin_routine_id,
                                      ecma_value_t arguments_list [],
                                      ecma_length_t arguments_number);

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

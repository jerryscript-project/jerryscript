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

/*
 * Global built-in description
 */

#ifndef OBJECT_ID
# define OBJECT_ID(builtin_object_id)
#endif /* !OBJECT_ID */

#ifndef SIMPLE_VALUE
# define SIMPLE_VALUE(name, simple_value, prop_attributes)
#endif /* !SIMPLE_VALUE */

#ifndef NUMBER_VALUE
# define NUMBER_VALUE(name, number_value, prop_attributes)
#endif /* !NUMBER_VALUE */

#ifndef OBJECT_VALUE
# define OBJECT_VALUE(name, obj_builtin_id, prop_attributes)
#endif /* !OBJECT_VALUE */

#ifndef ROUTINE
# define ROUTINE(name, c_function_name, args_number, length_prop_value)
#endif /* !ROUTINE */

/* Object identifier */
OBJECT_ID (ECMA_BUILTIN_ID_GLOBAL)

/* Simple value properties:
 * (property name, simple value, writable, enumerable, configurable) */

// ECMA-262 v5, 15.1.1.3
SIMPLE_VALUE (LIT_MAGIC_STRING_UNDEFINED,
              ECMA_SIMPLE_VALUE_UNDEFINED,
              ECMA_PROPERTY_FIXED)

/* Number properties:
 *  (property name, number value, writable, enumerable, configurable) */

// ECMA-262 v5, 15.1.1.1
NUMBER_VALUE (LIT_MAGIC_STRING_NAN,
              ECMA_BUILTIN_NUMBER_NAN,
              ECMA_PROPERTY_FIXED)

// ECMA-262 v5, 15.1.1.2
NUMBER_VALUE (LIT_MAGIC_STRING_INFINITY_UL,
              ECMA_BUILTIN_NUMBER_POSITIVE_INFINITY,
              ECMA_PROPERTY_FIXED)

/* Object properties:
 *  (property name, object pointer getter) */

// ECMA-262 v5, 15.1.4.1
OBJECT_VALUE (LIT_MAGIC_STRING_OBJECT_UL,
              ECMA_BUILTIN_ID_OBJECT,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

// ECMA-262 v5, 15.1.4.2
OBJECT_VALUE (LIT_MAGIC_STRING_FUNCTION_UL,
              ECMA_BUILTIN_ID_FUNCTION,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

// ECMA-262 v5, 15.1.4.3
#ifndef CONFIG_DISABLE_ARRAY_BUILTIN
OBJECT_VALUE (LIT_MAGIC_STRING_ARRAY_UL,
              ECMA_BUILTIN_ID_ARRAY,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)
#endif /* !CONFIG_DISABLE_ARRAY_BUILTIN*/

#ifndef CONFIG_DISABLE_STRING_BUILTIN
// ECMA-262 v5, 15.1.4.4
OBJECT_VALUE (LIT_MAGIC_STRING_STRING_UL,
              ECMA_BUILTIN_ID_STRING,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)
#endif /* !CONFIG_DISABLE_STRING_BUILTIN */

#ifndef CONFIG_DISABLE_BOOLEAN_BUILTIN
// ECMA-262 v5, 15.1.4.5
OBJECT_VALUE (LIT_MAGIC_STRING_BOOLEAN_UL,
              ECMA_BUILTIN_ID_BOOLEAN,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)
#endif /* !CONFIG_DISABLE_BOOLEAN_BUILTIN */

#ifndef CONFIG_DISABLE_NUMBER_BUILTIN
// ECMA-262 v5, 15.1.4.6
OBJECT_VALUE (LIT_MAGIC_STRING_NUMBER_UL,
              ECMA_BUILTIN_ID_NUMBER,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)
#endif /* !CONFIG_DISABLE_NUMBER_BUILTIN */

#ifndef CONFIG_DISABLE_DATE_BUILTIN
// ECMA-262 v5, 15.1.4.7
OBJECT_VALUE (LIT_MAGIC_STRING_DATE_UL,
              ECMA_BUILTIN_ID_DATE,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)
#endif /* !CONFIG_DISABLE_DATE_BUILTIN */

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN
// ECMA-262 v5, 15.1.4.8
OBJECT_VALUE (LIT_MAGIC_STRING_REGEXP_UL,
              ECMA_BUILTIN_ID_REGEXP,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)
#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */

// ECMA-262 v5, 15.1.4.9
OBJECT_VALUE (LIT_MAGIC_STRING_ERROR_UL,
              ECMA_BUILTIN_ID_ERROR,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

#ifndef CONFIG_DISABLE_ERROR_BUILTINS

// ECMA-262 v5, 15.1.4.10
OBJECT_VALUE (LIT_MAGIC_STRING_EVAL_ERROR_UL,
              ECMA_BUILTIN_ID_EVAL_ERROR,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

// ECMA-262 v5, 15.1.4.11
OBJECT_VALUE (LIT_MAGIC_STRING_RANGE_ERROR_UL,
              ECMA_BUILTIN_ID_RANGE_ERROR,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

// ECMA-262 v5, 15.1.4.12
OBJECT_VALUE (LIT_MAGIC_STRING_REFERENCE_ERROR_UL,
              ECMA_BUILTIN_ID_REFERENCE_ERROR,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

// ECMA-262 v5, 15.1.4.13
OBJECT_VALUE (LIT_MAGIC_STRING_SYNTAX_ERROR_UL,
              ECMA_BUILTIN_ID_SYNTAX_ERROR,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

// ECMA-262 v5, 15.1.4.14
OBJECT_VALUE (LIT_MAGIC_STRING_TYPE_ERROR_UL,
              ECMA_BUILTIN_ID_TYPE_ERROR,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

// ECMA-262 v5, 15.1.4.15
OBJECT_VALUE (LIT_MAGIC_STRING_URI_ERROR_UL,
              ECMA_BUILTIN_ID_URI_ERROR,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)
#endif /* !CONFIG_DISABLE_ERROR_BUILTINS */

#ifndef CONFIG_DISABLE_MATH_BUILTIN
// ECMA-262 v5, 15.1.5.1
OBJECT_VALUE (LIT_MAGIC_STRING_MATH_UL,
              ECMA_BUILTIN_ID_MATH,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)
#endif /* !CONFIG_DISABLE_MATH_BUILTIN */

#ifndef CONFIG_DISABLE_JSON_BUILTIN
// ECMA-262 v5, 15.1.5.2
OBJECT_VALUE (LIT_MAGIC_STRING_JSON_U,
              ECMA_BUILTIN_ID_JSON,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)
#endif /* !CONFIG_DISABLE_JSON_BUILTIN */

/* Routine properties:
 *  (property name, C routine name, arguments number or NON_FIXED, value of the routine's length property) */

// Implementation-defined 'print' routine
ROUTINE (LIT_MAGIC_STRING_PRINT, ecma_builtin_global_object_print, NON_FIXED, 1)

ROUTINE (LIT_MAGIC_STRING_EVAL, ecma_builtin_global_object_eval, 1, 1)
ROUTINE (LIT_MAGIC_STRING_PARSE_FLOAT, ecma_builtin_global_object_parse_float, 1, 1)
ROUTINE (LIT_MAGIC_STRING_IS_NAN, ecma_builtin_global_object_is_nan, 1, 1)
ROUTINE (LIT_MAGIC_STRING_IS_FINITE, ecma_builtin_global_object_is_finite, 1, 1)
ROUTINE (LIT_MAGIC_STRING_DECODE_URI, ecma_builtin_global_object_decode_uri, 1, 1)
ROUTINE (LIT_MAGIC_STRING_DECODE_URI_COMPONENT, ecma_builtin_global_object_decode_uri_component, 1, 1)
ROUTINE (LIT_MAGIC_STRING_ENCODE_URI, ecma_builtin_global_object_encode_uri, 1, 1)
ROUTINE (LIT_MAGIC_STRING_ENCODE_URI_COMPONENT, ecma_builtin_global_object_encode_uri_component, 1, 1)
ROUTINE (LIT_MAGIC_STRING_PARSE_INT, ecma_builtin_global_object_parse_int, 2, 2)

#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN
ROUTINE (LIT_MAGIC_STRING_ESCAPE, ecma_builtin_global_object_escape, 1, 1)
ROUTINE (LIT_MAGIC_STRING_UNESCAPE, ecma_builtin_global_object_unescape, 1, 1)
#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */

#undef OBJECT_ID
#undef SIMPLE_VALUE
#undef NUMBER_VALUE
#undef STRING_VALUE
#undef OBJECT_VALUE
#undef ROUTINE

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

/* Description of built-in objects
   in format (ECMA_BUILTIN_ID_id, object_type, prototype_id, is_extensible, is_static, underscored_id) */

/* The Object.prototype object (15.2.4) */
BUILTIN (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID__COUNT /* no prototype */,
         true,
         true,
         object_prototype)

/* The Object object (15.2.1) */
BUILTIN (ECMA_BUILTIN_ID_OBJECT,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         object)

#ifndef CONFIG_DISABLE_ARRAY_BUILTIN
/* The Array.prototype object (15.4.4) */
BUILTIN (ECMA_BUILTIN_ID_ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_ARRAY,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         true,
         array_prototype)

/* The Array object (15.4.1) */
BUILTIN (ECMA_BUILTIN_ID_ARRAY,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         array)
#endif /* !CONFIG_DISABLE_ARRAY_BUILTIN*/

#ifndef CONFIG_DISABLE_STRING_BUILTIN
/* The String.prototype object (15.5.4) */
BUILTIN (ECMA_BUILTIN_ID_STRING_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         true,
         string_prototype)

/* The String object (15.5.1) */
BUILTIN (ECMA_BUILTIN_ID_STRING,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         string)
#endif /* !CONFIG_DISABLE_STRING_BUILTIN */

#ifndef CONFIG_DISABLE_BOOLEAN_BUILTIN
/* The Boolean.prototype object (15.6.4) */
BUILTIN (ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         true,
         boolean_prototype)

/* The Boolean object (15.6.1) */
BUILTIN (ECMA_BUILTIN_ID_BOOLEAN,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         boolean)
#endif /* !CONFIG_DISABLE_BOOLEAN_BUILTIN */

#ifndef CONFIG_DISABLE_NUMBER_BUILTIN
/* The Number.prototype object (15.7.4) */
BUILTIN (ECMA_BUILTIN_ID_NUMBER_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         true,
         number_prototype)

/* The Number object (15.7.1) */
BUILTIN (ECMA_BUILTIN_ID_NUMBER,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         number)
#endif /* !CONFIG_DISABLE_NUMBER_BUILTIN */

/* The Function.prototype object (15.3.4) */
BUILTIN (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         true,
         function_prototype)

/* The Function object (15.3.1) */
BUILTIN (ECMA_BUILTIN_ID_FUNCTION,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         function)

#ifndef CONFIG_DISABLE_MATH_BUILTIN
/* The Math object (15.8) */
BUILTIN (ECMA_BUILTIN_ID_MATH,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         true,
         math)
#endif /* !CONFIG_DISABLE_MATH_BUILTIN */

#ifndef CONFIG_DISABLE_JSON_BUILTIN
/* The JSON object (15.12) */
BUILTIN (ECMA_BUILTIN_ID_JSON,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         true,
         json)
#endif /* !CONFIG_DISABLE_JSON_BUILTIN */

#ifndef CONFIG_DISABLE_DATE_BUILTIN
/* The Date.prototype object (15.9.4) */
BUILTIN (ECMA_BUILTIN_ID_DATE_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         true,
         date_prototype)

/* The Date object (15.9.3) */
BUILTIN (ECMA_BUILTIN_ID_DATE,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         date)
#endif /* !CONFIG_DISABLE_DATE_BUILTIN */

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN
/* The RegExp.prototype object (15.10.6) */
BUILTIN (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         true,
         regexp_prototype)

/* The RegExp object (15.10) */
BUILTIN (ECMA_BUILTIN_ID_REGEXP,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         regexp)
#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */

/* The Error object (15.11.1) */
BUILTIN (ECMA_BUILTIN_ID_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         error)

/* The Error.prototype object (15.11.4) */
BUILTIN (ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         true,
         error_prototype)

#ifndef CONFIG_DISABLE_ERROR_BUILTINS
/* The EvalError.prototype object (15.11.6.1) */
BUILTIN (ECMA_BUILTIN_ID_EVAL_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         true,
         eval_error_prototype)

/* The EvalError object (15.11.6.1) */
BUILTIN (ECMA_BUILTIN_ID_EVAL_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         eval_error)

/* The RangeError.prototype object (15.11.6.2) */
BUILTIN (ECMA_BUILTIN_ID_RANGE_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         true,
         range_error_prototype)

/* The RangeError object (15.11.6.2) */
BUILTIN (ECMA_BUILTIN_ID_RANGE_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         range_error)

/* The ReferenceError.prototype object (15.11.6.3) */
BUILTIN (ECMA_BUILTIN_ID_REFERENCE_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         true,
         reference_error_prototype)

/* The ReferenceError object (15.11.6.3) */
BUILTIN (ECMA_BUILTIN_ID_REFERENCE_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         reference_error)

/* The SyntaxError.prototype object (15.11.6.4) */
BUILTIN (ECMA_BUILTIN_ID_SYNTAX_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         true,
         syntax_error_prototype)

/* The SyntaxError object (15.11.6.4) */
BUILTIN (ECMA_BUILTIN_ID_SYNTAX_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         syntax_error)

/* The TypeError.prototype object (15.11.6.5) */
BUILTIN (ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         true,
         type_error_prototype)

/* The TypeError object (15.11.6.5) */
BUILTIN (ECMA_BUILTIN_ID_TYPE_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         type_error)

/* The URIError.prototype object (15.11.6.6) */
BUILTIN (ECMA_BUILTIN_ID_URI_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         true,
         uri_error_prototype)

/* The URIError object (15.11.6.6) */
BUILTIN (ECMA_BUILTIN_ID_URI_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         true,
         uri_error)
#endif /* !CONFIG_DISABLE_ERROR_BUILTINS */

/**< The [[ThrowTypeError]] object (13.2.3) */
BUILTIN (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         false,
         true,
         type_error_thrower)

/* The Global object (15.1) */
BUILTIN (ECMA_BUILTIN_ID_GLOBAL,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE, /* Implementation-dependent */
         true,
         true,
         global)

#undef BUILTIN

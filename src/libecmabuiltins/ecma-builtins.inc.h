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

/* Description of built-in objects
   in format (ECMA_BUILTIN_ID_id, object_type, class_magic_string_id, prototype_id, is_extensible, underscored_id) */

/* The Object.prototype object (15.2.4) */
BUILTIN (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_MAGIC_STRING_OBJECT_UL,
         ECMA_BUILTIN_ID__COUNT /* no prototype */,
         true,
         object_prototype)

/* The Function.prototype object (15.3.4) */
BUILTIN (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_FUNCTION_UL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         function_prototype)

/* The Array.prototype object (15.4.4) */
BUILTIN (ECMA_BUILTIN_ID_ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_ARRAY,
         ECMA_MAGIC_STRING_ARRAY_UL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         array_prototype)

/* The Array object (15.4.1) */
BUILTIN (ECMA_BUILTIN_ID_STRING_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_MAGIC_STRING_STRING_UL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         string_prototype)

/* The Boolean.prototype object (15.6.4) */
BUILTIN (ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_MAGIC_STRING_BOOLEAN_UL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         boolean_prototype)

/* The Number.prototype object (15.7.4) */
BUILTIN (ECMA_BUILTIN_ID_NUMBER_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_MAGIC_STRING_NUMBER_UL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         number_prototype)

/* The Object object (15.2.1) */
BUILTIN (ECMA_BUILTIN_ID_OBJECT,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_OBJECT_UL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         object)

/* The Math object (15.8) */
BUILTIN (ECMA_BUILTIN_ID_MATH,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_MAGIC_STRING_MATH_UL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         math)

/* The Array object (15.4.1) */
BUILTIN (ECMA_BUILTIN_ID_ARRAY,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_ARRAY_UL,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         array)

/* The String object (15.5.1) */
BUILTIN (ECMA_BUILTIN_ID_STRING,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_STRING_UL,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         string)

/* The Boolean object (15.6.1) */
BUILTIN (ECMA_BUILTIN_ID_BOOLEAN,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_BOOLEAN_UL,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         boolean)

/* The Number object (15.7.1) */
BUILTIN (ECMA_BUILTIN_ID_NUMBER,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_NUMBER_UL,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         number)

/* The Function object (15.3.1) */
BUILTIN (ECMA_BUILTIN_ID_FUNCTION,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_FUNCTION_UL,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         true,
         function)

/* The Error.prototype object (15.11.4) */
BUILTIN (ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_MAGIC_STRING_ERROR_UL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         error_prototype)

/* The Error object (15.11.1) */
BUILTIN (ECMA_BUILTIN_ID_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_ERROR_UL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         error)

/* The EvalError.prototype object (15.11.6.1) */
BUILTIN (ECMA_BUILTIN_ID_EVAL_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_MAGIC_STRING_ERROR_UL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         eval_error_prototype)

/* The EvalError object (15.11.6.1) */
BUILTIN (ECMA_BUILTIN_ID_EVAL_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_ERROR_UL,
         ECMA_BUILTIN_ID_EVAL_ERROR_PROTOTYPE,
         true,
         eval_error)

/* The RangeError.prototype object (15.11.6.2) */
BUILTIN (ECMA_BUILTIN_ID_RANGE_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_MAGIC_STRING_ERROR_UL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         range_error_prototype)

/* The RangeError object (15.11.6.2) */
BUILTIN (ECMA_BUILTIN_ID_RANGE_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_ERROR_UL,
         ECMA_BUILTIN_ID_RANGE_ERROR_PROTOTYPE,
         true,
         range_error)

/* The ReferenceError.prototype object (15.11.6.3) */
BUILTIN (ECMA_BUILTIN_ID_REFERENCE_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_MAGIC_STRING_ERROR_UL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         reference_error_prototype)

/* The ReferenceError object (15.11.6.3) */
BUILTIN (ECMA_BUILTIN_ID_REFERENCE_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_ERROR_UL,
         ECMA_BUILTIN_ID_REFERENCE_ERROR_PROTOTYPE,
         true,
         reference_error)

/* The SyntaxError.prototype object (15.11.6.4) */
BUILTIN (ECMA_BUILTIN_ID_SYNTAX_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_MAGIC_STRING_ERROR_UL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         syntax_error_prototype)

/* The SyntaxError object (15.11.6.4) */
BUILTIN (ECMA_BUILTIN_ID_SYNTAX_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_ERROR_UL,
         ECMA_BUILTIN_ID_SYNTAX_ERROR_PROTOTYPE,
         true,
         syntax_error)

/* The TypeError.prototype object (15.11.6.5) */
BUILTIN (ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_MAGIC_STRING_ERROR_UL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         type_error_prototype)

/* The TypeError object (15.11.6.5) */
BUILTIN (ECMA_BUILTIN_ID_TYPE_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_ERROR_UL,
         ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE,
         true,
         type_error)

/* The URIError.prototype object (15.11.6.6) */
BUILTIN (ECMA_BUILTIN_ID_URI_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_MAGIC_STRING_ERROR_UL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         uri_error_prototype)

/* The URIError object (15.11.6.6) */
BUILTIN (ECMA_BUILTIN_ID_URI_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_ERROR_UL,
         ECMA_BUILTIN_ID_URI_ERROR_PROTOTYPE,
         true,
         uri_error)

/**< The [[ThrowTypeError]] object (13.2.3) */
BUILTIN (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_FUNCTION_UL,
         ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
         false,
         type_error_thrower)

#ifdef CONFIG_ECMA_COMPACT_PROFILE
/* The CompactProfileError object defined in the Compact Profile */
BUILTIN (ECMA_BUILTIN_ID_COMPACT_PROFILE_ERROR,
         ECMA_OBJECT_TYPE_FUNCTION,
         ECMA_MAGIC_STRING_COMPACT_PROFILE_ERROR_UL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         false,
         compact_profile_error)
#endif /* CONFIG_ECMA_COMPACT_PROFILE */

/* The Global object (15.1) */
BUILTIN (ECMA_BUILTIN_ID_GLOBAL,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_MAGIC_STRING_OBJECT_UL,
         ECMA_BUILTIN_ID__COUNT /* no prototype */,
         true,
         global)

#undef BUILTIN

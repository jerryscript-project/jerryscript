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

/* Description of built-in objects
   in format (ECMA_BUILTIN_ID_id, object_type, prototype_id, is_extensible, is_static, underscored_id) */

/* The Object.prototype object (15.2.4) */
BUILTIN (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID__COUNT /* no prototype */,
         true,
         object_prototype)

/* The Object object (15.2.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_OBJECT,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 object)

#if ENABLED (JERRY_BUILTIN_ARRAY)
/* The Array.prototype object (15.4.4) */
BUILTIN (ECMA_BUILTIN_ID_ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_ARRAY,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         array_prototype)

/* The Array object (15.4.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 array)
#endif /* ENABLED (JERRY_BUILTIN_ARRAY) */

#if ENABLED (JERRY_BUILTIN_STRING)
/* The String.prototype object (15.5.4) */
BUILTIN (ECMA_BUILTIN_ID_STRING_PROTOTYPE,
         ECMA_OBJECT_TYPE_CLASS,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         string_prototype)

/* The String object (15.5.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_STRING,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 string)
#endif /* ENABLED (JERRY_BUILTIN_STRING) */

#if ENABLED (JERRY_BUILTIN_BOOLEAN)
/* The Boolean.prototype object (15.6.4) */
BUILTIN (ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE,
         ECMA_OBJECT_TYPE_CLASS,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         boolean_prototype)

/* The Boolean object (15.6.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_BOOLEAN,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 boolean)
#endif /* ENABLED (JERRY_BUILTIN_BOOLEAN) */

#if ENABLED (JERRY_BUILTIN_NUMBER)
/* The Number.prototype object (15.7.4) */
BUILTIN (ECMA_BUILTIN_ID_NUMBER_PROTOTYPE,
         ECMA_OBJECT_TYPE_CLASS,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         number_prototype)

/* The Number object (15.7.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_NUMBER,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 number)
#endif /* ENABLED (JERRY_BUILTIN_NUMBER) */

/* The Function.prototype object (15.3.4) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
                 true,
                 function_prototype)

/* The Function object (15.3.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_FUNCTION,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 function)

#if ENABLED (JERRY_BUILTIN_MATH)
/* The Math object (15.8) */
BUILTIN (ECMA_BUILTIN_ID_MATH,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         math)
#endif /* ENABLED (JERRY_BUILTIN_MATH) */

#if ENABLED (JERRY_ES2015_BUILTIN_REFLECT)

/* The Reflect object (26.1) */
BUILTIN (ECMA_BUILTIN_ID_REFLECT,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         reflect)
#endif /* ENABLED (JERRY_ES2015_BUILTIN_REFLECT) */

#if ENABLED (JERRY_BUILTIN_JSON)
/* The JSON object (15.12) */
BUILTIN (ECMA_BUILTIN_ID_JSON,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         json)
#endif /* ENABLED (JERRY_BUILTIN_JSON) */

#if ENABLED (JERRY_BUILTIN_DATE)
/* The Date.prototype object (15.9.4) */
BUILTIN (ECMA_BUILTIN_ID_DATE_PROTOTYPE,
         ECMA_OBJECT_TYPE_CLASS,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         date_prototype)

/* The Date object (15.9.3) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_DATE,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 date)
#endif /* ENABLED (JERRY_BUILTIN_DATE) */

#if ENABLED (JERRY_BUILTIN_REGEXP)
/* The RegExp.prototype object (15.10.6) */
BUILTIN (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE,
         ECMA_OBJECT_TYPE_CLASS,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         regexp_prototype)

/* The RegExp object (15.10) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_REGEXP,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 regexp)
#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */

/* The Error object (15.11.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_ERROR,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 error)

/* The Error.prototype object (15.11.4) */
BUILTIN (ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         error_prototype)

#if ENABLED (JERRY_BUILTIN_ERRORS)
/* The EvalError.prototype object (15.11.6.1) */
BUILTIN (ECMA_BUILTIN_ID_EVAL_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         eval_error_prototype)

/* The EvalError object (15.11.6.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_EVAL_ERROR,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 eval_error)

/* The RangeError.prototype object (15.11.6.2) */
BUILTIN (ECMA_BUILTIN_ID_RANGE_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         range_error_prototype)

/* The RangeError object (15.11.6.2) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_RANGE_ERROR,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 range_error)

/* The ReferenceError.prototype object (15.11.6.3) */
BUILTIN (ECMA_BUILTIN_ID_REFERENCE_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         reference_error_prototype)

/* The ReferenceError object (15.11.6.3) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_REFERENCE_ERROR,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 reference_error)

/* The SyntaxError.prototype object (15.11.6.4) */
BUILTIN (ECMA_BUILTIN_ID_SYNTAX_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         syntax_error_prototype)

/* The SyntaxError object (15.11.6.4) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_SYNTAX_ERROR,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 syntax_error)

/* The TypeError.prototype object (15.11.6.5) */
BUILTIN (ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         type_error_prototype)

/* The TypeError object (15.11.6.5) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_TYPE_ERROR,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 type_error)

/* The URIError.prototype object (15.11.6.6) */
BUILTIN (ECMA_BUILTIN_ID_URI_ERROR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ERROR_PROTOTYPE,
         true,
         uri_error_prototype)

/* The URIError object (15.11.6.6) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_URI_ERROR,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 uri_error)
#endif /* ENABLED (JERRY_BUILTIN_ERRORS) */

/**< The [[ThrowTypeError]] object (13.2.3) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 false,
                 type_error_thrower)

#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)

/* The ArrayBuffer.prototype object (ES2015 24.1.4) */
BUILTIN (ECMA_BUILTIN_ID_ARRAYBUFFER_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         arraybuffer_prototype)

/* The ArrayBuffer object (ES2015 24.1.2) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_ARRAYBUFFER,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 arraybuffer)

 /* The %TypedArrayPrototype% object (ES2015 24.2.3) */
BUILTIN (ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         typedarray_prototype)

/* The %TypedArray% intrinsic object (ES2015 22.2.1)
   Note: The routines must be in this order. */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_TYPEDARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 typedarray)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_INT8ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 int8array)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_UINT8ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 uint8array)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_UINT8CLAMPEDARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 uint8clampedarray)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_INT16ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 int16array)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_UINT16ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 uint16array)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_INT32ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 int32array)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_UINT32ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 uint32array)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_FLOAT32ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 float32array)

#if ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_FLOAT64ARRAY,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_TYPEDARRAY,
                 true,
                 float64array)
#endif /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */


BUILTIN (ECMA_BUILTIN_ID_INT8ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         int8array_prototype)

BUILTIN (ECMA_BUILTIN_ID_UINT8ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         uint8array_prototype)

BUILTIN (ECMA_BUILTIN_ID_UINT8CLAMPEDARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         uint8clampedarray_prototype)

BUILTIN (ECMA_BUILTIN_ID_INT16ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         int16array_prototype)

BUILTIN (ECMA_BUILTIN_ID_UINT16ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         uint16array_prototype)

BUILTIN (ECMA_BUILTIN_ID_INT32ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         int32array_prototype)

BUILTIN (ECMA_BUILTIN_ID_UINT32ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         uint32array_prototype)

BUILTIN (ECMA_BUILTIN_ID_FLOAT32ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         float32array_prototype)

#if ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
BUILTIN (ECMA_BUILTIN_ID_FLOAT64ARRAY_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE,
         true,
         float64array_prototype)
#endif /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */

#if ENABLED (JERRY_ES2015_BUILTIN_PROMISE)

BUILTIN (ECMA_BUILTIN_ID_PROMISE_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         promise_prototype)

BUILTIN_ROUTINE (ECMA_BUILTIN_ID_PROMISE,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 promise)

#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROMISE) */

#if ENABLED (JERRY_ES2015_BUILTIN_MAP)

/* The Map prototype object (23.1.3) */
BUILTIN (ECMA_BUILTIN_ID_MAP_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         map_prototype)

/* The Map routine (ECMA-262 v6, 23.1.1.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_MAP,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 map)

#endif /* ENABLED (JERRY_ES2015_BUILTIN_MAP) */

#if ENABLED (JERRY_ES2015_BUILTIN_SET)

/* The Set prototype object (23.1.3) */
BUILTIN (ECMA_BUILTIN_ID_SET_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         set_prototype)

/* The Set routine (ECMA-262 v6, 23.1.1.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_SET,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 set)

#endif /* ENABLED (JERRY_ES2015_BUILTIN_SET) */

#if ENABLED (JERRY_ES2015)

/* The Symbol prototype object (ECMA-262 v6, 19.4.2.7) */
BUILTIN (ECMA_BUILTIN_ID_SYMBOL_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         symbol_prototype)

/* The Symbol routine (ECMA-262 v6, 19.4.2.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_SYMBOL,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 symbol)

#endif /* ENABLED (JERRY_ES2015) */
#if ENABLED (JERRY_ES2015_BUILTIN_ITERATOR)
/* The %IteratorPrototype% object (ECMA-262 v6, 25.1.2) */
BUILTIN (ECMA_BUILTIN_ID_ITERATOR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         iterator_prototype)

/* The %ArrayIteratorPrototype% object (ECMA-262 v6, 22.1.5.2) */
BUILTIN (ECMA_BUILTIN_ID_ARRAY_ITERATOR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ITERATOR_PROTOTYPE,
         true,
         array_iterator_prototype)

/* The %StringIteratorPrototype% object (ECMA-262 v6, 22.1.5.2) */
BUILTIN (ECMA_BUILTIN_ID_STRING_ITERATOR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ITERATOR_PROTOTYPE,
         true,
         string_iterator_prototype)

#if ENABLED (JERRY_ES2015_BUILTIN_SET)
/* The %SetIteratorPrototype% object (ECMA-262 v6, 23.2.5.2) */
BUILTIN (ECMA_BUILTIN_ID_SET_ITERATOR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ITERATOR_PROTOTYPE,
         true,
         set_iterator_prototype)
#endif /* ENABLED (JERRY_ES2015_BUILTIN_SET) */

#if ENABLED (JERRY_ES2015_BUILTIN_MAP)
/* The %MapIteratorPrototype% object (ECMA-262 v6, 23.1.5.2) */
BUILTIN (ECMA_BUILTIN_ID_MAP_ITERATOR_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_ITERATOR_PROTOTYPE,
         true,
         map_iterator_prototype)
#endif /* ENABLED (JERRY_ES2015_BUILTIN_SET) */
#endif /* ENABLED (JERRY_ES2015_BUILTIN_ITERATOR) */

#if ENABLED (JERRY_ES2015_BUILTIN_DATAVIEW)
/* The DataView prototype object (ECMA-262 v6, 24.2.3.1) */
BUILTIN (ECMA_BUILTIN_ID_DATAVIEW_PROTOTYPE,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
         true,
         dataview_prototype)

/* The DataView routine (ECMA-262 v6, 24.2.2.1) */
BUILTIN_ROUTINE (ECMA_BUILTIN_ID_DATAVIEW,
                 ECMA_OBJECT_TYPE_FUNCTION,
                 ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE,
                 true,
                 dataview)
#endif /* ENABLED (JERRY_ES2015_BUILTIN_DATAVIEW */

/* The Global object (15.1) */
BUILTIN (ECMA_BUILTIN_ID_GLOBAL,
         ECMA_OBJECT_TYPE_GENERAL,
         ECMA_BUILTIN_ID_OBJECT_PROTOTYPE, /* Implementation-dependent */
         true,
         global)

#undef BUILTIN

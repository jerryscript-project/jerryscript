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

#ifndef CONFIG_H
#define CONFIG_H

/*
 * By default all built-ins are enabled if they are not defined.
 */
#ifndef JERRY_BUILTINS
# define JERRY_BUILTINS 1
#endif

#ifndef JERRY_BUILTIN_ANNEXB
# define JERRY_BUILTIN_ANNEXB JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_ANNEXB) */

#ifndef JERRY_BUILTIN_ARRAY
# define JERRY_BUILTIN_ARRAY JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_ARRAY) */

#ifndef JERRY_BUILTIN_DATE
# define JERRY_BUILTIN_DATE JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_DATE) */

#ifndef JERRY_BUILTIN_ERRORS
# define JERRY_BUILTIN_ERRORS JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_ERRORS) */

#ifndef JERRY_BUILTIN_BOOLEAN
# define JERRY_BUILTIN_BOOLEAN JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_BOOLEAN) */

#ifndef JERRY_BUILTIN_JSON
# define JERRY_BUILTIN_JSON JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_JSON) */

#ifndef JERRY_BUILTIN_MATH
# define JERRY_BUILTIN_MATH JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_MATH) */

#ifndef JERRY_BUILTIN_NUMBER
# define JERRY_BUILTIN_NUMBER JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_NUMBER) */

#ifndef JERRY_BUILTIN_REGEXP
# define JERRY_BUILTIN_REGEXP JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_REGEXP) */

#ifndef JERRY_BUILTIN_STRING
# define JERRY_BUILTIN_STRING JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_STRING) */

/**
 * ES2015 related features, by default all of them are enabled.
 */
#ifndef JERRY_ES2015
# define JERRY_ES2015 1
#endif

#ifndef JERRY_ES2015_BUILTIN
# define JERRY_ES2015_BUILTIN JERRY_ES2015
#endif /* !defined (JERRY_ES2015_BUILTIN) */

#ifndef JERRY_ES2015_BUILTIN_DATAVIEW
# define JERRY_ES2015_BUILTIN_DATAVIEW JERRY_ES2015
#endif /* !defined (JERRY_ES2015_BUILTIN_DATAVIEW) */

#ifndef JERRY_ES2015_BUILTIN_ITERATOR
# define JERRY_ES2015_BUILTIN_ITERATOR JERRY_ES2015
#endif /* !defined (JERRY_ES2015_BUILTIN_ITERATOR) */

#ifndef JERRY_ES2015_BUILTIN_MAP
# define JERRY_ES2015_BUILTIN_MAP JERRY_ES2015
#endif /* !defined (JERRY_ES2015_BUILTIN_MAP) */

#ifndef JERRY_ES2015_BUILTIN_SET
# define JERRY_ES2015_BUILTIN_SET JERRY_ES2015
#endif /* !defined (JERRY_ES2015_BUILTIN_SET) */

#ifndef JERRY_ES2015_BUILTIN_PROMISE
# define JERRY_ES2015_BUILTIN_PROMISE JERRY_ES2015
#endif /* !defined (JERRY_ES2015_BUILTIN_PROMISE) */

#ifndef JERRY_ES2015_BUILTIN_SYMBOL
# define JERRY_ES2015_BUILTIN_SYMBOL JERRY_ES2015
#endif /* !defined (JERRY_ES2015_BUILTIN_SYMBOL) */

#ifndef JERRY_ES2015_BUILTIN_TYPEDARRAY
# define JERRY_ES2015_BUILTIN_TYPEDARRAY JERRY_ES2015
#endif /* !defined (JERRY_ES2015_BUILTIN_TYPEDARRAY) */

#ifndef JERRY_ES2015_ARROW_FUNCTION
# define JERRY_ES2015_ARROW_FUNCTION JERRY_ES2015
#endif /* !defined (JERRY_ES2015_ARROW_FUNCTION) */

#ifndef JERRY_ES2015_CLASS
# define JERRY_ES2015_CLASS JERRY_ES2015
#endif /* !defined (JERRY_ES2015_CLASS) */

#ifndef JERRY_ES2015_FOR_OF
# define JERRY_ES2015_FOR_OF JERRY_ES2015
#endif /* !defined (JERRY_ES2015_FOR_OF) */

#ifndef JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER
# define JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER JERRY_ES2015
#endif /* !defined (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER) */

#ifndef JERRY_ES2015_FUNCTION_REST_PARAMETER
# define JERRY_ES2015_FUNCTION_REST_PARAMETER JERRY_ES2015
#endif /* !defined (JERRY_ES2015_FUNCTION_REST_PARAMETER) */

#ifndef JERRY_ES2015_OBJECT_INITIALIZER
# define JERRY_ES2015_OBJECT_INITIALIZER JERRY_ES2015
#endif /* !defined (JERRY_ES2015_OBJECT_INITIALIZER) */

#ifndef JERRY_ES2015_MODULE_SYSTEM
# define JERRY_ES2015_MODULE_SYSTEM JERRY_ES2015
#endif /* !defined (JERRY_ES2015_MODULE_SYSTEM) */

#ifndef JERRY_ES2015_TEMPLATE_STRINGS
# define JERRY_ES2015_TEMPLATE_STRINGS JERRY_ES2015
#endif /* !defined (JERRY_ES2015_TEMPLATE_STRINGS) */

/**
 * Enables/disables the RegExp strict mode
 *
 * Default value: 0
 */
#ifndef JERRY_REGEXP_STRICT_MODE
# define JERRY_REGEXP_STRICT_MODE 0
#endif

/**
 * Enables/disables the unicode case conversion in the engine.
 * By default Unicode case conversion is enabled.
 */
#ifndef JERRY_UNICODE_CASE_CONVERSION
# define JERRY_UNICODE_CASE_CONVERSION 1
#endif /* !defined (JERRY_UNICODE_CASE_CONVERSION) */

/**
 * Size of heap
 */
#ifndef CONFIG_MEM_HEAP_AREA_SIZE
# define CONFIG_MEM_HEAP_AREA_SIZE (512 * 1024)
#endif /* !CONFIG_MEM_HEAP_AREA_SIZE */

/**
 * Max heap usage limit
 */
#define CONFIG_MEM_HEAP_MAX_LIMIT 8192

/**
 * Desired limit of heap usage
 */
#define CONFIG_MEM_HEAP_DESIRED_LIMIT (JERRY_MIN (CONFIG_MEM_HEAP_AREA_SIZE / 32, CONFIG_MEM_HEAP_MAX_LIMIT))

/**
 * Use 32-bit/64-bit float for ecma-numbers
 * This option is for expert use only!
 *
 * Allowed values:
 *  1: use 64-bit floating point number mode
 *  0: use 32-bit floating point number mode
 *
 * Default value: 1
 */
#ifndef JERRY_NUMBER_TYPE_FLOAT64
# define JERRY_NUMBER_TYPE_FLOAT64 1
#endif

/**
 * Disable ECMA lookup cache
 */
// #define CONFIG_ECMA_LCACHE_DISABLE

/**
 * Disable ECMA property hashmap
 */
// #define CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE

/**
 * Share of newly allocated since last GC objects among all currently allocated objects,
 * after achieving which, GC is started upon low severity try-give-memory-back requests.
 *
 * Share is calculated as the following:
 *                1.0 / CONFIG_ECMA_GC_NEW_OBJECTS_SHARE_TO_START_GC
 */
#define CONFIG_ECMA_GC_NEW_OBJECTS_SHARE_TO_START_GC (16)


/**
 * Sanity check for macros to see if the values are 0 or 1
 *
 * If a new feature is added this should be updated.
 */
/**
 * Check base builtins.
 */
#if !defined (JERRY_BUILTIN_ANNEXB) \
|| ((JERRY_BUILTIN_ANNEXB != 0) && (JERRY_BUILTIN_ANNEXB != 1))
# error "Invalid value for JERRY_BUILTIN_ANNEXB macro."
#endif
#if !defined (JERRY_BUILTIN_ARRAY) \
|| ((JERRY_BUILTIN_ARRAY != 0) && (JERRY_BUILTIN_ARRAY != 1))
# error "Invalid value for JERRY_BUILTIN_ARRAY macro."
#endif
#if !defined (JERRY_BUILTIN_BOOLEAN) \
|| ((JERRY_BUILTIN_BOOLEAN != 0) && (JERRY_BUILTIN_BOOLEAN != 1))
# error "Invalid value for JERRY_BUILTIN_BOOLEAN macro."
#endif
#if !defined (JERRY_BUILTIN_DATE) \
|| ((JERRY_BUILTIN_DATE != 0) && (JERRY_BUILTIN_DATE != 1))
# error "Invalid value for JERRY_BUILTIN_DATE macro."
#endif
#if !defined (JERRY_BUILTIN_ERRORS) \
|| ((JERRY_BUILTIN_ERRORS != 0) && (JERRY_BUILTIN_ERRORS != 1))
# error "Invalid value for JERRY_BUILTIN_ERRORS macro."
#endif
#if !defined (JERRY_BUILTIN_JSON) \
|| ((JERRY_BUILTIN_JSON != 0) && (JERRY_BUILTIN_JSON != 1))
# error "Invalid value for JERRY_BUILTIN_JSON macro."
#endif
#if !defined (JERRY_BUILTIN_MATH) \
|| ((JERRY_BUILTIN_MATH != 0) && (JERRY_BUILTIN_MATH != 1))
# error "Invalid value for JERRY_BUILTIN_MATH macro."
#endif
#if !defined (JERRY_BUILTIN_NUMBER) \
|| ((JERRY_BUILTIN_NUMBER != 0) && (JERRY_BUILTIN_NUMBER != 1))
# error "Invalid value for JERRY_BUILTIN_NUMBER macro."
#endif
#if !defined (JERRY_BUILTIN_REGEXP) \
|| ((JERRY_BUILTIN_REGEXP != 0) && (JERRY_BUILTIN_REGEXP != 1))
# error "Invalid value for JERRY_BUILTIN_REGEXP macro."
#endif
#if !defined (JERRY_BUILTIN_STRING) \
|| ((JERRY_BUILTIN_STRING != 0) && (JERRY_BUILTIN_STRING != 1))
# error "Invalid value for JERRY_BUILTIN_STRING macro."
#endif
#if !defined (JERRY_BUILTINS) \
|| ((JERRY_BUILTINS != 0) && (JERRY_BUILTINS != 1))
# error "Invalid value for JERRY_BUILTINS macro."
#endif

/**
 * Check ES2015 features
 */
#if !defined (JERRY_ES2015) \
|| ((JERRY_ES2015 != 0) && (JERRY_ES2015 != 1))
# error "Invalid value for JERRY_ES2015 macro."
#endif
#if !defined (JERRY_ES2015_ARROW_FUNCTION) \
|| ((JERRY_ES2015_ARROW_FUNCTION != 0) && (JERRY_ES2015_ARROW_FUNCTION != 1))
# error "Invalid value for JERRY_ES2015_ARROW_FUNCTION macro."
#endif
#if !defined (JERRY_ES2015_BUILTIN) \
|| ((JERRY_ES2015_BUILTIN != 0) && (JERRY_ES2015_BUILTIN != 1))
# error "Invalid value for JERRY_ES2015_BUILTIN macro."
#endif
#if !defined (JERRY_ES2015_BUILTIN_ITERATOR) \
|| ((JERRY_ES2015_BUILTIN_ITERATOR != 0) && (JERRY_ES2015_BUILTIN_ITERATOR != 1))
# error "Invalid value for JERRY_ES2015_BUILTIN_ITERATOR macro."
#endif
#if !defined (JERRY_ES2015_BUILTIN_DATAVIEW) \
|| ((JERRY_ES2015_BUILTIN_DATAVIEW != 0) && (JERRY_ES2015_BUILTIN_DATAVIEW != 1))
# error "Invalid value for JERRY_ES2015_BUILTIN_DATAVIEW macro."
#endif
#if !defined (JERRY_ES2015_BUILTIN_MAP) \
|| ((JERRY_ES2015_BUILTIN_MAP != 0) && (JERRY_ES2015_BUILTIN_MAP != 1))
# error "Invalid value for JERRY_ES2015_BUILTIN_MAP macro."
#endif
#if !defined (JERRY_ES2015_BUILTIN_SET) \
|| ((JERRY_ES2015_BUILTIN_SET != 0) && (JERRY_ES2015_BUILTIN_SET != 1))
# error "Invalid value for JERRY_ES2015_BUILTIN_SET macro."
#endif
#if !defined (JERRY_ES2015_BUILTIN_PROMISE) \
|| ((JERRY_ES2015_BUILTIN_PROMISE != 0) && (JERRY_ES2015_BUILTIN_PROMISE != 1))
# error "Invalid value for JERRY_ES2015_BUILTIN_PROMISE macro."
#endif
#if !defined (JERRY_ES2015_BUILTIN_SYMBOL) \
|| ((JERRY_ES2015_BUILTIN_SYMBOL != 0) && (JERRY_ES2015_BUILTIN_SYMBOL != 1))
# error "Invalid value for JERRY_ES2015_BUILTIN_SYMBOL macro."
#endif
#if !defined (JERRY_ES2015_BUILTIN_TYPEDARRAY) \
|| ((JERRY_ES2015_BUILTIN_TYPEDARRAY != 0) && (JERRY_ES2015_BUILTIN_TYPEDARRAY != 1))
# error "Invalid value for JERRY_ES2015_BUILTIN_TYPEDARRAY macro."
#endif
#if !defined (JERRY_ES2015_CLASS) \
|| ((JERRY_ES2015_CLASS != 0) && (JERRY_ES2015_CLASS != 1))
# error "Invalid value for JERRY_ES2015_CLASS macro."
#endif
#if !defined (JERRY_ES2015_FOR_OF) \
|| ((JERRY_ES2015_FOR_OF != 0) && (JERRY_ES2015_FOR_OF != 1))
# error "Invalid value for JERRY_ES2015_FOR_OF macro."
#endif
#if !defined (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER) \
|| ((JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER != 0) && (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER != 1))
# error "Invalid value for JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER macro."
#endif
#if !defined (JERRY_ES2015_FUNCTION_REST_PARAMETER) \
|| ((JERRY_ES2015_FUNCTION_REST_PARAMETER != 0) && (JERRY_ES2015_FUNCTION_REST_PARAMETER != 1))
# error "Invalid value for JERRY_ES2015_FUNCTION_REST_PARAMETER macro."
#endif
#if !defined (JERRY_ES2015_OBJECT_INITIALIZER) \
|| ((JERRY_ES2015_OBJECT_INITIALIZER != 0) && (JERRY_ES2015_OBJECT_INITIALIZER != 1))
# error "Invalid value for JERRY_ES2015_OBJECT_INITIALIZER macro."
#endif
#if !defined (JERRY_ES2015_MODULE_SYSTEM) \
|| ((JERRY_ES2015_MODULE_SYSTEM != 0) && (JERRY_ES2015_MODULE_SYSTEM != 1))
# error "Invalid value for JERRY_ES2015_MODULE_SYSTEM macro."
#endif
#if !defined (JERRY_ES2015_TEMPLATE_STRINGS) \
|| ((JERRY_ES2015_TEMPLATE_STRINGS != 0) && (JERRY_ES2015_TEMPLATE_STRINGS != 1))
# error "Invalid value for JERRY_ES2015_TEMPLATE_STRINGS macro."
#endif

/**
 * Internal options.
 */
#if !defined (JERRY_UNICODE_CASE_CONVERSION) \
|| ((JERRY_UNICODE_CASE_CONVERSION != 0) && (JERRY_UNICODE_CASE_CONVERSION != 1))
# error "Invalid value for JERRY_UNICODE_CASE_CONVERSION macro."
#endif
#if !defined (JERRY_NUMBER_TYPE_FLOAT64) \
|| ((JERRY_NUMBER_TYPE_FLOAT64 != 0) && (JERRY_NUMBER_TYPE_FLOAT64 != 1))
# error "Invalid value for JERRY_NUMBER_TYPE_FLOAT64 macro."
#endif

#define ENABLED(FEATURE) ((FEATURE) == 1)
#define DISABLED(FEATURE) ((FEATURE) != 1)

/**
 * Cross component requirements check.
 */
/**
 * The date module can only use the float 64 number types.
 * Do a check for this.
 */
#if ENABLED (JERRY_BUILTIN_DATE) && !ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
#  error "Date does not support float32"
#endif

#endif /* !CONFIG_H */

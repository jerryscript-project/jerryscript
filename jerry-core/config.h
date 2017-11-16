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

/**
 * Group of builtin-related features that can be disabled together.
 */
#ifdef CONFIG_DISABLE_BUILTINS
# define CONFIG_DISABLE_ANNEXB_BUILTIN
# define CONFIG_DISABLE_ARRAY_BUILTIN
# define CONFIG_DISABLE_BOOLEAN_BUILTIN
# define CONFIG_DISABLE_DATE_BUILTIN
# define CONFIG_DISABLE_ERROR_BUILTINS
# define CONFIG_DISABLE_JSON_BUILTIN
# define CONFIG_DISABLE_MATH_BUILTIN
# define CONFIG_DISABLE_NUMBER_BUILTIN
# define CONFIG_DISABLE_REGEXP_BUILTIN
# define CONFIG_DISABLE_STRING_BUILTIN
#endif /* CONFIG_DISABLE_BUILTINS */

/**
 * Group of ES2015-related features that can be disabled together.
 */
#ifdef CONFIG_DISABLE_ES2015
# define CONFIG_DISABLE_ES2015_ARROW_FUNCTION
# define CONFIG_DISABLE_ES2015_BUILTIN
# define CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
# define CONFIG_DISABLE_ES2015_TEMPLATE_STRINGS
# define CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
#endif /* CONFIG_DISABLE_ES2015 */

/**
 * Limit of data (system heap, engine's data except engine's own heap)
 */
#define CONFIG_MEM_DATA_LIMIT_MINUS_HEAP_SIZE (1024)

/**
 * Limit of stack size
 */
#define CONFIG_MEM_STACK_LIMIT (4096)

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
 */
#define CONFIG_ECMA_NUMBER_FLOAT32 (1u) /* 32-bit float */
#define CONFIG_ECMA_NUMBER_FLOAT64 (2u) /* 64-bit float */

#ifndef CONFIG_ECMA_NUMBER_TYPE
# define CONFIG_ECMA_NUMBER_TYPE CONFIG_ECMA_NUMBER_FLOAT64
#else /* CONFIG_ECMA_NUMBER_TYPE */
# if (CONFIG_ECMA_NUMBER_TYPE != CONFIG_ECMA_NUMBER_FLOAT32 \
      && CONFIG_ECMA_NUMBER_TYPE != CONFIG_ECMA_NUMBER_FLOAT64)
#  error "ECMA-number storage is configured incorrectly"
# endif /* CONFIG_ECMA_NUMBER_TYPE != CONFIG_ECMA_NUMBER_FLOAT32
           && CONFIG_ECMA_NUMBER_TYPE != CONFIG_ECMA_NUMBER_FLOAT64 */
#endif /* !CONFIG_ECMA_NUMBER_TYPE */

#if (!defined (CONFIG_DISABLE_DATE_BUILTIN) && (CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32))
#  error "Date does not support float32"
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
 * Link Global Environment to an empty declarative lexical environment
 * instead of lexical environment bound to Global Object.
 */
// #define CONFIG_ECMA_GLOBAL_ENVIRONMENT_DECLARATIVE

/**
 * Number of ecma values inlined into VM stack frame
 */
#define CONFIG_VM_STACK_FRAME_INLINED_VALUES_NUMBER (16)

#endif /* !CONFIG_H */

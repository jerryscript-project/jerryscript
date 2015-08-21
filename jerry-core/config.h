/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
 * Limit of data (system heap, engine's data except engine's own heap)
 */
#define CONFIG_MEM_DATA_LIMIT_MINUS_HEAP_SIZE (1024)

/**
 * Limit of stack size
 */
#define CONFIG_MEM_STACK_LIMIT (4096)

/**
 * Size of pool chunk
 *
 * Should not be less than size of any of ECMA Object Model's data types.
 */
#define CONFIG_MEM_POOL_CHUNK_SIZE (8)

/**
 * Size of heap chunk
 */
#define CONFIG_MEM_HEAP_CHUNK_SIZE (64)

/**
 * Size of heap
 */
#ifndef CONFIG_MEM_HEAP_AREA_SIZE
# define CONFIG_MEM_HEAP_AREA_SIZE (256 * 1024)
#elif CONFIG_MEM_HEAP_AREA_SIZE > (256 * 1024)
# error "Currently, maximum 256 kilobytes heap size is supported"
#endif /* !CONFIG_MEM_HEAP_AREA_SIZE */

/**
 * Desired limit of heap usage
 */
#define CONFIG_MEM_HEAP_DESIRED_LIMIT (CONFIG_MEM_HEAP_AREA_SIZE / 32)

/**
 * Log2 of maximum possible offset in the heap
 *
 * The option affects size of compressed pointer that in turn
 * affects size of ECMA Object Model's data types.
 *
 * In any case size of any of the types should not exceed CONFIG_MEM_POOL_CHUNK_SIZE.
 *
 * On the other hand, value 2 ^ CONFIG_MEM_HEAP_OFFSET_LOG should not be less than CONFIG_MEM_HEAP_AREA_SIZE.
 */
#define CONFIG_MEM_HEAP_OFFSET_LOG (18)

/**
 * Number of lower bits in key of literal hash table.
 */
#define CONFIG_LITERAL_HASH_TABLE_KEY_BITS (7)

/**
 * Width of fields used for holding counter of references to ecma-strings and ecma-objects
 *
 * The option affects maximum number of simultaneously existing:
 *  - references to one string;
 *  - stack references to one object
 * The number is ((2 ^ CONFIG_ECMA_REFERENCE_COUNTER_WIDTH) - 1).
 *
 * Also the option affects size of ECMA Object Model's data types.
 * In any case size of any of the types should not exceed CONFIG_MEM_POOL_CHUNK_SIZE.
 */
#define CONFIG_ECMA_REFERENCE_COUNTER_WIDTH (10)

/**
 * Maximum length of strings' concatenation
 */
#define CONFIG_ECMA_STRING_MAX_CONCATENATION_LENGTH (1048576)

/**
 * Use 32-bit/64-bit float for ecma-numbers
 */
#define CONFIG_ECMA_NUMBER_FLOAT32 (1u) /* 32-bit float */
#define CONFIG_ECMA_NUMBER_FLOAT64 (2u) /* 64-bit float */

#ifndef CONFIG_ECMA_NUMBER_TYPE
# define CONFIG_ECMA_NUMBER_TYPE CONFIG_ECMA_NUMBER_FLOAT32
#else /* !CONFIG_ECMA_NUMBER_TYPE */
# if (CONFIG_ECMA_NUMBER_TYPE != CONFIG_ECMA_NUMBER_FLOAT32 \
      && CONFIG_ECMA_NUMBER_TYPE != CONFIG_ECMA_NUMBER_FLOAT64)
#  error "ECMA-number storage is configured incorrectly"
# endif /* CONFIG_ECMA_NUMBER_TYPE != CONFIG_ECMA_NUMBER_FLOAT32
           && CONFIG_ECMA_NUMBER_TYPE != CONFIG_ECMA_NUMBER_FLOAT64 */
#endif /* CONFIG_ECMA_NUMBER_TYPE */

/**
 * Representation for ecma-characters
 */
#define CONFIG_ECMA_CHAR_ASCII (1) /* ASCII */
#define CONFIG_ECMA_CHAR_UTF16 (2) /* UTF-16 */

#ifndef CONFIG_ECMA_CHAR_ENCODING
# define CONFIG_ECMA_CHAR_ENCODING CONFIG_ECMA_CHAR_ASCII
#else /* !CONFIG_ECMA_CHAR_ENCODING */
# if (CONFIG_ECMA_CHAR_ENCODING != CONFIG_ECMA_CHAR_ASCII \
      && CONFIG_ECMA_CHAR_ENCODING != CONFIG_ECMA_CHAR_UTF16)
#  error "ECMA-char encoding is configured incorrectly"
# endif /* CONFIG_ECMA_CHAR_ENCODING != CONFIG_ECMA_CHAR_ASCII
           && CONFIG_ECMA_CHAR_ENCODING != CONFIG_ECMA_CHAR_UTF16 */
#endif /* CONFIG_ECMA_CHAR_ENCODING */

/**
 * Disable ECMA lookup cache
 */
// #define CONFIG_ECMA_LCACHE_DISABLE

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
 * Implementation should correspond to ECMA Compact Profile
 */
// #define CONFIG_ECMA_COMPACT_PROFILE

#ifdef CONFIG_ECMA_COMPACT_PROFILE
// #define CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN
// #define CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN
// #define CONFIG_ECMA_COMPACT_PROFILE_DISABLE_BOOLEAN_BUILTIN
// #define CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ERROR_BUILTINS
// #define CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN
// #define CONFIG_ECMA_COMPACT_PROFILE_DISABLE_MATH_BUILTIN
// #define CONFIG_ECMA_COMPACT_PROFILE_DISABLE_JSON_BUILTIN
#define CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN
#define CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN
#define CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ANNEXB_BUILTIN
#endif /* CONFIG_ECMA_COMPACT_PROFILE */

/**
 * Number of ecma-values inlined into VM stack frame
 */
#define CONFIG_VM_STACK_FRAME_INLINED_VALUES_NUMBER (16)

/**
 * Run GC after execution of each byte-code instruction
 */
// #define CONFIG_VM_RUN_GC_AFTER_EACH_OPCODE

/**
 * Flag, indicating whether to enable parser-time byte-code optimizations
 */
#define CONFIG_PARSER_ENABLE_PARSE_TIME_BYTE_CODE_OPTIMIZER

#endif /* !CONFIG_H */

/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#ifndef LIT_GLOBALS_H
#define LIT_GLOBALS_H

#include "jrt.h"

#if CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_ASCII
/**
 * Description of an ecma-character
 */
typedef uint8_t ecma_char_t;
#elif CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_UTF16
/**
 * Description of an ecma-character
 */
typedef uint16_t ecma_char_t;
#endif /* CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_UTF16 */

/**
 * Description of an ecma-character pointer
 */
typedef ecma_char_t *ecma_char_ptr_t;

/**
 * Null character (zt-string end marker)
 */
#define ECMA_CHAR_NULL  ((ecma_char_t) '\0')

/**
 * Description of a collection's/string's length
 */
typedef uint16_t ecma_length_t;

/**
 * ECMA string hash
 */
typedef uint8_t lit_string_hash_t;

/**
 * Length of string hash, in bits
 */
#define LIT_STRING_HASH_BITS (sizeof (lit_string_hash_t) * JERRY_BITSINBYTE)

/**
 * Number of string's last characters to use for hash calculation
 */
#define LIT_STRING_HASH_LAST_BYTES_COUNT (2)

#endif /* LIT_GLOBALS_H */

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

#ifndef LIT_MAGIC_STRINGS_H
#define LIT_MAGIC_STRINGS_H

#include "lit-globals.h"

/**
 * Limit for magic string length
 */
#define LIT_MAGIC_STRING_LENGTH_LIMIT 32

/**
 * Identifiers of ECMA and implementation-defined magic string constants
 */
typedef enum
{
#define LIT_MAGIC_STRING_FIRST_STRING_WITH_SIZE(size, id)
#define LIT_MAGIC_STRING_DEF(id, ascii_zt_string) \
     id,
#include "lit-magic-strings.inc.h"
#undef LIT_MAGIC_STRING_DEF
#undef LIT_MAGIC_STRING_FIRST_STRING_WITH_SIZE
  LIT_NON_INTERNAL_MAGIC_STRING__COUNT, /**< number of non-internal magic strings */
  LIT_INTERNAL_MAGIC_STRING_PROMISE = LIT_NON_INTERNAL_MAGIC_STRING__COUNT, /**<  [[Promise]] of promise
                                                                             *    reject or resolve functions */
  LIT_INTERNAL_MAGIC_STRING_ALREADY_RESOLVED, /**< [[AlreadyResolved]] of promise reject or resolve functions */
  LIT_INTERNAL_MAGIC_STRING_RESOLVE_FUNCTION, /**< the resolve funtion of the promise object */
  LIT_INTERNAL_MAGIC_STRING_REJECT_FUNCTION, /**< the reject function of the promise object */
  LIT_NEED_MARK_MAGIC_STRING__COUNT,  /**< number of internal magic strings which will be used as properties' names,
                                       *   and the properties need to be marked during gc. */
  LIT_INTERNAL_MAGIC_STRING_NATIVE_HANDLE = LIT_NEED_MARK_MAGIC_STRING__COUNT, /**< native handle package
                                                                                *   associated with an object */
  LIT_INTERNAL_MAGIC_STRING_NATIVE_POINTER, /**< native pointer package associated with an object */
  LIT_MAGIC_STRING__COUNT /**< number of magic strings */
} lit_magic_string_id_t;

/**
 * Identifiers of implementation-defined external magic string constants
 */
typedef uint32_t lit_magic_string_ex_id_t;

uint32_t lit_get_magic_string_ex_count (void);

const lit_utf8_byte_t *lit_get_magic_string_utf8 (lit_magic_string_id_t id);
lit_utf8_size_t lit_get_magic_string_size (lit_magic_string_id_t id);
lit_magic_string_id_t lit_get_magic_string_size_block_start (lit_utf8_size_t size);

const lit_utf8_byte_t *lit_get_magic_string_ex_utf8 (lit_magic_string_ex_id_t id);
lit_utf8_size_t lit_get_magic_string_ex_size (lit_magic_string_ex_id_t id);

void lit_magic_strings_ex_set (const lit_utf8_byte_t **ex_str_items, uint32_t count,
                               const lit_utf8_size_t *ex_str_sizes);

lit_magic_string_id_t lit_is_utf8_string_magic (const lit_utf8_byte_t *string_p, lit_utf8_size_t string_size);
lit_magic_string_id_t lit_is_utf8_string_pair_magic (const lit_utf8_byte_t *string1_p, lit_utf8_size_t string1_size,
                                                     const lit_utf8_byte_t *string2_p, lit_utf8_size_t string2_size);

lit_magic_string_ex_id_t lit_is_ex_utf8_string_magic (const lit_utf8_byte_t *string_p, lit_utf8_size_t string_size);
lit_magic_string_ex_id_t lit_is_ex_utf8_string_pair_magic (const lit_utf8_byte_t *string1_p,
                                                           lit_utf8_size_t string1_size,
                                                           const lit_utf8_byte_t *string2_p,
                                                           lit_utf8_size_t string2_size);

lit_utf8_byte_t *lit_copy_magic_string_to_buffer (lit_magic_string_id_t id, lit_utf8_byte_t *buffer_p,
                                                  lit_utf8_size_t buffer_size);

#endif /* !LIT_MAGIC_STRINGS_H */

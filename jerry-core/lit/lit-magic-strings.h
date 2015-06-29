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
#define LIT_MAGIC_STRING_DEF(id, ascii_zt_string) \
     id,
#include "lit-magic-strings.inc.h"
#undef LIT_MAGIC_STRING_DEF

  LIT_MAGIC_STRING__COUNT /**< number of magic strings */
} lit_magic_string_id_t;

/**
 * Identifiers of implementation-defined external magic string constants
 */
typedef uint32_t lit_magic_string_ex_id_t;

extern void lit_magic_strings_init (void);
extern void lit_magic_strings_ex_init (void);

extern void lit_magic_strings_ex_set (const ecma_char_ptr_t *,
                                      uint32_t,
                                      const ecma_length_t *);
extern uint32_t ecma_get_magic_string_ex_count (void);

extern const ecma_char_t *lit_get_magic_string_zt (lit_magic_string_id_t);
extern ecma_length_t lit_get_magic_string_length (lit_magic_string_id_t);

extern const ecma_char_t *lit_get_magic_string_ex_zt (lit_magic_string_ex_id_t);
extern ecma_length_t lit_get_magic_string_ex_length (lit_magic_string_ex_id_t);

extern bool lit_is_zt_string_magic (const ecma_char_t *, lit_magic_string_id_t *);
extern bool lit_is_zt_ex_string_magic (const ecma_char_t *, lit_magic_string_ex_id_t *);

#endif /* LIT_MAGIC_STRINGS_H */

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

#ifndef LIT_LITERAL_H
#define LIT_LITERAL_H

#include "ecma-globals.h"

extern void lit_init ();
extern void lit_finalize ();
extern void lit_dump_literals ();

extern lit_literal_t lit_create_literal_from_utf8_string (const lit_utf8_byte_t *, lit_utf8_size_t);
extern lit_literal_t lit_find_literal_by_utf8_string (const lit_utf8_byte_t *, lit_utf8_size_t);
extern lit_literal_t lit_find_or_create_literal_from_utf8_string (const lit_utf8_byte_t *, lit_utf8_size_t);

extern lit_literal_t lit_create_literal_from_num (ecma_number_t);
extern lit_literal_t lit_find_literal_by_num (ecma_number_t);
extern lit_literal_t lit_find_or_create_literal_from_num (ecma_number_t);

extern bool lit_literal_equal_utf8 (lit_literal_t, const lit_utf8_byte_t *, lit_utf8_size_t);
extern bool lit_literal_equal_num (lit_literal_t, ecma_number_t);
extern bool lit_literal_equal (lit_literal_t, lit_literal_t);

extern bool lit_literal_equal_type_utf8 (lit_literal_t, const lit_utf8_byte_t *, lit_utf8_size_t);
extern bool lit_literal_equal_type_cstr (lit_literal_t, const char *);
extern bool lit_literal_equal_type_num (lit_literal_t, ecma_number_t);
extern bool lit_literal_equal_type (lit_literal_t, lit_literal_t);

extern const lit_utf8_byte_t *lit_literal_to_utf8_string (lit_literal_t, lit_utf8_byte_t *, size_t);
extern const char *lit_literal_to_str_internal_buf (lit_literal_t);

extern lit_literal_t lit_get_literal_by_cp (lit_cpointer_t);

extern ecma_number_t lit_number_literal_get_number (lit_literal_t);
extern lit_string_hash_t lit_charset_literal_get_hash (lit_literal_t);
extern lit_utf8_size_t lit_charset_literal_get_size (lit_literal_t);
extern ecma_length_t lit_charset_literal_get_length (lit_literal_t);

extern lit_magic_string_id_t lit_magic_literal_get_magic_str_id (lit_literal_t);
extern lit_magic_string_ex_id_t lit_magic_literal_ex_get_magic_str_id (lit_literal_t);

#endif /* LIT_LITERAL_H */

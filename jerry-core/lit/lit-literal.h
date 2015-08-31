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

#include "lit-globals.h"
#include "lit-literal-storage.h"
#include "lit-magic-strings.h"

void lit_init ();
void lit_finalize ();
void lit_dump_literals ();

literal_t lit_create_literal_from_utf8_string (const lit_utf8_byte_t *, lit_utf8_size_t);
literal_t lit_find_literal_by_utf8_string (const lit_utf8_byte_t *, lit_utf8_size_t);
literal_t lit_find_or_create_literal_from_utf8_string (const lit_utf8_byte_t *, lit_utf8_size_t);

literal_t lit_create_literal_from_num (ecma_number_t);
literal_t lit_find_literal_by_num (ecma_number_t);
literal_t lit_find_or_create_literal_from_num (ecma_number_t);

bool lit_literal_equal_utf8 (literal_t, const lit_utf8_byte_t *, lit_utf8_size_t);
bool lit_literal_equal_num (literal_t, ecma_number_t);
bool lit_literal_equal (literal_t, literal_t);

bool lit_literal_equal_type_utf8 (literal_t, const lit_utf8_byte_t *, lit_utf8_size_t);
bool lit_literal_equal_type_cstr (literal_t, const char *);
bool lit_literal_equal_type_num (literal_t, ecma_number_t);
bool lit_literal_equal_type (literal_t, literal_t);

const lit_utf8_byte_t *lit_literal_to_utf8_string (literal_t, lit_utf8_byte_t *, size_t);
const char *lit_literal_to_str_internal_buf (literal_t);

literal_t lit_get_literal_by_cp (lit_cpointer_t);

lit_string_hash_t lit_charset_literal_get_hash (literal_t);
ecma_number_t lit_charset_literal_get_number (literal_t);
lit_utf8_size_t lit_charset_record_get_size (literal_t);
ecma_length_t lit_charset_record_get_length (literal_t);

lit_magic_string_id_t lit_magic_record_get_magic_str_id (literal_t);
lit_magic_string_ex_id_t lit_magic_record_ex_get_magic_str_id (literal_t);

#endif /* LIT_LITERAL_H */

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

#ifndef LITERAL_H
#define LITERAL_H

#include "ecma-globals.h"
#include "lp-string.h"

typedef enum
{
  LIT_UNKNOWN,
  LIT_STR,
  LIT_MAGIC_STR,
  LIT_NUMBER
}
literal_type;

typedef struct
{
  union
  {
    ecma_magic_string_id_t magic_str_id;
    ecma_number_t num;
    lp_string lp;
    void *none;
  }
  data;
  literal_type type;
}
literal;

#define LITERAL_TO_REWRITE (INVALID_VALUE - 1)

literal create_empty_literal (void);
literal create_literal_from_num (ecma_number_t);
literal create_literal_from_str (const char *, ecma_length_t);
literal create_literal_from_str_compute_len (const char *);
literal create_literal_from_zt (const ecma_char_t *, ecma_length_t);
bool literal_equal (literal, literal);
bool literal_equal_s (literal, const char *);
bool literal_equal_zt (literal, const ecma_char_t *);
bool literal_equal_num (literal, ecma_number_t);
bool literal_equal_type (literal, literal);
bool literal_equal_type_s (literal, const char *);
bool literal_equal_type_zt (literal, const ecma_char_t *);
bool literal_equal_type_num (literal, ecma_number_t);
const ecma_char_t *literal_to_zt (literal);

#endif /* LITERAL_H */

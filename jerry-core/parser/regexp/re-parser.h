/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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

#ifndef RE_PARSER_H
#define RE_PARSER_H

#include "opcodes-dumper.h"

typedef uint8_t token_type_t;

#define RE_TOK_MATCH      1
#define RE_TOK_CHAR       2
#define RE_TOK_RE_START   3
#define RE_TOK_RE_END     4

typedef struct re_token_t
{
  token_type_t type;
  uint32_t value;
} re_token_t;

operand
parse_regexp_literal();

re_token_t
re_parse_next_token (ecma_char_t **pattern);

#endif /* RE_PARSER_H */

/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#ifndef RE_COMPILER_H
#define RE_COMPILER_H

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN

#include "ecma-globals.h"
#include "re-parser.h"

/* RegExp opcodes
 * Group opcode order is important, because RE_IS_CAPTURE_GROUP is based on it.
 * Change it carfully. Capture opcodes should be at first.
 */
#define RE_OP_EOF                                           0

#define RE_OP_CAPTURE_GROUP_START                           1
#define RE_OP_CAPTURE_GREEDY_ZERO_GROUP_START               2
#define RE_OP_CAPTURE_NON_GREEDY_ZERO_GROUP_START           3
#define RE_OP_CAPTURE_GREEDY_GROUP_END                      4
#define RE_OP_CAPTURE_NON_GREEDY_GROUP_END                  5
#define RE_OP_NON_CAPTURE_GROUP_START                       6
#define RE_OP_NON_CAPTURE_GREEDY_ZERO_GROUP_START           7
#define RE_OP_NON_CAPTURE_NON_GREEDY_ZERO_GROUP_START       8
#define RE_OP_NON_CAPTURE_GREEDY_GROUP_END                  9
#define RE_OP_NON_CAPTURE_NON_GREEDY_GROUP_END              10

#define RE_OP_MATCH                                         11
#define RE_OP_CHAR                                          12
#define RE_OP_SAVE_AT_START                                 13
#define RE_OP_SAVE_AND_MATCH                                14
#define RE_OP_PERIOD                                        15
#define RE_OP_ALTERNATIVE                                   16
#define RE_OP_GREEDY_ITERATOR                               17
#define RE_OP_NON_GREEDY_ITERATOR                           18
#define RE_OP_ASSERT_START                                  19
#define RE_OP_ASSERT_END                                    20
#define RE_OP_ASSERT_WORD_BOUNDARY                          21
#define RE_OP_ASSERT_NOT_WORD_BOUNDARY                      22
#define RE_OP_LOOKAHEAD_POS                                 23
#define RE_OP_LOOKAHEAD_NEG                                 24
#define RE_OP_BACKREFERENCE                                 25
#define RE_OP_CHAR_CLASS                                    26
#define RE_OP_INV_CHAR_CLASS                                27

#define RE_COMPILE_RECURSION_LIMIT  100

#define RE_IS_CAPTURE_GROUP(x) (((x) < RE_OP_NON_CAPTURE_GROUP_START) ? 1 : 0)

typedef uint8_t re_opcode_t; /* type of RegExp opcodes */
typedef uint8_t re_bytecode_t; /* type of standard bytecode elements (ex.: opcode parameters) */

/**
 * Context of RegExp bytecode container
 *
 * FIXME:
 *       Add comments with description of the structure members
 */
typedef struct
{
  re_bytecode_t *block_start_p;
  re_bytecode_t *block_end_p;
  re_bytecode_t *current_p;
} re_bytecode_ctx_t;

/**
 * Context of RegExp compiler
 *
 * FIXME:
 *       Add comments with description of the structure members
 */
typedef struct
{
  uint8_t flags;
  uint32_t recursion_depth;
  uint32_t num_of_captures;
  uint32_t num_of_non_captures;
  uint32_t highest_backref;
  re_bytecode_ctx_t *bytecode_ctx_p;
  re_token_t current_token;
  re_parser_ctx_t *parser_ctx_p;
} re_compiler_ctx_t;

ecma_completion_value_t
re_compile_bytecode (ecma_property_t *bytecode_p, ecma_string_t *pattern_str_p, uint8_t flags);

re_opcode_t
re_get_opcode (re_bytecode_t **bc_p);

uint32_t
re_get_value (re_bytecode_t **bc_p);

#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
#endif /* RE_COMPILER_H */

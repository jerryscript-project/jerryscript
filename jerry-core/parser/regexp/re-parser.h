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

#ifndef RE_PARSER_H
#define RE_PARSER_H

#if ENABLED (JERRY_BUILTIN_REGEXP)

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup regexparser Regular expression
 * @{
 *
 * \addtogroup regexparser_bytecode Bytecode
 * @{
 */

/**
 * RegExp token type definitions
 */
typedef enum
{
  RE_TOK_EOF,                        /**< EOF */
  RE_TOK_BACKREFERENCE,              /**< "\[0..9]" */
  RE_TOK_CHAR,                       /**< any character */
  RE_TOK_ALTERNATIVE,                /**< "|" */
  RE_TOK_ASSERT_START,               /**< "^" */
  RE_TOK_ASSERT_END,                 /**< "$" */
  RE_TOK_PERIOD,                     /**< "." */
  RE_TOK_START_CAPTURE_GROUP,        /**< "(" */
  RE_TOK_START_NON_CAPTURE_GROUP,    /**< "(?:" */
  RE_TOK_END_GROUP,                  /**< ")" */
  RE_TOK_ASSERT_START_POS_LOOKAHEAD, /**< "(?=" */
  RE_TOK_ASSERT_START_NEG_LOOKAHEAD, /**< "(?!" */
  RE_TOK_ASSERT_WORD_BOUNDARY,       /**< "\b" */
  RE_TOK_ASSERT_NOT_WORD_BOUNDARY,   /**< "\B" */
  RE_TOK_DIGIT,                      /**< "\d" */
  RE_TOK_NOT_DIGIT,                  /**< "\D" */
  RE_TOK_WHITE,                      /**< "\s" */
  RE_TOK_NOT_WHITE,                  /**< "\S" */
  RE_TOK_WORD_CHAR,                  /**< "\w" */
  RE_TOK_NOT_WORD_CHAR,              /**< "\W" */
  RE_TOK_START_CHAR_CLASS,           /**< "[ ]" */
  RE_TOK_START_INV_CHAR_CLASS,       /**< "[^ ]" */
} re_token_type_t;

/**
 * @}
 *
 * \addtogroup regexparser_parser Parser
 * @{
 */

/**
 * RegExp constant of infinite
 */
#define RE_ITERATOR_INFINITE ((uint32_t) - 1)

/**
 * Maximum number of decimal escape digits
 */
#define RE_MAX_RE_DECESC_DIGITS 9

/**
 * RegExp token type
 */
typedef struct
{
  re_token_type_t type;   /**< type of the token */
  uint32_t value;         /**< value of the token */
  uint32_t qmin;          /**< minimum number of token iterations */
  uint32_t qmax;          /**< maximum number of token iterations */
  bool greedy;            /**< type of iteration */
} re_token_t;

/**
  * RegExp parser context
  */
typedef struct
{
  const lit_utf8_byte_t *input_start_p; /**< start of input pattern */
  const lit_utf8_byte_t *input_curr_p;  /**< current position in input pattern */
  const lit_utf8_byte_t *input_end_p;   /**< end of input pattern */
  int groups_count;                     /**< number of groups */
  uint32_t classes_count;               /**< number of character classes */
} re_parser_ctx_t;

bool re_hex_lookup (re_parser_ctx_t *parser_ctx_p, uint32_t lookup);
uint32_t re_parse_octal (re_parser_ctx_t *parser_ctx_p);
ecma_value_t re_parse_iterator (re_parser_ctx_t *parser_ctx_p, re_token_t *re_token_p);
ecma_value_t re_parse_next_token (re_parser_ctx_t *parser_ctx_p, re_token_t *out_token_p);

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */
#endif /* !RE_PARSER_H */

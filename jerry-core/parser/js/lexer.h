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

#ifndef LEXER_H
#define LEXER_H

#include "lit-literal.h"
#include "lit-strings.h"

#define INVALID_LITERAL (rcs_cpointer_null_cp ())

/* Keywords.  */
typedef enum __attr_packed___
{
  /* Not a keyword.  */
  KW_BREAK,
  KW_CASE,
  KW_CATCH,
  KW_CLASS,

  KW_CONST,
  KW_CONTINUE,
  KW_DEBUGGER,
  KW_DEFAULT,
  KW_DELETE,

  KW_DO,
  KW_ELSE,
  KW_ENUM,
  KW_EXPORT,
  KW_EXTENDS,

  KW_FINALLY,
  KW_FOR,
  KW_FUNCTION,
  KW_IF,
  KW_IN,

  KW_INSTANCEOF,
  KW_INTERFACE,
  KW_IMPORT,
  KW_IMPLEMENTS,
  KW_LET,

  KW_NEW,
  KW_PACKAGE,
  KW_PRIVATE,
  KW_PROTECTED,
  KW_PUBLIC,

  KW_RETURN,
  KW_STATIC,
  KW_SUPER,
  KW_SWITCH,
  KW_THIS,

  KW_THROW,
  KW_TRY,
  KW_TYPEOF,
  KW_VAR,
  KW_VOID,

  KW_WHILE,
  KW_WITH,
  KW_YIELD
} keyword;


/* Type of tokens.  */
typedef enum __attr_packed___
{
  TOKEN_TYPE__BEGIN = 1,

  TOK_EOF = TOKEN_TYPE__BEGIN, // End of file
  TOK_NAME, // Identifier
  TOK_SMALL_INT,
  TOK_NUMBER,

  TOK_NULL,
  TOK_BOOL,
  TOK_STRING,
  TOK_OPEN_BRACE, // {

  TOK_CLOSE_BRACE, // }
  TOK_OPEN_PAREN, // (
  TOK_CLOSE_PAREN, //)
  TOK_OPEN_SQUARE, // [
  TOK_CLOSE_SQUARE, // ]

  TOK_DOT, // .
  TOK_SEMICOLON, // ;
  TOK_COMMA, // ,

  TOKEN_TYPE__UNARY_BEGIN,
  TOKEN_TYPE__ADDITIVE_BEGIN = TOKEN_TYPE__UNARY_BEGIN,

  TOK_PLUS = TOKEN_TYPE__ADDITIVE_BEGIN, // +
  TOK_MINUS, // -

  TOKEN_TYPE__ADDITIVE_END = TOK_MINUS,

  TOK_DOUBLE_PLUS, // ++
  TOK_DOUBLE_MINUS, // --
  TOK_NOT, // !
  TOK_COMPL, // ~

  TOKEN_TYPE__UNARY_END = TOK_COMPL, /* keywords are not listed
                                      * in the range */

  TOKEN_TYPE__MULTIPLICATIVE_BEGIN,

  TOK_MULT = TOKEN_TYPE__MULTIPLICATIVE_BEGIN, // *
  TOK_MOD, // %
  TOK_DIV, // /

  TOKEN_TYPE__MULTIPLICATIVE_END = TOK_DIV,

  TOKEN_TYPE__SHIFT_BEGIN,

  TOK_LSHIFT = TOKEN_TYPE__SHIFT_BEGIN, // <<
  TOK_RSHIFT, // >>
  TOK_RSHIFT_EX, // >>>

  TOKEN_TYPE__SHIFT_END = TOK_RSHIFT_EX,

  TOKEN_TYPE__RELATIONAL_BEGIN,

  TOK_LESS = TOKEN_TYPE__RELATIONAL_BEGIN, // <
  TOK_GREATER, // >
  TOK_LESS_EQ, // <=
  TOK_GREATER_EQ, // <=

  TOKEN_TYPE__RELATIONAL_END = TOK_GREATER_EQ,

  TOKEN_TYPE__EQUALITY_BEGIN,

  TOK_DOUBLE_EQ = TOKEN_TYPE__EQUALITY_BEGIN, // ==
  TOK_NOT_EQ, // !=
  TOK_TRIPLE_EQ, // ===
  TOK_NOT_DOUBLE_EQ, // !==

  TOKEN_TYPE__EQUALITY_END = TOK_NOT_DOUBLE_EQ,

  TOK_AND, // &
  TOK_OR, // |
  TOK_XOR, // ^

  TOK_DOUBLE_AND, // &&
  TOK_DOUBLE_OR, // ||
  TOK_QUERY, // ?
  TOK_COLON, // :

  TOKEN_TYPE__ASSIGNMENTS_BEGIN,

  TOK_EQ = TOKEN_TYPE__ASSIGNMENTS_BEGIN, // =
  TOK_PLUS_EQ, // +=
  TOK_MINUS_EQ, // -=
  TOK_MULT_EQ, // *=
  TOK_MOD_EQ, // %=
  TOK_LSHIFT_EQ, // <<=
  TOK_RSHIFT_EQ, // >>=
  TOK_RSHIFT_EX_EQ, // >>>=
  TOK_AND_EQ, // &=
  TOK_OR_EQ, // |=
  TOK_XOR_EQ, // ^=
  TOK_DIV_EQ, // /=

  TOKEN_TYPE__ASSIGNMENTS_END = TOK_DIV_EQ,

  TOK_EMPTY,
  TOK_REGEXP, // RegularExpressionLiteral (/.../gim)

  TOKEN_TYPE__KEYWORD_BEGIN,

  TOK_KW_BREAK,
  TOK_KW_CASE,
  TOK_KW_CATCH,
  TOK_KW_CLASS,

  TOK_KW_CONST,
  TOK_KW_CONTINUE,
  TOK_KW_DEBUGGER,
  TOK_KW_DEFAULT,
  TOK_KW_DELETE,

  TOK_KW_DO,
  TOK_KW_ELSE,
  TOK_KW_ENUM,
  TOK_KW_EXPORT,
  TOK_KW_EXTENDS,

  TOK_KW_FINALLY,
  TOK_KW_FOR,
  TOK_KW_FUNCTION,
  TOK_KW_IF,
  TOK_KW_IN,

  TOK_KW_INSTANCEOF,
  TOK_KW_INTERFACE,
  TOK_KW_IMPORT,
  TOK_KW_IMPLEMENTS,
  TOK_KW_LET,

  TOK_KW_NEW,
  TOK_KW_PACKAGE,
  TOK_KW_PRIVATE,
  TOK_KW_PROTECTED,
  TOK_KW_PUBLIC,

  TOK_KW_RETURN,
  TOK_KW_STATIC,
  TOK_KW_SUPER,
  TOK_KW_SWITCH,
  TOK_KW_THIS,

  TOK_KW_THROW,
  TOK_KW_TRY,
  TOK_KW_TYPEOF,
  TOK_KW_VAR,
  TOK_KW_VOID,

  TOK_KW_WHILE,
  TOK_KW_WITH,
  TOK_KW_YIELD,

  TOKEN_TYPE__KEYWORD_END = TOK_KW_YIELD,

  TOKEN_TYPE__END = TOKEN_TYPE__KEYWORD_END
} jsp_token_type_t;

/* Flags, describing token properties */
typedef enum
{
  JSP_TOKEN_FLAG__NO_FLAGS            = 0x00, /* default flag */
  JSP_TOKEN_FLAG_PRECEDED_BY_NEWLINES = 0x01  /* designates that newline precedes the token */
} jsp_token_flag_t;

typedef lit_utf8_iterator_pos_t locus;

/* Represents the contents of a token.  */
typedef struct
{
  locus loc; /**< token location */
  uint16_t uid; /**< encodes token's value, depending on token type */
  uint8_t type; /**< token type */
  uint8_t flags; /**< token flags */
} token;

/**
 * Initializer for empty token
 */
#define TOKEN_EMPTY_INITIALIZER {LIT_ITERATOR_POS_ZERO, 0, TOK_EMPTY, JSP_TOKEN_FLAG__NO_FLAGS}

void lexer_init (const jerry_api_char_t *, size_t, bool);

token lexer_next_token (bool, bool);

void lexer_seek (locus);
void lexer_locus_to_line_and_column (locus, size_t *, size_t *);
void lexer_dump_line (size_t);
const char *lexer_token_type_to_string (jsp_token_type_t);

jsp_token_type_t lexer_get_token_type (token);
bool lexer_is_preceded_by_newlines (token);

extern bool lexer_are_tokens_with_same_identifier (token, token);

extern bool lexer_is_no_escape_sequences_in_token_string (token);

#endif

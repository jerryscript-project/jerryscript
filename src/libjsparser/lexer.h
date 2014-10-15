/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#include "globals.h"
#include "ecma-globals.h"
#include "lp-string.h"

/* Keywords.  */
typedef enum
{
  /* Not a keyword.  */
  KW_NONE = 0,
  KW_BREAK,
  KW_CASE,
  KW_CATCH,
  KW_CONTINUE,

  KW_DEBUGGER,
  KW_DEFAULT,
  KW_DELETE,
  KW_DO,
  KW_ELSE,

  KW_FINALLY,
  KW_FOR,
  KW_FUNCTION,
  KW_IF,
  KW_IN,

  KW_INSTANCEOF,
  KW_NEW,
  KW_RETURN,
  KW_SWITCH,
  KW_THIS,

  KW_THROW,
  KW_TRY,
  KW_TYPEOF,
  KW_VAR,
  KW_VOID,

  KW_WHILE,
  KW_WITH,
}
keyword;


/* Type of tokens.  */
typedef enum
{
  TOK_EOF = 0, // End of file
  TOK_NAME, // Identifier
  TOK_KEYWORD, // Keyword
  TOK_SMALL_INT,
  TOK_NUMBER,

  TOK_NULL,
  TOK_BOOL,
  TOK_NEWLINE,
  TOK_STRING,
  TOK_OPEN_BRACE, // {

  TOK_CLOSE_BRACE, // }
  TOK_OPEN_PAREN, // (
  TOK_CLOSE_PAREN, //)
  TOK_OPEN_SQUARE, // [
  TOK_CLOSE_SQUARE, // [

  TOK_DOT, // .
  TOK_SEMICOLON, // ;
  TOK_COMMA, // ,
  TOK_LESS, // <
  TOK_GREATER, // >

  TOK_LESS_EQ, // <=
  TOK_GREATER_EQ, // <=
  TOK_DOUBLE_EQ, // ==
  TOK_NOT_EQ, // !=
  TOK_TRIPLE_EQ, // ===

  TOK_NOT_DOUBLE_EQ, // !==
  TOK_PLUS, // +
  TOK_MINUS, // -
  TOK_MULT, // *
  TOK_MOD, // %

  TOK_DOUBLE_PLUS, // ++
  TOK_DOUBLE_MINUS, // --
  TOK_LSHIFT, // <<
  TOK_RSHIFT, // >>
  TOK_RSHIFT_EX, // >>>

  TOK_AND, // &
  TOK_OR, // |
  TOK_XOR, // ^
  TOK_NOT, // !
  TOK_COMPL, // ~

  TOK_DOUBLE_AND, // &&
  TOK_DOUBLE_OR, // ||
  TOK_QUERY, // ?
  TOK_COLON, // :
  TOK_EQ, // =

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

  TOK_DIV, // /
  TOK_DIV_EQ, // /=
  TOK_UNDEFINED, // undefined
  TOK_EMPTY,
}
token_type;

typedef size_t locus;

/* Represents the contents of a token.  */
typedef struct
{
  locus loc;
  token_type type;
  uint8_t uid;
}
token;

void lexer_init (const char *, size_t, bool);
void lexer_free (void);

void lexer_run_first_pass (void);

token lexer_next_token (void);
void lexer_save_token (token);
token lexer_prev_token (void);

uint8_t lexer_get_reserved_ids_count (void);

const lp_string *lexer_get_strings (void);
uint8_t lexer_get_strings_count (void);
lp_string lexer_get_string_by_id (uint8_t);

const ecma_number_t *lexer_get_nums (void);
ecma_number_t lexer_get_num_by_id (uint8_t);
uint8_t lexer_get_nums_count (void);

void lexer_adjust_num_ids (void);

void lexer_seek (locus);
void lexer_locus_to_line_and_column (locus, size_t *, size_t *);
void lexer_dump_line (size_t);
const char *lexer_keyword_to_string (keyword);
const char *lexer_token_type_to_string (token_type);

#endif

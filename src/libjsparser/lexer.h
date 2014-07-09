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

#include "mappings.h"

/* Keywords.  */
typedef enum
{
  /* Not a keyword.  */
  KW_NONE = 0,
  /* Future reserved keyword.  */
  KW_RESERVED,

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
  KW_WITH
}
keyword;

/* Type of tokens.  */
typedef enum
{
  TOK_EOF = 0x0,		// End of file
  TOK_NAME = 0x1,		// Identifier
  TOK_KEYWORD = 0x2,		// Keyword
  TOK_INT = 0x3,
  TOK_FLOAT = 0x4,
  TOK_NULL = 0x5,
  TOK_BOOL = 0x6,
  TOK_NEWLINE = 0x7,
  TOK_STRING = 0x8,

  /* Punctuators.  */
  TOK_OPEN_BRACE = 0x9,	// {
  TOK_CLOSE_BRACE = 0xa,	// }
  TOK_OPEN_PAREN = 0xb,	// (
  TOK_CLOSE_PAREN = 0xc,	// )
  TOK_OPEN_SQUARE,	// [
  TOK_CLOSE_SQUARE,	// [

  TOK_DOT,		// .
  TOK_SEMICOLON,	// ;
  TOK_COMMA,		// ,
  TOK_LESS,		// <
  TOK_GREATER,		// >
  TOK_LESS_EQ,		// <=

  TOK_GREATER_EQ,	// <=
  TOK_DOUBLE_EQ,	// ==
  TOK_NOT_EQ,		// !=
  TOK_TRIPLE_EQ,	// ===
  TOK_NOT_DOUBLE_EQ,	// !==

  TOK_PLUS,		// +
  TOK_MINUS,		// -
  TOK_MULT,		// *
  TOK_MOD,		// %
  TOK_DOUBLE_PLUS,	// ++
  TOK_DOUBLE_MINUS,	// --

  TOK_LSHIFT,		// <<
  TOK_RSHIFT,		// >>
  TOK_RSHIFT_EX,	// >>>
  TOK_AND,		// &
  TOK_OR,		// |
  TOK_XOR,		// ^

  TOK_NOT,		// !
  TOK_COMPL,		// ~
  TOK_DOUBLE_AND,	// &&
  TOK_DOUBLE_OR,	// ||
  TOK_QUERY,		// ?
  TOK_COLON,		// :

  TOK_EQ,		// =
  TOK_PLUS_EQ,		// +=
  TOK_MINUS_EQ,		// -=
  TOK_MULT_EQ,		// *=
  TOK_MOD_EQ,		// %=
  TOK_LSHIFT_EQ,	// <<=

  TOK_RSHIFT_EQ,	// >>=
  TOK_RSHIFT_EX_EQ,	// >>>=
  TOK_AND_EQ,		// &=
  TOK_OR_EQ,		// |=
  TOK_XOR_EQ,		// ^=

  TOK_DIV,		// /
  TOK_DIV_EQ,		// /=
  TOK_EMPTY
}
token_type;

/* Represents the contents of a token.  */
typedef struct 
{
  token_type type;

  union
  {
    void *none;
    keyword kw;
    const char *name;
    bool is_true;
    int num;
    float fp_num;
    const char *str;
  }
  data;
}
token;

#ifdef JERRY_NDEBUG
void lexer_set_file (FILE *);
#else
void lexer_set_source (const char *);
#endif
token lexer_next_token ();
void lexer_save_token (token);

#endif
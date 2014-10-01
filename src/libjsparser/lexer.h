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
typedef uint8_t keyword;

/* Not a keyword.  */
#define KW_NONE 0
/* Future reserved keyword.  */
#define KW_RESERVED 1
#define KW_BREAK 2
#define KW_CASE 3
#define KW_CATCH 4

#define KW_CONTINUE 5
#define KW_DEBUGGER 6
#define KW_DEFAULT 7
#define KW_DELETE 8
#define KW_DO 9

#define KW_ELSE 10
#define KW_FINALLY 11
#define KW_FOR 12
#define KW_FUNCTION 13
#define KW_IF 14

#define KW_IN 15
#define KW_INSTANCEOF 16
#define KW_NEW 17
#define KW_RETURN 18
#define KW_SWITCH 19

#define KW_THIS 20
#define KW_THROW 21
#define KW_TRY 22
#define KW_TYPEOF 23
#define KW_VAR 24

#define KW_VOID 25
#define KW_WHILE 26
#define KW_WITH 27


/* Type of tokens.  */
typedef uint8_t token_type;

#define TOK_EOF 0 // End of file
#define TOK_NAME 1 // Identifier
#define TOK_KEYWORD 2 // Keyword
#define TOK_SMALL_INT 3
#define TOK_NUMBER 4

#define TOK_NULL 5
#define TOK_BOOL 6
#define TOK_NEWLINE 7
#define TOK_STRING 8
#define TOK_OPEN_BRACE 9 // {

#define TOK_CLOSE_BRACE 10 // }
#define TOK_OPEN_PAREN 11 // (
#define TOK_CLOSE_PAREN 12 //)
#define TOK_OPEN_SQUARE 13 // [
#define TOK_CLOSE_SQUARE 14 // [

#define TOK_DOT 15 // .
#define TOK_SEMICOLON 16 // ;
#define TOK_COMMA 17 // ,
#define TOK_LESS 18 // <
#define TOK_GREATER 19 // >

#define TOK_LESS_EQ 20 // <=
#define TOK_GREATER_EQ 21 // <=
#define TOK_DOUBLE_EQ 22 // ==
#define TOK_NOT_EQ 23 // !=
#define TOK_TRIPLE_EQ 24 // ===

#define TOK_NOT_DOUBLE_EQ 25 // !==
#define TOK_PLUS 26 // +
#define TOK_MINUS 27 // -
#define TOK_MULT 28 // *
#define TOK_MOD 29 // %

#define TOK_DOUBLE_PLUS 30 // ++
#define TOK_DOUBLE_MINUS 31 // --
#define TOK_LSHIFT 32 // <<
#define TOK_RSHIFT 33 // >>
#define TOK_RSHIFT_EX 34 // >>>

#define TOK_AND 35 // &
#define TOK_OR 36 // |
#define TOK_XOR 37 // ^
#define TOK_NOT 38 // !
#define TOK_COMPL 39 // ~

#define TOK_DOUBLE_AND 40 // &&
#define TOK_DOUBLE_OR 41 // ||
#define TOK_QUERY 42 // ?
#define TOK_COLON 43 // :
#define TOK_EQ 44 // =

#define TOK_PLUS_EQ 45 // +=
#define TOK_MINUS_EQ 46 // -=
#define TOK_MULT_EQ 47 // *=
#define TOK_MOD_EQ 48 // %=
#define TOK_LSHIFT_EQ 49 // <<=

#define TOK_RSHIFT_EQ 50 // >>=
#define TOK_RSHIFT_EX_EQ 51 // >>>=
#define TOK_AND_EQ 52 // &=
#define TOK_OR_EQ 53 // |=
#define TOK_XOR_EQ 54 // ^=

#define TOK_DIV 55 // /
#define TOK_DIV_EQ 56 // /=
#define TOK_UNDEFINED 57 // undefined
#define TOK_EMPTY 58


/* Represents the contents of a token.  */
typedef struct
{
  token_type type;

  uint8_t uid;
}
__packed
token;

void lexer_init (const char *, size_t, bool);
void lexer_free (void);

void lexer_run_first_pass (void);

token lexer_next_token (void);
void lexer_save_token (token);

void lexer_dump_buffer_state (void);

uint8_t lexer_get_reserved_ids_count (void);

const lp_string *lexer_get_strings (void);
uint8_t lexer_get_strings_count (void);
lp_string lexer_get_string_by_id (uint8_t);

const ecma_number_t *lexer_get_nums (void);
ecma_number_t lexer_get_num_by_id (uint8_t);
uint8_t lexer_get_nums_count (void);

void lexer_adjust_num_ids (void);

#endif

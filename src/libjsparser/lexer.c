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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "error.h"
#include "lexer.h"

static token saved_token;

#ifdef DEBUG
FILE *lexer_debug_log;
#endif

/* If TOKEN represents a keyword, return decoded keyword,
   if TOKEN represents a Future Reserved Word, return KW_RESERVED,
   otherwise return KW_NONE.  */
static keyword
decode_keyword (const char *tok)
{
  assert (tok);

  if (!strcmp ("break", tok))
    return KW_BREAK;
  else if (!strcmp ("case", tok))
    return KW_CASE;
  else if (!strcmp ("catch", tok))
    return KW_CATCH;
  else if (!strcmp ("continue", tok))
    return KW_CONTINUE;
  else if (!strcmp ("debugger", tok))
    return KW_DEBUGGER;
  else if (!strcmp ("default", tok))
    return KW_DEFAULT;
  else if (!strcmp ("delete", tok))
    return KW_DELETE;
  else if (!strcmp ("do", tok))
    return KW_DO;
  else if (!strcmp ("else", tok))
    return KW_ELSE;
  else if (!strcmp ("finally", tok))
    return KW_FINALLY;
  else if (!strcmp ("for", tok))
    return KW_FOR;
  else if (!strcmp ("function", tok))
    return KW_FUNCTION;
  else if (!strcmp ("if", tok))
    return KW_IF;
  else if (!strcmp ("in", tok))
    return KW_IN;
  else if (!strcmp ("instanceof", tok))
    return KW_INSTANCEOF;
  else if (!strcmp ("new", tok))
    return KW_NEW;
  else if (!strcmp ("return", tok))
    return KW_RETURN;
  else if (!strcmp ("switch", tok))
    return KW_SWITCH;
  else if (!strcmp ("this", tok))
    return KW_THIS;
  else if (!strcmp ("throw", tok))
    return KW_THROW;
  else if (!strcmp ("try", tok))
    return KW_TRY;
  else if (!strcmp ("typeof", tok))
    return KW_TYPEOF;
  else if (!strcmp ("var", tok))
    return KW_VAR;
  else if (!strcmp ("void", tok))
    return KW_VOID;
  else if (!strcmp ("while", tok))
    return KW_WHILE;
  else if (!strcmp ("with", tok))
    return KW_WITH;
  else if (!strcmp ("class", tok) || !strcmp ("const", tok)
           || !strcmp ("enum", tok) || !strcmp ("export", tok)
           || !strcmp ("extends", tok) || !strcmp ("import", tok)
           || !strcmp ("super", tok) || !strcmp ("implements", tok)
           || !strcmp ("interface", tok) || !strcmp ("let", tok)
           || !strcmp ("package", tok) || !strcmp ("private", tok)
           || !strcmp ("protected", tok) || !strcmp ("public", tok)
           || !strcmp ("static", tok) || !strcmp ("yield", tok))
    return KW_RESERVED;
  else
    return KW_NONE;
}
static FILE *file;

/* Represents the contents of a file.  */
static char *buffer = NULL;
static char *buffer_start;
static char *token_start;

#define BUFFER_SIZE 1024

static char
get_char (int i)
{
  int error;
  const int tail_size = BUFFER_SIZE - (buffer - buffer_start);

  assert (file);

  if (buffer == NULL)
  {
    buffer = (char *) malloc (BUFFER_SIZE);
    error = fread (buffer, 1, BUFFER_SIZE, file);
    if (error < 0)
      fatal (ERR_IO);
    if (error == 0)
      return '\0';
    if (error < BUFFER_SIZE)
      memset (buffer + error, '\0', BUFFER_SIZE - error);
    buffer_start = buffer;
  }

  if (tail_size <= i)
  {
    /* We are almost at the end of the buffer.  */
    if (token_start)
    {
      const int token_size = buffer - token_start;
      /* Whole buffer contains single token.  */
      if (token_start == buffer_start)
        fatal (ERR_BUFFER_SIZE);
      /* Move parsed token and tail of buffer to head.  */
      memmove (buffer_start, token_start, tail_size + token_size);
      /* Adjust pointers.  */
      token_start = buffer_start;
      buffer = buffer_start + token_size;
      /* Read more characters form input file.  */
      error = fread (buffer + tail_size, 1, BUFFER_SIZE - tail_size - token_size, file);
      if (error < 0)
        fatal (ERR_IO);
      if (error == 0)
        return '\0';
      if (error < BUFFER_SIZE - tail_size - token_size)
        memset (buffer + tail_size + error, '\0',
                BUFFER_SIZE - tail_size - token_size - error);
    }
    else
    {
      memmove (buffer_start, buffer, tail_size);
      buffer = buffer_start;
      error = fread (buffer + tail_size, 1, BUFFER_SIZE - tail_size, file);
      if (error < 0)
        fatal (ERR_IO);
      if (error == 0)
        return '\0';
      if (error < BUFFER_SIZE - tail_size)
        memset (buffer + tail_size + error, '\0', BUFFER_SIZE - tail_size - error);
    }
  }

  return *(buffer + i);
}

#define LA(I) 			(get_char (I))

static inline void
new_token ()
{
  assert (buffer);
  token_start = buffer;
}

static inline void
consume_char ()
{
  assert (buffer);
  buffer++;
}

static inline const char *
current_token ()
{
  assert (buffer);
  assert (token_start);
  int length = buffer - token_start;
  char *res = (char *) malloc (length + 1);
  strncpy (res, token_start, length);
  res[length] = '\0';
  token_start = NULL;
  return res;
}

#define RETURN_PUNC_EX(TOK, NUM) \
  do \
    { \
      buffer += NUM; \
      return (token) { .type = TOK, .data.none = NULL }; \
    } \
  while (0)

#define RETURN_PUNC(TOK) RETURN_PUNC_EX(TOK, 1)

#define IF_LA_N_IS(CHAR, THEN_TOK, ELSE_TOK, NUM)	\
  do \
    { \
      if (LA (NUM) == CHAR) \
        RETURN_PUNC_EX (THEN_TOK, NUM + 1); \
      else \
        RETURN_PUNC_EX (ELSE_TOK, NUM); \
    } \
  while (0)

#define IF_LA_IS(CHAR, THEN_TOK, ELSE_TOK)	\
  IF_LA_N_IS (CHAR, THEN_TOK, ELSE_TOK, 1)

#define IF_LA_IS_OR(CHAR1, THEN1_TOK, CHAR2, THEN2_TOK, ELSE_TOK)	\
  do \
    { \
      if (LA (1) == CHAR1) \
        RETURN_PUNC_EX (THEN1_TOK, 2); \
      else if (LA (1) == CHAR2) \
        RETURN_PUNC_EX (THEN2_TOK, 2); \
      else \
        RETURN_PUNC (ELSE_TOK); \
    } \
  while (0)

static token
parse_name ()
{
  char c = LA (0);
  bool every_char_islower = isalpha (c) && islower (c);
  const char *tok = NULL;

  assert (isalpha (c) || c == '$' || c == '_');

  new_token ();
  consume_char ();
  while (true)
  {
    c = LA (0);
    if (c == '\0')
      c = c;
    if (!isalpha (c) && !isdigit (c) && c != '$' && c != '_')
      break;
    if (every_char_islower && (!isalpha (c) || !islower (c)))
      every_char_islower = false;
    consume_char ();
  }

  tok = current_token ();
  if (every_char_islower)
  {
    keyword kw = decode_keyword (tok);
    if (kw != KW_NONE)
    {
      free ((char *) tok);

      return (token)
      {
        .type = TOK_KEYWORD, .data.kw = kw
      };
    }

    if (!strcmp ("null", tok))
    {
      free ((char *) tok);

      return (token)
      {
        .type = TOK_NULL, .data.none = NULL
      };
    }

    if (!strcmp ("true", tok))
    {
      free ((char *) tok);

      return (token)
      {
        .type = TOK_BOOL, .data.is_true = true
      };
    }

    if (!strcmp ("false", tok))
    {
      free ((char *) tok);

      return (token)
      {
        .type = TOK_BOOL, .data.is_true = false
      };
    }
  }

  return (token)
  {
    .type = TOK_NAME, .data.name = tok
  };
}

static bool
is_hex_digit (char c)
{
  return isdigit (c) || c == 'a' || c == 'A' || c == 'b' || c == 'B'
          || c == 'c' || c == 'C' || c == 'd' || c == 'D'
          || c == 'e' || c == 'E' || c == 'f' || c == 'F';
}

static int
hex_to_int (char hex)
{
  switch (hex)
  {
  case '0': return 0x0;
  case '1': return 0x1;
  case '2': return 0x2;
  case '3': return 0x3;
  case '4': return 0x4;
  case '5': return 0x5;
  case '6': return 0x6;
  case '7': return 0x7;
  case '8': return 0x8;
  case '9': return 0x9;
  case 'a':
  case 'A': return 0xA;
  case 'b':
  case 'B': return 0xB;
  case 'c':
  case 'C': return 0xC;
  case 'd':
  case 'D': return 0xD;
  case 'e':
  case 'E': return 0xE;
  case 'f':
  case 'F': return 0xF;
  default: unreachable ();
  }
}

/* In this function we cannot use strtol function
   since there is no octal literals in ECMAscript.  */
static token
parse_number ()
{
  char c = LA (0);
  bool is_hex = false;
  bool is_fp = false;
  bool is_exp = false;
  const char *tok = NULL;
  int tok_length = 0;
  int res = 0;

  assert (isdigit (c) || c == '.');

  if (c == '0')
    if (LA (1) == 'x' || LA (1) == 'X')
      is_hex = true;

  if (c == '.')
  {
    assert (!isalpha (LA (1)));
    is_fp = true;
  }

  if (is_hex)
  {
    // Eat up '0x'
    consume_char ();
    consume_char ();
    new_token ();
    while (true)
    {
      c = LA (0);
      if (!is_hex_digit (c))
        break;
      consume_char ();
    }

    if (isalpha (c) || c == '_' || c == '$')
      fatal (ERR_INT_LITERAL);

    tok_length = buffer - token_start;
    tok = current_token ();
    // OK, I know that integer overflow can occur here
    for (int i = 0; i < tok_length; i++)
      res = (res << 4) + hex_to_int (tok[i]);

    free ((char *) tok);

    return (token)
    {
      .type = TOK_INT, .data.num = res
    };
  }

  assert (!is_hex && !is_exp);

  new_token ();

  // Eat up '.'
  if (is_fp)
    consume_char ();

  while (true)
  {
    c = LA (0);
    if (is_fp && c == '.')
      fatal (ERR_INT_LITERAL);
    if (is_exp && (c == 'e' || c == 'E'))
      fatal (ERR_INT_LITERAL);

    if (c == '.')
    {
      if (isalpha (LA (1)) || LA (1) == '_' || LA (1) == '$')
        fatal (ERR_INT_LITERAL);
      is_fp = true;
      consume_char ();
      continue;
    }

    if (c == 'e' || c == 'E')
    {
      if (LA (1) == '-' || LA (1) == '+')
        consume_char ();
      if (!isdigit (LA (1)))
        fatal (ERR_INT_LITERAL);
      is_exp = true;
      consume_char ();
      continue;
    }

    if (isalpha (c) || c == '_' || c == '$')
      fatal (ERR_INT_LITERAL);

    if (!isdigit (c))
      break;

    consume_char ();
  }

  if (is_fp || is_exp)
  {
    tok = current_token ();
    float res = strtof (tok, NULL);
    free ((char *) tok);

    return (token)
    {
      .type = TOK_FLOAT, .data.fp_num = res
    };
  }

  tok_length = buffer - token_start;
  tok = current_token ();

  for (int i = 0; i < tok_length; i++)
    res = res * 10 + hex_to_int (tok[i]);

  free ((char *) tok);

  return (token)
  {
    .type = TOK_INT, .data.num = res
  };
}

static char
escape_char (char c)
{
  switch (c)
  {
  case 'b': return '\b';
  case 'f': return '\f';
  case 'n': return '\n';
  case 'r': return '\r';
  case 't': return '\t';
  case 'v': return '\v';
  case '\'':
  case '"':
  case '\\':
  default: return c;
  }
}

static token
parse_string ()
{
  char c = LA (0);
  bool is_double_quoted;
  char *tok = NULL;
  char *index = NULL;
  int length;

  assert (c == '\'' || c == '"');

  is_double_quoted = (c == '"');

  // Eat up '"'
  consume_char ();
  new_token ();

  while (true)
  {
    c = LA (0);
    if (c == '\0')
      fatal (ERR_UNCLOSED);
    if (c == '\n')
      fatal (ERR_STRING);
    if (c == '\\')
    {
      /* Only single escape character is allowed.  */
      if (LA (1) == 'x' || LA (1) == 'u' || isdigit (LA (1)))
        fatal (ERR_STRING);
      if ((LA (1) == '\'' && !is_double_quoted)
          || (LA (1) == '"' && is_double_quoted)
          || LA (1) == '\n')
      {
        consume_char ();
        consume_char ();
        continue;
      }
    }
    else if ((c == '\'' && !is_double_quoted)
             || (c == '"' && is_double_quoted))
      break;

    consume_char ();
  }

  length = buffer - token_start;
  tok = (char *) malloc (length);
  index = tok;

  for (char *i = token_start; i < buffer; i++)
  {
    if (*i == '\\')
    {
      if (*(i + 1) == '\n')
      {
        i++;
        continue;
      }
      *index = escape_char (*(i + 1));
      index++;
      i++;
      continue;
    }

    *index = *i;
    index++;
  }

  memset (index, '\0', length - (index - tok));

  token_start = NULL;
  // Eat up '"'
  consume_char ();

  return (token)
  {
    .type = TOK_STRING, .data.str = tok
  };
}

static void
grobble_whitespaces ()
{
  char c = LA (0);

  while ((isspace (c) && c != '\n') || c == '\0')
  {
    consume_char ();
    c = LA (0);
  }
}

void
lexer_set_file (FILE *ex_file)
{
  assert (ex_file);
  file = ex_file;
#ifdef DEBUG
  lexer_debug_log = fopen ("lexer.log", "w");
#endif
}

static bool
replace_comment_by_newline ()
{
  char c = LA (0);
  bool multiline;
  bool was_newlines = false;

  assert (LA (0) == '/');
  assert (LA (1) == '/' || LA (1) == '*');

  multiline = (LA (1) == '*');

  consume_char ();
  consume_char ();

  while (true)
  {
    c = LA (0);
    if (!multiline && (c == '\n' || c == '\0'))
      return false;
    if (multiline && c == '*' && LA (1) == '/')
    {
      consume_char ();
      consume_char ();
      if (was_newlines)
        return true;
      else
        return false;
    }
    if (multiline && c == '\n')
      was_newlines = true;
    if (multiline && c == '\0')
      fatal (ERR_UNCLOSED);
    consume_char ();
  }
}

token
#ifdef DEBUG
lexer_next_token_private ()
#else
lexer_next_token ()
#endif
{
  char c = LA (0);

  if (saved_token.type != TOK_EOF)
  {
    token res = saved_token;
    saved_token.type = TOK_EOF;
    return res;
  }

  assert (token_start == NULL);

  if (isalpha (c) || c == '$' || c == '_')
    return parse_name ();

  if (isdigit (c) || (c == '.' && isdigit (LA (1))))
    return parse_number ();

  if (c == '\n')
  {
    consume_char ();

    return (token)
    {
      .type = TOK_NEWLINE, .data.none = NULL
    };
  }

  if (c == '\0')
    return (token)
  {
    .type = TOK_EOF, .data.none = NULL
  };

  if (c == '\'' || c == '"')
    return parse_string ();

  if (isspace (c))
  {
    grobble_whitespaces ();
    return lexer_next_token ();
  }

  if (c == '/' && LA (1) == '*')
  {
    if (replace_comment_by_newline ())
      return (token)
    {
      .type = TOK_NEWLINE, .data.none = NULL
    };
    else
      return lexer_next_token ();
  }

  if (c == '/' && LA (1) == '/')
  {
    replace_comment_by_newline ();
    return lexer_next_token ();
  }

  switch (c)
  {
  case '{': RETURN_PUNC (TOK_OPEN_BRACE);
  case '}': RETURN_PUNC (TOK_CLOSE_BRACE);
  case '(': RETURN_PUNC (TOK_OPEN_PAREN);
  case ')': RETURN_PUNC (TOK_CLOSE_PAREN);
  case '[': RETURN_PUNC (TOK_OPEN_SQUARE);
  case ']': RETURN_PUNC (TOK_CLOSE_SQUARE);
  case '.': RETURN_PUNC (TOK_DOT);
  case ';': RETURN_PUNC (TOK_SEMICOLON);
  case ',': RETURN_PUNC (TOK_COMMA);
  case '~': RETURN_PUNC (TOK_COMPL);
  case ':': RETURN_PUNC (TOK_COLON);
  case '?': RETURN_PUNC (TOK_QUERY);

  case '*': IF_LA_IS ('=', TOK_MULT_EQ, TOK_MULT);
  case '/': IF_LA_IS ('=', TOK_DIV_EQ, TOK_DIV);
  case '^': IF_LA_IS ('=', TOK_XOR_EQ, TOK_XOR);
  case '%': IF_LA_IS ('=', TOK_MOD_EQ, TOK_MOD);

  case '+': IF_LA_IS_OR ('+', TOK_DOUBLE_PLUS, '=', TOK_PLUS_EQ, TOK_PLUS);
  case '-': IF_LA_IS_OR ('-', TOK_DOUBLE_MINUS, '=', TOK_MINUS_EQ, TOK_MINUS);
  case '&': IF_LA_IS_OR ('&', TOK_DOUBLE_AND, '=', TOK_AND_EQ, TOK_AND);
  case '|': IF_LA_IS_OR ('|', TOK_DOUBLE_OR, '=', TOK_OR_EQ, TOK_OR);

  case '<':
    switch (LA (1))
    {
    case '<': IF_LA_N_IS ('=', TOK_LSHIFT_EQ, TOK_LSHIFT, 2);
    case '=': RETURN_PUNC_EX (TOK_LESS_EQ, 2);
    default: RETURN_PUNC (TOK_LESS);
    }

  case '>':
    switch (LA (1))
    {
    case '>':
      switch (LA (2))
      {
      case '>': IF_LA_N_IS ('=', TOK_RSHIFT_EX_EQ, TOK_RSHIFT_EX, 3);
      case '=': RETURN_PUNC_EX (TOK_RSHIFT_EQ, 3);
      default: RETURN_PUNC_EX (TOK_RSHIFT, 2);
      }
    case '=': RETURN_PUNC_EX (TOK_GREATER_EQ, 2);
    default: RETURN_PUNC (TOK_GREATER);
    }

  case '=':
    if (LA (1) == '=')
      IF_LA_N_IS ('=', TOK_TRIPLE_EQ, TOK_DOUBLE_EQ, 2);
    else
      RETURN_PUNC (TOK_EQ);

  case '!':
    if (LA (1) == '=')
      IF_LA_N_IS ('=', TOK_NOT_DOUBLE_EQ, TOK_NOT_EQ, 2);
    else
      RETURN_PUNC (TOK_NOT);

  default:
    unreachable ();
  }
  fatal (ERR_NON_CHAR);
}

#ifdef DEBUG
static int i = 0;

token
lexer_next_token ()
{
  token tok = lexer_next_token_private ();
  if (tok.type == TOK_NEWLINE)
    return tok;
  if (tok.type == TOK_CLOSE_BRACE)
  {
    if (i == 300)
      fprintf (lexer_debug_log, "lexer_next_token(%d): type=0x%x, data=%p\n", i, tok.type, tok.data.none);
    i++;
  }
  return tok;
}
#endif

void
lexer_save_token (token tok)
{
#ifdef DEBUG
  if (tok.type == TOK_CLOSE_BRACE)
    fprintf (lexer_debug_log, "lexer_save_token(%d): type=0x%x, data=%p\n", i, tok.type, tok.data.none);
#endif
  saved_token = tok;
}

void
lexer_dump_buffer_state ()
{
  //printf ("%s\n", buffer);
}
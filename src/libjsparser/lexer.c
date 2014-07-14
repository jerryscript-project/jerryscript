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

#include "allocator.h"
#include "globals.h"
#include "jerry-libc.h"
#include "lexer.h"
#include "parser.h"

static token saved_token;
static token empty_token = { .type = TOK_EMPTY, .data.none = NULL };

typedef struct
{
  const char *str;
  token tok;
}
string_and_token;

static string_and_token keyword_tokens[] = 
{
  { .str = "break", .tok = { .type = TOK_KEYWORD, .data.kw = KW_BREAK } },
  { .str = "case", .tok = { .type = TOK_KEYWORD, .data.kw = KW_CASE } },
  { .str = "catch", .tok = { .type = TOK_KEYWORD, .data.kw = KW_CATCH } },
  { .str = "class", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "const", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "continue", .tok = { .type = TOK_KEYWORD, .data.kw = KW_CONTINUE } },
  { .str = "debugger", .tok = { .type = TOK_KEYWORD, .data.kw = KW_DEBUGGER } },
  { .str = "default", .tok = { .type = TOK_KEYWORD, .data.kw = KW_DEFAULT } },
  { .str = "delete", .tok = { .type = TOK_KEYWORD, .data.kw = KW_DELETE } },
  { .str = "do", .tok = { .type = TOK_KEYWORD, .data.kw = KW_DO } },
  { .str = "else", .tok = { .type = TOK_KEYWORD, .data.kw = KW_ELSE } },
  { .str = "enum", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "export", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "extends", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "false", .tok = { .type = TOK_BOOL, .data.is_true = false } },
  { .str = "finally", .tok = { .type = TOK_KEYWORD, .data.kw = KW_FINALLY } },
  { .str = "for", .tok = { .type = TOK_KEYWORD, .data.kw = KW_FOR } },
  { .str = "function", .tok = { .type = TOK_KEYWORD, .data.kw = KW_FUNCTION } },
  { .str = "if", .tok = { .type = TOK_KEYWORD, .data.kw = KW_IF } },
  { .str = "instanceof", .tok = { .type = TOK_KEYWORD, .data.kw = KW_INSTANCEOF } },
  { .str = "interface", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "in", .tok = { .type = TOK_KEYWORD, .data.kw = KW_IN } },
  { .str = "import", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "implements", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "let", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "new", .tok = { .type = TOK_KEYWORD, .data.kw = KW_NEW } },
  { .str = "null", .tok = { .type = TOK_NULL, .data.none = NULL } },
  { .str = "package", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "private", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "protected", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "public", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "return", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RETURN } },
  { .str = "static", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "super", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } },
  { .str = "switch", .tok = { .type = TOK_KEYWORD, .data.kw = KW_SWITCH } },
  { .str = "this", .tok = { .type = TOK_KEYWORD, .data.kw = KW_THIS } },
  { .str = "throw", .tok = { .type = TOK_KEYWORD, .data.kw = KW_THROW } },
  { .str = "true", .tok = { .type = TOK_BOOL, .data.is_true = true } },
  { .str = "try", .tok = { .type = TOK_KEYWORD, .data.kw = KW_TRY } },
  { .str = "typeof", .tok = { .type = TOK_KEYWORD, .data.kw = KW_TYPEOF } },
  { .str = "var", .tok = { .type = TOK_KEYWORD, .data.kw = KW_VAR } },
  { .str = "void", .tok = { .type = TOK_KEYWORD, .data.kw = KW_VOID } },
  { .str = "while", .tok = { .type = TOK_KEYWORD, .data.kw = KW_WHILE } },
  { .str = "with", .tok = { .type = TOK_KEYWORD, .data.kw = KW_WITH } },
  { .str = "yield", .tok = { .type = TOK_KEYWORD, .data.kw = KW_RESERVED } }
};

#define MAX_NAMES 100

static string_and_token seen_names[MAX_NAMES];
static size_t seen_names_num;

static bool
is_empty (token tok)
{
  return tok.type == TOK_EMPTY;
}

#ifdef __HOST
FILE *lexer_debug_log;
#endif

#ifdef __HOST
static FILE *file;
static char *buffer_start;

/* Represents the contents of a file.  */
static char *buffer = NULL;
static char *token_start;

#define BUFFER_SIZE 1024

static char
get_char (size_t i)
{
  size_t error;
  JERRY_ASSERT (buffer >= buffer_start);
  const size_t tail_size = BUFFER_SIZE - (size_t) (buffer - buffer_start);

  JERRY_ASSERT (file);

  if (buffer == NULL)
    {
      buffer = (char *) malloc (BUFFER_SIZE);
      error = __fread (buffer, 1, BUFFER_SIZE, file);
      if (error == 0)
        return '\0';
      if (error < BUFFER_SIZE)
        __memset (buffer + error, '\0', BUFFER_SIZE - error);
      buffer_start = buffer;
    }

  if (tail_size <= i)
    {
    /* We are almost at the end of the buffer.  */
      if (token_start)
        {
          JERRY_ASSERT (buffer >= token_start);
          const size_t token_size = (size_t) (buffer - token_start);
          /* Whole buffer contains single token.  */
          if (token_start == buffer_start)
            parser_fatal (ERR_BUFFER_SIZE);
          /* Move parsed token and tail of buffer to head.  */
          __memmove (buffer_start, token_start, tail_size + token_size);
          /* Adjust pointers.  */
          token_start = buffer_start;
          buffer = buffer_start + token_size;
          /* Read more characters form input file.  */
          error = __fread (buffer + tail_size, 1, BUFFER_SIZE - tail_size - token_size, file);
          if (error == 0)
            return '\0';
          if (error < BUFFER_SIZE - tail_size - token_size)
            __memset (buffer + tail_size + error, '\0',
                    BUFFER_SIZE - tail_size - token_size - error);
        }
      else
        {
          __memmove (buffer_start, buffer, tail_size);
          buffer = buffer_start;
          error = __fread (buffer + tail_size, 1, BUFFER_SIZE - tail_size, file);
          if (error == 0)
            return '\0';
          if (error < BUFFER_SIZE - tail_size)
            __memset (buffer + tail_size + error, '\0', BUFFER_SIZE - tail_size - error);
        }
    }

  return *(buffer + i);
}

#define LA(I) 			(get_char (I))

#else

/* Represents the contents of a file.  */
static const char *buffer = NULL;
static const char *token_start;

#define LA(I)       (*(buffer + I))

#endif // __HOST

/* If TOKEN represents a keyword, return decoded keyword,
   if TOKEN represents a Future Reserved Word, return KW_RESERVED,
   otherwise return KW_NONE.  */
static token
decode_keyword (void)
{
  size_t size = sizeof (keyword_tokens) / sizeof (string_and_token);
  size_t i;

  for (i = 0; i < size; i++)
    {
      if (!__strncmp (keyword_tokens[i].str, token_start, __strlen (keyword_tokens[i].str)))
        return keyword_tokens[i].tok;
    }

  return empty_token;
}

static token
convert_seen_name_to_token (void)
{
  size_t i;

  for (i = 0; i < seen_names_num; i++)
    {
      if (!__strncmp (seen_names[i].str, token_start, __strlen (seen_names[i].str)))
        return seen_names[i].tok;
    }

  return empty_token;
}

static void
add_to_seen_tokens (string_and_token snt)
{
  JERRY_ASSERT (seen_names_num < MAX_NAMES);

  seen_names[seen_names_num++] = snt;
}

static void
new_token (void)
{
  JERRY_ASSERT (buffer);
  token_start = buffer;
}

static void
consume_char (void)
{
  JERRY_ASSERT (buffer);
  buffer++;
}

static const char *
current_token (void)
{
  JERRY_ASSERT (buffer);
  JERRY_ASSERT (token_start);
  JERRY_ASSERT (token_start <= buffer);
  size_t length = (size_t) (buffer - token_start);
  char *res = (char *) malloc (length + 1);
  __strncpy (res, token_start, length);
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
parse_name (void)
{
  char c = LA (0);
  bool every_char_islower = __islower (c);
  const char *string = NULL;
  token known_token = empty_token;

  JERRY_ASSERT (__isalpha (c) || c == '$' || c == '_');

  new_token ();
  consume_char ();
  while (true)
    {
      c = LA (0);
      if (c == '\0')
        c = c;
      if (!__isalpha (c) && !__isdigit (c) && c != '$' && c != '_')
        break;
      if (every_char_islower && (!__islower (c)))
        every_char_islower = false;
      consume_char ();
    }

  if (every_char_islower)
    {
      known_token = decode_keyword ();
      if (!is_empty (known_token))
        {
          token_start = NULL;
          return known_token;
        }
    }

  known_token = convert_seen_name_to_token ();
  if (!is_empty (known_token))
    {
      token_start = NULL;
      return known_token;
    }

  string = current_token ();
  known_token = (token) { .type = TOK_NAME, .data.name = string };
  
  add_to_seen_tokens ((string_and_token) { .str = string, .tok = known_token });

  return known_token;
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
    default: JERRY_UNREACHABLE ();
  }
}

/* In this function we cannot use strtol function
   since there is no octal literals in ECMAscript.  */
static token
parse_number (void)
{
  char c = LA (0);
  bool is_hex = false;
  bool is_fp = false;
  bool is_exp = false;
  size_t tok_length = 0, i;
  int res = 0;

  JERRY_ASSERT (__isdigit (c) || c == '.');

  if (c == '0')
    if (LA (1) == 'x' || LA (1) == 'X')
      is_hex = true;

  if (c == '.')
    {
      JERRY_ASSERT (!__isalpha (LA (1)));
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
          if (!__isxdigit (c))
            break;
          consume_char ();
        }

      if (__isalpha (c) || c == '_' || c == '$')
        parser_fatal (ERR_INT_LITERAL);

      tok_length = (size_t) (buffer - token_start);
      // OK, I know that integer overflow can occur here
      for (i = 0; i < tok_length; i++)
        res = (res << 4) + hex_to_int (token_start[i]);

      token_start = NULL;
      return (token) { .type = TOK_INT, .data.num = res };
    }

  JERRY_ASSERT (!is_hex && !is_exp);

  new_token ();

  // Eat up '.'
  if (is_fp)
    consume_char ();

  while (true)
    {
      c = LA (0);
      if (is_fp && c == '.')
        parser_fatal (ERR_INT_LITERAL);
      if (is_exp && (c == 'e' || c == 'E'))
        parser_fatal (ERR_INT_LITERAL);

      if (c == '.')
        {
          if (__isalpha (LA (1)) || LA (1) == '_' || LA (1) == '$')
            parser_fatal (ERR_INT_LITERAL);
          is_fp = true;
          consume_char ();
          continue;
        }

      if (c == 'e' || c == 'E')
        {
          if (LA (1) == '-' || LA (1) == '+')
            consume_char ();
          if (!__isdigit (LA (1)))
            parser_fatal (ERR_INT_LITERAL);
          is_exp = true;
          consume_char ();
          continue;
        }

      if (__isalpha (c) || c == '_' || c == '$')
        parser_fatal (ERR_INT_LITERAL);

      if (!__isdigit (c))
        break;

      consume_char ();
    }

  if (is_fp || is_exp)
    {
      float res = __strtof (token_start, NULL);
      token_start = NULL;
      return (token) { .type = TOK_FLOAT, .data.fp_num = res };
    }

  tok_length = (size_t) (buffer - token_start);;
  for (i = 0; i < tok_length; i++)
    res = res * 10 + hex_to_int (token_start[i]);

  token_start = NULL;

  return (token) { .type = TOK_INT, .data.num = res };
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
parse_string (void)
{
  char c = LA (0);
  bool is_double_quoted;
  char *tok = NULL;
  char *index = NULL;
  const char *i;
  size_t length;
  token res = empty_token;

  JERRY_ASSERT (c == '\'' || c == '"');

  is_double_quoted = (c == '"');

  // Eat up '"'
  consume_char ();
  new_token ();

  while (true)
    {
      c = LA (0);
      if (c == '\0')
        parser_fatal (ERR_UNCLOSED);
      if (c == '\n')
        parser_fatal (ERR_STRING);
      if (c == '\\')
        {
          /* Only single escape character is allowed.  */
          if (LA (1) == 'x' || LA (1) == 'u' || __isdigit (LA (1)))
            parser_fatal (ERR_STRING);
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

  length = (size_t) (buffer - token_start);
  tok = (char *) malloc (length);
  index = tok;

  for (i = token_start; i < buffer; i++)
    {
      if (*i == '\\')
        {
          if (*(i+1) == '\n')
            {
              i++;
              continue;
            }
          *index = escape_char (*(i+1));
          index++;
          i++;
          continue;
        }

      *index = *i;
      index++;
    }

  __memset (index, '\0', length - (size_t) (index - tok));

  token_start = NULL;
  // Eat up '"'
  consume_char ();

  res = (token) { .type = TOK_STRING, .data.str = tok };

  add_to_seen_tokens ((string_and_token) { .str = tok, .tok = res });

  return res;
}

static void
grobble_whitespaces (void)
{
  char c = LA (0);

  while ((__isspace (c) && c != '\n') || c == '\0')
    {
      consume_char ();
      c = LA (0);
    }
}

#ifdef __HOST
void
lexer_set_file (FILE *ex_file)
{
  JERRY_ASSERT (ex_file);
  file = ex_file;
  lexer_debug_log = __fopen ("lexer.log", "w");
  saved_token = empty_token;
}
#else
void
lexer_set_source (const char * source)
{
  buffer = source;
  saved_token = empty_token;
}

#endif

static bool
replace_comment_by_newline (void)
{
  char c = LA (0);
  bool multiline;
  bool was_newlines = false;

  JERRY_ASSERT (LA (0) == '/');
  JERRY_ASSERT (LA (1) == '/' || LA (1) == '*');

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
        parser_fatal (ERR_UNCLOSED);
      consume_char ();
    }
}

#ifdef __HOST
static token
lexer_next_token_private (void)
#else
token
lexer_next_token (void)
#endif
{
  char c = LA (0);

  if (!is_empty (saved_token))
    {
      token res = saved_token;
      saved_token = empty_token;
      return res;
    }

  JERRY_ASSERT (token_start == NULL);

  if (__isalpha (c) || c == '$' || c == '_')
    return parse_name ();

  if (__isdigit (c) || (c == '.' && __isdigit (LA (1))))
    return parse_number ();

  if (c == '\n')
    {
      consume_char ();
      return (token) { .type = TOK_NEWLINE, .data.none = NULL };
    }

  if (c == '\0')
    return (token) { .type = TOK_EOF, .data.none = NULL };;

  if (c == '\'' || c == '"')
    return parse_string ();

  if (__isspace (c))
    {
      grobble_whitespaces ();
      return 
#ifdef __HOST
        lexer_next_token_private ();
#else
        lexer_next_token ();
#endif
    }

  if (c == '/' && LA (1) == '*')
    {
      if (replace_comment_by_newline ())
        return (token) { .type = TOK_NEWLINE, .data.none = NULL };
      else
        return 
#ifdef __HOST
          lexer_next_token_private ();
#else
          lexer_next_token ();
#endif
    }

  if (c == '/' && LA (1) == '/')
    {
      replace_comment_by_newline ();;
      return 
#ifdef __HOST
        lexer_next_token_private ();
#else
        lexer_next_token ();
#endif
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
      JERRY_UNREACHABLE ();
  }
  parser_fatal (ERR_NON_CHAR);
}

#ifdef __HOST
static int i = 0;

token
lexer_next_token (void)
{
  token tok = lexer_next_token_private ();
  if (tok.type == TOK_NEWLINE)
    return tok;
  // if (tok.type == TOK_CLOSE_BRACE)
    {
      // if (i == 300)
        __fprintf (lexer_debug_log, "lexer_next_token(%d): type=0x%x, data=%p\n", i, tok.type, tok.data.none);
      i++;
    }
  return tok;
}
#endif

void
lexer_save_token (token tok)
{
  #ifdef __HOST
  // if (tok.type == TOK_CLOSE_BRACE)
    __fprintf (lexer_debug_log, "lexer_save_token(%d): type=0x%x, data=%p\n", i, tok.type, tok.data.none);
  #endif
  saved_token = tok;
}

void
lexer_dump_buffer_state (void)
{
  __printf ("%s\n", buffer);
}

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

#include "mem-allocator.h"
#include "jerry-libc.h"
#include "lexer.h"
#include "parser.h"
#include "stack.h"
#include "opcodes.h"
#include "parse-error.h"

static token saved_token, prev_token, sent_token;
static token empty_token =
{
  .type = TOK_EMPTY,
  .uid = 0,
  .locus = 0
};

static bool allow_dump_lines = false;
static size_t buffer_size = 0;

/* Represents the contents of a script.  */
static const char *buffer_start = NULL;
static const char *buffer = NULL;
static const char *token_start;

#define LA(I)       (get_char (I))

enum
{
  strings_global_size
};
STATIC_STACK (strings, uint8_t, lp_string)

enum
{
  numbers_global_size
};
STATIC_STACK (numbers, uint8_t, ecma_number_t)

enum
{
  num_ids_global_size
};
STATIC_STACK (num_ids, uint8_t, idx_t)

static bool
is_empty (token tok)
{
  return tok.type == TOK_EMPTY;
}

static char
get_char (size_t i)
{
  if ((buffer + i) >= (buffer_start + buffer_size))
  {
    return '\0';
  }
  return *(buffer + i);
}

static void
dump_current_line (void)
{
  const char *i;

  if (!allow_dump_lines)
  {
    return;
  }

  __printf ("// ");

  for (i = buffer; *i != '\n' && *i != 0; i++)
  {
    __putchar (*i);
  }
  __putchar ('\n');
}

static token
create_token (token_type type, size_t loc __unused, uint8_t uid)
{
  return (token)
  {
    .type = type,
    .locus = loc,
    .uid = uid
  };
}

static token
create_token_from_current_token (token_type type, uint8_t uid)
{
  return create_token (type, (size_t) (token_start - buffer_start), uid);
}

static token
create_token_from_buffer_state (token_type type, uint8_t uid)
{
  return create_token (type, (size_t) (buffer - buffer_start), uid);
}

static bool
current_token_equals_to (const char *str)
{
  if (__strlen (str) != (size_t) (buffer - token_start))
  {
    return false;
  }
  if (!__strncmp (str, token_start, (size_t) (buffer - token_start)))
  {
    return true;
  }
  return false;
}

static bool
current_token_equals_to_lp (lp_string str)
{
  if (str.length != (ecma_length_t) (buffer - token_start))
  {
    return false;
  }
  if (!__strncmp ((const char *) str.str, token_start, str.length))
  {
    return true;
  }
  return false;
}

/* If TOKEN represents a keyword, return decoded keyword,
   if TOKEN represents a Future Reserved Word, return KW_RESERVED,
   otherwise return KW_NONE.  */
static token
decode_keyword (void)
{
  if (current_token_equals_to ("break"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_BREAK);
  }
  if (current_token_equals_to ("case"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_CASE);
  }
  if (current_token_equals_to ("catch"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_CATCH);
  }
  if (current_token_equals_to ("class"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("const"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("continue"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_CONTINUE);
  }
  if (current_token_equals_to ("debugger"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_DEBUGGER);
  }
  if (current_token_equals_to ("default"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_DEFAULT);
  }
  if (current_token_equals_to ("delete"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_DELETE);
  }
  if (current_token_equals_to ("do"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_DO);
  }
  if (current_token_equals_to ("else"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_ELSE);
  }
  if (current_token_equals_to ("enum"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("export"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("extends"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("false"))
  {
    return create_token_from_current_token (TOK_BOOL, false);
  }
  if (current_token_equals_to ("finally"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_FINALLY);
  }
  if (current_token_equals_to ("for"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_FOR);
  }
  if (current_token_equals_to ("function"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_FUNCTION);
  }
  if (current_token_equals_to ("if"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_IF);
  }
  if (current_token_equals_to ("instanceof"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_INSTANCEOF);
  }
  if (current_token_equals_to ("interface"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("in"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_IN);
  }
  if (current_token_equals_to ("import"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("implements"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("let"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("new"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_NEW);
  }
  if (current_token_equals_to ("null"))
  {
    return create_token_from_current_token (TOK_NULL, 0);
  }
  if (current_token_equals_to ("package"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("private"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("protected"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("public"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("return"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RETURN);
  }
  if (current_token_equals_to ("static"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("super"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("switch"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_SWITCH);
  }
  if (current_token_equals_to ("this"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_THIS);
  }
  if (current_token_equals_to ("throw"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_THROW);
  }
  if (current_token_equals_to ("true"))
  {
    return create_token_from_current_token (TOK_BOOL, true);
  }
  if (current_token_equals_to ("try"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_TRY);
  }
  if (current_token_equals_to ("typeof"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_TYPEOF);
  }
  if (current_token_equals_to ("var"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_VAR);
  }
  if (current_token_equals_to ("void"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_VOID);
  }
  if (current_token_equals_to ("while"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_WHILE);
  }
  if (current_token_equals_to ("with"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_WITH);
  }
  if (current_token_equals_to ("yield"))
  {
    return create_token_from_current_token (TOK_KEYWORD, KW_RESERVED);
  }
  if (current_token_equals_to ("undefined"))
  {
    return create_token_from_current_token (TOK_UNDEFINED, 0);
  }
  return empty_token;
}

static token
convert_current_token_to_token (token_type tt)
{
  JERRY_ASSERT (token_start);

  for (uint8_t i = 0; i < STACK_SIZE (strings); i++)
  {
    if (current_token_equals_to_lp (STACK_ELEMENT (strings, i)))
    {
      return create_token_from_current_token (tt, i);
    }
  }

  const lp_string str = (lp_string)
  {
    .length = (uint8_t) (buffer - token_start),
    .str = (const ecma_char_t *) token_start
  };

  STACK_PUSH (strings, str);

  return create_token_from_current_token (tt, (idx_t) (STACK_SIZE (strings) - 1));
}

static token
convert_seen_num_to_token (ecma_number_t num)
{
  uint8_t num_id;

  JERRY_ASSERT (STACK_SIZE (num_ids) == STACK_SIZE (numbers));
  for (uint8_t i = 0; i < STACK_SIZE (numbers); i++)
  {
    if (STACK_ELEMENT (numbers, i) == num)
    {
      return create_token_from_current_token (TOK_NUMBER, STACK_ELEMENT (num_ids, i));
    }
  }

  num_id = STACK_SIZE (num_ids);
  STACK_PUSH (num_ids, num_id);
  STACK_PUSH (numbers, num);

  return create_token_from_current_token (TOK_NUMBER, num_id);
}

const lp_string *
lexer_get_strings (void)
{
  lp_string *data = NULL;
  STACK_CONVERT_TO_RAW_DATA (strings, data);
  return data;
}

uint8_t
lexer_get_strings_count (void)
{
  return STACK_SIZE (strings);
}

uint8_t
lexer_get_reserved_ids_count (void)
{
  return (uint8_t) (STACK_SIZE (strings) + STACK_SIZE (numbers));
}

lp_string
lexer_get_string_by_id (uint8_t id)
{
  JERRY_ASSERT (id < STACK_SIZE (strings));
  return STACK_ELEMENT (strings, id);
}

const ecma_number_t *
lexer_get_nums (void)
{
  ecma_number_t *data = NULL;
  STACK_CONVERT_TO_RAW_DATA (numbers, data);
  return data;
}

uint8_t
lexer_get_nums_count (void)
{
  return STACK_SIZE (numbers);
}

void
lexer_adjust_num_ids (void)
{
  JERRY_ASSERT (STACK_SIZE (numbers) == STACK_SIZE (num_ids));
  for (uint8_t i = 0; i < STACK_SIZE (numbers); i++)
  {
    STACK_SET_ELEMENT (num_ids, i, (uint8_t) (STACK_ELEMENT (num_ids, i) + STACK_SIZE (strings)));
  }
}

ecma_number_t
lexer_get_num_by_id (uint8_t id)
{
  JERRY_ASSERT (id >= lexer_get_strings_count () && id < lexer_get_reserved_ids_count ());
  JERRY_ASSERT (STACK_ELEMENT (num_ids, id - lexer_get_strings_count ()) == id);
  return STACK_ELEMENT (numbers, id - lexer_get_strings_count ());
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

#define RETURN_PUNC_EX(TOK, NUM) \
  do \
  { \
    buffer += NUM; \
    return create_token_from_buffer_state (TOK, 0); \
  } \
  while (0)

#define RETURN_PUNC(TOK) RETURN_PUNC_EX(TOK, 1)

#define IF_LA_N_IS(CHAR, THEN_TOK, ELSE_TOK, NUM) \
  do \
  { \
    if (LA (NUM) == CHAR) \
    { \
      RETURN_PUNC_EX (THEN_TOK, NUM + 1); \
    } \
    else \
    { \
      RETURN_PUNC_EX (ELSE_TOK, NUM); \
    } \
  } \
  while (0)

#define IF_LA_IS(CHAR, THEN_TOK, ELSE_TOK) \
  IF_LA_N_IS (CHAR, THEN_TOK, ELSE_TOK, 1)

#define IF_LA_IS_OR(CHAR1, THEN1_TOK, CHAR2, THEN2_TOK, ELSE_TOK) \
  do \
  { \
    if (LA (1) == CHAR1) \
    { \
      RETURN_PUNC_EX (THEN1_TOK, 2); \
    } \
    else if (LA (1) == CHAR2) \
    { \
      RETURN_PUNC_EX (THEN2_TOK, 2); \
    } \
    else \
    { \
      RETURN_PUNC (ELSE_TOK); \
    } \
  } \
  while (0)

static token
parse_name (void)
{
  char c = LA (0);
  bool every_char_islower = __islower (c);
  token known_token = empty_token;

  JERRY_ASSERT (__isalpha (c) || c == '$' || c == '_');

  new_token ();
  consume_char ();
  while (true)
  {
    c = LA (0);
    if (c == '\0')
    {
      break;
    }
    if (!__isalpha (c) && !__isdigit (c) && c != '$' && c != '_')
    {
      break;
    }
    if (every_char_islower && (!__islower (c)))
    {
      every_char_islower = false;
    }
    consume_char ();
  }

  if (every_char_islower)
  {
    known_token = decode_keyword ();
    if (!is_empty (known_token))
    {
      goto end;
    }
  }

  known_token = convert_current_token_to_token (TOK_NAME);

end:
  token_start = NULL;
  return known_token;
}

static uint32_t
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
  uint32_t res = 0;
  token known_token;

  JERRY_ASSERT (__isdigit (c) || c == '.');

  if (c == '0')
  {
    if (LA (1) == 'x' || LA (1) == 'X')
    {
      is_hex = true;
    }
  }

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
      {
        break;
      }
      consume_char ();
    }

    if (__isalpha (c) || c == '_' || c == '$')
    {
      PARSE_ERROR ("Integer literal shall not contain non-digit characters", buffer - buffer_start);
    }

    tok_length = (size_t) (buffer - token_start);

    for (i = 0; i < tok_length; i++)
    {
#ifndef JERRY_NDEBUG
      uint32_t old_res = res;
#endif
      res = (res << 4) + hex_to_int (token_start[i]);
      FIXME (Replace with conversion to ecma_number_t)
#ifndef JERRY_NDEBUG
      JERRY_ASSERT (old_res <= res);
#endif
    }

    if (res <= 255)
    {
      known_token = create_token_from_current_token (TOK_SMALL_INT, (uint8_t) res);
      token_start = NULL;
      return known_token;
    }

    known_token = convert_seen_num_to_token ((ecma_number_t) res);
    JERRY_ASSERT (!is_empty (known_token));

    token_start = NULL;

    return known_token;
  }

  JERRY_ASSERT (!is_hex && !is_exp);

  new_token ();

  // Eat up '.'
  if (is_fp)
  {
    consume_char ();
  }

  while (true)
  {
    c = LA (0);
    if (is_fp && c == '.')
    {
      PARSE_ERROR ("Integer literal shall not contain more than one dot character", buffer - buffer_start);
    }
    if (is_exp && (c == 'e' || c == 'E'))
    {
      PARSE_ERROR ("Integer literal shall not contain more than exponential marker ('e' or 'E')",
                   buffer - buffer_start);
    }

    if (c == '.')
    {
      if (__isalpha (LA (1)) || LA (1) == '_' || LA (1) == '$')
      {
        PARSE_ERROR ("Integer literal shall not contain non-digit character after got character",
                     buffer - buffer_start);
      }
      is_fp = true;
      consume_char ();
      continue;
    }

    if (c == 'e' || c == 'E')
    {
      if (LA (1) == '-' || LA (1) == '+')
      {
        consume_char ();
      }
      if (!__isdigit (LA (1)))
      {
        PARSE_ERROR ("Integer literal shall not contain non-digit character after exponential marker ('e' or 'E')",
                     buffer - buffer_start);
      }
      is_exp = true;
      consume_char ();
      continue;
    }

    if (__isalpha (c) || c == '_' || c == '$')
    {
      PARSE_ERROR ("Integer literal shall not contain non-digit characters", buffer - buffer_start);
    }

    if (!__isdigit (c))
    {
      break;
    }

    consume_char ();
  }

  if (is_fp || is_exp)
  {
    ecma_number_t res = __strtof (token_start, NULL);

    known_token = convert_seen_num_to_token (res);
    token_start = NULL;
    return known_token;
  }

  tok_length = (size_t) (buffer - token_start);;
  for (i = 0; i < tok_length; i++)
  {
    res = res * 10 + hex_to_int (token_start[i]);
  }

  if (res <= 255)
  {
    known_token = create_token_from_current_token (TOK_SMALL_INT, (uint8_t) res);
    token_start = NULL;
    return known_token;
  }

  known_token = convert_seen_num_to_token ((ecma_number_t) res);
  token_start = NULL;
  return known_token;
}

static token
parse_string (void)
{
  char c = LA (0);
  bool is_double_quoted;
  token result;

  JERRY_ASSERT (c == '\'' || c == '"');

  is_double_quoted = (c == '"');

  // Eat up '"'
  consume_char ();
  new_token ();

  while (true)
  {
    c = LA (0);
    if (c == '\0')
    {
      PARSE_ERROR ("Unclosed string", token_start - buffer_start);
    }
    if (c == '\n')
    {
      PARSE_ERROR ("String literal shall not contain newline character", token_start - buffer_start);
    }
    if (c == '\\')
    {
      /* Only single escape character is allowed.  */
      if (LA (1) == 'x' || LA (1) == 'u' || __isdigit (LA (1)))
      {
        PARSE_WARN ("Escape sequences are ignored yet", token_start - buffer_start);
        consume_char ();
        consume_char ();
        continue;
      }
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
    {
      break;
    }

    consume_char ();
  }

  result = convert_current_token_to_token (TOK_STRING);

  // Eat up '"'
  consume_char ();
  token_start = NULL;

  return result;
}

static void
grobble_whitespaces (void)
{
  char c = LA (0);

  while ((__isspace (c) && c != '\n'))
  {
    consume_char ();
    c = LA (0);
  }
}

static void
lexer_set_source (const char * source)
{
  buffer_start = source;
  buffer = buffer_start;
}

static void
lexer_rewind (void)
{
  JERRY_ASSERT (buffer_start != NULL);

  buffer = buffer_start;
}

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
    {
      return false;
    }
    if (multiline && c == '*' && LA (1) == '/')
    {
      consume_char ();
      consume_char ();

      if (was_newlines)
      {
        return true;
      }
      else
      {
        return false;
      }
    }
    if (multiline && c == '\n')
    {
      was_newlines = true;
    }
    if (multiline && c == '\0')
    {
      PARSE_ERROR ("Unclosed multiline comment", buffer - buffer_start);
    }
    consume_char ();
  }
}

static token
lexer_next_token_private (void)
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
  {
    return parse_name ();
  }

  if (__isdigit (c) || (c == '.' && __isdigit (LA (1))))
  {
    return parse_number ();
  }

  if (c == '\n')
  {
    consume_char ();
    return create_token_from_buffer_state (TOK_NEWLINE, 0);
  }

  if (c == '\0')
  {
    return create_token_from_buffer_state (TOK_EOF, 0);
  }

  if (c == '\'' || c == '"')
  {
    return parse_string ();
  }

  if (__isspace (c))
  {
    grobble_whitespaces ();
    return lexer_next_token_private ();
  }

  if (c == '/' && LA (1) == '*')
  {
    if (replace_comment_by_newline ())
    {
      return (token)
      {
        .type = TOK_NEWLINE,
        .uid = 0
      };
    }
    else
    {
      return lexer_next_token_private ();
    }
  }

  if (c == '/' && LA (1) == '/')
  {
    replace_comment_by_newline ();
    return lexer_next_token_private ();
  }

  switch (c)
  {
    case '{': RETURN_PUNC (TOK_OPEN_BRACE); break;
    case '}': RETURN_PUNC (TOK_CLOSE_BRACE); break;
    case '(': RETURN_PUNC (TOK_OPEN_PAREN); break;
    case ')': RETURN_PUNC (TOK_CLOSE_PAREN); break;
    case '[': RETURN_PUNC (TOK_OPEN_SQUARE); break;
    case ']': RETURN_PUNC (TOK_CLOSE_SQUARE); break;
    case '.': RETURN_PUNC (TOK_DOT); break;
    case ';': RETURN_PUNC (TOK_SEMICOLON); break;
    case ',': RETURN_PUNC (TOK_COMMA); break;
    case '~': RETURN_PUNC (TOK_COMPL); break;
    case ':': RETURN_PUNC (TOK_COLON); break;
    case '?': RETURN_PUNC (TOK_QUERY); break;

    case '*': IF_LA_IS ('=', TOK_MULT_EQ, TOK_MULT); break;
    case '/': IF_LA_IS ('=', TOK_DIV_EQ, TOK_DIV); break;
    case '^': IF_LA_IS ('=', TOK_XOR_EQ, TOK_XOR); break;
    case '%': IF_LA_IS ('=', TOK_MOD_EQ, TOK_MOD); break;

    case '+': IF_LA_IS_OR ('+', TOK_DOUBLE_PLUS, '=', TOK_PLUS_EQ, TOK_PLUS); break;
    case '-': IF_LA_IS_OR ('-', TOK_DOUBLE_MINUS, '=', TOK_MINUS_EQ, TOK_MINUS); break;
    case '&': IF_LA_IS_OR ('&', TOK_DOUBLE_AND, '=', TOK_AND_EQ, TOK_AND); break;
    case '|': IF_LA_IS_OR ('|', TOK_DOUBLE_OR, '=', TOK_OR_EQ, TOK_OR); break;

    case '<':
    {
      switch (LA (1))
      {
        case '<': IF_LA_N_IS ('=', TOK_LSHIFT_EQ, TOK_LSHIFT, 2); break;
        case '=': RETURN_PUNC_EX (TOK_LESS_EQ, 2); break;
        default: RETURN_PUNC (TOK_LESS);
      }
      break;
    }
    case '>':
    {
      switch (LA (1))
      {
        case '>':
        {
          switch (LA (2))
          {
            case '>': IF_LA_N_IS ('=', TOK_RSHIFT_EX_EQ, TOK_RSHIFT_EX, 3); break;
            case '=': RETURN_PUNC_EX (TOK_RSHIFT_EQ, 3); break;
            default: RETURN_PUNC_EX (TOK_RSHIFT, 2);
          }
          break;
        }
        case '=': RETURN_PUNC_EX (TOK_GREATER_EQ, 2); break;
        default: RETURN_PUNC (TOK_GREATER);
      }
      break;
    }
    case '=':
    {
      if (LA (1) == '=')
      {
        IF_LA_N_IS ('=', TOK_TRIPLE_EQ, TOK_DOUBLE_EQ, 2);
      }
      else
      {
        RETURN_PUNC (TOK_EQ);
      }
      break;
    }
    case '!':
    {
      if (LA (1) == '=')
      {
        IF_LA_N_IS ('=', TOK_NOT_DOUBLE_EQ, TOK_NOT_EQ, 2);
      }
      else
      {
        RETURN_PUNC (TOK_NOT);
      }
      break;
    }
    default: PARSE_SORRY ("Unknown character", buffer - buffer_start);
  }
  PARSE_SORRY ("Unknown character", buffer - buffer_start);
}

token
lexer_next_token (void)
{
  if (buffer == buffer_start)
  {
    dump_current_line ();
  }

  prev_token = sent_token;
  sent_token = lexer_next_token_private ();

  if (sent_token.type == TOK_NEWLINE)
  {
    dump_current_line ();
    return sent_token;
  }
  return sent_token;
}

void
lexer_save_token (token tok)
{
  saved_token = tok;
}

token
lexer_prev_token (void)
{
  return prev_token;
}

void
lexer_run_first_pass (void)
{
  token tok = lexer_next_token ();
  while (tok.type != TOK_EOF)
  {
    tok = lexer_next_token ();
  }

  lexer_rewind ();
}

void
lexer_locus_to_line_and_column (size_t locus, size_t *line, size_t *column)
{
  JERRY_ASSERT (locus < buffer_size);
  const char *buf;
  size_t l = 0, c = 0;
  for (buf = buffer_start; (size_t) (buf - buffer_start) < locus; buf++)
  {
    if (*buf == '\n')
    {
      c = 0;
      l++;
      continue;
    }
    c++;
  }

  if (line)
  {
    *line = l;
  }
  if (column)
  {
    *column = c;
  }
}

void
lexer_dump_line (size_t line)
{
  size_t l = 0;
  for (const char *buf = buffer_start; *buf != '\0'; buf++)
  {
    if (l == line)
    {
      for (; *buf != '\n' && *buf != '\0'; buf++)
      {
        __putchar (*buf);
      }
      return;
    }
    if (*buf == '\n')
    {
      l++;
    }
  }
}

const char *
lexer_keyword_to_string (keyword kw)
{
  switch (kw)
  {
    case KW_BREAK: return "break";
    case KW_CASE: return "case";
    case KW_CATCH: return "catch";

    case KW_CONTINUE: return "continue";
    case KW_DEBUGGER: return "debugger";
    case KW_DEFAULT: return "default";
    case KW_DELETE: return "delete";
    case KW_DO: return "do";

    case KW_ELSE: return "else";
    case KW_FINALLY: return "finally";
    case KW_FOR: return "for";
    case KW_FUNCTION: return "function";
    case KW_IF: return "if";

    case KW_IN: return "in";
    case KW_INSTANCEOF: return "instanceof";
    case KW_NEW: return "new";
    case KW_RETURN: return "return";
    case KW_SWITCH: return "switch";

    case KW_THIS: return "this";
    case KW_THROW: return "throw";
    case KW_TRY: return "try";
    case KW_TYPEOF: return "typeof";
    case KW_VAR: return "var";

    case KW_VOID: return "void";
    case KW_WHILE: return "while";
    case KW_WITH: return "with";
    default: JERRY_UNREACHABLE ();
  }
}

const char *
lexer_token_type_to_string (token_type tt)
{
  switch (tt)
  {
    case TOK_EOF: return "End of file";
    case TOK_NAME: return "Identifier";
    case TOK_KEYWORD: return "Keyword";
    case TOK_SMALL_INT: /* FALLTHRU */
    case TOK_NUMBER: return "Number";

    case TOK_NULL: return "null";
    case TOK_BOOL: return "bool";
    case TOK_NEWLINE: return "newline";
    case TOK_STRING: return "string";
    case TOK_OPEN_BRACE: return "{";

    case TOK_CLOSE_BRACE: return "}";
    case TOK_OPEN_PAREN: return "(";
    case TOK_CLOSE_PAREN: return ")";
    case TOK_OPEN_SQUARE: return "[";
    case TOK_CLOSE_SQUARE: return "]";

    case TOK_DOT: return ".";
    case TOK_SEMICOLON: return ";";
    case TOK_COMMA: return ",";
    case TOK_LESS: return "<";
    case TOK_GREATER: return ">";

    case TOK_LESS_EQ: return "<=";
    case TOK_GREATER_EQ: return "<=";
    case TOK_DOUBLE_EQ: return "==";
    case TOK_NOT_EQ: return "!=";
    case TOK_TRIPLE_EQ: return "===";

    case TOK_NOT_DOUBLE_EQ: return "!==";
    case TOK_PLUS: return "+";
    case TOK_MINUS: return "-";
    case TOK_MULT: return "*";
    case TOK_MOD: return "%%";

    case TOK_DOUBLE_PLUS: return "++";
    case TOK_DOUBLE_MINUS: return "--";
    case TOK_LSHIFT: return "<<";
    case TOK_RSHIFT: return ">>";
    case TOK_RSHIFT_EX: return ">>>";

    case TOK_AND: return "&";
    case TOK_OR: return "|";
    case TOK_XOR: return "^";
    case TOK_NOT: return "!";
    case TOK_COMPL: return "~";

    case TOK_DOUBLE_AND: return "&&";
    case TOK_DOUBLE_OR: return "||";
    case TOK_QUERY: return "?";
    case TOK_COLON: return ":";
    case TOK_EQ: return "=";

    case TOK_PLUS_EQ: return "+=";
    case TOK_MINUS_EQ: return "-=";
    case TOK_MULT_EQ: return "*=";
    case TOK_MOD_EQ: return "%%=";
    case TOK_LSHIFT_EQ: return "<<=";

    case TOK_RSHIFT_EQ: return ">>=";
    case TOK_RSHIFT_EX_EQ: return ">>>=";
    case TOK_AND_EQ: return "&=";
    case TOK_OR_EQ: return "|=";
    case TOK_XOR_EQ: return "^=";

    case TOK_DIV: return "/";
    case TOK_DIV_EQ: return "/=";
    case TOK_UNDEFINED: return "undefined";
    default: JERRY_UNREACHABLE ();
  }
}

void
lexer_init (const char *source, size_t source_size, bool show_opcodes)
{
  saved_token = prev_token = sent_token = empty_token;
  allow_dump_lines = show_opcodes;
  buffer_size = source_size;
  lexer_set_source (source);

  STACK_INIT (lp_string, strings);
  STACK_INIT (ecma_number_t, numbers);
  STACK_INIT (idx_t, num_ids);
}

void
lexer_free (void)
{
  STACK_FREE (strings);
  STACK_FREE (numbers);
  STACK_FREE (num_ids);
}

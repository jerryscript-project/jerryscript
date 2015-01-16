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

#include "mem-allocator.h"
#include "jerry-libc.h"
#include "lexer.h"
#include "parser.h"
#include "stack.h"
#include "opcodes.h"
#include "syntax-errors.h"
#include "parser.h"
#include "ecma-helpers.h"

static token saved_token, prev_token, sent_token, empty_token;

static bool allow_dump_lines = false, strict_mode;
static size_t buffer_size = 0;

/* Represents the contents of a script.  */
static const char *buffer_start = NULL;
static const char *buffer = NULL;
static const char *token_start;
static ecma_char_t *strings_cache;
static size_t strings_cache_size;
static size_t strings_cache_used_size;

#define LA(I)       (get_char (I))

enum
{
  literals_global_size
};
STATIC_STACK (literals, literal)

static bool
is_empty (token tok)
{
  return tok.type == TOK_EMPTY;
}

static locus
current_locus (void)
{
  if (token_start == NULL)
  {
    return (locus) (buffer - buffer_start);
  }
  else
  {
    return (locus) (token_start - buffer_start);
  }
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
create_token (token_type type, literal_index_t uid)
{
  token ret;

  ret.type = type;
  ret.loc = current_locus () - (type == TOK_STRING ? 1 : 0);
  ret.uid = uid;

  return ret;
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
current_token_equals_to_literal (literal lit)
{
  if (lit.type == LIT_STR)
  {
    if (lit.data.lp.length != (ecma_length_t) (buffer - token_start))
    {
      return false;
    }
    if (!__strncmp ((const char *) lit.data.lp.str, token_start, lit.data.lp.length))
    {
      return true;
    }
  }
  else if (lit.type == LIT_MAGIC_STR)
  {
    const char *str = (const char *) ecma_get_magic_string_zt (lit.data.magic_str_id);
    if (__strlen (str) != (size_t) (buffer - token_start))
    {
      return false;
    }
    if (!__strncmp (str, token_start, __strlen (str)))
    {
      return true;
    }
  }
  return false;
}

static literal
adjust_string_ptrs (literal lit, size_t diff)
{
  if (lit.type != LIT_STR)
  {
    return lit;
  }

  literal ret;

  ret.type = LIT_STR;
  ret.data.lp.length = lit.data.lp.length;
  ret.data.lp.hash = lit.data.lp.hash;
  ret.data.lp.str = lit.data.lp.str + diff;

  return ret;
}

static literal
add_current_token_to_string_cache (void)
{
  const ecma_length_t length = (ecma_length_t) (buffer - token_start);
  if (strings_cache_used_size + length * sizeof (ecma_char_t) >= strings_cache_size)
  {
    strings_cache_size = mem_heap_recommend_allocation_size (strings_cache_used_size
                                                             + ((size_t) length + 1) * sizeof (ecma_char_t));
    ecma_char_t *temp = (ecma_char_t *) mem_heap_alloc_block (strings_cache_size,
                                                              MEM_HEAP_ALLOC_SHORT_TERM);
    __memcpy (temp, strings_cache, strings_cache_used_size);
    STACK_ITERATE_VARG_SET (literals, adjust_string_ptrs, 0, (size_t) (temp - strings_cache));
    if (strings_cache)
    {
      mem_heap_free_block ((uint8_t *) strings_cache);
    }
    strings_cache = temp;
  }
  __strncpy ((char *) (strings_cache + strings_cache_used_size), token_start, length);
  (strings_cache + strings_cache_used_size)[length] = '\0';
  const literal res = create_literal_from_zt (strings_cache + strings_cache_used_size, length);
  strings_cache_used_size = (size_t) (((size_t) length + 1) * sizeof (ecma_char_t) + strings_cache_used_size);
  return res;
}

static token
convert_current_token_to_token (token_type tt)
{
  JERRY_ASSERT (token_start);

  for (literal_index_t i = 0; i < STACK_SIZE (literals); i++)
  {
    const literal lit = STACK_ELEMENT (literals, i);
    if ((lit.type == LIT_STR || lit.type == LIT_MAGIC_STR)
        && current_token_equals_to_literal (lit))
    {
      return create_token (tt, i);
    }
  }

  literal lit = create_literal_from_str (token_start, (ecma_length_t) (buffer - token_start));
  JERRY_ASSERT (lit.type == LIT_STR || lit.type == LIT_MAGIC_STR);
  if (lit.type == LIT_STR)
  {
    lit = add_current_token_to_string_cache ();
  }

  STACK_PUSH (literals, lit);

  return create_token (tt, (literal_index_t) (STACK_SIZE (literals) - 1));
}

/* If TOKEN represents a keyword, return decoded keyword,
   if TOKEN represents a Future Reserved Word, return KW_RESERVED,
   otherwise return KW_NONE.  */
static token
decode_keyword (void)
{
  if (current_token_equals_to ("break"))
  {
    return create_token (TOK_KEYWORD, KW_BREAK);
  }
  if (current_token_equals_to ("case"))
  {
    return create_token (TOK_KEYWORD, KW_CASE);
  }
  if (current_token_equals_to ("catch"))
  {
    return create_token (TOK_KEYWORD, KW_CATCH);
  }
  if (current_token_equals_to ("class"))
  {
    return create_token (TOK_KEYWORD, KW_CLASS);
  }
  if (current_token_equals_to ("const"))
  {
    return create_token (TOK_KEYWORD, KW_CONST);
  }
  if (current_token_equals_to ("continue"))
  {
    return create_token (TOK_KEYWORD, KW_CONTINUE);
  }
  if (current_token_equals_to ("debugger"))
  {
    return create_token (TOK_KEYWORD, KW_DEBUGGER);
  }
  if (current_token_equals_to ("default"))
  {
    return create_token (TOK_KEYWORD, KW_DEFAULT);
  }
  if (current_token_equals_to ("delete"))
  {
    return create_token (TOK_KEYWORD, KW_DELETE);
  }
  if (current_token_equals_to ("do"))
  {
    return create_token (TOK_KEYWORD, KW_DO);
  }
  if (current_token_equals_to ("else"))
  {
    return create_token (TOK_KEYWORD, KW_ELSE);
  }
  if (current_token_equals_to ("enum"))
  {
    return create_token (TOK_KEYWORD, KW_ENUM);
  }
  if (current_token_equals_to ("export"))
  {
    return create_token (TOK_KEYWORD, KW_EXPORT);
  }
  if (current_token_equals_to ("extends"))
  {
    return create_token (TOK_KEYWORD, KW_EXTENDS);
  }
  if (current_token_equals_to ("false"))
  {
    return create_token (TOK_BOOL, false);
  }
  if (current_token_equals_to ("finally"))
  {
    return create_token (TOK_KEYWORD, KW_FINALLY);
  }
  if (current_token_equals_to ("for"))
  {
    return create_token (TOK_KEYWORD, KW_FOR);
  }
  if (current_token_equals_to ("function"))
  {
    return create_token (TOK_KEYWORD, KW_FUNCTION);
  }
  if (current_token_equals_to ("if"))
  {
    return create_token (TOK_KEYWORD, KW_IF);
  }
  if (current_token_equals_to ("in"))
  {
    return create_token (TOK_KEYWORD, KW_IN);
  }
  if (current_token_equals_to ("instanceof"))
  {
    return create_token (TOK_KEYWORD, KW_INSTANCEOF);
  }
  if (current_token_equals_to ("interface"))
  {
    if (strict_mode)
    {
      return create_token (TOK_KEYWORD, KW_INTERFACE);
    }
    else
    {
      return convert_current_token_to_token (TOK_NAME);
    }
  }
  if (current_token_equals_to ("import"))
  {
    return create_token (TOK_KEYWORD, KW_IMPORT);
  }
  if (current_token_equals_to ("implements"))
  {
    if (strict_mode)
    {
      return create_token (TOK_KEYWORD, KW_IMPLEMENTS);
    }
    else
    {
      return convert_current_token_to_token (TOK_NAME);
    }
  }
  if (current_token_equals_to ("let"))
  {
    if (strict_mode)
    {
      return create_token (TOK_KEYWORD, KW_LET);
    }
    else
    {
      return convert_current_token_to_token (TOK_NAME);
    }
  }
  if (current_token_equals_to ("new"))
  {
    return create_token (TOK_KEYWORD, KW_NEW);
  }
  if (current_token_equals_to ("null"))
  {
    return create_token (TOK_NULL, 0);
  }
  if (current_token_equals_to ("package"))
  {
    if (strict_mode)
    {
      return create_token (TOK_KEYWORD, KW_PACKAGE);
    }
    else
    {
      return convert_current_token_to_token (TOK_NAME);
    }
  }
  if (current_token_equals_to ("private"))
  {
    if (strict_mode)
    {
      return create_token (TOK_KEYWORD, KW_PRIVATE);
    }
    else
    {
      return convert_current_token_to_token (TOK_NAME);
    }
  }
  if (current_token_equals_to ("protected"))
  {
    if (strict_mode)
    {
      return create_token (TOK_KEYWORD, KW_PROTECTED);
    }
    else
    {
      return convert_current_token_to_token (TOK_NAME);
    }
  }
  if (current_token_equals_to ("public"))
  {
    if (strict_mode)
    {
      return create_token (TOK_KEYWORD, KW_PUBLIC);
    }
    else
    {
      return convert_current_token_to_token (TOK_NAME);
    }
  }
  if (current_token_equals_to ("return"))
  {
    return create_token (TOK_KEYWORD, KW_RETURN);
  }
  if (current_token_equals_to ("static"))
  {
    if (strict_mode)
    {
      return create_token (TOK_KEYWORD, KW_STATIC);
    }
    else
    {
      return convert_current_token_to_token (TOK_NAME);
    }
  }
  if (current_token_equals_to ("super"))
  {
    return create_token (TOK_KEYWORD, KW_SUPER);
  }
  if (current_token_equals_to ("switch"))
  {
    return create_token (TOK_KEYWORD, KW_SWITCH);
  }
  if (current_token_equals_to ("this"))
  {
    return create_token (TOK_KEYWORD, KW_THIS);
  }
  if (current_token_equals_to ("throw"))
  {
    return create_token (TOK_KEYWORD, KW_THROW);
  }
  if (current_token_equals_to ("true"))
  {
    return create_token (TOK_BOOL, true);
  }
  if (current_token_equals_to ("try"))
  {
    return create_token (TOK_KEYWORD, KW_TRY);
  }
  if (current_token_equals_to ("typeof"))
  {
    return create_token (TOK_KEYWORD, KW_TYPEOF);
  }
  if (current_token_equals_to ("var"))
  {
    return create_token (TOK_KEYWORD, KW_VAR);
  }
  if (current_token_equals_to ("void"))
  {
    return create_token (TOK_KEYWORD, KW_VOID);
  }
  if (current_token_equals_to ("while"))
  {
    return create_token (TOK_KEYWORD, KW_WHILE);
  }
  if (current_token_equals_to ("with"))
  {
    return create_token (TOK_KEYWORD, KW_WITH);
  }
  if (current_token_equals_to ("yield"))
  {
    if (strict_mode)
    {
      return create_token (TOK_KEYWORD, KW_YIELD);
    }
    else
    {
      return convert_current_token_to_token (TOK_NAME);
    }
  }
  return empty_token;
}

static token
convert_seen_num_to_token (ecma_number_t num)
{
  for (literal_index_t i = 0; i < STACK_SIZE (literals); i++)
  {
    const literal lit = STACK_ELEMENT (literals, i);
    if (lit.type != LIT_NUMBER)
    {
      continue;
    }
    if (lit.data.num == num)
    {
      return create_token (TOK_NUMBER, i);
    }
  }

  STACK_PUSH (literals, create_literal_from_num (num));

  return create_token (TOK_NUMBER, (literal_index_t) (STACK_SIZE (literals) - 1));
}

const literal *
lexer_get_literals (void)
{
  literal *data = NULL;
  if (STACK_SIZE (literals) > 0)
  {
    STACK_CONVERT_TO_RAW_DATA (literals, data);
  }
  return data;
}

literal_index_t
lexer_get_literals_count (void)
{
  return (literal_index_t) STACK_SIZE (literals);
}

literal_index_t
lexer_lookup_literal_uid (literal lit)
{
  for (literal_index_t i = 0; i < STACK_SIZE (literals); i++)
  {
    if (literal_equal_type (STACK_ELEMENT (literals, i), lit))
    {
      return i;
    }
  }
  return INVALID_VALUE;
}

literal
lexer_get_literal_by_id (literal_index_t id)
{
  JERRY_ASSERT (id != INVALID_LITERAL);
  JERRY_ASSERT (id < STACK_SIZE (literals));
  return STACK_ELEMENT (literals, id);
}

const ecma_char_t *
lexer_get_strings_cache (void)
{
  return strings_cache;
}

void
lexer_add_literal_if_not_present (literal lit)
{
  for (literal_index_t i = 0; i < STACK_SIZE (literals); i++)
  {
    if (literal_equal_type (STACK_ELEMENT (literals, i), lit))
    {
      return;
    }
  }
  STACK_PUSH (literals, lit);
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
    token tok = create_token (TOK, 0); \
    buffer += NUM; \
    return tok; \
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
  bool is_overflow = false;
  ecma_number_t fp_res = .0;
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
      if (!is_overflow)
      {
        res = (res << 4) + hex_to_int (token_start[i]);
      }
      else
      {
        fp_res = fp_res * 16 + (ecma_number_t) hex_to_int (token_start[i]);
      }

      if (res > 255)
      {
        fp_res = (ecma_number_t) res;
        is_overflow = true;
        res = 0;
      }
    }

    if (is_overflow)
    {
      known_token = convert_seen_num_to_token (fp_res);
      token_start = NULL;
      return known_token;
    }
    else
    {
      known_token = create_token (TOK_SMALL_INT, (uint8_t) res);
      token_start = NULL;
      return known_token;
    }
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
      FIXME (/* This is wrong: 1..toString ().  */)
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

  tok_length = (size_t) (buffer - token_start);;
  if (is_fp || is_exp)
  {
    ecma_char_t *temp = (ecma_char_t*) mem_heap_alloc_block ((size_t) (tok_length + 1),
                                                             MEM_HEAP_ALLOC_SHORT_TERM);
    __strncpy ((char *) temp, token_start, (size_t) (tok_length));
    temp[tok_length] = '\0';
    ecma_number_t res = ecma_zt_string_to_number (temp);
    JERRY_ASSERT (!ecma_number_is_nan (res));
    mem_heap_free_block (temp);
    known_token = convert_seen_num_to_token (res);
    token_start = NULL;
    return known_token;
  }

  if (*token_start == '0' && tok_length != 1)
  {
    if (strict_mode)
    {
      PARSE_ERROR ("Octal tnteger literals are not allowed in strict mode", token_start - buffer_start);
    }
    for (i = 0; i < tok_length; i++)
    {
      if (!is_overflow)
      {
        res = res * 8 + hex_to_int (token_start[i]);
      }
      else
      {
        fp_res = fp_res * 8 + (ecma_number_t) hex_to_int (token_start[i]);
      }
      if (res > 255)
      {
        fp_res = (ecma_number_t) res;
        is_overflow = true;
        res = 0;
      }
    }
  }
  else
  {
    for (i = 0; i < tok_length; i++)
    {
      if (!is_overflow)
      {
        res = res * 10 + hex_to_int (token_start[i]);
      }
      else
      {
        fp_res = fp_res * 10 + (ecma_number_t) hex_to_int (token_start[i]);
      }
      if (res > 255)
      {
        fp_res = (ecma_number_t) res;
        is_overflow = true;
        res = 0;
      }
    }
  }

  if (is_overflow)
  {
    known_token = convert_seen_num_to_token (fp_res);
    token_start = NULL;
    return known_token;
  }
  else
  {
    known_token = create_token (TOK_SMALL_INT, (uint8_t) res);
    token_start = NULL;
    return known_token;
  }
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
        // PARSE_WARN ("Escape sequences are ignored yet", token_start - buffer_start);
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
    return create_token (TOK_NEWLINE, 0);
  }

  if (c == '\0')
  {
    return create_token (TOK_EOF, 0);
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
      token ret;

      ret.type = TOK_NEWLINE;
      ret.uid = 0;

      return ret;
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

  if (!is_empty (saved_token))
  {
    sent_token = saved_token;
    saved_token = empty_token;
    goto end;
  }

  prev_token = sent_token;
  sent_token = lexer_next_token_private ();

  if (sent_token.type == TOK_NEWLINE)
  {
    dump_current_line ();
  }

end:
  return sent_token;
}

void
lexer_save_token (token tok)
{
  JERRY_ASSERT (is_empty (saved_token));
  saved_token = tok;
}

token
lexer_prev_token (void)
{
  return prev_token;
}

void
lexer_seek (size_t locus)
{
  JERRY_ASSERT (locus < buffer_size);
  JERRY_ASSERT (token_start == NULL);

  buffer = buffer_start + locus;
  saved_token = empty_token;
}

void
lexer_locus_to_line_and_column (size_t locus, size_t *line, size_t *column)
{
  JERRY_ASSERT (locus <= buffer_size);
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
    case KW_CLASS: return "class";

    case KW_CONST: return "const";
    case KW_CONTINUE: return "continue";
    case KW_DEBUGGER: return "debugger";
    case KW_DEFAULT: return "default";
    case KW_DELETE: return "delete";

    case KW_DO: return "do";
    case KW_ELSE: return "else";
    case KW_ENUM: return "enum";
    case KW_EXPORT: return "export";
    case KW_EXTENDS: return "extends";

    case KW_FINALLY: return "finally";
    case KW_FOR: return "for";
    case KW_FUNCTION: return "function";
    case KW_IF: return "if";
    case KW_IN: return "in";

    case KW_INSTANCEOF: return "instanceof";
    case KW_INTERFACE: return "interface";
    case KW_IMPORT: return "import";
    case KW_IMPLEMENTS: return "implements";
    case KW_LET: return "let";

    case KW_NEW: return "new";
    case KW_PACKAGE: return "package";
    case KW_PRIVATE: return "private";
    case KW_PROTECTED: return "protected";
    case KW_PUBLIC: return "public";

    case KW_RETURN: return "return";
    case KW_STATIC: return "static";
    case KW_SUPER: return "super";
    case KW_SWITCH: return "switch";
    case KW_THIS: return "this";

    case KW_THROW: return "throw";
    case KW_TRY: return "try";
    case KW_TYPEOF: return "typeof";
    case KW_VAR: return "var";
    case KW_VOID: return "void";

    case KW_WHILE: return "while";
    case KW_WITH: return "with";
    case KW_YIELD: return "yield";
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
    default: JERRY_UNREACHABLE ();
  }
}

void
lexer_set_strict_mode (bool is_strict)
{
  strict_mode = is_strict;
}

void
lexer_init (const char *source, size_t source_size, bool show_opcodes)
{
  empty_token.type = TOK_EMPTY;
  empty_token.uid = 0;
  empty_token.loc = 0;

  saved_token = prev_token = sent_token = empty_token;
  allow_dump_lines = show_opcodes;
  buffer_size = source_size;
  lexer_set_source (source);
  strings_cache = NULL;
  strings_cache_used_size = strings_cache_size = 0;
  lexer_set_strict_mode (false);

  STACK_INIT (literals);
}

void
lexer_free (void)
{
  STACK_FREE (literals);
}

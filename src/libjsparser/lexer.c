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

static token saved_token;
static token empty_token =
{
  .type =
  TOK_EMPTY,
  .data.uid = 0
};

static bool allow_dump_lines = false;
static size_t buffer_size = 0;

typedef struct
{
  ecma_number_t num;
  token tok;
}
num_and_token;

#define MAX_NUMS 25

static uint8_t seen_names_count = 0;

static num_and_token seen_nums[MAX_NUMS];
static uint8_t seen_nums_count = 0;

static bool
is_empty (token tok)
{
  return tok.type == TOK_EMPTY;
}

/* Represents the contents of a script.  */
static const char *buffer_start = NULL;
static const char *buffer = NULL;
static const char *token_start;

static char
get_char (size_t i)
{
  if ((buffer + i) >= (buffer_start + buffer_size))
  {
    return '\0';
  }
  return *(buffer + i);
}

#define LA(I)       (get_char (I))

/* Continuous array of NULL-terminated strings.  */
static char *strings_cache = NULL;
static size_t strings_cache_size = 0;

static void
increase_strings_cache (void)
{
  char *new_cache;
  size_t new_cache_size;

  // if strings_cache_size == 0, allocator recommends minimum size that is more than 0
  new_cache_size = mem_heap_recommend_allocation_size (strings_cache_size * 2);
  new_cache = (char *) mem_heap_alloc_block (new_cache_size, MEM_HEAP_ALLOC_SHORT_TERM);

  if (!new_cache)
  {
    // Allocator alligns recommended memory size
    new_cache_size = mem_heap_recommend_allocation_size (strings_cache_size + 1);
    new_cache = (char *) mem_heap_alloc_block (new_cache_size, MEM_HEAP_ALLOC_SHORT_TERM);

    if (!new_cache)
    {
      parser_fatal (ERR_MEMORY);
    }
  }

  if (strings_cache)
  {
    __memcpy (new_cache, strings_cache, strings_cache_size);
    mem_heap_free_block ((uint8_t *) strings_cache);
  }

  strings_cache = new_cache;
  strings_cache_size = new_cache_size;
}

#ifdef __TARGET_HOST_x64
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
#endif

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

/* If TOKEN represents a keyword, return decoded keyword,
   if TOKEN represents a Future Reserved Word, return KW_RESERVED,
   otherwise return KW_NONE.  */
static token
decode_keyword (void)
{
  if (current_token_equals_to ("break"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_BREAK
    };
  }
  if (current_token_equals_to ("case"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_CASE
    };
  }
  if (current_token_equals_to ("catch"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_CATCH
    };
  }
  if (current_token_equals_to ("class"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("const"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("continue"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_CONTINUE
    };
  }
  if (current_token_equals_to ("debugger"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_DEBUGGER
    };
  }
  if (current_token_equals_to ("default"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_DEFAULT
    };
  }
  if (current_token_equals_to ("delete"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_DELETE
    };
  }
  if (current_token_equals_to ("do"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_DO
    };
  }
  if (current_token_equals_to ("else"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_ELSE
    };
  }
  if (current_token_equals_to ("enum"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("export"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("extends"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("false"))
  {
    return (token)
    {
      .type = TOK_BOOL,
      .data.uid = false
    };
  }
  if (current_token_equals_to ("finally"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_FINALLY
    };
  }
  if (current_token_equals_to ("for"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_FOR
    };
  }
  if (current_token_equals_to ("function"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_FUNCTION
    };
  }
  if (current_token_equals_to ("if"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_IF
    };
  }
  if (current_token_equals_to ("instanceof"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_INSTANCEOF
    };
  }
  if (current_token_equals_to ("interface"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("in"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_IN
    };
  }
  if (current_token_equals_to ("import"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("implements"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("let"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("new"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_NEW
    };
  }
  if (current_token_equals_to ("null"))
  {
    return (token)
    {
      .type = TOK_NULL,
      .data.uid = 0
    };
  }
  if (current_token_equals_to ("package"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("private"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("protected"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("public"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("return"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RETURN
    };
  }
  if (current_token_equals_to ("static"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("super"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  if (current_token_equals_to ("switch"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_SWITCH
    };
  }
  if (current_token_equals_to ("this"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_THIS
    };
  }
  if (current_token_equals_to ("throw"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_THROW
    };
  }
  if (current_token_equals_to ("true"))
  {
    return (token)
    {
      .type = TOK_BOOL,
      .data.uid = true
    };
  }
  if (current_token_equals_to ("try"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_TRY
    };
  }
  if (current_token_equals_to ("typeof"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_TYPEOF
    };
  }
  if (current_token_equals_to ("var"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_VAR
    };
  }
  if (current_token_equals_to ("void"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_VOID
    };
  }
  if (current_token_equals_to ("while"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_WHILE
    };
  }
  if (current_token_equals_to ("with"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_WITH
    };
  }
  if (current_token_equals_to ("yield"))
  {
    return (token)
    {
      .type = TOK_KEYWORD,
      .data.kw = KW_RESERVED
    };
  }
  return empty_token;
}

static token
convert_seen_name_to_token (token_type tt, const char *string)
{
  uint8_t i;
  char *current_string = strings_cache;
  JERRY_ASSERT (strings_cache);
  token ret_val = empty_token;

  for (i = 0; i < seen_names_count; i++)
  {
    if ((string == NULL && current_token_equals_to (current_string))
        || (string != NULL && !__strcmp (current_string, string)))
    {
      ret_val = (token) { .type = tt, .data.uid = i };
      break;
    }

    current_string += __strlen (current_string) + 1;
  }

  return ret_val;
}

static token
add_token_to_seen_names (token_type tt, const char *string)
{
  size_t i;
  char *current_string = strings_cache;
  size_t required_size;
  size_t len = (string == NULL ? (size_t) (buffer - token_start) : __strlen (string));
  token ret_val = empty_token;

  // Go to unused memory of cache
  for (i = 0; i < seen_names_count; i++)
  {
    current_string += __strlen (current_string) + 1;
  }

  required_size = (size_t) (current_string - strings_cache) + len + 1;
  if (required_size > strings_cache_size)
  {
    size_t offset = (size_t) (current_string - strings_cache);
    increase_strings_cache ();

    // Now our pointer are invalid, adjust it
    current_string = strings_cache + offset;
  }

  if (string == NULL)
  {
    // Copy current token with terminating NULL
    __strncpy (current_string, token_start, (size_t) (buffer - token_start));
    current_string += buffer - token_start;
    *current_string = '\0';
  }
  else
  {
    __memcpy (current_string, string, __strlen (string) + 1);
  }

  ret_val = (token) { .type = tt, .data.uid = seen_names_count++ };
  
  return ret_val;
}

static token
convert_seen_num_to_token (ecma_number_t num)
{
  size_t i;

  for (i = 0; i < seen_nums_count; i++)
  {
    // token must be exactly the same as seen
    if (seen_nums[i].num == num)
    {
      return seen_nums[i].tok;
    }
  }

  return empty_token;
}

static void
add_num_to_seen_tokens (num_and_token nat)
{
  JERRY_ASSERT (seen_nums_count < MAX_NUMS);

  seen_nums[seen_nums_count++] = nat;
}

uint8_t
lexer_get_strings (const char **strings)
{
  if (strings)
  {
    char *current_string = strings_cache;
    int i;
    for (i = 0; i < seen_names_count; i++)
    {
      strings[i] = current_string;
      current_string += __strlen (current_string) + 1;
    }
  }

  return seen_names_count;
}

uint8_t
lexer_get_reserved_ids_count (void)
{
  return (uint8_t) (seen_names_count + seen_nums_count);
}

const char *
lexer_get_string_by_id (uint8_t id)
{
  int i;
  char *current_string = strings_cache;
  JERRY_ASSERT (id < seen_names_count);

  for (i = 0 ; i < id; i++)
  {
    current_string += __strlen (current_string) + 1;
  }

  return current_string;
}

uint8_t
lexer_get_nums (ecma_number_t *nums)
{
  int i;

  if (!nums)
  {
    return seen_nums_count;
  }

  for (i = 0; i < seen_nums_count; i++)
  {
    nums[i] = seen_nums[i].num;
  }

  return seen_nums_count;
}

void
lexer_adjust_num_ids (void)
{
  size_t i;

  for (i = 0; i < seen_nums_count; i++)
  {
    seen_nums[i].tok.data.uid = (uint8_t) (seen_nums[i].tok.data.uid + seen_names_count);
  }
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
    return (token) \
    { \
      .type = TOK, \
      .data.uid = 0 \
    }; \
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

  known_token = convert_seen_name_to_token (TOK_NAME, NULL);
  if (!is_empty (known_token))
  {
    goto end;
  }

  known_token = add_token_to_seen_names (TOK_NAME, NULL);

end:
  token_start = NULL;
  return known_token;
}

static int32_t
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
  int32_t res = 0;
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
      parser_fatal (ERR_INT_LITERAL);
    }

    tok_length = (size_t) (buffer - token_start);
    // OK, I know that integer overflow can occur here
    for (i = 0; i < tok_length; i++)
    {
      res = (res << 4) + hex_to_int (token_start[i]);
    }

    token_start = NULL;

    if (res <= 255)
    {
      return (token)
      {
        .type = TOK_SMALL_INT,
        .data.uid = (uint8_t) res
      };
    }

    known_token = convert_seen_num_to_token ((ecma_number_t) res);
    if (!is_empty (known_token))
    {
      return known_token;
    }

    known_token = (token)
    {
      .type = TOK_NUMBER,
      .data.uid = seen_nums_count
    };
    add_num_to_seen_tokens (
      (num_and_token)
      {
        .num = (ecma_number_t) res,
        .tok = known_token
      }
    );
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
      parser_fatal (ERR_INT_LITERAL);
    }
    if (is_exp && (c == 'e' || c == 'E'))
    {
      parser_fatal (ERR_INT_LITERAL);
    }

    if (c == '.')
    {
      if (__isalpha (LA (1)) || LA (1) == '_' || LA (1) == '$')
      {
        parser_fatal (ERR_INT_LITERAL);
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
        parser_fatal (ERR_INT_LITERAL);
      }
      is_exp = true;
      consume_char ();
      continue;
    }

    if (__isalpha (c) || c == '_' || c == '$')
    {
      parser_fatal (ERR_INT_LITERAL);
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
    token_start = NULL;

    known_token = convert_seen_num_to_token (res);
    if (!is_empty (known_token))
    {
      return known_token;
    }

    known_token = (token)
    {
      .type = TOK_NUMBER,
      .data.uid = seen_nums_count
    };
    add_num_to_seen_tokens (
      (num_and_token)
      {
        .num = res,
        .tok = known_token
      }
    );
    return known_token;
  }

  tok_length = (size_t) (buffer - token_start);;
  for (i = 0; i < tok_length; i++)
  {
    res = res * 10 + hex_to_int (token_start[i]);
  }

  token_start = NULL;

  if (res <= 255)
  {
    return (token)
    {
      .type = TOK_SMALL_INT,
      .data.uid = (uint8_t) res
    };
  }

  known_token = convert_seen_num_to_token ((ecma_number_t) res);
  if (!is_empty (known_token))
  {
    return known_token;
  }

  known_token = (token)
  {
    .type = TOK_NUMBER,
    .data.uid = seen_nums_count
  };
  add_num_to_seen_tokens (
    (num_and_token)
    {
      .num = (ecma_number_t) res,
      .tok = known_token
    }
  );
  return known_token;
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
  token known_token = empty_token;

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
      parser_fatal (ERR_UNCLOSED);
    }
    if (c == '\n')
    {
      parser_fatal (ERR_STRING);
    }
    if (c == '\\')
    {
      /* Only single escape character is allowed.  */
      if (LA (1) == 'x' || LA (1) == 'u' || __isdigit (LA (1)))
      {
        parser_fatal (ERR_STRING);
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

  length = (size_t) (buffer - token_start);
  tok = (char *) mem_heap_alloc_block (length + 1, MEM_HEAP_ALLOC_SHORT_TERM);
  __memset (tok, '\0', length + 1);
  index = tok;

  // Copy current token to TOK and replace escape sequences by there meanings
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

  // Eat up '"'
  consume_char ();

  known_token = convert_seen_name_to_token (TOK_STRING, tok);
  if (!is_empty (known_token))
  {
    goto end;
  }

  known_token = add_token_to_seen_names (TOK_STRING, tok);

end:
  mem_heap_free_block ((uint8_t *) tok);
  token_start = NULL;
  return known_token;
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
      parser_fatal (ERR_UNCLOSED);
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
    return (token)
    {
      .type = TOK_NEWLINE,
      .data.uid = 0
    };
  }

  if (c == '\0')
  {
    return (token)
    {
      .type = TOK_EOF,
      .data.uid = 0
    };
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
        .data.uid = 0
      };
    }
    else
    {
      return lexer_next_token_private ();
    }
  }

  if (c == '/' && LA (1) == '/')
  {
    replace_comment_by_newline ();;
    return lexer_next_token_private ();
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
      {
        IF_LA_N_IS ('=', TOK_TRIPLE_EQ, TOK_DOUBLE_EQ, 2);
      }
      else
      {
        RETURN_PUNC (TOK_EQ);
      }

    case '!':
      if (LA (1) == '=')
      {
        IF_LA_N_IS ('=', TOK_NOT_DOUBLE_EQ, TOK_NOT_EQ, 2);
      }
      else
      {
        RETURN_PUNC (TOK_NOT);
      }

    default: JERRY_UNREACHABLE ();
  }
  parser_fatal (ERR_NON_CHAR);
}

token
lexer_next_token (void)
{
#ifdef __TARGET_HOST_x64
  if (buffer == buffer_start)
  {
    dump_current_line ();
  }
#endif /* __TARGET_HOST_x64 */

  token tok = lexer_next_token_private ();

#ifdef __TARGET_HOST_x64
  if (tok.type == TOK_NEWLINE)
  {
    dump_current_line ();
    return tok;
  }
#endif /* __TARGET_HOST_x64 */
  return tok;
}

void
lexer_save_token (token tok)
{
  saved_token = tok;
}

void
lexer_dump_buffer_state (void)
{
  __printf ("%s\n", buffer);
}

void
lexer_init (const char *source, size_t source_size, bool show_opcodes)
{
  saved_token = empty_token;
  allow_dump_lines = show_opcodes;
  buffer_size = source_size;
  lexer_set_source (source);
  increase_strings_cache ();
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
lexer_free (void)
{
  mem_heap_free_block ((uint8_t *) strings_cache);
  strings_cache = NULL;
  strings_cache_size = 0;
}

/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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

#include "ecma-helpers.h"
#include "jrt-libc-includes.h"
#include "jsp-mm.h"
#include "lexer.h"
#include "lit-char-helpers.h"
#include "lit-magic-strings.h"
#include "lit-strings.h"
#include "syntax-errors.h"

static token saved_token, prev_token, sent_token, empty_token;

static bool allow_dump_lines = false, strict_mode;
static size_t buffer_size = 0;

/* Represents the contents of a script.  */
static const jerry_api_char_t *buffer_start = NULL;
static const jerry_api_char_t *token_start;

static lit_utf8_iterator_t src_iter;

#define LA(I)       (get_char (I))

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
    return lit_utf8_iterator_get_offset (&src_iter);
  }
  else
  {
    return (locus) (token_start - buffer_start);
  }
}

static ecma_char_t
get_char (size_t i)
{
  lit_utf8_iterator_t iter = src_iter;
  ecma_char_t code_unit;
  do
  {
    if (lit_utf8_iterator_is_eos (&iter))
    {
      code_unit = LIT_CHAR_NULL;
      break;
    }

    code_unit = lit_utf8_iterator_read_next (&iter);
  }
  while (i--);

  return code_unit;
}

static void
dump_current_line (void)
{
  if (!allow_dump_lines)
  {
    return;
  }

  printf ("// ");

  lit_utf8_iterator_t iter = src_iter;

  while (!lit_utf8_iterator_is_eos (&iter))
  {
    ecma_char_t code_unit = lit_utf8_iterator_read_next (&iter);
    if (code_unit == '\n')
    {
      break;
    }

    lit_put_ecma_char (code_unit);
  }

  lit_put_ecma_char ('\n');
}

static token
create_token_from_lit (token_type type, literal_t lit)
{
  token ret;

  ret.type = type;
  ret.loc = current_locus () - (type == TOK_STRING ? 1 : 0);
  ret.uid = lit_cpointer_t::compress (lit).packed_value;

  return ret;
}

/**
 * Create token of specified type
 *
 * @return token descriptor
 */
static token
create_token (token_type type,  /**< type of token */
              uint16_t uid)     /**< uid of token */
{
  token ret;

  ret.type = type;
  ret.loc = current_locus () - (type == TOK_STRING ? 1 : 0);
  ret.uid = uid;

  return ret;
} /* create_token */

/**
 * Convert string to token of specified type
 *
 * @return token descriptor
 */
static token
convert_string_to_token (token_type tt, /**< token type */
                         const lit_utf8_byte_t *str_p, /**< characters buffer */
                         lit_utf8_size_t length) /**< string's length */
{
  JERRY_ASSERT (str_p != NULL);

  literal_t lit = lit_find_literal_by_utf8_string (str_p, length);
  if (lit != NULL)
  {
    return create_token_from_lit (tt, lit);
  }

  lit = lit_create_literal_from_utf8_string (str_p, length);
  JERRY_ASSERT (lit->get_type () == LIT_STR_T
                || lit->get_type () == LIT_MAGIC_STR_T
                || lit->get_type () == LIT_MAGIC_STR_EX_T);

  return create_token_from_lit (tt, lit);
}

/**
 * Try to decode specified string as ReservedWord (ECMA-262 v5, 7.6.1)
 *
 * @return TOK_KEYWORD - for Keyword or FutureReservedWord,
 *         TOK_NULL - for NullLiteral,
 *         TOK_BOOL - for BooleanLiteral,
 *         TOK_EMPTY - for other tokens.
 */
static token
lexer_parse_reserved_word (const lit_utf8_byte_t *str_p, /**< characters buffer */
                           lit_utf8_size_t str_size) /**< string's length */
{
  typedef struct
  {
    const char *keyword_p;
    keyword keyword_id;
  } kw_descr_t;

  const kw_descr_t keywords[] =
  {
#define KW_DESCR(literal, keyword_id) { literal, keyword_id }
    KW_DESCR ("break", KW_BREAK),
    KW_DESCR ("case", KW_CASE),
    KW_DESCR ("catch", KW_CATCH),
    KW_DESCR ("class", KW_CLASS),
    KW_DESCR ("const", KW_CONST),
    KW_DESCR ("continue", KW_CONTINUE),
    KW_DESCR ("debugger", KW_DEBUGGER),
    KW_DESCR ("default", KW_DEFAULT),
    KW_DESCR ("delete", KW_DELETE),
    KW_DESCR ("do", KW_DO),
    KW_DESCR ("else", KW_ELSE),
    KW_DESCR ("enum", KW_ENUM),
    KW_DESCR ("export", KW_EXPORT),
    KW_DESCR ("extends", KW_EXTENDS),
    KW_DESCR ("finally", KW_FINALLY),
    KW_DESCR ("for", KW_FOR),
    KW_DESCR ("function", KW_FUNCTION),
    KW_DESCR ("if", KW_IF),
    KW_DESCR ("in", KW_IN),
    KW_DESCR ("instanceof", KW_INSTANCEOF),
    KW_DESCR ("interface", KW_INTERFACE),
    KW_DESCR ("import", KW_IMPORT),
    KW_DESCR ("implements", KW_IMPLEMENTS),
    KW_DESCR ("let", KW_LET),
    KW_DESCR ("new", KW_NEW),
    KW_DESCR ("package", KW_PACKAGE),
    KW_DESCR ("private", KW_PRIVATE),
    KW_DESCR ("protected", KW_PROTECTED),
    KW_DESCR ("public", KW_PUBLIC),
    KW_DESCR ("return", KW_RETURN),
    KW_DESCR ("static", KW_STATIC),
    KW_DESCR ("super", KW_SUPER),
    KW_DESCR ("switch", KW_SWITCH),
    KW_DESCR ("this", KW_THIS),
    KW_DESCR ("throw", KW_THROW),
    KW_DESCR ("try", KW_TRY),
    KW_DESCR ("typeof", KW_TYPEOF),
    KW_DESCR ("var", KW_VAR),
    KW_DESCR ("void", KW_VOID),
    KW_DESCR ("while", KW_WHILE),
    KW_DESCR ("with", KW_WITH),
    KW_DESCR ("yield", KW_YIELD)
#undef KW_DESCR
  };

  keyword kw = KW_NONE;

  for (uint32_t i = 0; i < sizeof (keywords) / sizeof (kw_descr_t); i++)
  {
    if (lit_compare_utf8_strings (str_p,
                                  str_size,
                                  (lit_utf8_byte_t *) keywords[i].keyword_p,
                                  (lit_utf8_size_t) strlen (keywords[i].keyword_p)))
    {
      kw = keywords[i].keyword_id;
      break;
    }
  }

  if (!strict_mode)
  {
    switch (kw)
    {
      case KW_INTERFACE:
      case KW_IMPLEMENTS:
      case KW_LET:
      case KW_PACKAGE:
      case KW_PRIVATE:
      case KW_PROTECTED:
      case KW_PUBLIC:
      case KW_STATIC:
      case KW_YIELD:
      {
        return empty_token;
      }

      default:
      {
        break;
      }
    }
  }

  if (kw != KW_NONE)
  {
    return create_token (TOK_KEYWORD, kw);
  }
  else
  {
    if (lit_compare_utf8_string_and_magic_string (str_p, str_size, LIT_MAGIC_STRING_FALSE))
    {
      return create_token (TOK_BOOL, false);
    }
    else if (lit_compare_utf8_string_and_magic_string (str_p, str_size, LIT_MAGIC_STRING_TRUE))
    {
      return create_token (TOK_BOOL, true);
    }
    else if (lit_compare_utf8_string_and_magic_string (str_p, str_size, LIT_MAGIC_STRING_NULL))
    {
      return create_token (TOK_NULL, 0);
    }
    else
    {
      return empty_token;
    }
  }
} /* lexer_parse_reserved_word */

static token
convert_seen_num_to_token (ecma_number_t num)
{
  literal_t lit = lit_find_literal_by_num (num);
  if (lit != NULL)
  {
    return create_token_from_lit (TOK_NUMBER, lit);
  }

  return create_token_from_lit (TOK_NUMBER, lit_create_literal_from_num (num));
}

static void
new_token (void)
{
  JERRY_ASSERT (lit_utf8_iterator_get_ptr (&src_iter));
  token_start = lit_utf8_iterator_get_ptr (&src_iter);
}

static void
consume_char (void)
{
  lit_utf8_iterator_incr (&src_iter);
}

#define RETURN_PUNC_EX(TOK, NUM) \
  do \
  { \
    token tok = create_token (TOK, 0); \
    lit_utf8_iterator_advance (&src_iter, NUM); \
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

/**
 * Try to decode specified character as SingleEscapeCharacter (ECMA-262, v5, 7.8.4)
 *
 * If specified character is a SingleEscapeCharacter, convert it according to ECMA-262 v5, Table 4.
 * Otherwise, output it as is.
 *
 * @return true - if specified character is a SingleEscapeCharacter,
 *         false - otherwise.
 */
static bool
convert_single_escape_character (ecma_char_t c, /**< character to decode */
                                 ecma_char_t *out_converted_char_p) /**< out: decoded character */
{
  ecma_char_t converted_char;
  bool is_single_escape_character = true;

  switch (c)
  {
    case 'b':
    {
      converted_char = (ecma_char_t) '\b';
      break;
    }

    case 't':
    {
      converted_char = (ecma_char_t) '\t';
      break;
    }

    case 'n':
    {
      converted_char = (ecma_char_t) '\n';
      break;
    }

    case 'v':
    {
      converted_char = (ecma_char_t) '\v';
      break;
    }

    case 'f':
    {
      converted_char = (ecma_char_t) '\f';
      break;
    }

    case 'r':
    {
      converted_char = (ecma_char_t) '\r';
      break;
    }

    case '"':
    case '\'':
    case '\\':
    {
      converted_char = (ecma_char_t) c;
      break;
    }

    default:
    {
      converted_char = (ecma_char_t) c;
      is_single_escape_character = false;
      break;
    }
  }

  if (out_converted_char_p != NULL)
  {
    *out_converted_char_p = converted_char;
  }

  return is_single_escape_character;
} /* convert_single_escape_character */

/**
 * Convert specified string to token of specified type, transforming escape sequences
 *
 * @return token descriptor
 */
static token
convert_string_to_token_transform_escape_seq (token_type tok_type, /**< type of token to produce */
                                              const jerry_api_char_t *source_str_p, /**< string to convert,
                                                                                     *   located in source buffer */
                                              size_t source_str_size) /**< size of the string */
{
  token ret;

  if (source_str_size == 0)
  {
    return convert_string_to_token (tok_type,
                                    lit_get_magic_string_utf8 (LIT_MAGIC_STRING__EMPTY),
                                    0);
  }
  else
  {
    JERRY_ASSERT (source_str_p != NULL);
  }

  lit_utf8_byte_t *str_buf_p = (lit_utf8_byte_t*) jsp_mm_alloc (source_str_size);
  lit_utf8_byte_t *str_buf_iter_p = str_buf_p;

  /* const lit_utf8_byte_t *source_str_iter_p = source_str_p; */
  lit_utf8_iterator_t source_str_iter = lit_utf8_iterator_create (source_str_p, (lit_utf8_size_t) source_str_size);

  bool is_correct_sequence = true;
  bool every_char_islower = true;
  bool every_char_allowed_in_identifier = true;

  ecma_char_t prev_converted_char = LIT_CHAR_NULL;
  while (!lit_utf8_iterator_is_eos (&source_str_iter))
  {
    ecma_char_t converted_char = lit_utf8_iterator_read_next (&source_str_iter);

    if (converted_char == '\\')
    {
      const ecma_char_t escape_character = lit_utf8_iterator_read_next (&source_str_iter);

      if (isdigit (escape_character))
      {
        if (escape_character == '0')
        {
          converted_char = LIT_CHAR_NULL;
        }
        else
        {
          /* Implementation-defined (ECMA-262 v5, B.1.2): octal escape sequences are not implemented */
          is_correct_sequence = false;
          break;
        }
      }
      else if (escape_character == 'u'
               || escape_character == 'x')
      {
        const uint32_t hex_chars_num = (escape_character == 'u' ? 4u : 2u);

        if (lit_utf8_iterator_get_offset (&source_str_iter) + hex_chars_num > source_str_size)
        {
          is_correct_sequence = false;
          break;
        }

        bool chars_are_hex = true;
        uint16_t char_code = 0;

        for (uint32_t i = 0; i < hex_chars_num; i++)
        {
          const ecma_char_t next_char = lit_utf8_iterator_read_next (&source_str_iter);

          if (!isxdigit (next_char))
          {
            chars_are_hex = false;
            break;
          }
          else
          {
            /*
             * Check that highest 4 bits are zero, so the value would not overflow.
             */
            JERRY_ASSERT ((char_code & 0xF000u) == 0);

            char_code = (uint16_t) (char_code << 4u);
            char_code = (uint16_t) (char_code + lit_char_hex_to_int (next_char));
          }
        }

        JERRY_ASSERT (str_buf_iter_p <= str_buf_p + source_str_size);
        JERRY_ASSERT (lit_utf8_iterator_get_offset (&source_str_iter) <= source_str_size);

        if (!chars_are_hex)
        {
          is_correct_sequence = false;
          break;
        }

        /*
         * In CONFIG_ECMA_CHAR_ASCII mode size of ecma_char_t is 1 byte, so the conversion
         * would ignore highest part of 2-byte value, and in CONFIG_ECMA_CHAR_UTF16 mode this
         * would be just an assignment of 2-byte value.
         */
        converted_char = (ecma_char_t) char_code;
      }
      else if (lit_char_is_line_terminator (escape_character))
      {
        if (str_buf_iter_p + 1 <= source_str_p + source_str_size)
        {
          ecma_char_t next_char = lit_utf8_iterator_peek_next (&source_str_iter);

          if (escape_character == '\x0D'
              && next_char == '\x0A')
          {
            lit_utf8_iterator_incr (&source_str_iter);
          }
        }

        continue;
      }
      else
      {
        convert_single_escape_character ((ecma_char_t) escape_character, &converted_char);
      }
    }

    if (lit_is_code_unit_high_surrogate (prev_converted_char)
        && lit_is_code_unit_low_surrogate (converted_char))
    {
      str_buf_iter_p -= LIT_UTF8_MAX_BYTES_IN_CODE_UNIT;
      lit_code_point_t code_point = lit_convert_surrogate_pair_to_code_point (prev_converted_char, converted_char);
      str_buf_iter_p += lit_code_point_to_utf8 (code_point, str_buf_iter_p);
    }
    else
    {
      str_buf_iter_p += lit_code_unit_to_utf8 (converted_char, str_buf_iter_p);
      JERRY_ASSERT (str_buf_iter_p <= str_buf_p + source_str_size);
    }

    prev_converted_char = converted_char;

    if (!islower (converted_char))
    {
      every_char_islower = false;

      if (!isalpha (converted_char)
          && !isdigit (converted_char)
          && converted_char != '$'
          && converted_char != '_')
      {
        every_char_allowed_in_identifier = false;
      }
    }
  }

  if (is_correct_sequence)
  {
    lit_utf8_size_t length = (lit_utf8_size_t) (str_buf_iter_p - str_buf_p);
    ret = empty_token;

    if (tok_type == TOK_NAME)
    {
      if (every_char_islower)
      {
        ret = lexer_parse_reserved_word (str_buf_p, length);
      }
      else if (!every_char_allowed_in_identifier)
      {
        PARSE_ERROR ("Malformed identifier name", source_str_p - buffer_start);
      }
    }

    if (is_empty (ret))
    {
      ret = convert_string_to_token (tok_type, str_buf_p, length);
    }
  }
  else
  {
    PARSE_ERROR ("Malformed escape sequence", source_str_p - buffer_start);
  }

  jsp_mm_free (str_buf_p);

  return ret;
} /* convert_string_to_token_transform_escape_seq */

/**
 * Parse identifier (ECMA-262 v5, 7.6) or keyword (7.6.1.1)
 */
static token
parse_name (void)
{
  ecma_char_t c = (ecma_char_t) LA (0);

  token known_token = empty_token;

  JERRY_ASSERT (isalpha (c) || c == '$' || c == '_' || c == '\\');

  new_token ();

  while (true)
  {
    c = (ecma_char_t) LA (0);

    if (!isalpha (c)
        && !isdigit (c)
        && c != '$'
        && c != '_'
        && c != '\\')
    {
      break;
    }
    else
    {
      consume_char ();

      if (c == '\\')
      {
        bool is_correct_sequence = (LA (0) == 'u');
        if (is_correct_sequence)
        {
          consume_char ();
        }

        for (uint32_t i = 0;
             is_correct_sequence && i < 4;
             i++)
        {
          if (!isxdigit (LA (0)))
          {
            is_correct_sequence = false;
            break;
          }

          consume_char ();
        }

        if (!is_correct_sequence)
        {
          PARSE_ERROR ("Malformed escape sequence", token_start - buffer_start);
        }
      }
    }
  }

  const lit_utf8_size_t seq_size = (lit_utf8_size_t) (lit_utf8_iterator_get_ptr (&src_iter) - token_start);
  known_token = convert_string_to_token_transform_escape_seq (TOK_NAME,
                                                              token_start,
                                                              seq_size);

  token_start = NULL;

  return known_token;
} /* parse_name */

/* In this function we cannot use strtol function
   since there is no octal literals in ECMAscript.  */
static token
parse_number (void)
{
  ecma_char_t c = LA (0);
  bool is_hex = false;
  bool is_fp = false;
  bool is_exp = false;
  bool is_overflow = false;
  ecma_number_t fp_res = .0;
  size_t tok_length = 0, i;
  uint32_t res = 0;
  token known_token;

  JERRY_ASSERT (isdigit (c) || c == '.');

  if (c == '0')
  {
    if (LA (1) == 'x' || LA (1) == 'X')
    {
      is_hex = true;
    }
  }

  if (c == '.')
  {
    JERRY_ASSERT (!isalpha (LA (1)));
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
      if (!isxdigit (c))
      {
        break;
      }
      consume_char ();
    }

    if (isalpha (c) || c == '_' || c == '$')
    {
      PARSE_ERROR ("Integer literal shall not contain non-digit characters",
                   lit_utf8_iterator_get_offset (&src_iter));
    }

    tok_length = (size_t) (lit_utf8_iterator_get_ptr (&src_iter) - token_start);

    for (i = 0; i < tok_length; i++)
    {
      if (!is_overflow)
      {
        res = (res << 4) + lit_char_hex_to_int (token_start[i]);
      }
      else
      {
        fp_res = fp_res * 16 + (ecma_number_t) lit_char_hex_to_int (token_start[i]);
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
      PARSE_ERROR ("Integer literal shall not contain more than one dot character",
                   lit_utf8_iterator_get_offset (&src_iter));
    }
    if (is_exp && (c == 'e' || c == 'E'))
    {
      PARSE_ERROR ("Integer literal shall not contain more than exponential marker ('e' or 'E')",
                   lit_utf8_iterator_get_offset (&src_iter));
    }

    if (c == '.')
    {
      if (isalpha (LA (1)) || LA (1) == '_' || LA (1) == '$')
      {
        PARSE_ERROR ("Integer literal shall not contain non-digit character after got character",
                     lit_utf8_iterator_get_offset (&src_iter));
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
      if (!isdigit (LA (1)))
      {
        PARSE_ERROR ("Integer literal shall not contain non-digit character after exponential marker ('e' or 'E')",
                     lit_utf8_iterator_get_offset (&src_iter));
      }
      is_exp = true;
      consume_char ();
      continue;
    }

    if (isalpha (c) || c == '_' || c == '$')
    {
      PARSE_ERROR ("Integer literal shall not contain non-digit characters",
                   lit_utf8_iterator_get_offset (&src_iter));
    }

    if (!isdigit (c))
    {
      break;
    }

    consume_char ();
  }

  tok_length = (size_t) (lit_utf8_iterator_get_ptr (&src_iter) - token_start);
  if (is_fp || is_exp)
  {
    ecma_number_t res = ecma_utf8_string_to_number (token_start, (jerry_api_size_t) tok_length);
    JERRY_ASSERT (!ecma_number_is_nan (res));
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
        res = res * 8 + lit_char_hex_to_int (token_start[i]);
      }
      else
      {
        fp_res = fp_res * 8 + (ecma_number_t) lit_char_hex_to_int (token_start[i]);
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
        res = res * 10 + lit_char_hex_to_int (token_start[i]);
      }
      else
      {
        fp_res = fp_res * 10 + (ecma_number_t) lit_char_hex_to_int (token_start[i]);
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

/**
 * Parse string literal (ECMA-262 v5, 7.8.4)
 */
static token
parse_string (void)
{
  ecma_char_t c = (ecma_char_t) LA (0);
  JERRY_ASSERT (c == '\'' || c == '"');

  consume_char ();
  new_token ();

  const bool is_double_quoted = (c == '"');
  const char end_char = (is_double_quoted ? '"' : '\'');

  do
  {
    c = (ecma_char_t) LA (0);
    consume_char ();

    if (c == '\0')
    {
      PARSE_ERROR ("Unclosed string", token_start - buffer_start);
    }
    else if (lit_char_is_line_terminator (c))
    {
      PARSE_ERROR ("String literal shall not contain newline character", token_start - buffer_start);
    }
    else if (c == '\\')
    {
      ecma_char_t nc = (ecma_char_t) LA (0);

      if (convert_single_escape_character (nc, NULL))
      {
        consume_char ();
      }
      else if (lit_char_is_line_terminator (nc))
      {
        consume_char ();

        if (nc == LIT_CHAR_CR)
        {
          nc = (ecma_char_t) LA (0);

          if (nc == LIT_CHAR_LF)
          {
            consume_char ();
          }
        }
      }
    }
  }
  while (c != end_char);

  const lit_utf8_size_t esc_seq_size = (lit_utf8_size_t) (lit_utf8_iterator_get_ptr (&src_iter) -
                                                          token_start) - 1;

  token ret = convert_string_to_token_transform_escape_seq (TOK_STRING,
                                                            token_start,
                                                            esc_seq_size);

  token_start = NULL;

  return ret;
} /* parse_string */

/**
 * Parse string literal (ECMA-262 v5, 7.8.5)
 */
static token
parse_regexp (void)
{
  token result;
  bool is_char_class = false;

  /* Eat up '/' */
  JERRY_ASSERT ((ecma_char_t) LA (0) == '/');
  consume_char ();
  new_token ();

  while (true)
  {
    ecma_char_t c = (ecma_char_t) LA (0);

    if (c == '\0')
    {
      PARSE_ERROR ("Unclosed string", token_start - buffer_start);
    }
    else if (c == '\n')
    {
      PARSE_ERROR ("RegExp literal shall not contain newline character", token_start - buffer_start);
    }
    else if (c == '\\')
    {
      consume_char ();
    }
    else if (c == '[')
    {
      is_char_class = true;
    }
    else if (c == ']')
    {
      is_char_class = false;
    }
    else if (c == '/' && !is_char_class)
    {
      /* Eat up '/' */
      consume_char ();
      break;
    }

    consume_char ();
  }

  /* Try to parse RegExp flags */
  while (true)
  {
    ecma_char_t c = (ecma_char_t) LA (0);

    if (c == '\0'
        || !lit_char_is_word_char (c)
        || lit_char_is_line_terminator (c))
    {
      break;
    }
    consume_char ();
  }

  result = convert_string_to_token (TOK_REGEXP,
                                    (const lit_utf8_byte_t *) token_start,
                                    (lit_utf8_size_t) (lit_utf8_iterator_get_ptr (&src_iter) -
                                                      token_start));

  token_start = NULL;
  return result;
} /* parse_regexp */

static void
grobble_whitespaces (void)
{
  ecma_char_t c = LA (0);

  while ((isspace (c) && c != '\n'))
  {
    consume_char ();
    c = LA (0);
  }
}

static bool
replace_comment_by_newline (void)
{
  ecma_char_t c = LA (0);
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
      PARSE_ERROR ("Unclosed multiline comment", lit_utf8_iterator_get_offset (&src_iter));
    }
    consume_char ();
  }
}

static token
lexer_next_token_private (void)
{
  ecma_char_t c = LA (0);

  JERRY_ASSERT (token_start == NULL);

  if (isalpha (c) || c == '$' || c == '_' || c == '\\')
  {
    return parse_name ();
  }

  if (isdigit (c) || (c == '.' && isdigit (LA (1))))
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

  if (isspace (c))
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


  if (c == '/')
  {
    if (LA (1) == '/')
    {
      replace_comment_by_newline ();
      return lexer_next_token_private ();
    }
    else if (!(sent_token.type == TOK_NAME
             || sent_token.type == TOK_NULL
             || sent_token.type == TOK_BOOL
             || sent_token.type == TOK_CLOSE_BRACE
             || sent_token.type == TOK_CLOSE_SQUARE
             || sent_token.type == TOK_CLOSE_PAREN
             || sent_token.type == TOK_SMALL_INT
             || sent_token.type == TOK_NUMBER
             || sent_token.type == TOK_STRING
             || sent_token.type == TOK_REGEXP))
    {
      return parse_regexp ();
    }
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
    default: PARSE_SORRY ("Unknown character", lit_utf8_iterator_get_offset (&src_iter));
  }
  PARSE_SORRY ("Unknown character", lit_utf8_iterator_get_offset (&src_iter));
}

token
lexer_next_token (void)
{
  if (lit_utf8_iterator_get_offset (&src_iter) == 0)
  {
    dump_current_line ();
  }

  if (!is_empty (saved_token))
  {
    sent_token = saved_token;
    saved_token = empty_token;
    goto end;
  }

  /**
   * FIXME:
   *       The way to raise syntax errors for unexpected EOF
   *       should be reworked so that EOF would be checked by
   *       caller of the routine, and the following condition
   *       would be checked as assertion in the routine.
   */
  if (prev_token.type == TOK_EOF
      && sent_token.type == TOK_EOF)
  {
    PARSE_ERROR ("Unexpected EOF", lit_utf8_iterator_get_offset (&src_iter));
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

  lit_utf8_iterator_set_offset (&src_iter, (lit_utf8_size_t) locus);
  saved_token = empty_token;
}

void
lexer_locus_to_line_and_column (size_t locus, size_t *line, size_t *column)
{
  JERRY_ASSERT (locus <= buffer_size);
  const jerry_api_char_t *buf;
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
  for (const lit_utf8_byte_t *buf = buffer_start; *buf != '\0'; buf++)
  {
    if (l == line)
    {
      for (; *buf != '\n' && *buf != '\0'; buf++)
      {
        putchar (*buf);
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

/**
 * Check whether the identifier tokens represent the same identifiers
 *
 * Note:
 *      As all literals represent unique strings,
 *      it is sufficient to just check if literal indices
 *      in the tokens are equal.
 *
 * @return true / false
 */
bool
lexer_are_tokens_with_same_identifier (token id1, /**< identifier token (TOK_NAME) */
                                       token id2) /**< identifier token (TOK_NAME) */
{
  JERRY_ASSERT (id1.type == TOK_NAME);
  JERRY_ASSERT (id2.type == TOK_NAME);

  return (id1.uid == id2.uid);
} /* lexer_are_tokens_with_same_identifier */

/**
 * Initialize lexer to start parsing of a new source
 */
void
lexer_init (const jerry_api_char_t *source, /**< script source */
            size_t source_size, /**< script source size in bytes */
            bool show_opcodes) /**< flag indicating if to dump opcodes */
{
  empty_token.type = TOK_EMPTY;
  empty_token.uid = 0;
  empty_token.loc = 0;

  saved_token = prev_token = sent_token = empty_token;

  if (!lit_is_utf8_string_valid (source, (lit_utf8_size_t) source_size))
  {
    PARSE_ERROR ("Invalid source encoding", 0);
  }

  src_iter = lit_utf8_iterator_create (source, (lit_utf8_size_t) source_size);

  buffer_size = source_size;
  buffer_start = source;
  token_start = NULL;

  lexer_set_strict_mode (false);

#ifndef JERRY_NDEBUG
  allow_dump_lines = show_opcodes;
#else /* JERRY_NDEBUG */
  (void) show_opcodes;
  allow_dump_lines = false;
#endif /* JERRY_NDEBUG */
} /* lexer_init */

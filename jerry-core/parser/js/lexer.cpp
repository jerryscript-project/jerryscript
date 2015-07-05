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

/*
 * FIXME:
 *       jerry_api_char_t should not be used outside of API implementation
 */

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
 * Create token of specified type from charset
 *
 * @return token descriptor
 */
static token
lexer_create_token_for_charset (token_type tt, /**< token type */
                                const lit_utf8_byte_t *charset_p, /**< charset buffer */
                                lit_utf8_size_t size) /**< size of the charset */
{
  JERRY_ASSERT (charset_p != NULL);

  literal_t lit = lit_find_literal_by_utf8_string (charset_p, size);
  if (lit != NULL)
  {
    return create_token_from_lit (tt, lit);
  }

  lit = lit_create_literal_from_utf8_string (charset_p, size);
  JERRY_ASSERT (lit->get_type () == LIT_STR_T
                || lit->get_type () == LIT_MAGIC_STR_T
                || lit->get_type () == LIT_MAGIC_STR_EX_T);

  return create_token_from_lit (tt, lit);
} /* lexer_create_token_for_charset */

/**
 * Check if the character falls into IdentifierStart group (ECMA-262 v5, 7.6)
 *
 * @return true / false
 */
static bool
lexer_is_char_can_be_identifier_start (ecma_char_t c) /**< a character */
{
  return (lit_char_is_unicode_letter (c)
          || c == LIT_CHAR_DOLLAR_SIGN
          || c == LIT_CHAR_UNDERSCORE
          || c == LIT_CHAR_BACKSLASH);
} /* lexer_is_char_can_be_identifier_start */

/**
 * Check if the character falls into IdentifierPart group (ECMA-262 v5, 7.6)
 *
 * @return true / false
 */
static bool
lexer_is_char_can_be_identifier_part (ecma_char_t c) /**< a character */
{
  return (lexer_is_char_can_be_identifier_start (c)
          || lit_char_is_unicode_combining_mark (c)
          || lit_char_is_unicode_digit (c)
          || lit_char_is_unicode_connector_punctuation (c));
} /* lexer_is_char_can_be_identifier_part */

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
lexer_convert_single_escape_character (ecma_char_t c, /**< character to decode */
                                       ecma_char_t *out_converted_char_p) /**< out: decoded character */
{
  ecma_char_t converted_char;
  bool is_single_escape_character = true;

  switch (c)
  {
    case LIT_CHAR_LOWERCASE_B:
    {
      converted_char = LIT_CHAR_BS;
      break;
    }

    case LIT_CHAR_LOWERCASE_T:
    {
      converted_char = LIT_CHAR_TAB;
      break;
    }

    case LIT_CHAR_LOWERCASE_N:
    {
      converted_char = LIT_CHAR_LF;
      break;
    }

    case LIT_CHAR_LOWERCASE_V:
    {
      converted_char = LIT_CHAR_VTAB;
      break;
    }

    case LIT_CHAR_LOWERCASE_F:
    {
      converted_char = LIT_CHAR_FF;
      break;
    }

    case LIT_CHAR_LOWERCASE_R:
    {
      converted_char = LIT_CHAR_CR;
      break;
    }

    case LIT_CHAR_DOUBLE_QUOTE:
    case LIT_CHAR_SINGLE_QUOTE:
    case LIT_CHAR_BACKSLASH:
    {
      converted_char = c;
      break;
    }

    default:
    {
      converted_char = c;
      is_single_escape_character = false;
      break;
    }
  }

  if (out_converted_char_p != NULL)
  {
    *out_converted_char_p = converted_char;
  }

  return is_single_escape_character;
} /* lexer_convert_single_escape_character */

/**
 * Transform specified number of hexadecimal digits pointed by string iterator to character code
 *
 * @return true - upon successful conversion,
 *         false - otherwise (characters, pointed by iterator, are not hexadecimal digits,
 *                 or number of characters until end of string is less than specified).
 */
static bool
lexer_convert_escape_sequence_digits_to_char (lit_utf8_iterator_t *src_iter_p, /**< string iterator */
                                              bool is_unicode_escape_sequence, /**< UnicodeEscapeSequence (true)
                                                                                *   or HexEscapeSequence (false) */
                                              ecma_char_t *out_converted_char_p) /**< out: converted character */
{
  uint16_t char_code = 0;

  const uint32_t digits_num = is_unicode_escape_sequence ? 4 : 2;

  for (uint32_t i = 0; i < digits_num; i++)
  {
    if (lit_utf8_iterator_is_eos (src_iter_p))
    {
      return false;
    }

    const ecma_char_t next_char = lit_utf8_iterator_read_next (src_iter_p);

    if (!lit_char_is_hex_digit (next_char))
    {
      return false;
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

  *out_converted_char_p = (ecma_char_t) char_code;

  return true;
} /* lexer_convert_escape_sequence_digits_to_char */

/**
 * Transforming escape sequences in the charset, outputting converted string to specified buffer
 *
 * Note:
 *      Size of string with transformed escape sequences is always
 *      less or equal to size of corresponding source string.
 *
 * @return size of converted string
 */
static lit_utf8_size_t
lexer_transform_escape_sequences (const jerry_api_char_t *source_str_p, /**< string to convert,
                                                                         *   located in source buffer */
                                  lit_utf8_size_t source_str_size, /**< size of the string and of the output buffer */
                                  jerry_api_char_t *output_str_buf_p) /**< output buffer for converted string */
{
  if (source_str_size == 0)
  {
    return 0;
  }
  else
  {
    JERRY_ASSERT (source_str_p != NULL);
  }

  lit_utf8_byte_t *output_str_buf_iter_p = output_str_buf_p;
  const size_t output_str_buf_size = source_str_size;
  bool is_correct_sequence = true;

  lit_utf8_iterator_t source_str_iter = lit_utf8_iterator_create (source_str_p,
                                                                  source_str_size);

  ecma_char_t prev_converted_char = LIT_CHAR_NULL;

  while (!lit_utf8_iterator_is_eos (&source_str_iter))
  {
    ecma_char_t converted_char;

    const ecma_char_t next_char = lit_utf8_iterator_read_next (&source_str_iter);

    if (next_char == LIT_CHAR_BACKSLASH)
    {
      if (lit_utf8_iterator_is_eos (&source_str_iter))
      {
        is_correct_sequence = false;
        break;
      }

      const ecma_char_t char_after_next = lit_utf8_iterator_read_next (&source_str_iter);

      if (lit_char_is_decimal_digit (char_after_next))
      {
        if (lit_char_is_octal_digit (char_after_next))
        {
          if (char_after_next == LIT_CHAR_0
              && (lit_utf8_iterator_is_eos (&source_str_iter)
                  || !lit_char_is_octal_digit (lit_utf8_iterator_peek_next (&source_str_iter))))
          {
            lit_utf8_iterator_incr (&source_str_iter);

            converted_char = LIT_CHAR_NULL;
          }
          else
          {
            /* Implementation-defined (ECMA-262 v5, B.1.2): octal escape sequences are not implemented */
            is_correct_sequence = false;
            break;
          }
        }
        else
        {
          converted_char = char_after_next;
        }
      }
      else if (char_after_next == LIT_CHAR_LOWERCASE_U
               || char_after_next == LIT_CHAR_LOWERCASE_X)
      {
        if (!lexer_convert_escape_sequence_digits_to_char (&source_str_iter,
                                                           char_after_next == LIT_CHAR_LOWERCASE_U,
                                                           &converted_char))
        {
          is_correct_sequence = false;
          break;
        }
      }
      else if (lit_char_is_line_terminator (char_after_next))
      {
        /* Skip \, followed by a LineTerminatorSequence (ECMA-262, v5, 7.3) */
        if (char_after_next == LIT_CHAR_CR
            && !lit_utf8_iterator_is_eos (&source_str_iter))
        {
          if (lit_utf8_iterator_peek_next (&source_str_iter) == LIT_CHAR_LF)
          {
            lit_utf8_iterator_incr (&source_str_iter);
          }
        }

        continue;
      }
      else
      {
        lexer_convert_single_escape_character (char_after_next, &converted_char);
      }
    }
    else
    {
      converted_char = next_char;
    }

    if (lit_is_code_unit_high_surrogate (prev_converted_char)
        && lit_is_code_unit_low_surrogate (converted_char))
    {
      output_str_buf_iter_p -= LIT_UTF8_MAX_BYTES_IN_CODE_UNIT;

      lit_code_point_t code_point = lit_convert_surrogate_pair_to_code_point (prev_converted_char,
                                                                              converted_char);
      output_str_buf_iter_p += lit_code_point_to_utf8 (code_point, output_str_buf_iter_p);
    }
    else
    {
      output_str_buf_iter_p += lit_code_unit_to_utf8 (converted_char, output_str_buf_iter_p);
      JERRY_ASSERT (output_str_buf_iter_p <= output_str_buf_p + output_str_buf_size);
    }

    prev_converted_char = converted_char;
  }

  if (is_correct_sequence)
  {
    return (lit_utf8_size_t) (output_str_buf_iter_p - output_str_buf_p);
  }
  else
  {
    PARSE_ERROR ("Illegal escape sequence", source_str_p - buffer_start);
  }
} /* lexer_transform_escape_sequences */

/**
 * Create token of specified type from charset, transforming escape sequences.
 *
 * @return token descriptor
 */
static token
lexer_create_token_for_charset_transform_escape_sequences (token_type tt, /**< token type */
                                                           const lit_utf8_byte_t *charset_p, /**< charset buffer */
                                                           lit_utf8_size_t size) /**< size of the charset */
{
  lit_utf8_byte_t *converted_str_p = (lit_utf8_byte_t *) jsp_mm_alloc (size);

  lit_utf8_size_t converted_size = lexer_transform_escape_sequences (charset_p,
                                                                     size,
                                                                     converted_str_p);

  token ret = lexer_create_token_for_charset (tt,
                                              converted_str_p,
                                              converted_size);

  jsp_mm_free (converted_str_p);

  return ret;
} /* lexer_create_token_for_charset_transform_escape_sequences */

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
 * Parse Identifier (ECMA-262 v5, 7.6) or ReservedWord (7.6.1; 7.8.1; 7.8.2).
 *
 * @return TOK_NAME - for Identifier,
 *         TOK_KEYWORD - for Keyword or FutureReservedWord,
 *         TOK_NULL - for NullLiteral,
 *         TOK_BOOL - for BooleanLiteral
 */
static token
lexer_parse_identifier_or_keyword (void)
{
  ecma_char_t c = LA (0);

  JERRY_ASSERT (lexer_is_char_can_be_identifier_start (c));

  new_token ();

  bool is_correct_identifier_name = true;
  bool is_escape_sequence_occured = false;
  bool is_all_chars_were_lowercase_ascii = true;

  while (true)
  {
    c = LA (0);

    if (c == LIT_CHAR_BACKSLASH)
    {
      consume_char ();

      is_escape_sequence_occured = true;

      bool is_unicode_escape_sequence = (LA (0) == LIT_CHAR_LOWERCASE_U);
      consume_char ();

      if (is_unicode_escape_sequence)
      {
        /* UnicodeEscapeSequence */

        if (!lexer_convert_escape_sequence_digits_to_char (&src_iter,
                                                           true,
                                                           &c))
        {
          is_correct_identifier_name = false;
          break;
        }
        else
        {
          /* c now contains character, encoded in the UnicodeEscapeSequence */

          // Check character, converted from UnicodeEscapeSequence
          if (!lexer_is_char_can_be_identifier_part (c))
          {
            is_correct_identifier_name = false;
            break;
          }
        }
      }
      else
      {
        is_correct_identifier_name = false;
        break;
      }
    }
    else if (!lexer_is_char_can_be_identifier_part (c))
    {
      break;
    }
    else
    {
      if (!(c >= LIT_CHAR_ASCII_LOWERCASE_LETTERS_BEGIN
            && c <= LIT_CHAR_ASCII_LOWERCASE_LETTERS_END))
      {
        is_all_chars_were_lowercase_ascii = false;
      }

      consume_char ();
    }
  }

  if (!is_correct_identifier_name)
  {
    PARSE_ERROR ("Illegal identifier name", lit_utf8_iterator_get_offset (&src_iter));
  }

  const lit_utf8_size_t charset_size = (lit_utf8_size_t) (lit_utf8_iterator_get_ptr (&src_iter) - token_start);

  token ret = empty_token;

  if (!is_escape_sequence_occured
      && is_all_chars_were_lowercase_ascii)
  {
    /* Keyword or FutureReservedWord (TOK_KEYWORD), or boolean literal (TOK_BOOL), or null literal (TOK_NULL) */
    ret = lexer_parse_reserved_word (token_start, charset_size);
  }

  if (is_empty (ret))
  {
    /* Identifier (TOK_NAME) */

    if (!is_escape_sequence_occured)
    {
      ret = lexer_create_token_for_charset (TOK_NAME,
                                            token_start,
                                            charset_size);
    }
    else
    {
      ret = lexer_create_token_for_charset_transform_escape_sequences (TOK_NAME,
                                                                       token_start,
                                                                       charset_size);
    }
  }

  token_start = NULL;

  return ret;
} /* lexer_parse_identifier_or_keyword */

/**
 * Parse numeric literal (ECMA-262, v5, 7.8.3)
 *
 * @return token of TOK_SMALL_INT or TOK_NUMBER types
 */
static token
lexer_parse_number (void)
{
  ecma_char_t c = LA (0);
  bool is_hex = false;
  bool is_fp = false;
  ecma_number_t fp_res = .0;
  size_t tok_length = 0, i;
  token known_token;

  JERRY_ASSERT (lit_char_is_decimal_digit (c)
                || c == LIT_CHAR_DOT);

  if (c == LIT_CHAR_0)
  {
    if (LA (1) == LIT_CHAR_LOWERCASE_X
        || LA (1) == LIT_CHAR_UPPERCASE_X)
    {
      is_hex = true;
    }
  }
  else if (c == LIT_CHAR_DOT)
  {
    JERRY_ASSERT (lit_char_is_decimal_digit (LA (1)));
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
      if (!lit_char_is_hex_digit (c))
      {
        break;
      }
      consume_char ();
    }

    if (lexer_is_char_can_be_identifier_start (c))
    {
      PARSE_ERROR ("Identifier just after integer literal",
                   lit_utf8_iterator_get_offset (&src_iter));
    }

    tok_length = (size_t) (lit_utf8_iterator_get_ptr (&src_iter) - token_start);

    /* token is constructed at end of function */
    for (i = 0; i < tok_length; i++)
    {
      fp_res = fp_res * 16 + (ecma_number_t) lit_char_hex_to_int (token_start[i]);
    }
  }
  else
  {
    bool is_exp = false;

    new_token ();

    // Eat up '.'
    if (is_fp)
    {
      consume_char ();
    }

    while (true)
    {
      c = LA (0);

      if (c == LIT_CHAR_DOT)
      {
        if (is_fp)
        {
          /* token is constructed at end of function */
          break;
        }
        else
        {
          is_fp = true;
          consume_char ();

          continue;
        }
      }
      else if (c == LIT_CHAR_LOWERCASE_E
               || c == LIT_CHAR_UPPERCASE_E)
      {
        if (is_exp)
        {
          PARSE_ERROR ("Numeric literal shall not contain more than exponential marker ('e' or 'E')",
                       lit_utf8_iterator_get_offset (&src_iter));
        }
        else
        {
          is_exp = true;
          consume_char ();

          if (LA (0) == LIT_CHAR_MINUS
              || LA (0) == LIT_CHAR_PLUS)
          {
            consume_char ();
          }

          continue;
        }
      }
      else if (!lit_char_is_decimal_digit (c))
      {
        if (lexer_is_char_can_be_identifier_start (c))
        {
          PARSE_ERROR ("Numeric literal shall not contain non-numeric characters",
                       lit_utf8_iterator_get_offset (&src_iter));
        }

        /* token is constructed at end of function */
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
    else if (*token_start == LIT_CHAR_0
             && tok_length != 1)
    {
      /* Octal integer literals */
      if (strict_mode)
      {
        PARSE_ERROR ("Octal integer literals are not allowed in strict mode", token_start - buffer_start);
      }
      else
      {
        /* token is constructed at end of function */

        for (i = 0; i < tok_length; i++)
        {
          fp_res = fp_res * 8 + (ecma_number_t) lit_char_hex_to_int (token_start[i]);
        }
      }
    }
    else
    {
      /* token is constructed at end of function */

      for (i = 0; i < tok_length; i++)
      {
        fp_res = fp_res * 10 + (ecma_number_t) lit_char_hex_to_int (token_start[i]);
      }
    }
  }

  if (fp_res >= 0 && fp_res <= 255 && (uint8_t) fp_res == fp_res)
  {
    known_token = create_token (TOK_SMALL_INT, (uint8_t) fp_res);
    token_start = NULL;
    return known_token;
  }
  else
  {
    known_token = convert_seen_num_to_token (fp_res);
    token_start = NULL;
    return known_token;
  }
} /* lexer_parse_number */

/**
 * Parse string literal (ECMA-262 v5, 7.8.4)
 */
static token
lexer_parse_string (void)
{
  ecma_char_t c = (ecma_char_t) LA (0);
  JERRY_ASSERT (c == LIT_CHAR_SINGLE_QUOTE
                || c == LIT_CHAR_DOUBLE_QUOTE);

  /* Consume quote character */
  consume_char ();
  new_token ();

  const ecma_char_t end_char = c;

  bool is_escape_sequence_occured = false;

  do
  {
    c = LA (0);
    consume_char ();

    if (c == LIT_CHAR_NULL)
    {
      PARSE_ERROR ("Unclosed string", token_start - buffer_start);
    }
    else if (lit_char_is_line_terminator (c))
    {
      PARSE_ERROR ("String literal shall not contain newline character", token_start - buffer_start);
    }
    else if (c == LIT_CHAR_BACKSLASH)
    {
      is_escape_sequence_occured = true;

      ecma_char_t nc = (ecma_char_t) LA (0);
      consume_char ();

      if (nc == LIT_CHAR_CR)
      {
        if (LA (0) == LIT_CHAR_LF)
        {
          consume_char ();
        }
      }
    }
  }
  while (c != end_char);

  const lit_utf8_size_t charset_size = (lit_utf8_size_t) (lit_utf8_iterator_get_ptr (&src_iter) -
                                                          token_start) - 1;

  token ret;

  if (!is_escape_sequence_occured)
  {
    ret = lexer_create_token_for_charset (TOK_STRING,
                                          token_start,
                                          charset_size);
  }
  else
  {
    ret = lexer_create_token_for_charset_transform_escape_sequences (TOK_STRING,
                                                                     token_start,
                                                                     charset_size);
  }

  token_start = NULL;

  return ret;
} /* lexer_parse_string */

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

  result = lexer_create_token_for_charset (TOK_REGEXP,
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

  /* ECMA-262 v5, 7.6, Identifier */
  if (lexer_is_char_can_be_identifier_start (c))
  {
    return lexer_parse_identifier_or_keyword ();
  }

  /* ECMA-262 v5, 7.8.3, Numeric literal */
  if (lit_char_is_decimal_digit (c)
      || (c == LIT_CHAR_DOT
          && lit_char_is_decimal_digit (LA (1))))
  {
    return lexer_parse_number ();
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

  if (c == LIT_CHAR_SINGLE_QUOTE
      || c == LIT_CHAR_DOUBLE_QUOTE)
  {
    return lexer_parse_string ();
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

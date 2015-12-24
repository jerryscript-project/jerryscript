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
#include "jsp-early-error.h"
#include "rcs-records.h"

static token saved_token, prev_token, sent_token, empty_token;

static bool allow_dump_lines = false;
static size_t buffer_size = 0;

/*
 * FIXME:
 *       jerry_api_char_t should not be used outside of API implementation
 */

/* Represents the contents of a script.  */
static const jerry_api_char_t *buffer_start = NULL;
static lit_utf8_iterator_pos_t token_start_pos;
static bool is_token_parse_in_progress = false;

static lit_utf8_iterator_t src_iter;

#define LA(I)       (get_char (I))
#define TOK_START() (src_iter.buf_p + token_start_pos.offset)
#define TOK_SIZE() ((lit_utf8_size_t) (src_iter.buf_pos.offset - token_start_pos.offset))

static bool
is_empty (token tok)
{
  return lexer_get_token_type (tok) == TOK_EMPTY;
}

static locus
current_locus (void)
{
  if (is_token_parse_in_progress)
  {
    return token_start_pos;
  }
  else
  {
    return lit_utf8_iterator_get_pos (&src_iter);
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

/**
 * Dump current line
 */
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
    if (lit_char_is_line_terminator (code_unit))
    {
      if (code_unit == LIT_CHAR_CR
          && !lit_utf8_iterator_is_eos (&iter)
          && lit_utf8_iterator_peek_next (&iter) == LIT_CHAR_LF)
      {
        lit_utf8_iterator_incr (&iter);
      }
      break;
    }

    lit_put_ecma_char (code_unit);
  }

  lit_put_ecma_char (LIT_CHAR_LF);
} /* dump_current_line */

static token
create_token_from_lit (jsp_token_type_t type, lit_literal_t lit)
{
  token ret;

  ret.type = type;
  ret.loc = current_locus ();
  ret.uid = rcs_cpointer_compress (lit).packed_value;

  return ret;
}

/**
 * Create token of specified type
 *
 * @return token descriptor
 */
static token
create_token (jsp_token_type_t type, /**< type of token */
              uint16_t uid) /**< uid of token */
{
  token ret;

  ret.type = type;
  ret.loc = current_locus ();
  ret.uid = uid;

  return ret;
} /* create_token */

/**
 * Create token of specified type from charset
 *
 * @return token descriptor
 */
static token
lexer_create_token_for_charset (jsp_token_type_t tt, /**< token type */
                                const lit_utf8_byte_t *charset_p, /**< charset buffer */
                                lit_utf8_size_t size) /**< size of the charset */
{
  JERRY_ASSERT (charset_p != NULL);

  lit_utf8_iterator_t iter = lit_utf8_iterator_create (charset_p, (lit_utf8_size_t) size);
  lit_utf8_size_t new_size = 0;
  lit_utf8_size_t new_length = 0;
  bool should_convert = false;

  while (!lit_utf8_iterator_is_eos (&iter))
  {
    if (iter.buf_pos.is_non_bmp_middle)
    {
      should_convert = true;
    }
    lit_utf8_iterator_incr (&iter);
    new_size += LIT_CESU8_MAX_BYTES_IN_CODE_UNIT;
  }

  lit_utf8_byte_t *converted_str_p;

  if (unlikely (should_convert))
  {
    lit_utf8_iterator_seek_bos (&iter);
    converted_str_p = (lit_utf8_byte_t *) jsp_mm_alloc (new_size);

    while (!lit_utf8_iterator_is_eos (&iter))
    {
      ecma_char_t ch = lit_utf8_iterator_read_next (&iter);
      new_length += lit_code_unit_to_utf8 (ch, converted_str_p + new_length);
    }
  }
  else
  {
    converted_str_p = (lit_utf8_byte_t *) charset_p;
    new_length = size;
    JERRY_ASSERT (lit_is_cesu8_string_valid (converted_str_p, new_length));
  }

  lit_literal_t lit = lit_find_literal_by_utf8_string (converted_str_p, new_length);
  if (lit != NULL)
  {
    if (unlikely (should_convert))
    {
      jsp_mm_free (converted_str_p);
    }

    return create_token_from_lit (tt, lit);
  }
  lit = lit_create_literal_from_utf8_string (converted_str_p, new_length);
  rcs_record_type_t type = rcs_record_get_type (lit);

  JERRY_ASSERT (RCS_RECORD_TYPE_IS_CHARSET (type)
                || RCS_RECORD_TYPE_IS_MAGIC_STR (type)
                || RCS_RECORD_TYPE_IS_MAGIC_STR_EX (type));

  if (unlikely (should_convert))
  {
    jsp_mm_free (converted_str_p);
  }

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
            && !lit_utf8_iterator_is_eos (&source_str_iter)
            && lit_utf8_iterator_peek_next (&source_str_iter) == LIT_CHAR_LF)
        {
          lit_utf8_iterator_incr (&source_str_iter);
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

    if (lit_is_code_point_utf16_high_surrogate (prev_converted_char)
        && lit_is_code_point_utf16_low_surrogate (converted_char))
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
    PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "Illegal escape sequence", token_start_pos);
  }
} /* lexer_transform_escape_sequences */

/**
 * Create token of specified type from charset, transforming escape sequences.
 *
 * @return token descriptor
 */
static token
lexer_create_token_for_charset_transform_escape_sequences (jsp_token_type_t tt, /**< token type */
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
 * @return TOK_KW_* - for Keyword or FutureReservedWord,
 *         TOK_NULL - for NullLiteral,
 *         TOK_BOOL - for BooleanLiteral,
 *         TOK_EMPTY - for other tokens.
 */
static token
lexer_parse_reserved_word (const lit_utf8_byte_t *str_p, /**< characters buffer */
                           lit_utf8_size_t str_size, /**< string's length */
                           bool is_strict) /**< flag, indicating whether current code is in strict mode code */
{
  typedef struct
  {
    const char *keyword_p;
    jsp_token_type_t keyword_id;
  } kw_descr_t;

  const kw_descr_t keywords[] =
  {
#define KW_DESCR(literal, keyword_id) { literal, keyword_id }
    KW_DESCR ("break", TOK_KW_BREAK),
    KW_DESCR ("case", TOK_KW_CASE),
    KW_DESCR ("catch", TOK_KW_CATCH),
    KW_DESCR ("class", TOK_KW_CLASS),
    KW_DESCR ("const", TOK_KW_CONST),
    KW_DESCR ("continue", TOK_KW_CONTINUE),
    KW_DESCR ("debugger", TOK_KW_DEBUGGER),
    KW_DESCR ("default", TOK_KW_DEFAULT),
    KW_DESCR ("delete", TOK_KW_DELETE),
    KW_DESCR ("do", TOK_KW_DO),
    KW_DESCR ("else", TOK_KW_ELSE),
    KW_DESCR ("enum", TOK_KW_ENUM),
    KW_DESCR ("export", TOK_KW_EXPORT),
    KW_DESCR ("extends", TOK_KW_EXTENDS),
    KW_DESCR ("finally", TOK_KW_FINALLY),
    KW_DESCR ("for", TOK_KW_FOR),
    KW_DESCR ("function", TOK_KW_FUNCTION),
    KW_DESCR ("if", TOK_KW_IF),
    KW_DESCR ("in", TOK_KW_IN),
    KW_DESCR ("instanceof", TOK_KW_INSTANCEOF),
    KW_DESCR ("interface", TOK_KW_INTERFACE),
    KW_DESCR ("import", TOK_KW_IMPORT),
    KW_DESCR ("implements", TOK_KW_IMPLEMENTS),
    KW_DESCR ("let", TOK_KW_LET),
    KW_DESCR ("new", TOK_KW_NEW),
    KW_DESCR ("package", TOK_KW_PACKAGE),
    KW_DESCR ("private", TOK_KW_PRIVATE),
    KW_DESCR ("protected", TOK_KW_PROTECTED),
    KW_DESCR ("public", TOK_KW_PUBLIC),
    KW_DESCR ("return", TOK_KW_RETURN),
    KW_DESCR ("static", TOK_KW_STATIC),
    KW_DESCR ("super", TOK_KW_SUPER),
    KW_DESCR ("switch", TOK_KW_SWITCH),
    KW_DESCR ("this", TOK_KW_THIS),
    KW_DESCR ("throw", TOK_KW_THROW),
    KW_DESCR ("try", TOK_KW_TRY),
    KW_DESCR ("typeof", TOK_KW_TYPEOF),
    KW_DESCR ("var", TOK_KW_VAR),
    KW_DESCR ("void", TOK_KW_VOID),
    KW_DESCR ("while", TOK_KW_WHILE),
    KW_DESCR ("with", TOK_KW_WITH),
    KW_DESCR ("yield", TOK_KW_YIELD)
#undef KW_DESCR
  };

  jsp_token_type_t kw = TOK_EMPTY;

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

  if (!is_strict)
  {
    switch (kw)
    {
      case TOK_KW_INTERFACE:
      case TOK_KW_IMPLEMENTS:
      case TOK_KW_LET:
      case TOK_KW_PACKAGE:
      case TOK_KW_PRIVATE:
      case TOK_KW_PROTECTED:
      case TOK_KW_PUBLIC:
      case TOK_KW_STATIC:
      case TOK_KW_YIELD:
      {
        return empty_token;
      }

      default:
      {
        break;
      }
    }
  }

  if (kw != TOK_EMPTY)
  {
    return create_token (kw, 0);
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
  lit_literal_t lit = lit_find_literal_by_num (num);
  if (lit != NULL)
  {
    return create_token_from_lit (TOK_NUMBER, lit);
  }

  return create_token_from_lit (TOK_NUMBER, lit_create_literal_from_num (num));
}

static void
new_token (void)
{
  token_start_pos = lit_utf8_iterator_get_pos (&src_iter);
  JERRY_ASSERT (!token_start_pos.is_non_bmp_middle);
  is_token_parse_in_progress = true;
}

static void
consume_char (void)
{
  if (!lit_utf8_iterator_is_eos (&src_iter))
  {
    lit_utf8_iterator_incr (&src_iter);
  }
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
 *         TOK_KW_* - for Keyword or FutureReservedWord,
 *         TOK_NULL - for NullLiteral,
 *         TOK_BOOL - for BooleanLiteral
 */
static token
lexer_parse_identifier_or_keyword (bool is_strict) /**< flag, indicating whether current code is in strict mode code */
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
    PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "Illegal identifier name", lit_utf8_iterator_get_pos (&src_iter));
  }

  const lit_utf8_size_t charset_size = TOK_SIZE ();

  token ret = empty_token;

  if (!is_escape_sequence_occured
      && is_all_chars_were_lowercase_ascii)
  {
    /* Keyword or FutureReservedWord (TOK_KW_*), or boolean literal (TOK_BOOL), or null literal (TOK_NULL) */
    ret = lexer_parse_reserved_word (TOK_START (), charset_size, is_strict);
  }

  if (is_empty (ret))
  {
    /* Identifier (TOK_NAME) */

    if (!is_escape_sequence_occured)
    {
      ret = lexer_create_token_for_charset (TOK_NAME,
                                            TOK_START (),
                                            charset_size);
    }
    else
    {
      ret = lexer_create_token_for_charset_transform_escape_sequences (TOK_NAME,
                                                                       TOK_START (),
                                                                       charset_size);
    }
  }

  is_token_parse_in_progress = false;

  return ret;
} /* lexer_parse_identifier_or_keyword */

/**
 * Parse numeric literal (ECMA-262, v5, 7.8.3)
 *
 * @return token of TOK_SMALL_INT or TOK_NUMBER types
 */
static token
lexer_parse_number (bool is_strict) /**< flag, indicating whether current code is in strict mode code */
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
    new_token ();

    // Eat up '0x'
    consume_char ();
    consume_char ();

    c = LA (0);
    if (!lit_char_is_hex_digit (c))
    {
      PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "Invalid HexIntegerLiteral", lit_utf8_iterator_get_pos (&src_iter));
    }

    do
    {
      consume_char ();
      c = LA (0);
    }
    while (lit_char_is_hex_digit (c));

    if (lexer_is_char_can_be_identifier_start (c))
    {
      PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX,
                   "Identifier just after integer literal",
                   lit_utf8_iterator_get_pos (&src_iter));
    }

    tok_length = (size_t) (TOK_SIZE ());

    const size_t length_of_zero_and_x_str = 2u;
    const lit_utf8_byte_t *fp_buf_p = TOK_START () + length_of_zero_and_x_str;

    /* token is constructed at end of function */
    for (i = 0; i < tok_length - length_of_zero_and_x_str; i++)
    {
      fp_res = fp_res * 16 + (ecma_number_t) lit_char_hex_to_int (fp_buf_p[i]);
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
        if (is_fp || is_exp)
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
          PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX,
                       "Numeric literal shall not contain more than exponential marker ('e' or 'E')",
                       lit_utf8_iterator_get_pos (&src_iter));
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

          if (!lit_char_is_decimal_digit (LA (0)))
          {
            PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX,
                         "Exponential mark in a numeric literal should followed by a singed interger",
                         lit_utf8_iterator_get_pos (&src_iter));
          }

          continue;
        }
      }
      else if (!lit_char_is_decimal_digit (c))
      {
        if (lexer_is_char_can_be_identifier_start (c))
        {
          PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX,
                       "Numeric literal shall not contain non-numeric characters",
                       lit_utf8_iterator_get_pos (&src_iter));
        }

        /* token is constructed at end of function */
        break;
      }

      consume_char ();
    }

    tok_length = (size_t) (TOK_SIZE ());

    if (is_fp || is_exp)
    {
      ecma_number_t res = ecma_utf8_string_to_number (TOK_START (), (jerry_api_size_t) tok_length);
      JERRY_ASSERT (!ecma_number_is_nan (res));

      known_token = convert_seen_num_to_token (res);
      is_token_parse_in_progress = NULL;

      return known_token;
    }
    else if (*TOK_START () == LIT_CHAR_0
             && tok_length != 1)
    {
      /* Octal integer literals */
      if (is_strict)
      {
        PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "Octal integer literals are not allowed in strict mode", token_start_pos);
      }
      else
      {
        /* token is constructed at end of function */
        const lit_utf8_byte_t *fp_buf_p = TOK_START ();

        for (i = 0; i < tok_length; i++)
        {
          fp_res = fp_res * 8 + (ecma_number_t) lit_char_hex_to_int (fp_buf_p[i]);
        }
      }
    }
    else
    {
      const lit_utf8_byte_t *fp_buf_p = TOK_START ();
      /* token is constructed at end of function */

      ecma_number_t mult = 1.0f;
      for (i = tok_length; i > 0; i--, mult *= 10)
      {
        fp_res += (ecma_number_t) lit_char_hex_to_int (fp_buf_p[i - 1]) * mult;
      }
    }
  }

  if (fp_res >= 0 && fp_res <= 255 && (uint8_t) fp_res == fp_res)
  {
    known_token = create_token (TOK_SMALL_INT, (uint8_t) fp_res);
    is_token_parse_in_progress = NULL;
    return known_token;
  }
  else
  {
    known_token = convert_seen_num_to_token (fp_res);
    is_token_parse_in_progress = NULL;
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


  new_token ();

  /* Consume quote character */
  consume_char ();

  const ecma_char_t end_char = c;

  bool is_escape_sequence_occured = false;

  do
  {
    c = LA (0);
    consume_char ();

    if (lit_char_is_line_terminator (c))
    {
      PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "String literal shall not contain newline character", token_start_pos);
    }
    else if (c == LIT_CHAR_BACKSLASH)
    {
      is_escape_sequence_occured = true;

      ecma_char_t nc = (ecma_char_t) LA (0);
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
  while (c != end_char && !lit_utf8_iterator_is_eos (&src_iter));

  if (c == LIT_CHAR_NULL)
  {
    PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "Unclosed string", token_start_pos);
  }

  const lit_utf8_size_t charset_size = TOK_SIZE () - 2;

  token ret;

  if (!is_escape_sequence_occured)
  {
    ret = lexer_create_token_for_charset (TOK_STRING,
                                   TOK_START () + 1,
                                   charset_size);
  }
  else
  {
    ret = lexer_create_token_for_charset_transform_escape_sequences (TOK_STRING,
                                                                     TOK_START () + 1,
                                                                     charset_size);
  }

  is_token_parse_in_progress = false;

  return ret;
} /* lexer_parse_string */

/**
 * Parse string literal (ECMA-262 v5, 7.8.5)
 */
static token
lexer_parse_regexp (void)
{
  token result;
  bool is_char_class = false;

  JERRY_ASSERT (LA (0) == LIT_CHAR_SLASH);
  new_token ();

  /* Eat up '/' */
  consume_char ();

  while (true)
  {
    if (lit_utf8_iterator_is_eos (&src_iter))
    {
      PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "Unterminated RegExp literal", token_start_pos);
    }

    ecma_char_t c = (ecma_char_t) LA (0);
    if (lit_char_is_line_terminator (c))
    {
      PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "RegExp literal should not contain newline character", token_start_pos);
    }
    else if (c == LIT_CHAR_BACKSLASH)
    {
      consume_char ();
      if (lit_char_is_line_terminator (LA (0)))
      {
        PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX,
                     "RegExp literal backslash sequence should not contain newline character",
                     token_start_pos);
      }
    }
    else if (c == LIT_CHAR_LEFT_SQUARE)
    {
      is_char_class = true;
    }
    else if (c == LIT_CHAR_RIGHT_SQUARE)
    {
      is_char_class = false;
    }
    else if (c == LIT_CHAR_SLASH && !is_char_class)
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

    if (!lit_char_is_word_char (c) || lit_char_is_line_terminator (c))
    {
      break;
    }

    consume_char ();
  }

  result = lexer_create_token_for_charset (TOK_REGEXP, TOK_START () + 1, TOK_SIZE () - 1);

  is_token_parse_in_progress = false;
  return result;
} /* lexer_parse_regexp */

/**
 * Parse a comment
 *
 * @return true if newline was met during parsing
 *         false - otherwise
 */
static bool
lexer_parse_comment (void)
{
  ecma_char_t c = LA (0);
  bool multiline;
  bool was_newlines = false;

  JERRY_ASSERT (LA (0) == LIT_CHAR_SLASH);
  JERRY_ASSERT (LA (1) == LIT_CHAR_SLASH || LA (1) == LIT_CHAR_ASTERISK);

  multiline = (LA (1) == LIT_CHAR_ASTERISK);

  consume_char ();
  consume_char ();

  while (!lit_utf8_iterator_is_eos (&src_iter))
  {
    c = LA (0);

    if (!multiline)
    {
      if (lit_char_is_line_terminator (c))
      {
        return true;
      }
    }
    else
    {
      if (c == LIT_CHAR_ASTERISK
          && LA (1) == LIT_CHAR_SLASH)
      {
        consume_char ();
        consume_char ();

        return was_newlines;
      }
      else if (lit_char_is_line_terminator (c))
      {
        was_newlines = true;
      }
    }

    consume_char ();
  }

  if (multiline)
  {
    PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "Unclosed multiline comment", lit_utf8_iterator_get_pos (&src_iter));
  }

  return false;
} /* lexer_parse_comment */

/**
 * Skip any whitespace and comment tokens
 *
 * @return true - if a newline token was skipped,
 *         false - otherwise
 */
static bool
lexer_skip_whitespace_and_comments (void)
{
  bool new_lines_occurred = false;

  while (true)
  {
    ecma_char_t c = LA (0);

    if (lit_char_is_white_space (c))
    {
      do
      {
        consume_char ();

        c = LA (0);
      }
      while (lit_char_is_white_space (c));
    }
    else if (lit_char_is_line_terminator (c))
    {
      dump_current_line ();

      new_lines_occurred = true;

      do
      {
        consume_char ();

        c = LA (0);
      }
      while (lit_char_is_line_terminator (c));
    }
    else if (c == LIT_CHAR_SLASH
             && (LA (1) == LIT_CHAR_SLASH
                 || LA (1) == LIT_CHAR_ASTERISK))
    {
      /* ECMA-262 v5, 7.4, SingleLineComment or MultiLineComment */

      if (lexer_parse_comment ())
      {
        new_lines_occurred = true;
      }
    }
    else
    {
      break;
    }
  }

  return new_lines_occurred;
} /* lexer_skip_whitespace_and_comments */

/**
 * Parse and construct lexer token
 *
 * Note:
 *      Currently, lexer token doesn't fully correspond to Token, defined in ECMA-262, v5, 7.5.
 *      For example, there is no new-line token type in the token definition of ECMA-262 v5.
 *
 * Note:
 *      For Lexer alone, it is hard to find out a token is whether a regexp or a division.
 *      Parser must set maybe_regexp to true if a regexp is expected.
 *      Otherwise, a division is expected.
 *
 * @return constructed token
 */
static token
lexer_parse_token (bool maybe_regexp, /**< read '/' as regexp? */
                   bool *out_is_preceed_by_new_lines_p, /**< out: is constructed token preceded by newlines? */
                   bool is_strict) /**< flag, indicating whether current code is in strict mode code */

{
  JERRY_ASSERT (is_token_parse_in_progress == false);

  *out_is_preceed_by_new_lines_p = lexer_skip_whitespace_and_comments ();

  ecma_char_t c = LA (0);

  /* ECMA-262 v5, 7.6, Identifier */
  if (lexer_is_char_can_be_identifier_start (c))
  {
    return lexer_parse_identifier_or_keyword (is_strict);
  }

  /* ECMA-262 v5, 7.8.3, Numeric literal */
  if (lit_char_is_decimal_digit (c)
      || (c == LIT_CHAR_DOT
          && lit_char_is_decimal_digit (LA (1))))
  {
    return lexer_parse_number (is_strict);
  }

  if (c == LIT_CHAR_NULL)
  {
    return create_token (TOK_EOF, 0);
  }

  if (c == LIT_CHAR_SINGLE_QUOTE
      || c == LIT_CHAR_DOUBLE_QUOTE)
  {
    return lexer_parse_string ();
  }

  if (c == LIT_CHAR_SLASH && maybe_regexp)
  {
    return lexer_parse_regexp ();
  }

  /* ECMA-262 v5, 7.7, Punctuator */
  switch (c)
  {
    case LIT_CHAR_LEFT_BRACE:
    {
      RETURN_PUNC (TOK_OPEN_BRACE);
      break;
    }
    case LIT_CHAR_RIGHT_BRACE:
    {
      RETURN_PUNC (TOK_CLOSE_BRACE);
      break;
    }
    case LIT_CHAR_LEFT_PAREN:
    {
      RETURN_PUNC (TOK_OPEN_PAREN);
      break;
    }
    case LIT_CHAR_RIGHT_PAREN:
    {
      RETURN_PUNC (TOK_CLOSE_PAREN);
      break;
    }
    case LIT_CHAR_LEFT_SQUARE:
    {
      RETURN_PUNC (TOK_OPEN_SQUARE);
      break;
    }
    case LIT_CHAR_RIGHT_SQUARE:
    {
      RETURN_PUNC (TOK_CLOSE_SQUARE);
      break;
    }
    case LIT_CHAR_DOT:
    {
      RETURN_PUNC (TOK_DOT);
      break;
    }
    case LIT_CHAR_SEMICOLON:
    {
      RETURN_PUNC (TOK_SEMICOLON);
      break;
    }
    case LIT_CHAR_COMMA:
    {
      RETURN_PUNC (TOK_COMMA);
      break;
    }
    case LIT_CHAR_TILDE:
    {
      RETURN_PUNC (TOK_COMPL);
      break;
    }
    case LIT_CHAR_COLON:
    {
      RETURN_PUNC (TOK_COLON);
      break;
    }
    case LIT_CHAR_QUESTION:
    {
      RETURN_PUNC (TOK_QUERY);
      break;
    }

    case LIT_CHAR_ASTERISK:
    {
      IF_LA_IS (LIT_CHAR_EQUALS, TOK_MULT_EQ, TOK_MULT);
      break;
    }
    case LIT_CHAR_SLASH:
    {
      IF_LA_IS (LIT_CHAR_EQUALS, TOK_DIV_EQ, TOK_DIV);
      break;
    }
    case LIT_CHAR_CIRCUMFLEX:
    {
      IF_LA_IS (LIT_CHAR_EQUALS, TOK_XOR_EQ, TOK_XOR);
      break;
    }
    case LIT_CHAR_PERCENT:
    {
      IF_LA_IS (LIT_CHAR_EQUALS, TOK_MOD_EQ, TOK_MOD);
      break;
    }
    case LIT_CHAR_PLUS:
    {
      IF_LA_IS_OR (LIT_CHAR_PLUS, TOK_DOUBLE_PLUS, LIT_CHAR_EQUALS, TOK_PLUS_EQ, TOK_PLUS);
      break;
    }
    case LIT_CHAR_MINUS:
    {
      IF_LA_IS_OR (LIT_CHAR_MINUS, TOK_DOUBLE_MINUS, LIT_CHAR_EQUALS, TOK_MINUS_EQ, TOK_MINUS);
      break;
    }
    case LIT_CHAR_AMPERSAND:
    {
      IF_LA_IS_OR (LIT_CHAR_AMPERSAND, TOK_DOUBLE_AND, LIT_CHAR_EQUALS, TOK_AND_EQ, TOK_AND);
      break;
    }
    case LIT_CHAR_VLINE:
    {
      IF_LA_IS_OR (LIT_CHAR_VLINE, TOK_DOUBLE_OR, LIT_CHAR_EQUALS, TOK_OR_EQ, TOK_OR);
      break;
    }
    case LIT_CHAR_LESS_THAN:
    {
      switch (LA (1))
      {
        case LIT_CHAR_LESS_THAN: IF_LA_N_IS (LIT_CHAR_EQUALS, TOK_LSHIFT_EQ, TOK_LSHIFT, 2); break;
        case LIT_CHAR_EQUALS: RETURN_PUNC_EX (TOK_LESS_EQ, 2); break;
        default: RETURN_PUNC (TOK_LESS);
      }
      break;
    }
    case LIT_CHAR_GREATER_THAN:
    {
      switch (LA (1))
      {
        case LIT_CHAR_GREATER_THAN:
        {
          switch (LA (2))
          {
            case LIT_CHAR_GREATER_THAN: IF_LA_N_IS (LIT_CHAR_EQUALS, TOK_RSHIFT_EX_EQ, TOK_RSHIFT_EX, 3); break;
            case LIT_CHAR_EQUALS: RETURN_PUNC_EX (TOK_RSHIFT_EQ, 3); break;
            default: RETURN_PUNC_EX (TOK_RSHIFT, 2);
          }
          break;
        }
        case LIT_CHAR_EQUALS: RETURN_PUNC_EX (TOK_GREATER_EQ, 2); break;
        default: RETURN_PUNC (TOK_GREATER);
      }
      break;
    }
    case LIT_CHAR_EQUALS:
    {
      if (LA (1) == LIT_CHAR_EQUALS)
      {
        IF_LA_N_IS (LIT_CHAR_EQUALS, TOK_TRIPLE_EQ, TOK_DOUBLE_EQ, 2);
      }
      else
      {
        RETURN_PUNC (TOK_EQ);
      }
      break;
    }
    case LIT_CHAR_EXCLAMATION:
    {
      if (LA (1) == LIT_CHAR_EQUALS)
      {
        IF_LA_N_IS (LIT_CHAR_EQUALS, TOK_NOT_DOUBLE_EQ, TOK_NOT_EQ, 2);
      }
      else
      {
        RETURN_PUNC (TOK_NOT);
      }
      break;
    }
  }

  PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "Illegal character", lit_utf8_iterator_get_pos (&src_iter));
} /* lexer_parse_token */

/**
 * Construct next token from current source code position and increment the position
 *
 * @return the constructed token
 */
token
lexer_next_token (bool maybe_regexp, /**< read '/' as regexp? */
                  bool is_strict) /**< strict mode is on (true) / off (false) */
{
  lit_utf8_iterator_pos_t src_pos = lit_utf8_iterator_get_pos (&src_iter);
  if (src_pos.offset == 0 && !src_pos.is_non_bmp_middle)
  {
    dump_current_line ();
  }

  if (!is_empty (saved_token))
  {
    sent_token = saved_token;
    saved_token = empty_token;
  }
  else
  {
    /**
     * FIXME:
     *       The way to raise syntax errors for unexpected EOF
     *       should be reworked so that EOF would be checked by
     *       caller of the routine, and the following condition
     *       would be checked as assertion in the routine.
     */
    if (lexer_get_token_type (prev_token) == TOK_EOF
        && lexer_get_token_type (sent_token) == TOK_EOF)
    {
      PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "Unexpected EOF", lit_utf8_iterator_get_pos (&src_iter));
    }

    prev_token = sent_token;

    jsp_token_flag_t flags = JSP_TOKEN_FLAG__NO_FLAGS;

    bool is_preceded_by_new_lines;
    sent_token = lexer_parse_token (maybe_regexp, &is_preceded_by_new_lines, is_strict);

    if (is_preceded_by_new_lines)
    {
      flags = (jsp_token_flag_t) (flags | JSP_TOKEN_FLAG_PRECEDED_BY_NEWLINES);
    }

    sent_token.flags = flags;
  }

  return sent_token;
} /* lexer_next_token */

/**
 * Set lexer's iteraror over source file to the specified position
 */
void
lexer_seek (lit_utf8_iterator_pos_t locus) /**< position in the source buffer */
{
  JERRY_ASSERT (is_token_parse_in_progress == false);

  lit_utf8_iterator_seek (&src_iter, locus);
  saved_token = empty_token;
  prev_token = empty_token;
} /* lexer_seek */

/**
 * Convert locus to line and column
 */
void
lexer_locus_to_line_and_column (lit_utf8_iterator_pos_t locus, /**< iterator position in the source script */
                                size_t *line, /**< @out: line number */
                                size_t *column) /**< @out: column number */
{
  JERRY_ASSERT ((lit_utf8_size_t) (locus.offset + locus.is_non_bmp_middle) <= buffer_size);

  lit_utf8_iterator_t iter = lit_utf8_iterator_create (buffer_start, (lit_utf8_size_t) buffer_size);
  lit_utf8_iterator_pos_t iter_pos = lit_utf8_iterator_get_pos (&iter);

  size_t l = 0, c = 0;
  while (!lit_utf8_iterator_is_eos (&iter) && lit_utf8_iterator_pos_cmp (iter_pos, locus) < 0)
  {
    ecma_char_t code_unit = lit_utf8_iterator_read_next (&iter);
    iter_pos = lit_utf8_iterator_get_pos (&iter);

    if (lit_char_is_line_terminator (code_unit))
    {
      if (code_unit == LIT_CHAR_CR
          && !lit_utf8_iterator_is_eos (&iter)
          && lit_utf8_iterator_peek_next (&iter) == LIT_CHAR_LF)
      {
        lit_utf8_iterator_incr (&iter);
      }
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
} /* lexer_locus_to_line_and_column */

/**
 * Dump specified line of the source script
 */
void
lexer_dump_line (size_t line) /**< line number */
{
  size_t l = 0;
  lit_utf8_iterator_t iter = src_iter;

  lit_utf8_iterator_seek_bos (&iter);

  while (!lit_utf8_iterator_is_eos (&iter))
  {
    ecma_char_t code_unit;

    if (l == line)
    {
      while (!lit_utf8_iterator_is_eos (&iter))
      {
        code_unit = lit_utf8_iterator_read_next (&iter);
        if (lit_char_is_line_terminator (code_unit))
        {
          break;
        }
        lit_put_ecma_char (code_unit);
      }
      return;
    }

    code_unit = lit_utf8_iterator_read_next (&iter);

    if (lit_char_is_line_terminator (code_unit))
    {
      l++;
      if (code_unit == LIT_CHAR_CR
          && !lit_utf8_iterator_is_eos (&iter)
          && lit_utf8_iterator_peek_next (&iter) == LIT_CHAR_LF)
      {
        lit_utf8_iterator_incr (&iter);
      }
    }
  }
} /* lexer_dump_line */

/**
 * Convert token type to string
 *
 * @return string, describing token type
 */
const char *
lexer_token_type_to_string (jsp_token_type_t tt)
{
  switch (tt)
  {
    case TOK_EOF: return "End of file";
    case TOK_NAME: return "Identifier";
    case TOK_SMALL_INT: /* FALLTHRU */
    case TOK_NUMBER: return "Number";
    case TOK_REGEXP: return "RegExp";

    case TOK_NULL: return "null";
    case TOK_BOOL: return "bool";
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
    case TOK_KW_BREAK: return "break";
    case TOK_KW_CASE: return "case";
    case TOK_KW_CATCH: return "catch";
    case TOK_KW_CLASS: return "class";

    case TOK_KW_CONST: return "const";
    case TOK_KW_CONTINUE: return "continue";
    case TOK_KW_DEBUGGER: return "debugger";
    case TOK_KW_DEFAULT: return "default";
    case TOK_KW_DELETE: return "delete";

    case TOK_KW_DO: return "do";
    case TOK_KW_ELSE: return "else";
    case TOK_KW_ENUM: return "enum";
    case TOK_KW_EXPORT: return "export";
    case TOK_KW_EXTENDS: return "extends";

    case TOK_KW_FINALLY: return "finally";
    case TOK_KW_FOR: return "for";
    case TOK_KW_FUNCTION: return "function";
    case TOK_KW_IF: return "if";
    case TOK_KW_IN: return "in";

    case TOK_KW_INSTANCEOF: return "instanceof";
    case TOK_KW_INTERFACE: return "interface";
    case TOK_KW_IMPORT: return "import";
    case TOK_KW_IMPLEMENTS: return "implements";
    case TOK_KW_LET: return "let";

    case TOK_KW_NEW: return "new";
    case TOK_KW_PACKAGE: return "package";
    case TOK_KW_PRIVATE: return "private";
    case TOK_KW_PROTECTED: return "protected";
    case TOK_KW_PUBLIC: return "public";

    case TOK_KW_RETURN: return "return";
    case TOK_KW_STATIC: return "static";
    case TOK_KW_SUPER: return "super";
    case TOK_KW_SWITCH: return "switch";
    case TOK_KW_THIS: return "this";

    case TOK_KW_THROW: return "throw";
    case TOK_KW_TRY: return "try";
    case TOK_KW_TYPEOF: return "typeof";
    case TOK_KW_VAR: return "var";
    case TOK_KW_VOID: return "void";

    case TOK_KW_WHILE: return "while";
    case TOK_KW_WITH: return "with";
    case TOK_KW_YIELD: return "yield";
    default: JERRY_UNREACHABLE ();
  }
} /* lexer_token_type_to_string */

/**
 * Get type of specified token
 *
 * @return token type
 */
jsp_token_type_t __attr_always_inline___
lexer_get_token_type (token t) /**< the token */
{
  JERRY_ASSERT (t.type >= TOKEN_TYPE__BEGIN && t.type <= TOKEN_TYPE__END);

  return (jsp_token_type_t) t.type;
} /* lexer_get_token_type */

bool __attr_always_inline___
lexer_is_preceded_by_newlines (token t)
{
  return ((t.flags & JSP_TOKEN_FLAG_PRECEDED_BY_NEWLINES) != 0);
} /* lexer_is_preceded_by_newlines */

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
  JERRY_ASSERT (lexer_get_token_type (id1) == TOK_NAME);
  JERRY_ASSERT (lexer_get_token_type (id2) == TOK_NAME);

  return (id1.uid == id2.uid);
} /* lexer_are_tokens_with_same_identifier */

/**
 * Checks that TOK_STRING doesn't contain EscapeSequence or LineContinuation
 *
 * @return true, if token's string in source buffer doesn't contain backslash
 *         false, otherwise
 */
bool
lexer_is_no_escape_sequences_in_token_string (token tok) /**< token of type TOK_STRING */
{
  JERRY_ASSERT (lexer_get_token_type (tok) == TOK_STRING);

  lit_utf8_iterator_t iter = src_iter;
  lit_utf8_iterator_seek (&iter, tok.loc);

  JERRY_ASSERT (!lit_utf8_iterator_is_eos (&iter));
  ecma_char_t c = lit_utf8_iterator_read_next (&iter);
  JERRY_ASSERT (c == LIT_CHAR_SINGLE_QUOTE
                || c == LIT_CHAR_DOUBLE_QUOTE);

  const ecma_char_t end_char = c;

  do
  {
    JERRY_ASSERT (!lit_utf8_iterator_is_eos (&iter));
    c = lit_utf8_iterator_read_next (&iter);

    if (c == LIT_CHAR_BACKSLASH)
    {
      return false;
    }
  }
  while (c != end_char);

  return true;
} /* lexer_is_no_escape_sequences_in_token_string */

/**
 * Initialize lexer to start parsing of a new source
 */
void
lexer_init (const jerry_api_char_t *source, /**< script source */
            size_t source_size, /**< script source size in bytes */
            bool is_print_source_code) /**< flag indicating whether to dump
                                        *   processed source code */
{
  empty_token.type = TOK_EMPTY;
  empty_token.uid = 0;
  empty_token.loc = LIT_ITERATOR_POS_ZERO;

  saved_token = prev_token = sent_token = empty_token;

  if (!lit_is_utf8_string_valid (source, (lit_utf8_size_t) source_size))
  {
    PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "Invalid source encoding", LIT_ITERATOR_POS_ZERO);
  }

  src_iter = lit_utf8_iterator_create (source, (lit_utf8_size_t) source_size);

  buffer_size = source_size;
  buffer_start = source;
  is_token_parse_in_progress = false;

#ifndef JERRY_NDEBUG
  allow_dump_lines = is_print_source_code;
#else /* JERRY_NDEBUG */
  (void) is_print_source_code;
  allow_dump_lines = false;
#endif /* JERRY_NDEBUG */
} /* lexer_init */

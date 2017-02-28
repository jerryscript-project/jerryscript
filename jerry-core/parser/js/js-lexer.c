/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include "ecma-alloc.h"
#include "ecma-helpers.h"
#include "ecma-function-object.h"
#include "ecma-literal-storage.h"
#include "js-parser-internal.h"
#include "lit-char-helpers.h"

#ifdef JERRY_JS_PARSER

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup jsparser JavaScript
 * @{
 *
 * \addtogroup jsparser_lexer Lexer
 * @{
 */

#define IS_UTF8_INTERMEDIATE_OCTET(byte) (((byte) & LIT_UTF8_EXTRA_BYTE_MASK) == LIT_UTF8_2_BYTE_CODE_POINT_MIN)

/**
 * Align column to the next tab position.
 *
 * @return aligned position
 */
static parser_line_counter_t
align_column_to_tab (parser_line_counter_t column) /**< current column */
{
  /* Tab aligns to zero column start position. */
  return (parser_line_counter_t) (((column + (8u - 1u)) & ~ECMA_STRING_CONTAINER_MASK) + 1u);
} /* align_column_to_tab */

/**
 * Parse hexadecimal character sequence
 *
 * @return character value
 */
ecma_char_t
lexer_hex_to_character (parser_context_t *context_p, /**< context */
                        const uint8_t *source_p, /**< current source position */
                        int length) /**< source length */
{
  uint32_t result = 0;

  do
  {
    uint32_t byte = *source_p++;

    result <<= 4;

    if (byte >= LIT_CHAR_0 && byte <= LIT_CHAR_9)
    {
      result += byte - LIT_CHAR_0;
    }
    else
    {
      byte = LEXER_TO_ASCII_LOWERCASE (byte);
      if (byte >= LIT_CHAR_LOWERCASE_A && byte <= LIT_CHAR_LOWERCASE_F)
      {
        result += byte - (LIT_CHAR_LOWERCASE_A - 10);
      }
      else
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_ESCAPE_SEQUENCE);
      }
    }
  }
  while (--length > 0);

  return (ecma_char_t) result;
} /* lexer_hex_to_character */

/**
 * Skip space mode
 */
typedef enum
{
  LEXER_SKIP_SPACES,                 /**< skip spaces mode */
  LEXER_SKIP_SINGLE_LINE_COMMENT,    /**< parse single line comment */
  LEXER_SKIP_MULTI_LINE_COMMENT,     /**< parse multi line comment */
} skip_mode_t;

/**
 * Skip spaces.
 */
static void
skip_spaces (parser_context_t *context_p) /**< context */
{
  skip_mode_t mode = LEXER_SKIP_SPACES;
  const uint8_t *source_end_p = context_p->source_end_p;

  context_p->token.was_newline = 0;

  while (true)
  {
    if (context_p->source_p >= source_end_p)
    {
      if (mode == LEXER_SKIP_MULTI_LINE_COMMENT)
      {
        parser_raise_error (context_p, PARSER_ERR_UNTERMINATED_MULTILINE_COMMENT);
      }
      return;
    }

    switch (context_p->source_p[0])
    {
      case LIT_CHAR_CR:
      {
        if (context_p->source_p + 1 < source_end_p
            && context_p->source_p[1] == LIT_CHAR_LF)
        {
          context_p->source_p++;
        }
        /* FALLTHRU */
      }

      case LIT_CHAR_LF:
      {
        context_p->line++;
        context_p->column = 0;
        context_p->token.was_newline = 1;

        if (mode == LEXER_SKIP_SINGLE_LINE_COMMENT)
        {
          mode = LEXER_SKIP_SPACES;
        }
        /* FALLTHRU */
      }

      case LIT_CHAR_VTAB:
      case LIT_CHAR_FF:
      case LIT_CHAR_SP:
      {
        context_p->source_p++;
        context_p->column++;
        continue;
      }

      case LIT_CHAR_TAB:
      {
        context_p->column = align_column_to_tab (context_p->column);
        context_p->source_p++;
        continue;
      }

      case LIT_CHAR_SLASH:
      {
        if (mode == LEXER_SKIP_SPACES
            && context_p->source_p + 1 < source_end_p)
        {
          if (context_p->source_p[1] == LIT_CHAR_SLASH)
          {
            mode = LEXER_SKIP_SINGLE_LINE_COMMENT;
          }
          else if (context_p->source_p[1] == LIT_CHAR_ASTERISK)
          {
            mode = LEXER_SKIP_MULTI_LINE_COMMENT;
            context_p->token.line = context_p->line;
            context_p->token.column = context_p->column;
          }

          if (mode != LEXER_SKIP_SPACES)
          {
            context_p->source_p += 2;
            PARSER_PLUS_EQUAL_LC (context_p->column, 2);
            continue;
          }
        }
        break;
      }

      case LIT_CHAR_ASTERISK:
      {
        if (mode == LEXER_SKIP_MULTI_LINE_COMMENT
            && context_p->source_p + 1 < source_end_p
            && context_p->source_p[1] == LIT_CHAR_SLASH)
        {
          mode = LEXER_SKIP_SPACES;
          context_p->source_p += 2;
          PARSER_PLUS_EQUAL_LC (context_p->column, 2);
          continue;
        }
        break;
      }

      case 0xc2:
      {
        if (context_p->source_p + 1 < source_end_p
            && context_p->source_p[1] == 0xa0)
        {
          /* Codepoint \u00A0 */
          context_p->source_p += 2;
          context_p->column++;
          continue;
        }
        break;
      }

      case LEXER_NEWLINE_LS_PS_BYTE_1:
      {
        JERRY_ASSERT (context_p->source_p + 2 < source_end_p);
        if (LEXER_NEWLINE_LS_PS_BYTE_23 (context_p->source_p))
        {
          /* Codepoint \u2028 and \u2029 */
          context_p->source_p += 3;
          context_p->line++;
          context_p->column = 1;
          context_p->token.was_newline = 1;

          if (mode == LEXER_SKIP_SINGLE_LINE_COMMENT)
          {
            mode = LEXER_SKIP_SPACES;
          }
          continue;
        }
        break;
      }

      case 0xef:
      {
        if (context_p->source_p + 2 < source_end_p
            && context_p->source_p[1] == 0xbb
            && context_p->source_p[2] == 0xbf)
        {
          /* Codepoint \uFEFF */
          context_p->source_p += 3;
          context_p->column++;
          continue;
        }
        break;
      }

      default:
      {
        break;
      }
    }

    if (mode == LEXER_SKIP_SPACES)
    {
      return;
    }

    context_p->source_p++;

    if (context_p->source_p < source_end_p
        && IS_UTF8_INTERMEDIATE_OCTET (context_p->source_p[0]))
    {
      context_p->column++;
    }
  }
} /* skip_spaces */

/**
 * Keyword data.
 */
typedef struct
{
  const uint8_t *keyword_p;     /**< keyword string */
  lexer_token_type_t type;      /**< keyword token type */
} keyword_string_t;

#define LEXER_KEYWORD(name, type) { (const uint8_t *) (name), (type) }
#define LEXER_KEYWORD_END()       { (const uint8_t *) NULL, LEXER_EOS }

/**
 * Keywords with 2 characters.
 */
static const keyword_string_t keyword_length_2[4] =
{
  LEXER_KEYWORD ("do", LEXER_KEYW_DO),
  LEXER_KEYWORD ("if", LEXER_KEYW_IF),
  LEXER_KEYWORD ("in", LEXER_KEYW_IN),
  LEXER_KEYWORD_END ()
};

/**
 * Keywords with 3 characters.
 */
static const keyword_string_t keyword_length_3[6] =
{
  LEXER_KEYWORD ("for", LEXER_KEYW_FOR),
  LEXER_KEYWORD ("let", LEXER_KEYW_LET),
  LEXER_KEYWORD ("new", LEXER_KEYW_NEW),
  LEXER_KEYWORD ("try", LEXER_KEYW_TRY),
  LEXER_KEYWORD ("var", LEXER_KEYW_VAR),
  LEXER_KEYWORD_END ()
};

/**
 * Keywords with 4 characters.
 */
static const keyword_string_t keyword_length_4[9] =
{
  LEXER_KEYWORD ("case", LEXER_KEYW_CASE),
  LEXER_KEYWORD ("else", LEXER_KEYW_ELSE),
  LEXER_KEYWORD ("enum", LEXER_KEYW_ENUM),
  LEXER_KEYWORD ("null", LEXER_LIT_NULL),
  LEXER_KEYWORD ("this", LEXER_KEYW_THIS),
  LEXER_KEYWORD ("true", LEXER_LIT_TRUE),
  LEXER_KEYWORD ("void", LEXER_KEYW_VOID),
  LEXER_KEYWORD ("with", LEXER_KEYW_WITH),
  LEXER_KEYWORD_END ()
};

/**
 * Keywords with 5 characters.
 */
static const keyword_string_t keyword_length_5[10] =
{
  LEXER_KEYWORD ("break", LEXER_KEYW_BREAK),
  LEXER_KEYWORD ("catch", LEXER_KEYW_CATCH),
  LEXER_KEYWORD ("class", LEXER_KEYW_CLASS),
  LEXER_KEYWORD ("const", LEXER_KEYW_CONST),
  LEXER_KEYWORD ("false", LEXER_LIT_FALSE),
  LEXER_KEYWORD ("super", LEXER_KEYW_SUPER),
  LEXER_KEYWORD ("throw", LEXER_KEYW_THROW),
  LEXER_KEYWORD ("while", LEXER_KEYW_WHILE),
  LEXER_KEYWORD ("yield", LEXER_KEYW_YIELD),
  LEXER_KEYWORD_END ()
};

/**
 * Keywords with 6 characters.
 */
static const keyword_string_t keyword_length_6[9] =
{
  LEXER_KEYWORD ("delete", LEXER_KEYW_DELETE),
  LEXER_KEYWORD ("export", LEXER_KEYW_EXPORT),
  LEXER_KEYWORD ("import", LEXER_KEYW_IMPORT),
  LEXER_KEYWORD ("public", LEXER_KEYW_PUBLIC),
  LEXER_KEYWORD ("return", LEXER_KEYW_RETURN),
  LEXER_KEYWORD ("static", LEXER_KEYW_STATIC),
  LEXER_KEYWORD ("switch", LEXER_KEYW_SWITCH),
  LEXER_KEYWORD ("typeof", LEXER_KEYW_TYPEOF),
  LEXER_KEYWORD_END ()
};

/**
 * Keywords with 7 characters.
 */
static const keyword_string_t keyword_length_7[6] =
{
  LEXER_KEYWORD ("default", LEXER_KEYW_DEFAULT),
  LEXER_KEYWORD ("extends", LEXER_KEYW_EXTENDS),
  LEXER_KEYWORD ("finally", LEXER_KEYW_FINALLY),
  LEXER_KEYWORD ("package", LEXER_KEYW_PACKAGE),
  LEXER_KEYWORD ("private", LEXER_KEYW_PRIVATE),
  LEXER_KEYWORD_END ()
};

/**
 * Keywords with 8 characters.
 */
static const keyword_string_t keyword_length_8[4] =
{
  LEXER_KEYWORD ("continue", LEXER_KEYW_CONTINUE),
  LEXER_KEYWORD ("debugger", LEXER_KEYW_DEBUGGER),
  LEXER_KEYWORD ("function", LEXER_KEYW_FUNCTION),
  LEXER_KEYWORD_END ()
};

/**
 * Keywords with 9 characters.
 */
static const keyword_string_t keyword_length_9[3] =
{
  LEXER_KEYWORD ("interface", LEXER_KEYW_INTERFACE),
  LEXER_KEYWORD ("protected", LEXER_KEYW_PROTECTED),
  LEXER_KEYWORD_END ()
};

/**
 * Keywords with 10 characters.
 */
static const keyword_string_t keyword_length_10[3] =
{
  LEXER_KEYWORD ("implements", LEXER_KEYW_IMPLEMENTS),
  LEXER_KEYWORD ("instanceof", LEXER_KEYW_INSTANCEOF),
  LEXER_KEYWORD_END ()
};

/**
 * List to the keywords.
 */
static const keyword_string_t * const keyword_string_list[9] =
{
  keyword_length_2,
  keyword_length_3,
  keyword_length_4,
  keyword_length_5,
  keyword_length_6,
  keyword_length_7,
  keyword_length_8,
  keyword_length_9,
  keyword_length_10
};

#undef LEXER_KEYWORD
#undef LEXER_KEYWORD_END

/**
 * Parse identifier.
 */
static void
lexer_parse_identifier (parser_context_t *context_p, /**< context */
                        bool check_keywords) /**< check keywords */
{
  /* Only very few identifiers contains \u escape sequences. */
  const uint8_t *source_p = context_p->source_p;
  const uint8_t *ident_start_p = context_p->source_p;
  /* Note: newline or tab cannot be part of an identifier. */
  parser_line_counter_t column = context_p->column;
  const uint8_t *source_end_p = context_p->source_end_p;
  size_t length = 0;

  context_p->token.type = LEXER_LITERAL;
  context_p->token.literal_is_reserved = false;
  context_p->token.lit_location.type = LEXER_IDENT_LITERAL;
  context_p->token.lit_location.has_escape = false;

  do
  {
    if (*source_p == LIT_CHAR_BACKSLASH)
    {
      uint16_t character;

      context_p->token.lit_location.has_escape = true;
      context_p->source_p = source_p;
      context_p->token.column = column;

      if ((source_p + 6 > source_end_p) || (source_p[1] != LIT_CHAR_LOWERCASE_U))
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_UNICODE_ESCAPE_SEQUENCE);
      }

      character = lexer_hex_to_character (context_p, source_p + 2, 4);

      if (length == 0)
      {
        if (!lit_char_is_identifier_start_character (character))
        {
          parser_raise_error (context_p, PARSER_ERR_INVALID_IDENTIFIER_START);
        }
      }
      else
      {
        if (!lit_char_is_identifier_part_character (character))
        {
          parser_raise_error (context_p, PARSER_ERR_INVALID_IDENTIFIER_PART);
        }
      }

      length += lit_char_get_utf8_length (character);
      source_p += 6;
      PARSER_PLUS_EQUAL_LC (column, 6);
      continue;
    }

    /* Valid identifiers cannot contain 4 byte long utf-8
     * characters, since those characters are represented
     * by 2 ecmascript (UTF-16) characters, and those
     * characters cannot be literal characters. */
    JERRY_ASSERT (source_p[0] < LEXER_UTF8_4BYTE_START);

    source_p++;
    length++;
    column++;

    while (source_p < source_end_p
           && IS_UTF8_INTERMEDIATE_OCTET (source_p[0]))
    {
      source_p++;
      length++;
    }
  }
  while (source_p < source_end_p
         && (lit_char_is_identifier_part (source_p) || *source_p == LIT_CHAR_BACKSLASH));

  context_p->source_p = ident_start_p;
  context_p->token.column = context_p->column;

  if (length > PARSER_MAXIMUM_IDENT_LENGTH)
  {
    parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_TOO_LONG);
  }

  /* Check keywords (Only if there is no \u escape sequence in the pattern). */
  if (check_keywords
      && !context_p->token.lit_location.has_escape
      && (length >= 2 && length <= 10))
  {
    const keyword_string_t *keyword_p = keyword_string_list[length - 2];

    do
    {
      if (ident_start_p[0] == keyword_p->keyword_p[0]
          && ident_start_p[1] == keyword_p->keyword_p[1]
          && memcmp (ident_start_p, keyword_p->keyword_p, length) == 0)
      {
        if (keyword_p->type >= LEXER_FIRST_FUTURE_STRICT_RESERVED_WORD)
        {
          if (context_p->status_flags & PARSER_IS_STRICT)
          {
            parser_raise_error (context_p, PARSER_ERR_STRICT_IDENT_NOT_ALLOWED);
          }

          context_p->token.literal_is_reserved = true;
          break;
        }

        context_p->token.type = keyword_p->type;
        break;
      }
      keyword_p++;
    }
    while (keyword_p->type != LEXER_EOS);
  }

  if (context_p->token.type == LEXER_LITERAL)
  {
    /* Fill literal data. */
    context_p->token.lit_location.char_p = ident_start_p;
    context_p->token.lit_location.length = (uint16_t) length;
  }

  context_p->source_p = source_p;
  context_p->column = column;
} /* lexer_parse_identifier */

/**
 * Parse string.
 */
static void
lexer_parse_string (parser_context_t *context_p) /**< context */
{
  uint8_t str_end_character = context_p->source_p[0];
  const uint8_t *source_p = context_p->source_p + 1;
  const uint8_t *string_start_p = source_p;
  const uint8_t *source_end_p = context_p->source_end_p;
  parser_line_counter_t line = context_p->line;
  parser_line_counter_t column = (parser_line_counter_t) (context_p->column + 1);
  parser_line_counter_t original_line = line;
  parser_line_counter_t original_column = column;
  size_t length = 0;
  uint8_t has_escape = false;

  while (true)
  {
    if (source_p >= source_end_p)
    {
      context_p->token.line = original_line;
      context_p->token.column = (parser_line_counter_t) (original_column - 1);
      parser_raise_error (context_p, PARSER_ERR_UNTERMINATED_STRING);
    }

    if (*source_p == str_end_character)
    {
      break;
    }

    if (*source_p == LIT_CHAR_BACKSLASH)
    {
      source_p++;
      column++;
      if (source_p >= source_end_p)
      {
        /* Will throw an unterminated string error. */
        continue;
      }

      has_escape = true;

      /* Newline is ignored. */
      if (*source_p == LIT_CHAR_CR
          || *source_p == LIT_CHAR_LF
          || (*source_p == LEXER_NEWLINE_LS_PS_BYTE_1 && LEXER_NEWLINE_LS_PS_BYTE_23 (source_p)))
      {
        if (*source_p == LIT_CHAR_CR)
        {
          source_p++;
          if (source_p < source_end_p
              && *source_p == LIT_CHAR_LF)
          {
            source_p++;
          }
        }
        else if (*source_p == LIT_CHAR_LF)
        {
          source_p++;
        }
        else
        {
          source_p += 3;
        }

        line++;
        column = 1;
        continue;
      }

      /* Except \x, \u, and octal numbers, everything is
       * converted to a character which has the same byte length. */
      if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_3)
      {
        if (context_p->status_flags & PARSER_IS_STRICT)
        {
          parser_raise_error (context_p, PARSER_ERR_OCTAL_ESCAPE_NOT_ALLOWED);
        }

        source_p++;
        column++;

        if (source_p < source_end_p && *source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
        {
          source_p++;
          column++;

          if (source_p < source_end_p && *source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
          {
            /* Numbers >= 0x200 (0x80) requires
             * two bytes for encoding in UTF-8. */
            if (source_p[-2] >= LIT_CHAR_2)
            {
              length++;
            }

            source_p++;
            column++;
          }
        }

        length++;
        continue;
      }

      if (*source_p >= LIT_CHAR_4 && *source_p <= LIT_CHAR_7)
      {
        if (context_p->status_flags & PARSER_IS_STRICT)
        {
          parser_raise_error (context_p, PARSER_ERR_OCTAL_ESCAPE_NOT_ALLOWED);
        }

        source_p++;
        column++;

        if (source_p < source_end_p && *source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
        {
          source_p++;
          column++;
        }

        /* The maximum number is 0x4d so the UTF-8
         * representation is always one byte. */
        length++;
        continue;
      }

      if (*source_p == LIT_CHAR_LOWERCASE_X || *source_p == LIT_CHAR_LOWERCASE_U)
      {
        uint8_t hex_part_length = (*source_p == LIT_CHAR_LOWERCASE_X) ? 2 : 4;

        context_p->token.line = line;
        context_p->token.column = (parser_line_counter_t) (column - 1);
        if (source_p + 1 + hex_part_length > source_end_p)
        {
          parser_raise_error (context_p, PARSER_ERR_INVALID_ESCAPE_SEQUENCE);
        }

        length += lit_char_get_utf8_length (lexer_hex_to_character (context_p,
                                                                    source_p + 1,
                                                                    hex_part_length));
        source_p += hex_part_length + 1;
        PARSER_PLUS_EQUAL_LC (column, hex_part_length + 1u);
        continue;
      }
    }

    if (*source_p >= LEXER_UTF8_4BYTE_START)
    {
      /* Processing 4 byte unicode sequence (even if it is
       * after a backslash). Always converted to two 3 byte
       * long sequence. */
      length += 2 * 3;
      has_escape = true;
      source_p += 4;
      column++;
      continue;
    }
    else if (*source_p == LIT_CHAR_CR
             || *source_p == LIT_CHAR_LF
             || (*source_p == LEXER_NEWLINE_LS_PS_BYTE_1 && LEXER_NEWLINE_LS_PS_BYTE_23 (source_p)))
    {
      context_p->token.line = line;
      context_p->token.column = column;
      parser_raise_error (context_p, PARSER_ERR_NEWLINE_NOT_ALLOWED);
    }
    else if (*source_p == LIT_CHAR_TAB)
    {
      column = align_column_to_tab (column);
      /* Subtract -1 because column is increased below. */
      column--;
    }

    source_p++;
    column++;
    length++;

    while (source_p < source_end_p
           && IS_UTF8_INTERMEDIATE_OCTET (*source_p))
    {
      source_p++;
      length++;
    }
  }

  if (length > PARSER_MAXIMUM_STRING_LENGTH)
  {
    parser_raise_error (context_p, PARSER_ERR_STRING_TOO_LONG);
  }

  context_p->token.type = LEXER_LITERAL;

  /* Fill literal data. */
  context_p->token.lit_location.char_p = string_start_p;
  context_p->token.lit_location.length = (uint16_t) length;
  context_p->token.lit_location.type = LEXER_STRING_LITERAL;
  context_p->token.lit_location.has_escape = has_escape;

  context_p->source_p = source_p + 1;
  context_p->line = line;
  context_p->column = (parser_line_counter_t) (column + 1);
} /* lexer_parse_string */

/**
 * Parse number.
 */
static void
lexer_parse_number (parser_context_t *context_p) /**< context */
{
  const uint8_t *source_p = context_p->source_p;
  const uint8_t *source_end_p = context_p->source_end_p;
  bool can_be_float = false;
  size_t length;

  context_p->token.type = LEXER_LITERAL;
  context_p->token.literal_is_reserved = false;
  context_p->token.extra_value = LEXER_NUMBER_DECIMAL;
  context_p->token.lit_location.char_p = source_p;
  context_p->token.lit_location.type = LEXER_NUMBER_LITERAL;
  context_p->token.lit_location.has_escape = false;

  if (source_p[0] == LIT_CHAR_0
      && source_p + 1 < source_end_p)
  {
    if (LEXER_TO_ASCII_LOWERCASE (source_p[1]) == LIT_CHAR_LOWERCASE_X)
    {
      context_p->token.extra_value = LEXER_NUMBER_HEXADECIMAL;
      source_p += 2;

      if (source_p >= source_end_p
          || !lit_char_is_hex_digit (source_p[0]))
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_HEX_DIGIT);
      }

      do
      {
        source_p++;
      }
      while (source_p < source_end_p
             && lit_char_is_hex_digit (source_p[0]));
    }
    else if (source_p[1] >= LIT_CHAR_0
             && source_p[1] <= LIT_CHAR_7)
    {
      context_p->token.extra_value = LEXER_NUMBER_OCTAL;

      if (context_p->status_flags & PARSER_IS_STRICT)
      {
        parser_raise_error (context_p, PARSER_ERR_OCTAL_NUMBER_NOT_ALLOWED);
      }

      do
      {
        source_p++;
      }
      while (source_p < source_end_p
             && source_p[0] >= LIT_CHAR_0
             && source_p[0] <= LIT_CHAR_7);

      if (source_p < source_end_p
          && source_p[0] >= LIT_CHAR_8
          && source_p[0] <= LIT_CHAR_9)
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_NUMBER);
      }
    }
    else if (source_p[1] >= LIT_CHAR_8
             && source_p[1] <= LIT_CHAR_9)
    {
      parser_raise_error (context_p, PARSER_ERR_INVALID_NUMBER);
    }
    else
    {
      can_be_float = true;
      source_p++;
    }
  }
  else
  {
    do
    {
      source_p++;
    }
    while (source_p < source_end_p
           && source_p[0] >= LIT_CHAR_0
           && source_p[0] <= LIT_CHAR_9);

    can_be_float = true;
  }

  if (can_be_float)
  {
    if (source_p < source_end_p
        && source_p[0] == LIT_CHAR_DOT)
    {
      source_p++;
      while (source_p < source_end_p
             && source_p[0] >= LIT_CHAR_0
             && source_p[0] <= LIT_CHAR_9)
      {
        source_p++;
      }
    }

    if (source_p < source_end_p
        && LEXER_TO_ASCII_LOWERCASE (source_p[0]) == LIT_CHAR_LOWERCASE_E)
    {
      source_p++;

      if (source_p < source_end_p
          && (source_p[0] == LIT_CHAR_PLUS || source_p[0] == LIT_CHAR_MINUS))
      {
        source_p++;
      }

      if (source_p >= source_end_p
          || source_p[0] < LIT_CHAR_0
          || source_p[0] > LIT_CHAR_9)
      {
        parser_raise_error (context_p, PARSER_ERR_MISSING_EXPONENT);
      }

      do
      {
        source_p++;
      }
      while (source_p < source_end_p
             && source_p[0] >= LIT_CHAR_0
             && source_p[0] <= LIT_CHAR_9);
    }
  }

  if (source_p < source_end_p
      && (lit_char_is_identifier_start (source_p) || source_p[0] == LIT_CHAR_BACKSLASH))
  {
    parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_AFTER_NUMBER);
  }

  length = (size_t) (source_p - context_p->source_p);
  if (length > PARSER_MAXIMUM_IDENT_LENGTH)
  {
    parser_raise_error (context_p, PARSER_ERR_NUMBER_TOO_LONG);
  }

  context_p->token.lit_location.length = (uint16_t) length;
  PARSER_PLUS_EQUAL_LC (context_p->column, length);
  context_p->source_p = source_p;
} /* lexer_parse_number */

#define LEXER_TYPE_A_TOKEN(char1, type1) \
  case (uint8_t) (char1) : \
  { \
    context_p->token.type = (type1); \
    length = 1; \
    break; \
  }

#define LEXER_TYPE_B_TOKEN(char1, type1, char2, type2) \
  case (uint8_t) (char1) : \
  { \
    if (length >= 2 && context_p->source_p[1] == (uint8_t) (char2)) \
    { \
      context_p->token.type = (type2); \
      length = 2; \
      break; \
    } \
    \
    context_p->token.type = (type1); \
    length = 1; \
    break; \
  }

#define LEXER_TYPE_C_TOKEN(char1, type1, char2, type2, char3, type3) \
  case (uint8_t) (char1) : \
  { \
    if (length >= 2) \
    { \
      if (context_p->source_p[1] == (uint8_t) (char2)) \
      { \
        context_p->token.type = (type2); \
        length = 2; \
        break; \
      } \
      \
      if (context_p->source_p[1] == (uint8_t) (char3)) \
      { \
        context_p->token.type = (type3); \
        length = 2; \
        break; \
      } \
    } \
    \
    context_p->token.type = (type1); \
    length = 1; \
    break; \
  }

#define LEXER_TYPE_D_TOKEN(char1, type1, char2, type2, char3, type3) \
  case (uint8_t) (char1) : \
  { \
    if (length >= 2 && context_p->source_p[1] == (uint8_t) (char2)) \
    { \
      if (length >= 3 && context_p->source_p[2] == (uint8_t) (char3)) \
      { \
        context_p->token.type = (type3); \
        length = 3; \
        break; \
      } \
      \
      context_p->token.type = (type2); \
      length = 2; \
      break; \
    } \
    \
    context_p->token.type = (type1); \
    length = 1; \
    break; \
  }

/**
 * Get next token.
 */
void
lexer_next_token (parser_context_t *context_p) /**< context */
{
  size_t length;

  skip_spaces (context_p);

  context_p->token.line = context_p->line;
  context_p->token.column = context_p->column;

  length = (size_t) (context_p->source_end_p - context_p->source_p);
  if (length == 0)
  {
    context_p->token.type = LEXER_EOS;
    return;
  }

  if (lit_char_is_identifier_start (context_p->source_p)
      || context_p->source_p[0] == LIT_CHAR_BACKSLASH)
  {
    lexer_parse_identifier (context_p, true);
    return;
  }

  if (context_p->source_p[0] >= LIT_CHAR_0 && context_p->source_p[0] <= LIT_CHAR_9)
  {
    lexer_parse_number (context_p);
    return;
  }

  switch (context_p->source_p[0])
  {
    LEXER_TYPE_A_TOKEN (LIT_CHAR_LEFT_BRACE, LEXER_LEFT_BRACE);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_LEFT_PAREN, LEXER_LEFT_PAREN);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_LEFT_SQUARE, LEXER_LEFT_SQUARE);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_RIGHT_BRACE, LEXER_RIGHT_BRACE);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_RIGHT_PAREN, LEXER_RIGHT_PAREN);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_RIGHT_SQUARE, LEXER_RIGHT_SQUARE);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_SEMICOLON, LEXER_SEMICOLON);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_COMMA, LEXER_COMMA);

    case (uint8_t) LIT_CHAR_DOT :
    {
      if (length >= 2
          && (context_p->source_p[1] >= LIT_CHAR_0 && context_p->source_p[1] <= LIT_CHAR_9))
      {
        lexer_parse_number (context_p);
        return;
      }

      context_p->token.type = LEXER_DOT;
      length = 1;
      break;
    }

    case (uint8_t) LIT_CHAR_LESS_THAN:
    {
      if (length >= 2)
      {
        if (context_p->source_p[1] == (uint8_t) LIT_CHAR_EQUALS)
        {
          context_p->token.type = LEXER_LESS_EQUAL;
          length = 2;
          break;
        }

        if (context_p->source_p[1] == (uint8_t) LIT_CHAR_LESS_THAN)
        {
          if (length >= 3 && context_p->source_p[2] == (uint8_t) LIT_CHAR_EQUALS)
          {
            context_p->token.type = LEXER_ASSIGN_LEFT_SHIFT;
            length = 3;
            break;
          }

          context_p->token.type = LEXER_LEFT_SHIFT;
          length = 2;
          break;
        }
      }

      context_p->token.type = LEXER_LESS;
      length = 1;
      break;
    }

    case LIT_CHAR_GREATER_THAN:
    {
      if (length >= 2)
      {
        if (context_p->source_p[1] == (uint8_t) LIT_CHAR_EQUALS)
        {
          context_p->token.type = LEXER_GREATER_EQUAL;
          length = 2;
          break;
        }

        if (context_p->source_p[1] == (uint8_t) LIT_CHAR_GREATER_THAN)
        {
          if (length >= 3)
          {
            if (context_p->source_p[2] == (uint8_t) LIT_CHAR_EQUALS)
            {
              context_p->token.type = LEXER_ASSIGN_RIGHT_SHIFT;
              length = 3;
              break;
            }

            if (context_p->source_p[2] == (uint8_t) LIT_CHAR_GREATER_THAN)
            {
              if (length >= 4 && context_p->source_p[3] == (uint8_t) LIT_CHAR_EQUALS)
              {
                context_p->token.type = LEXER_ASSIGN_UNS_RIGHT_SHIFT;
                length = 4;
                break;
              }

              context_p->token.type = LEXER_UNS_RIGHT_SHIFT;
              length = 3;
              break;
            }
          }

          context_p->token.type = LEXER_RIGHT_SHIFT;
          length = 2;
          break;
        }
      }

      context_p->token.type = LEXER_GREATER;
      length = 1;
      break;
    }

    LEXER_TYPE_D_TOKEN (LIT_CHAR_EQUALS, LEXER_ASSIGN, LIT_CHAR_EQUALS,
                        LEXER_EQUAL, LIT_CHAR_EQUALS, LEXER_STRICT_EQUAL)
    LEXER_TYPE_D_TOKEN (LIT_CHAR_EXCLAMATION, LEXER_LOGICAL_NOT, LIT_CHAR_EQUALS,
                        LEXER_NOT_EQUAL, LIT_CHAR_EQUALS, LEXER_STRICT_NOT_EQUAL)

    LEXER_TYPE_C_TOKEN (LIT_CHAR_PLUS, LEXER_ADD, LIT_CHAR_EQUALS,
                        LEXER_ASSIGN_ADD, LIT_CHAR_PLUS, LEXER_INCREASE)
    LEXER_TYPE_C_TOKEN (LIT_CHAR_MINUS, LEXER_SUBTRACT, LIT_CHAR_EQUALS,
                        LEXER_ASSIGN_SUBTRACT, LIT_CHAR_MINUS, LEXER_DECREASE)

    LEXER_TYPE_B_TOKEN (LIT_CHAR_ASTERISK, LEXER_MULTIPLY, LIT_CHAR_EQUALS,
                        LEXER_ASSIGN_MULTIPLY)
    LEXER_TYPE_B_TOKEN (LIT_CHAR_SLASH, LEXER_DIVIDE, LIT_CHAR_EQUALS,
                        LEXER_ASSIGN_DIVIDE)
    LEXER_TYPE_B_TOKEN (LIT_CHAR_PERCENT, LEXER_MODULO, LIT_CHAR_EQUALS,
                        LEXER_ASSIGN_MODULO)

    LEXER_TYPE_C_TOKEN (LIT_CHAR_AMPERSAND, LEXER_BIT_AND, LIT_CHAR_EQUALS,
                        LEXER_ASSIGN_BIT_AND, LIT_CHAR_AMPERSAND, LEXER_LOGICAL_AND)
    LEXER_TYPE_C_TOKEN (LIT_CHAR_VLINE, LEXER_BIT_OR, LIT_CHAR_EQUALS,
                        LEXER_ASSIGN_BIT_OR, LIT_CHAR_VLINE, LEXER_LOGICAL_OR)

    LEXER_TYPE_B_TOKEN (LIT_CHAR_CIRCUMFLEX, LEXER_BIT_XOR, LIT_CHAR_EQUALS,
                        LEXER_ASSIGN_BIT_XOR)

    LEXER_TYPE_A_TOKEN (LIT_CHAR_TILDE, LEXER_BIT_NOT);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_QUESTION, LEXER_QUESTION_MARK);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_COLON, LEXER_COLON);

    case LIT_CHAR_SINGLE_QUOTE:
    case LIT_CHAR_DOUBLE_QUOTE:
    {
      lexer_parse_string (context_p);
      return;
    }

    default:
    {
      parser_raise_error (context_p, PARSER_ERR_INVALID_CHARACTER);
    }
  }

  context_p->source_p += length;
  PARSER_PLUS_EQUAL_LC (context_p->column, length);
} /* lexer_next_token */

#undef LEXER_TYPE_A_TOKEN
#undef LEXER_TYPE_B_TOKEN
#undef LEXER_TYPE_C_TOKEN
#undef LEXER_TYPE_D_TOKEN

/**
 * Search or append the string to the literal pool.
 */
static void
lexer_process_char_literal (parser_context_t *context_p, /**< context */
                            const uint8_t *char_p, /**< characters */
                            size_t length, /**< length of string */
                            uint8_t literal_type, /**< final literal type */
                            bool has_escape) /**< has escape sequences */
{
  parser_list_iterator_t literal_iterator;
  lexer_literal_t *literal_p;
  uint32_t literal_index = 0;

  JERRY_ASSERT (literal_type == LEXER_IDENT_LITERAL
                || literal_type == LEXER_STRING_LITERAL);

  JERRY_ASSERT (literal_type != LEXER_IDENT_LITERAL || length <= PARSER_MAXIMUM_IDENT_LENGTH);
  JERRY_ASSERT (literal_type != LEXER_STRING_LITERAL || length <= PARSER_MAXIMUM_STRING_LENGTH);

  parser_list_iterator_init (&context_p->literal_pool, &literal_iterator);

  while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
  {
    if (literal_p->type == literal_type
        && literal_p->prop.length == length
        && memcmp (literal_p->u.char_p, char_p, length) == 0)
    {
      context_p->lit_object.literal_p = literal_p;
      context_p->lit_object.index = (uint16_t) literal_index;
      return;
    }

    literal_index++;
  }

  JERRY_ASSERT (literal_index == context_p->literal_count);

  if (literal_index >= PARSER_MAXIMUM_NUMBER_OF_LITERALS)
  {
    parser_raise_error (context_p, PARSER_ERR_LITERAL_LIMIT_REACHED);
  }

  literal_p = (lexer_literal_t *) parser_list_append (context_p, &context_p->literal_pool);
  literal_p->prop.length = (uint16_t) length;
  literal_p->type = literal_type;
  literal_p->status_flags = has_escape ? 0 : LEXER_FLAG_SOURCE_PTR;

  if (has_escape)
  {
    literal_p->u.char_p = (uint8_t *) jmem_heap_alloc_block (length);
    memcpy ((uint8_t *) literal_p->u.char_p, char_p, length);
  }
  else
  {
    literal_p->u.char_p = char_p;
  }

  context_p->lit_object.literal_p = literal_p;
  context_p->lit_object.index = (uint16_t) literal_index;
  context_p->literal_count++;
} /* lexer_process_char_literal */

/* Maximum buffer size for identifiers which contains escape sequences. */
#define LEXER_MAX_LITERAL_LOCAL_BUFFER_SIZE 48

/**
 * Construct a literal object from an identifier.
 */
void
lexer_construct_literal_object (parser_context_t *context_p, /**< context */
                                lexer_lit_location_t *literal_p, /**< literal location */
                                uint8_t literal_type) /**< final literal type */
{
  uint8_t *destination_start_p;
  const uint8_t *source_p;
  uint8_t local_byte_array[LEXER_MAX_LITERAL_LOCAL_BUFFER_SIZE];

  JERRY_ASSERT (literal_p->type == LEXER_IDENT_LITERAL
                || literal_p->type == LEXER_STRING_LITERAL);
  JERRY_ASSERT (context_p->allocated_buffer_p == NULL);

  destination_start_p = local_byte_array;
  source_p = literal_p->char_p;

  if (literal_p->has_escape)
  {
    uint8_t *destination_p;

    if (literal_p->length > LEXER_MAX_LITERAL_LOCAL_BUFFER_SIZE)
    {
      destination_start_p = (uint8_t *) parser_malloc_local (context_p, literal_p->length);
      context_p->allocated_buffer_p = destination_start_p;
      context_p->allocated_buffer_size = literal_p->length;
    }

    destination_p = destination_start_p;

    if (literal_p->type == LEXER_IDENT_LITERAL)
    {
      const uint8_t *source_end_p = context_p->source_end_p;

      JERRY_ASSERT (literal_p->length <= PARSER_MAXIMUM_IDENT_LENGTH);

      do
      {
        if (*source_p == LIT_CHAR_BACKSLASH)
        {
          destination_p += lit_char_to_utf8_bytes (destination_p,
                                                   lexer_hex_to_character (context_p, source_p + 2, 4));
          source_p += 6;
          continue;
        }

        *destination_p++ = *source_p++;

        while (source_p < source_end_p
               && IS_UTF8_INTERMEDIATE_OCTET (*source_p))
        {
          *destination_p++ = *source_p++;
        }
      }
      while (source_p < source_end_p
             && (lit_char_is_identifier_part (source_p) || *source_p == LIT_CHAR_BACKSLASH));

      JERRY_ASSERT (destination_p == destination_start_p + literal_p->length);
    }
    else
    {
      uint8_t str_end_character = source_p[-1];

      while (true)
      {
        if (*source_p == str_end_character)
        {
          break;
        }

        if (*source_p == LIT_CHAR_BACKSLASH)
        {
          uint8_t conv_character;

          source_p++;
          JERRY_ASSERT (source_p < context_p->source_end_p);

          /* Newline is ignored. */
          if (*source_p == LIT_CHAR_CR
              || *source_p == LIT_CHAR_LF
              || (*source_p == LEXER_NEWLINE_LS_PS_BYTE_1 && LEXER_NEWLINE_LS_PS_BYTE_23 (source_p)))
          {
            if (*source_p == LIT_CHAR_CR)
            {
              source_p++;
              JERRY_ASSERT (source_p < context_p->source_end_p);

              if (*source_p == LIT_CHAR_LF)
              {
                source_p++;
              }
            }
            else if (*source_p == LIT_CHAR_LF)
            {
              source_p++;
            }
            else
            {
              source_p += 3;
            }
            continue;
          }

          if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_3)
          {
            uint32_t octal_number = (uint32_t) (*source_p - LIT_CHAR_0);

            source_p++;
            JERRY_ASSERT (source_p < context_p->source_end_p);

            if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
            {
              octal_number = octal_number * 8 + (uint32_t) (*source_p - LIT_CHAR_0);
              source_p++;
              JERRY_ASSERT (source_p < context_p->source_end_p);

              if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
              {
                octal_number = octal_number * 8 + (uint32_t) (*source_p - LIT_CHAR_0);
                source_p++;
                JERRY_ASSERT (source_p < context_p->source_end_p);
              }
            }

            destination_p += lit_char_to_utf8_bytes (destination_p, (uint16_t) octal_number);
            continue;
          }

          if (*source_p >= LIT_CHAR_4 && *source_p <= LIT_CHAR_7)
          {
            uint32_t octal_number = (uint32_t) (*source_p - LIT_CHAR_0);

            source_p++;
            JERRY_ASSERT (source_p < context_p->source_end_p);

            if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
            {
              octal_number = octal_number * 8 + (uint32_t) (*source_p - LIT_CHAR_0);
              source_p++;
              JERRY_ASSERT (source_p < context_p->source_end_p);
            }

            *destination_p++ = (uint8_t) octal_number;
            continue;
          }

          if (*source_p == LIT_CHAR_LOWERCASE_X || *source_p == LIT_CHAR_LOWERCASE_U)
          {
            int hex_part_length = (*source_p == LIT_CHAR_LOWERCASE_X) ? 2 : 4;
            JERRY_ASSERT (source_p + 1 + hex_part_length <= context_p->source_end_p);

            destination_p += lit_char_to_utf8_bytes (destination_p,
                                                     lexer_hex_to_character (context_p,
                                                                             source_p + 1,
                                                                             hex_part_length));
            source_p += hex_part_length + 1;
            continue;
          }

          conv_character = *source_p;
          switch (*source_p)
          {
            case LIT_CHAR_LOWERCASE_B:
            {
              conv_character = 0x08;
              break;
            }
            case LIT_CHAR_LOWERCASE_T:
            {
              conv_character = 0x09;
              break;
            }
            case LIT_CHAR_LOWERCASE_N:
            {
              conv_character = 0x0a;
              break;
            }
            case LIT_CHAR_LOWERCASE_V:
            {
              conv_character = 0x0b;
              break;
            }
            case LIT_CHAR_LOWERCASE_F:
            {
              conv_character = 0x0c;
              break;
            }
            case LIT_CHAR_LOWERCASE_R:
            {
              conv_character = 0x0d;
              break;
            }
          }

          if (conv_character != *source_p)
          {
            *destination_p++ = conv_character;
            source_p++;
            continue;
          }
        }

        if (*source_p >= LEXER_UTF8_4BYTE_START)
        {
          /* Processing 4 byte unicode sequence (even if it is
           * after a backslash). Always converted to two 3 byte
           * long sequence. */

          uint32_t character = ((((uint32_t) source_p[0]) & 0x7) << 18);
          character |= ((((uint32_t) source_p[1]) & LIT_UTF8_LAST_6_BITS_MASK) << 12);
          character |= ((((uint32_t) source_p[2]) & LIT_UTF8_LAST_6_BITS_MASK) << 6);
          character |= (((uint32_t) source_p[3]) & LIT_UTF8_LAST_6_BITS_MASK);

          JERRY_ASSERT (character >= 0x10000);
          character -= 0x10000;
          destination_p += lit_char_to_utf8_bytes (destination_p,
                                                   (ecma_char_t) (0xd800 | (character >> 10)));
          destination_p += lit_char_to_utf8_bytes (destination_p,
                                                   (ecma_char_t) (0xdc00 | (character & LIT_UTF16_LAST_10_BITS_MASK)));
          source_p += 4;
          continue;
        }

        *destination_p++ = *source_p++;

        /* There is no need to check the source_end_p
         * since the string is terminated by a quotation mark. */
        while (IS_UTF8_INTERMEDIATE_OCTET (*source_p))
        {
          *destination_p++ = *source_p++;
        }
      }

      JERRY_ASSERT (destination_p == destination_start_p + literal_p->length);
    }

    source_p = destination_start_p;
  }

  lexer_process_char_literal (context_p,
                              source_p,
                              literal_p->length,
                              literal_type,
                              literal_p->has_escape);

  context_p->lit_object.type = LEXER_LITERAL_OBJECT_ANY;

  if (literal_type == LEXER_IDENT_LITERAL)
  {
    if ((context_p->status_flags & PARSER_INSIDE_WITH)
        && context_p->lit_object.literal_p->type == LEXER_IDENT_LITERAL)
    {
      context_p->lit_object.literal_p->status_flags |= LEXER_FLAG_NO_REG_STORE;
    }

    if (literal_p->length == 4
        && source_p[0] == LIT_CHAR_LOWERCASE_E
        && source_p[3] == LIT_CHAR_LOWERCASE_L
        && source_p[1] == LIT_CHAR_LOWERCASE_V
        && source_p[2] == LIT_CHAR_LOWERCASE_A)
    {
      context_p->lit_object.type = LEXER_LITERAL_OBJECT_EVAL;
    }

    if (literal_p->length == 9
        && source_p[0] == LIT_CHAR_LOWERCASE_A
        && source_p[8] == LIT_CHAR_LOWERCASE_S
        && memcmp (source_p + 1, "rgument", 7) == 0)
    {
      context_p->lit_object.type = LEXER_LITERAL_OBJECT_ARGUMENTS;
      if (!(context_p->status_flags & PARSER_ARGUMENTS_NOT_NEEDED))
      {
        context_p->status_flags |= PARSER_ARGUMENTS_NEEDED | PARSER_LEXICAL_ENV_NEEDED;
        context_p->lit_object.literal_p->status_flags |= LEXER_FLAG_NO_REG_STORE;
      }
    }
  }

  if (destination_start_p != local_byte_array)
  {
    JERRY_ASSERT (context_p->allocated_buffer_p == destination_start_p);

    context_p->allocated_buffer_p = NULL;
    parser_free_local (destination_start_p,
                       context_p->allocated_buffer_size);
  }

  JERRY_ASSERT (context_p->allocated_buffer_p == NULL);
} /* lexer_construct_literal_object */

#undef LEXER_MAX_LITERAL_LOCAL_BUFFER_SIZE

/**
 * Construct a number object.
 *
 * @return true if number is small number
 */
bool
lexer_construct_number_object (parser_context_t *context_p, /**< context */
                               bool push_number_allowed, /**< push number support is allowed */
                               bool is_negative_number) /**< sign is negative */
{
  parser_list_iterator_t literal_iterator;
  lexer_literal_t *literal_p;
  ecma_number_t num;
  uint32_t literal_index = 0;
  uint16_t length = context_p->token.lit_location.length;

  if (context_p->token.extra_value != LEXER_NUMBER_OCTAL)
  {
    num = ecma_utf8_string_to_number (context_p->token.lit_location.char_p,
                                      length);
  }
  else
  {
    const uint8_t *src_p = context_p->token.lit_location.char_p;
    const uint8_t *src_end_p = src_p + length - 1;

    num = 0;
    do
    {
      src_p++;
      num = num * 8 + (ecma_number_t) (*src_p - LIT_CHAR_0);
    }
    while (src_p < src_end_p);
  }

  if (push_number_allowed)
  {
    int32_t int_num = (int32_t) num;

    if (int_num == num)
    {
      if (int_num <= CBC_PUSH_NUMBER_BYTE_RANGE_END
          && (int_num != 0 || !is_negative_number))
      {
        context_p->lit_object.index = (uint16_t) int_num;
        return true;
      }
    }
  }

  if (is_negative_number)
  {
    num = -num;
  }

  jmem_cpointer_t lit_cp = ecma_find_or_create_literal_number (num);
  parser_list_iterator_init (&context_p->literal_pool, &literal_iterator);

  while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
  {
    if (literal_p->type == LEXER_NUMBER_LITERAL
        && literal_p->u.value == lit_cp)
    {
      context_p->lit_object.literal_p = literal_p;
      context_p->lit_object.index = (uint16_t) literal_index;
      context_p->lit_object.type = LEXER_LITERAL_OBJECT_ANY;
      return false;
    }

    literal_index++;
  }

  JERRY_ASSERT (literal_index == context_p->literal_count);

  if (literal_index >= PARSER_MAXIMUM_NUMBER_OF_LITERALS)
  {
    parser_raise_error (context_p, PARSER_ERR_LITERAL_LIMIT_REACHED);
  }

  literal_p = (lexer_literal_t *) parser_list_append (context_p, &context_p->literal_pool);
  literal_p->prop.length = context_p->token.lit_location.length;
  literal_p->type = LEXER_UNUSED_LITERAL;
  literal_p->status_flags = 0;

  context_p->literal_count++;

  literal_p->u.value = lit_cp;
  literal_p->type = LEXER_NUMBER_LITERAL;

  context_p->lit_object.literal_p = literal_p;
  context_p->lit_object.index = (uint16_t) literal_index;
  context_p->lit_object.type = LEXER_LITERAL_OBJECT_ANY;

  return false;
} /* lexer_construct_number_object */

/**
 * Construct a function literal object.
 */
void
lexer_construct_function_object (parser_context_t *context_p, /**< context */
                                 uint32_t extra_status_flags) /**< extra status flags */
{
  ecma_compiled_code_t *compiled_code_p;
  lexer_literal_t *literal_p;

  if (context_p->literal_count >= PARSER_MAXIMUM_NUMBER_OF_LITERALS)
  {
    parser_raise_error (context_p, PARSER_ERR_LITERAL_LIMIT_REACHED);
  }

  if (context_p->status_flags & PARSER_RESOLVE_THIS_FOR_CALLS)
  {
    extra_status_flags |= PARSER_RESOLVE_THIS_FOR_CALLS;
  }

  context_p->status_flags |= PARSER_LEXICAL_ENV_NEEDED;

  literal_p = (lexer_literal_t *) parser_list_append (context_p, &context_p->literal_pool);
  literal_p->type = LEXER_UNUSED_LITERAL;
  literal_p->status_flags = 0;

  context_p->literal_count++;

  compiled_code_p = parser_parse_function (context_p, extra_status_flags);

  literal_p->u.bytecode_p = compiled_code_p;

  literal_p->type = LEXER_FUNCTION_LITERAL;
  context_p->status_flags |= PARSER_NO_REG_STORE;
} /* lexer_construct_function_object */

/**
 * Construct a regular expression object.
 */
void
lexer_construct_regexp_object (parser_context_t *context_p, /**< context */
                               bool parse_only) /**< parse only */
{
#ifndef CONFIG_DISABLE_REGEXP_BUILTIN
  const uint8_t *source_p = context_p->source_p;
  const uint8_t *regex_start_p = context_p->source_p;
  const uint8_t *regex_end_p = regex_start_p;
  const uint8_t *source_end_p = context_p->source_end_p;
  parser_line_counter_t column = context_p->column;
  lexer_literal_t *literal_p;
  bool in_class = false;
  uint16_t current_flags;
  lit_utf8_size_t length;

  JERRY_ASSERT (context_p->token.type == LEXER_DIVIDE
                || context_p->token.type == LEXER_ASSIGN_DIVIDE);

  if (context_p->token.type == LEXER_ASSIGN_DIVIDE)
  {
    regex_start_p--;
  }

  while (true)
  {
    if (source_p >= source_end_p)
    {
      parser_raise_error (context_p, PARSER_ERR_UNTERMINATED_REGEXP);
    }

    if (!in_class && source_p[0] == LIT_CHAR_SLASH)
    {
      regex_end_p = source_p;
      source_p++;
      column++;
      break;
    }

    switch (source_p[0])
    {
      case LIT_CHAR_CR:
      case LIT_CHAR_LF:
      case LEXER_NEWLINE_LS_PS_BYTE_1:
      {
        if (source_p[0] != LEXER_NEWLINE_LS_PS_BYTE_1
            || LEXER_NEWLINE_LS_PS_BYTE_23 (source_p))
        {
          parser_raise_error (context_p, PARSER_ERR_NEWLINE_NOT_ALLOWED);
        }
        break;
      }
      case LIT_CHAR_TAB:
      {
        column = align_column_to_tab (column);
         /* Subtract -1 because column is increased below. */
        column--;
        break;
      }
      case LIT_CHAR_LEFT_SQUARE:
      {
        in_class = true;
        break;
      }
      case LIT_CHAR_RIGHT_SQUARE:
      {
        in_class = false;
        break;
      }
      case LIT_CHAR_BACKSLASH:
      {
        if (source_p + 1 >= source_end_p)
        {
          parser_raise_error (context_p, PARSER_ERR_UNTERMINATED_REGEXP);
        }

        if (source_p[1] >= 0x20 && source_p[1] <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
        {
          source_p++;
          column++;
        }
      }
    }

    source_p++;
    column++;

    while (source_p < source_end_p
           && IS_UTF8_INTERMEDIATE_OCTET (source_p[0]))
    {
      source_p++;
    }
  }

  current_flags = 0;
  while (source_p < source_end_p)
  {
    uint32_t flag = 0;

    if (source_p[0] == LIT_CHAR_LOWERCASE_G)
    {
      flag = RE_FLAG_GLOBAL;
    }
    else if (source_p[0] == LIT_CHAR_LOWERCASE_I)
    {
      flag = RE_FLAG_IGNORE_CASE;
    }
    else if (source_p[0] == LIT_CHAR_LOWERCASE_M)
    {
      flag = RE_FLAG_MULTILINE;
    }

    if (flag == 0)
    {
      break;
    }

    if (current_flags & flag)
    {
      parser_raise_error (context_p, PARSER_ERR_DUPLICATED_REGEXP_FLAG);
    }

    current_flags = (uint16_t) (current_flags | flag);
    source_p++;
    column++;
  }

  if (source_p < source_end_p
      && lit_char_is_identifier_part (source_p))
  {
    parser_raise_error (context_p, PARSER_ERR_UNKNOWN_REGEXP_FLAG);
  }

  context_p->source_p = source_p;
  context_p->column = column;

  length = (lit_utf8_size_t) (regex_end_p - regex_start_p);
  if (length > PARSER_MAXIMUM_STRING_LENGTH)
  {
    parser_raise_error (context_p, PARSER_ERR_REGEXP_TOO_LONG);
  }

  context_p->column = column;
  context_p->source_p = source_p;

  if (parse_only)
  {
    return;
  }

  if (context_p->literal_count >= PARSER_MAXIMUM_NUMBER_OF_LITERALS)
  {
    parser_raise_error (context_p, PARSER_ERR_LITERAL_LIMIT_REACHED);
  }

  literal_p = (lexer_literal_t *) parser_list_append (context_p, &context_p->literal_pool);
  literal_p->prop.length = (uint16_t) length;
  literal_p->type = LEXER_UNUSED_LITERAL;
  literal_p->status_flags = 0;

  context_p->literal_count++;

  /* Compile the RegExp literal and store the RegExp bytecode pointer */
  const re_compiled_code_t *re_bytecode_p = NULL;
  ecma_value_t completion_value;

  ecma_string_t *pattern_str_p = ecma_new_ecma_string_from_utf8 (regex_start_p, length);
  completion_value = re_compile_bytecode (&re_bytecode_p,
                                          pattern_str_p,
                                          current_flags);
  ecma_deref_ecma_string (pattern_str_p);

  bool is_throw = ECMA_IS_VALUE_ERROR (completion_value);

  ecma_free_value (completion_value);

  if (is_throw)
  {
    parser_raise_error (context_p, PARSER_ERR_INVALID_REGEXP);
  }

  literal_p->type = LEXER_REGEXP_LITERAL;
  literal_p->u.bytecode_p = (ecma_compiled_code_t *) re_bytecode_p;

  context_p->token.type = LEXER_LITERAL;
  context_p->token.literal_is_reserved = false;
  context_p->token.lit_location.type = LEXER_REGEXP_LITERAL;

  context_p->lit_object.literal_p = literal_p;
  context_p->lit_object.index = (uint16_t) (context_p->literal_count - 1);
  context_p->lit_object.type = LEXER_LITERAL_OBJECT_ANY;
#else /* CONFIG_DISABLE_REGEXP_BUILTIN */
  JERRY_UNUSED (parse_only);
  parser_raise_error (context_p, PARSER_ERR_UNSUPPORTED_REGEXP);
#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */
} /* lexer_construct_regexp_object */

/**
 * Next token must be an identifier.
 */
void
lexer_expect_identifier (parser_context_t *context_p, /**< context */
                         uint8_t literal_type) /**< literal type */
{
  JERRY_ASSERT (literal_type == LEXER_STRING_LITERAL
                || literal_type == LEXER_IDENT_LITERAL);

  skip_spaces (context_p);
  context_p->token.line = context_p->line;
  context_p->token.column = context_p->column;

  if (context_p->source_p < context_p->source_end_p
      && (lit_char_is_identifier_start (context_p->source_p) || context_p->source_p[0] == LIT_CHAR_BACKSLASH))
  {
    lexer_parse_identifier (context_p, literal_type != LEXER_STRING_LITERAL);

    if (context_p->token.type == LEXER_LITERAL)
    {
      lexer_construct_literal_object (context_p,
                                      &context_p->token.lit_location,
                                      literal_type);

      if (literal_type == LEXER_IDENT_LITERAL
          && (context_p->status_flags & PARSER_IS_STRICT)
          && context_p->lit_object.type != LEXER_LITERAL_OBJECT_ANY)
      {
        parser_error_t error;

        if (context_p->lit_object.type == LEXER_LITERAL_OBJECT_EVAL)
        {
          error = PARSER_ERR_EVAL_NOT_ALLOWED;
        }
        else
        {
          JERRY_ASSERT (context_p->lit_object.type == LEXER_LITERAL_OBJECT_ARGUMENTS);
          error = PARSER_ERR_ARGUMENTS_NOT_ALLOWED;
        }

        parser_raise_error (context_p, error);
      }

      context_p->token.lit_location.type = literal_type;
      return;
    }
  }

  parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
} /* lexer_expect_identifier */

static const lexer_lit_location_t lexer_get_literal =
{
  (const uint8_t *) "get", 3, LEXER_IDENT_LITERAL, false
};

static const lexer_lit_location_t lexer_set_literal =
{
  (const uint8_t *) "set", 3, LEXER_IDENT_LITERAL, false
};

/**
 * Next token must be an identifier.
 */
void
lexer_expect_object_literal_id (parser_context_t *context_p, /**< context */
                                bool must_be_identifier) /**< only identifiers are accepted */
{
  skip_spaces (context_p);

  context_p->token.line = context_p->line;
  context_p->token.column = context_p->column;

  if (context_p->source_p < context_p->source_end_p)
  {
    bool create_literal_object = false;

    if (lit_char_is_identifier_start (context_p->source_p) || context_p->source_p[0] == LIT_CHAR_BACKSLASH)
    {
      lexer_parse_identifier (context_p, false);

      if (!must_be_identifier
          && context_p->token.lit_location.length == 3)
      {
        skip_spaces (context_p);

        if (context_p->source_p < context_p->source_end_p
            && context_p->source_p[0] != LIT_CHAR_COLON)
        {
          if (lexer_compare_identifier_to_current (context_p, &lexer_get_literal))
          {
            context_p->token.type = LEXER_PROPERTY_GETTER;
            return;
          }
          else if (lexer_compare_identifier_to_current (context_p, &lexer_set_literal))
          {
            context_p->token.type = LEXER_PROPERTY_SETTER;
            return;
          }
        }
      }

      create_literal_object = true;
    }
    else if (context_p->source_p[0] == LIT_CHAR_DOUBLE_QUOTE
             || context_p->source_p[0] == LIT_CHAR_SINGLE_QUOTE)
    {
      lexer_parse_string (context_p);
      create_literal_object = true;
    }
    else if (!must_be_identifier && context_p->source_p[0] == LIT_CHAR_RIGHT_BRACE)
    {
      context_p->token.type = LEXER_RIGHT_BRACE;
      context_p->source_p += 1;
      context_p->column++;
      return;
    }
    else
    {
      const uint8_t *char_p = context_p->source_p;

      if (char_p[0] == LIT_CHAR_DOT)
      {
        char_p++;
      }

      if (char_p < context_p->source_end_p
          && char_p[0] >= LIT_CHAR_0
          && char_p[0] <= LIT_CHAR_9)
      {
        lexer_parse_number (context_p);
        lexer_construct_number_object (context_p, false, false);
        return;
      }
    }

    if (create_literal_object)
    {
      lexer_construct_literal_object (context_p,
                                      &context_p->token.lit_location,
                                      LEXER_STRING_LITERAL);
      return;
    }
  }

  parser_raise_error (context_p, PARSER_ERR_PROPERTY_IDENTIFIER_EXPECTED);
} /* lexer_expect_object_literal_id */

/**
 * Next token must be an identifier.
 */
void
lexer_scan_identifier (parser_context_t *context_p, /**< context */
                       bool propety_name) /**< property name */
{
  skip_spaces (context_p);
  context_p->token.line = context_p->line;
  context_p->token.column = context_p->column;

  if (context_p->source_p < context_p->source_end_p
      && (lit_char_is_identifier_start (context_p->source_p) || context_p->source_p[0] == LIT_CHAR_BACKSLASH))
  {
    lexer_parse_identifier (context_p, false);

    if (propety_name && context_p->token.lit_location.length == 3)
    {
      skip_spaces (context_p);

      if (context_p->source_p < context_p->source_end_p
          && context_p->source_p[0] != LIT_CHAR_COLON)
      {
        if (lexer_compare_identifier_to_current (context_p, &lexer_get_literal))
        {
          context_p->token.type = LEXER_PROPERTY_GETTER;
        }
        else if (lexer_compare_identifier_to_current (context_p, &lexer_set_literal))
        {
          context_p->token.type = LEXER_PROPERTY_SETTER;
        }
      }
    }
    return;
  }

  if (propety_name)
  {
    lexer_next_token (context_p);

    if (context_p->token.type == LEXER_LITERAL
        || context_p->token.type == LEXER_RIGHT_BRACE)
    {
      return;
    }
  }

  parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
} /* lexer_scan_identifier */

/**
 * Compares the given identifier to that which is the current token
 * in the parser context.
 *
 * @return true if the input identifiers are the same
 */
bool
lexer_compare_identifier_to_current (parser_context_t *context_p,        /**< context */
                                     const lexer_lit_location_t *right)  /**< identifier */
{
  lexer_lit_location_t *left = &context_p->token.lit_location;
  const uint8_t *left_p;
  const uint8_t *right_p;
  size_t count;

  JERRY_ASSERT (left->length > 0 && right->length > 0);

  if (left->length != right->length)
  {
    return 0;
  }

  if (!left->has_escape && !right->has_escape)
  {
    return memcmp (left->char_p, right->char_p, left->length) == 0;
  }

  left_p = left->char_p;
  right_p = right->char_p;
  count = left->length;

  do
  {
    uint8_t utf8_buf[3];
    size_t utf8_len, offset;

    /* Backslash cannot be part of a multibyte UTF-8 character. */
    if (*left_p != LIT_CHAR_BACKSLASH && *right_p != LIT_CHAR_BACKSLASH)
    {
      if (*left_p++ != *right_p++)
      {
        return false;
      }
      count--;
      continue;
    }

    if (*left_p == LIT_CHAR_BACKSLASH && *right_p == LIT_CHAR_BACKSLASH)
    {
      uint16_t left_chr = lexer_hex_to_character (context_p, left_p, 6);

      if (left_chr != lexer_hex_to_character (context_p, right_p, 6))
      {
        return false;
      }

      left_p += 6;
      right_p += 6;
      count += lit_char_get_utf8_length (left_chr);
      continue;
    }

    /* One character is encoded as unicode sequence. */
    if (*right_p == LIT_CHAR_BACKSLASH)
    {
      /* The pointers can be swapped. */
      const uint8_t *swap_p = left_p;
      left_p = right_p;
      right_p = swap_p;
    }

    utf8_len = lit_char_to_utf8_bytes (utf8_buf, lexer_hex_to_character (context_p, left_p, 6));
    JERRY_ASSERT (utf8_len > 0);
    count -= utf8_len;
    offset = 0;

    do
    {
      if (utf8_buf[offset] != *right_p++)
      {
        return false;
      }
      offset++;
    }
    while (offset < utf8_len);

    left_p += 6;
  }
  while (count > 0);

  return true;
} /* lexer_compare_identifier_to_current */

/**
 * @}
 * @}
 * @}
 */

#endif /* JERRY_JS_PARSER */

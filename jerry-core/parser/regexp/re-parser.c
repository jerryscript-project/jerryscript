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

#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-try-catch-macro.h"
#include "jrt-libc-includes.h"
#include "lit-char-helpers.h"
#include "re-compiler.h"
#include "re-parser.h"

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup regexparser Regular expression
 * @{
 *
 * \addtogroup regexparser_parser Parser
 * @{
 */

/**
 * Lookup a character in the input string.
 *
 * @return true - if lookup number of characters ahead are hex digits
 *         false - otherwise
 */
static bool
re_hex_lookup (re_parser_ctx_t *parser_ctx_p, /**< RegExp parser context */
               uint32_t lookup) /**< size of lookup */
{
  bool is_digit = true;
  const lit_utf8_byte_t *curr_p = parser_ctx_p->input_curr_p;

  for (uint32_t i = 0; is_digit && i < lookup; i++)
  {
    if (curr_p < parser_ctx_p->input_end_p)
    {
      is_digit = lit_char_is_hex_digit (*curr_p++);
    }
    else
    {
      return false;
    }
  }

  return is_digit;
} /* re_hex_lookup */

/**
 * Consume non greedy (question mark) character if present.
 *
 * @return true - if non-greedy character found
 *         false - otherwise
 */
static inline bool __attr_always_inline___
re_parse_non_greedy_char (re_parser_ctx_t *parser_ctx_p) /**< RegExp parser context */
{
  if (parser_ctx_p->input_curr_p < parser_ctx_p->input_end_p
      && *parser_ctx_p->input_curr_p == LIT_CHAR_QUESTION)
  {
    parser_ctx_p->input_curr_p++;
    return true;
  }

  return false;
} /* re_parse_non_greedy_char */

/**
 * Parse a max 3 digit long octal number from input string iterator.
 *
 * @return uint32_t - parsed octal number
 */
static uint32_t
re_parse_octal (re_parser_ctx_t *parser_ctx_p) /**< RegExp parser context */
{
  uint32_t number = 0;
  for (int index = 0;
       index < 3
       && parser_ctx_p->input_curr_p < parser_ctx_p->input_end_p
       && lit_char_is_octal_digit (*parser_ctx_p->input_curr_p);
       index++)
  {
    number = number * 8 + lit_char_hex_to_int (*parser_ctx_p->input_curr_p++);
  }

  return number;
} /* re_parse_octal */

/**
 * Parse RegExp iterators
 *
 * @return empty ecma value - if parsed successfully
 *         error ecma value - otherwise
 *
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
re_parse_iterator (re_parser_ctx_t *parser_ctx_p, /**< RegExp parser context */
                   re_token_t *re_token_p) /**< [out] output token */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  re_token_p->qmin = 1;
  re_token_p->qmax = 1;
  re_token_p->greedy = true;

  if (parser_ctx_p->input_curr_p >= parser_ctx_p->input_end_p)
  {
    return ret_value;
  }

  ecma_char_t ch = *parser_ctx_p->input_curr_p;

  switch (ch)
  {
    case LIT_CHAR_QUESTION:
    {
      parser_ctx_p->input_curr_p++;
      re_token_p->qmin = 0;
      re_token_p->qmax = 1;
      re_token_p->greedy = !re_parse_non_greedy_char (parser_ctx_p);
      break;
    }
    case LIT_CHAR_ASTERISK:
    {
      parser_ctx_p->input_curr_p++;
      re_token_p->qmin = 0;
      re_token_p->qmax = RE_ITERATOR_INFINITE;
      re_token_p->greedy = !re_parse_non_greedy_char (parser_ctx_p);
      break;
    }
    case LIT_CHAR_PLUS:
    {
      parser_ctx_p->input_curr_p++;
      re_token_p->qmin = 1;
      re_token_p->qmax = RE_ITERATOR_INFINITE;
      re_token_p->greedy = !re_parse_non_greedy_char (parser_ctx_p);
      break;
    }
    case LIT_CHAR_LEFT_BRACE:
    {
      parser_ctx_p->input_curr_p++;
      uint32_t qmin = 0;
      uint32_t qmax = RE_ITERATOR_INFINITE;
      uint32_t digits = 0;

      while (true)
      {
        if (parser_ctx_p->input_curr_p >= parser_ctx_p->input_end_p)
        {
          return ecma_raise_syntax_error (ECMA_ERR_MSG ("invalid quantifier"));
        }

        ch = *parser_ctx_p->input_curr_p++;

        if (lit_char_is_decimal_digit (ch))
        {
          if (digits >= ECMA_NUMBER_MAX_DIGITS)
          {
            return ecma_raise_syntax_error (ECMA_ERR_MSG ("RegExp quantifier error: too many digits."));
          }
          digits++;
          qmin = qmin * 10 + lit_char_hex_to_int (ch);
        }
        else if (ch == LIT_CHAR_COMMA)
        {
          if (qmax != RE_ITERATOR_INFINITE)
          {
            return ecma_raise_syntax_error (ECMA_ERR_MSG ("RegExp quantifier error: double comma."));
          }

          if (parser_ctx_p->input_curr_p >= parser_ctx_p->input_end_p)
          {
            return ecma_raise_syntax_error (ECMA_ERR_MSG ("invalid quantifier"));
          }

          if (*parser_ctx_p->input_curr_p == LIT_CHAR_RIGHT_BRACE)
          {
            if (digits == 0)
            {
              return ecma_raise_syntax_error (ECMA_ERR_MSG ("RegExp quantifier error: missing digits."));
            }

            parser_ctx_p->input_curr_p++;
            re_token_p->qmin = qmin;
            re_token_p->qmax = RE_ITERATOR_INFINITE;
            break;
          }
          qmax = qmin;
          qmin = 0;
          digits = 0;
        }
        else if (ch == LIT_CHAR_RIGHT_BRACE)
        {
          if (digits == 0)
          {
            return ecma_raise_syntax_error (ECMA_ERR_MSG ("RegExp quantifier error: missing digits."));
          }

          if (qmax != RE_ITERATOR_INFINITE)
          {
            re_token_p->qmin = qmax;
          }
          else
          {
            re_token_p->qmin = qmin;
          }

          re_token_p->qmax = qmin;

          break;
        }
        else
        {
          return ecma_raise_syntax_error (ECMA_ERR_MSG ("RegExp quantifier error: unknown char."));
        }
      }

      re_token_p->greedy = !re_parse_non_greedy_char (parser_ctx_p);
      break;
    }
    default:
    {
      break;
    }
  }

  JERRY_ASSERT (ecma_is_value_empty (ret_value));

  if (re_token_p->qmin > re_token_p->qmax)
  {
    ret_value = ecma_raise_syntax_error (ECMA_ERR_MSG ("RegExp quantifier error: qmin > qmax."));
  }

  return ret_value;
} /* re_parse_iterator */

/**
 * Count the number of groups in pattern
 */
static void
re_count_num_of_groups (re_parser_ctx_t *parser_ctx_p) /**< RegExp parser context */
{
  int char_class_in = 0;
  parser_ctx_p->num_of_groups = 0;
  const lit_utf8_byte_t *curr_p = parser_ctx_p->input_start_p;

  while (curr_p < parser_ctx_p->input_end_p)
  {
    switch (*curr_p++)
    {
      case LIT_CHAR_BACKSLASH:
      {
        lit_utf8_incr (&curr_p);
        break;
      }
      case LIT_CHAR_LEFT_SQUARE:
      {
        char_class_in++;
        break;
      }
      case LIT_CHAR_RIGHT_SQUARE:
      {
        if (char_class_in)
        {
          char_class_in--;
        }
        break;
      }
      case LIT_CHAR_LEFT_PAREN:
      {
        if (curr_p < parser_ctx_p->input_end_p
            && *curr_p != LIT_CHAR_QUESTION
            && !char_class_in)
        {
          parser_ctx_p->num_of_groups++;
        }
        break;
      }
    }
  }
} /* re_count_num_of_groups */

/**
 * Read the input pattern and parse the range of character class
 *
 * @return empty ecma value - if parsed successfully
 *         error ecma value - otherwise
 *
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
re_parse_char_class (re_parser_ctx_t *parser_ctx_p, /**< number of classes */
                     re_char_class_callback append_char_class, /**< callback function,
                                                                *   which adds the char-ranges
                                                                *   to the bytecode */
                     void *re_ctx_p, /**< regexp compiler context */
                     re_token_t *out_token_p) /**< [out] output token */
{
  re_token_type_t token_type = ((re_compiler_ctx_t *) re_ctx_p)->current_token.type;
  out_token_p->qmax = out_token_p->qmin = 1;
  ecma_char_t start = LIT_CHAR_UNDEF;
  bool is_range = false;
  parser_ctx_p->num_of_classes = 0;

  if (lit_utf8_peek_prev (parser_ctx_p->input_curr_p) != LIT_CHAR_LEFT_SQUARE)
  {
    lit_utf8_decr (&parser_ctx_p->input_curr_p);
    lit_utf8_decr (&parser_ctx_p->input_curr_p);
  }

  do
  {
    if (parser_ctx_p->input_curr_p >= parser_ctx_p->input_end_p)
    {
      return ecma_raise_syntax_error (ECMA_ERR_MSG ("invalid character class, end of string"));
    }

    ecma_char_t ch = lit_utf8_read_next (&parser_ctx_p->input_curr_p);

    if (ch == LIT_CHAR_RIGHT_SQUARE)
    {
      if (start != LIT_CHAR_UNDEF)
      {
        append_char_class (re_ctx_p, start, start);
      }
      break;
    }
    else if (ch == LIT_CHAR_MINUS)
    {
      if (parser_ctx_p->input_curr_p >= parser_ctx_p->input_end_p)
      {
        return ecma_raise_syntax_error (ECMA_ERR_MSG ("invalid character class, end of string after '-'"));
      }

      if (start != LIT_CHAR_UNDEF
          && !is_range
          && *parser_ctx_p->input_curr_p != LIT_CHAR_RIGHT_SQUARE)
      {
        is_range = true;
        continue;
      }
    }
    else if (ch == LIT_CHAR_BACKSLASH)
    {
      if (parser_ctx_p->input_curr_p >= parser_ctx_p->input_end_p)
      {
        return ecma_raise_syntax_error (ECMA_ERR_MSG ("invalid character class, end of string after '\\'"));
      }

      ch = lit_utf8_read_next (&parser_ctx_p->input_curr_p);

      if (ch == LIT_CHAR_LOWERCASE_B)
      {
        ch = LIT_CHAR_BS;
      }
      else if (ch == LIT_CHAR_LOWERCASE_F)
      {
        ch = LIT_CHAR_FF;
      }
      else if (ch == LIT_CHAR_LOWERCASE_N)
      {
        ch = LIT_CHAR_LF;
      }
      else if (ch == LIT_CHAR_LOWERCASE_T)
      {
        ch = LIT_CHAR_TAB;
      }
      else if (ch == LIT_CHAR_LOWERCASE_R)
      {
        ch = LIT_CHAR_CR;
      }
      else if (ch == LIT_CHAR_LOWERCASE_V)
      {
        ch = LIT_CHAR_VTAB;
      }
      else if (ch == LIT_CHAR_LOWERCASE_C)
      {
        if (parser_ctx_p->input_curr_p < parser_ctx_p->input_end_p)
        {
          ch = *parser_ctx_p->input_curr_p;

          if ((ch >= LIT_CHAR_ASCII_UPPERCASE_LETTERS_BEGIN && ch <= LIT_CHAR_ASCII_UPPERCASE_LETTERS_END)
              || (ch >= LIT_CHAR_ASCII_LOWERCASE_LETTERS_BEGIN && ch <= LIT_CHAR_ASCII_LOWERCASE_LETTERS_END)
              || (ch >= LIT_CHAR_0 && ch <= LIT_CHAR_9))
          {
            /* See ECMA-262 v5, 15.10.2.10 (Point 3) */
            ch = (ch % 32);
            parser_ctx_p->input_curr_p++;
          }
          else
          {
            ch = LIT_CHAR_LOWERCASE_C;
          }
        }
      }
      else if (ch == LIT_CHAR_LOWERCASE_X)
      {
        ecma_char_t code_unit;

        if (!lit_read_code_unit_from_hex (parser_ctx_p->input_curr_p, 2, &code_unit))
        {
          return ecma_raise_syntax_error (ECMA_ERR_MSG ("invalid character class, end of string after '\\x'"));
        }

        parser_ctx_p->input_curr_p += 2;
        if (is_range == false && lit_utf8_peek_next (parser_ctx_p->input_curr_p) == LIT_CHAR_MINUS)
        {
          start = code_unit;
          continue;
        }

        ch = code_unit;
      }
      else if (ch == LIT_CHAR_LOWERCASE_U)
      {
        ecma_char_t code_unit;

        if (!lit_read_code_unit_from_hex (parser_ctx_p->input_curr_p, 4, &code_unit))
        {
          return ecma_raise_syntax_error (ECMA_ERR_MSG ("invalid character class, end of string after '\\u'"));
        }

        parser_ctx_p->input_curr_p += 4;
        if (is_range == false && lit_utf8_peek_next (parser_ctx_p->input_curr_p) == LIT_CHAR_MINUS)
        {
          start = code_unit;
          continue;
        }

        ch = code_unit;
      }
      else if (ch == LIT_CHAR_LOWERCASE_D)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        append_char_class (re_ctx_p, LIT_CHAR_ASCII_DIGITS_BEGIN, LIT_CHAR_ASCII_DIGITS_END);
        ch = LIT_CHAR_UNDEF;
      }
      else if (ch == LIT_CHAR_UPPERCASE_D)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        append_char_class (re_ctx_p, LIT_CHAR_NULL, LIT_CHAR_ASCII_DIGITS_BEGIN - 1);
        append_char_class (re_ctx_p, LIT_CHAR_ASCII_DIGITS_END + 1, LIT_UTF16_CODE_UNIT_MAX);
        ch = LIT_CHAR_UNDEF;
      }
      else if (ch == LIT_CHAR_LOWERCASE_S)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        append_char_class (re_ctx_p, LIT_CHAR_TAB, LIT_CHAR_CR);
        append_char_class (re_ctx_p, LIT_CHAR_SP, LIT_CHAR_SP);
        append_char_class (re_ctx_p, LIT_CHAR_NBSP, LIT_CHAR_NBSP);
        append_char_class (re_ctx_p, 0x1680UL, 0x1680UL); /* Ogham Space Mark */
        append_char_class (re_ctx_p, 0x180EUL, 0x180EUL); /* Mongolian Vowel Separator */
        append_char_class (re_ctx_p, 0x2000UL, 0x200AUL); /* En Quad - Hair Space */
        append_char_class (re_ctx_p, LIT_CHAR_LS, LIT_CHAR_PS);
        append_char_class (re_ctx_p, 0x202FUL, 0x202FUL); /* Narrow No-Break Space */
        append_char_class (re_ctx_p, 0x205FUL, 0x205FUL); /* Medium Mathematical Space */
        append_char_class (re_ctx_p, 0x3000UL, 0x3000UL); /* Ideographic Space */
        append_char_class (re_ctx_p, LIT_CHAR_BOM, LIT_CHAR_BOM);
        ch = LIT_CHAR_UNDEF;
      }
      else if (ch == LIT_CHAR_UPPERCASE_S)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        append_char_class (re_ctx_p, LIT_CHAR_NULL, LIT_CHAR_TAB - 1);
        append_char_class (re_ctx_p, LIT_CHAR_CR + 1, LIT_CHAR_SP - 1);
        append_char_class (re_ctx_p, LIT_CHAR_SP + 1, LIT_CHAR_NBSP - 1);
        append_char_class (re_ctx_p, LIT_CHAR_NBSP + 1, 0x167FUL);
        append_char_class (re_ctx_p, 0x1681UL, 0x180DUL);
        append_char_class (re_ctx_p, 0x180FUL, 0x1FFFUL);
        append_char_class (re_ctx_p, 0x200BUL, LIT_CHAR_LS - 1);
        append_char_class (re_ctx_p, LIT_CHAR_PS + 1, 0x202EUL);
        append_char_class (re_ctx_p, 0x2030UL, 0x205EUL);
        append_char_class (re_ctx_p, 0x2060UL, 0x2FFFUL);
        append_char_class (re_ctx_p, 0x3001UL, LIT_CHAR_BOM - 1);
        append_char_class (re_ctx_p, LIT_CHAR_BOM + 1, LIT_UTF16_CODE_UNIT_MAX);
        ch = LIT_CHAR_UNDEF;
      }
      else if (ch == LIT_CHAR_LOWERCASE_W)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        append_char_class (re_ctx_p, LIT_CHAR_0, LIT_CHAR_9);
        append_char_class (re_ctx_p, LIT_CHAR_UPPERCASE_A, LIT_CHAR_UPPERCASE_Z);
        append_char_class (re_ctx_p, LIT_CHAR_UNDERSCORE, LIT_CHAR_UNDERSCORE);
        append_char_class (re_ctx_p, LIT_CHAR_LOWERCASE_A, LIT_CHAR_LOWERCASE_Z);
        ch = LIT_CHAR_UNDEF;
      }
      else if (ch == LIT_CHAR_UPPERCASE_W)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        append_char_class (re_ctx_p, LIT_CHAR_NULL, LIT_CHAR_0 - 1);
        append_char_class (re_ctx_p, LIT_CHAR_9 + 1, LIT_CHAR_UPPERCASE_A - 1);
        append_char_class (re_ctx_p, LIT_CHAR_UPPERCASE_Z + 1, LIT_CHAR_UNDERSCORE - 1);
        append_char_class (re_ctx_p, LIT_CHAR_UNDERSCORE + 1, LIT_CHAR_LOWERCASE_A - 1);
        append_char_class (re_ctx_p, LIT_CHAR_LOWERCASE_Z + 1, LIT_UTF16_CODE_UNIT_MAX);
        ch = LIT_CHAR_UNDEF;
      }
      else if (lit_char_is_octal_digit ((ecma_char_t) ch)
               && ch != LIT_CHAR_0)
      {
        lit_utf8_decr (&parser_ctx_p->input_curr_p);
        ch = (ecma_char_t) re_parse_octal (parser_ctx_p);
      }
    } /* ch == LIT_CHAR_BACKSLASH */

    if (start != LIT_CHAR_UNDEF)
    {
      if (is_range)
      {
        if (start > ch)
        {
          return ecma_raise_syntax_error (ECMA_ERR_MSG ("invalid character class, wrong order"));
        }
        else
        {
          append_char_class (re_ctx_p, start, ch);
          start = LIT_CHAR_UNDEF;
          is_range = false;
        }
      }
      else
      {
        append_char_class (re_ctx_p, start, start);
        start = ch;
      }
    }
    else
    {
      start = ch;
    }
  }
  while (token_type == RE_TOK_START_CHAR_CLASS || token_type == RE_TOK_START_INV_CHAR_CLASS);

  return re_parse_iterator (parser_ctx_p, out_token_p);
} /* re_parse_char_class */

/**
 * Read the input pattern and parse the next token for the RegExp compiler
 *
 * @return empty ecma value - if parsed successfully
 *         error ecma value - otherwise
 *
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
re_parse_next_token (re_parser_ctx_t *parser_ctx_p, /**< RegExp parser context */
                     re_token_t *out_token_p) /**< [out] output token */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (parser_ctx_p->input_curr_p >= parser_ctx_p->input_end_p)
  {
    out_token_p->type = RE_TOK_EOF;
    return ret_value;
  }

  ecma_char_t ch = lit_utf8_read_next (&parser_ctx_p->input_curr_p);

  switch (ch)
  {
    case LIT_CHAR_VLINE:
    {
      out_token_p->type = RE_TOK_ALTERNATIVE;
      break;
    }
    case LIT_CHAR_CIRCUMFLEX:
    {
      out_token_p->type = RE_TOK_ASSERT_START;
      break;
    }
    case LIT_CHAR_DOLLAR_SIGN:
    {
      out_token_p->type = RE_TOK_ASSERT_END;
      break;
    }
    case LIT_CHAR_DOT:
    {
      out_token_p->type = RE_TOK_PERIOD;
      ret_value = re_parse_iterator (parser_ctx_p, out_token_p);
      break;
    }
    case LIT_CHAR_BACKSLASH:
    {
      if (parser_ctx_p->input_curr_p >= parser_ctx_p->input_end_p)
      {
        return ecma_raise_syntax_error (ECMA_ERR_MSG ("invalid regular experssion"));
      }

      out_token_p->type = RE_TOK_CHAR;
      ch = lit_utf8_read_next (&parser_ctx_p->input_curr_p);

      if (ch == LIT_CHAR_LOWERCASE_B)
      {
        out_token_p->type = RE_TOK_ASSERT_WORD_BOUNDARY;
      }
      else if (ch == LIT_CHAR_UPPERCASE_B)
      {
        out_token_p->type = RE_TOK_ASSERT_NOT_WORD_BOUNDARY;
      }
      else if (ch == LIT_CHAR_LOWERCASE_F)
      {
        out_token_p->value = LIT_CHAR_FF;
      }
      else if (ch == LIT_CHAR_LOWERCASE_N)
      {
        out_token_p->value = LIT_CHAR_LF;
      }
      else if (ch == LIT_CHAR_LOWERCASE_T)
      {
        out_token_p->value = LIT_CHAR_TAB;
      }
      else if (ch == LIT_CHAR_LOWERCASE_R)
      {
        out_token_p->value = LIT_CHAR_CR;
      }
      else if (ch == LIT_CHAR_LOWERCASE_V)
      {
        out_token_p->value = LIT_CHAR_VTAB;
      }
      else if (ch == LIT_CHAR_LOWERCASE_C)
      {
        if (parser_ctx_p->input_curr_p < parser_ctx_p->input_end_p)
        {
          ch = *parser_ctx_p->input_curr_p;

          if ((ch >= LIT_CHAR_ASCII_UPPERCASE_LETTERS_BEGIN && ch <= LIT_CHAR_ASCII_UPPERCASE_LETTERS_END)
              || (ch >= LIT_CHAR_ASCII_LOWERCASE_LETTERS_BEGIN && ch <= LIT_CHAR_ASCII_LOWERCASE_LETTERS_END))
          {
            out_token_p->value = (ch % 32);
            parser_ctx_p->input_curr_p++;
          }
          else
          {
            out_token_p->value = LIT_CHAR_BACKSLASH;
            parser_ctx_p->input_curr_p--;
          }
        }
        else
        {
          out_token_p->value = LIT_CHAR_BACKSLASH;
          parser_ctx_p->input_curr_p--;
        }
      }
      else if (ch == LIT_CHAR_LOWERCASE_X
               && re_hex_lookup (parser_ctx_p, 2))
      {
        ecma_char_t code_unit;

        if (!lit_read_code_unit_from_hex (parser_ctx_p->input_curr_p, 2, &code_unit))
        {
          return ecma_raise_syntax_error (ECMA_ERR_MSG ("decode error"));
        }

        parser_ctx_p->input_curr_p += 2;
        out_token_p->value = code_unit;
      }
      else if (ch == LIT_CHAR_LOWERCASE_U
               && re_hex_lookup (parser_ctx_p, 4))
      {
        ecma_char_t code_unit;

        if (!lit_read_code_unit_from_hex (parser_ctx_p->input_curr_p, 4, &code_unit))
        {
          return ecma_raise_syntax_error (ECMA_ERR_MSG ("decode error"));
        }

        parser_ctx_p->input_curr_p += 4;
        out_token_p->value = code_unit;
      }
      else if (ch == LIT_CHAR_LOWERCASE_D)
      {
        out_token_p->type = RE_TOK_DIGIT;
        break;
      }
      else if (ch == LIT_CHAR_UPPERCASE_D)
      {
        out_token_p->type = RE_TOK_NOT_DIGIT;
        break;
      }
      else if (ch == LIT_CHAR_LOWERCASE_S)
      {
        out_token_p->type = RE_TOK_WHITE;
        break;
      }
      else if (ch == LIT_CHAR_UPPERCASE_S)
      {
        out_token_p->type = RE_TOK_NOT_WHITE;
        break;
      }
      else if (ch == LIT_CHAR_LOWERCASE_W)
      {
        out_token_p->type = RE_TOK_WORD_CHAR;
        break;
      }
      else if (ch == LIT_CHAR_UPPERCASE_W)
      {
        out_token_p->type = RE_TOK_NOT_WORD_CHAR;
        break;
      }
      else if (lit_char_is_decimal_digit (ch))
      {
        if (ch == LIT_CHAR_0)
        {
          if (parser_ctx_p->input_curr_p < parser_ctx_p->input_end_p
              && lit_char_is_decimal_digit (*parser_ctx_p->input_curr_p))
          {
            return ecma_raise_syntax_error (ECMA_ERR_MSG ("RegExp escape pattern error."));
          }

          out_token_p->value = LIT_UNICODE_CODE_POINT_NULL;
        }
        else
        {
          if (parser_ctx_p->num_of_groups == -1)
          {
            re_count_num_of_groups (parser_ctx_p);
          }

          if (parser_ctx_p->num_of_groups)
          {
            parser_ctx_p->input_curr_p--;
            uint32_t number = 0;
            int index = 0;

            do
            {
              if (index >= RE_MAX_RE_DECESC_DIGITS)
              {
                ret_value = ecma_raise_syntax_error (ECMA_ERR_MSG ("RegExp escape error: decimal escape too long."));
                return ret_value;
              }
              if (parser_ctx_p->input_curr_p >= parser_ctx_p->input_end_p)
              {
                break;
              }

              ecma_char_t digit = *parser_ctx_p->input_curr_p++;

              if (!lit_char_is_decimal_digit (digit))
              {
                parser_ctx_p->input_curr_p--;
                break;
              }
              number = number * 10 + lit_char_hex_to_int (digit);
              index++;
            }
            while (true);

            if ((int) number <= parser_ctx_p->num_of_groups)
            {
              out_token_p->type = RE_TOK_BACKREFERENCE;
            }
            else
            /* Invalid backreference, fallback to octal */
            {
              /* Rewind to start of number. */
              parser_ctx_p->input_curr_p -= index;

              /* Try to reparse as octal. */
              ecma_char_t digit = *parser_ctx_p->input_curr_p;

              if (!lit_char_is_octal_digit (digit))
              {
                /* Not octal, keep digit character value. */
                number = digit;
                parser_ctx_p->input_curr_p++;
              }
              else
              {
                number = re_parse_octal (parser_ctx_p);
              }
            }
            out_token_p->value = number;
          }
          else
          /* Invalid backreference, fallback to octal if possible */
          {
            if (!lit_char_is_octal_digit (ch))
            {
              /* Not octal, keep character value. */
              out_token_p->value = ch;
            }
            else
            {
              parser_ctx_p->input_curr_p--;
              out_token_p->value = re_parse_octal (parser_ctx_p);
            }
          }
        }
      }
      else
      {
        out_token_p->value = ch;
      }

      ret_value = re_parse_iterator (parser_ctx_p, out_token_p);
      break;
    }
    case LIT_CHAR_LEFT_PAREN:
    {
      if (parser_ctx_p->input_curr_p >= parser_ctx_p->input_end_p)
      {
        return ecma_raise_syntax_error (ECMA_ERR_MSG ("Unterminated group"));
      }

      if (*parser_ctx_p->input_curr_p == LIT_CHAR_QUESTION)
      {
        parser_ctx_p->input_curr_p++;
        if (parser_ctx_p->input_curr_p >= parser_ctx_p->input_end_p)
        {
          return ecma_raise_syntax_error (ECMA_ERR_MSG ("Invalid group"));
        }

        ch = *parser_ctx_p->input_curr_p++;

        if (ch == LIT_CHAR_EQUALS)
        {
          /* (?= */
          out_token_p->type = RE_TOK_ASSERT_START_POS_LOOKAHEAD;
        }
        else if (ch == LIT_CHAR_EXCLAMATION)
        {
          /* (?! */
          out_token_p->type = RE_TOK_ASSERT_START_NEG_LOOKAHEAD;
        }
        else if (ch == LIT_CHAR_COLON)
        {
          /* (?: */
          out_token_p->type = RE_TOK_START_NON_CAPTURE_GROUP;
        }
        else
        {
          return ecma_raise_syntax_error (ECMA_ERR_MSG ("Invalid group"));
        }
      }
      else
      {
        /* ( */
        out_token_p->type = RE_TOK_START_CAPTURE_GROUP;
      }
      break;
    }
    case LIT_CHAR_RIGHT_PAREN:
    {
      out_token_p->type = RE_TOK_END_GROUP;
      ret_value = re_parse_iterator (parser_ctx_p, out_token_p);
      break;
    }
    case LIT_CHAR_LEFT_SQUARE:
    {
      out_token_p->type = RE_TOK_START_CHAR_CLASS;

      if (parser_ctx_p->input_curr_p >= parser_ctx_p->input_end_p)
      {
        return ecma_raise_syntax_error (ECMA_ERR_MSG ("invalid character class"));
      }

      if (*parser_ctx_p->input_curr_p == LIT_CHAR_CIRCUMFLEX)
      {
        out_token_p->type = RE_TOK_START_INV_CHAR_CLASS;
        parser_ctx_p->input_curr_p++;
      }

      break;
    }
    case LIT_CHAR_QUESTION:
    case LIT_CHAR_ASTERISK:
    case LIT_CHAR_PLUS:
    case LIT_CHAR_LEFT_BRACE:
    {
      return ecma_raise_syntax_error (ECMA_ERR_MSG ("Invalid RegExp token."));
    }
    case LIT_CHAR_NULL:
    {
      out_token_p->type = RE_TOK_EOF;
      break;
    }
    default:
    {
      out_token_p->type = RE_TOK_CHAR;
      out_token_p->value = ch;
      ret_value = re_parse_iterator (parser_ctx_p, out_token_p);
      break;
    }
  }

  return ret_value;
} /* re_parse_next_token */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */

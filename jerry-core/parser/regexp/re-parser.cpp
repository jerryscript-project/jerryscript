/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-try-catch-macro.h"
#include "jrt-libc-includes.h"
#include "lit-char-helpers.h"
#include "re-compiler.h"
#include "re-parser.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN

/**
 * Lookup a character in the input string.
 *
 * @return true, if lookup number of characters ahead are hex digits
 *         false, otherwise
 */
static bool
re_hex_lookup (lit_utf8_iterator_t iter, /**< input string iterator */
               uint32_t lookup) /**< size of lookup */
{
  ecma_char_t ch;
  bool is_digit = true;
  for (uint32_t i = 0; is_digit && i < lookup; i++)
  {
    if (!lit_utf8_iterator_is_eos (&iter))
    {
      ch = lit_utf8_iterator_read_next (&iter);
      is_digit = lit_char_is_hex_digit (ch);
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
 * @return true, if non-greedy character found
 *         false, otherwise
 */
static bool __attr_always_inline___
re_parse_non_greedy_char (lit_utf8_iterator_t *iter_p) /**< RegExp pattern */
{
  if (!lit_utf8_iterator_is_eos (iter_p) && lit_utf8_iterator_peek_next (iter_p) == LIT_CHAR_QUESTION)
  {
    lit_utf8_iterator_advance (iter_p, 1);
    return true;
  }

  return false;
} /* re_parse_non_greedy_char */

/**
 * Parse RegExp iterators
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
re_parse_iterator (re_parser_ctx_t *parser_ctx_p, /**< RegExp parser context */
                   re_token_t *re_token_p) /**< out: output token */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  lit_utf8_iterator_t *iter_p = &(parser_ctx_p->iter);
  re_token_p->qmin = 1;
  re_token_p->qmax = 1;
  re_token_p->greedy = true;

  if (lit_utf8_iterator_is_eos (iter_p))
  {
    return ret_value;
  }

  ecma_char_t ch = lit_utf8_iterator_peek_next (iter_p);

  switch (ch)
  {
    case LIT_CHAR_QUESTION:
    {
      lit_utf8_iterator_advance (iter_p, 1);
      re_token_p->qmin = 0;
      re_token_p->qmax = 1;
      re_token_p->greedy = !re_parse_non_greedy_char (iter_p);
      break;
    }
    case LIT_CHAR_ASTERISK:
    {
      lit_utf8_iterator_advance (iter_p, 1);
      re_token_p->qmin = 0;
      re_token_p->qmax = RE_ITERATOR_INFINITE;
      re_token_p->greedy = !re_parse_non_greedy_char (iter_p);
      break;
    }
    case LIT_CHAR_PLUS:
    {
      lit_utf8_iterator_advance (iter_p, 1);
      re_token_p->qmin = 1;
      re_token_p->qmax = RE_ITERATOR_INFINITE;
      re_token_p->greedy = !re_parse_non_greedy_char (iter_p);
      break;
    }
    case LIT_CHAR_LEFT_BRACE:
    {
      lit_utf8_iterator_advance (iter_p, 1);
      uint32_t qmin = 0;
      uint32_t qmax = RE_ITERATOR_INFINITE;
      uint32_t digits = 0;

      while (true)
      {
        if (lit_utf8_iterator_is_eos (iter_p))
        {
          return ecma_raise_syntax_error ("invalid quantifier");
        }

        ch = lit_utf8_iterator_read_next (iter_p);

        if (lit_char_is_decimal_digit (ch))
        {
          if (digits >= ECMA_NUMBER_MAX_DIGITS)
          {
            return ecma_raise_syntax_error ("RegExp quantifier error: too many digits.");
          }
          digits++;
          qmin = qmin * 10 + lit_char_hex_to_int (ch);
        }
        else if (ch == LIT_CHAR_COMMA)
        {
          if (qmax != RE_ITERATOR_INFINITE)
          {
            return ecma_raise_syntax_error ("RegExp quantifier error: double comma.");
          }

          if (lit_utf8_iterator_is_eos (iter_p))
          {
            return ecma_raise_syntax_error ("invalid quantifier");
          }

          if (lit_utf8_iterator_peek_next (iter_p) == LIT_CHAR_RIGHT_BRACE)
          {
            if (digits == 0)
            {
              return ecma_raise_syntax_error ("RegExp quantifier error: missing digits.");
            }

            lit_utf8_iterator_advance (iter_p, 1);
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
            return ecma_raise_syntax_error ("RegExp quantifier error: missing digits.");
          }

          if (qmax != RE_ITERATOR_INFINITE)
          {
            re_token_p->qmin = qmax;
            re_token_p->qmax = qmin;
          }
          else
          {
            re_token_p->qmin = qmin;
            re_token_p->qmax = qmin;
          }

          break;
        }
        else
        {
          return ecma_raise_syntax_error ("RegExp quantifier error: unknown char.");
        }
      }

      re_token_p->greedy = !re_parse_non_greedy_char (iter_p);
      break;
    }
    default:
    {
      break;
    }
  }

  JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));

  if (re_token_p->qmin > re_token_p->qmax)
  {
    ret_value = ecma_raise_syntax_error ("RegExp quantifier error: qmin > qmax.");
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
  lit_utf8_iterator_t iter = lit_utf8_iterator_create (parser_ctx_p->iter.buf_p,
                                                       parser_ctx_p->iter.buf_size);

  while (!lit_utf8_iterator_is_eos (&iter))
  {
    switch (lit_utf8_iterator_read_next (&iter))
    {
      case LIT_CHAR_BACKSLASH:
      {
        lit_utf8_iterator_advance (&iter, 1);
        break;
      }
      case LIT_CHAR_LEFT_SQUARE:
      {
        char_class_in++;
        break;
      }
      case LIT_CHAR_RIGHT_SQUARE:
      {
        if (!char_class_in)
        {
          char_class_in--;
        }
        break;
      }
      case LIT_CHAR_LEFT_PAREN:
      {
        if (!lit_utf8_iterator_is_eos (&iter)
            && lit_utf8_iterator_peek_next (&iter) != LIT_CHAR_QUESTION
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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
re_parse_char_class (re_parser_ctx_t *parser_ctx_p, /**< number of classes */
                     re_char_class_callback append_char_class, /**< callback function,
                                                                *   which adds the char-ranges
                                                                *   to the bytecode */
                     void* re_ctx_p, /**< regexp compiler context */
                     re_token_t *out_token_p) /**< out: output token */
{
  re_token_type_t token_type = ((re_compiler_ctx_t *) re_ctx_p)->current_token.type;
  out_token_p->qmax = out_token_p->qmin = 1;
  uint32_t start = RE_CHAR_UNDEF;
  bool is_range = false;
  parser_ctx_p->num_of_classes = 0;
  lit_utf8_iterator_t *iter_p = &(parser_ctx_p->iter);
  if (lit_utf8_iterator_peek_prev (iter_p) != LIT_CHAR_LEFT_SQUARE)
  {
    lit_utf8_iterator_read_prev (iter_p);
    lit_utf8_iterator_read_prev (iter_p);
  }

  do
  {
    if (lit_utf8_iterator_is_eos (iter_p))
    {
      return ecma_raise_syntax_error ("invalid character class, end of string");
    }

    uint32_t ch = lit_utf8_iterator_read_next (iter_p);

    if (ch == LIT_CHAR_RIGHT_SQUARE)
    {
      if (start != RE_CHAR_UNDEF)
      {
        append_char_class (re_ctx_p, start, start);
      }
      break;
    }
    else if (ch == LIT_CHAR_MINUS)
    {
      if (lit_utf8_iterator_is_eos (iter_p))
      {
        return ecma_raise_syntax_error ("invalid character class, end of string after '-'");
      }

      if (start != RE_CHAR_UNDEF
          && !is_range
          && lit_utf8_iterator_peek_next (iter_p) != LIT_CHAR_RIGHT_SQUARE)
      {
        is_range = true;
        continue;
      }
    }
    else if (ch == LIT_CHAR_BACKSLASH)
    {
      if (lit_utf8_iterator_is_eos (iter_p))
      {
        return ecma_raise_syntax_error ("invalid character class, end of string after '\\'");
      }

      ch = lit_utf8_iterator_read_next (iter_p);

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
        if (lit_utf8_iterator_is_eos (iter_p))
        {
          return ecma_raise_syntax_error ("invalid character class, end of string after '\\c'");
        }

        ch = lit_utf8_iterator_read_next (iter_p);

        if ((ch >= LIT_CHAR_ASCII_UPPERCASE_LETTERS_BEGIN && ch <= LIT_CHAR_ASCII_UPPERCASE_LETTERS_END)
            || (ch >= LIT_CHAR_ASCII_LOWERCASE_LETTERS_BEGIN && ch <= LIT_CHAR_ASCII_LOWERCASE_LETTERS_END))
        {
          /* See ECMA-262 v5, 15.10.2.10 (Point 3) */
          ch = (ch % 32);
        }
      }
      else if (ch == LIT_CHAR_LOWERCASE_X)
      {
        lit_code_point_t code_point;
        const lit_utf8_byte_t *hex_start_p = parser_ctx_p->iter.buf_p + parser_ctx_p->iter.buf_pos.offset;

        if (!lit_read_code_point_from_hex (hex_start_p, 2, &code_point))
        {
          return ecma_raise_syntax_error ("invalid character class, end of string after '\\x'");
        }

        lit_utf8_iterator_advance (iter_p, 2);

        append_char_class (re_ctx_p, code_point, code_point);
      }
      else if (ch == LIT_CHAR_LOWERCASE_U)
      {
        lit_code_point_t code_point;
        const lit_utf8_byte_t *hex_start_p = parser_ctx_p->iter.buf_p + parser_ctx_p->iter.buf_pos.offset;

        if (!lit_read_code_point_from_hex (hex_start_p, 4, &code_point))
        {
          return ecma_raise_syntax_error ("invalid character class, end of string after '\\u'");
        }

        lit_utf8_iterator_advance (iter_p, 4);

        append_char_class (re_ctx_p, code_point, code_point);
      }
      else if (ch == LIT_CHAR_LOWERCASE_D)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        append_char_class (re_ctx_p, LIT_CHAR_ASCII_DIGITS_BEGIN, LIT_CHAR_ASCII_DIGITS_END);
        ch = RE_CHAR_UNDEF;
      }
      else if (ch == LIT_CHAR_UPPERCASE_D)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        append_char_class (re_ctx_p, LIT_CHAR_NULL, LIT_CHAR_ASCII_DIGITS_BEGIN - 1);
        append_char_class (re_ctx_p, LIT_CHAR_ASCII_DIGITS_END + 1, LIT_UTF16_CODE_UNIT_MAX);
        ch = RE_CHAR_UNDEF;
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
        ch = RE_CHAR_UNDEF;
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
        ch = RE_CHAR_UNDEF;
      }
      else if (ch == LIT_CHAR_LOWERCASE_W)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        append_char_class (re_ctx_p, LIT_CHAR_0, LIT_CHAR_9);
        append_char_class (re_ctx_p, LIT_CHAR_UPPERCASE_A, LIT_CHAR_UPPERCASE_Z);
        append_char_class (re_ctx_p, LIT_CHAR_UNDERSCORE, LIT_CHAR_UNDERSCORE);
        append_char_class (re_ctx_p, LIT_CHAR_LOWERCASE_A, LIT_CHAR_LOWERCASE_Z);
        ch = RE_CHAR_UNDEF;
      }
      else if (ch == LIT_CHAR_UPPERCASE_W)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        append_char_class (re_ctx_p, LIT_CHAR_NULL, LIT_CHAR_0 - 1);
        append_char_class (re_ctx_p, LIT_CHAR_9 + 1, LIT_CHAR_UPPERCASE_A - 1);
        append_char_class (re_ctx_p, LIT_CHAR_UPPERCASE_Z + 1, LIT_CHAR_UNDERSCORE - 1);
        append_char_class (re_ctx_p, LIT_CHAR_UNDERSCORE + 1, LIT_CHAR_LOWERCASE_A - 1);
        append_char_class (re_ctx_p, LIT_CHAR_LOWERCASE_Z + 1, LIT_UTF16_CODE_UNIT_MAX);
        ch = RE_CHAR_UNDEF;
      }
      else if (ch <= LIT_UTF16_CODE_UNIT_MAX
               && lit_char_is_decimal_digit ((ecma_char_t) ch))
      {
        if (lit_utf8_iterator_is_eos (iter_p))
        {
          return ecma_raise_syntax_error ("invalid character class, end of string after '\\<digits>'");
        }

        if (ch != LIT_CHAR_0
            || lit_char_is_decimal_digit (lit_utf8_iterator_peek_next (iter_p)))
        {
          /* FIXME: octal support */
        }
      }
      /* FIXME: depends on the unicode support
      else if (!jerry_unicode_identifier (ch))
      {
        JERRY_ERROR_MSG ("RegExp escape pattern error. (Char class)");
      }
      */
    }

    if (ch == RE_CHAR_UNDEF)
    {
      if (start != RE_CHAR_UNDEF)
      {
        if (is_range)
        {
          return ecma_raise_syntax_error ("invalid character class, invalid range");
        }
        else
        {
          append_char_class (re_ctx_p, start, start);
          start = RE_CHAR_UNDEF;
        }
      }
    }
    else
    {
      if (start != RE_CHAR_UNDEF)
      {
        if (is_range)
        {
          if (start > ch)
          {
            return ecma_raise_syntax_error ("invalid character class, wrong order");
          }
          else
          {
            append_char_class (re_ctx_p, start, ch);
            start = RE_CHAR_UNDEF;
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
  }
  while (token_type == RE_TOK_START_CHAR_CLASS || token_type == RE_TOK_START_INV_CHAR_CLASS);

  return re_parse_iterator (parser_ctx_p, out_token_p);
} /* re_parse_char_class */

/**
 * Read the input pattern and parse the next token for the RegExp compiler
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
re_parse_next_token (re_parser_ctx_t *parser_ctx_p, /**< RegExp parser context */
                     re_token_t *out_token_p) /**< out: output token */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  lit_utf8_iterator_t *iter_p = &(parser_ctx_p->iter);

  if (lit_utf8_iterator_is_eos (iter_p))
  {
    out_token_p->type = RE_TOK_EOF;
    return ret_value;
  }

  ecma_char_t ch = lit_utf8_iterator_read_next (iter_p);

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
      if (lit_utf8_iterator_is_eos (iter_p))
      {
        return ecma_raise_syntax_error ("invalid regular experssion");
      }

      out_token_p->type = RE_TOK_CHAR;
      ch = lit_utf8_iterator_read_next (iter_p);

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
        if (lit_utf8_iterator_is_eos (iter_p))
        {
          out_token_p->value = ch;
          break;
        }

        ch = lit_utf8_iterator_peek_next (iter_p);

        if ((ch >= LIT_CHAR_ASCII_UPPERCASE_LETTERS_BEGIN && ch <= LIT_CHAR_ASCII_UPPERCASE_LETTERS_END)
            || (ch >= LIT_CHAR_ASCII_LOWERCASE_LETTERS_BEGIN && ch <= LIT_CHAR_ASCII_LOWERCASE_LETTERS_END))
        {
          out_token_p->value = (ch % 32);
          lit_utf8_iterator_advance (iter_p, 1);
        }
      }
      else if (ch == LIT_CHAR_LOWERCASE_X
               && re_hex_lookup (*iter_p, 2))
      {
        lit_code_point_t code_point;
        const lit_utf8_byte_t *hex_start_p = iter_p->buf_p + iter_p->buf_pos.offset;

        if (!lit_read_code_point_from_hex (hex_start_p, 2, &code_point))
        {
          return ecma_raise_syntax_error ("decode error");
        }

        out_token_p->value = code_point;
        lit_utf8_iterator_advance (iter_p, 2);
      }
      else if (ch == LIT_CHAR_LOWERCASE_U
               && re_hex_lookup (*iter_p, 4))
      {
        lit_code_point_t code_point;
        const lit_utf8_byte_t *hex_start_p = iter_p->buf_p + iter_p->buf_pos.offset;

        if (!lit_read_code_point_from_hex (hex_start_p, 4, &code_point))
        {
          return ecma_raise_syntax_error ("decode error");
        }

        out_token_p->value = code_point;
        lit_utf8_iterator_advance (iter_p, 4);
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
          if (!lit_utf8_iterator_is_eos (iter_p)
              && lit_char_is_decimal_digit (lit_utf8_iterator_peek_next (iter_p)))
          {
            return ecma_raise_syntax_error ("RegExp escape pattern error.");
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
            lit_utf8_iterator_read_prev (iter_p);
            uint32_t number = 0;
            int index = 0;

            do
            {
              if (index >= RE_MAX_RE_DECESC_DIGITS)
              {
                ret_value = ecma_raise_syntax_error ("RegExp escape pattern error: decimal escape too long.");
                return ret_value;
              }
              if (lit_utf8_iterator_is_eos (iter_p))
              {
                break;
              }

              ecma_char_t digit = lit_utf8_iterator_read_next (iter_p);

              if (!lit_char_is_decimal_digit (digit))
              {
                lit_utf8_iterator_read_prev (iter_p);
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

            out_token_p->value = number;
          }
          else
          {
            out_token_p->value = ch;
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
      if (lit_utf8_iterator_is_eos (iter_p))
      {
        return ecma_raise_syntax_error ("Unterminated group");
      }

      if (lit_utf8_iterator_peek_next (iter_p) == LIT_CHAR_QUESTION)
      {
        lit_utf8_iterator_advance (iter_p, 1);
        if (lit_utf8_iterator_is_eos (iter_p))
        {
          return ecma_raise_syntax_error ("Invalid group");
        }

        ch = lit_utf8_iterator_read_next (iter_p);

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
          return ecma_raise_syntax_error ("Invalid group");
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

      if (lit_utf8_iterator_is_eos (iter_p))
      {
        return ecma_raise_syntax_error ("invalid character class");
      }

      if (lit_utf8_iterator_peek_next (iter_p) == LIT_CHAR_CIRCUMFLEX)
      {
        out_token_p->type = RE_TOK_START_INV_CHAR_CLASS;
        lit_utf8_iterator_advance (iter_p, 1);
      }

      break;
    }
    case LIT_CHAR_QUESTION:
    case LIT_CHAR_ASTERISK:
    case LIT_CHAR_PLUS:
    case LIT_CHAR_LEFT_BRACE:
    {
      return ecma_raise_syntax_error ("Invalid RegExp token.");
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

#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */

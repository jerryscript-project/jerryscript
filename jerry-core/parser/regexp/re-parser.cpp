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
 * @return unicode codeunit
 */
static ecma_char_t
re_lookup (lit_utf8_iterator_t iter, /**< input string iterator */
           uint32_t lookup) /**< size of lookup */
{
  ecma_char_t ch = 0;
  for (uint32_t i = 0; i <= lookup; i++)
  {
    if (!lit_utf8_iterator_is_eos (&iter))
    {
      ch = lit_utf8_iterator_read_next (&iter);
    }
    else
    {
      ch = 0;
      break;
    }
  }

  return ch;
} /* re_lookup */

/**
 * Parse RegExp iterators
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
re_parse_iterator (lit_utf8_iterator_t iter, /**< RegExp pattern */
                   re_token_t *re_token_p, /**< output token */
                   uint32_t lookup, /**< size of lookup */
                   uint32_t *advance_p) /**< output length of current advance */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ecma_char_t ch0 = re_lookup (iter, lookup);
  ecma_char_t ch1 = re_lookup (iter, lookup + 1);

  switch (ch0)
  {
    case LIT_CHAR_QUESTION:
    {
      re_token_p->qmin = 0;
      re_token_p->qmax = 1;

      if (ch1 == LIT_CHAR_QUESTION)
      {
        *advance_p = 2;
        re_token_p->greedy = false;
      }
      else
      {
        *advance_p = 1;
        re_token_p->greedy = true;
      }
      break;
    }
    case LIT_CHAR_ASTERISK:
    {
      re_token_p->qmin = 0;
      re_token_p->qmax = RE_ITERATOR_INFINITE;

      if (ch1 == LIT_CHAR_QUESTION)
      {
        *advance_p = 2;
        re_token_p->greedy = false;
      }
      else
      {
        *advance_p = 1;
        re_token_p->greedy = true;
      }
      break;
    }
    case LIT_CHAR_PLUS:
    {
      re_token_p->qmin = 1;
      re_token_p->qmax = RE_ITERATOR_INFINITE;

      if (ch1 == LIT_CHAR_QUESTION)
      {
        *advance_p = 2;
        re_token_p->greedy = false;
      }
      else
      {
        *advance_p = 1;
        re_token_p->greedy = true;
      }
      break;
    }
    case LIT_CHAR_LEFT_BRACE:
    {
      uint32_t qmin = 0;
      uint32_t qmax = RE_ITERATOR_INFINITE;
      uint32_t digits = 0;

      while (true)
      {
        (*advance_p)++;
        ch1 = re_lookup (iter, lookup + *advance_p);

        if (lit_char_is_decimal_digit (ch1))
        {
          if (digits >= ECMA_NUMBER_MAX_DIGITS)
          {
            ret_value = ecma_raise_syntax_error ("RegExp quantifier error: too many digits.");
            return ret_value;
          }
          digits++;
          qmin = qmin * 10 + lit_char_hex_to_int (ch1);
        }
        else if (ch1 == LIT_CHAR_COMMA)
        {
          if (qmax != RE_ITERATOR_INFINITE)
          {
            ret_value = ecma_raise_syntax_error ("RegExp quantifier error: double comma.");
            return ret_value;
          }
          if ((re_lookup (iter, lookup + *advance_p + 1)) == LIT_CHAR_RIGHT_BRACE)
          {
            if (digits == 0)
            {
              ret_value = ecma_raise_syntax_error ("RegExp quantifier error: missing digits.");
              return ret_value;
            }

            re_token_p->qmin = qmin;
            re_token_p->qmax = RE_ITERATOR_INFINITE;
            *advance_p += 2;
            break;
          }
          qmax = qmin;
          qmin = 0;
          digits = 0;
        }
        else if (ch1 == LIT_CHAR_RIGHT_BRACE)
        {
          if (digits == 0)
          {
            ret_value = ecma_raise_syntax_error ("RegExp quantifier error: missing digits.");
            return ret_value;
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

          *advance_p += 1;
          break;
        }
        else
        {
          ret_value = ecma_raise_syntax_error ("RegExp quantifier error: unknown char.");
          return ret_value;
        }
      }

      if ((re_lookup (iter, lookup + *advance_p)) == LIT_CHAR_QUESTION)
      {
        re_token_p->greedy = false;
        *advance_p += 1;
      }
      else
      {
        re_token_p->greedy = true;
      }
      break;

      JERRY_UNREACHABLE ();
      break;
    }
    default:
    {
      re_token_p->qmin = 1;
      re_token_p->qmax = 1;
      re_token_p->greedy = true;
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
    ecma_char_t ch0 = lit_utf8_iterator_read_next (&iter);

    switch (ch0)
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
                     re_token_t *out_token_p) /**< output token */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  re_token_type_t token_type = ((re_compiler_ctx_t *) re_ctx_p)->current_token.type;
  out_token_p->qmax = out_token_p->qmin = 1;
  uint32_t start = RE_CHAR_UNDEF;
  bool is_range = false;
  parser_ctx_p->num_of_classes = 0;

  do
  {
    if (lit_utf8_iterator_is_eos (&(parser_ctx_p->iter)))
    {
      ret_value = ecma_raise_syntax_error ("invalid character class");
      return ret_value;
    }

    uint32_t ch = lit_utf8_iterator_read_next (&(parser_ctx_p->iter));

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
      if (start != RE_CHAR_UNDEF && !is_range && re_lookup (parser_ctx_p->iter, 0) != LIT_CHAR_RIGHT_SQUARE)
      {
        is_range = true;
        continue;
      }
    }
    else if (ch == LIT_CHAR_BACKSLASH)
    {
      if (lit_utf8_iterator_is_eos (&(parser_ctx_p->iter)))
      {
        ret_value = ecma_raise_syntax_error ("invalid character class");
        return ret_value;
      }

      ch = lit_utf8_iterator_read_next (&(parser_ctx_p->iter));

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
        if (lit_utf8_iterator_is_eos (&(parser_ctx_p->iter)))
        {
          ret_value = ecma_raise_syntax_error ("decode error");
          return ret_value;
        }

        if (lit_utf8_iterator_is_eos (&(parser_ctx_p->iter)))
        {
          ret_value = ecma_raise_syntax_error ("invalid character class");
          return ret_value;
        }

        ch = lit_utf8_iterator_read_next (&(parser_ctx_p->iter));

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
        const lit_utf8_byte_t *hex_start = parser_ctx_p->iter.buf_p + parser_ctx_p->iter.buf_pos.offset;

        if (!lit_read_code_point_from_hex (hex_start, 2, &code_point))
        {
          ret_value = ecma_raise_syntax_error ("decode error");
          return ret_value;
        }

        lit_utf8_iterator_advance (&(parser_ctx_p->iter), 2);

        append_char_class (re_ctx_p, code_point, code_point);
      }
      else if (ch == LIT_CHAR_LOWERCASE_U)
      {
        lit_code_point_t code_point;
        const lit_utf8_byte_t *hex_start = parser_ctx_p->iter.buf_p + parser_ctx_p->iter.buf_pos.offset;

        if (!lit_read_code_point_from_hex (hex_start, 4, &code_point))
        {
          ret_value = ecma_raise_syntax_error ("decode error");
          return ret_value;
        }

        lit_utf8_iterator_advance (&(parser_ctx_p->iter), 4);

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
        if (ch != LIT_CHAR_0
            || lit_char_is_decimal_digit (re_lookup (parser_ctx_p->iter, 1)))
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
          ret_value = ecma_raise_syntax_error ("invalid character class range");
          return ret_value;
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
            ret_value = ecma_raise_syntax_error ("invalid character class range");
            return ret_value;
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

  uint32_t advance = 0;
  ECMA_TRY_CATCH (empty,
                  re_parse_iterator (parser_ctx_p->iter,
                                     out_token_p,
                                     0,
                                     &advance),
                  ret_value);
  lit_utf8_iterator_advance (&(parser_ctx_p->iter), advance);
  ECMA_FINALIZE (empty);

  return ret_value;
} /* re_parse_char_class */

/**
 * Read the input pattern and parse the next token for the RegExp compiler
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
re_parse_next_token (re_parser_ctx_t *parser_ctx_p, /**< RegExp parser context */
                     re_token_t *out_token_p) /**< output token */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (lit_utf8_iterator_is_eos (&(parser_ctx_p->iter)))
  {
    out_token_p->type = RE_TOK_EOF;
    return ret_value;
  }

  uint32_t advance = 0;
  ecma_char_t ch0 = lit_utf8_iterator_peek_next (&(parser_ctx_p->iter));

  switch (ch0)
  {
    case LIT_CHAR_VLINE:
    {
      advance = 1;
      out_token_p->type = RE_TOK_ALTERNATIVE;
      break;
    }
    case LIT_CHAR_CIRCUMFLEX:
    {
      advance = 1;
      out_token_p->type = RE_TOK_ASSERT_START;
      break;
    }
    case LIT_CHAR_DOLLAR_SIGN:
    {
      advance = 1;
      out_token_p->type = RE_TOK_ASSERT_END;
      break;
    }
    case LIT_CHAR_DOT:
    {
      ECMA_TRY_CATCH (empty,
                      re_parse_iterator (parser_ctx_p->iter,
                                         out_token_p,
                                         1,
                                         &advance),
                      ret_value);
      advance += 1;
      out_token_p->type = RE_TOK_PERIOD;
      ECMA_FINALIZE (empty);
      break;
    }
    case LIT_CHAR_BACKSLASH:
    {
      out_token_p->type = RE_TOK_CHAR;
      ecma_char_t ch1 = re_lookup (parser_ctx_p->iter, 1);

      if (ch1 == LIT_CHAR_LOWERCASE_B)
      {
        advance = 2;
        out_token_p->type = RE_TOK_ASSERT_WORD_BOUNDARY;
      }
      else if (ch1 == LIT_CHAR_UPPERCASE_B)
      {
        advance = 2;
        out_token_p->type = RE_TOK_ASSERT_NOT_WORD_BOUNDARY;
      }
      else if (ch1 == LIT_CHAR_LOWERCASE_F)
      {
        out_token_p->value = LIT_CHAR_FF;
      }
      else if (ch1 == LIT_CHAR_LOWERCASE_N)
      {
        out_token_p->value = LIT_CHAR_LF;
      }
      else if (ch1 == LIT_CHAR_LOWERCASE_T)
      {
        out_token_p->value = LIT_CHAR_TAB;
      }
      else if (ch1 == LIT_CHAR_LOWERCASE_R)
      {
        out_token_p->value = LIT_CHAR_CR;
      }
      else if (ch1 == LIT_CHAR_LOWERCASE_V)
      {
        out_token_p->value = LIT_CHAR_VTAB;
      }
      else if (ch1 == LIT_CHAR_LOWERCASE_C)
      {
        lit_utf8_iterator_advance (&(parser_ctx_p->iter), 2);

        if (lit_utf8_iterator_is_eos (&(parser_ctx_p->iter)))
        {
          ret_value = ecma_raise_syntax_error ("invalid character class");
          return ret_value;
        }

        ecma_char_t ch2 = lit_utf8_iterator_read_next (&(parser_ctx_p->iter));

        if ((ch2 >= LIT_CHAR_ASCII_UPPERCASE_LETTERS_BEGIN && ch2 <= LIT_CHAR_ASCII_UPPERCASE_LETTERS_END)
            || (ch2 >= LIT_CHAR_ASCII_LOWERCASE_LETTERS_BEGIN && ch2 <= LIT_CHAR_ASCII_LOWERCASE_LETTERS_END))
        {
          out_token_p->type = RE_TOK_CHAR;
          out_token_p->value = (ch2 % 32);
        }
      }
      else if (ch1 == LIT_CHAR_LOWERCASE_X
               && lit_char_is_hex_digit (re_lookup (parser_ctx_p->iter, 2))
               && lit_char_is_hex_digit (re_lookup (parser_ctx_p->iter, 3)))
      {
        lit_utf8_iterator_advance (&(parser_ctx_p->iter), 2);

        lit_code_point_t code_point;
        const lit_utf8_byte_t *hex_start = parser_ctx_p->iter.buf_p + parser_ctx_p->iter.buf_pos.offset;

        if (!lit_read_code_point_from_hex (hex_start, 2, &code_point))
        {
          ret_value = ecma_raise_syntax_error ("decode error");
          return ret_value;
        }

        lit_utf8_iterator_advance (&(parser_ctx_p->iter), 2);

        out_token_p->type = RE_TOK_CHAR;
        out_token_p->value = code_point;
      }
      else if (ch1 == LIT_CHAR_LOWERCASE_U
               && lit_char_is_hex_digit (re_lookup (parser_ctx_p->iter, 2))
               && lit_char_is_hex_digit (re_lookup (parser_ctx_p->iter, 3))
               && lit_char_is_hex_digit (re_lookup (parser_ctx_p->iter, 4))
               && lit_char_is_hex_digit (re_lookup (parser_ctx_p->iter, 5)))
      {
        lit_utf8_iterator_advance (&(parser_ctx_p->iter), 2);

        lit_code_point_t code_point;
        const lit_utf8_byte_t *hex_start = parser_ctx_p->iter.buf_p + parser_ctx_p->iter.buf_pos.offset;

        if (!lit_read_code_point_from_hex (hex_start, 4, &code_point))
        {
          ret_value = ecma_raise_syntax_error ("decode error");
          return ret_value;
        }

        lit_utf8_iterator_advance (&(parser_ctx_p->iter), 4);

        out_token_p->type = RE_TOK_CHAR;
        out_token_p->value = code_point;
      }
      else if (ch1 == LIT_CHAR_LOWERCASE_D)
      {
        out_token_p->type = RE_TOK_DIGIT;
      }
      else if (ch1 == LIT_CHAR_UPPERCASE_D)
      {
        out_token_p->type = RE_TOK_NOT_DIGIT;
      }
      else if (ch1 == LIT_CHAR_LOWERCASE_S)
      {
        out_token_p->type = RE_TOK_WHITE;
      }
      else if (ch1 == LIT_CHAR_UPPERCASE_S)
      {
        out_token_p->type = RE_TOK_NOT_WHITE;
      }
      else if (ch1 == LIT_CHAR_LOWERCASE_W)
      {
        out_token_p->type = RE_TOK_WORD_CHAR;
      }
      else if (ch1 == LIT_CHAR_UPPERCASE_W)
      {
        out_token_p->type = RE_TOK_NOT_WORD_CHAR;
      }
      else if (lit_char_is_decimal_digit (ch1))
      {
        if (ch1 == LIT_CHAR_0)
        {
          if (lit_char_is_decimal_digit (re_lookup (parser_ctx_p->iter, 2)))
          {
            ret_value = ecma_raise_syntax_error ("RegExp escape pattern error.");
            break;
          }

          advance = 2;
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
            uint32_t number = 0;
            int index = 0;

            do
            {
              if (index >= RE_MAX_RE_DECESC_DIGITS)
              {
                ret_value = ecma_raise_syntax_error ("RegExp escape pattern error: decimal escape too long.");
                return ret_value;
              }

              advance++;
              ecma_char_t digit = re_lookup (parser_ctx_p->iter,
                                             advance);
              if (!lit_char_is_decimal_digit (digit))
              {
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
            out_token_p->value = ch1;
          }
        }
      }
      else
      {
        advance = 2;
        out_token_p->value = ch1;
      }

      uint32_t iter_adv = 0;
      ECMA_TRY_CATCH (empty,
                      re_parse_iterator (parser_ctx_p->iter,
                                         out_token_p,
                                         advance,
                                         &iter_adv),
                      ret_value);
      advance += iter_adv;
      ECMA_FINALIZE (empty);
      break;
    }
    case LIT_CHAR_LEFT_PAREN:
    {
      if (re_lookup (parser_ctx_p->iter, 1) == LIT_CHAR_QUESTION)
      {
        ecma_char_t ch2 = re_lookup (parser_ctx_p->iter, 2);

        if (ch2 == LIT_CHAR_EQUALS)
        {
          /* (?= */
          advance = 3;
          out_token_p->type = RE_TOK_ASSERT_START_POS_LOOKAHEAD;
        }
        else if (ch2 == LIT_CHAR_EXCLAMATION)
        {
          /* (?! */
          advance = 3;
          out_token_p->type = RE_TOK_ASSERT_START_NEG_LOOKAHEAD;
        }
        else if (ch2 == LIT_CHAR_COLON)
        {
          /* (?: */
          advance = 3;
          out_token_p->type = RE_TOK_START_NON_CAPTURE_GROUP;
        }
      }
      else
      {
        /* ( */
        advance = 1;
        out_token_p->type = RE_TOK_START_CAPTURE_GROUP;
      }
      break;
    }
    case LIT_CHAR_RIGHT_PAREN:
    {
      ECMA_TRY_CATCH (empty,
                      re_parse_iterator (parser_ctx_p->iter,
                                         out_token_p,
                                         1,
                                         &advance),
                      ret_value);
      advance += 1;
      out_token_p->type = RE_TOK_END_GROUP;
      ECMA_FINALIZE (empty);
      break;
    }
    case LIT_CHAR_LEFT_SQUARE:
    {
      advance = 1;
      out_token_p->type = RE_TOK_START_CHAR_CLASS;
      if (re_lookup (parser_ctx_p->iter, 1) == LIT_CHAR_CIRCUMFLEX)
      {
        advance = 2;
        out_token_p->type = RE_TOK_START_INV_CHAR_CLASS;
      }
      break;
    }
    case LIT_CHAR_RIGHT_SQUARE:
    case LIT_CHAR_RIGHT_BRACE:
    case LIT_CHAR_QUESTION:
    case LIT_CHAR_ASTERISK:
    case LIT_CHAR_PLUS:
    case LIT_CHAR_LEFT_BRACE:
    {
      ret_value = ecma_raise_syntax_error ("Invalid RegExp token.");
      return ret_value;
    }
    case LIT_CHAR_NULL:
    {
      advance = 0;
      out_token_p->type = RE_TOK_EOF;
      break;
    }
    default:
    {
      ECMA_TRY_CATCH (empty,
                      re_parse_iterator (parser_ctx_p->iter,
                                         out_token_p,
                                         1,
                                         &advance),
                      ret_value);
      advance += 1;
      out_token_p->type = RE_TOK_CHAR;
      out_token_p->value = ch0;
      ECMA_FINALIZE (empty);
      break;
    }
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    lit_utf8_iterator_advance (&(parser_ctx_p->iter), advance);
  }

  return ret_value;
} /* re_parse_next_token */

#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */

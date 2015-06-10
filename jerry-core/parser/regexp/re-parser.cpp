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

#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-try-catch-macro.h"
#include "jrt-libc-includes.h"
#include "re-parser.h"
#include "syntax-errors.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN

#define RE_LOOKUP(str_p, lookup)  *(str_p + lookup)
#define RE_ADVANCE(str_p, advance) do { str_p += advance; } while (0)

static uint32_t
hex_to_int (ecma_char_t hex)
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

static ecma_char_t
get_ecma_char (ecma_char_t** char_p)
{
  ecma_char_t ch = **char_p;
  RE_ADVANCE (*char_p, 1);
  return ch;
}

/**
 * Parse RegExp iterators
 */
static ecma_completion_value_t
parse_re_iterator (ecma_char_t *pattern_p, /**< RegExp pattern */
                   re_token_t *re_token_p, /**< output token */
                   uint32_t lookup, /**< size of lookup */
                   uint32_t *advance_p) /**< output length of current advance */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  re_token_p->qmin = 1;
  re_token_p->qmax = 1;
  re_token_p->greedy = true;

  ecma_char_t ch0 = RE_LOOKUP (pattern_p, lookup);
  ecma_char_t ch1 = RE_LOOKUP (pattern_p, lookup + 1);

  switch (ch0)
  {
    case '?':
    {
      re_token_p->qmin = 0;
      re_token_p->qmax = 1;
      if (ch1 == '?')
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
    case '*':
    {
      re_token_p->qmin = 0;
      re_token_p->qmax = RE_ITERATOR_INFINITE;
      if (ch1 == '?')
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
    case '+':
    {
      re_token_p->qmin = 1;
      re_token_p->qmax = RE_ITERATOR_INFINITE;
      if (ch1 == '?')
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
    case '{':
    {
      uint32_t qmin = 0;
      uint32_t qmax = RE_ITERATOR_INFINITE;
      uint32_t digits = 0;
      while (true)
      {
        (*advance_p)++;
        ch1 = RE_LOOKUP (pattern_p, lookup + *advance_p);

        if (isdigit (ch1))
        {
          if (digits >= ECMA_NUMBER_MAX_DIGITS)
          {
            SYNTAX_ERROR_OBJ (ret_value, "RegExp quantifier error: too many digits.");
            return ret_value;
          }
          digits++;
          qmin = qmin * 10 + hex_to_int (ch1);
        }
        else if (ch1 == ',')
        {
          if (qmax != RE_ITERATOR_INFINITE)
          {
            SYNTAX_ERROR_OBJ (ret_value, "RegExp quantifier error: double comma.");
            return ret_value;
          }
          if ((RE_LOOKUP (pattern_p, lookup + *advance_p + 1)) == '}')
          {
            if (digits == 0)
            {
              SYNTAX_ERROR_OBJ (ret_value, "RegExp quantifier error: missing digits.");
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
        else if (ch1 == '}')
        {
          if (digits == 0)
          {
            SYNTAX_ERROR_OBJ (ret_value, "RegExp quantifier error: missing digits.");
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
          SYNTAX_ERROR_OBJ (ret_value, "RegExp quantifier error: unknown char.");
          return ret_value;
        }
      }

      if ((RE_LOOKUP (pattern_p, lookup + *advance_p)) == '?')
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
  }

  JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));

  if (re_token_p->qmin > re_token_p->qmax)
  {
    SYNTAX_ERROR_OBJ (ret_value, "RegExp quantifier error: qmin > qmax.");
  }

  return ret_value;
}

/**
 * Count the number of groups in pattern
 */
static void
re_count_num_of_groups (re_parser_ctx_t *parser_ctx_p) /**< RegExp parser context */
{
  ecma_char_t **pattern_p = &(parser_ctx_p->pattern_start_p);
  ecma_char_t ch1;
  int char_class_in = 0;
  parser_ctx_p->num_of_groups = 0;

  ch1 = get_ecma_char (pattern_p);
  while (ch1 != '\0')
  {
    ecma_char_t ch0 = ch1;
    ch1 = get_ecma_char (pattern_p);
    switch (ch0)
    {
      case '\\':
      {
        ch1 = get_ecma_char (pattern_p);
        break;
      }
      case '[':
      {
        char_class_in++;
        break;
      }
      case ']':
      {
        if (!char_class_in)
        {
          char_class_in--;
        }
        break;
      }
      case '(':
      {
        if (ch1 != '?' && !char_class_in)
        {
          parser_ctx_p->num_of_groups++;
        }
        break;
      }
    }
  }
}

/**
 * Read the input pattern and parse the range of character class
 *
 * @return current parsed token
 */
ecma_completion_value_t
re_parse_char_class (re_parser_ctx_t *parser_ctx_p, /**< number of classes */
                     re_char_class_callback append_char_class, /**< callback function,
                                                                *   which add the char-ranges
                                                                *   to the bytecode */
                     void* re_ctx_p, /**< regexp compiler context */
                     re_token_t *out_token_p) /**< output token */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  ecma_char_t **pattern_p = &(parser_ctx_p->current_char_p);

  out_token_p->qmax = out_token_p->qmin = 1;
  ecma_char_t start = RE_CHAR_UNDEF;
  bool is_range = false;
  parser_ctx_p->num_of_classes = 0;

  do
  {
    ecma_char_t ch = get_ecma_char (pattern_p);
    if (ch == ']')
    {
      if (start != RE_CHAR_UNDEF)
      {
        append_char_class (re_ctx_p, start, start);
      }
      break;
    }
    else if (ch == '-')
    {
      if (start != RE_CHAR_UNDEF && !is_range && RE_LOOKUP (*pattern_p, 0) != ']')
      {
        is_range = true;
        continue;
      }
    }
    else if (ch == '\\')
    {
      ch = get_ecma_char (pattern_p);

      if (ch == 'b')
      {
        ch = RE_CONTROL_CHAR_BEL;
      }
      else if (ch == 'f')
      {
        ch = RE_CONTROL_CHAR_FF;
      }
      else if (ch == 'n')
      {
        ch = RE_CONTROL_CHAR_EOL;
      }
      else if (ch == 't')
      {
        ch = RE_CONTROL_CHAR_TAB;
      }
      else if (ch == 'r')
      {
        ch = RE_CONTROL_CHAR_CR;
      }
      else if (ch == 'v')
      {
        ch = RE_CONTROL_CHAR_VT;
      }
      else if (ch == 'c')
      {
        ch = get_ecma_char (pattern_p);
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
        {
          ch = (ch % 32);
        }
        else
        {
          SYNTAX_ERROR_OBJ (ret_value, "invalid regexp control escape");
          return ret_value;
        }
      }
      else if (ch == 'x')
      {
        /* FIXME: get unicode char from hex-digits */
        /* ch = ...; */
      }
      else if (ch == 'u')
      {
        /* FIXME: get unicode char from digits */
        /* ch = ...; */
      }
      else if (ch == 'd')
      {
        /* append digits from '0' to '9'. */
        append_char_class (re_ctx_p, 0x0030UL, 0x0039UL);
        ch = RE_CHAR_UNDEF;
      }
      else if (ch == 'D')
      {
        append_char_class (re_ctx_p, 0x0000UL, 0x002FUL);
        append_char_class (re_ctx_p, 0x003AUL, 0xFFFFUL);
        ch = RE_CHAR_UNDEF;
      }
      else if (ch == 's')
      {
        append_char_class (re_ctx_p, 0x0009UL, 0x000DUL);
        append_char_class (re_ctx_p, 0x0020UL, 0x0020UL);
        append_char_class (re_ctx_p, 0x00A0UL, 0x00A0UL);
        append_char_class (re_ctx_p, 0x1680UL, 0x1680UL);
        append_char_class (re_ctx_p, 0x180EUL, 0x180EUL);
        append_char_class (re_ctx_p, 0x2000UL, 0x200AUL);
        append_char_class (re_ctx_p, 0x2028UL, 0x2029UL);
        append_char_class (re_ctx_p, 0x202FUL, 0x202FUL);
        append_char_class (re_ctx_p, 0x205FUL, 0x205FUL);
        append_char_class (re_ctx_p, 0x3000UL, 0x3000UL);
        append_char_class (re_ctx_p, 0xFEFFUL, 0xFEFFUL);
        ch = RE_CHAR_UNDEF;
      }
      else if (ch == 'S')
      {
        append_char_class (re_ctx_p, 0x0000UL, 0x0008UL);
        append_char_class (re_ctx_p, 0x000EUL, 0x001FUL);
        append_char_class (re_ctx_p, 0x0021UL, 0x009FUL);
        append_char_class (re_ctx_p, 0x00A1UL, 0x167FUL);
        append_char_class (re_ctx_p, 0x1681UL, 0x180DUL);
        append_char_class (re_ctx_p, 0x180FUL, 0x1FFFUL);
        append_char_class (re_ctx_p, 0x200BUL, 0x2027UL);
        append_char_class (re_ctx_p, 0x202AUL, 0x202EUL);
        append_char_class (re_ctx_p, 0x2030UL, 0x205EUL);
        append_char_class (re_ctx_p, 0x2060UL, 0x2FFFUL);
        append_char_class (re_ctx_p, 0x3001UL, 0xFEFEUL);
        append_char_class (re_ctx_p, 0xFF00UL, 0xFFFFUL);
        ch = RE_CHAR_UNDEF;
      }
      else if (ch == 'w')
      {
        append_char_class (re_ctx_p, 0x0030UL, 0x0039UL);
        append_char_class (re_ctx_p, 0x0041UL, 0x005AUL);
        append_char_class (re_ctx_p, 0x005FUL, 0x005FUL);
        append_char_class (re_ctx_p, 0x0061UL, 0x007AUL);
        ch = RE_CHAR_UNDEF;
      }
      else if (ch == 'W')
      {
        append_char_class (re_ctx_p, 0x0000UL, 0x002FUL);
        append_char_class (re_ctx_p, 0x003AUL, 0x0040UL);
        append_char_class (re_ctx_p, 0x005BUL, 0x005EUL);
        append_char_class (re_ctx_p, 0x0060UL, 0x0060UL);
        append_char_class (re_ctx_p, 0x007BUL, 0xFFFFUL);
        ch = RE_CHAR_UNDEF;
      }
      else if (isdigit (ch))
      {
        if (ch != '\0' || isdigit (RE_LOOKUP (*pattern_p, 1)))
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
          SYNTAX_ERROR_OBJ (ret_value, "invalid character class range");
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
            SYNTAX_ERROR_OBJ (ret_value, "invalid character class range");
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
  while (true);

  uint32_t advance = 0;
  ECMA_TRY_CATCH (empty,
                  parse_re_iterator (parser_ctx_p->current_char_p,
                                     out_token_p,
                                     0,
                                     &advance),
                  ret_value);
  RE_ADVANCE (parser_ctx_p->current_char_p, advance);
  ECMA_FINALIZE (empty);

  return ret_value;
} /* re_parse_char_class */

/**
 * Read the input pattern and parse the next token for the RegExp compiler
 */
ecma_completion_value_t
re_parse_next_token (re_parser_ctx_t *parser_ctx_p, /**< RegExp parser context */
                     re_token_t *out_token_p) /**< output token */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  uint32_t advance = 0;
  ecma_char_t ch0 = *(parser_ctx_p->current_char_p);

  switch (ch0)
  {
    case '|':
    {
      advance = 1;
      out_token_p->type = RE_TOK_ALTERNATIVE;
      break;
    }
    case '^':
    {
      advance = 1;
      out_token_p->type = RE_TOK_ASSERT_START;
      break;
    }
    case '$':
    {
      advance = 1;
      out_token_p->type = RE_TOK_ASSERT_END;
      break;
    }
    case '.':
    {
      ECMA_TRY_CATCH (empty,
                      parse_re_iterator (parser_ctx_p->current_char_p,
                                         out_token_p,
                                         1,
                                         &advance),
                      ret_value);
      advance += 1;
      out_token_p->type = RE_TOK_PERIOD;
      ECMA_FINALIZE (empty);
      break;
    }
    case '\\':
    {
      advance = 2;
      out_token_p->type = RE_TOK_CHAR;
      ecma_char_t ch1 = RE_LOOKUP (parser_ctx_p->current_char_p, 1);

      if (ch1 == 'b')
      {
        out_token_p->type = RE_TOK_ASSERT_WORD_BOUNDARY;
      }
      else if (ch1 == 'B')
      {
        out_token_p->type = RE_TOK_ASSERT_NOT_WORD_BOUNDARY;
      }
      else if (ch1 == 'f')
      {
        out_token_p->value = RE_CONTROL_CHAR_FF;
      }
      else if (ch1 == 'n')
      {
        out_token_p->value = RE_CONTROL_CHAR_EOL;
      }
      else if (ch1 == 't')
      {
        out_token_p->value = RE_CONTROL_CHAR_TAB;
      }
      else if (ch1 == 'r')
      {
        out_token_p->value = RE_CONTROL_CHAR_CR;
      }
      else if (ch1 == 'v')
      {
        out_token_p->value = RE_CONTROL_CHAR_VT;
      }
      else if (ch1 == 'c')
      {
        ecma_char_t ch2 = RE_LOOKUP (parser_ctx_p->current_char_p, 2);
        if ((ch2 >= 'A' && ch2 <= 'Z') || (ch2 >= 'a' && ch2 <= 'z'))
        {
          advance = 3;
          out_token_p->type = RE_TOK_CHAR;
          out_token_p->value = (ch2 % 32);
        }
        else
        {
          SYNTAX_ERROR_OBJ (ret_value, "invalid regexp control escape");
          break;
        }
      }
      else if (ch1 == 'x'
               && isxdigit (RE_LOOKUP (parser_ctx_p->current_char_p, 2))
               && isxdigit (RE_LOOKUP (parser_ctx_p->current_char_p, 3)))
      {
        advance = 4;
        out_token_p->type = RE_TOK_CHAR;
        /* FIXME: get unicode char from hex-digits */
        /* result.value = ...; */
      }
      else if (ch1 == 'u'
               && isxdigit (RE_LOOKUP (parser_ctx_p->current_char_p, 2))
               && isxdigit (RE_LOOKUP (parser_ctx_p->current_char_p, 3))
               && isxdigit (RE_LOOKUP (parser_ctx_p->current_char_p, 4))
               && isxdigit (RE_LOOKUP (parser_ctx_p->current_char_p, 5)))
      {
        advance = 4;
        out_token_p->type = RE_TOK_CHAR;
        /* FIXME: get unicode char from digits */
        /* result.value = ...; */
      }
      else if (ch1 == 'd')
      {
        advance = 2;
        out_token_p->type = RE_TOK_DIGIT;
      }
      else if (ch1 == 'D')
      {
        advance = 2;
        out_token_p->type = RE_TOK_NOT_DIGIT;
      }
      else if (ch1 == 's')
      {
        advance = 2;
        out_token_p->type = RE_TOK_WHITE;
      }
      else if (ch1 == 'S')
      {
        advance = 2;
        out_token_p->type = RE_TOK_NOT_WHITE;
      }
      else if (ch1 == 'w')
      {
        advance = 2;
        out_token_p->type = RE_TOK_WORD_CHAR;
      }
      else if (ch1 == 'W')
      {
        advance = 2;
        out_token_p->type = RE_TOK_NOT_WORD_CHAR;
      }
      else if (isdigit (ch1))
      {
        if (ch1 == '0')
        {
          if (isdigit (RE_LOOKUP (parser_ctx_p->current_char_p, 2)))
          {
            SYNTAX_ERROR_OBJ (ret_value, "RegExp escape pattern error.");
            break;
          }

          advance = 2;
          out_token_p->value = RE_CONTROL_CHAR_NUL;
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
            advance = 0;

            do
            {
              if (index >= RE_MAX_RE_DECESC_DIGITS)
              {
                SYNTAX_ERROR_OBJ (ret_value, "RegExp escape pattern error: decimal escape too long.");
                return ret_value;
              }

              advance++;
              ecma_char_t digit = RE_LOOKUP (parser_ctx_p->current_char_p, advance);
              if (!isdigit (digit))
              {
                break;
              }
              number = number * 10 + hex_to_int (digit);
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
        out_token_p->value = ch1;
      }

      uint32_t iter_adv = 0;
      ECMA_TRY_CATCH (empty,
                      parse_re_iterator (parser_ctx_p->current_char_p,
                                         out_token_p,
                                         advance,
                                         &iter_adv),
                      ret_value);
      advance += iter_adv;
      ECMA_FINALIZE (empty);
      break;
    }
    case '(':
    {
      if (RE_LOOKUP (parser_ctx_p->current_char_p, 1) == '?')
      {
        ecma_char_t ch2 = RE_LOOKUP (parser_ctx_p->current_char_p, 2);
        if (ch2 == '=')
        {
          /* (?= */
          advance = 3;
          out_token_p->type = RE_TOK_ASSERT_START_POS_LOOKAHEAD;
        }
        else if (ch2 == '!')
        {
          /* (?! */
          advance = 3;
          out_token_p->type = RE_TOK_ASSERT_START_NEG_LOOKAHEAD;
        }
        else if (ch2 == ':')
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
    case ')':
    {
      ECMA_TRY_CATCH (empty,
                      parse_re_iterator (parser_ctx_p->current_char_p,
                                         out_token_p,
                                         1,
                                         &advance),
                      ret_value);
      advance += 1;
      out_token_p->type = RE_TOK_END_GROUP;
      ECMA_FINALIZE (empty);
      break;
    }
    case '[':
    {
      advance = 1;
      out_token_p->type = RE_TOK_START_CHAR_CLASS;
      if (RE_LOOKUP (parser_ctx_p->current_char_p, 1) == '^')
      {
        advance = 2;
        out_token_p->type = RE_TOK_START_INV_CHAR_CLASS;
      }
      break;
    }
    case ']':
    case '}':
    case '?':
    case '*':
    case '+':
    case '{':
    {
      JERRY_UNREACHABLE ();
      break;
    }
    case '\0':
    {
      advance = 0;
      out_token_p->type = RE_TOK_EOF;
      break;
    }
    default:
    {
      ECMA_TRY_CATCH (empty,
                      parse_re_iterator (parser_ctx_p->current_char_p,
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
    RE_ADVANCE (parser_ctx_p->current_char_p, advance);
  }

  return ret_value;
} /* re_parse_next_token */

#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */

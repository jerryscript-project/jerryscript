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

#include "jrt-libc-includes.h"
#include "re-parser.h"
#include "syntax-errors.h"

#define JERRY_LOOKUP(str_p, lookup)  *(str_p + lookup)

operand
parse_regexp_literal ()
{
  // FIXME: Impelement this!
  return operand ();
}

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

static uint32_t
parse_re_iterator (ecma_char_t *pattern_p,
                   re_token_t *re_token_p,
                   uint32_t lookup)
{
  uint32_t advance = 0;
  re_token_p->qmin = 1;
  re_token_p->qmax = 1;
  re_token_p->greedy = true;

  ecma_char_t ch0 = JERRY_LOOKUP (pattern_p, lookup);
  ecma_char_t ch1 = JERRY_LOOKUP (pattern_p, lookup + 1);

  switch (ch0)
  {
    case '?':
    {
      re_token_p->qmin = 0;
      re_token_p->qmax = 1;
      if (ch1 == '?')
      {
        advance = 2;
        re_token_p->greedy = false;
      }
      else
      {
        advance = 1;
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
        advance = 2;
        re_token_p->greedy = false;
      }
      else
      {
        advance = 1;
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
        advance = 2;
        re_token_p->greedy = false;
      }
      else
      {
        advance = 1;
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
        advance++;
        ch1 = JERRY_LOOKUP (pattern_p, lookup + advance);

        if (isdigit (ch1))
        {
          if (digits >= ECMA_NUMBER_MAX_DIGITS)
          {
            JERRY_ERROR_MSG ("RegExp quantifier error: too many digits.");
          }
          digits++;
          qmin = qmin * 10 + hex_to_int (ch1);
        }
        else if (ch1 == ',')
        {
          if (qmax != RE_ITERATOR_INFINITE)
          {
            JERRY_ERROR_MSG ("RegExp quantifier error: double comma.");
          }
          if ((JERRY_LOOKUP (pattern_p, lookup + advance + 1)) == '}')
          {
            if (digits == 0)
            {
              JERRY_ERROR_MSG ("RegExp quantifier error: missing digits.");
            }

            re_token_p->qmin = qmin;
            re_token_p->qmax = RE_ITERATOR_INFINITE;
            advance += 2;
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
            JERRY_ERROR_MSG ("RegExp quantifier error: missing digits.");
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

          advance += 1;
          break;
        }
        else
        {
          JERRY_ERROR_MSG ("RegExp quantifier error: unknown char.");
        }
      }

      if ((JERRY_LOOKUP (pattern_p, lookup + advance)) == '?')
      {
        re_token_p->greedy = false;
        advance += 1;
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

  if (re_token_p->qmin > re_token_p->qmax)
  {
    JERRY_ERROR_MSG ("RegExp quantifier error: qmin > qmax.");
  }

  return advance;
}

/**
 * Read the input pattern and parse the next token for the RegExp compiler
 */
re_token_t
re_parse_next_token (re_parser_ctx_t *parser_ctx_p)
{
  re_token_t result;
  uint32_t advance = 0;
  ecma_char_t ch0 = *(parser_ctx_p->current_char_p);

  switch (ch0)
  {
    case '|':
    {
      advance = 1;
      result.type = RE_TOK_ALTERNATIVE;
      break;
    }
    case '^':
    {
      advance = 1;
      result.type = RE_TOK_ASSERT_START;
      break;
    }
    case '$':
    {
      advance = 1;
      result.type = RE_TOK_ASSERT_END;
      break;
    }
    case '.':
    {
      advance = 1;
      advance += parse_re_iterator (parser_ctx_p->current_char_p, &result, 1);
      result.type = RE_TOK_PERIOD;
      break;
    }
    case '\\':
    {
      advance = 2;
      result.type = RE_TOK_CHAR;
      ecma_char_t ch1 = JERRY_LOOKUP (parser_ctx_p->current_char_p, 1);

      if (ch1 == 'b')
      {
        result.type = RE_TOK_ASSERT_WORD_BOUNDARY;
      }
      else if (ch1 == 'B')
      {
        result.type = RE_TOK_ASSERT_NOT_WORD_BOUNDARY;
      }
      else if (ch1 == 'f')
      {
        result.value = RE_CONTROL_CHAR_FF;
      }
      else if (ch1 == 'n')
      {
        result.value = RE_CONTROL_CHAR_EOL;
      }
      else if (ch1 == 't')
      {
        result.value = RE_CONTROL_CHAR_TAB;
      }
      else if (ch1 == 'r')
      {
        result.value = RE_CONTROL_CHAR_CR;
      }
      else if (ch1 == 'v')
      {
        result.value = RE_CONTROL_CHAR_VT;
      }
      else if (ch1 == 'c')
      {
        ecma_char_t ch2 = JERRY_LOOKUP (parser_ctx_p->current_char_p, 2);
        if ((ch2 >= 'A' && ch2 <= 'Z') || (ch2 >= 'a' && ch2 <= 'z'))
        {
          advance = 3;
          result.type = RE_TOK_CHAR;
          result.value = (ch2 % 32);
        }
        else
        {
          /* FIXME: invalid regexp control escape */
        }
      }
      else if (ch1 == 'x'
               && isxdigit (JERRY_LOOKUP (parser_ctx_p->current_char_p, 2))
               && isxdigit (JERRY_LOOKUP (parser_ctx_p->current_char_p, 3)))
      {
        advance = 4;
        result.type = RE_TOK_CHAR;
        /* FIXME: get unicode char from hex-digits */
        /* result.value = ...; */
      }
      else if (ch1 == 'u'
               && isxdigit (JERRY_LOOKUP (parser_ctx_p->current_char_p, 2))
               && isxdigit (JERRY_LOOKUP (parser_ctx_p->current_char_p, 3))
               && isxdigit (JERRY_LOOKUP (parser_ctx_p->current_char_p, 4))
               && isxdigit (JERRY_LOOKUP (parser_ctx_p->current_char_p, 5)))
      {
        advance = 4;
        result.type = RE_TOK_CHAR;
        /* FIXME: get unicode char from digits */
        /* result.value = ...; */
      }
      else if (ch1 == 'd')
      {
        advance = 2;
        result.type = RE_TOK_DIGIT;
      }
      else if (ch1 == 'D')
      {
        advance = 2;
        result.type = RE_TOK_NOT_DIGIT;
      }
      else if (ch1 == 's')
      {
        advance = 2;
        result.type = RE_TOK_WHITE;
      }
      else if (ch1 == 'S')
      {
        advance = 2;
        result.type = RE_TOK_NOT_WHITE;
      }
      else if (ch1 == 'w')
      {
        advance = 2;
        result.type = RE_TOK_WORD_CHAR;
      }
      else if (ch1 == 'W')
      {
        advance = 2;
        result.type = RE_TOK_NOT_WORD_CHAR;
      }
      else if (isdigit (ch1))
      {
        if (ch1 == '0')
        {
          if (isdigit (JERRY_LOOKUP (parser_ctx_p->current_char_p, 2)))
          {
            JERRY_ERROR_MSG ("RegExp escape pattern error.");
          }

          advance = 2;
          result.value = RE_CONTROL_CHAR_NULL;
        }
        else
        {
          /* FIXME: Unimplemented BACKREFERENCE */
        }
      }
      else
      {
        result.value = ch1;
      }

      advance += parse_re_iterator (parser_ctx_p->current_char_p, &result, advance);
      break;
    }
    case '(':
    {
      if (JERRY_LOOKUP (parser_ctx_p->current_char_p, 1) == '?')
      {
        ecma_char_t ch2 = JERRY_LOOKUP (parser_ctx_p->current_char_p, 2);
        if (ch2 == '=')
        {
          /* (?= */
          advance = 3;
          result.type = RE_TOK_ASSERT_START_POS_LOOKAHEAD;
        }
        else if (ch2 == '!')
        {
          /* (?! */
          advance = 3;
          result.type = RE_TOK_ASSERT_START_NEG_LOOKAHEAD;
        }
        else if (ch2 == ':')
        {
          /* (?: */
          advance = 3;
          result.type = RE_TOK_START_NON_CAPTURE_GROUP;
        }
      }
      else
      {
        /* ( */
        advance = 1;
        result.type = RE_TOK_START_CAPTURE_GROUP;
      }
      break;
    }
    case ')':
    {
      advance = 1;
      advance += parse_re_iterator (parser_ctx_p->current_char_p, &result, advance);
      result.type = RE_TOK_END_GROUP;
      break;
    }
    case '[':
    {
      advance = 1;
      result.type = RE_TOK_START_CHARCLASS;
      if (JERRY_LOOKUP (parser_ctx_p->current_char_p, 1) == '^')
      {
        advance = 2;
        result.type = RE_TOK_START_CHARCLASS_INVERTED;
      }
      break;
    }
    case ']':
    case '}':
    {
      JERRY_UNIMPLEMENTED ("Unimplemented RegExp token!");
      break;
    }
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
      result.type = RE_TOK_EOF;
      break;
    }
    default:
    {
      advance = 1;
      advance += parse_re_iterator (parser_ctx_p->current_char_p, &result, 1);
      result.type = RE_TOK_CHAR;
      result.value = ch0;
    }
  }

  (parser_ctx_p->current_char_p) += advance;

  return result;
} /* re_parse_next_token */

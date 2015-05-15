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
parse_re_iterator (ecma_char_t **pattern,
                   re_token_t *re_token,
                   uint32_t lookup)
{
  uint32_t advance = 0;
  re_token->qmin = 1;
  re_token->qmax = 1;
  re_token->greedy = true;

  ecma_char_t ch0 = JERRY_LOOKUP (*pattern, lookup);
  ecma_char_t ch1 = JERRY_LOOKUP (*pattern, lookup + 1);

  switch (ch0)
  {
    case '?':
    {
      re_token->qmin = 0;
      re_token->qmax = 1;
      if (ch1 == '?')
      {
        advance = 2;
        re_token->greedy = false;
      }
      else
      {
        advance = 1;
        re_token->greedy = true;
      }
      break;
    }
    case '*':
    {
      re_token->qmin = 0;
      re_token->qmax = RE_ITERATOR_INFINITE;
      if (ch1 == '?')
      {
        advance = 2;
        re_token->greedy = false;
      }
      else
      {
        advance = 1;
        re_token->greedy = true;
      }
      break;
    }
    case '+':
    {
      re_token->qmin = 1;
      re_token->qmax = RE_ITERATOR_INFINITE;
      if (ch1 == '?')
      {
        advance = 2;
        re_token->greedy = false;
      }
      else
      {
        advance = 1;
        re_token->greedy = true;
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
        ch1 = JERRY_LOOKUP (*pattern, lookup + advance);

        if (isdigit (ch1))
        {
          if (digits >= ECMA_NUMBER_MAX_DIGITS)
          {
            PARSE_ERROR ("RegExp quantifier error: too many digits.", 0 /* FIXME: locus needed */);
          }
          digits++;
          qmin = qmin * 10 + hex_to_int (ch1);
        }
        else if (ch1 == ',')
        {
          if (qmax != RE_ITERATOR_INFINITE)
          {
            PARSE_ERROR ("RegExp quantifier error: double comma.", 0 /* FIXME: locus needed */);
          }
          if ((JERRY_LOOKUP (*pattern, lookup + advance + 1)) == '}')
          {
            if (digits == 0)
            {
              PARSE_ERROR ("RegExp quantifier error: missing digits.", 0 /* FIXME: locus needed */);
            }

            re_token->qmin = qmin;
            re_token->qmax = RE_ITERATOR_INFINITE;
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
            PARSE_ERROR ("RegExp quantifier error: missing digits.", 0 /* FIXME: locus needed */);
          }

          if (qmax != RE_ITERATOR_INFINITE)
          {
            re_token->qmin = qmax;
            re_token->qmax = qmin;
          }
          else
          {
            re_token->qmin = qmin;
            re_token->qmax = qmin;
          }

          advance += 1;
          break;
        }
        else
        {
          PARSE_ERROR ("RegExp quantifier error: unknown char.", 0 /* FIXME: locus needed */);
        }
      }

      if ((JERRY_LOOKUP (*pattern, lookup + advance)) == '?')
      {
        re_token->greedy = false;
        advance += 1;
      }
      else
      {
        re_token->greedy = true;
      }
      break;

      JERRY_UNREACHABLE ();
      break;
    }
  }

  if (re_token->qmin > re_token->qmax)
  {
    PARSE_ERROR ("RegExp quantifier error: qmin > qmax.", 0 /* FIXME: locus needed */);
  }

  return advance;
}

/**
 * Read the input pattern and parse the next token for the RegExp compiler
 */
re_token_t
re_parse_next_token (ecma_char_t **pattern)
{
  re_token_t result;
  uint32_t advance = 0;
  ecma_char_t ch = **pattern;

  switch (ch)
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
      advance += parse_re_iterator (pattern, &result, 1);
      result.type = RE_TOK_ATOM_PERIOD;
      break;
    }
    case '(':
    {
      if (JERRY_LOOKUP (*pattern, 1) == '?')
      {
        if (JERRY_LOOKUP (*pattern, 2) == '=')
        {
          /* (?= */
          advance = 3;
          result.type = RE_TOK_ASSERT_START_POS_LOOKAHEAD;
        }
        else if (JERRY_LOOKUP (*pattern, 2) == '!')
        {
          /* (?! */
          advance = 3;
          result.type = RE_TOK_ASSERT_START_NEG_LOOKAHEAD;
        }
        else if (JERRY_LOOKUP (*pattern, 2) == ':')
        {
          /* (?: */
          advance = 3;
          result.type = RE_TOK_ATOM_START_NONCAPTURE_GROUP;
        }
      }
      else
      {
        /* ( */
        advance = 1;
        result.type = RE_TOK_ATOM_START_CAPTURE_GROUP;
      }
      break;
    }
    case ')':
    {
      advance = 1;
      result.type = RE_TOK_ATOM_START_CAPTURE_GROUP;
      break;
    }
    case '[':
    case ']':
    case '}':
    case '\\':
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
    default:
    {
      advance = 1;
      advance += parse_re_iterator (pattern, &result, 1);
      result.type = RE_TOK_CHAR;
      result.value = ch;
    }
  }

  JERRY_ASSERT (advance > 0);
  (*pattern) += advance;

  return result;
} /* re_parse_next_token */

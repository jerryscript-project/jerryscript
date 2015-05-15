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

#include "re-parser.h"

#define JERRY_LOOKUP(str_p, lookup)  *(str_p + lookup)

operand
parse_regexp_literal ()
{
  // FIXME: Impelement this!
  return operand ();
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
    case '*':
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
    case '+':
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
    case '{':
    {
      JERRY_UNREACHABLE ();
      break;
    }
  }

  /* FIXME: Throw a syntax error if (qmin > qmax) */

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
      advance = 1;
      result.type = RE_TOK_ALTERNATIVE;
      break;
    case '^':
      advance = 1;
      result.type = RE_TOK_ASSERT_START;
      break;
    case '$':
      advance = 1;
      result.type = RE_TOK_ASSERT_END;
      break;
    case '.':
      advance = 1;
      advance += parse_re_iterator (pattern, &result, 1);
      result.type = RE_TOK_ATOM_PERIOD;
      break;
    case '(':
      if (JERRY_LOOKUP (*pattern, 1) == '?') {
          if (JERRY_LOOKUP (*pattern, 2) == '=') {
            /* (?= */
            advance = 3;
            result.type = RE_TOK_ASSERT_START_POS_LOOKAHEAD;
          } else if (JERRY_LOOKUP (*pattern, 2) == '!') {
            /* (?! */
            advance = 3;
            result.type = RE_TOK_ASSERT_START_NEG_LOOKAHEAD;
          } else if (JERRY_LOOKUP (*pattern, 2) == ':') {
            /* (?: */
            advance = 3;
            result.type = RE_TOK_ATOM_START_NONCAPTURE_GROUP;
          }
        } else {
          /* ( */
          advance = 1;
          result.type = RE_TOK_ATOM_START_CAPTURE_GROUP;
        }
        break;
    case ')':
      advance = 1;
      result.type = RE_TOK_ATOM_START_CAPTURE_GROUP;
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
      JERRY_UNREACHABLE ();
      break;
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

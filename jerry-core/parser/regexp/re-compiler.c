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
#include "ecma-helpers.h"
#include "ecma-regexp-object.h"
#include "ecma-try-catch-macro.h"
#include "lit-char-helpers.h"
#include "jcontext.h"
#include "jrt-libc-includes.h"
#include "jmem.h"
#include "re-bytecode.h"
#include "re-compiler.h"
#include "re-parser.h"

#if ENABLED (JERRY_BUILTIN_REGEXP)

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup regexparser Regular expression
 * @{
 *
 * \addtogroup regexparser_compiler Compiler
 * @{
 */

/**
 * Insert simple atom iterator
 *
 * @return empty ecma value - if inserted successfully
 *         error ecma value - otherwise
 *
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
re_insert_simple_iterator (re_compiler_ctx_t *re_ctx_p, /**< RegExp compiler context */
                           uint32_t new_atom_start_offset) /**< atom start offset */
{
  uint32_t atom_code_length;
  uint32_t offset;
  uint32_t qmin, qmax;

  qmin = re_ctx_p->current_token.qmin;
  qmax = re_ctx_p->current_token.qmax;

  if (qmin == 1 && qmax == 1)
  {
    return ECMA_VALUE_EMPTY;
  }
  else if (qmin > qmax)
  {
    /* ECMA-262 v5.1 15.10.2.5 */
    return ecma_raise_syntax_error (ECMA_ERR_MSG ("RegExp quantifier error: min > max."));
  }

  /* TODO: optimize bytecode length. Store 0 rather than INF */

  re_append_opcode (re_ctx_p->bytecode_ctx_p, RE_OP_MATCH);   /* complete 'sub atom' */
  uint32_t bytecode_length = re_get_bytecode_length (re_ctx_p->bytecode_ctx_p);
  atom_code_length = (uint32_t) (bytecode_length - new_atom_start_offset);

  offset = new_atom_start_offset;
  re_insert_u32 (re_ctx_p->bytecode_ctx_p, offset, atom_code_length);
  re_insert_u32 (re_ctx_p->bytecode_ctx_p, offset, qmax);
  re_insert_u32 (re_ctx_p->bytecode_ctx_p, offset, qmin);
  if (re_ctx_p->current_token.greedy)
  {
    re_insert_opcode (re_ctx_p->bytecode_ctx_p, offset, RE_OP_GREEDY_ITERATOR);
  }
  else
  {
    re_insert_opcode (re_ctx_p->bytecode_ctx_p, offset, RE_OP_NON_GREEDY_ITERATOR);
  }

  return ECMA_VALUE_EMPTY;
} /* re_insert_simple_iterator */

/**
 * Get the type of a group start
 *
 * @return RegExp opcode
 */
static re_opcode_t
re_get_start_opcode_type (re_compiler_ctx_t *re_ctx_p, /**< RegExp compiler context */
                          bool is_capturable) /**< is capturable group */
{
  if (is_capturable)
  {
    if (re_ctx_p->current_token.qmin == 0)
    {
      if (re_ctx_p->current_token.greedy)
      {
        return RE_OP_CAPTURE_GREEDY_ZERO_GROUP_START;
      }

      return RE_OP_CAPTURE_NON_GREEDY_ZERO_GROUP_START;
    }

    return RE_OP_CAPTURE_GROUP_START;
  }

  if (re_ctx_p->current_token.qmin == 0)
  {
    if (re_ctx_p->current_token.greedy)
    {
      return RE_OP_NON_CAPTURE_GREEDY_ZERO_GROUP_START;
    }

    return RE_OP_NON_CAPTURE_NON_GREEDY_ZERO_GROUP_START;
  }

  return RE_OP_NON_CAPTURE_GROUP_START;
} /* re_get_start_opcode_type */

/**
 * Get the type of a group end
 *
 * @return RegExp opcode
 */
static re_opcode_t
re_get_end_opcode_type (re_compiler_ctx_t *re_ctx_p, /**< RegExp compiler context */
                        bool is_capturable) /**< is capturable group */
{
  if (is_capturable)
  {
    if (re_ctx_p->current_token.greedy)
    {
      return RE_OP_CAPTURE_GREEDY_GROUP_END;
    }

    return RE_OP_CAPTURE_NON_GREEDY_GROUP_END;
  }

  if (re_ctx_p->current_token.greedy)
  {
    return RE_OP_NON_CAPTURE_GREEDY_GROUP_END;
  }

  return RE_OP_NON_CAPTURE_NON_GREEDY_GROUP_END;
} /* re_get_end_opcode_type */

/**
 * Enclose the given bytecode to a group
 *
 * @return empty ecma value - if inserted successfully
 *         error ecma value - otherwise
 *
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
re_insert_into_group (re_compiler_ctx_t *re_ctx_p, /**< RegExp compiler context */
                      uint32_t group_start_offset, /**< offset of group start */
                      uint32_t idx, /**< index of group */
                      bool is_capturable) /**< is capturable group */
{
  uint32_t qmin = re_ctx_p->current_token.qmin;
  uint32_t qmax = re_ctx_p->current_token.qmax;

  if (qmin > qmax)
  {
    /* ECMA-262 v5.1 15.10.2.5 */
    return ecma_raise_syntax_error (ECMA_ERR_MSG ("RegExp quantifier error: min > max."));
  }

  re_opcode_t start_opcode = re_get_start_opcode_type (re_ctx_p, is_capturable);
  re_opcode_t end_opcode = re_get_end_opcode_type (re_ctx_p, is_capturable);

  uint32_t start_head_offset_len = re_get_bytecode_length (re_ctx_p->bytecode_ctx_p);
  re_insert_u32 (re_ctx_p->bytecode_ctx_p, group_start_offset, idx);
  re_insert_opcode (re_ctx_p->bytecode_ctx_p, group_start_offset, start_opcode);
  start_head_offset_len = re_get_bytecode_length (re_ctx_p->bytecode_ctx_p) - start_head_offset_len;
  re_append_opcode (re_ctx_p->bytecode_ctx_p, end_opcode);
  re_append_u32 (re_ctx_p->bytecode_ctx_p, idx);
  re_append_u32 (re_ctx_p->bytecode_ctx_p, qmin);
  re_append_u32 (re_ctx_p->bytecode_ctx_p, qmax);

  group_start_offset += start_head_offset_len;
  re_append_jump_offset (re_ctx_p->bytecode_ctx_p,
                         re_get_bytecode_length (re_ctx_p->bytecode_ctx_p) - group_start_offset);

  if (start_opcode != RE_OP_CAPTURE_GROUP_START && start_opcode != RE_OP_NON_CAPTURE_GROUP_START)
  {
    re_insert_u32 (re_ctx_p->bytecode_ctx_p,
                   group_start_offset,
                   re_get_bytecode_length (re_ctx_p->bytecode_ctx_p) - group_start_offset);
  }

  return ECMA_VALUE_EMPTY;
} /* re_insert_into_group */

/**
 * Enclose the given bytecode to a group and inster jump value
 *
 * @return empty ecma value - if inserted successfully
 *         error ecma value - otherwise
 *
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
re_insert_into_group_with_jump (re_compiler_ctx_t *re_ctx_p, /**< RegExp compiler context */
                                uint32_t group_start_offset, /**< offset of group start */
                                uint32_t idx, /**< index of group */
                                bool is_capturable) /**< is capturable group */
{
  re_insert_u32 (re_ctx_p->bytecode_ctx_p,
                 group_start_offset,
                 re_get_bytecode_length (re_ctx_p->bytecode_ctx_p) - group_start_offset);
  return re_insert_into_group (re_ctx_p, group_start_offset, idx, is_capturable);
} /* re_insert_into_group_with_jump */

/**
 * Append a character class range to the bytecode
 */
static void
re_append_char_class (re_compiler_ctx_t *re_ctx_p, /**< RegExp compiler context */
                      ecma_char_t start, /**< character class range from */
                      ecma_char_t end) /**< character class range to */
{
  re_append_char (re_ctx_p->bytecode_ctx_p, ecma_regexp_canonicalize (start, re_ctx_p->flags & RE_FLAG_IGNORE_CASE));
  re_append_char (re_ctx_p->bytecode_ctx_p, ecma_regexp_canonicalize (end, re_ctx_p->flags & RE_FLAG_IGNORE_CASE));
  re_ctx_p->parser_ctx_p->classes_count++;
} /* re_append_char_class */

/**
 * Read the input pattern and parse the range of character class
 *
 * @return empty ecma value - if parsed successfully
 *         error ecma value - otherwise
 *
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
re_parse_char_class (re_compiler_ctx_t *re_ctx_p, /**< number of classes */
                     re_token_t *out_token_p) /**< [out] output token */
{
  re_parser_ctx_t *const parser_ctx_p = re_ctx_p->parser_ctx_p;
  out_token_p->qmax = out_token_p->qmin = 1;
  parser_ctx_p->classes_count = 0;

  ecma_char_t start = LIT_CHAR_UNDEF;
  bool is_range = false;
  const bool is_char_class = (re_ctx_p->current_token.type == RE_TOK_START_CHAR_CLASS
                              || re_ctx_p->current_token.type == RE_TOK_START_INV_CHAR_CLASS);

  const ecma_char_t prev_char = lit_utf8_peek_prev (parser_ctx_p->input_curr_p);
  if (prev_char != LIT_CHAR_LEFT_SQUARE && prev_char != LIT_CHAR_CIRCUMFLEX)
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
        re_append_char_class (re_ctx_p, start, start);
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
      else if (ch == LIT_CHAR_LOWERCASE_X && re_hex_lookup (parser_ctx_p, 2))
      {
        ecma_char_t code_unit;

        if (!lit_read_code_unit_from_hex (parser_ctx_p->input_curr_p, 2, &code_unit))
        {
          return ecma_raise_syntax_error (ECMA_ERR_MSG ("invalid character class, end of string after '\\x'"));
        }

        parser_ctx_p->input_curr_p += 2;
        if (parser_ctx_p->input_curr_p < parser_ctx_p->input_end_p
            && is_range == false
            && lit_utf8_peek_next (parser_ctx_p->input_curr_p) == LIT_CHAR_MINUS)
        {
          start = code_unit;
          continue;
        }

        ch = code_unit;
      }
      else if (ch == LIT_CHAR_LOWERCASE_U && re_hex_lookup (parser_ctx_p, 4))
      {
        ecma_char_t code_unit;

        if (!lit_read_code_unit_from_hex (parser_ctx_p->input_curr_p, 4, &code_unit))
        {
          return ecma_raise_syntax_error (ECMA_ERR_MSG ("invalid character class, end of string after '\\u'"));
        }

        parser_ctx_p->input_curr_p += 4;
        if (parser_ctx_p->input_curr_p < parser_ctx_p->input_end_p
            && is_range == false
            && lit_utf8_peek_next (parser_ctx_p->input_curr_p) == LIT_CHAR_MINUS)
        {
          start = code_unit;
          continue;
        }

        ch = code_unit;
      }
      else if (ch == LIT_CHAR_LOWERCASE_D)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        re_append_char_class (re_ctx_p, LIT_CHAR_ASCII_DIGITS_BEGIN, LIT_CHAR_ASCII_DIGITS_END);
        ch = LIT_CHAR_UNDEF;
      }
      else if (ch == LIT_CHAR_UPPERCASE_D)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        re_append_char_class (re_ctx_p, LIT_CHAR_NULL, LIT_CHAR_ASCII_DIGITS_BEGIN - 1);
        re_append_char_class (re_ctx_p, LIT_CHAR_ASCII_DIGITS_END + 1, LIT_UTF16_CODE_UNIT_MAX);
        ch = LIT_CHAR_UNDEF;
      }
      else if (ch == LIT_CHAR_LOWERCASE_S)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        re_append_char_class (re_ctx_p, LIT_CHAR_TAB, LIT_CHAR_CR);
        re_append_char_class (re_ctx_p, LIT_CHAR_SP, LIT_CHAR_SP);
        re_append_char_class (re_ctx_p, LIT_CHAR_NBSP, LIT_CHAR_NBSP);
        re_append_char_class (re_ctx_p, 0x1680UL, 0x1680UL); /* Ogham Space Mark */
        re_append_char_class (re_ctx_p, 0x180EUL, 0x180EUL); /* Mongolian Vowel Separator */
        re_append_char_class (re_ctx_p, 0x2000UL, 0x200AUL); /* En Quad - Hair Space */
        re_append_char_class (re_ctx_p, LIT_CHAR_LS, LIT_CHAR_PS);
        re_append_char_class (re_ctx_p, 0x202FUL, 0x202FUL); /* Narrow No-Break Space */
        re_append_char_class (re_ctx_p, 0x205FUL, 0x205FUL); /* Medium Mathematical Space */
        re_append_char_class (re_ctx_p, 0x3000UL, 0x3000UL); /* Ideographic Space */
        re_append_char_class (re_ctx_p, LIT_CHAR_BOM, LIT_CHAR_BOM);
        ch = LIT_CHAR_UNDEF;
      }
      else if (ch == LIT_CHAR_UPPERCASE_S)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        re_append_char_class (re_ctx_p, LIT_CHAR_NULL, LIT_CHAR_TAB - 1);
        re_append_char_class (re_ctx_p, LIT_CHAR_CR + 1, LIT_CHAR_SP - 1);
        re_append_char_class (re_ctx_p, LIT_CHAR_SP + 1, LIT_CHAR_NBSP - 1);
        re_append_char_class (re_ctx_p, LIT_CHAR_NBSP + 1, 0x167FUL);
        re_append_char_class (re_ctx_p, 0x1681UL, 0x180DUL);
        re_append_char_class (re_ctx_p, 0x180FUL, 0x1FFFUL);
        re_append_char_class (re_ctx_p, 0x200BUL, LIT_CHAR_LS - 1);
        re_append_char_class (re_ctx_p, LIT_CHAR_PS + 1, 0x202EUL);
        re_append_char_class (re_ctx_p, 0x2030UL, 0x205EUL);
        re_append_char_class (re_ctx_p, 0x2060UL, 0x2FFFUL);
        re_append_char_class (re_ctx_p, 0x3001UL, LIT_CHAR_BOM - 1);
        re_append_char_class (re_ctx_p, LIT_CHAR_BOM + 1, LIT_UTF16_CODE_UNIT_MAX);
        ch = LIT_CHAR_UNDEF;
      }
      else if (ch == LIT_CHAR_LOWERCASE_W)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        re_append_char_class (re_ctx_p, LIT_CHAR_0, LIT_CHAR_9);
        re_append_char_class (re_ctx_p, LIT_CHAR_UPPERCASE_A, LIT_CHAR_UPPERCASE_Z);
        re_append_char_class (re_ctx_p, LIT_CHAR_UNDERSCORE, LIT_CHAR_UNDERSCORE);
        re_append_char_class (re_ctx_p, LIT_CHAR_LOWERCASE_A, LIT_CHAR_LOWERCASE_Z);
        ch = LIT_CHAR_UNDEF;
      }
      else if (ch == LIT_CHAR_UPPERCASE_W)
      {
        /* See ECMA-262 v5, 15.10.2.12 */
        re_append_char_class (re_ctx_p, LIT_CHAR_NULL, LIT_CHAR_0 - 1);
        re_append_char_class (re_ctx_p, LIT_CHAR_9 + 1, LIT_CHAR_UPPERCASE_A - 1);
        re_append_char_class (re_ctx_p, LIT_CHAR_UPPERCASE_Z + 1, LIT_CHAR_UNDERSCORE - 1);
        re_append_char_class (re_ctx_p, LIT_CHAR_UNDERSCORE + 1, LIT_CHAR_LOWERCASE_A - 1);
        re_append_char_class (re_ctx_p, LIT_CHAR_LOWERCASE_Z + 1, LIT_UTF16_CODE_UNIT_MAX);
        ch = LIT_CHAR_UNDEF;
      }
      else if (lit_char_is_octal_digit ((ecma_char_t) ch))
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
          re_append_char_class (re_ctx_p, start, ch);
          start = LIT_CHAR_UNDEF;
          is_range = false;
        }
      }
      else
      {
        re_append_char_class (re_ctx_p, start, start);
        start = ch;
      }
    }
    else
    {
      start = ch;
    }
  }
  while (is_char_class);

  return re_parse_iterator (parser_ctx_p, out_token_p);
} /* re_parse_char_class */

/**
 * Parse alternatives
 *
 * @return empty ecma value - if alternative was successfully parsed
 *         error ecma value - otherwise
 *
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
re_parse_alternative (re_compiler_ctx_t *re_ctx_p, /**< RegExp compiler context */
                      bool expect_eof) /**< expect end of file */
{
  ECMA_CHECK_STACK_USAGE ();
  uint32_t idx;
  re_bytecode_ctx_t *bc_ctx_p = re_ctx_p->bytecode_ctx_p;
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  uint32_t alternative_offset = re_get_bytecode_length (re_ctx_p->bytecode_ctx_p);

  while (ecma_is_value_empty (ret_value))
  {
    ecma_value_t next_token_result = re_parse_next_token (re_ctx_p->parser_ctx_p,
                                                          &(re_ctx_p->current_token));
    if (ECMA_IS_VALUE_ERROR (next_token_result))
    {
      return next_token_result;
    }

    JERRY_ASSERT (ecma_is_value_empty (next_token_result));

    uint32_t new_atom_start_offset = re_get_bytecode_length (re_ctx_p->bytecode_ctx_p);

    switch (re_ctx_p->current_token.type)
    {
      case RE_TOK_START_CAPTURE_GROUP:
      {
        idx = re_ctx_p->captures_count++;
        JERRY_TRACE_MSG ("Compile a capture group start (idx: %u)\n", (unsigned int) idx);

        ret_value = re_parse_alternative (re_ctx_p, false);

        if (ecma_is_value_empty (ret_value))
        {
          ret_value = re_insert_into_group (re_ctx_p, new_atom_start_offset, idx, true);
        }

        break;
      }
      case RE_TOK_START_NON_CAPTURE_GROUP:
      {
        idx = re_ctx_p->non_captures_count++;
        JERRY_TRACE_MSG ("Compile a non-capture group start (idx: %u)\n", (unsigned int) idx);

        ret_value = re_parse_alternative (re_ctx_p, false);

        if (ecma_is_value_empty (ret_value))
        {
          ret_value = re_insert_into_group (re_ctx_p, new_atom_start_offset, idx, false);
        }

        break;
      }
      case RE_TOK_CHAR:
      {
        JERRY_TRACE_MSG ("Compile character token: %c, qmin: %u, qmax: %u\n",
                         (char) re_ctx_p->current_token.value, (unsigned int) re_ctx_p->current_token.qmin,
                         (unsigned int) re_ctx_p->current_token.qmax);

        re_append_opcode (bc_ctx_p, RE_OP_CHAR);
        re_append_char (bc_ctx_p, ecma_regexp_canonicalize ((ecma_char_t) re_ctx_p->current_token.value,
                                                            re_ctx_p->flags & RE_FLAG_IGNORE_CASE));

        ret_value = re_insert_simple_iterator (re_ctx_p, new_atom_start_offset);
        break;
      }
      case RE_TOK_PERIOD:
      {
        JERRY_TRACE_MSG ("Compile a period\n");
        re_append_opcode (bc_ctx_p, RE_OP_PERIOD);

        ret_value = re_insert_simple_iterator (re_ctx_p, new_atom_start_offset);
        break;
      }
      case RE_TOK_ALTERNATIVE:
      {
        JERRY_TRACE_MSG ("Compile an alternative\n");
        re_insert_u32 (bc_ctx_p, alternative_offset, re_get_bytecode_length (bc_ctx_p) - alternative_offset);
        re_append_opcode (bc_ctx_p, RE_OP_ALTERNATIVE);
        alternative_offset = re_get_bytecode_length (re_ctx_p->bytecode_ctx_p);
        break;
      }
      case RE_TOK_ASSERT_START:
      {
        JERRY_TRACE_MSG ("Compile a start assertion\n");
        re_append_opcode (bc_ctx_p, RE_OP_ASSERT_START);
        break;
      }
      case RE_TOK_ASSERT_END:
      {
        JERRY_TRACE_MSG ("Compile an end assertion\n");
        re_append_opcode (bc_ctx_p, RE_OP_ASSERT_END);
        break;
      }
      case RE_TOK_ASSERT_WORD_BOUNDARY:
      {
        JERRY_TRACE_MSG ("Compile a word boundary assertion\n");
        re_append_opcode (bc_ctx_p, RE_OP_ASSERT_WORD_BOUNDARY);
        break;
      }
      case RE_TOK_ASSERT_NOT_WORD_BOUNDARY:
      {
        JERRY_TRACE_MSG ("Compile a not word boundary assertion\n");
        re_append_opcode (bc_ctx_p, RE_OP_ASSERT_NOT_WORD_BOUNDARY);
        break;
      }
      case RE_TOK_ASSERT_START_POS_LOOKAHEAD:
      {
        JERRY_TRACE_MSG ("Compile a positive lookahead assertion\n");
        idx = re_ctx_p->non_captures_count++;
        re_append_opcode (bc_ctx_p, RE_OP_LOOKAHEAD_POS);

        ret_value = re_parse_alternative (re_ctx_p, false);

        if (ecma_is_value_empty (ret_value))
        {
          re_append_opcode (bc_ctx_p, RE_OP_MATCH);

          ret_value = re_insert_into_group_with_jump (re_ctx_p, new_atom_start_offset, idx, false);
        }

        break;
      }
      case RE_TOK_ASSERT_START_NEG_LOOKAHEAD:
      {
        JERRY_TRACE_MSG ("Compile a negative lookahead assertion\n");
        idx = re_ctx_p->non_captures_count++;
        re_append_opcode (bc_ctx_p, RE_OP_LOOKAHEAD_NEG);

        ret_value = re_parse_alternative (re_ctx_p, false);

        if (ecma_is_value_empty (ret_value))
        {
          re_append_opcode (bc_ctx_p, RE_OP_MATCH);

          ret_value = re_insert_into_group_with_jump (re_ctx_p, new_atom_start_offset, idx, false);
        }

        break;
      }
      case RE_TOK_BACKREFERENCE:
      {
        uint32_t backref = (uint32_t) re_ctx_p->current_token.value;
        idx = re_ctx_p->non_captures_count++;

        if (backref > re_ctx_p->highest_backref)
        {
          re_ctx_p->highest_backref = backref;
        }

        JERRY_TRACE_MSG ("Compile a backreference: %u\n", (unsigned int) backref);
        re_append_opcode (bc_ctx_p, RE_OP_BACKREFERENCE);
        re_append_u32 (bc_ctx_p, backref);

        ret_value = re_insert_into_group_with_jump (re_ctx_p, new_atom_start_offset, idx, false);
        break;
      }
      case RE_TOK_DIGIT:
      case RE_TOK_NOT_DIGIT:
      case RE_TOK_WHITE:
      case RE_TOK_NOT_WHITE:
      case RE_TOK_WORD_CHAR:
      case RE_TOK_NOT_WORD_CHAR:
      case RE_TOK_START_CHAR_CLASS:
      case RE_TOK_START_INV_CHAR_CLASS:
      {
        JERRY_TRACE_MSG ("Compile a character class\n");
        re_append_opcode (bc_ctx_p,
                          re_ctx_p->current_token.type == RE_TOK_START_INV_CHAR_CLASS
                                                       ? RE_OP_INV_CHAR_CLASS
                                                       : RE_OP_CHAR_CLASS);
        uint32_t offset = re_get_bytecode_length (re_ctx_p->bytecode_ctx_p);

        ret_value = re_parse_char_class (re_ctx_p,
                                         &(re_ctx_p->current_token));

        if (!ECMA_IS_VALUE_ERROR (ret_value))
        {
          re_insert_u32 (bc_ctx_p, offset, re_ctx_p->parser_ctx_p->classes_count);
          ret_value = re_insert_simple_iterator (re_ctx_p, new_atom_start_offset);
        }

        break;
      }
      case RE_TOK_END_GROUP:
      {
        JERRY_TRACE_MSG ("Compile a group end\n");

        if (expect_eof)
        {
          return ecma_raise_syntax_error (ECMA_ERR_MSG ("Unexpected end of paren."));
        }

        re_insert_u32 (bc_ctx_p, alternative_offset, re_get_bytecode_length (bc_ctx_p) - alternative_offset);
        return ECMA_VALUE_EMPTY;
      }
      case RE_TOK_EOF:
      {
        if (!expect_eof)
        {
          return ecma_raise_syntax_error (ECMA_ERR_MSG ("Unexpected end of pattern."));
        }

        re_insert_u32 (bc_ctx_p, alternative_offset, re_get_bytecode_length (bc_ctx_p) - alternative_offset);
        return ECMA_VALUE_EMPTY;
      }
      default:
      {
        return ecma_raise_syntax_error (ECMA_ERR_MSG ("Unexpected RegExp token."));
      }
    }
  }

  return ret_value;
} /* re_parse_alternative */

/**
 * Search for the given pattern in the RegExp cache
 *
 * @return index of bytecode in cache - if found
 *         RE_CACHE_SIZE              - otherwise
 */
static uint8_t
re_find_bytecode_in_cache (ecma_string_t *pattern_str_p, /**< pattern string */
                           uint16_t flags) /**< flags */
{
  uint8_t free_idx = RE_CACHE_SIZE;

  for (uint8_t idx = 0u; idx < RE_CACHE_SIZE; idx++)
  {
    const re_compiled_code_t *cached_bytecode_p = JERRY_CONTEXT (re_cache)[idx];

    if (cached_bytecode_p != NULL)
    {
      ecma_string_t *cached_pattern_str_p = ecma_get_string_from_value (cached_bytecode_p->source);

      if ((cached_bytecode_p->header.status_flags & RE_FLAGS_MASK) == flags
          && ecma_compare_ecma_strings (cached_pattern_str_p, pattern_str_p))
      {
        JERRY_TRACE_MSG ("RegExp is found in cache\n");
        return idx;
      }
    }
    else
    {
      /* mark as free, so it can be overridden if the cache is full */
      free_idx = idx;
    }
  }

  JERRY_TRACE_MSG ("RegExp is NOT found in cache\n");
  return free_idx;
} /* re_find_bytecode_in_cache */

/**
 * Run gerbage collection in RegExp cache
 */
void
re_cache_gc_run (void)
{
  for (uint32_t i = 0u; i < RE_CACHE_SIZE; i++)
  {
    const re_compiled_code_t *cached_bytecode_p = JERRY_CONTEXT (re_cache)[i];

    if (cached_bytecode_p != NULL
        && cached_bytecode_p->header.refs == 1)
    {
      /* Only the cache has reference for the bytecode */
      ecma_bytecode_deref ((ecma_compiled_code_t *) cached_bytecode_p);
      JERRY_CONTEXT (re_cache)[i] = NULL;
    }
  }
} /* re_cache_gc_run */

/**
 * Compilation of RegExp bytecode
 *
 * @return empty ecma value - if bytecode was compiled successfully
 *         error ecma value - otherwise
 *
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
re_compile_bytecode (const re_compiled_code_t **out_bytecode_p, /**< [out] pointer to bytecode */
                     ecma_string_t *pattern_str_p, /**< pattern */
                     uint16_t flags) /**< flags */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;
  uint8_t cache_idx = re_find_bytecode_in_cache (pattern_str_p, flags);

  if (cache_idx < RE_CACHE_SIZE)
  {
    *out_bytecode_p = JERRY_CONTEXT (re_cache)[cache_idx];

    if (*out_bytecode_p != NULL)
    {
      ecma_bytecode_ref ((ecma_compiled_code_t *) *out_bytecode_p);
      return ret_value;
    }
  }

  /* not in the RegExp cache, so compile it */
  re_compiler_ctx_t re_ctx;
  re_ctx.flags = flags;
  re_ctx.highest_backref = 0;
  re_ctx.non_captures_count = 0;

  re_bytecode_ctx_t bc_ctx;
  re_ctx.bytecode_ctx_p = &bc_ctx;
  re_initialize_regexp_bytecode (&bc_ctx);

  ECMA_STRING_TO_UTF8_STRING (pattern_str_p, pattern_start_p, pattern_start_size);

  re_parser_ctx_t parser_ctx;
  parser_ctx.input_start_p = pattern_start_p;
  parser_ctx.input_curr_p = (lit_utf8_byte_t *) pattern_start_p;
  parser_ctx.input_end_p = pattern_start_p + pattern_start_size;
  parser_ctx.groups_count = -1;
  re_ctx.parser_ctx_p = &parser_ctx;

  /* Parse RegExp pattern */
  re_ctx.captures_count = 1;
  re_append_opcode (&bc_ctx, RE_OP_SAVE_AT_START);

  ecma_value_t result = re_parse_alternative (&re_ctx, true);

  ECMA_FINALIZE_UTF8_STRING (pattern_start_p, pattern_start_size);

  if (ECMA_IS_VALUE_ERROR (result))
  {
    ret_value = result;
  }
  /* Check for invalid backreference */
  else if (re_ctx.highest_backref >= re_ctx.captures_count)
  {
    ret_value = ecma_raise_syntax_error ("Invalid backreference.\n");
  }
  else
  {
    re_append_opcode (&bc_ctx, RE_OP_SAVE_AND_MATCH);
    re_append_opcode (&bc_ctx, RE_OP_EOF);

    /* Initialize bytecode header */
    re_compiled_code_t *re_compiled_code_p = (re_compiled_code_t *) bc_ctx.block_start_p;
    re_compiled_code_p->header.refs = 1;
    re_compiled_code_p->header.status_flags = re_ctx.flags;
    ecma_ref_ecma_string (pattern_str_p);
    re_compiled_code_p->source = ecma_make_string_value (pattern_str_p);
    re_compiled_code_p->captures_count = re_ctx.captures_count;
    re_compiled_code_p->non_captures_count = re_ctx.non_captures_count;
  }

  size_t byte_code_size = (size_t) (bc_ctx.block_end_p - bc_ctx.block_start_p);

  if (!ecma_is_value_empty (ret_value))
  {
    /* Compilation failed, free bytecode. */
    JERRY_TRACE_MSG ("RegExp compilation failed!\n");
    jmem_heap_free_block (bc_ctx.block_start_p, byte_code_size);
    *out_bytecode_p = NULL;
  }
  else
  {
#if ENABLED (JERRY_REGEXP_DUMP_BYTE_CODE)
    if (JERRY_CONTEXT (jerry_init_flags) & ECMA_INIT_SHOW_REGEXP_OPCODES)
    {
      re_dump_bytecode (&bc_ctx);
    }
#endif /* ENABLED (JERRY_REGEXP_DUMP_BYTE_CODE) */

    *out_bytecode_p = (re_compiled_code_t *) bc_ctx.block_start_p;
    ((re_compiled_code_t *) bc_ctx.block_start_p)->header.size = (uint16_t) (byte_code_size >> JMEM_ALIGNMENT_LOG);

    if (cache_idx == RE_CACHE_SIZE)
    {
      if (JERRY_CONTEXT (re_cache_idx) == RE_CACHE_SIZE)
      {
        JERRY_CONTEXT (re_cache_idx) = 0;
      }

      JERRY_TRACE_MSG ("RegExp cache is full! Remove the element on idx: %d\n", JERRY_CONTEXT (re_cache_idx));

      cache_idx = JERRY_CONTEXT (re_cache_idx)++;

      /* The garbage collector might run during the byte code
       * allocations above and it may free this entry. */
      if (JERRY_CONTEXT (re_cache)[cache_idx] != NULL)
      {
        ecma_bytecode_deref ((ecma_compiled_code_t *) JERRY_CONTEXT (re_cache)[cache_idx]);
      }
    }

    JERRY_TRACE_MSG ("Insert bytecode into RegExp cache (idx: %d).\n", cache_idx);
    ecma_bytecode_ref ((ecma_compiled_code_t *) *out_bytecode_p);
    JERRY_CONTEXT (re_cache)[cache_idx] = *out_bytecode_p;
  }

  return ret_value;
} /* re_compile_bytecode */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */

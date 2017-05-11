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
#include "jcontext.h"
#include "jrt-libc-includes.h"
#include "jmem.h"
#include "re-bytecode.h"
#include "re-compiler.h"
#include "re-parser.h"

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN

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
 * Callback function of character class generation
 */
static void
re_append_char_class (void *re_ctx_p, /**< RegExp compiler context */
                      ecma_char_t start, /**< character class range from */
                      ecma_char_t end) /**< character class range to */
{
  re_compiler_ctx_t *ctx_p = (re_compiler_ctx_t *) re_ctx_p;
  re_append_char (ctx_p->bytecode_ctx_p, start);
  re_append_char (ctx_p->bytecode_ctx_p, end);
  ctx_p->parser_ctx_p->num_of_classes++;
} /* re_append_char_class */

/**
 * Insert simple atom iterator
 */
static void
re_insert_simple_iterator (re_compiler_ctx_t *re_ctx_p, /**< RegExp compiler context */
                           uint32_t new_atom_start_offset) /**< atom start offset */
{
  uint32_t atom_code_length;
  uint32_t offset;
  uint32_t qmin, qmax;

  qmin = re_ctx_p->current_token.qmin;
  qmax = re_ctx_p->current_token.qmax;
  JERRY_ASSERT (qmin <= qmax);

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
 */
static void
re_insert_into_group (re_compiler_ctx_t *re_ctx_p, /**< RegExp compiler context */
                      uint32_t group_start_offset, /**< offset of group start */
                      uint32_t idx, /**< index of group */
                      bool is_capturable) /**< is capturable group */
{
  uint32_t qmin, qmax;
  re_opcode_t start_opcode = re_get_start_opcode_type (re_ctx_p, is_capturable);
  re_opcode_t end_opcode = re_get_end_opcode_type (re_ctx_p, is_capturable);
  uint32_t start_head_offset_len;

  qmin = re_ctx_p->current_token.qmin;
  qmax = re_ctx_p->current_token.qmax;
  JERRY_ASSERT (qmin <= qmax);

  start_head_offset_len = re_get_bytecode_length (re_ctx_p->bytecode_ctx_p);
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
} /* re_insert_into_group */

/**
 * Enclose the given bytecode to a group and inster jump value
 */
static void
re_insert_into_group_with_jump (re_compiler_ctx_t *re_ctx_p, /**< RegExp compiler context */
                                uint32_t group_start_offset, /**< offset of group start */
                                uint32_t idx, /**< index of group */
                                bool is_capturable) /**< is capturable group */
{
  re_insert_u32 (re_ctx_p->bytecode_ctx_p,
                 group_start_offset,
                 re_get_bytecode_length (re_ctx_p->bytecode_ctx_p) - group_start_offset);
  re_insert_into_group (re_ctx_p, group_start_offset, idx, is_capturable);
} /* re_insert_into_group_with_jump */

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
  uint32_t idx;
  re_bytecode_ctx_t *bc_ctx_p = re_ctx_p->bytecode_ctx_p;
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  uint32_t alterantive_offset = re_get_bytecode_length (re_ctx_p->bytecode_ctx_p);
  bool should_loop = true;

  while (ecma_is_value_empty (ret_value) && should_loop)
  {
    ECMA_TRY_CATCH (empty,
                    re_parse_next_token (re_ctx_p->parser_ctx_p,
                                         &(re_ctx_p->current_token)),
                    ret_value);

    uint32_t new_atom_start_offset = re_get_bytecode_length (re_ctx_p->bytecode_ctx_p);

    switch (re_ctx_p->current_token.type)
    {
      case RE_TOK_START_CAPTURE_GROUP:
      {
        idx = re_ctx_p->num_of_captures++;
        JERRY_TRACE_MSG ("Compile a capture group start (idx: %u)\n", (unsigned int) idx);

        ret_value = re_parse_alternative (re_ctx_p, false);

        if (ecma_is_value_empty (ret_value))
        {
          re_insert_into_group (re_ctx_p, new_atom_start_offset, idx, true);
        }

        break;
      }
      case RE_TOK_START_NON_CAPTURE_GROUP:
      {
        idx = re_ctx_p->num_of_non_captures++;
        JERRY_TRACE_MSG ("Compile a non-capture group start (idx: %u)\n", (unsigned int) idx);

        ret_value = re_parse_alternative (re_ctx_p, false);

        if (ecma_is_value_empty (ret_value))
        {
          re_insert_into_group (re_ctx_p, new_atom_start_offset, idx, false);
        }

        break;
      }
      case RE_TOK_CHAR:
      {
        JERRY_TRACE_MSG ("Compile character token: %c, qmin: %u, qmax: %u\n",
                         (char) re_ctx_p->current_token.value, (unsigned int) re_ctx_p->current_token.qmin,
                         (unsigned int) re_ctx_p->current_token.qmax);

        re_append_opcode (bc_ctx_p, RE_OP_CHAR);
        re_append_char (bc_ctx_p, re_canonicalize ((ecma_char_t) re_ctx_p->current_token.value,
                                                   re_ctx_p->flags & RE_FLAG_IGNORE_CASE));

        if ((re_ctx_p->current_token.qmin != 1) || (re_ctx_p->current_token.qmax != 1))
        {
          re_insert_simple_iterator (re_ctx_p, new_atom_start_offset);
        }
        break;
      }
      case RE_TOK_PERIOD:
      {
        JERRY_TRACE_MSG ("Compile a period\n");
        re_append_opcode (bc_ctx_p, RE_OP_PERIOD);

        if ((re_ctx_p->current_token.qmin != 1) || (re_ctx_p->current_token.qmax != 1))
        {
          re_insert_simple_iterator (re_ctx_p, new_atom_start_offset);
        }
        break;
      }
      case RE_TOK_ALTERNATIVE:
      {
        JERRY_TRACE_MSG ("Compile an alternative\n");
        re_insert_u32 (bc_ctx_p, alterantive_offset, re_get_bytecode_length (bc_ctx_p) - alterantive_offset);
        re_append_opcode (bc_ctx_p, RE_OP_ALTERNATIVE);
        alterantive_offset = re_get_bytecode_length (re_ctx_p->bytecode_ctx_p);
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
        idx = re_ctx_p->num_of_non_captures++;
        re_append_opcode (bc_ctx_p, RE_OP_LOOKAHEAD_POS);

        ret_value = re_parse_alternative (re_ctx_p, false);

        if (ecma_is_value_empty (ret_value))
        {
          re_append_opcode (bc_ctx_p, RE_OP_MATCH);

          re_insert_into_group_with_jump (re_ctx_p, new_atom_start_offset, idx, false);
        }

        break;
      }
      case RE_TOK_ASSERT_START_NEG_LOOKAHEAD:
      {
        JERRY_TRACE_MSG ("Compile a negative lookahead assertion\n");
        idx = re_ctx_p->num_of_non_captures++;
        re_append_opcode (bc_ctx_p, RE_OP_LOOKAHEAD_NEG);

        ret_value = re_parse_alternative (re_ctx_p, false);

        if (ecma_is_value_empty (ret_value))
        {
          re_append_opcode (bc_ctx_p, RE_OP_MATCH);

          re_insert_into_group_with_jump (re_ctx_p, new_atom_start_offset, idx, false);
        }

        break;
      }
      case RE_TOK_BACKREFERENCE:
      {
        uint32_t backref = (uint32_t) re_ctx_p->current_token.value;
        idx = re_ctx_p->num_of_non_captures++;

        if (backref > re_ctx_p->highest_backref)
        {
          re_ctx_p->highest_backref = backref;
        }

        JERRY_TRACE_MSG ("Compile a backreference: %u\n", (unsigned int) backref);
        re_append_opcode (bc_ctx_p, RE_OP_BACKREFERENCE);
        re_append_u32 (bc_ctx_p, backref);

        re_insert_into_group_with_jump (re_ctx_p, new_atom_start_offset, idx, false);
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

        ECMA_TRY_CATCH (empty_value,
                        re_parse_char_class (re_ctx_p->parser_ctx_p,
                                             re_append_char_class,
                                             re_ctx_p,
                                             &(re_ctx_p->current_token)),
                        ret_value);
        re_insert_u32 (bc_ctx_p, offset, re_ctx_p->parser_ctx_p->num_of_classes);

        if ((re_ctx_p->current_token.qmin != 1) || (re_ctx_p->current_token.qmax != 1))
        {
          re_insert_simple_iterator (re_ctx_p, new_atom_start_offset);
        }

        ECMA_FINALIZE (empty_value);

        break;
      }
      case RE_TOK_END_GROUP:
      {
        JERRY_TRACE_MSG ("Compile a group end\n");

        if (expect_eof)
        {
          ret_value = ecma_raise_syntax_error (ECMA_ERR_MSG ("Unexpected end of paren."));
        }
        else
        {
          re_insert_u32 (bc_ctx_p, alterantive_offset, re_get_bytecode_length (bc_ctx_p) - alterantive_offset);
          should_loop = false;
        }
        break;
      }
      case RE_TOK_EOF:
      {
        if (!expect_eof)
        {
          ret_value = ecma_raise_syntax_error (ECMA_ERR_MSG ("Unexpected end of pattern."));
        }
        else
        {
          re_insert_u32 (bc_ctx_p, alterantive_offset, re_get_bytecode_length (bc_ctx_p) - alterantive_offset);
          should_loop = false;
        }

        break;
      }
      default:
      {
        ret_value = ecma_raise_syntax_error (ECMA_ERR_MSG ("Unexpected RegExp token."));
        break;
      }
    }
    ECMA_FINALIZE (empty);
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
      ecma_string_t *cached_pattern_str_p;
      cached_pattern_str_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t, cached_bytecode_p->pattern_cp);

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
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
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
  re_ctx.num_of_non_captures = 0;

  re_bytecode_ctx_t bc_ctx;
  bc_ctx.block_start_p = NULL;
  bc_ctx.block_end_p = NULL;
  bc_ctx.current_p = NULL;

  re_ctx.bytecode_ctx_p = &bc_ctx;

  ECMA_STRING_TO_UTF8_STRING (pattern_str_p, pattern_start_p, pattern_start_size);

  re_parser_ctx_t parser_ctx;
  parser_ctx.input_start_p = pattern_start_p;
  parser_ctx.input_curr_p = (lit_utf8_byte_t *) pattern_start_p;
  parser_ctx.input_end_p = pattern_start_p + pattern_start_size;
  parser_ctx.num_of_groups = -1;
  re_ctx.parser_ctx_p = &parser_ctx;

  /* 1. Parse RegExp pattern */
  re_ctx.num_of_captures = 1;
  re_append_opcode (&bc_ctx, RE_OP_SAVE_AT_START);

  ECMA_TRY_CATCH (empty, re_parse_alternative (&re_ctx, true), ret_value);

  /* 2. Check for invalid backreference */
  if (re_ctx.highest_backref >= re_ctx.num_of_captures)
  {
    ret_value = ecma_raise_syntax_error ("Invalid backreference.\n");
  }
  else
  {
    re_append_opcode (&bc_ctx, RE_OP_SAVE_AND_MATCH);
    re_append_opcode (&bc_ctx, RE_OP_EOF);

    /* 3. Insert extra informations for bytecode header */
    re_compiled_code_t re_compiled_code;

    re_compiled_code.header.refs = 1;
    re_compiled_code.header.status_flags = re_ctx.flags;
    ecma_ref_ecma_string (pattern_str_p);
    ECMA_SET_NON_NULL_POINTER (re_compiled_code.pattern_cp, pattern_str_p);
    re_compiled_code.num_of_captures = re_ctx.num_of_captures * 2;
    re_compiled_code.num_of_non_captures = re_ctx.num_of_non_captures;

    re_bytecode_list_insert (&bc_ctx,
                             0,
                             (uint8_t *) &re_compiled_code,
                             sizeof (re_compiled_code_t));
  }

  ECMA_FINALIZE (empty);

  ECMA_FINALIZE_UTF8_STRING (pattern_start_p, pattern_start_size);

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
#ifdef REGEXP_DUMP_BYTE_CODE
    if (JERRY_CONTEXT (jerry_init_flags) & ECMA_INIT_SHOW_REGEXP_OPCODES)
    {
      re_dump_bytecode (&bc_ctx);
    }
#endif /* REGEXP_DUMP_BYTE_CODE */

    /* The RegExp bytecode contains at least a RE_OP_SAVE_AT_START opdoce, so it cannot be NULL. */
    JERRY_ASSERT (bc_ctx.block_start_p != NULL);
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

#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */

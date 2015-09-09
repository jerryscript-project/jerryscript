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
#include "ecma-helpers.h"
#include "ecma-regexp-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt-libc-includes.h"
#include "mem-heap.h"
#include "re-compiler.h"
#include "re-parser.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN

/**
 * Size of block of RegExp bytecode. Used for allocation
 */
#define REGEXP_BYTECODE_BLOCK_SIZE 256UL

/**
 * Get length of bytecode
 */
static uint32_t
re_get_bytecode_length (re_bytecode_ctx_t *bc_ctx_p)
{
  return ((uint32_t) (bc_ctx_p->current_p - bc_ctx_p->block_start_p));
} /* re_get_bytecode_length */

void
re_dump_bytecode (re_bytecode_ctx_t *bc_ctx);

/**
 * Realloc the bytecode container
 *
 * @return current position in RegExp bytecode
 */
static re_bytecode_t*
re_realloc_regexp_bytecode_block (re_bytecode_ctx_t *bc_ctx_p) /**< RegExp bytecode context */
{
  JERRY_ASSERT (bc_ctx_p->block_end_p - bc_ctx_p->block_start_p >= 0);
  size_t old_size = static_cast<size_t> (bc_ctx_p->block_end_p - bc_ctx_p->block_start_p);

  /* If one of the members of RegExp bytecode context is NULL, then all member should be NULL
   * (it means first allocation), otherwise all of the members should be a non NULL pointer. */
  JERRY_ASSERT ((!bc_ctx_p->current_p && !bc_ctx_p->block_end_p && !bc_ctx_p->block_start_p)
                || (bc_ctx_p->current_p && bc_ctx_p->block_end_p && bc_ctx_p->block_start_p));

  size_t new_block_size = old_size + REGEXP_BYTECODE_BLOCK_SIZE;
  JERRY_ASSERT (bc_ctx_p->current_p - bc_ctx_p->block_start_p >= 0);
  size_t current_ptr_offset = static_cast<size_t> (bc_ctx_p->current_p - bc_ctx_p->block_start_p);

  re_bytecode_t *new_block_start_p = (re_bytecode_t *) mem_heap_alloc_block (new_block_size,
                                                                             MEM_HEAP_ALLOC_SHORT_TERM);
  if (bc_ctx_p->current_p)
  {
    memcpy (new_block_start_p, bc_ctx_p->block_start_p, static_cast<size_t> (current_ptr_offset));
    mem_heap_free_block (bc_ctx_p->block_start_p);
  }
  bc_ctx_p->block_start_p = new_block_start_p;
  bc_ctx_p->block_end_p = new_block_start_p + new_block_size;
  bc_ctx_p->current_p = new_block_start_p + current_ptr_offset;

  return bc_ctx_p->current_p;
} /* re_realloc_regexp_bytecode_block */

/**
 * Append a new bytecode to the and of the bytecode container
 */
static void
re_bytecode_list_append (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
                         re_bytecode_t *bytecode_p, /**< input bytecode */
                         size_t length) /**< length of input */
{
  JERRY_ASSERT (length <= REGEXP_BYTECODE_BLOCK_SIZE);

  re_bytecode_t *current_p = bc_ctx_p->current_p;
  if (current_p + length > bc_ctx_p->block_end_p)
  {
    current_p = re_realloc_regexp_bytecode_block (bc_ctx_p);
  }

  memcpy (current_p, bytecode_p, length);
  bc_ctx_p->current_p += length;
} /* re_bytecode_list_append */

/**
 * Insert a new bytecode to the bytecode container
 */
static void
re_bytecode_list_insert (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
                         size_t offset, /**< distance from the start of the container */
                         re_bytecode_t *bytecode_p, /**< input bytecode */
                         size_t length) /**< length of input */
{
  JERRY_ASSERT (length <= REGEXP_BYTECODE_BLOCK_SIZE);

  re_bytecode_t *current_p = bc_ctx_p->current_p;
  if (current_p + length > bc_ctx_p->block_end_p)
  {
    re_realloc_regexp_bytecode_block (bc_ctx_p);
  }

  re_bytecode_t *src_p = bc_ctx_p->block_start_p + offset;
  if ((re_get_bytecode_length (bc_ctx_p) - offset) > 0)
  {
    re_bytecode_t *dest_p = src_p + length;
    re_bytecode_t *tmp_block_start_p;
    tmp_block_start_p = (re_bytecode_t *) mem_heap_alloc_block ((re_get_bytecode_length (bc_ctx_p) - offset),
                                                                 MEM_HEAP_ALLOC_SHORT_TERM);
    memcpy (tmp_block_start_p, src_p, (size_t) (re_get_bytecode_length (bc_ctx_p) - offset));
    memcpy (dest_p, tmp_block_start_p, (size_t) (re_get_bytecode_length (bc_ctx_p) - offset));
    mem_heap_free_block (tmp_block_start_p);
  }
  memcpy (src_p, bytecode_p, length);

  bc_ctx_p->current_p += length;
} /* re_bytecode_list_insert */

/**
 * Append a RegExp opcode
 */
static void
re_append_opcode (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
                  re_opcode_t opcode) /**< input opcode */
{
  re_bytecode_list_append (bc_ctx_p, (re_bytecode_t*) &opcode, sizeof (re_bytecode_t));
} /* re_append_opcode */

/**
 * Append a parameter of a RegExp opcode
 */
static void
re_append_u32 (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
               uint32_t value) /**< input value */
{
  re_bytecode_list_append (bc_ctx_p, (re_bytecode_t*) &value, sizeof (uint32_t));
} /* re_append_u32 */

/**
 * Append a jump offset parameter of a RegExp opcode
 */
static void
re_append_jump_offset (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
                       uint32_t value) /**< input value */
{
  value += (uint32_t) (sizeof (uint32_t));
  re_append_u32 (bc_ctx_p, value);
} /* re_append_jump_offset */

/**
 * Insert a RegExp opcode
 */
static void
re_insert_opcode (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
                  uint32_t offset, /**< distance from the start of the container */
                  re_opcode_t opcode) /**< input opcode */
{
  re_bytecode_list_insert (bc_ctx_p, offset, (re_bytecode_t*) &opcode, sizeof (re_bytecode_t));
} /* re_insert_opcode */

/**
 * Insert a parameter of a RegExp opcode
 */
static void
re_insert_u32 (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
               uint32_t offset, /**< distance from the start of the container */
               uint32_t value) /**< input value */
{
  re_bytecode_list_insert (bc_ctx_p, offset, (re_bytecode_t*) &value, sizeof (uint32_t));
} /* re_insert_u32 */

/**
 * Get a RegExp opcode
 */
re_opcode_t
re_get_opcode (re_bytecode_t **bc_p) /**< pointer to bytecode start */
{
  re_bytecode_t bytecode = **bc_p;
  (*bc_p) += sizeof (re_bytecode_t);
  return (re_opcode_t) bytecode;
} /* get_opcode */

/**
 * Get a parameter of a RegExp opcode
 */
uint32_t
re_get_value (re_bytecode_t **bc_p) /**< pointer to bytecode start */
{
  uint32_t value = *((uint32_t*) *bc_p);
  (*bc_p) += sizeof (uint32_t);
  return value;
} /* get_value */

/**
 * Callback function of character class generation
 */
static void
re_append_char_class (void* re_ctx_p, /**< RegExp compiler context */
                      uint32_t start, /**< character class range from */
                      uint32_t end) /**< character class range to */
{
  /* FIXME: Handle ignore case flag and add unicode support. */
  re_compiler_ctx_t *ctx_p = (re_compiler_ctx_t*) re_ctx_p;
  re_append_u32 (ctx_p->bytecode_ctx_p, start);
  re_append_u32 (ctx_p->bytecode_ctx_p, end);
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

  /* FIXME: optimize bytecode length. Store 0 rather than INF */

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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
re_parse_alternative (re_compiler_ctx_t *re_ctx_p, /**< RegExp compiler context */
                      bool expect_eof) /**< expect end of file */
{
  uint32_t idx;
  re_bytecode_ctx_t *bc_ctx_p = re_ctx_p->bytecode_ctx_p;
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  uint32_t alterantive_offset = re_get_bytecode_length (re_ctx_p->bytecode_ctx_p);
  bool should_loop = true;

  while (ecma_is_completion_value_empty (ret_value) && should_loop)
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
        JERRY_DDLOG ("Compile a capture group start (idx: %d)\n", idx);

        ret_value = re_parse_alternative (re_ctx_p, false);

        if (ecma_is_completion_value_empty (ret_value))
        {
          re_insert_into_group (re_ctx_p, new_atom_start_offset, idx, true);
        }

        break;
      }
      case RE_TOK_START_NON_CAPTURE_GROUP:
      {
        idx = re_ctx_p->num_of_non_captures++;
        JERRY_DDLOG ("Compile a non-capture group start (idx: %d)\n", idx);

        ret_value = re_parse_alternative (re_ctx_p, false);

        if (ecma_is_completion_value_empty (ret_value))
        {
          re_insert_into_group (re_ctx_p, new_atom_start_offset, idx, false);
        }

        break;
      }
      case RE_TOK_CHAR:
      {
        JERRY_DDLOG ("Compile character token: %c, qmin: %d, qmax: %d\n",
                     re_ctx_p->current_token.value, re_ctx_p->current_token.qmin, re_ctx_p->current_token.qmax);

        re_append_opcode (bc_ctx_p, RE_OP_CHAR);
        re_append_u32 (bc_ctx_p, re_canonicalize ((ecma_char_t) re_ctx_p->current_token.value,
                                                   re_ctx_p->flags & RE_FLAG_IGNORE_CASE));

        if ((re_ctx_p->current_token.qmin != 1) || (re_ctx_p->current_token.qmax != 1))
        {
          re_insert_simple_iterator (re_ctx_p, new_atom_start_offset);
        }
        break;
      }
      case RE_TOK_PERIOD:
      {
        JERRY_DDLOG ("Compile a period\n");
        re_append_opcode (bc_ctx_p, RE_OP_PERIOD);

        if ((re_ctx_p->current_token.qmin != 1) || (re_ctx_p->current_token.qmax != 1))
        {
          re_insert_simple_iterator (re_ctx_p, new_atom_start_offset);
        }
        break;
      }
      case RE_TOK_ALTERNATIVE:
      {
        JERRY_DDLOG ("Compile an alternative\n");
        re_insert_u32 (bc_ctx_p, alterantive_offset, re_get_bytecode_length (bc_ctx_p) - alterantive_offset);
        re_append_opcode (bc_ctx_p, RE_OP_ALTERNATIVE);
        alterantive_offset = re_get_bytecode_length (re_ctx_p->bytecode_ctx_p);
        break;
      }
      case RE_TOK_ASSERT_START:
      {
        JERRY_DDLOG ("Compile a start assertion\n");
        re_append_opcode (bc_ctx_p, RE_OP_ASSERT_START);
        break;
      }
      case RE_TOK_ASSERT_END:
      {
        JERRY_DDLOG ("Compile an end assertion\n");
        re_append_opcode (bc_ctx_p, RE_OP_ASSERT_END);
        break;
      }
      case RE_TOK_ASSERT_WORD_BOUNDARY:
      {
        JERRY_DDLOG ("Compile a word boundary assertion\n");
        re_append_opcode (bc_ctx_p, RE_OP_ASSERT_WORD_BOUNDARY);
        break;
      }
      case RE_TOK_ASSERT_NOT_WORD_BOUNDARY:
      {
        JERRY_DDLOG ("Compile a not word boundary assertion\n");
        re_append_opcode (bc_ctx_p, RE_OP_ASSERT_NOT_WORD_BOUNDARY);
        break;
      }
      case RE_TOK_ASSERT_START_POS_LOOKAHEAD:
      {
        JERRY_DDLOG ("Compile a positive lookahead assertion\n");
        idx = re_ctx_p->num_of_non_captures++;
        re_append_opcode (bc_ctx_p, RE_OP_LOOKAHEAD_POS);

        ret_value = re_parse_alternative (re_ctx_p, false);

        if (ecma_is_completion_value_empty (ret_value))
        {
          re_append_opcode (bc_ctx_p, RE_OP_MATCH);

          re_insert_into_group_with_jump (re_ctx_p, new_atom_start_offset, idx, false);
        }

        break;
      }
      case RE_TOK_ASSERT_START_NEG_LOOKAHEAD:
      {
        JERRY_DDLOG ("Compile a negative lookahead assertion\n");
        idx = re_ctx_p->num_of_non_captures++;
        re_append_opcode (bc_ctx_p, RE_OP_LOOKAHEAD_NEG);

        ret_value = re_parse_alternative (re_ctx_p, false);

        if (ecma_is_completion_value_empty (ret_value))
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

        JERRY_DDLOG ("Compile a backreference: %d\n", backref);
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
        JERRY_DDLOG ("Compile a character class\n");
        re_append_opcode (bc_ctx_p,
                          re_ctx_p->current_token.type == RE_TOK_START_INV_CHAR_CLASS
                                                       ? RE_OP_INV_CHAR_CLASS
                                                       : RE_OP_CHAR_CLASS);
        uint32_t offset = re_get_bytecode_length (re_ctx_p->bytecode_ctx_p);

        ECMA_TRY_CATCH (empty,
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

        ECMA_FINALIZE (empty);

        break;
      }
      case RE_TOK_END_GROUP:
      {
        JERRY_DDLOG ("Compile a group end\n");

        if (expect_eof)
        {
          ret_value = ecma_raise_syntax_error ("Unexpected end of paren.");
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
          ret_value = ecma_raise_syntax_error ("Unexpected end of pattern.");
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
        ret_value = ecma_raise_syntax_error ("Unexpected RegExp token.");
        break;
      }
    }
    ECMA_FINALIZE (empty);
  }

  return ret_value;
} /* re_parse_alternative */

/**
 * Compilation of RegExp bytecode
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
re_compile_bytecode (re_bytecode_t **out_bytecode_p, /**< out:pointer to bytecode */
                     ecma_string_t *pattern_str_p, /**< pattern */
                     uint8_t flags) /**< flags */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  re_compiler_ctx_t re_ctx;
  re_ctx.flags = flags;
  re_ctx.highest_backref = 0;
  re_ctx.num_of_non_captures = 0;

  re_bytecode_ctx_t bc_ctx;
  bc_ctx.block_start_p = NULL;
  bc_ctx.block_end_p = NULL;
  bc_ctx.current_p = NULL;

  re_ctx.bytecode_ctx_p = &bc_ctx;

  lit_utf8_size_t pattern_str_size = ecma_string_get_size (pattern_str_p);
  MEM_DEFINE_LOCAL_ARRAY (pattern_start_p, pattern_str_size, lit_utf8_byte_t);

  ssize_t sz = ecma_string_to_utf8_string (pattern_str_p, pattern_start_p, (ssize_t) pattern_str_size);
  JERRY_ASSERT (sz >= 0);

  re_parser_ctx_t parser_ctx;
  parser_ctx.input_start_p = pattern_start_p;
  parser_ctx.input_curr_p = pattern_start_p;
  parser_ctx.input_end_p = pattern_start_p + pattern_str_size;
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
    re_insert_u32 (&bc_ctx, 0, (uint32_t) re_ctx.num_of_non_captures);
    re_insert_u32 (&bc_ctx, 0, (uint32_t) re_ctx.num_of_captures * 2);
    re_insert_u32 (&bc_ctx, 0, (uint32_t) re_ctx.flags);
  }
  ECMA_FINALIZE (empty);

  MEM_FINALIZE_LOCAL_ARRAY (pattern_start_p);

  if (!ecma_is_completion_value_empty (ret_value))
  {
    /* Compilation failed, free bytecode. */
    mem_heap_free_block (bc_ctx.block_start_p);
    *out_bytecode_p = NULL;
  }
  else
  {
    /* The RegExp bytecode contains at least a RE_OP_SAVE_AT_START opdoce, so it cannot be NULL. */
    JERRY_ASSERT (bc_ctx.block_start_p != NULL);
    *out_bytecode_p = bc_ctx.block_start_p;
  }

#ifdef JERRY_ENABLE_LOG
  re_dump_bytecode (&bc_ctx);
#endif

  return ret_value;
} /* re_compile_bytecode */

#ifdef JERRY_ENABLE_LOG
/**
 * RegExp bytecode dumper
 */
void
re_dump_bytecode (re_bytecode_ctx_t *bc_ctx_p)
{
  re_bytecode_t *bytecode_p = bc_ctx_p->block_start_p;
  JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
  JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
  JERRY_DLOG ("%d | ", re_get_value (&bytecode_p));

  re_opcode_t op;
  while ((op = re_get_opcode (&bytecode_p)))
  {
    switch (op)
    {
      case RE_OP_MATCH:
      {
        JERRY_DLOG ("MATCH, ");
        break;
      }
      case RE_OP_CHAR:
      {
        JERRY_DLOG ("CHAR ");
        JERRY_DLOG ("%c, ", (char) re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_CAPTURE_NON_GREEDY_ZERO_GROUP_START:
      {
        JERRY_DLOG ("N");
        /* FALLTHRU */
      }
      case RE_OP_CAPTURE_GREEDY_ZERO_GROUP_START:
      {
        JERRY_DLOG ("GZ_START ");
        JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_CAPTURE_GROUP_START:
      {
        JERRY_DLOG ("START ");
        JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_CAPTURE_NON_GREEDY_GROUP_END:
      {
        JERRY_DLOG ("N");
        /* FALLTHRU */
      }
      case RE_OP_CAPTURE_GREEDY_GROUP_END:
      {
        JERRY_DLOG ("G_END ");
        JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_NON_CAPTURE_NON_GREEDY_ZERO_GROUP_START:
      {
        JERRY_DLOG ("N");
        /* FALLTHRU */
      }
      case RE_OP_NON_CAPTURE_GREEDY_ZERO_GROUP_START:
      {
        JERRY_DLOG ("GZ_NC_START ");
        JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_NON_CAPTURE_GROUP_START:
      {
        JERRY_DLOG ("NC_START ");
        JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_NON_CAPTURE_NON_GREEDY_GROUP_END:
      {
        JERRY_DLOG ("N");
        /* FALLTHRU */
      }
      case RE_OP_NON_CAPTURE_GREEDY_GROUP_END:
      {
        JERRY_DLOG ("G_NC_END ");
        JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_SAVE_AT_START:
      {
        JERRY_DLOG ("RE_START ");
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_SAVE_AND_MATCH:
      {
        JERRY_DLOG ("RE_END, ");
        break;
      }
      case RE_OP_GREEDY_ITERATOR:
      {
        JERRY_DLOG ("GREEDY_ITERATOR ");
        JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_NON_GREEDY_ITERATOR:
      {
        JERRY_DLOG ("NON_GREEDY_ITERATOR ");
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_PERIOD:
      {
        JERRY_DLOG ("PERIOD ");
        break;
      }
      case RE_OP_ALTERNATIVE:
      {
        JERRY_DLOG ("ALTERNATIVE ");
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_ASSERT_START:
      {
        JERRY_DLOG ("ASSERT_START ");
        break;
      }
      case RE_OP_ASSERT_END:
      {
        JERRY_DLOG ("ASSERT_END ");
        break;
      }
      case RE_OP_ASSERT_WORD_BOUNDARY:
      {
        JERRY_DLOG ("ASSERT_WORD_BOUNDARY ");
        break;
      }
      case RE_OP_ASSERT_NOT_WORD_BOUNDARY:
      {
        JERRY_DLOG ("ASSERT_NOT_WORD_BOUNDARY ");
        break;
      }
      case RE_OP_LOOKAHEAD_POS:
      {
        JERRY_DLOG ("LOOKAHEAD_POS ");
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_LOOKAHEAD_NEG:
      {
        JERRY_DLOG ("LOOKAHEAD_NEG ");
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_BACKREFERENCE:
      {
        JERRY_DLOG ("BACKREFERENCE ");
        JERRY_DLOG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_INV_CHAR_CLASS:
      {
        JERRY_DLOG ("INV_");
        /* FALLTHRU */
      }
      case RE_OP_CHAR_CLASS:
      {
        JERRY_DLOG ("CHAR_CLASS ");
        uint32_t num_of_class = re_get_value (&bytecode_p);
        JERRY_DLOG ("%d", num_of_class);
        while (num_of_class)
        {
          JERRY_DLOG (" %d", re_get_value (&bytecode_p));
          JERRY_DLOG ("-%d", re_get_value (&bytecode_p));
          num_of_class--;
        }
        JERRY_DLOG (", ");
        break;
      }
      default:
      {
        JERRY_DLOG ("UNKNOWN(%d), ", (uint32_t) op);
        break;
      }
    }
  }
  JERRY_DLOG ("EOF\n");
} /* re_dump_bytecode */
#endif /* JERRY_ENABLE_LOG */

#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */

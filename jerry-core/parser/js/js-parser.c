/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged.
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
#include "ecma-literal-storage.h"
#include "jcontext.h"
#include "js-parser-internal.h"

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup jsparser JavaScript
 * @{
 *
 * \addtogroup jsparser_parser Parser
 * @{
 */

/**
 * Compute real literal indicies.
 *
 * @return length of the prefix opcodes
 */
static size_t
parser_compute_indicies (parser_context_t *context_p, /**< context */
                         uint16_t *ident_end, /**< end of the identifier group */
                         uint16_t *uninitialized_var_end, /**< end of the uninitialized var group */
                         uint16_t *initialized_var_end, /**< end of the initialized var group */
                         uint16_t *const_literal_end) /**< end of the const literal group */
{
  parser_list_iterator_t literal_iterator;
  lexer_literal_t *literal_p;
  size_t length = 0;
  uint16_t literal_one_byte_limit;
  uint32_t status_flags = context_p->status_flags;
  uint16_t argument_count;

  uint16_t register_count = context_p->register_count;
  uint16_t uninitialized_var_count = 0;
  uint16_t initialized_var_count = 0;
  uint16_t ident_count = 0;
  uint16_t const_literal_count = 0;

  uint16_t register_index;
  uint16_t uninitialized_var_index;
  uint16_t initialized_var_index;
  uint16_t ident_index;
  uint16_t const_literal_index;
  uint16_t literal_index;

  if (status_flags & PARSER_ARGUMENTS_NOT_NEEDED)
  {
    status_flags &= ~PARSER_ARGUMENTS_NEEDED;
    context_p->status_flags = status_flags;
  }

  /* First phase: count the number of items in each group. */
  parser_list_iterator_init (&context_p->literal_pool, &literal_iterator);
  while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)))
  {
#ifndef PARSER_DUMP_BYTE_CODE
    if (literal_p->type == LEXER_IDENT_LITERAL
        || literal_p->type == LEXER_STRING_LITERAL)
    {
      const uint8_t *char_p = literal_p->u.char_p;

      if ((literal_p->status_flags & LEXER_FLAG_SOURCE_PTR)
          && literal_p->prop.length < 0xfff)
      {
        size_t bytes_to_end = (size_t) (context_p->source_end_p - char_p);

        if (bytes_to_end < 0xfffff)
        {
          literal_p->u.source_data = ((uint32_t) bytes_to_end) | (((uint32_t) literal_p->prop.length) << 20);
          literal_p->status_flags |= LEXER_FLAG_LATE_INIT;
          status_flags |= PARSER_HAS_LATE_LIT_INIT;
          context_p->status_flags = status_flags;
          char_p = NULL;
        }
      }

      if (char_p != NULL)
      {
        literal_p->u.value = ecma_find_or_create_literal_string (char_p,
                                                                 literal_p->prop.length);

        if (!(literal_p->status_flags & LEXER_FLAG_SOURCE_PTR))
        {
          jmem_heap_free_block ((void *) char_p, literal_p->prop.length);
        }
      }
    }
#endif /* !PARSER_DUMP_BYTE_CODE */

    switch (literal_p->type)
    {
      case LEXER_IDENT_LITERAL:
      {
        if (literal_p->status_flags & LEXER_FLAG_VAR)
        {
          if (status_flags & PARSER_NO_REG_STORE)
          {
            literal_p->status_flags |= LEXER_FLAG_NO_REG_STORE;
          }

          if (literal_p->status_flags & LEXER_FLAG_INITIALIZED)
          {
            if (literal_p->status_flags & LEXER_FLAG_FUNCTION_NAME)
            {
              JERRY_ASSERT (literal_p == PARSER_GET_LITERAL (0));

              status_flags |= PARSER_NAMED_FUNCTION_EXP | PARSER_NO_REG_STORE | PARSER_LEXICAL_ENV_NEEDED;
              context_p->status_flags = status_flags;

              literal_p->status_flags |= LEXER_FLAG_NO_REG_STORE;
              context_p->literal_count++;
            }

            if (literal_p->status_flags & LEXER_FLAG_FUNCTION_ARGUMENT)
            {
              if ((status_flags & PARSER_ARGUMENTS_NEEDED)
                  && !(status_flags & PARSER_IS_STRICT))
              {
                literal_p->status_flags |= LEXER_FLAG_NO_REG_STORE;
              }

              /* Arguments are bound to their position, or
               * moved to the initialized var section. */
              if (literal_p->status_flags & LEXER_FLAG_NO_REG_STORE)
              {
                initialized_var_count++;
                context_p->literal_count++;
              }
            }
            else if (!(literal_p->status_flags & LEXER_FLAG_NO_REG_STORE)
                     && register_count < PARSER_MAXIMUM_NUMBER_OF_REGISTERS)
            {
              register_count++;
            }
            else
            {
              literal_p->status_flags |= LEXER_FLAG_NO_REG_STORE;
              initialized_var_count++;
            }

            if (context_p->literal_count >= PARSER_MAXIMUM_NUMBER_OF_LITERALS)
            {
              parser_raise_error (context_p, PARSER_ERR_LITERAL_LIMIT_REACHED);
            }
          }
          else if (!(literal_p->status_flags & LEXER_FLAG_NO_REG_STORE)
                   && register_count < PARSER_MAXIMUM_NUMBER_OF_REGISTERS)
          {
            register_count++;
          }
          else
          {
            literal_p->status_flags |= LEXER_FLAG_NO_REG_STORE;
            uninitialized_var_count++;
          }
        }
        else
        {
          ident_count++;
        }
        break;
      }
      case LEXER_STRING_LITERAL:
      case LEXER_NUMBER_LITERAL:
      {
        const_literal_count++;
        break;
      }
      case LEXER_UNUSED_LITERAL:
      {
        if (!(literal_p->status_flags & LEXER_FLAG_FUNCTION_ARGUMENT))
        {
          context_p->literal_count--;
        }
        break;
      }
    }
  }

  if (context_p->literal_count <= CBC_MAXIMUM_SMALL_VALUE)
  {
    literal_one_byte_limit = CBC_MAXIMUM_BYTE_VALUE - 1;
  }
  else
  {
    literal_one_byte_limit = CBC_LOWER_SEVEN_BIT_MASK;
  }

  if (uninitialized_var_count > 0)
  {
    /* Opcode byte and a literal argument. */
    length += 2;
    if ((register_count + uninitialized_var_count - 1) > literal_one_byte_limit)
    {
      length++;
    }
  }

  register_index = context_p->register_count;
  uninitialized_var_index = register_count;
  initialized_var_index = (uint16_t) (uninitialized_var_index + uninitialized_var_count);
  ident_index = (uint16_t) (initialized_var_index + initialized_var_count);
  const_literal_index = (uint16_t) (ident_index + ident_count);
  literal_index = (uint16_t) (const_literal_index + const_literal_count);

  if (initialized_var_count > 2)
  {
    status_flags |= PARSER_HAS_INITIALIZED_VARS;
    context_p->status_flags = status_flags;

    /* Opcode byte and two literal arguments. */
    length += 3;
    if (initialized_var_index > literal_one_byte_limit)
    {
      length++;
    }
    if (ident_index - 1 > literal_one_byte_limit)
    {
      length++;
    }
  }

  /* Second phase: Assign an index to each literal. */
  parser_list_iterator_init (&context_p->literal_pool, &literal_iterator);
  argument_count = 0;

  while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)))
  {
    uint16_t init_index;

    if (literal_p->type != LEXER_IDENT_LITERAL)
    {
      if (literal_p->type == LEXER_STRING_LITERAL
          || literal_p->type == LEXER_NUMBER_LITERAL)
      {
        JERRY_ASSERT ((literal_p->status_flags & ~(LEXER_FLAG_SOURCE_PTR | LEXER_FLAG_LATE_INIT)) == 0);
        literal_p->prop.index = const_literal_index;
        const_literal_index++;
        continue;
      }

      if (literal_p->type != LEXER_UNUSED_LITERAL)
      {
        JERRY_ASSERT (literal_p->status_flags == 0);
        JERRY_ASSERT (literal_p->type == LEXER_FUNCTION_LITERAL
                      || literal_p->type == LEXER_REGEXP_LITERAL);

        literal_p->prop.index = literal_index;
        literal_index++;
        continue;
      }

      JERRY_ASSERT ((literal_p->status_flags & ~(LEXER_FLAG_FUNCTION_ARGUMENT | LEXER_FLAG_SOURCE_PTR)) == 0);

      if (literal_p->status_flags & LEXER_FLAG_FUNCTION_ARGUMENT)
      {
        argument_count++;
      }
      continue;
    }

    if (!(literal_p->status_flags & LEXER_FLAG_VAR))
    {
      literal_p->prop.index = ident_index;
      ident_index++;
      continue;
    }

    if (!(literal_p->status_flags & LEXER_FLAG_INITIALIZED))
    {
      if (!(literal_p->status_flags & LEXER_FLAG_NO_REG_STORE))
      {
        JERRY_ASSERT (register_count < PARSER_MAXIMUM_NUMBER_OF_REGISTERS);
        /* This var literal can be stored in a register. */
        literal_p->prop.index = register_index;
        register_index++;
      }
      else
      {
        literal_p->prop.index = uninitialized_var_index;
        uninitialized_var_index++;
      }
      continue;
    }

    if (literal_p->status_flags & LEXER_FLAG_FUNCTION_ARGUMENT)
    {
      if (literal_p->status_flags & LEXER_FLAG_NO_REG_STORE)
      {
        literal_p->prop.index = initialized_var_index;
        initialized_var_index++;
        init_index = argument_count;
        argument_count++;
      }
      else
      {
        literal_p->prop.index = argument_count;
        argument_count++;
        continue;
      }
    }
    else
    {
      if (!(literal_p->status_flags & LEXER_FLAG_NO_REG_STORE))
      {
        JERRY_ASSERT (register_count < PARSER_MAXIMUM_NUMBER_OF_REGISTERS);
        /* This var literal can be stored in a register. */
        literal_p->prop.index = register_index;
        register_index++;
      }
      else
      {
        literal_p->prop.index = initialized_var_index;
        initialized_var_index++;
      }

      init_index = literal_index;
      literal_index++;

      if (!(literal_p->status_flags & LEXER_FLAG_FUNCTION_NAME))
      {
        lexer_literal_t *func_literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator);

        JERRY_ASSERT (func_literal_p != NULL
                      && func_literal_p->type == LEXER_FUNCTION_LITERAL);
        func_literal_p->prop.index = init_index;
      }
    }

    /* A CBC_INITIALIZE_VAR instruction or part of a CBC_INITIALIZE_VARS instruction. */
    if (!(status_flags & PARSER_HAS_INITIALIZED_VARS))
    {
      length += 2;
      if (literal_p->prop.index > literal_one_byte_limit)
      {
        length++;
      }
    }

    length++;
    if (init_index > literal_one_byte_limit)
    {
      length++;
    }
  }

  JERRY_ASSERT (argument_count == context_p->argument_count);
  JERRY_ASSERT (register_index == register_count);
  JERRY_ASSERT (uninitialized_var_index == register_count + uninitialized_var_count);
  JERRY_ASSERT (initialized_var_index == uninitialized_var_index + initialized_var_count);
  JERRY_ASSERT (ident_index == initialized_var_index + ident_count);
  JERRY_ASSERT (const_literal_index == ident_index + const_literal_count);
  JERRY_ASSERT (literal_index == context_p->literal_count);

  *ident_end = ident_index;
  *uninitialized_var_end = uninitialized_var_index;
  *initialized_var_end = initialized_var_index;
  *const_literal_end = const_literal_index;
  context_p->register_count = register_index;

  return length;
} /* parser_compute_indicies */

/**
 * Encode a literal argument.
 *
 * @return position after the encoded values
 */
static inline uint8_t *
parser_encode_literal (uint8_t *dst_p, /**< destination buffer */
                       uint16_t literal_index, /**< literal index */
                       uint16_t literal_one_byte_limit) /**< maximum value of a literal
                                                          *   encoded in one byte */
{
  if (literal_index <= literal_one_byte_limit)
  {
    *dst_p++ = (uint8_t) (literal_index);
  }
  else
  {
    if (literal_one_byte_limit == CBC_MAXIMUM_BYTE_VALUE - 1)
    {
      *dst_p++ = (uint8_t) (CBC_MAXIMUM_BYTE_VALUE);
      *dst_p++ = (uint8_t) (literal_index - CBC_MAXIMUM_BYTE_VALUE);
    }
    else
    {
      *dst_p++ = (uint8_t) ((literal_index >> 8) | CBC_HIGHEST_BIT_MASK);
      *dst_p++ = (uint8_t) (literal_index & CBC_MAXIMUM_BYTE_VALUE);
    }
  }
  return dst_p;
} /* parser_encode_literal */

/**
 * Generate initializer byte codes.
 *
 * @return the end of the initializer stream
 */
static uint8_t *
parser_generate_initializers (parser_context_t *context_p, /**< context */
                              uint8_t *dst_p, /**< destination buffer */
                              jmem_cpointer_t *literal_pool_p, /**< start of literal pool */
                              uint16_t uninitialized_var_end, /**< end of the uninitialized var group */
                              uint16_t initialized_var_end, /**< end of the initialized var group */
                              uint16_t const_literal_end, /**< end of the const literal group */
                              uint16_t literal_one_byte_limit) /**< maximum value of a literal
                                                                *   encoded in one byte */
{
  parser_list_iterator_t literal_iterator;
  lexer_literal_t *literal_p;
  uint16_t argument_count;

  if (uninitialized_var_end > context_p->register_count)
  {
    *dst_p++ = CBC_DEFINE_VARS;
    dst_p = parser_encode_literal (dst_p,
                                   (uint16_t) (uninitialized_var_end - 1),
                                   literal_one_byte_limit);
    context_p->status_flags |= PARSER_LEXICAL_ENV_NEEDED;
  }

  if (context_p->status_flags & PARSER_HAS_INITIALIZED_VARS)
  {
    const uint8_t expected_status_flags = LEXER_FLAG_VAR | LEXER_FLAG_NO_REG_STORE | LEXER_FLAG_INITIALIZED;
#ifndef JERRY_NDEBUG
    uint16_t next_index = uninitialized_var_end;
#endif /* !JERRY_NDEBUG */

    context_p->status_flags |= PARSER_LEXICAL_ENV_NEEDED;

    *dst_p++ = CBC_INITIALIZE_VARS;
    dst_p = parser_encode_literal (dst_p,
                                   (uint16_t) uninitialized_var_end,
                                   literal_one_byte_limit);
    dst_p = parser_encode_literal (dst_p,
                                   (uint16_t) (initialized_var_end - 1),
                                   literal_one_byte_limit);

    parser_list_iterator_init (&context_p->literal_pool, &literal_iterator);
    argument_count = 0;

    while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)))
    {
      if (literal_p->status_flags & LEXER_FLAG_FUNCTION_ARGUMENT)
      {
        argument_count++;
      }

      if ((literal_p->status_flags & expected_status_flags) == expected_status_flags)
      {
        uint16_t init_index;

        JERRY_ASSERT (literal_p->type == LEXER_IDENT_LITERAL);
#ifndef JERRY_NDEBUG
        JERRY_ASSERT (literal_p->prop.index == next_index);
        next_index++;
#endif /* !JERRY_NDEBUG */
        literal_p->status_flags = (uint8_t) (literal_p->status_flags & ~LEXER_FLAG_INITIALIZED);


        if (literal_p->status_flags & LEXER_FLAG_FUNCTION_NAME)
        {
          init_index = const_literal_end;
        }
        else if (literal_p->status_flags & LEXER_FLAG_FUNCTION_ARGUMENT)
        {
          init_index = (uint16_t) (argument_count - 1);
        }
        else
        {
          literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator);

          JERRY_ASSERT (literal_p != NULL
                        && literal_p->type == LEXER_FUNCTION_LITERAL);
          init_index = literal_p->prop.index;
        }

        dst_p = parser_encode_literal (dst_p, init_index, literal_one_byte_limit);
      }
    }

    JERRY_ASSERT (argument_count == context_p->argument_count);
  }

  parser_list_iterator_init (&context_p->literal_pool, &literal_iterator);
  argument_count = 0;

  while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)))
  {
    const uint8_t expected_status_flags = LEXER_FLAG_VAR | LEXER_FLAG_INITIALIZED;

    if (literal_p->type != LEXER_UNUSED_LITERAL)
    {
      if (literal_p->type == LEXER_IDENT_LITERAL
          || literal_p->type == LEXER_STRING_LITERAL)
      {
#ifdef PARSER_DUMP_BYTE_CODE
        jmem_cpointer_t lit_cp = ecma_find_or_create_literal_string (literal_p->u.char_p,
                                                                     literal_p->prop.length);
        literal_pool_p[literal_p->prop.index] = lit_cp;

        if (!context_p->is_show_opcodes
            && !(literal_p->status_flags & LEXER_FLAG_SOURCE_PTR))
        {
          jmem_heap_free_block ((void *) literal_p->u.char_p, literal_p->prop.length);
        }
#else /* !PARSER_DUMP_BYTE_CODE */
        literal_pool_p[literal_p->prop.index] = literal_p->u.value;
#endif /* PARSER_DUMP_BYTE_CODE */
      }
      else if ((literal_p->type == LEXER_FUNCTION_LITERAL)
               || (literal_p->type == LEXER_REGEXP_LITERAL))
      {
        ECMA_SET_NON_NULL_POINTER (literal_pool_p[literal_p->prop.index],
                                   literal_p->u.bytecode_p);
      }
      else
      {
        JERRY_ASSERT (literal_p->type == LEXER_NUMBER_LITERAL);
        literal_pool_p[literal_p->prop.index] = literal_p->u.value;
      }
    }

    if (literal_p->status_flags & LEXER_FLAG_FUNCTION_ARGUMENT)
    {
      argument_count++;
    }

    if ((literal_p->status_flags & expected_status_flags) == expected_status_flags)
    {
      uint16_t index = literal_p->prop.index;
      uint16_t init_index;

      JERRY_ASSERT (literal_p->type == LEXER_IDENT_LITERAL);

      context_p->status_flags |= PARSER_LEXICAL_ENV_NEEDED;

      if (literal_p->status_flags & LEXER_FLAG_FUNCTION_NAME)
      {
        init_index = const_literal_end;
      }
      else if (literal_p->status_flags & LEXER_FLAG_FUNCTION_ARGUMENT)
      {
        init_index = (uint16_t) (argument_count - 1);

        if (init_index == literal_p->prop.index)
        {
          continue;
        }
      }
      else
      {
        literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator);

        JERRY_ASSERT (literal_p != NULL
                      && literal_p->type == LEXER_FUNCTION_LITERAL);
        init_index = literal_p->prop.index;
        ECMA_SET_NON_NULL_POINTER (literal_pool_p[literal_p->prop.index],
                                   literal_p->u.bytecode_p);
      }

      *dst_p++ = CBC_INITIALIZE_VAR;
      dst_p = parser_encode_literal (dst_p, index, literal_one_byte_limit);
      dst_p = parser_encode_literal (dst_p, init_index, literal_one_byte_limit);
    }
  }

  JERRY_ASSERT (argument_count == context_p->argument_count);
  return dst_p;
} /* parser_generate_initializers */

/*
 * During byte code post processing certain bytes are not
 * copied into the final byte code buffer. For example, if
 * one byte is enough for encoding a literal index, the
 * second byte is not copied. However, when a byte is skipped,
 * the offsets of those branches which crosses (jumps over)
 * that byte code should also be decreased by one. Instead
 * of finding these jumps every time when a byte is skipped,
 * all branch offset updates are computed in one step.
 *
 * Branch offset mapping example:
 *
 * Let's assume that each parser_mem_page of the byte_code
 * buffer is 8 bytes long and only 4 bytes are kept for a
 * given page:
 *
 * +---+---+---+---+---+---+---+---+
 * | X | 1 | 2 | 3 | X | 4 | X | X |
 * +---+---+---+---+---+---+---+---+
 *
 * X marks those bytes which are removed. The resulting
 * offset mapping is the following:
 *
 * +---+---+---+---+---+---+---+---+
 * | 0 | 1 | 2 | 3 | 3 | 4 | 4 | 4 |
 * +---+---+---+---+---+---+---+---+
 *
 * Each X is simply replaced by the index of the previous
 * index starting from zero. This shows the number of
 * copied bytes before a given byte including the byte
 * itself. The last byte always shows the number of bytes
 * copied from this page.
 *
 * This mapping allows recomputing all branch targets,
 * since mapping[to] - mapping[from] is the new argument
 * for forward branches. As for backward branches, the
 * equation is reversed to mapping[from] - mapping[to].
 *
 * The mapping is relative to one page, so distance
 * computation affecting multiple pages requires a loop.
 * We should also note that only argument bytes can
 * be skipped, so removed bytes cannot be targeted by
 * branches. Valid branches always target instruction
 * starts only.
 */

/**
 * Recompute the argument of a forward branch.
 *
 * @return the new distance
 */
static size_t
parser_update_forward_branch (parser_mem_page_t *page_p, /**< current page */
                              size_t full_distance, /**< full distance */
                              uint8_t bytes_copied_before_jump) /**< bytes copied before jump */
{
  size_t new_distance = 0;

  while (full_distance > PARSER_CBC_STREAM_PAGE_SIZE)
  {
    new_distance += page_p->bytes[PARSER_CBC_STREAM_PAGE_SIZE - 1] & CBC_LOWER_SEVEN_BIT_MASK;
    full_distance -= PARSER_CBC_STREAM_PAGE_SIZE;
    page_p = page_p->next_p;
  }

  new_distance += page_p->bytes[full_distance - 1] & CBC_LOWER_SEVEN_BIT_MASK;
  return new_distance - bytes_copied_before_jump;
} /* parser_update_forward_branch */

/**
 * Recompute the argument of a backward branch.
 *
 * @return the new distance
 */
static size_t
parser_update_backward_branch (parser_mem_page_t *page_p, /**< current page */
                               size_t full_distance, /**< full distance */
                               uint8_t bytes_copied_before_jump) /**< bytes copied before jump */
{
  size_t new_distance = bytes_copied_before_jump;

  while (full_distance >= PARSER_CBC_STREAM_PAGE_SIZE)
  {
    JERRY_ASSERT (page_p != NULL);
    new_distance += page_p->bytes[PARSER_CBC_STREAM_PAGE_SIZE - 1] & CBC_LOWER_SEVEN_BIT_MASK;
    full_distance -= PARSER_CBC_STREAM_PAGE_SIZE;
    page_p = page_p->next_p;
  }

  if (full_distance > 0)
  {
    size_t offset = PARSER_CBC_STREAM_PAGE_SIZE - full_distance;

    JERRY_ASSERT (page_p != NULL);

    new_distance += page_p->bytes[PARSER_CBC_STREAM_PAGE_SIZE - 1] & CBC_LOWER_SEVEN_BIT_MASK;
    new_distance -= page_p->bytes[offset - 1] & CBC_LOWER_SEVEN_BIT_MASK;
  }

  return new_distance;
} /* parser_update_backward_branch */

/**
 * Update targets of all branches in one step.
 */
static void
parse_update_branches (parser_context_t *context_p, /**< context */
                       uint8_t *byte_code_p) /**< byte code */
{
  parser_mem_page_t *page_p = context_p->byte_code.first_p;
  parser_mem_page_t *prev_page_p = NULL;
  parser_mem_page_t *last_page_p = context_p->byte_code.last_p;
  size_t last_position = context_p->byte_code.last_position;
  size_t offset = 0;
  size_t bytes_copied = 0;

  if (last_position >= PARSER_CBC_STREAM_PAGE_SIZE)
  {
    last_page_p = NULL;
    last_position = 0;
  }

  while (page_p != last_page_p || offset < last_position)
  {
    /* Branch instructions are marked to improve search speed. */
    if (page_p->bytes[offset] & CBC_HIGHEST_BIT_MASK)
    {
      uint8_t *bytes_p = byte_code_p + bytes_copied;
      uint8_t flags;
      uint8_t bytes_copied_before_jump = 0;
      size_t branch_argument_length;
      size_t target_distance;
      size_t length;

      if (offset > 0)
      {
        bytes_copied_before_jump = page_p->bytes[offset - 1] & CBC_LOWER_SEVEN_BIT_MASK;
      }
      bytes_p += bytes_copied_before_jump;

      if (*bytes_p == CBC_EXT_OPCODE)
      {
        bytes_p++;
        flags = cbc_ext_flags[*bytes_p];
      }
      else
      {
        flags = cbc_flags[*bytes_p];
      }

      JERRY_ASSERT (flags & CBC_HAS_BRANCH_ARG);
      branch_argument_length = CBC_BRANCH_OFFSET_LENGTH (*bytes_p);
      bytes_p++;

      /* Decoding target. */
      length = branch_argument_length;
      target_distance = 0;
      do
      {
        target_distance = (target_distance << 8) | *bytes_p;
        bytes_p++;
      }
      while (--length > 0);

      if (CBC_BRANCH_IS_FORWARD (flags))
      {
        /* Branch target was not set. */
        JERRY_ASSERT (target_distance > 0);

        target_distance = parser_update_forward_branch (page_p,
                                                        offset + target_distance,
                                                        bytes_copied_before_jump);
      }
      else
      {
        if (target_distance < offset)
        {
          uint8_t bytes_copied_before_target = page_p->bytes[offset - target_distance - 1];
          bytes_copied_before_target = bytes_copied_before_target & CBC_LOWER_SEVEN_BIT_MASK;

          target_distance = (size_t) (bytes_copied_before_jump - bytes_copied_before_target);
        }
        else if (target_distance == offset)
        {
          target_distance = bytes_copied_before_jump;
        }
        else
        {
          target_distance = parser_update_backward_branch (prev_page_p,
                                                           target_distance - offset,
                                                           bytes_copied_before_jump);
        }
      }

      /* Encoding target again. */
      do
      {
        bytes_p--;
        *bytes_p = (uint8_t) (target_distance & 0xff);
        target_distance >>= 8;
      }
      while (--branch_argument_length > 0);
    }

    offset++;
    if (offset >= PARSER_CBC_STREAM_PAGE_SIZE)
    {
      parser_mem_page_t *next_p = page_p->next_p;

      /* We reverse the pages before the current page. */
      page_p->next_p = prev_page_p;
      prev_page_p = page_p;

      bytes_copied += page_p->bytes[PARSER_CBC_STREAM_PAGE_SIZE - 1] & CBC_LOWER_SEVEN_BIT_MASK;
      page_p = next_p;
      offset = 0;
    }
  }

  /* After this point the pages of the byte code stream are
   * not used anymore. However, they needs to be freed during
   * cleanup, so the first and last pointers of the stream
   * descriptor are reversed as well. */
  if (last_page_p != NULL)
  {
    JERRY_ASSERT (last_page_p == context_p->byte_code.last_p);
    last_page_p->next_p = prev_page_p;
  }
  else
  {
    last_page_p = context_p->byte_code.last_p;
  }

  context_p->byte_code.last_p = context_p->byte_code.first_p;
  context_p->byte_code.first_p = last_page_p;
} /* parse_update_branches */

#ifdef PARSER_DUMP_BYTE_CODE

/**
 * Print literal.
 */
static void
parse_print_literal (ecma_compiled_code_t *compiled_code_p, /**< compiled code */
                     uint16_t literal_index, /**< literal index */
                     parser_list_t *literal_pool_p) /**< literal pool */
{
  parser_list_iterator_t literal_iterator;
  uint16_t argument_end;
  uint16_t register_end;
  uint16_t ident_end;
  uint16_t const_literal_end;

  if (compiled_code_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) compiled_code_p;
    argument_end = args_p->argument_end;
    register_end = args_p->register_end;
    ident_end = args_p->ident_end;
    const_literal_end = args_p->const_literal_end;
  }
  else
  {
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) compiled_code_p;
    argument_end = args_p->argument_end;
    register_end = args_p->register_end;
    ident_end = args_p->ident_end;
    const_literal_end = args_p->const_literal_end;
  }

  parser_list_iterator_init (literal_pool_p, &literal_iterator);

  while (true)
  {
    lexer_literal_t *literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator);

    if (literal_p == NULL)
    {

      if (literal_index == const_literal_end)
      {
        JERRY_DEBUG_MSG (" idx:%d(self)->function", literal_index);
        break;
      }

      JERRY_ASSERT (literal_index < argument_end);
      JERRY_DEBUG_MSG (" idx:%d(arg)->undefined", literal_index);
      break;
    }

    if (literal_p->prop.index == literal_index
        && literal_p->type != LEXER_UNUSED_LITERAL)
    {
      JERRY_DEBUG_MSG (" idx:%d", literal_index);

      if (literal_index < argument_end)
      {
        JERRY_DEBUG_MSG ("(arg)->");
      }
      else if (literal_index < register_end)
      {
        JERRY_DEBUG_MSG ("(reg)->");
      }
      else if (literal_index < ident_end)
      {
        JERRY_DEBUG_MSG ("(ident)->");
      }
      else
      {
        JERRY_DEBUG_MSG ("(lit)->");
      }

      util_print_literal (literal_p);
      return;
    }
  }
} /* parse_print_literal */

#define PARSER_READ_IDENTIFIER_INDEX(name) \
  name = *byte_code_p++; \
  if (name >= encoding_limit) \
  { \
    name = (uint16_t) (((name << 8) | byte_code_p[0]) - encoding_delta); \
    byte_code_p++; \
  }

/**
 * Print CBC_DEFINE_VARS instruction.
 *
 * @return next byte code position
 */
static uint8_t *
parse_print_define_vars (ecma_compiled_code_t *compiled_code_p, /**< compiled code */
                         uint8_t *byte_code_p, /**< byte code position */
                         uint16_t encoding_limit, /**< literal encoding limit */
                         uint16_t encoding_delta, /**< literal encoding delta */
                         parser_list_t *literal_pool_p) /**< literal pool */
{
  uint16_t identifier_index;
  uint16_t identifier_end;

  if (compiled_code_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) compiled_code_p;
    identifier_index = args_p->register_end;
  }
  else
  {
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) compiled_code_p;
    identifier_index = args_p->register_end;
  }

  PARSER_READ_IDENTIFIER_INDEX (identifier_end);

  JERRY_DEBUG_MSG (" from: %d to: %d\n", identifier_index, identifier_end);

  while (identifier_index <= identifier_end)
  {
    JERRY_DEBUG_MSG ("        ");
    parse_print_literal (compiled_code_p, identifier_index, literal_pool_p);
    identifier_index++;
    JERRY_DEBUG_MSG ("\n");
  }

  return byte_code_p;
} /* parse_print_define_vars */

/**
 * Print CBC_INITIALIZE_VARS instruction.
 *
 * @return next byte code position
 */
static uint8_t *
parse_print_initialize_vars (ecma_compiled_code_t *compiled_code_p, /**< compiled code */
                             uint8_t *byte_code_p, /**< byte code position */
                             uint16_t encoding_limit, /**< literal encoding limit */
                             uint16_t encoding_delta, /**< literal encoding delta */
                             parser_list_t *literal_pool_p) /**< literal pool */
{
  uint16_t identifier_index;
  uint16_t identifier_end;

  PARSER_READ_IDENTIFIER_INDEX (identifier_index);
  PARSER_READ_IDENTIFIER_INDEX (identifier_end);

  JERRY_DEBUG_MSG (" from: %d to: %d\n", identifier_index, identifier_end);

  while (identifier_index <= identifier_end)
  {
    uint16_t literal_index;

    JERRY_DEBUG_MSG ("        ");
    parse_print_literal (compiled_code_p, identifier_index, literal_pool_p);
    JERRY_DEBUG_MSG (" =");

    PARSER_READ_IDENTIFIER_INDEX (literal_index);

    parse_print_literal (compiled_code_p, literal_index, literal_pool_p);
    identifier_index++;
    JERRY_DEBUG_MSG ("\n");
  }

  return byte_code_p;
} /* parse_print_initialize_vars */

/**
 * Print byte code.
 */
static void
parse_print_final_cbc (ecma_compiled_code_t *compiled_code_p, /**< compiled code */
                       parser_list_t *literal_pool_p, /**< literal pool */
                       size_t length) /**< length of byte code */
{
  uint8_t flags;
  uint8_t *byte_code_start_p;
  uint8_t *byte_code_end_p;
  uint8_t *byte_code_p;
  uint16_t encoding_limit;
  uint16_t encoding_delta;
  uint16_t stack_limit;
  uint16_t argument_end;
  uint16_t register_end;
  uint16_t ident_end;
  uint16_t const_literal_end;
  uint16_t literal_end;

  if (compiled_code_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    cbc_uint16_arguments_t *args = (cbc_uint16_arguments_t *) compiled_code_p;
    stack_limit = args->stack_limit;
    argument_end = args->argument_end;
    register_end = args->register_end;
    ident_end = args->ident_end;
    const_literal_end = args->const_literal_end;
    literal_end = args->literal_end;
  }
  else
  {
    cbc_uint8_arguments_t *args = (cbc_uint8_arguments_t *) compiled_code_p;
    stack_limit = args->stack_limit;
    argument_end = args->argument_end;
    register_end = args->register_end;
    ident_end = args->ident_end;
    const_literal_end = args->const_literal_end;
    literal_end = args->literal_end;
  }

  JERRY_DEBUG_MSG ("\nFinal byte code dump:\n\n  Maximum stack depth: %d\n  Flags: [",
                   (int) (stack_limit + register_end));

  if (!(compiled_code_p->status_flags & CBC_CODE_FLAGS_FULL_LITERAL_ENCODING))
  {
    JERRY_DEBUG_MSG ("small_lit_enc");
    encoding_limit = 255;
    encoding_delta = 0xfe01;
  }
  else
  {
    JERRY_DEBUG_MSG ("full_lit_enc");
    encoding_limit = 128;
    encoding_delta = 0x8000;
  }

  if (compiled_code_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    JERRY_DEBUG_MSG (",uint16_arguments");
  }

  if (compiled_code_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE)
  {
    JERRY_DEBUG_MSG (",strict_mode");
  }

  if (compiled_code_p->status_flags & CBC_CODE_FLAGS_ARGUMENTS_NEEDED)
  {
    JERRY_DEBUG_MSG (",arguments_needed");
  }

  if (compiled_code_p->status_flags & CBC_CODE_FLAGS_LEXICAL_ENV_NOT_NEEDED)
  {
    JERRY_DEBUG_MSG (",no_lexical_env");
  }

  JERRY_DEBUG_MSG ("]\n");

  JERRY_DEBUG_MSG ("  Argument range end: %d\n", (int) argument_end);
  JERRY_DEBUG_MSG ("  Register range end: %d\n", (int) register_end);
  JERRY_DEBUG_MSG ("  Identifier range end: %d\n", (int) ident_end);
  JERRY_DEBUG_MSG ("  Const literal range end: %d\n", (int) const_literal_end);
  JERRY_DEBUG_MSG ("  Literal range end: %d\n\n", (int) literal_end);

  byte_code_start_p = (uint8_t *) compiled_code_p;

  if (compiled_code_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    byte_code_start_p += sizeof (cbc_uint16_arguments_t);
  }
  else
  {
    byte_code_start_p += sizeof (cbc_uint8_arguments_t);
  }

  byte_code_start_p += literal_end * sizeof (jmem_cpointer_t);
  byte_code_end_p = byte_code_start_p + length;
  byte_code_p = byte_code_start_p;

  while (byte_code_p < byte_code_end_p)
  {
    cbc_opcode_t opcode;
    cbc_ext_opcode_t ext_opcode;
    size_t cbc_offset;

    opcode = (cbc_opcode_t) *byte_code_p;
    ext_opcode = CBC_EXT_NOP;
    cbc_offset = (size_t) (byte_code_p - byte_code_start_p);

    if (opcode != CBC_EXT_OPCODE)
    {
      flags = cbc_flags[opcode];
      JERRY_DEBUG_MSG (" %3d : %s", (int) cbc_offset, cbc_names[opcode]);
      byte_code_p++;

      if (opcode == CBC_INITIALIZE_VARS)
      {
        byte_code_p = parse_print_initialize_vars (compiled_code_p,
                                                   byte_code_p,
                                                   encoding_limit,
                                                   encoding_delta,
                                                   literal_pool_p);
        continue;
      }

      if (opcode == CBC_DEFINE_VARS)
      {
        byte_code_p = parse_print_define_vars (compiled_code_p,
                                               byte_code_p,
                                               encoding_limit,
                                               encoding_delta,
                                               literal_pool_p);
        continue;
      }

      if (opcode == CBC_PUSH_NUMBER_POS_BYTE)
      {
        int value = *byte_code_p++;
        JERRY_DEBUG_MSG (" number:%d\n", value + 1);
        continue;
      }

      if (opcode == CBC_PUSH_NUMBER_NEG_BYTE)
      {
        int value = *byte_code_p++;
        JERRY_DEBUG_MSG (" number:%d\n", -(value + 1));
        continue;
      }
    }
    else
    {
      ext_opcode = (cbc_ext_opcode_t) byte_code_p[1];
      flags = cbc_ext_flags[ext_opcode];
      JERRY_DEBUG_MSG (" %3d : %s", (int) cbc_offset, cbc_ext_names[ext_opcode]);
      byte_code_p += 2;
    }

    if (flags & (CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2))
    {
      uint16_t literal_index;

      PARSER_READ_IDENTIFIER_INDEX (literal_index);
      parse_print_literal (compiled_code_p, literal_index, literal_pool_p);
    }

    if (flags & CBC_HAS_LITERAL_ARG2)
    {
      uint16_t literal_index;

      PARSER_READ_IDENTIFIER_INDEX (literal_index);
      parse_print_literal (compiled_code_p, literal_index, literal_pool_p);

      if (!(flags & CBC_HAS_LITERAL_ARG))
      {
        PARSER_READ_IDENTIFIER_INDEX (literal_index);
        parse_print_literal (compiled_code_p, literal_index, literal_pool_p);
      }
    }

    if (flags & CBC_HAS_BYTE_ARG)
    {
      JERRY_DEBUG_MSG (" byte_arg:%d", *byte_code_p);
      byte_code_p++;
    }

    if (flags & CBC_HAS_BRANCH_ARG)
    {
      size_t branch_offset_length = CBC_BRANCH_OFFSET_LENGTH (opcode);
      size_t offset = 0;

      if (opcode == CBC_EXT_OPCODE)
      {
        branch_offset_length = CBC_BRANCH_OFFSET_LENGTH (ext_opcode);
      }

      do
      {
        offset = (offset << 8) | *byte_code_p++;
      }
      while (--branch_offset_length > 0);

      if (CBC_BRANCH_IS_FORWARD (flags))
      {
        JERRY_DEBUG_MSG (" offset:%d(->%d)", (int) offset, (int) (cbc_offset + offset));
      }
      else
      {
        JERRY_DEBUG_MSG (" offset:%d(->%d)", (int) offset, (int) (cbc_offset - offset));
      }
    }

    JERRY_DEBUG_MSG ("\n");
  }
} /* parse_print_final_cbc */

#undef PARSER_READ_IDENTIFIER_INDEX

#endif /* PARSER_DUMP_BYTE_CODE */

#define PARSER_NEXT_BYTE(page_p, offset) \
  do { \
    if (++(offset) >= PARSER_CBC_STREAM_PAGE_SIZE) \
    { \
      offset = 0; \
      page_p = page_p->next_p; \
    } \
  } while (0)

#define PARSER_NEXT_BYTE_UPDATE(page_p, offset, real_offset) \
  do { \
    page_p->bytes[offset] = real_offset; \
    if (++(offset) >= PARSER_CBC_STREAM_PAGE_SIZE) \
    { \
      offset = 0; \
      real_offset = 0; \
      page_p = page_p->next_p; \
    } \
  } while (0)

/**
 * Post processing main function.
 *
 * @return compiled code
 */
static ecma_compiled_code_t *
parser_post_processing (parser_context_t *context_p) /**< context */
{
  uint16_t literal_one_byte_limit;
  uint16_t ident_end;
  uint16_t uninitialized_var_end;
  uint16_t initialized_var_end;
  uint16_t const_literal_end;
  parser_mem_page_t *page_p;
  parser_mem_page_t *last_page_p = context_p->byte_code.last_p;
  size_t last_position = context_p->byte_code.last_position;
  size_t offset;
  size_t length;
  size_t total_size;
  size_t initializers_length;
  uint8_t real_offset;
  uint8_t *byte_code_p;
  bool needs_uint16_arguments;
  cbc_opcode_t last_opcode = CBC_EXT_OPCODE;
  ecma_compiled_code_t *compiled_code_p;
  jmem_cpointer_t *literal_pool_p;
  uint8_t *dst_p;

  if ((size_t) context_p->stack_limit + (size_t) context_p->register_count > PARSER_MAXIMUM_STACK_LIMIT)
  {
    parser_raise_error (context_p, PARSER_ERR_STACK_LIMIT_REACHED);
  }

  JERRY_ASSERT (context_p->literal_count <= PARSER_MAXIMUM_NUMBER_OF_LITERALS);

  initializers_length = parser_compute_indicies (context_p,
                                                 &ident_end,
                                                 &uninitialized_var_end,
                                                 &initialized_var_end,
                                                 &const_literal_end);
  length = initializers_length;

  if (context_p->literal_count <= CBC_MAXIMUM_SMALL_VALUE)
  {
    literal_one_byte_limit = CBC_MAXIMUM_BYTE_VALUE - 1;
  }
  else
  {
    literal_one_byte_limit = CBC_LOWER_SEVEN_BIT_MASK;
  }

  if (last_position >= PARSER_CBC_STREAM_PAGE_SIZE)
  {
    last_page_p = NULL;
    last_position = 0;
  }

  page_p = context_p->byte_code.first_p;
  offset = 0;

  while (page_p != last_page_p || offset < last_position)
  {
    uint8_t *opcode_p;
    uint8_t flags;
    size_t branch_offset_length;

    opcode_p = page_p->bytes + offset;
    last_opcode = (cbc_opcode_t) (*opcode_p);
    PARSER_NEXT_BYTE (page_p, offset);
    branch_offset_length = CBC_BRANCH_OFFSET_LENGTH (last_opcode);
    flags = cbc_flags[last_opcode];
    length++;

    if (last_opcode == CBC_EXT_OPCODE)
    {
      cbc_ext_opcode_t ext_opcode;

      ext_opcode = (cbc_ext_opcode_t) page_p->bytes[offset];
      branch_offset_length = CBC_BRANCH_OFFSET_LENGTH (ext_opcode);
      flags = cbc_ext_flags[ext_opcode];
      PARSER_NEXT_BYTE (page_p, offset);
      length++;
    }

    while (flags & (CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2))
    {
      uint8_t *first_byte = page_p->bytes + offset;
      size_t literal_index = *first_byte;
      lexer_literal_t *literal_p;

      PARSER_NEXT_BYTE (page_p, offset);
      length++;

      literal_index |= ((size_t) page_p->bytes[offset]) << 8;
      literal_p = PARSER_GET_LITERAL (literal_index);

      if (literal_p->type == LEXER_UNUSED_LITERAL)
      {
        /* In a few cases uninitialized literals may have been converted to initialized
         * literals later. Byte code references to the old (uninitialized) literals
         * must be redirected to the new instance of the literal. */
        literal_p = PARSER_GET_LITERAL (literal_p->prop.index);

        JERRY_ASSERT (literal_p != NULL && literal_p->type != LEXER_UNUSED_LITERAL);
      }

      if (literal_p->prop.index <= literal_one_byte_limit)
      {
        *first_byte = (uint8_t) literal_p->prop.index;
      }
      else
      {
        if (context_p->literal_count <= CBC_MAXIMUM_SMALL_VALUE)
        {
          JERRY_ASSERT (literal_index <= CBC_MAXIMUM_SMALL_VALUE);
          *first_byte = CBC_MAXIMUM_BYTE_VALUE;
          page_p->bytes[offset] = (uint8_t) (literal_p->prop.index - CBC_MAXIMUM_BYTE_VALUE);
          length++;
        }
        else
        {
          JERRY_ASSERT (literal_index <= CBC_MAXIMUM_FULL_VALUE);
          *first_byte = (uint8_t) ((literal_p->prop.index >> 8) | CBC_HIGHEST_BIT_MASK);
          page_p->bytes[offset] = (uint8_t) (literal_p->prop.index & 0xff);
          length++;
        }
      }
      PARSER_NEXT_BYTE (page_p, offset);

      if (flags & CBC_HAS_LITERAL_ARG2)
      {
        if (flags & CBC_HAS_LITERAL_ARG)
        {
          flags = CBC_HAS_LITERAL_ARG;
        }
        else
        {
          flags = CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2;
        }
      }
      else
      {
        break;
      }
    }

    if (flags & CBC_HAS_BYTE_ARG)
    {
      /* This argument will be copied without modification. */
      PARSER_NEXT_BYTE (page_p, offset);
      length++;
    }

    if (flags & CBC_HAS_BRANCH_ARG)
    {
      bool prefix_zero = true;
#if PARSER_MAXIMUM_CODE_SIZE <= 65535
      cbc_opcode_t jump_forward = CBC_JUMP_FORWARD_2;
#else /* PARSER_MAXIMUM_CODE_SIZE > 65535 */
      cbc_opcode_t jump_forward = CBC_JUMP_FORWARD_3;
#endif /* PARSER_MAXIMUM_CODE_SIZE <= 65535 */

      /* The leading zeroes are dropped from the stream.
       * Although dropping these zeroes for backward
       * branches are unnecessary, we use the same
       * code path for simplicity. */
      JERRY_ASSERT (branch_offset_length > 0 && branch_offset_length <= 3);

      while (--branch_offset_length > 0)
      {
        uint8_t byte = page_p->bytes[offset];
        if (byte > 0 || !prefix_zero)
        {
          prefix_zero = false;
          length++;
        }
        else
        {
          JERRY_ASSERT (CBC_BRANCH_IS_FORWARD (flags));
        }
        PARSER_NEXT_BYTE (page_p, offset);
      }

      if (last_opcode == jump_forward
          && prefix_zero
          && page_p->bytes[offset] == CBC_BRANCH_OFFSET_LENGTH (jump_forward) + 1)
      {
        /* Uncoditional jumps which jump right after the instruction
         * are effectively NOPs. These jumps are removed from the
         * stream. The 1 byte long CBC_JUMP_FORWARD form marks these
         * instructions, since this form is constructed during post
         * processing and cannot be emitted directly. */
        *opcode_p = CBC_JUMP_FORWARD;
        length--;
      }
      else
      {
        /* Other last bytes are always copied. */
        length++;
      }

      PARSER_NEXT_BYTE (page_p, offset);
    }
  }

  if (!(context_p->status_flags & PARSER_NO_END_LABEL)
      || !(PARSER_OPCODE_IS_RETURN (last_opcode)))
  {
    context_p->status_flags &= ~PARSER_NO_END_LABEL;
    length++;
  }

  needs_uint16_arguments = false;
  total_size = sizeof (cbc_uint8_arguments_t);

  if (context_p->stack_limit > CBC_MAXIMUM_BYTE_VALUE
      || context_p->register_count > CBC_MAXIMUM_BYTE_VALUE
      || context_p->literal_count > CBC_MAXIMUM_BYTE_VALUE)
  {
    needs_uint16_arguments = true;
    total_size = sizeof (cbc_uint16_arguments_t);
  }

  total_size += length + context_p->literal_count * sizeof (jmem_cpointer_t);
  total_size = JERRY_ALIGNUP (total_size, JMEM_ALIGNMENT);

  compiled_code_p = (ecma_compiled_code_t *) parser_malloc (context_p, total_size);

  byte_code_p = (uint8_t *) compiled_code_p;
  compiled_code_p->size = (uint16_t) (total_size >> JMEM_ALIGNMENT_LOG);
  compiled_code_p->refs = 1;
  compiled_code_p->status_flags = CBC_CODE_FLAGS_FUNCTION;

  if (needs_uint16_arguments)
  {
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) compiled_code_p;

    args_p->stack_limit = context_p->stack_limit;
    args_p->argument_end = context_p->argument_count;
    args_p->register_end = context_p->register_count;
    args_p->ident_end = ident_end;
    args_p->const_literal_end = const_literal_end;
    args_p->literal_end = context_p->literal_count;

    compiled_code_p->status_flags |= CBC_CODE_FLAGS_UINT16_ARGUMENTS;
    byte_code_p += sizeof (cbc_uint16_arguments_t);
  }
  else
  {
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) compiled_code_p;

    args_p->stack_limit = (uint8_t) context_p->stack_limit;
    args_p->argument_end = (uint8_t) context_p->argument_count;
    args_p->register_end = (uint8_t) context_p->register_count;
    args_p->ident_end = (uint8_t) ident_end;
    args_p->const_literal_end = (uint8_t) const_literal_end;
    args_p->literal_end = (uint8_t) context_p->literal_count;

    byte_code_p += sizeof (cbc_uint8_arguments_t);
  }

  if (context_p->literal_count > CBC_MAXIMUM_SMALL_VALUE)
  {
    compiled_code_p->status_flags |= CBC_CODE_FLAGS_FULL_LITERAL_ENCODING;
  }

  if (context_p->status_flags & PARSER_IS_STRICT)
  {
    compiled_code_p->status_flags |= CBC_CODE_FLAGS_STRICT_MODE;
  }

  if (context_p->status_flags & PARSER_ARGUMENTS_NEEDED)
  {
    compiled_code_p->status_flags |= CBC_CODE_FLAGS_ARGUMENTS_NEEDED;
    /* Arguments is stored in the lexical environment. */
    context_p->status_flags |= PARSER_LEXICAL_ENV_NEEDED;
  }

  if (!(context_p->status_flags & PARSER_LEXICAL_ENV_NEEDED))
  {
    compiled_code_p->status_flags |= CBC_CODE_FLAGS_LEXICAL_ENV_NOT_NEEDED;
  }

  literal_pool_p = (jmem_cpointer_t *) byte_code_p;
  byte_code_p += context_p->literal_count * sizeof (jmem_cpointer_t);

  dst_p = parser_generate_initializers (context_p,
                                        byte_code_p,
                                        literal_pool_p,
                                        uninitialized_var_end,
                                        initialized_var_end,
                                        const_literal_end,
                                        literal_one_byte_limit);

  JERRY_ASSERT (dst_p == byte_code_p + initializers_length);

  page_p = context_p->byte_code.first_p;
  offset = 0;
  real_offset = 0;

  while (page_p != last_page_p || offset < last_position)
  {
    uint8_t flags;
    uint8_t *opcode_p;
    uint8_t *branch_mark_p;
    cbc_opcode_t opcode;
    size_t branch_offset_length;

    opcode_p = dst_p;
    branch_mark_p = page_p->bytes + offset;
    opcode = (cbc_opcode_t) (*branch_mark_p);
    branch_offset_length = CBC_BRANCH_OFFSET_LENGTH (opcode);

    if (opcode == CBC_JUMP_FORWARD)
    {
      /* These opcodes are deleted from the stream. */
#if PARSER_MAXIMUM_CODE_SIZE <= 65535
      size_t length = 3;
#else /* PARSER_MAXIMUM_CODE_SIZE > 65535 */
      size_t length = 4;
#endif /* PARSER_MAXIMUM_CODE_SIZE <= 65535 */

      do
      {
        PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);
      }
      while (--length > 0);

      continue;
    }

    /* Storing the opcode */
    *dst_p++ = opcode;
    real_offset++;
    PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);
    flags = cbc_flags[opcode];

    if (opcode == CBC_EXT_OPCODE)
    {
      cbc_ext_opcode_t ext_opcode;

      ext_opcode = (cbc_ext_opcode_t) page_p->bytes[offset];
      flags = cbc_ext_flags[ext_opcode];
      branch_offset_length = CBC_BRANCH_OFFSET_LENGTH (ext_opcode);

      /* Storing the extended opcode */
      *dst_p++ = ext_opcode;
      opcode_p++;
      real_offset++;
      PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);
    }

    if (flags & CBC_HAS_BRANCH_ARG)
    {
      *branch_mark_p |= CBC_HIGHEST_BIT_MASK;
    }

    /* Only literal and call arguments can be combined. */
    JERRY_ASSERT (!(flags & CBC_HAS_BRANCH_ARG)
                   || !(flags & (CBC_HAS_BYTE_ARG | CBC_HAS_LITERAL_ARG)));

    while (flags & (CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2))
    {
      uint8_t first_byte = page_p->bytes[offset];

      *dst_p++ = first_byte;
      real_offset++;
      PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);

      if (first_byte > literal_one_byte_limit)
      {
        *dst_p++ = page_p->bytes[offset];
        real_offset++;
      }
      PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);

      if (flags & CBC_HAS_LITERAL_ARG2)
      {
        if (flags & CBC_HAS_LITERAL_ARG)
        {
          flags = CBC_HAS_LITERAL_ARG;
        }
        else
        {
          flags = CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2;
        }
      }
      else
      {
        break;
      }
    }

    if (flags & CBC_HAS_BYTE_ARG)
    {
      /* This argument will be copied without modification. */
      *dst_p++ = page_p->bytes[offset];
      real_offset++;
      PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);
    }

    if (flags & CBC_HAS_BRANCH_ARG)
    {
      bool prefix_zero = true;

      /* The leading zeroes are dropped from the stream. */
      JERRY_ASSERT (branch_offset_length > 0 && branch_offset_length <= 3);

      while (--branch_offset_length > 0)
      {
        uint8_t byte = page_p->bytes[offset];
        if (byte > 0 || !prefix_zero)
        {
          prefix_zero = false;
          *dst_p++ = page_p->bytes[offset];
          real_offset++;
        }
        else
        {
          /* When a leading zero is dropped, the branch
           * offset length must be decreased as well. */
          (*opcode_p)--;
        }
        PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);
      }

      *dst_p++ = page_p->bytes[offset];
      real_offset++;
      PARSER_NEXT_BYTE_UPDATE (page_p, offset, real_offset);
    }
  }

  if (!(context_p->status_flags & PARSER_NO_END_LABEL))
  {
    *dst_p++ = CBC_RETURN_WITH_BLOCK;
  }
  JERRY_ASSERT (dst_p == byte_code_p + length);

  parse_update_branches (context_p,
                         byte_code_p + initializers_length);

  parser_cbc_stream_free (&context_p->byte_code);

#ifdef PARSER_DUMP_BYTE_CODE
  if (context_p->is_show_opcodes)
  {
    parser_list_iterator_t literal_iterator;
    lexer_literal_t *literal_p;

    parse_print_final_cbc (compiled_code_p, &context_p->literal_pool, length);
    JERRY_DEBUG_MSG ("\nByte code size: %d bytes\n", (int) length);
    context_p->total_byte_code_size += (uint32_t) length;

    parser_list_iterator_init (&context_p->literal_pool, &literal_iterator);
    while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)))
    {
      if ((literal_p->type == LEXER_IDENT_LITERAL || literal_p->type == LEXER_STRING_LITERAL)
          && !(literal_p->status_flags & LEXER_FLAG_SOURCE_PTR))
      {
        jmem_heap_free_block ((void *) literal_p->u.char_p, literal_p->prop.length);
      }
    }
  }
#else /* !PARSER_DUMP_BYTE_CODE */
  if (context_p->status_flags & PARSER_HAS_LATE_LIT_INIT)
  {
    parser_list_iterator_t literal_iterator;
    lexer_literal_t *literal_p;

    parser_list_iterator_init (&context_p->literal_pool, &literal_iterator);
    while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)))
    {
      if (literal_p->status_flags & LEXER_FLAG_LATE_INIT)
      {
        uint32_t source_data = literal_p->u.source_data;
        const uint8_t *char_p = context_p->source_end_p - (source_data & 0xfffff);
        jmem_cpointer_t lit_cp = ecma_find_or_create_literal_string (char_p,
                                                                     source_data >> 20);
        literal_pool_p[literal_p->prop.index] = lit_cp;
      }
    }
  }
#endif /* PARSER_DUMP_BYTE_CODE */

  if ((context_p->status_flags & PARSER_ARGUMENTS_NEEDED)
      && !(context_p->status_flags & PARSER_IS_STRICT))
  {
    parser_list_iterator_t literal_iterator;
    uint16_t argument_count = 0;

    parser_list_iterator_init (&context_p->literal_pool, &literal_iterator);
    while (argument_count < context_p->argument_count)
    {
      lexer_literal_t *literal_p;
      literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator);

      JERRY_ASSERT (literal_p != NULL);

      if (!(literal_p->status_flags & LEXER_FLAG_FUNCTION_ARGUMENT))
      {
        continue;
      }

      /* All arguments must be moved to initialized registers. */
      if (literal_p->type == LEXER_UNUSED_LITERAL)
      {
        if (literal_p->u.char_p == NULL)
        {
          literal_pool_p[argument_count] = JMEM_CP_NULL;
          argument_count++;
          continue;
        }

        literal_p = PARSER_GET_LITERAL (literal_p->prop.index);

        JERRY_ASSERT (literal_p != NULL);
      }

      JERRY_ASSERT (literal_p->type == LEXER_IDENT_LITERAL
                    && (literal_p->status_flags & LEXER_FLAG_VAR));

      JERRY_ASSERT (argument_count < literal_p->prop.index);

      literal_pool_p[argument_count] = literal_pool_p[literal_p->prop.index];
      argument_count++;
    }
  }

  if (context_p->status_flags & PARSER_NAMED_FUNCTION_EXP)
  {
    ECMA_SET_NON_NULL_POINTER (literal_pool_p[const_literal_end],
                               compiled_code_p);
  }

  return compiled_code_p;
} /* parser_post_processing */

#undef PARSER_NEXT_BYTE
#undef PARSER_NEXT_BYTE_UPDATE

/**
 * Free identifiers and literals.
 */
static void
parser_free_literals (parser_list_t *literal_pool_p) /**< literals */
{
  parser_list_iterator_t literal_iterator;
  lexer_literal_t *literal_p;

  parser_list_iterator_init (literal_pool_p, &literal_iterator);
  while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
  {
    util_free_literal (literal_p);
  }

  parser_list_free (literal_pool_p);
} /* parser_free_literals */

/**
 * Parse and compile EcmaScript source code
 *
 * Note: source must be a valid UTF-8 string
 *
 * @return compiled code
 */
static ecma_compiled_code_t *
parser_parse_source (const uint8_t *source_p, /**< valid UTF-8 source code */
                     size_t size, /**< size of the source code */
                     int strict_mode, /**< strict mode */
                     parser_error_location_t *error_location_p) /**< error location */
{
  parser_context_t context;
  ecma_compiled_code_t *compiled_code;

  context.error = PARSER_ERR_NO_ERROR;
  context.allocated_buffer_p = NULL;

  if (error_location_p != NULL)
  {
    error_location_p->error = PARSER_ERR_NO_ERROR;
  }

  context.status_flags = PARSER_NO_REG_STORE | PARSER_LEXICAL_ENV_NEEDED | PARSER_ARGUMENTS_NOT_NEEDED;
  context.stack_depth = 0;
  context.stack_limit = 0;
  context.last_context_p = NULL;
  context.last_statement.current_p = NULL;

  if (strict_mode)
  {
    context.status_flags |= PARSER_IS_STRICT;
  }

  context.source_p = source_p;
  context.source_end_p = source_p + size;
  context.line = 1;
  context.column = 1;

  context.last_cbc_opcode = PARSER_CBC_UNAVAILABLE;

  context.argument_count = 0;
  context.register_count = 0;
  context.literal_count = 0;

  parser_cbc_stream_init (&context.byte_code);
  context.byte_code_size = 0;
  parser_list_init (&context.literal_pool,
                    sizeof (lexer_literal_t),
                    (uint32_t) ((128 - sizeof (void *)) / sizeof (lexer_literal_t)));
  parser_stack_init (&context);

#ifndef JERRY_NDEBUG
  context.context_stack_depth = 0;
#endif /* !JERRY_NDEBUG */

#ifdef PARSER_DUMP_BYTE_CODE
  context.is_show_opcodes = (JERRY_CONTEXT (jerry_init_flags) & JERRY_INIT_SHOW_OPCODES);
  context.total_byte_code_size = 0;

  if (context.is_show_opcodes)
  {
    JERRY_DEBUG_MSG ("\n--- Script parsing start ---\n\n");
  }
#endif /* PARSER_DUMP_BYTE_CODE */

  PARSER_TRY (context.try_buffer)
  {
    /* Pushing a dummy value ensures the stack is never empty.
     * This simplifies the stack management routines. */
    parser_stack_push_uint8 (&context, CBC_MAXIMUM_BYTE_VALUE);
    /* The next token must always be present to make decisions
     * in the parser. Therefore when a token is consumed, the
     * lexer_next_token() must be immediately called. */
    lexer_next_token (&context);

    parser_parse_statements (&context);

    /* When the parsing is successful, only the
     * dummy value can be remained on the stack. */
    JERRY_ASSERT (context.stack_top_uint8 == CBC_MAXIMUM_BYTE_VALUE
                  && context.stack.last_position == 1
                  && context.stack.first_p != NULL
                  && context.stack.first_p->next_p == NULL
                  && context.stack.last_p == NULL);
    JERRY_ASSERT (context.last_statement.current_p == NULL);

    JERRY_ASSERT (context.last_cbc_opcode == PARSER_CBC_UNAVAILABLE);
    JERRY_ASSERT (context.allocated_buffer_p == NULL);

    compiled_code = parser_post_processing (&context);
    parser_list_free (&context.literal_pool);

#ifdef PARSER_DUMP_BYTE_CODE
    if (context.is_show_opcodes)
    {
      JERRY_DEBUG_MSG ("\nScript parsing successfully completed. Total byte code size: %d bytes\n",
                       (int) context.total_byte_code_size);
    }
#endif /* PARSER_DUMP_BYTE_CODE */
  }
  PARSER_CATCH
  {
    if (context.last_statement.current_p != NULL)
    {
      parser_free_jumps (context.last_statement);
    }

    if (context.allocated_buffer_p != NULL)
    {
      parser_free_local (context.allocated_buffer_p,
                         context.allocated_buffer_size);
    }

    if (error_location_p != NULL)
    {
      error_location_p->error = context.error;
      error_location_p->line = context.token.line;
      error_location_p->column = context.token.column;
    }

    compiled_code = NULL;
    parser_free_literals (&context.literal_pool);
    parser_cbc_stream_free (&context.byte_code);
  }
  PARSER_TRY_END

#ifdef PARSER_DUMP_BYTE_CODE
  if (context.is_show_opcodes)
  {
    JERRY_DEBUG_MSG ("\n--- Script parsing end ---\n\n");
  }
#endif /* PARSER_DUMP_BYTE_CODE */

  parser_stack_free (&context);

  return compiled_code;
} /* parser_parse_source */

/**
 * Parse function code
 *
 * @return compiled code
 */
ecma_compiled_code_t *
parser_parse_function (parser_context_t *context_p, /**< context */
                       uint32_t status_flags) /**< extra status flags */
{
  parser_saved_context_t saved_context;
  ecma_compiled_code_t *compiled_code_p;

  JERRY_ASSERT (context_p->last_cbc_opcode == PARSER_CBC_UNAVAILABLE);

  /* Save private part of the context. */

  saved_context.status_flags = context_p->status_flags;
  saved_context.stack_depth = context_p->stack_depth;
  saved_context.stack_limit = context_p->stack_limit;
  saved_context.prev_context_p = context_p->last_context_p;
  saved_context.last_statement = context_p->last_statement;

  saved_context.argument_count = context_p->argument_count;
  saved_context.register_count = context_p->register_count;
  saved_context.literal_count = context_p->literal_count;

  saved_context.byte_code = context_p->byte_code;
  saved_context.byte_code_size = context_p->byte_code_size;
  saved_context.literal_pool_data = context_p->literal_pool.data;

#ifndef JERRY_NDEBUG
  saved_context.context_stack_depth = context_p->context_stack_depth;
#endif /* !JERRY_NDEBUG */

  /* Reset private part of the context. */

  JERRY_ASSERT (status_flags & PARSER_IS_FUNCTION);

  context_p->status_flags &= PARSER_IS_STRICT;
  context_p->status_flags |= status_flags;
  context_p->stack_depth = 0;
  context_p->stack_limit = 0;
  context_p->last_context_p = &saved_context;
  context_p->last_statement.current_p = NULL;

  context_p->argument_count = 0;
  context_p->register_count = 0;
  context_p->literal_count = 0;

  parser_cbc_stream_init (&context_p->byte_code);
  context_p->byte_code_size = 0;
  parser_list_reset (&context_p->literal_pool);

#ifndef JERRY_NDEBUG
  context_p->context_stack_depth = 0;
#endif /* !JERRY_NDEBUG */

#ifdef PARSER_DUMP_BYTE_CODE
  if (context_p->is_show_opcodes)
  {
    JERRY_DEBUG_MSG ("\n--- Function parsing start ---\n\n");
  }
#endif /* PARSER_DUMP_BYTE_CODE */

  lexer_next_token (context_p);

  if (context_p->status_flags & PARSER_IS_FUNC_EXPRESSION
      && context_p->token.type == LEXER_LITERAL
      && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
  {
    lexer_construct_literal_object (context_p,
                                    &context_p->token.lit_location,
                                    LEXER_IDENT_LITERAL);

    /* The arguments object is created later than the binding to the
     * function expression name, so there is no need to assign special flags. */
    if (context_p->lit_object.type != LEXER_LITERAL_OBJECT_ARGUMENTS)
    {
      uint8_t status_flags = LEXER_FLAG_VAR | LEXER_FLAG_INITIALIZED | LEXER_FLAG_FUNCTION_NAME;
      context_p->lit_object.literal_p->status_flags |= status_flags;
    }

    if (context_p->token.literal_is_reserved
        || context_p->lit_object.type != LEXER_LITERAL_OBJECT_ANY)
    {
      context_p->status_flags |= PARSER_HAS_NON_STRICT_ARG;
    }

    lexer_next_token (context_p);
  }

  if (context_p->token.type != LEXER_LEFT_PAREN)
  {
    parser_raise_error (context_p, PARSER_ERR_ARGUMENT_LIST_EXPECTED);
  }

  lexer_next_token (context_p);

  /* Argument parsing. */
  if (context_p->token.type != LEXER_RIGHT_PAREN)
  {
    while (true)
    {
      uint16_t literal_count = context_p->literal_count;

      if (context_p->token.type != LEXER_LITERAL
          || context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
      {
        parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
      }

      lexer_construct_literal_object (context_p,
                                      &context_p->token.lit_location,
                                      LEXER_IDENT_LITERAL);

      if (literal_count == context_p->literal_count
          || context_p->token.literal_is_reserved
          || context_p->lit_object.type != LEXER_LITERAL_OBJECT_ANY)
      {
        context_p->status_flags |= PARSER_HAS_NON_STRICT_ARG;
      }

      if (context_p->lit_object.type == LEXER_LITERAL_OBJECT_ARGUMENTS)
      {
        uint8_t literal_status_flags = context_p->lit_object.literal_p->status_flags;

        literal_status_flags = (uint8_t) (literal_status_flags & ~LEXER_FLAG_NO_REG_STORE);
        context_p->lit_object.literal_p->status_flags = literal_status_flags;

        context_p->status_flags |= PARSER_ARGUMENTS_NOT_NEEDED;
        context_p->status_flags &= ~(PARSER_LEXICAL_ENV_NEEDED | PARSER_ARGUMENTS_NEEDED);
      }

      if (context_p->literal_count == literal_count)
      {
        lexer_literal_t *literal_p;

        if (context_p->literal_count >= PARSER_MAXIMUM_NUMBER_OF_LITERALS)
        {
          parser_raise_error (context_p, PARSER_ERR_LITERAL_LIMIT_REACHED);
        }

        literal_p = (lexer_literal_t *) parser_list_append (context_p, &context_p->literal_pool);
        *literal_p = *context_p->lit_object.literal_p;

        literal_p->status_flags &= LEXER_FLAG_SOURCE_PTR;
        literal_p->status_flags |= LEXER_FLAG_VAR | LEXER_FLAG_INITIALIZED | LEXER_FLAG_FUNCTION_ARGUMENT;

        context_p->literal_count++;

        /* There cannot be references from the byte code to these literals
         * since no byte code has been emitted yet. Therefore there is no
         * need to set the index field. */
        context_p->lit_object.literal_p->type = LEXER_UNUSED_LITERAL;

        /* Only the LEXER_FLAG_FUNCTION_ARGUMENT flag is kept. */
        context_p->lit_object.literal_p->status_flags &= LEXER_FLAG_FUNCTION_ARGUMENT;
        context_p->lit_object.literal_p->u.char_p = NULL;
      }
      else
      {
        uint8_t status_flags = LEXER_FLAG_VAR | LEXER_FLAG_INITIALIZED | LEXER_FLAG_FUNCTION_ARGUMENT;
        context_p->lit_object.literal_p->status_flags |= status_flags;
      }

      context_p->argument_count++;
      if (context_p->argument_count >= PARSER_MAXIMUM_NUMBER_OF_REGISTERS)
      {
        parser_raise_error (context_p, PARSER_ERR_REGISTER_LIMIT_REACHED);
      }

      lexer_next_token (context_p);

      if (context_p->token.type != LEXER_COMMA)
      {
        break;
      }

      lexer_next_token (context_p);
    }
  }

  if (context_p->token.type != LEXER_RIGHT_PAREN)
  {
    parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
  }

  lexer_next_token (context_p);

  context_p->register_count = context_p->argument_count;

  if ((context_p->status_flags & PARSER_IS_PROPERTY_GETTER)
      && context_p->argument_count != 0)
  {
    parser_raise_error (context_p, PARSER_ERR_NO_ARGUMENTS_EXPECTED);
  }

  if ((context_p->status_flags & PARSER_IS_PROPERTY_SETTER)
      && context_p->argument_count != 1)
  {
    parser_raise_error (context_p, PARSER_ERR_ONE_ARGUMENT_EXPECTED);
  }

#ifdef PARSER_DUMP_BYTE_CODE
  if (context_p->is_show_opcodes
      && (context_p->status_flags & PARSER_HAS_NON_STRICT_ARG))
  {
    JERRY_DEBUG_MSG ("  Note: legacy (non-strict) argument definition\n\n");
  }
#endif /* PARSER_DUMP_BYTE_CODE */

  if (context_p->token.type != LEXER_LEFT_BRACE)
  {
    parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_EXPECTED);
  }

  lexer_next_token (context_p);
  parser_parse_statements (context_p);
  compiled_code_p = parser_post_processing (context_p);

#ifdef PARSER_DUMP_BYTE_CODE
  if (context_p->is_show_opcodes)
  {
    JERRY_DEBUG_MSG ("\n--- Function parsing end ---\n\n");
  }
#endif /* PARSER_DUMP_BYTE_CODE */

  parser_list_free (&context_p->literal_pool);

  /* Restore private part of the context. */

  JERRY_ASSERT (context_p->last_cbc_opcode == PARSER_CBC_UNAVAILABLE);

  context_p->status_flags = saved_context.status_flags;
  context_p->stack_depth = saved_context.stack_depth;
  context_p->stack_limit = saved_context.stack_limit;
  context_p->last_context_p = saved_context.prev_context_p;
  context_p->last_statement = saved_context.last_statement;

  context_p->argument_count = saved_context.argument_count;
  context_p->register_count = saved_context.register_count;
  context_p->literal_count = saved_context.literal_count;

  context_p->byte_code = saved_context.byte_code;
  context_p->byte_code_size = saved_context.byte_code_size;
  context_p->literal_pool.data = saved_context.literal_pool_data;

#ifndef JERRY_NDEBUG
  context_p->context_stack_depth = saved_context.context_stack_depth;
#endif /* !JERRY_NDEBUG */

  return compiled_code_p;
} /* parser_parse_function */

/**
 * Raise a parse error
 */
void
parser_raise_error (parser_context_t *context_p, /**< context */
                    parser_error_t error) /**< error code */
{
  parser_saved_context_t *saved_context_p = context_p->last_context_p;

  while (saved_context_p != NULL)
  {
    parser_cbc_stream_free (&saved_context_p->byte_code);

    /* First the current literal pool is freed, and then it is replaced
     * by the literal pool coming from the saved context. Since literals
     * are not used anymore, this is a valid replacement. The last pool
     * is freed by parser_parse_source. */

    parser_free_literals (&context_p->literal_pool);
    context_p->literal_pool.data = saved_context_p->literal_pool_data;

    if (saved_context_p->last_statement.current_p != NULL)
    {
      parser_free_jumps (saved_context_p->last_statement);
    }

    saved_context_p = saved_context_p->prev_context_p;
  }

  context_p->error = error;
  PARSER_THROW (context_p->try_buffer);
  /* Should never been reached. */
  JERRY_ASSERT (0);
} /* parser_raise_error */

#define PARSE_ERR_POS_START       " [line: "
#define PARSE_ERR_POS_START_SIZE  ((uint32_t) sizeof (PARSE_ERR_POS_START) - 1)
#define PARSE_ERR_POS_MIDDLE      ", column: "
#define PARSE_ERR_POS_MIDDLE_SIZE ((uint32_t) sizeof (PARSE_ERR_POS_MIDDLE) - 1)
#define PARSE_ERR_POS_END         "]"
#define PARSE_ERR_POS_END_SIZE    ((uint32_t) sizeof (PARSE_ERR_POS_END))

/**
 * Parse EcamScript source code
 *
 * Note:
 *      returned value must be freed with ecma_free_value
 *
 * @return true - if success
 *         syntax error - otherwise
 */
ecma_value_t
parser_parse_script (const uint8_t *source_p, /**< source code */
                     size_t size, /**< size of the source code */
                     bool is_strict, /**< strict mode */
                     ecma_compiled_code_t **bytecode_data_p) /**< [out] JS bytecode */
{
  parser_error_location_t parser_error;
  *bytecode_data_p = parser_parse_source (source_p, size, is_strict, &parser_error);

  if (!*bytecode_data_p)
  {
    if (parser_error.error == PARSER_ERR_OUT_OF_MEMORY)
    {
      /* It is unlikely that memory can be allocated in an out-of-memory
       * situation. However, a simple value can still be thrown. */
      return ecma_make_error_value (ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL));
    }
#if JERRY_ENABLE_ERROR_MESSAGES
    const char *err_str_p = parser_error_to_string (parser_error.error);
    uint32_t err_str_size = lit_zt_utf8_string_size ((const lit_utf8_byte_t *) err_str_p);

    char line_str_p[ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32];
    uint32_t line_len = ecma_uint32_to_utf8_string (parser_error.line,
                                                    (lit_utf8_byte_t *) line_str_p,
                                                    ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32);

    char col_str_p[ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32];
    uint32_t col_len = ecma_uint32_to_utf8_string (parser_error.column,
                                                   (lit_utf8_byte_t *) col_str_p,
                                                   ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32);

    uint32_t msg_size = (err_str_size
                         + line_len
                         + col_len
                         + PARSE_ERR_POS_START_SIZE
                         + PARSE_ERR_POS_MIDDLE_SIZE
                         + PARSE_ERR_POS_END_SIZE);

    ecma_value_t error_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

    JMEM_DEFINE_LOCAL_ARRAY (error_msg_p, msg_size, char);
    char *err_msg_pos_p = error_msg_p;

    strncpy (err_msg_pos_p, err_str_p, err_str_size);
    err_msg_pos_p += err_str_size;

    strncpy (err_msg_pos_p, PARSE_ERR_POS_START, PARSE_ERR_POS_START_SIZE);
    err_msg_pos_p += PARSE_ERR_POS_START_SIZE;

    strncpy (err_msg_pos_p, line_str_p, line_len);
    err_msg_pos_p += line_len;

    strncpy (err_msg_pos_p, PARSE_ERR_POS_MIDDLE, PARSE_ERR_POS_MIDDLE_SIZE);
    err_msg_pos_p += PARSE_ERR_POS_MIDDLE_SIZE;

    strncpy (err_msg_pos_p, col_str_p, col_len);
    err_msg_pos_p += col_len;

    strncpy (err_msg_pos_p, PARSE_ERR_POS_END, PARSE_ERR_POS_END_SIZE);

    error_value = ecma_raise_syntax_error (error_msg_p);
    JMEM_FINALIZE_LOCAL_ARRAY (error_msg_p);

    return error_value;
#else /* !JERRY_ENABLE_ERROR_MESSAGES */
    return ecma_raise_syntax_error ("");
#endif /* JERRY_ENABLE_ERROR_MESSAGES */
  }

  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
} /* parser_parse_script */

/**
 * @}
 * @}
 * @}
 */

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

#include "js-parser-internal.h"

#ifndef JERRY_DISABLE_PARSER

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup jsparser JavaScript
 * @{
 *
 * \addtogroup jsparser_utils Utility
 * @{
 */

/**********************************************************************/
/* Emitting byte codes                                                */
/**********************************************************************/

/**
 * Append two bytes to the cbc stream.
 */
static void
parser_emit_two_bytes (parser_context_t *context_p, /**< context */
                       uint8_t first_byte, /**< first byte */
                       uint8_t second_byte) /**< second byte */
{
  uint32_t last_position = context_p->byte_code.last_position;

  if (last_position + 2 <= PARSER_CBC_STREAM_PAGE_SIZE)
  {
    parser_mem_page_t *page_p = context_p->byte_code.last_p;

    page_p->bytes[last_position] = first_byte;
    page_p->bytes[last_position + 1] = second_byte;
    context_p->byte_code.last_position = last_position + 2;
  }
  else if (last_position >= PARSER_CBC_STREAM_PAGE_SIZE)
  {
    parser_mem_page_t *page_p;

    parser_cbc_stream_alloc_page (context_p, &context_p->byte_code);
    page_p = context_p->byte_code.last_p;
    page_p->bytes[0] = first_byte;
    page_p->bytes[1] = second_byte;
    context_p->byte_code.last_position = 2;
  }
  else
  {
    context_p->byte_code.last_p->bytes[PARSER_CBC_STREAM_PAGE_SIZE - 1] = first_byte;
    parser_cbc_stream_alloc_page (context_p, &context_p->byte_code);
    context_p->byte_code.last_p->bytes[0] = second_byte;
    context_p->byte_code.last_position = 1;
  }
} /* parser_emit_two_bytes */

#define PARSER_APPEND_TO_BYTE_CODE(context_p, byte) \
  if ((context_p)->byte_code.last_position >= PARSER_CBC_STREAM_PAGE_SIZE) \
  { \
    parser_cbc_stream_alloc_page ((context_p), &(context_p)->byte_code); \
  } \
  (context_p)->byte_code.last_p->bytes[(context_p)->byte_code.last_position++] = (uint8_t) (byte)

/**
 * Append the current byte code to the stream
 */
void
parser_flush_cbc (parser_context_t *context_p) /**< context */
{
  uint8_t flags;

  if (context_p->last_cbc_opcode == PARSER_CBC_UNAVAILABLE)
  {
    return;
  }

  context_p->status_flags |= PARSER_NO_END_LABEL;

  if (PARSER_IS_BASIC_OPCODE (context_p->last_cbc_opcode))
  {
    cbc_opcode_t opcode = (cbc_opcode_t) context_p->last_cbc_opcode;
    flags = cbc_flags[opcode];

    PARSER_APPEND_TO_BYTE_CODE (context_p, opcode);
    context_p->byte_code_size++;
  }
  else
  {
    cbc_ext_opcode_t opcode = (cbc_ext_opcode_t) PARSER_GET_EXT_OPCODE (context_p->last_cbc_opcode);

    flags = cbc_ext_flags[opcode];
    parser_emit_two_bytes (context_p, CBC_EXT_OPCODE, opcode);
    context_p->byte_code_size += 2;
  }

  JERRY_ASSERT ((flags >> CBC_STACK_ADJUST_SHIFT) >= CBC_STACK_ADJUST_BASE
                 || (CBC_STACK_ADJUST_BASE - (flags >> CBC_STACK_ADJUST_SHIFT)) <= context_p->stack_depth);
  PARSER_PLUS_EQUAL_U16 (context_p->stack_depth, CBC_STACK_ADJUST_VALUE (flags));

  if (flags & (CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2))
  {
    uint16_t literal_index = context_p->last_cbc.literal_index;

    parser_emit_two_bytes (context_p,
                           (uint8_t) (literal_index & 0xff),
                           (uint8_t) (literal_index >> 8));
    context_p->byte_code_size += 2;
  }

  if (flags & CBC_HAS_LITERAL_ARG2)
  {
    uint16_t literal_index = context_p->last_cbc.value;

    parser_emit_two_bytes (context_p,
                           (uint8_t) (literal_index & 0xff),
                           (uint8_t) (literal_index >> 8));
    context_p->byte_code_size += 2;

    if (!(flags & CBC_HAS_LITERAL_ARG))
    {
      literal_index = context_p->last_cbc.third_literal_index;

      parser_emit_two_bytes (context_p,
                             (uint8_t) (literal_index & 0xff),
                             (uint8_t) (literal_index >> 8));
      context_p->byte_code_size += 2;
    }
  }

  if (flags & CBC_HAS_BYTE_ARG)
  {
    uint8_t byte_argument = (uint8_t) context_p->last_cbc.value;

    JERRY_ASSERT (context_p->last_cbc.value <= CBC_MAXIMUM_BYTE_VALUE);

    if (flags & CBC_POP_STACK_BYTE_ARG)
    {
      JERRY_ASSERT (context_p->stack_depth >= byte_argument);
      PARSER_MINUS_EQUAL_U16 (context_p->stack_depth, byte_argument);
    }

    PARSER_APPEND_TO_BYTE_CODE (context_p, byte_argument);
    context_p->byte_code_size++;
  }

#ifdef PARSER_DUMP_BYTE_CODE
  if (context_p->is_show_opcodes)
  {
    const char *name_p;

    if (PARSER_IS_BASIC_OPCODE (context_p->last_cbc_opcode))
    {
      name_p = cbc_names[context_p->last_cbc_opcode];
    }
    else
    {
      name_p = cbc_ext_names[PARSER_GET_EXT_OPCODE (context_p->last_cbc_opcode)];
    }

    JERRY_DEBUG_MSG ("  [%3d] %s", (int) context_p->stack_depth, name_p);

    if (flags & (CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2))
    {
      uint16_t literal_index = context_p->last_cbc.literal_index;
      lexer_literal_t *literal_p = PARSER_GET_LITERAL (literal_index);
      JERRY_DEBUG_MSG (" idx:%d->", literal_index);
      util_print_literal (literal_p);
    }

    if (flags & CBC_HAS_LITERAL_ARG2)
    {
      uint16_t literal_index = context_p->last_cbc.value;
      lexer_literal_t *literal_p = PARSER_GET_LITERAL (literal_index);
      JERRY_DEBUG_MSG (" idx:%d->", literal_index);
      util_print_literal (literal_p);

      if (!(flags & CBC_HAS_LITERAL_ARG))
      {
        literal_index = context_p->last_cbc.third_literal_index;

        lexer_literal_t *literal_p = PARSER_GET_LITERAL (literal_index);
        JERRY_DEBUG_MSG (" idx:%d->", literal_index);
        util_print_literal (literal_p);
      }
    }

    if (flags & CBC_HAS_BYTE_ARG)
    {
      JERRY_DEBUG_MSG (" byte_arg:%d", (int) context_p->last_cbc.value);
    }

    JERRY_DEBUG_MSG ("\n");
  }
#endif /* PARSER_DUMP_BYTE_CODE */

  if (context_p->stack_depth > context_p->stack_limit)
  {
    context_p->stack_limit = context_p->stack_depth;
    if (context_p->stack_limit > PARSER_MAXIMUM_STACK_LIMIT)
    {
      parser_raise_error (context_p, PARSER_ERR_STACK_LIMIT_REACHED);
    }
  }

  context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
} /* parser_flush_cbc */

/**
 * Append a byte code
 */
void
parser_emit_cbc (parser_context_t *context_p, /**< context */
                 uint16_t opcode) /**< opcode */
{
  JERRY_ASSERT (PARSER_ARGS_EQ (opcode, 0));

  if (context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (context_p);
  }

  context_p->last_cbc_opcode = opcode;
} /* parser_emit_cbc */

/**
 * Append a byte code with a literal argument
 */
void
parser_emit_cbc_literal (parser_context_t *context_p, /**< context */
                         uint16_t opcode, /**< opcode */
                         uint16_t literal_index) /**< literal index */
{
  JERRY_ASSERT (PARSER_ARGS_EQ (opcode, CBC_HAS_LITERAL_ARG));

  if (context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (context_p);
  }

  context_p->last_cbc_opcode = opcode;
  context_p->last_cbc.literal_index = literal_index;
  context_p->last_cbc.literal_type = LEXER_UNUSED_LITERAL;
  context_p->last_cbc.literal_object_type = LEXER_LITERAL_OBJECT_ANY;
} /* parser_emit_cbc_literal */

/**
 * Append a byte code with the current literal argument
 */
void
parser_emit_cbc_literal_from_token (parser_context_t *context_p, /**< context */
                                    uint16_t opcode) /**< opcode */
{
  JERRY_ASSERT (PARSER_ARGS_EQ (opcode, CBC_HAS_LITERAL_ARG));

  if (context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (context_p);
  }

  context_p->last_cbc_opcode = opcode;
  context_p->last_cbc.literal_index = context_p->lit_object.index;
  context_p->last_cbc.literal_type = context_p->token.lit_location.type;
  context_p->last_cbc.literal_object_type = context_p->lit_object.type;
} /* parser_emit_cbc_literal_from_token */

/**
 * Append a byte code with a call argument
 */
void
parser_emit_cbc_call (parser_context_t *context_p, /**< context */
                      uint16_t opcode, /**< opcode */
                      size_t call_arguments) /**< number of arguments */
{
  JERRY_ASSERT (PARSER_ARGS_EQ (opcode, CBC_HAS_BYTE_ARG));
  JERRY_ASSERT (call_arguments <= CBC_MAXIMUM_BYTE_VALUE);

  if (context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (context_p);
  }

  context_p->last_cbc_opcode = opcode;
  context_p->last_cbc.value = (uint16_t) call_arguments;
} /* parser_emit_cbc_call */

/**
 * Append a push number 1/2 byte code
 */
void
parser_emit_cbc_push_number (parser_context_t *context_p, /**< context */
                             bool is_negative_number) /**< sign is negative */
{
  uint16_t value = context_p->lit_object.index;

  if (context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (context_p);
  }

  cbc_opcode_t opcode = is_negative_number ? CBC_PUSH_NUMBER_NEG_BYTE : CBC_PUSH_NUMBER_POS_BYTE;

  JERRY_ASSERT (value > 0 && value <= CBC_PUSH_NUMBER_BYTE_RANGE_END);
  JERRY_ASSERT (CBC_STACK_ADJUST_VALUE (cbc_flags[opcode]) == 1);

  context_p->stack_depth++;

#ifdef PARSER_DUMP_BYTE_CODE
  if (context_p->is_show_opcodes)
  {
    int real_value = value;

    if (is_negative_number)
    {
      real_value = -real_value;
    }

    JERRY_DEBUG_MSG ("  [%3d] %s number:%d\n",
                     (int) context_p->stack_depth,
                     cbc_names[opcode],
                     real_value);
  }
#endif /* PARSER_DUMP_BYTE_CODE */

  parser_emit_two_bytes (context_p, opcode, (uint8_t) (value - 1));

  context_p->byte_code_size += 2;

  if (context_p->stack_depth > context_p->stack_limit)
  {
    context_p->stack_limit = context_p->stack_depth;
    if (context_p->stack_limit > PARSER_MAXIMUM_STACK_LIMIT)
    {
      parser_raise_error (context_p, PARSER_ERR_STACK_LIMIT_REACHED);
    }
  }
} /* parser_emit_cbc_push_number */

/**
 * Append a byte code with a branch argument
 */
void
parser_emit_cbc_forward_branch (parser_context_t *context_p, /**< context */
                                uint16_t opcode, /**< opcode */
                                parser_branch_t *branch_p) /**< branch result */
{
  uint8_t flags;
  uint32_t extra_byte_code_increase;

  if (context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (context_p);
  }

  context_p->status_flags |= PARSER_NO_END_LABEL;

  if (PARSER_IS_BASIC_OPCODE (opcode))
  {
    flags = cbc_flags[opcode];
    extra_byte_code_increase = 0;
  }
  else
  {
    PARSER_APPEND_TO_BYTE_CODE (context_p, CBC_EXT_OPCODE);
    opcode = (uint16_t) PARSER_GET_EXT_OPCODE (opcode);

    flags = cbc_ext_flags[opcode];
    extra_byte_code_increase = 1;
  }

  JERRY_ASSERT (flags & CBC_HAS_BRANCH_ARG);
  JERRY_ASSERT (CBC_BRANCH_IS_FORWARD (flags));
  JERRY_ASSERT (CBC_BRANCH_OFFSET_LENGTH (opcode) == 1);

  /* Branch opcodes never push anything onto the stack. */
  JERRY_ASSERT ((flags >> CBC_STACK_ADJUST_SHIFT) >= CBC_STACK_ADJUST_BASE
                 || (CBC_STACK_ADJUST_BASE - (flags >> CBC_STACK_ADJUST_SHIFT)) <= context_p->stack_depth);
  PARSER_PLUS_EQUAL_U16 (context_p->stack_depth, CBC_STACK_ADJUST_VALUE (flags));

#ifdef PARSER_DUMP_BYTE_CODE
  if (context_p->is_show_opcodes)
  {
    if (extra_byte_code_increase == 0)
    {
      JERRY_DEBUG_MSG ("  [%3d] %s\n", (int) context_p->stack_depth, cbc_names[opcode]);
    }
    else
    {
      JERRY_DEBUG_MSG ("  [%3d] %s\n", (int) context_p->stack_depth, cbc_ext_names[opcode]);
    }
  }
#endif /* PARSER_DUMP_BYTE_CODE */

#if PARSER_MAXIMUM_CODE_SIZE <= 65535
  opcode++;
#else /* PARSER_MAXIMUM_CODE_SIZE > 65535 */
  PARSER_PLUS_EQUAL_U16 (opcode, 2);
#endif /* PARSER_MAXIMUM_CODE_SIZE <= 65535 */

  parser_emit_two_bytes (context_p, (uint8_t) opcode, 0);
  branch_p->page_p = context_p->byte_code.last_p;
  branch_p->offset = (context_p->byte_code.last_position - 1) | (context_p->byte_code_size << 8);

  context_p->byte_code_size += extra_byte_code_increase;

#if PARSER_MAXIMUM_CODE_SIZE <= 65535
  PARSER_APPEND_TO_BYTE_CODE (context_p, 0);
  context_p->byte_code_size += 3;
#else /* PARSER_MAXIMUM_CODE_SIZE > 65535 */
  parser_emit_two_bytes (context_p, 0, 0);
  context_p->byte_code_size += 4;
#endif /* PARSER_MAXIMUM_CODE_SIZE <= 65535 */

  if (context_p->stack_depth > context_p->stack_limit)
  {
    context_p->stack_limit = context_p->stack_depth;
    if (context_p->stack_limit > PARSER_MAXIMUM_STACK_LIMIT)
    {
      parser_raise_error (context_p, PARSER_ERR_STACK_LIMIT_REACHED);
    }
  }
} /* parser_emit_cbc_forward_branch */

/**
 * Append a branch byte code and create an item
 */
parser_branch_node_t *
parser_emit_cbc_forward_branch_item (parser_context_t *context_p, /**< context */
                                     uint16_t opcode, /**< opcode */
                                     parser_branch_node_t *next_p) /**< next branch */
{
  parser_branch_t branch;
  parser_branch_node_t *new_item;

  /* Since byte code insertion may throw an out-of-memory error,
   * the branch is constructed locally, and copied later. */
  parser_emit_cbc_forward_branch (context_p, opcode, &branch);

  new_item = (parser_branch_node_t *) parser_malloc (context_p, sizeof (parser_branch_node_t));
  new_item->branch = branch;
  new_item->next_p = next_p;
  return new_item;
} /* parser_emit_cbc_forward_branch_item */

/**
 * Append a byte code with a branch argument
 */
void
parser_emit_cbc_backward_branch (parser_context_t *context_p, /**< context */
                                 uint16_t opcode, /**< opcode */
                                 uint32_t offset) /**< destination offset */
{
  uint8_t flags;
#ifdef PARSER_DUMP_BYTE_CODE
  const char *name;
#endif /* PARSER_DUMP_BYTE_CODE */

  if (context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (context_p);
  }

  context_p->status_flags |= PARSER_NO_END_LABEL;
  offset = context_p->byte_code_size - offset;

  if (PARSER_IS_BASIC_OPCODE (opcode))
  {
    flags = cbc_flags[opcode];

#ifdef PARSER_DUMP_BYTE_CODE
    name = cbc_names[opcode];
#endif /* PARSER_DUMP_BYTE_CODE */
  }
  else
  {
    PARSER_APPEND_TO_BYTE_CODE (context_p, CBC_EXT_OPCODE);
    opcode = (uint16_t) PARSER_GET_EXT_OPCODE (opcode);

    flags = cbc_ext_flags[opcode];
    context_p->byte_code_size++;

#ifdef PARSER_DUMP_BYTE_CODE
    name = cbc_ext_names[opcode];
#endif /* PARSER_DUMP_BYTE_CODE */
  }

  JERRY_ASSERT (flags & CBC_HAS_BRANCH_ARG);
  JERRY_ASSERT (CBC_BRANCH_IS_BACKWARD (flags));
  JERRY_ASSERT (CBC_BRANCH_OFFSET_LENGTH (opcode) == 1);
  JERRY_ASSERT (offset <= context_p->byte_code_size);

  /* Branch opcodes never push anything onto the stack. */
  JERRY_ASSERT ((flags >> CBC_STACK_ADJUST_SHIFT) >= CBC_STACK_ADJUST_BASE
                 || (CBC_STACK_ADJUST_BASE - (flags >> CBC_STACK_ADJUST_SHIFT)) <= context_p->stack_depth);
  PARSER_PLUS_EQUAL_U16 (context_p->stack_depth, CBC_STACK_ADJUST_VALUE (flags));

#ifdef PARSER_DUMP_BYTE_CODE
  if (context_p->is_show_opcodes)
  {
    JERRY_DEBUG_MSG ("  [%3d] %s\n", (int) context_p->stack_depth, name);
  }
#endif /* PARSER_DUMP_BYTE_CODE */

  context_p->byte_code_size += 2;
#if PARSER_MAXIMUM_CODE_SIZE <= 65535
  if (offset > 255)
  {
    opcode++;
    context_p->byte_code_size++;
  }
#else /* PARSER_MAXIMUM_CODE_SIZE > 65535 */
  if (offset > 65535)
  {
    PARSER_PLUS_EQUAL_U16 (opcode, 2);
    context_p->byte_code_size += 2;
  }
  else if (offset > 255)
  {
    opcode++;
    context_p->byte_code_size++;
  }
#endif /* PARSER_MAXIMUM_CODE_SIZE <= 65535 */

  PARSER_APPEND_TO_BYTE_CODE (context_p, (uint8_t) opcode);

#if PARSER_MAXIMUM_CODE_SIZE > 65535
  if (offset > 65535)
  {
    PARSER_APPEND_TO_BYTE_CODE (context_p, offset >> 16);
  }
#endif /* PARSER_MAXIMUM_CODE_SIZE > 65535 */

  if (offset > 255)
  {
    PARSER_APPEND_TO_BYTE_CODE (context_p, (offset >> 8) & 0xff);
  }

  PARSER_APPEND_TO_BYTE_CODE (context_p, offset & 0xff);
} /* parser_emit_cbc_backward_branch */

#undef PARSER_CHECK_LAST_POSITION
#undef PARSER_APPEND_TO_BYTE_CODE

/**
 * Set a branch to the current byte code position
 */
void
parser_set_branch_to_current_position (parser_context_t *context_p, /**< context */
                                       parser_branch_t *branch_p) /**< branch result */
{
  uint32_t delta;
  size_t offset;
  parser_mem_page_t *page_p = branch_p->page_p;

  if (context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (context_p);
  }

  context_p->status_flags &= ~PARSER_NO_END_LABEL;

  JERRY_ASSERT (context_p->byte_code_size > (branch_p->offset >> 8));

  delta = context_p->byte_code_size - (branch_p->offset >> 8);
  offset = (branch_p->offset & CBC_LOWER_SEVEN_BIT_MASK);

  JERRY_ASSERT (delta <= PARSER_MAXIMUM_CODE_SIZE);

#if PARSER_MAXIMUM_CODE_SIZE <= 65535
  page_p->bytes[offset++] = (uint8_t) (delta >> 8);
  if (offset >= PARSER_CBC_STREAM_PAGE_SIZE)
  {
    page_p = page_p->next_p;
    offset = 0;
  }
#else /* PARSER_MAXIMUM_CODE_SIZE > 65535 */
  page_p->bytes[offset++] = (uint8_t) (delta >> 16);
  if (offset >= PARSER_CBC_STREAM_PAGE_SIZE)
  {
    page_p = page_p->next_p;
    offset = 0;
  }
  page_p->bytes[offset++] = (uint8_t) ((delta >> 8) & 0xff);
  if (offset >= PARSER_CBC_STREAM_PAGE_SIZE)
  {
    page_p = page_p->next_p;
    offset = 0;
  }
#endif /* PARSER_MAXIMUM_CODE_SIZE <= 65535 */
  page_p->bytes[offset++] = delta & 0xff;
} /* parser_set_branch_to_current_position */

/**
 * Set breaks to the current byte code position
 */
void
parser_set_breaks_to_current_position (parser_context_t *context_p, /**< context */
                                       parser_branch_node_t *current_p) /**< branch list */
{
  while (current_p != NULL)
  {
    parser_branch_node_t *next_p = current_p->next_p;

    if (!(current_p->branch.offset & CBC_HIGHEST_BIT_MASK))
    {
      parser_set_branch_to_current_position (context_p, &current_p->branch);
    }
    parser_free (current_p, sizeof (parser_branch_node_t));
    current_p = next_p;
  }
} /* parser_set_breaks_to_current_position */

/**
 * Set continues to the current byte code position
 */
void
parser_set_continues_to_current_position (parser_context_t *context_p, /**< context */
                                          parser_branch_node_t *current_p) /**< branch list */
{
  while (current_p != NULL)
  {
    if (current_p->branch.offset & CBC_HIGHEST_BIT_MASK)
    {
      parser_set_branch_to_current_position (context_p, &current_p->branch);
    }
    current_p = current_p->next_p;
  }
} /* parser_set_continues_to_current_position */

#if JERRY_ENABLE_ERROR_MESSAGES
/**
 * Returns with the string representation of the error
 */
const char *
parser_error_to_string (parser_error_t error) /**< error code */
{
  switch (error)
  {
    case PARSER_ERR_OUT_OF_MEMORY:
    {
      return "Out of memory.";
    }
    case PARSER_ERR_LITERAL_LIMIT_REACHED:
    {
      return "Maximum number of literals reached.";
    }
    case PARSER_ERR_ARGUMENT_LIMIT_REACHED:
    {
      return "Maximum number of function arguments reached.";
    }
    case PARSER_ERR_STACK_LIMIT_REACHED:
    {
      return "Maximum function stack size reached.";
    }
    case PARSER_ERR_REGISTER_LIMIT_REACHED:
    {
      return "Maximum number of registers is reached.";
    }
    case PARSER_ERR_INVALID_CHARACTER:
    {
      return "Invalid (unexpected) character.";
    }
    case PARSER_ERR_INVALID_HEX_DIGIT:
    {
      return "Invalid hexadecimal digit.";
    }
    case PARSER_ERR_INVALID_ESCAPE_SEQUENCE:
    {
      return "Invalid escape sequence.";
    }
    case PARSER_ERR_INVALID_UNICODE_ESCAPE_SEQUENCE:
    {
      return "Invalid unicode escape sequence.";
    }
    case PARSER_ERR_INVALID_IDENTIFIER_START:
    {
      return "Character cannot be start of an identifier.";
    }
    case PARSER_ERR_INVALID_IDENTIFIER_PART:
    {
      return "Character cannot be part of an identifier.";
    }
    case PARSER_ERR_INVALID_NUMBER:
    {
      return "Invalid number.";
    }
    case PARSER_ERR_MISSING_EXPONENT:
    {
      return "Missing exponent part.";
    }
    case PARSER_ERR_IDENTIFIER_AFTER_NUMBER:
    {
      return "Identifier cannot start after a number.";
    }
    case PARSER_ERR_INVALID_REGEXP:
    {
      return "Invalid regular expression.";
    }
    case PARSER_ERR_UNKNOWN_REGEXP_FLAG:
    {
      return "Unknown regexp flag.";
    }
    case PARSER_ERR_DUPLICATED_REGEXP_FLAG:
    {
      return "Duplicated regexp flag.";
    }
    case PARSER_ERR_UNSUPPORTED_REGEXP:
    {
      return "Regexp is not supported in the selected profile.";
    }
    case PARSER_ERR_IDENTIFIER_TOO_LONG:
    {
      return "Identifier is too long.";
    }
    case PARSER_ERR_STRING_TOO_LONG:
    {
      return "String is too long.";
    }
    case PARSER_ERR_NUMBER_TOO_LONG:
    {
      return "Number too long.";
    }
    case PARSER_ERR_REGEXP_TOO_LONG:
    {
      return "Regexp too long.";
    }
    case PARSER_ERR_UNTERMINATED_MULTILINE_COMMENT:
    {
      return "Unterminated multiline comment.";
    }
    case PARSER_ERR_UNTERMINATED_STRING:
    {
      return "Unterminated string literal.";
    }
    case PARSER_ERR_UNTERMINATED_REGEXP:
    {
      return "Unterminated regexp literal.";
    }
    case PARSER_ERR_NEWLINE_NOT_ALLOWED:
    {
      return "Newline is not allowed in strings or regexps.";
    }
    case PARSER_ERR_OCTAL_NUMBER_NOT_ALLOWED:
    {
      return "Octal numbers are not allowed in strict mode.";
    }
    case PARSER_ERR_OCTAL_ESCAPE_NOT_ALLOWED:
    {
      return "Octal escape sequences are not allowed in strict mode.";
    }
    case PARSER_ERR_STRICT_IDENT_NOT_ALLOWED:
    {
      return "Identifier name is reserved in strict mode.";
    }
    case PARSER_ERR_EVAL_NOT_ALLOWED:
    {
      return "Eval is not allowed to use here in strict mode.";
    }
    case PARSER_ERR_ARGUMENTS_NOT_ALLOWED:
    {
      return "Arguments is not allowed to use here in strict mode.";
    }
    case PARSER_ERR_DELETE_IDENT_NOT_ALLOWED:
    {
      return "Deleting identifier is not allowed in strict mode.";
    }
    case PARSER_ERR_EVAL_CANNOT_ASSIGNED:
    {
      return "Eval cannot assigned in strict mode.";
    }
    case PARSER_ERR_ARGUMENTS_CANNOT_ASSIGNED:
    {
      return "Arguments cannot assigned in strict mode.";
    }
    case PARSER_ERR_WITH_NOT_ALLOWED:
    {
      return "With statement not allowed in strict mode.";
    }
    case PARSER_ERR_MULTIPLE_DEFAULTS_NOT_ALLOWED:
    {
      return "Multiple default cases not allowed.";
    }
    case PARSER_ERR_DEFAULT_NOT_IN_SWITCH:
    {
      return "Default statement must be in a switch block.";
    }
    case PARSER_ERR_CASE_NOT_IN_SWITCH:
    {
      return "Case statement must be in a switch block.";
    }
    case PARSER_ERR_LEFT_PAREN_EXPECTED:
    {
      return "Expected '(' token.";
    }
    case PARSER_ERR_LEFT_BRACE_EXPECTED:
    {
      return "Expected '{' token.";
    }
    case PARSER_ERR_RIGHT_PAREN_EXPECTED:
    {
      return "Expected ')' token.";
    }
    case PARSER_ERR_RIGHT_SQUARE_EXPECTED:
    {
      return "Expected ']' token.";
    }
    case PARSER_ERR_COLON_EXPECTED:
    {
      return "Expected ':' token.";
    }
    case PARSER_ERR_COLON_FOR_CONDITIONAL_EXPECTED:
    {
      return "Expected ':' token for ?: conditional expression.";
    }
    case PARSER_ERR_SEMICOLON_EXPECTED:
    {
      return "Expected ';' token.";
    }
    case PARSER_ERR_IN_EXPECTED:
    {
      return "Expected 'in' token.";
    }
    case PARSER_ERR_WHILE_EXPECTED:
    {
      return "While expected for do-while loop.";
    }
    case PARSER_ERR_CATCH_FINALLY_EXPECTED:
    {
      return "Catch or finally block expected.";
    }
    case PARSER_ERR_ARRAY_ITEM_SEPARATOR_EXPECTED:
    {
      return "Expected ',' or ']' after an array item.";
    }
    case PARSER_ERR_OBJECT_ITEM_SEPARATOR_EXPECTED:
    {
      return "Expected ',' or '}' after a property definition.";
    }
    case PARSER_ERR_IDENTIFIER_EXPECTED:
    {
      return "Identifier expected.";
    }
    case PARSER_ERR_EXPRESSION_EXPECTED:
    {
      return "Expression expected.";
    }
    case PARSER_ERR_PRIMARY_EXP_EXPECTED:
    {
      return "Primary expression expected.";
    }
    case PARSER_ERR_STATEMENT_EXPECTED:
    {
      return "Statement expected.";
    }
    case PARSER_ERR_PROPERTY_IDENTIFIER_EXPECTED:
    {
      return "Property identifier expected.";
    }
    case PARSER_ERR_ARGUMENT_LIST_EXPECTED:
    {
      return "Expected argument list.";
    }
    case PARSER_ERR_NO_ARGUMENTS_EXPECTED:
    {
      return "Property getters must have no arguments.";
    }
    case PARSER_ERR_ONE_ARGUMENT_EXPECTED:
    {
      return "Property setters must have one argument.";
    }
    case PARSER_ERR_INVALID_EXPRESSION:
    {
      return "Invalid expression.";
    }
    case PARSER_ERR_INVALID_SWITCH:
    {
      return "Invalid switch body.";
    }
    case PARSER_ERR_INVALID_BREAK:
    {
      return "Break statement must be inside a loop or switch.";
    }
    case PARSER_ERR_INVALID_BREAK_LABEL:
    {
      return "Labelled statement targeted by a break not found.";
    }
    case PARSER_ERR_INVALID_CONTINUE:
    {
      return "Continue statement must be inside a loop.";
    }
    case PARSER_ERR_INVALID_CONTINUE_LABEL:
    {
      return "Labelled statement targeted by a continue noty found.";
    }
    case PARSER_ERR_INVALID_RETURN:
    {
      return "Return statement must be inside a function body.";
    }
    case PARSER_ERR_INVALID_RIGHT_SQUARE:
    {
      return "Unexpected '}' token.";
    }
    case PARSER_ERR_DUPLICATED_LABEL:
    {
      return "Duplicated label.";
    }
    case PARSER_ERR_OBJECT_PROPERTY_REDEFINED:
    {
      return "Property of object literal redefined.";
    }
    case PARSER_ERR_NON_STRICT_ARG_DEFINITION:
    {
      return "Non strict argument definition.";
    }
    default:
    {
      JERRY_ASSERT (error == PARSER_ERR_NO_ERROR);
      return "No error.";
    }
  }
} /* parser_error_to_string */
#endif /* JERRY_ENABLE_ERROR_MESSAGES */

/**
 * @}
 * @}
 * @}
 */

#endif /* !JERRY_DISABLE_PARSER */

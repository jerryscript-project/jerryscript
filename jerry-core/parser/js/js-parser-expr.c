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

#include "js-parser-internal.h"

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup jsparser JavaScript
 * @{
 *
 * \addtogroup jsparser_expr Expression parser
 * @{
 */

/**
 * Precedence of the binary tokens.
 *
 * See also:
 *    lexer_token_type_t
 */
static const uint8_t parser_binary_precedence_table[36] =
{
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  4, 5, 6, 7, 8, 9, 10, 10, 10, 10,
  11, 11, 11, 11, 11, 11, 12, 12, 12,
  13, 13, 14, 14, 14
};

/**
 * Generate byte code for operators with lvalue.
 */
static inline void
parser_push_result (parser_context_t *context_p) /**< context */
{
  if (CBC_NO_RESULT_COMPOUND_ASSIGMENT (context_p->last_cbc_opcode))
  {
    context_p->last_cbc_opcode = (uint16_t) PARSER_TO_BINARY_OPERATION_WITH_RESULT (context_p->last_cbc_opcode);
    parser_flush_cbc (context_p);
  }
  else if (CBC_NO_RESULT_OPERATION (context_p->last_cbc_opcode))
  {
    JERRY_ASSERT (CBC_SAME_ARGS (context_p->last_cbc_opcode, context_p->last_cbc_opcode + 1));

    if (context_p->last_cbc_opcode == CBC_POST_INCR
        || context_p->last_cbc_opcode == CBC_POST_DECR)
    {
      if (context_p->stack_depth >= context_p->stack_limit)
      {
        /* Stack limit is increased for CBC_POST_INCR_PUSH_RESULT
         * and CBC_POST_DECR_PUSH_RESULT opcodes. Needed by vm.c. */
        JERRY_ASSERT (context_p->stack_depth == context_p->stack_limit);

        context_p->stack_limit++;

        if (context_p->stack_limit > PARSER_MAXIMUM_STACK_LIMIT)
        {
          parser_raise_error (context_p, PARSER_ERR_STACK_LIMIT_REACHED);
        }
      }
    }

    context_p->last_cbc_opcode++;
    parser_flush_cbc (context_p);
  }
} /* parser_push_result */

/**
 * Generate byte code for operators with lvalue.
 */
static void
parser_emit_unary_lvalue_opcode (parser_context_t *context_p, /**< context */
                                 cbc_opcode_t opcode) /**< opcode */
{
  if (PARSER_IS_PUSH_LITERAL (context_p->last_cbc_opcode)
      && context_p->last_cbc.literal_type == LEXER_IDENT_LITERAL)
  {
    if (context_p->status_flags & PARSER_IS_STRICT)
    {
      if (context_p->last_cbc.literal_object_type != LEXER_LITERAL_OBJECT_ANY)
      {
        parser_error_t error;

        if (context_p->last_cbc.literal_object_type == LEXER_LITERAL_OBJECT_EVAL)
        {
          error = PARSER_ERR_EVAL_CANNOT_ASSIGNED;
        }
        else
        {
          JERRY_ASSERT (context_p->last_cbc.literal_object_type == LEXER_LITERAL_OBJECT_ARGUMENTS);
          error = PARSER_ERR_ARGUMENTS_CANNOT_ASSIGNED;
        }
        parser_raise_error (context_p, error);
      }
      if (opcode == CBC_DELETE_PUSH_RESULT)
      {
        parser_raise_error (context_p, PARSER_ERR_DELETE_IDENT_NOT_ALLOWED);
      }
    }

    if (opcode == CBC_DELETE_PUSH_RESULT)
    {
      context_p->status_flags |= PARSER_LEXICAL_ENV_NEEDED;

      if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
      {
        context_p->last_cbc_opcode = CBC_DELETE_IDENT_PUSH_RESULT;
      }
      else if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
      {
        context_p->last_cbc_opcode = CBC_PUSH_LITERAL;
        parser_emit_cbc_literal (context_p,
                                 CBC_DELETE_IDENT_PUSH_RESULT,
                                 context_p->last_cbc.value);
      }
      else
      {
        JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_THREE_LITERALS);

        context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
        parser_emit_cbc_literal (context_p,
                                 CBC_DELETE_IDENT_PUSH_RESULT,
                                 context_p->last_cbc.third_literal_index);
      }
      return;
    }

    JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_LITERAL, opcode + CBC_UNARY_LVALUE_WITH_IDENT));

    if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
    {
      context_p->last_cbc_opcode = (uint16_t) (opcode + CBC_UNARY_LVALUE_WITH_IDENT);
    }
    else if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
    {
      context_p->last_cbc_opcode = CBC_PUSH_LITERAL;
      parser_emit_cbc_literal (context_p,
                               (uint16_t) (opcode + CBC_UNARY_LVALUE_WITH_IDENT),
                               context_p->last_cbc.value);
    }
    else
    {
      JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_THREE_LITERALS);

      context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
      parser_emit_cbc_literal (context_p,
                               (uint16_t) (opcode + CBC_UNARY_LVALUE_WITH_IDENT),
                               context_p->last_cbc.third_literal_index);
    }
  }
  else if (context_p->last_cbc_opcode == CBC_PUSH_PROP)
  {
    JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_PROP, opcode));
    context_p->last_cbc_opcode = (uint16_t) opcode;
  }
  else
  {
    switch (context_p->last_cbc_opcode)
    {
      case CBC_PUSH_PROP_LITERAL:
      {
        JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_PROP_LITERAL, CBC_PUSH_LITERAL));
        context_p->last_cbc_opcode = CBC_PUSH_LITERAL;
        break;
      }
      case CBC_PUSH_PROP_LITERAL_LITERAL:
      {
        JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_PROP_LITERAL_LITERAL, CBC_PUSH_TWO_LITERALS));
        context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
        break;
      }
      case CBC_PUSH_PROP_THIS_LITERAL:
      {
        JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_PROP_THIS_LITERAL, CBC_PUSH_THIS_LITERAL));
        context_p->last_cbc_opcode = CBC_PUSH_THIS_LITERAL;
        break;
      }
      default:
      {
        /* Invalid LeftHandSide expression. */
        parser_emit_cbc_ext (context_p, (opcode == CBC_DELETE_PUSH_RESULT) ? CBC_EXT_PUSH_UNDEFINED_BASE
                                                                           : CBC_EXT_THROW_REFERENCE_ERROR);
        break;
      }
    }
    parser_emit_cbc (context_p, opcode);
  }
} /* parser_emit_unary_lvalue_opcode */

/**
 * Parse array literal.
 */
static void
parser_parse_array_literal (parser_context_t *context_p) /**< context */
{
  uint32_t pushed_items = 0;

  JERRY_ASSERT (context_p->token.type == LEXER_LEFT_SQUARE);

  parser_emit_cbc (context_p, CBC_CREATE_ARRAY);
  lexer_next_token (context_p);

  while (true)
  {
    if (context_p->token.type == LEXER_RIGHT_SQUARE)
    {
      if (pushed_items > 0)
      {
        parser_emit_cbc_call (context_p, CBC_ARRAY_APPEND, pushed_items);
      }
      return;
    }

    pushed_items++;

    if (context_p->token.type == LEXER_COMMA)
    {
      parser_emit_cbc (context_p, CBC_PUSH_ELISION);
      lexer_next_token (context_p);
    }
    else
    {
      parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);

      if (context_p->token.type == LEXER_COMMA)
      {
        lexer_next_token (context_p);
      }
      else if (context_p->token.type != LEXER_RIGHT_SQUARE)
      {
        parser_raise_error (context_p, PARSER_ERR_ARRAY_ITEM_SEPARATOR_EXPECTED);
      }
    }

    if (pushed_items >= 64)
    {
      parser_emit_cbc_call (context_p, CBC_ARRAY_APPEND, pushed_items);
      pushed_items = 0;
    }
  }
} /* parser_parse_array_literal */

/**
 * Object literal item types.
 */
typedef enum
{
  PARSER_OBJECT_PROPERTY_START,                /**< marks the start of the property list */
  PARSER_OBJECT_PROPERTY_VALUE,                /**< value property */
  PARSER_OBJECT_PROPERTY_GETTER,               /**< getter property */
  PARSER_OBJECT_PROPERTY_SETTER,               /**< setter property */
  PARSER_OBJECT_PROPERTY_BOTH_ACCESSORS,       /**< both getter and setter properties are set */
} parser_object_literal_item_types_t;

/**
 * Parse object literal.
 */
static void
parser_append_object_literal_item (parser_context_t *context_p, /**< context */
                                   uint16_t item_index, /**< index of the item name */
                                   parser_object_literal_item_types_t item_type) /**< type of the item */
{
  parser_stack_iterator_t iterator;
  uint8_t *current_item_type_p;

  iterator.current_p = context_p->stack.first_p;
  iterator.current_position = context_p->stack.last_position;

  while (true)
  {
    current_item_type_p = iterator.current_p->bytes + iterator.current_position - 1;

    if (*current_item_type_p == PARSER_OBJECT_PROPERTY_START)
    {
      parser_stack_push_uint16 (context_p, item_index);
      parser_stack_push_uint8 (context_p, (uint8_t) item_type);
      return;
    }

    iterator.current_position--;
    if (iterator.current_position == 0)
    {
      iterator.current_p = iterator.current_p->next_p;
      iterator.current_position = PARSER_STACK_PAGE_SIZE;
    }

    uint32_t current_item_index = iterator.current_p->bytes[iterator.current_position - 1];

    iterator.current_position--;
    if (iterator.current_position == 0)
    {
      iterator.current_p = iterator.current_p->next_p;
      iterator.current_position = PARSER_STACK_PAGE_SIZE;
    }

    current_item_index |= ((uint32_t) iterator.current_p->bytes[iterator.current_position - 1]) << 8;

    iterator.current_position--;
    if (iterator.current_position == 0)
    {
      iterator.current_p = iterator.current_p->next_p;
      iterator.current_position = PARSER_STACK_PAGE_SIZE;
    }

    if (current_item_index == item_index)
    {
      if (item_type == PARSER_OBJECT_PROPERTY_VALUE
          && *current_item_type_p == PARSER_OBJECT_PROPERTY_VALUE
          && !(context_p->status_flags & PARSER_IS_STRICT))
      {
        return;
      }

      if (item_type == PARSER_OBJECT_PROPERTY_GETTER
          && *current_item_type_p == PARSER_OBJECT_PROPERTY_SETTER)
      {
        break;
      }

      if (item_type == PARSER_OBJECT_PROPERTY_SETTER
          && *current_item_type_p == PARSER_OBJECT_PROPERTY_GETTER)
      {
        break;
      }

      parser_raise_error (context_p, PARSER_ERR_OBJECT_PROPERTY_REDEFINED);
    }
  }

  uint8_t *last_page_p = context_p->stack.first_p->bytes;

  *current_item_type_p = PARSER_OBJECT_PROPERTY_BOTH_ACCESSORS;

  if (current_item_type_p == (last_page_p + context_p->stack.last_position - 1))
  {
    context_p->stack_top_uint8 = PARSER_OBJECT_PROPERTY_BOTH_ACCESSORS;
  }
} /* parser_append_object_literal_item */

/**
 * Parse object literal.
 */
static void
parser_parse_object_literal (parser_context_t *context_p) /**< context */
{
  JERRY_ASSERT (context_p->token.type == LEXER_LEFT_BRACE);

  parser_emit_cbc (context_p, CBC_CREATE_OBJECT);

  parser_stack_push_uint8 (context_p, PARSER_OBJECT_PROPERTY_START);

  while (true)
  {
    lexer_expect_object_literal_id (context_p, false);

    if (context_p->token.type == LEXER_RIGHT_BRACE)
    {
      break;
    }

    if (context_p->token.type == LEXER_PROPERTY_GETTER
        || context_p->token.type == LEXER_PROPERTY_SETTER)
    {
      uint32_t status_flags;
      cbc_ext_opcode_t opcode;
      uint16_t literal_index;
      parser_object_literal_item_types_t item_type;

      if (context_p->token.type == LEXER_PROPERTY_GETTER)
      {
        status_flags = PARSER_IS_FUNCTION | PARSER_IS_CLOSURE | PARSER_IS_PROPERTY_GETTER;
        opcode = CBC_EXT_SET_GETTER;
        item_type = PARSER_OBJECT_PROPERTY_GETTER;
      }
      else
      {
        status_flags = PARSER_IS_FUNCTION | PARSER_IS_CLOSURE | PARSER_IS_PROPERTY_SETTER;
        opcode = CBC_EXT_SET_SETTER;
        item_type = PARSER_OBJECT_PROPERTY_SETTER;
      }

      if (context_p->status_flags & PARSER_INSIDE_WITH)
      {
        status_flags |= PARSER_RESOLVE_THIS_FOR_CALLS;
      }

      lexer_expect_object_literal_id (context_p, true);
      literal_index = context_p->lit_object.index;

      parser_append_object_literal_item (context_p, literal_index, item_type);

      parser_flush_cbc (context_p);
      lexer_construct_function_object (context_p, status_flags);

      parser_emit_cbc_literal (context_p,
                               CBC_PUSH_LITERAL,
                               literal_index);

      JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_LITERAL);
      context_p->last_cbc_opcode = PARSER_TO_EXT_OPCODE (opcode);
      context_p->last_cbc.value = (uint16_t) (context_p->literal_count - 1);

      lexer_next_token (context_p);
    }
    else
    {
      uint16_t literal_index = context_p->lit_object.index;

      parser_append_object_literal_item (context_p,
                                         literal_index,
                                         PARSER_OBJECT_PROPERTY_VALUE);

      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_COLON)
      {
        parser_raise_error (context_p, PARSER_ERR_COLON_EXPECTED);
      }

      lexer_next_token (context_p);
      parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);

      parser_emit_cbc_literal (context_p, CBC_SET_PROPERTY, literal_index);
    }

    if (context_p->token.type == LEXER_RIGHT_BRACE)
    {
      break;
    }
    else if (context_p->token.type != LEXER_COMMA)
    {
      parser_raise_error (context_p, PARSER_ERR_OBJECT_ITEM_SEPARATOR_EXPECTED);
    }
  }

  while (context_p->stack_top_uint8 != PARSER_OBJECT_PROPERTY_START)
  {
    parser_stack_pop (context_p, NULL, 3);
  }

  parser_stack_pop_uint8 (context_p);
} /* parser_parse_object_literal */

/**
 * Parse and record unary operators, and parse the primary literal.
 */
static void
parser_parse_unary_expression (parser_context_t *context_p, /**< context */
                               size_t *grouping_level_p) /**< grouping level */
{
  int new_was_seen = 0;

  /* Collect unary operators. */
  while (true)
  {
    /* Convert plus and minus binary operators to unary operators. */
    if (context_p->token.type == LEXER_ADD)
    {
      context_p->token.type = LEXER_PLUS;
    }
    else if (context_p->token.type == LEXER_SUBTRACT)
    {
      context_p->token.type = LEXER_NEGATE;
    }

    /* Bracketed expressions are primary expressions. At this
     * point their left paren is pushed onto the stack and
     * they are processed when their closing paren is reached. */
    if (context_p->token.type == LEXER_LEFT_PAREN)
    {
      (*grouping_level_p)++;
      new_was_seen = 0;
    }
    else if (context_p->token.type == LEXER_KEYW_NEW)
    {
      /* After 'new' unary operators are not allowed. */
      new_was_seen = 1;
    }
    else if (new_was_seen || !LEXER_IS_UNARY_OP_TOKEN (context_p->token.type))
    {
      break;
    }

    parser_stack_push_uint8 (context_p, context_p->token.type);
    lexer_next_token (context_p);
  }

  /* Parse primary expression. */
  switch (context_p->token.type)
  {
    case LEXER_LITERAL:
    {
      cbc_opcode_t opcode = CBC_PUSH_LITERAL;

      if (context_p->token.lit_location.type == LEXER_IDENT_LITERAL
          || context_p->token.lit_location.type == LEXER_STRING_LITERAL)
      {
        lexer_construct_literal_object (context_p,
                                        &context_p->token.lit_location,
                                        context_p->token.lit_location.type);
      }
      else if (context_p->token.lit_location.type == LEXER_NUMBER_LITERAL)
      {
        bool is_negative_number = false;

        while (context_p->stack_top_uint8 == LEXER_PLUS
               || context_p->stack_top_uint8 == LEXER_NEGATE)
        {
          if (context_p->stack_top_uint8 == LEXER_NEGATE)
          {
            is_negative_number = !is_negative_number;
          }
          parser_stack_pop_uint8 (context_p);
        }

        if (lexer_construct_number_object (context_p, true, is_negative_number))
        {
          JERRY_ASSERT (context_p->lit_object.index <= CBC_PUSH_NUMBER_BYTE_RANGE_END);

          if (context_p->lit_object.index == 0)
          {
            parser_emit_cbc (context_p, CBC_PUSH_NUMBER_0);
            break;
          }

          parser_emit_cbc_push_number (context_p, is_negative_number);
          break;
        }
      }

      if (context_p->lit_object.type != LEXER_LITERAL_OBJECT_EVAL)
      {
        if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
        {
          context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
          context_p->last_cbc.value = context_p->lit_object.index;
          context_p->last_cbc.literal_type = context_p->token.lit_location.type;
          context_p->last_cbc.literal_object_type = context_p->lit_object.type;
          break;
        }

        if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
        {
          context_p->last_cbc_opcode = CBC_PUSH_THREE_LITERALS;
          context_p->last_cbc.third_literal_index = context_p->lit_object.index;
          context_p->last_cbc.literal_type = context_p->token.lit_location.type;
          context_p->last_cbc.literal_object_type = context_p->lit_object.type;
          break;
        }

        if (context_p->last_cbc_opcode == CBC_PUSH_THIS)
        {
          context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
          opcode = CBC_PUSH_THIS_LITERAL;
        }
      }

      parser_emit_cbc_literal_from_token (context_p, opcode);
      break;
    }
    case LEXER_KEYW_FUNCTION:
    {
      int literals = 0;
      uint16_t literal1 = 0;
      uint16_t literal2 = 0;

      if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
      {
        literals = 1;
        literal1 = context_p->last_cbc.literal_index;
        context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
      }
      else if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
      {
        literals = 2;
        literal1 = context_p->last_cbc.literal_index;
        literal2 = context_p->last_cbc.value;
        context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
      }
      else
      {
        parser_flush_cbc (context_p);
      }

      uint32_t status_flags = PARSER_IS_FUNCTION | PARSER_IS_FUNC_EXPRESSION | PARSER_IS_CLOSURE;

      if (context_p->status_flags & PARSER_INSIDE_WITH)
      {
        status_flags |= PARSER_RESOLVE_THIS_FOR_CALLS;
      }

      lexer_construct_function_object (context_p, status_flags);

      JERRY_ASSERT (context_p->last_cbc_opcode == PARSER_CBC_UNAVAILABLE);

      if (literals == 1)
      {
        context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
        context_p->last_cbc.literal_index = literal1;
        context_p->last_cbc.value = (uint16_t) (context_p->literal_count - 1);
      }
      else if (literals == 2)
      {
        context_p->last_cbc_opcode = CBC_PUSH_THREE_LITERALS;
        context_p->last_cbc.literal_index = literal1;
        context_p->last_cbc.value = literal2;
        context_p->last_cbc.third_literal_index = (uint16_t) (context_p->literal_count - 1);
      }
      else
      {
        parser_emit_cbc_literal (context_p,
                                 CBC_PUSH_LITERAL,
                                 (uint16_t) (context_p->literal_count - 1));
      }

      context_p->last_cbc.literal_type = LEXER_FUNCTION_LITERAL;
      context_p->last_cbc.literal_object_type = LEXER_LITERAL_OBJECT_ANY;
      break;
    }
    case LEXER_LEFT_BRACE:
    {
      parser_parse_object_literal (context_p);
      break;
    }
    case LEXER_LEFT_SQUARE:
    {
      parser_parse_array_literal (context_p);
      break;
    }
    case LEXER_DIVIDE:
    case LEXER_ASSIGN_DIVIDE:
    {
      lexer_construct_regexp_object (context_p, false);

      if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
      {
        context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
        context_p->last_cbc.value = (uint16_t) (context_p->literal_count - 1);
      }
      else if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
      {
        context_p->last_cbc_opcode = CBC_PUSH_THREE_LITERALS;
        context_p->last_cbc.third_literal_index = (uint16_t) (context_p->literal_count - 1);
      }
      else
      {
        parser_emit_cbc_literal (context_p,
                                 CBC_PUSH_LITERAL,
                                 (uint16_t) (context_p->literal_count - 1));
      }

      context_p->last_cbc.literal_type = LEXER_REGEXP_LITERAL;
      context_p->last_cbc.literal_object_type = LEXER_LITERAL_OBJECT_ANY;
      break;
    }
    case LEXER_KEYW_THIS:
    {
      parser_emit_cbc (context_p, CBC_PUSH_THIS);
      break;
    }
    case LEXER_LIT_TRUE:
    {
      parser_emit_cbc (context_p, CBC_PUSH_TRUE);
      break;
    }
    case LEXER_LIT_FALSE:
    {
      parser_emit_cbc (context_p, CBC_PUSH_FALSE);
      break;
    }
    case LEXER_LIT_NULL:
    {
      parser_emit_cbc (context_p, CBC_PUSH_NULL);
      break;
    }
    default:
    {
      parser_raise_error (context_p, PARSER_ERR_PRIMARY_EXP_EXPECTED);
      break;
    }
  }
  lexer_next_token (context_p);
} /* parser_parse_unary_expression */

/**
 * Parse the postfix part of unary operators, and
 * generate byte code for the whole expression.
 */
static void
parser_process_unary_expression (parser_context_t *context_p) /**< context */
{
  /* Parse postfix part of a primary expression. */
  while (true)
  {
    /* Since break would only break the switch, we use
     * continue to continue this loop. Without continue,
     * the code abandons the loop. */
    switch (context_p->token.type)
    {
      case LEXER_DOT:
      {
        parser_push_result (context_p);

        lexer_expect_identifier (context_p, LEXER_STRING_LITERAL);
        JERRY_ASSERT (context_p->token.type == LEXER_LITERAL
                      && context_p->token.lit_location.type == LEXER_STRING_LITERAL);

        if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
        {
          JERRY_ASSERT (CBC_ARGS_EQ (CBC_PUSH_PROP_LITERAL_LITERAL,
                                     CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2));
          context_p->last_cbc_opcode = CBC_PUSH_PROP_LITERAL_LITERAL;
          context_p->last_cbc.value = context_p->lit_object.index;
        }
        else if (context_p->last_cbc_opcode == CBC_PUSH_THIS)
        {
          context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
          parser_emit_cbc_literal_from_token (context_p, CBC_PUSH_PROP_THIS_LITERAL);
        }
        else
        {
          parser_emit_cbc_literal_from_token (context_p, CBC_PUSH_PROP_LITERAL);
        }
        lexer_next_token (context_p);
        continue;
      }

      case LEXER_LEFT_SQUARE:
      {
        parser_push_result (context_p);

        lexer_next_token (context_p);
        parser_parse_expression (context_p, PARSE_EXPR);
        if (context_p->token.type != LEXER_RIGHT_SQUARE)
        {
          parser_raise_error (context_p, PARSER_ERR_RIGHT_SQUARE_EXPECTED);
        }
        lexer_next_token (context_p);

        if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
        {
          context_p->last_cbc_opcode = CBC_PUSH_PROP_LITERAL;
        }
        else if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
        {
          context_p->last_cbc_opcode = CBC_PUSH_PROP_LITERAL_LITERAL;
        }
        else if (context_p->last_cbc_opcode == CBC_PUSH_THIS_LITERAL)
        {
          context_p->last_cbc_opcode = CBC_PUSH_PROP_THIS_LITERAL;
        }
        else
        {
          parser_emit_cbc (context_p, CBC_PUSH_PROP);
        }
        continue;
      }

      case LEXER_LEFT_PAREN:
      {
        size_t call_arguments = 0;
        uint16_t opcode = CBC_CALL;
        bool is_eval = false;

        parser_push_result (context_p);

        if (context_p->stack_top_uint8 == LEXER_KEYW_NEW)
        {
          parser_stack_pop_uint8 (context_p);
          opcode = CBC_NEW;
        }
        else
        {
          if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL
              && context_p->last_cbc.literal_object_type == LEXER_LITERAL_OBJECT_EVAL)
          {
            JERRY_ASSERT (context_p->last_cbc.literal_type == LEXER_IDENT_LITERAL);
            context_p->status_flags |= PARSER_ARGUMENTS_NEEDED | PARSER_LEXICAL_ENV_NEEDED | PARSER_NO_REG_STORE;
            is_eval = true;
          }

          if (context_p->last_cbc_opcode == CBC_PUSH_PROP)
          {
            context_p->last_cbc_opcode = CBC_PUSH_PROP_REFERENCE;
            opcode = CBC_CALL_PROP;
          }
          else if (context_p->last_cbc_opcode == CBC_PUSH_PROP_LITERAL)
          {
            context_p->last_cbc_opcode = CBC_PUSH_PROP_LITERAL_REFERENCE;
            opcode = CBC_CALL_PROP;
          }
          else if (context_p->last_cbc_opcode == CBC_PUSH_PROP_LITERAL_LITERAL)
          {
            context_p->last_cbc_opcode = CBC_PUSH_PROP_LITERAL_LITERAL_REFERENCE;
            opcode = CBC_CALL_PROP;
          }
          else if (context_p->last_cbc_opcode == CBC_PUSH_PROP_THIS_LITERAL)
          {
            context_p->last_cbc_opcode = CBC_PUSH_PROP_THIS_LITERAL_REFERENCE;
            opcode = CBC_CALL_PROP;
          }
          else if ((context_p->status_flags & (PARSER_INSIDE_WITH | PARSER_RESOLVE_THIS_FOR_CALLS))
                   && PARSER_IS_PUSH_LITERAL (context_p->last_cbc_opcode)
                   && context_p->last_cbc.literal_type == LEXER_IDENT_LITERAL)
          {
            opcode = CBC_CALL_PROP;

            if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
            {
              context_p->last_cbc_opcode = CBC_PUSH_IDENT_REFERENCE;
            }
            else if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
            {
              context_p->last_cbc_opcode = CBC_PUSH_LITERAL;
              parser_emit_cbc_literal (context_p,
                                       CBC_PUSH_IDENT_REFERENCE,
                                       context_p->last_cbc.value);
            }
            else
            {
              JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_THREE_LITERALS);

              context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
              parser_emit_cbc_literal (context_p,
                                       CBC_PUSH_IDENT_REFERENCE,
                                       context_p->last_cbc.third_literal_index);
            }
          }
        }

        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_RIGHT_PAREN)
        {
          while (true)
          {
            if (++call_arguments > CBC_MAXIMUM_BYTE_VALUE)
            {
              parser_raise_error (context_p, PARSER_ERR_ARGUMENT_LIMIT_REACHED);
            }

            parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);

            if (context_p->token.type != LEXER_COMMA)
            {
              break;
            }
            lexer_next_token (context_p);
          }

          if (context_p->token.type != LEXER_RIGHT_PAREN)
          {
            parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
          }
        }

        lexer_next_token (context_p);

        if (is_eval)
        {
          parser_emit_cbc (context_p, CBC_EVAL);
        }

        if (call_arguments == 0)
        {
          if (opcode == CBC_CALL)
          {
            parser_emit_cbc (context_p, CBC_CALL0);
            continue;
          }
          if (opcode == CBC_CALL_PROP)
          {
            parser_emit_cbc (context_p, CBC_CALL0_PROP);
            continue;
          }
          if (opcode == CBC_NEW)
          {
            parser_emit_cbc (context_p, CBC_NEW0);
            continue;
          }
        }

        if (call_arguments == 1)
        {
          if (opcode == CBC_CALL)
          {
            parser_emit_cbc (context_p, CBC_CALL1);
            continue;
          }
          if (opcode == CBC_CALL_PROP)
          {
            parser_emit_cbc (context_p, CBC_CALL1_PROP);
            continue;
          }
          if (opcode == CBC_NEW)
          {
            parser_emit_cbc (context_p, CBC_NEW1);
            continue;
          }
        }

        if (call_arguments == 2)
        {
          if (opcode == CBC_CALL)
          {
            parser_emit_cbc (context_p, CBC_CALL2);
            continue;
          }
          if (opcode == CBC_CALL_PROP)
          {
            parser_flush_cbc (context_p);
            /* Manually adjusting stack usage. */
            JERRY_ASSERT (context_p->stack_depth > 0);
            context_p->stack_depth--;
            parser_emit_cbc (context_p, CBC_CALL2_PROP);
            continue;
          }
        }

        parser_emit_cbc_call (context_p, opcode, call_arguments);
        continue;
      }

      default:
      {
        if (context_p->stack_top_uint8 == LEXER_KEYW_NEW)
        {
          parser_push_result (context_p);
          parser_emit_cbc (context_p, CBC_NEW0);
          parser_stack_pop_uint8 (context_p);
          continue;
        }

        if (!context_p->token.was_newline
            && (context_p->token.type == LEXER_INCREASE || context_p->token.type == LEXER_DECREASE))
        {
          cbc_opcode_t opcode = (context_p->token.type == LEXER_INCREASE) ? CBC_POST_INCR : CBC_POST_DECR;
          parser_push_result (context_p);
          parser_emit_unary_lvalue_opcode (context_p, opcode);
          lexer_next_token (context_p);
        }
        break;
      }
    }
    break;
  }

  /* Generate byte code for the unary operators. */
  while (true)
  {
    uint8_t token = context_p->stack_top_uint8;
    if (!LEXER_IS_UNARY_OP_TOKEN (token))
    {
      break;
    }

    parser_push_result (context_p);
    parser_stack_pop_uint8 (context_p);

    if (LEXER_IS_UNARY_LVALUE_OP_TOKEN (token))
    {
      if (token == LEXER_KEYW_DELETE)
      {
        token = CBC_DELETE_PUSH_RESULT;
      }
      else
      {
        token = (uint8_t) (LEXER_UNARY_LVALUE_OP_TOKEN_TO_OPCODE (token));
      }
      parser_emit_unary_lvalue_opcode (context_p, (cbc_opcode_t) token);
    }
    else
    {
      token = (uint8_t) (LEXER_UNARY_OP_TOKEN_TO_OPCODE (token));

      if (token == CBC_TYPEOF)
      {
        if (PARSER_IS_PUSH_LITERAL (context_p->last_cbc_opcode)
            && context_p->last_cbc.literal_type == LEXER_IDENT_LITERAL)
        {
          if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
          {
            context_p->last_cbc_opcode = CBC_TYPEOF_IDENT;
          }
          else if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
          {
            context_p->last_cbc_opcode = CBC_PUSH_LITERAL;
            parser_emit_cbc_literal (context_p,
                                     CBC_TYPEOF_IDENT,
                                     context_p->last_cbc.value);
          }
          else
          {
            JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_THREE_LITERALS);

            context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
            parser_emit_cbc_literal (context_p,
                                     CBC_TYPEOF_IDENT,
                                     context_p->last_cbc.third_literal_index);
          }
        }
        else
        {
          parser_emit_cbc (context_p, token);
        }
      }
      else
      {
        if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
        {
          /* It is not worth to combine with push multiple literals
           * since the byte code size will not decrease. */
          JERRY_ASSERT (CBC_SAME_ARGS (context_p->last_cbc_opcode, token + 1));
          context_p->last_cbc_opcode = (uint16_t) (token + 1);
        }
        else
        {
          parser_emit_cbc (context_p, token);
        }
      }
    }
  }
} /* parser_process_unary_expression */

/**
 * Append a binary token.
 */
static void
parser_append_binary_token (parser_context_t *context_p) /**< context */
{
  JERRY_ASSERT (LEXER_IS_BINARY_OP_TOKEN (context_p->token.type));

  parser_push_result (context_p);

  if (context_p->token.type == LEXER_ASSIGN)
  {
    /* Unlike other tokens, the whole byte code is saved for binary
     * assignment, since it has multiple forms depending on the
     * previous instruction. */

    if (PARSER_IS_PUSH_LITERAL (context_p->last_cbc_opcode)
        && context_p->last_cbc.literal_type == LEXER_IDENT_LITERAL)
    {
      JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_LITERAL, CBC_ASSIGN_SET_IDENT));

      if ((context_p->status_flags & PARSER_IS_STRICT)
          && context_p->last_cbc.literal_object_type != LEXER_LITERAL_OBJECT_ANY)
      {
        parser_error_t error;

        if (context_p->last_cbc.literal_object_type == LEXER_LITERAL_OBJECT_EVAL)
        {
          error = PARSER_ERR_EVAL_CANNOT_ASSIGNED;
        }
        else
        {
          JERRY_ASSERT (context_p->last_cbc.literal_object_type == LEXER_LITERAL_OBJECT_ARGUMENTS);
          error = PARSER_ERR_ARGUMENTS_CANNOT_ASSIGNED;
        }
        parser_raise_error (context_p, error);
      }

      if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
      {
        parser_stack_push_uint16 (context_p, context_p->last_cbc.literal_index);
        context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
      }
      else if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
      {
        parser_stack_push_uint16 (context_p, context_p->last_cbc.value);
        context_p->last_cbc_opcode = CBC_PUSH_LITERAL;
      }
      else
      {
        JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_THREE_LITERALS);

        parser_stack_push_uint16 (context_p, context_p->last_cbc.third_literal_index);
        context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
      }
      parser_stack_push_uint8 (context_p, CBC_ASSIGN_SET_IDENT);
    }
    else if (context_p->last_cbc_opcode == CBC_PUSH_PROP)
    {
      JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_PROP, CBC_ASSIGN));
      parser_stack_push_uint8 (context_p, CBC_ASSIGN);
      context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
    }
    else if (context_p->last_cbc_opcode == CBC_PUSH_PROP_LITERAL)
    {
      if (context_p->last_cbc.literal_type != LEXER_IDENT_LITERAL)
      {
        JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_PROP_LITERAL, CBC_ASSIGN_PROP_LITERAL));
        parser_stack_push_uint16 (context_p, context_p->last_cbc.literal_index);
        parser_stack_push_uint8 (context_p, CBC_ASSIGN_PROP_LITERAL);
        context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
      }
      else
      {
        context_p->last_cbc_opcode = CBC_PUSH_LITERAL;
        parser_stack_push_uint8 (context_p, CBC_ASSIGN);
      }
    }
    else if (context_p->last_cbc_opcode == CBC_PUSH_PROP_LITERAL_LITERAL)
    {
      JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_PROP_LITERAL_LITERAL, CBC_PUSH_TWO_LITERALS));
      context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
      parser_stack_push_uint8 (context_p, CBC_ASSIGN);
    }
    else if (context_p->last_cbc_opcode == CBC_PUSH_PROP_THIS_LITERAL)
    {
      if (context_p->last_cbc.literal_type != LEXER_IDENT_LITERAL)
      {
        JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_PROP_THIS_LITERAL, CBC_ASSIGN_PROP_THIS_LITERAL));
        parser_stack_push_uint16 (context_p, context_p->last_cbc.literal_index);
        parser_stack_push_uint8 (context_p, CBC_ASSIGN_PROP_THIS_LITERAL);
        context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
      }
      else
      {
        context_p->last_cbc_opcode = CBC_PUSH_THIS_LITERAL;
        parser_stack_push_uint8 (context_p, CBC_ASSIGN);
      }
    }
    else
    {
      /* Invalid LeftHandSide expression. */
      parser_emit_cbc_ext (context_p, CBC_EXT_THROW_REFERENCE_ERROR);
      parser_stack_push_uint8 (context_p, CBC_ASSIGN);
    }
  }
  else if (LEXER_IS_BINARY_LVALUE_TOKEN (context_p->token.type))
  {
    if (PARSER_IS_PUSH_LITERAL (context_p->last_cbc_opcode)
        && context_p->last_cbc.literal_type == LEXER_IDENT_LITERAL)
    {
      if ((context_p->status_flags & PARSER_IS_STRICT)
          && context_p->last_cbc.literal_object_type != LEXER_LITERAL_OBJECT_ANY)
      {
        parser_error_t error;

        if (context_p->last_cbc.literal_object_type == LEXER_LITERAL_OBJECT_EVAL)
        {
          error = PARSER_ERR_EVAL_CANNOT_ASSIGNED;
        }
        else
        {
          JERRY_ASSERT (context_p->last_cbc.literal_object_type == LEXER_LITERAL_OBJECT_ARGUMENTS);
          error = PARSER_ERR_ARGUMENTS_CANNOT_ASSIGNED;
        }
        parser_raise_error (context_p, error);
      }

      if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
      {
        context_p->last_cbc_opcode = CBC_PUSH_IDENT_REFERENCE;
      }
      else if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
      {
        context_p->last_cbc_opcode = CBC_PUSH_LITERAL;
        parser_emit_cbc_literal (context_p,
                                 CBC_PUSH_IDENT_REFERENCE,
                                 context_p->last_cbc.value);
      }
      else
      {
        JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_THREE_LITERALS);

        context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
        parser_emit_cbc_literal (context_p,
                                 CBC_PUSH_IDENT_REFERENCE,
                                 context_p->last_cbc.third_literal_index);
      }
    }
    else if (context_p->last_cbc_opcode == CBC_PUSH_PROP)
    {
      JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_PROP, CBC_PUSH_PROP_REFERENCE));
      context_p->last_cbc_opcode = CBC_PUSH_PROP_REFERENCE;
    }
    else if (context_p->last_cbc_opcode == CBC_PUSH_PROP_LITERAL)
    {
      JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_PROP_LITERAL,
                                   CBC_PUSH_PROP_LITERAL_REFERENCE));
      context_p->last_cbc_opcode = CBC_PUSH_PROP_LITERAL_REFERENCE;
    }
    else if (context_p->last_cbc_opcode == CBC_PUSH_PROP_LITERAL_LITERAL)
    {
      JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_PROP_LITERAL_LITERAL,
                                   CBC_PUSH_PROP_LITERAL_LITERAL_REFERENCE));
      context_p->last_cbc_opcode = CBC_PUSH_PROP_LITERAL_LITERAL_REFERENCE;
    }
    else if (context_p->last_cbc_opcode == CBC_PUSH_PROP_THIS_LITERAL)
    {
      JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_PROP_THIS_LITERAL,
                                   CBC_PUSH_PROP_THIS_LITERAL_REFERENCE));
      context_p->last_cbc_opcode = CBC_PUSH_PROP_THIS_LITERAL_REFERENCE;
    }
    else
    {
      /* Invalid LeftHandSide expression. */
      parser_emit_cbc_ext (context_p, CBC_EXT_THROW_REFERENCE_ERROR);
      parser_emit_cbc (context_p, CBC_PUSH_PROP_REFERENCE);
    }
  }
  else if (context_p->token.type == LEXER_LOGICAL_OR
           || context_p->token.type == LEXER_LOGICAL_AND)
  {
    parser_branch_t branch;
    uint16_t opcode = CBC_BRANCH_IF_LOGICAL_TRUE;

    if (context_p->token.type == LEXER_LOGICAL_AND)
    {
      opcode = CBC_BRANCH_IF_LOGICAL_FALSE;
    }

    parser_emit_cbc_forward_branch (context_p, opcode, &branch);
    parser_stack_push (context_p, &branch, sizeof (parser_branch_t));
  }

  parser_stack_push_uint8 (context_p, context_p->token.type);
} /* parser_append_binary_token */

/**
 * Emit opcode for binary computations.
 */
static void
parser_process_binary_opcodes (parser_context_t *context_p, /**< context */
                               uint8_t min_prec_treshold) /**< minimal precedence of tokens */
{
  while (true)
  {
    uint8_t token = context_p->stack_top_uint8;
    cbc_opcode_t opcode;

    /* For left-to-right operators (all binary operators except assignment
     * and logical operators), the byte code is flushed if the precedence
     * of the next operator is less or equal than the current operator. For
     * assignment and logical operators, we add 1 to the min precendence to
     * force right-to-left evaluation order. */

    if (!LEXER_IS_BINARY_OP_TOKEN (token)
        || parser_binary_precedence_table[token - LEXER_FIRST_BINARY_OP] < min_prec_treshold)
    {
      return;
    }

    parser_push_result (context_p);
    parser_stack_pop_uint8 (context_p);

    if (token == LEXER_ASSIGN)
    {
      opcode = (cbc_opcode_t) context_p->stack_top_uint8;
      parser_stack_pop_uint8 (context_p);

      if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
      {
        if (opcode == CBC_ASSIGN_SET_IDENT)
        {
          JERRY_ASSERT (CBC_ARGS_EQ (CBC_ASSIGN_LITERAL_SET_IDENT,
                                     CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2));
          context_p->last_cbc.value = parser_stack_pop_uint16 (context_p);
          context_p->last_cbc_opcode = CBC_ASSIGN_LITERAL_SET_IDENT;
          continue;
        }
      }

      if (cbc_flags[opcode] & CBC_HAS_LITERAL_ARG)
      {
        uint16_t index = parser_stack_pop_uint16 (context_p);
        parser_emit_cbc_literal (context_p, opcode, index);

        if (opcode == CBC_ASSIGN_PROP_THIS_LITERAL
            && (context_p->stack_depth >= context_p->stack_limit))
        {
          /* Stack limit is increased for VM_OC_ASSIGN_PROP_THIS. Needed by vm.c. */
          JERRY_ASSERT (context_p->stack_depth == context_p->stack_limit);

          context_p->stack_limit++;

          if (context_p->stack_limit > PARSER_MAXIMUM_STACK_LIMIT)
          {
            parser_raise_error (context_p, PARSER_ERR_STACK_LIMIT_REACHED);
          }
        }
        continue;
      }
    }
    else if (LEXER_IS_BINARY_LVALUE_TOKEN (token))
    {
      opcode = LEXER_BINARY_LVALUE_OP_TOKEN_TO_OPCODE (token);

      if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
      {
        JERRY_ASSERT (CBC_ARGS_EQ (opcode + CBC_BINARY_LVALUE_WITH_LITERAL,
                                   CBC_HAS_LITERAL_ARG));
        context_p->last_cbc_opcode = (uint16_t) (opcode + CBC_BINARY_LVALUE_WITH_LITERAL);
        continue;
      }
    }
    else if (token == LEXER_LOGICAL_OR || token == LEXER_LOGICAL_AND)
    {
      parser_branch_t branch;
      parser_stack_pop (context_p, &branch, sizeof (parser_branch_t));
      parser_set_branch_to_current_position (context_p, &branch);
      continue;
    }
    else
    {
      opcode = LEXER_BINARY_OP_TOKEN_TO_OPCODE (token);

      if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
      {
        JERRY_ASSERT (CBC_SAME_ARGS (context_p->last_cbc_opcode, opcode + CBC_BINARY_WITH_LITERAL));
        context_p->last_cbc_opcode = (uint16_t) (opcode + CBC_BINARY_WITH_LITERAL);
        continue;
      }
      else if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
      {
        JERRY_ASSERT (CBC_ARGS_EQ (opcode + CBC_BINARY_WITH_TWO_LITERALS,
                                   CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2));
        context_p->last_cbc_opcode = (uint16_t) (opcode + CBC_BINARY_WITH_TWO_LITERALS);
        continue;
      }
    }
    parser_emit_cbc (context_p, opcode);
  }
} /* parser_process_binary_opcodes */

/**
 * Parse expression.
 */
void
parser_parse_expression (parser_context_t *context_p, /**< context */
                         int options) /**< option flags */
{
  size_t grouping_level = 0;

  parser_stack_push_uint8 (context_p, LEXER_EXPRESSION_START);

  while (true)
  {
    if (options & PARSE_EXPR_HAS_LITERAL)
    {
      JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_LITERAL);
      /* True only for the first expression. */
      options &= ~PARSE_EXPR_HAS_LITERAL;
    }
    else
    {
      parser_parse_unary_expression (context_p, &grouping_level);
    }

    while (true)
    {
      parser_process_unary_expression (context_p);

      /* The engine flush binary opcodes above this precedence. */
      uint8_t min_prec_treshold = CBC_MAXIMUM_BYTE_VALUE;

      if (LEXER_IS_BINARY_OP_TOKEN (context_p->token.type))
      {
        min_prec_treshold = parser_binary_precedence_table[context_p->token.type - LEXER_FIRST_BINARY_OP];
        if (LEXER_IS_BINARY_LVALUE_TOKEN (context_p->token.type)
            || context_p->token.type == LEXER_LOGICAL_OR
            || context_p->token.type == LEXER_LOGICAL_AND)
        {
          /* Right-to-left evaluation order. */
          min_prec_treshold++;
        }
      }
      else
      {
        min_prec_treshold = 0;
      }

      parser_process_binary_opcodes (context_p, min_prec_treshold);

      if (context_p->token.type == LEXER_RIGHT_PAREN)
      {
        if (context_p->stack_top_uint8 == LEXER_LEFT_PAREN
            || context_p->stack_top_uint8 == LEXER_COMMA_SEP_LIST)
        {
          JERRY_ASSERT (grouping_level > 0);
          grouping_level--;

          if (context_p->stack_top_uint8 == LEXER_COMMA_SEP_LIST)
          {
            parser_push_result (context_p);
            parser_flush_cbc (context_p);
          }

          parser_stack_pop_uint8 (context_p);
          lexer_next_token (context_p);
          continue;
        }
      }
      else if (context_p->token.type == LEXER_QUESTION_MARK)
      {
        cbc_opcode_t opcode = CBC_BRANCH_IF_FALSE_FORWARD;
        parser_branch_t cond_branch;
        parser_branch_t uncond_branch;

        parser_push_result (context_p);

        if (context_p->last_cbc_opcode == CBC_LOGICAL_NOT)
        {
          context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
          opcode = CBC_BRANCH_IF_TRUE_FORWARD;
        }

        parser_emit_cbc_forward_branch (context_p, opcode, &cond_branch);

        lexer_next_token (context_p);
        parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);
        parser_emit_cbc_forward_branch (context_p, CBC_JUMP_FORWARD, &uncond_branch);
        parser_set_branch_to_current_position (context_p, &cond_branch);

        /* Although byte code is constructed for two branches,
         * only one of them will be executed. To reflect this
         * the stack is manually adjusted. */
        JERRY_ASSERT (context_p->stack_depth > 0);
        context_p->stack_depth--;

        if (context_p->token.type != LEXER_COLON)
        {
          parser_raise_error (context_p, PARSER_ERR_COLON_FOR_CONDITIONAL_EXPECTED);
        }

        lexer_next_token (context_p);

        parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);
        parser_set_branch_to_current_position (context_p, &uncond_branch);

        /* Last opcode rewrite is not allowed because
         * the result may come from the first branch. */
        parser_flush_cbc (context_p);
        continue;
      }
      break;
    }

    if (context_p->token.type == LEXER_COMMA)
    {
      if (!(options & PARSE_EXPR_NO_COMMA) || grouping_level > 0)
      {
        if (!CBC_NO_RESULT_OPERATION (context_p->last_cbc_opcode))
        {
          parser_emit_cbc (context_p, CBC_POP);
        }
        if (context_p->stack_top_uint8 == LEXER_LEFT_PAREN)
        {
          parser_mem_page_t *page_p = context_p->stack.first_p;

          JERRY_ASSERT (page_p != NULL);

          page_p->bytes[context_p->stack.last_position - 1] = LEXER_COMMA_SEP_LIST;
          context_p->stack_top_uint8 = LEXER_COMMA_SEP_LIST;
        }
        lexer_next_token (context_p);
        continue;
      }
    }
    else if (LEXER_IS_BINARY_OP_TOKEN (context_p->token.type))
    {
      parser_append_binary_token (context_p);
      lexer_next_token (context_p);
      continue;
    }
    break;
  }

  if (grouping_level != 0)
  {
    parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
  }

  JERRY_ASSERT (context_p->stack_top_uint8 == LEXER_EXPRESSION_START);
  parser_stack_pop_uint8 (context_p);

  if (options & PARSE_EXPR_STATEMENT)
  {
    if (!CBC_NO_RESULT_OPERATION (context_p->last_cbc_opcode))
    {
      parser_emit_cbc (context_p, CBC_POP);
    }
  }
  else if (options & PARSE_EXPR_BLOCK)
  {
    if (CBC_NO_RESULT_COMPOUND_ASSIGMENT (context_p->last_cbc_opcode))
    {
      context_p->last_cbc_opcode = PARSER_TO_BINARY_OPERATION_WITH_BLOCK (context_p->last_cbc_opcode);
      parser_flush_cbc (context_p);
    }
    else if (CBC_NO_RESULT_BLOCK (context_p->last_cbc_opcode))
    {
      JERRY_ASSERT (CBC_SAME_ARGS (context_p->last_cbc_opcode, context_p->last_cbc_opcode + 2));
      PARSER_PLUS_EQUAL_U16 (context_p->last_cbc_opcode, 2);
      parser_flush_cbc (context_p);
    }
    else
    {
      if (CBC_NO_RESULT_OPERATION (context_p->last_cbc_opcode))
      {
        JERRY_ASSERT (CBC_SAME_ARGS (context_p->last_cbc_opcode, context_p->last_cbc_opcode + 1));
        context_p->last_cbc_opcode++;
      }
      parser_emit_cbc (context_p, CBC_POP_BLOCK);
    }
  }
  else
  {
    parser_push_result (context_p);
  }
} /* parser_parse_expression */

/**
 * @}
 * @}
 * @}
 */

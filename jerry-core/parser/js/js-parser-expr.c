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

#if ENABLED (JERRY_PARSER)
#include "jcontext.h"

#include "ecma-helpers.h"
#include "lit-char-helpers.h"

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
 * Maximum precedence for right-to-left binary operation evaluation
 */
#define PARSER_RIGHT_TO_LEFT_ORDER_MAX_PRECEDENCE 6

/**
 * Precende for ternary operation
 */
#define PARSER_RIGHT_TO_LEFT_ORDER_TERNARY_PRECEDENCE 4

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
  if (CBC_NO_RESULT_OPERATION (context_p->last_cbc_opcode))
  {
    JERRY_ASSERT (CBC_SAME_ARGS (context_p->last_cbc_opcode, context_p->last_cbc_opcode + 1));

    if ((context_p->last_cbc_opcode == CBC_POST_INCR
         || context_p->last_cbc_opcode == CBC_POST_DECR)
        && context_p->stack_depth >= context_p->stack_limit)
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

    context_p->last_cbc_opcode++;
    parser_flush_cbc (context_p);
  }
} /* parser_push_result */

/**
 * Check for invalid assignment for "eval" and "arguments"
 */
static void
parser_check_invalid_assign (parser_context_t *context_p) /**< context */
{
  JERRY_ASSERT (context_p->last_cbc.literal_type == LEXER_IDENT_LITERAL);

  if (JERRY_UNLIKELY ((context_p->status_flags & PARSER_IS_STRICT)
                      && context_p->last_cbc.literal_object_type != LEXER_LITERAL_OBJECT_ANY))
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
} /* parser_check_invalid_assign */

/**
 * Emit identifier reference
 */
static void
parser_emit_ident_reference (parser_context_t *context_p, /**< context */
                             uint16_t opcode) /* opcode */
{
  if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
  {
    context_p->last_cbc_opcode = opcode;
    return;
  }

  uint16_t literal_index;

  if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
  {
    context_p->last_cbc_opcode = CBC_PUSH_LITERAL;
    literal_index = context_p->last_cbc.value;
  }
  else if (context_p->last_cbc_opcode == CBC_PUSH_THIS_LITERAL)
  {
    context_p->last_cbc_opcode = CBC_PUSH_THIS;
    literal_index = context_p->lit_object.index;
  }
  else
  {
    JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_THREE_LITERALS);
    context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
    literal_index = context_p->last_cbc.third_literal_index;
  }

  parser_emit_cbc_literal (context_p, opcode, literal_index);
} /* parser_emit_ident_reference */

/**
 * Generate byte code for operators with lvalue.
 */
static void
parser_emit_unary_lvalue_opcode (parser_context_t *context_p, /**< context */
                                 cbc_opcode_t opcode) /**< opcode */
{
  if (PARSER_IS_PUSH_LITERALS_WITH_THIS (context_p->last_cbc_opcode)
      && context_p->last_cbc.literal_type == LEXER_IDENT_LITERAL)
  {
    parser_check_invalid_assign (context_p);

    uint16_t unary_opcode;

    if (opcode == CBC_DELETE_PUSH_RESULT)
    {
      if (JERRY_UNLIKELY (context_p->status_flags & PARSER_IS_STRICT))
      {
        parser_raise_error (context_p, PARSER_ERR_DELETE_IDENT_NOT_ALLOWED);
      }

      context_p->status_flags |= PARSER_LEXICAL_ENV_NEEDED;
      unary_opcode = CBC_DELETE_IDENT_PUSH_RESULT;
    }
    else
    {
      JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_LITERAL, opcode + CBC_UNARY_LVALUE_WITH_IDENT));
      unary_opcode = (uint16_t) (opcode + CBC_UNARY_LVALUE_WITH_IDENT);
    }

    parser_emit_ident_reference (context_p, unary_opcode);
    return;
  }

  if (context_p->last_cbc_opcode == CBC_PUSH_PROP)
  {
    JERRY_ASSERT (CBC_SAME_ARGS (CBC_PUSH_PROP, opcode));
    context_p->last_cbc_opcode = (uint16_t) opcode;
    return;
  }

  if (PARSER_IS_PUSH_PROP_LITERAL (context_p->last_cbc_opcode))
  {
    context_p->last_cbc_opcode = PARSER_PUSH_PROP_LITERAL_TO_PUSH_LITERAL (context_p->last_cbc_opcode);
  }
  else
  {
    /* Invalid LeftHandSide expression. */
    parser_emit_cbc_ext (context_p, (opcode == CBC_DELETE_PUSH_RESULT) ? CBC_EXT_PUSH_UNDEFINED_BASE
                                                                       : CBC_EXT_THROW_REFERENCE_ERROR);
  }

  parser_emit_cbc (context_p, (uint16_t) opcode);
} /* parser_emit_unary_lvalue_opcode */

/**
 * Parse array literal.
 */
static void
parser_parse_array_literal (parser_context_t *context_p) /**< context */
{
  uint32_t pushed_items = 0;
  uint16_t opcode = (uint16_t) CBC_ARRAY_APPEND;

  JERRY_ASSERT (context_p->token.type == LEXER_LEFT_SQUARE);

  parser_emit_cbc (context_p, CBC_CREATE_ARRAY);
  lexer_next_token (context_p);

  while (true)
  {
    if (context_p->token.type == LEXER_RIGHT_SQUARE)
    {
      if (pushed_items > 0)
      {
        parser_emit_cbc_call (context_p, opcode, pushed_items);
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
#if ENABLED (JERRY_ES2015)
      bool is_spread = false;
      if (context_p->token.type == LEXER_THREE_DOTS)
      {
        opcode = (uint16_t) (PARSER_TO_EXT_OPCODE (CBC_EXT_SPREAD_ARRAY_APPEND));
        is_spread = true;
        lexer_next_token (context_p);
      }
#endif /* ENABLED (JERRY_ES2015) */

      parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);

#if ENABLED (JERRY_ES2015)
      if (is_spread)
      {
        parser_emit_cbc_ext (context_p, CBC_EXT_CREATE_SPREAD_OBJECT);
      }
#endif /* ENABLED (JERRY_ES2015) */

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
      parser_emit_cbc_call (context_p, opcode, pushed_items);
#if ENABLED (JERRY_ES2015)
      opcode = (uint16_t) CBC_ARRAY_APPEND;
#endif /* ENABLED (JERRY_ES2015) */
      pushed_items = 0;
    }
  }
} /* parser_parse_array_literal */

#if !ENABLED (JERRY_ES2015)
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
#endif /* !ENABLED (JERRY_ES2015) */

#if ENABLED (JERRY_ES2015)
/**
 * Description of "get" literal string.
 */
static const lexer_lit_location_t lexer_get_literal =
{
  (const uint8_t *) "get", 3, LEXER_STRING_LITERAL, false
};

/**
 * Description of "set" literal string.
 */
static const lexer_lit_location_t lexer_set_literal =
{
  (const uint8_t *) "set", 3, LEXER_STRING_LITERAL, false
};

/**
 * Parse class as an object literal.
 */
static void
parser_parse_class_literal (parser_context_t *context_p) /**< context */
{
  JERRY_ASSERT (context_p->token.type == LEXER_LEFT_BRACE);

  bool super_called = false;
  uint32_t status_flags = PARSER_IS_FUNCTION | PARSER_IS_CLOSURE;
  status_flags |= context_p->status_flags & (PARSER_CLASS_HAS_SUPER | PARSER_CLASS_IMPLICIT_SUPER);

  while (true)
  {
    if (!(status_flags & PARSER_CLASS_STATIC_FUNCTION))
    {
      lexer_skip_empty_statements (context_p);
    }

    lexer_expect_object_literal_id (context_p, LEXER_OBJ_IDENT_CLASS_METHOD);

    if (context_p->token.type == LEXER_RIGHT_BRACE)
    {
      break;
    }

    bool is_computed = false;

    if (context_p->token.type == LEXER_PROPERTY_GETTER || context_p->token.type == LEXER_PROPERTY_SETTER)
    {
      uint16_t literal_index, function_literal_index;
      bool is_getter = (context_p->token.type == LEXER_PROPERTY_GETTER);

      lexer_skip_empty_statements (context_p);

      if (lexer_check_next_character (context_p, LIT_CHAR_LEFT_PAREN))
      {
        lexer_construct_literal_object (context_p,
                                        (is_getter ? (lexer_lit_location_t *) &lexer_get_literal
                                                   : (lexer_lit_location_t *) &lexer_set_literal),
                                        LEXER_STRING_LITERAL);
        goto parse_class_method;
      }

      uint32_t accessor_status_flags = PARSER_IS_FUNCTION | PARSER_IS_CLOSURE;
      accessor_status_flags |= (is_getter ? PARSER_IS_PROPERTY_GETTER : PARSER_IS_PROPERTY_SETTER);

      lexer_expect_object_literal_id (context_p, LEXER_OBJ_IDENT_CLASS_METHOD | LEXER_OBJ_IDENT_ONLY_IDENTIFIERS);
      literal_index = context_p->lit_object.index;

      if (context_p->token.type == LEXER_RIGHT_SQUARE)
      {
        is_computed = true;
      }
      else if (!(status_flags & PARSER_CLASS_STATIC_FUNCTION)
               && LEXER_IS_IDENT_OR_STRING (context_p->token.lit_location.type)
               && lexer_compare_literal_to_string (context_p, "constructor", 11))
      {
        parser_raise_error (context_p, PARSER_ERR_CLASS_CONSTRUCTOR_AS_ACCESSOR);
      }

      parser_flush_cbc (context_p);
      function_literal_index = lexer_construct_function_object (context_p, accessor_status_flags);

      parser_emit_cbc_literal (context_p,
                               CBC_PUSH_LITERAL,
                               literal_index);

      JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_LITERAL);

      cbc_ext_opcode_t opcode;
      bool is_static = (status_flags & PARSER_CLASS_STATIC_FUNCTION) != 0;

      if (is_computed)
      {
        context_p->last_cbc.literal_index = function_literal_index;

        if (is_getter)
        {
          opcode = is_static ? CBC_EXT_SET_STATIC_COMPUTED_GETTER : CBC_EXT_SET_COMPUTED_GETTER;
        }
        else
        {
          opcode = is_static ? CBC_EXT_SET_STATIC_COMPUTED_SETTER : CBC_EXT_SET_COMPUTED_SETTER;
        }
      }
      else
      {
        context_p->last_cbc.value = function_literal_index;

        if (is_getter)
        {
          opcode = is_static ? CBC_EXT_SET_STATIC_GETTER : CBC_EXT_SET_GETTER;
        }
        else
        {
          opcode = is_static ? CBC_EXT_SET_STATIC_SETTER : CBC_EXT_SET_SETTER;
        }
      }

      context_p->last_cbc_opcode = PARSER_TO_EXT_OPCODE (opcode);
      status_flags &= (uint32_t) ~PARSER_CLASS_STATIC_FUNCTION;
      continue;
    }

    if (!(status_flags & PARSER_CLASS_STATIC_FUNCTION) && context_p->token.type == LEXER_CLASS_CONSTRUCTOR)
    {
      if (super_called)
      {
        /* 14.5.1 */
        parser_raise_error (context_p, PARSER_ERR_MULTIPLE_CLASS_CONSTRUCTORS);
      }
      else
      {
        super_called = true;
      }

      parser_flush_cbc (context_p);
      uint32_t constructor_status_flags = status_flags | PARSER_CLASS_CONSTRUCTOR;

      if (context_p->status_flags & PARSER_CLASS_HAS_SUPER)
      {
        constructor_status_flags |= PARSER_LEXICAL_ENV_NEEDED;
      }

      if (context_p->literal_count >= PARSER_MAXIMUM_NUMBER_OF_LITERALS)
      {
        parser_raise_error (context_p, PARSER_ERR_LITERAL_LIMIT_REACHED);
      }

      uint16_t result_index = context_p->literal_count;
      lexer_literal_t *literal_p = (lexer_literal_t *) parser_list_append (context_p, &context_p->literal_pool);
      literal_p->type = LEXER_UNUSED_LITERAL;
      literal_p->status_flags = 0;
      literal_p->u.bytecode_p = parser_parse_function (context_p, constructor_status_flags);
      literal_p->type = LEXER_FUNCTION_LITERAL;
      parser_emit_cbc_literal (context_p, PARSER_TO_EXT_OPCODE (CBC_EXT_SET_CLASS_LITERAL), result_index);
      context_p->literal_count++;
      continue;
    }

    if (!(status_flags & PARSER_CLASS_STATIC_FUNCTION) && context_p->token.type == LEXER_KEYW_STATIC)
    {
      status_flags |= PARSER_CLASS_STATIC_FUNCTION;
      continue;
    }

    if (context_p->token.type == LEXER_RIGHT_SQUARE)
    {
      is_computed = true;
    }
    else if ((status_flags & PARSER_CLASS_STATIC_FUNCTION)
             && LEXER_IS_IDENT_OR_STRING (context_p->token.lit_location.type)
             && lexer_compare_literal_to_string (context_p, "prototype", 9))
    {
      parser_raise_error (context_p, PARSER_ERR_CLASS_STATIC_PROTOTYPE);
    }

parse_class_method:
    parser_flush_cbc (context_p);

    uint16_t literal_index = context_p->lit_object.index;
    uint16_t function_literal_index = lexer_construct_function_object (context_p, status_flags);

    parser_emit_cbc_literal (context_p,
                             CBC_PUSH_LITERAL,
                             function_literal_index);

    JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_LITERAL);

    context_p->last_cbc.value = literal_index;

    if ((status_flags & PARSER_CLASS_STATIC_FUNCTION))
    {
      context_p->last_cbc_opcode = PARSER_TO_EXT_OPCODE (is_computed ? CBC_EXT_SET_STATIC_COMPUTED_PROPERTY_LITERAL
                                                                     : CBC_EXT_SET_STATIC_PROPERTY_LITERAL);
      status_flags &= (uint32_t) ~PARSER_CLASS_STATIC_FUNCTION;
    }
    else
    {
      context_p->last_cbc_opcode = (is_computed ? PARSER_TO_EXT_OPCODE (CBC_EXT_SET_COMPUTED_PROPERTY_LITERAL)
                                                : CBC_SET_LITERAL_PROPERTY);
    }
  }

  if (!super_called && (context_p->status_flags & PARSER_CLASS_HAS_SUPER))
  {
    parser_emit_cbc_ext (context_p, CBC_EXT_IMPLICIT_CONSTRUCTOR_CALL);
  }

  if (context_p->status_flags & PARSER_CLASS_HAS_SUPER)
  {
    parser_emit_cbc_ext (context_p, CBC_EXT_INHERIT_AND_SET_CONSTRUCTOR);
  }
} /* parser_parse_class_literal */

/**
 * Description of "prototype" literal string.
 */
static const lexer_lit_location_t lexer_prototype_literal =
{
  (const uint8_t *) "prototype", 9, LEXER_STRING_LITERAL, false
};

/**
 * Parse class statement or expression.
 */
void
parser_parse_class (parser_context_t *context_p, /**< context */
                    bool is_statement) /**< true - if class is parsed as a statement
                                        *   false - otherwise (as an expression) */
{
  JERRY_ASSERT (context_p->token.type == LEXER_KEYW_CLASS);

  uint16_t class_ident_index = PARSER_MAXIMUM_NUMBER_OF_LITERALS;

  if (is_statement)
  {
    /* Class statement must contain an identifier. */
    lexer_expect_identifier (context_p, LEXER_IDENT_LITERAL);
    JERRY_ASSERT (context_p->token.type == LEXER_LITERAL
                  && context_p->token.lit_location.type == LEXER_IDENT_LITERAL);

    if (context_p->next_scanner_info_p->source_p == context_p->source_p)
    {
      JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_ERR_REDECLARED);
      parser_raise_error (context_p, PARSER_ERR_VARIABLE_REDECLARED);
    }

    class_ident_index = context_p->lit_object.index;

#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
    if (context_p->status_flags & PARSER_MODULE_STORE_IDENT)
    {
      context_p->module_identifier_lit_p = context_p->lit_object.literal_p;
      context_p->status_flags &= (uint32_t) ~(PARSER_MODULE_STORE_IDENT);
    }
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */

    lexer_next_token (context_p);
  }
  else
  {
    lexer_next_token (context_p);

    /* Class expression may contain an identifier. */
    if (context_p->token.type == LEXER_LITERAL && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
    {
      /* NOTE: If 'Function.name' will be supported, the current literal object must be set to 'name' property. */
      lexer_next_token (context_p);
    }
  }

  bool create_class_env = (context_p->token.type == LEXER_KEYW_EXTENDS
                           || (context_p->status_flags & PARSER_CLASS_HAS_SUPER));

  if (create_class_env)
  {
    parser_parse_super_class_context_start (context_p);
  }

  if (context_p->token.type != LEXER_LEFT_BRACE)
  {
    parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_EXPECTED);
  }

  parser_emit_cbc_ext (context_p, CBC_EXT_PUSH_CLASS_CONSTRUCTOR_AND_PROTOTYPE);

  bool is_strict = context_p->status_flags & PARSER_IS_STRICT;

  /* 14.5. A ClassBody is always strict code. */
  context_p->status_flags |= PARSER_IS_STRICT;

  /* ClassDeclaration is parsed. Continue with class body. */
  parser_parse_class_literal (context_p);

  JERRY_ASSERT (context_p->token.type == LEXER_RIGHT_BRACE);

  lexer_construct_literal_object (context_p,
                                  (lexer_lit_location_t *) &lexer_prototype_literal,
                                  lexer_prototype_literal.type);

  parser_emit_cbc_literal (context_p, CBC_SET_PROPERTY, context_p->lit_object.index);

  if (create_class_env)
  {
    parser_parse_super_class_context_end (context_p);
    context_p->status_flags &= (uint32_t) ~(PARSER_CLASS_HAS_SUPER | PARSER_CLASS_IMPLICIT_SUPER);
  }

  if (is_statement)
  {
    parser_emit_cbc_literal (context_p,
                             class_ident_index >= PARSER_REGISTER_START ? CBC_MOV_IDENT : CBC_ASSIGN_LET_CONST,
                             class_ident_index);
  }

  parser_flush_cbc (context_p);

  if (!is_strict)
  {
    /* Restore flag */
    context_p->status_flags &= (uint32_t) ~PARSER_IS_STRICT;
  }

  lexer_next_token (context_p);
} /* parser_parse_class */
#endif /* ENABLED (JERRY_ES2015) */

#if ENABLED (JERRY_ES2015)
/**
 * Parse object initializer method definition.
 *
 * See also: ES2015 14.3
 */
static void
parser_parse_object_method (parser_context_t *context_p) /**< context */
{
  parser_flush_cbc (context_p);

  context_p->source_p--;
  context_p->column--;
  uint16_t function_literal_index = lexer_construct_function_object (context_p,
                                                                     PARSER_IS_FUNCTION | PARSER_IS_CLOSURE);

  parser_emit_cbc_literal (context_p,
                           CBC_PUSH_LITERAL,
                           function_literal_index);

  lexer_next_token (context_p);
} /* parser_parse_object_method */
#endif /* ENABLED (JERRY_ES2015) */

/**
 * Parse object literal.
 */
static void
parser_parse_object_literal (parser_context_t *context_p) /**< context */
{
  JERRY_ASSERT (context_p->token.type == LEXER_LEFT_BRACE);

  parser_emit_cbc (context_p, CBC_CREATE_OBJECT);

#if !ENABLED (JERRY_ES2015)
  parser_stack_push_uint8 (context_p, PARSER_OBJECT_PROPERTY_START);
#endif /* !ENABLED (JERRY_ES2015) */

  while (true)
  {
    lexer_expect_object_literal_id (context_p, LEXER_OBJ_IDENT_NO_OPTS);

    switch (context_p->token.type)
    {
      case LEXER_RIGHT_BRACE:
      {
        break;
      }
      case LEXER_PROPERTY_GETTER:
      case LEXER_PROPERTY_SETTER:
      {
        uint32_t status_flags;
        cbc_ext_opcode_t opcode;
        uint16_t literal_index, function_literal_index;
#if !ENABLED (JERRY_ES2015)
        parser_object_literal_item_types_t item_type;
#endif /* !ENABLED (JERRY_ES2015) */

        if (context_p->token.type == LEXER_PROPERTY_GETTER)
        {
          status_flags = PARSER_IS_FUNCTION | PARSER_IS_CLOSURE | PARSER_IS_PROPERTY_GETTER;
          opcode = CBC_EXT_SET_GETTER;
#if !ENABLED (JERRY_ES2015)
          item_type = PARSER_OBJECT_PROPERTY_GETTER;
#endif /* !ENABLED (JERRY_ES2015) */
        }
        else
        {
          status_flags = PARSER_IS_FUNCTION | PARSER_IS_CLOSURE | PARSER_IS_PROPERTY_SETTER;
          opcode = CBC_EXT_SET_SETTER;
#if !ENABLED (JERRY_ES2015)
          item_type = PARSER_OBJECT_PROPERTY_SETTER;
#endif /* !ENABLED (JERRY_ES2015) */
        }

        lexer_expect_object_literal_id (context_p, LEXER_OBJ_IDENT_ONLY_IDENTIFIERS);

        /* This assignment is a nop for computed getters/setters. */
        literal_index = context_p->lit_object.index;

#if ENABLED (JERRY_ES2015)
        if (context_p->token.type == LEXER_RIGHT_SQUARE)
        {
          opcode = ((opcode == CBC_EXT_SET_GETTER) ? CBC_EXT_SET_COMPUTED_GETTER
                                                   : CBC_EXT_SET_COMPUTED_SETTER);
        }
#else /* !ENABLED (JERRY_ES2015) */
        parser_append_object_literal_item (context_p, literal_index, item_type);
#endif /* ENABLED (JERRY_ES2015) */

        parser_flush_cbc (context_p);
        function_literal_index = lexer_construct_function_object (context_p, status_flags);

#if ENABLED (JERRY_ES2015)
        if (opcode >= CBC_EXT_SET_COMPUTED_GETTER)
        {
          literal_index = function_literal_index;
        }
#endif /* ENABLED (JERRY_ES2015) */

        parser_emit_cbc_literal (context_p,
                                 CBC_PUSH_LITERAL,
                                 literal_index);

        JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_LITERAL);
        context_p->last_cbc_opcode = PARSER_TO_EXT_OPCODE (opcode);
        context_p->last_cbc.value = function_literal_index;

        lexer_next_token (context_p);
        break;
      }
#if ENABLED (JERRY_ES2015)
      case LEXER_RIGHT_SQUARE:
      {
        lexer_next_token (context_p);

        if (context_p->token.type == LEXER_LEFT_PAREN)
        {
          parser_parse_object_method (context_p);

          JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_LITERAL);
          context_p->last_cbc_opcode = PARSER_TO_EXT_OPCODE (CBC_EXT_SET_COMPUTED_PROPERTY_LITERAL);
          break;
        }

        if (context_p->token.type != LEXER_COLON)
        {
          parser_raise_error (context_p, PARSER_ERR_COLON_EXPECTED);
        }

        lexer_next_token (context_p);
        parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);

        if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
        {
          context_p->last_cbc_opcode = PARSER_TO_EXT_OPCODE (CBC_EXT_SET_COMPUTED_PROPERTY_LITERAL);
        }
        else
        {
          parser_emit_cbc_ext (context_p, CBC_EXT_SET_COMPUTED_PROPERTY);
        }
        break;
      }
#endif /* ENABLED (JERRY_ES2015) */
      default:
      {
        uint16_t literal_index = context_p->lit_object.index;

#if ENABLED (JERRY_ES2015)
        parser_line_counter_t start_line = context_p->token.line;
        parser_line_counter_t start_column = context_p->token.column;
#else /* !ENABLED (JERRY_ES2015) */
        parser_append_object_literal_item (context_p,
                                           literal_index,
                                           PARSER_OBJECT_PROPERTY_VALUE);
#endif /* ENABLED (JERRY_ES2015) */

        lexer_next_token (context_p);

#if ENABLED (JERRY_ES2015)
        if (context_p->token.type == LEXER_LEFT_PAREN)
        {
          parser_parse_object_method (context_p);

          JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_LITERAL);
          context_p->last_cbc_opcode = CBC_SET_LITERAL_PROPERTY;
          context_p->last_cbc.value = literal_index;
          break;
        }

        if (context_p->token.type == LEXER_RIGHT_BRACE
            || context_p->token.type == LEXER_COMMA)
        {
          /* Re-parse the literal as common identifier. */
          context_p->source_p = context_p->token.lit_location.char_p;
          context_p->line = start_line;
          context_p->column = start_column;

          lexer_next_token (context_p);

          if (context_p->token.type != LEXER_LITERAL
              || context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
          {
            parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
          }

          lexer_construct_literal_object (context_p,
                                          &context_p->token.lit_location,
                                          context_p->token.lit_location.type);

          parser_emit_cbc_literal_from_token (context_p, CBC_PUSH_LITERAL);

          context_p->last_cbc_opcode = CBC_SET_LITERAL_PROPERTY;
          context_p->last_cbc.value = literal_index;

          lexer_next_token (context_p);
          break;
        }
#endif /* ENABLED (JERRY_ES2015) */

        if (context_p->token.type != LEXER_COLON)
        {
          parser_raise_error (context_p, PARSER_ERR_COLON_EXPECTED);
        }

        lexer_next_token (context_p);
        parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);

        if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
        {
          context_p->last_cbc_opcode = CBC_SET_LITERAL_PROPERTY;
          context_p->last_cbc.value = literal_index;
        }
        else
        {
          parser_emit_cbc_literal (context_p, CBC_SET_PROPERTY, literal_index);
        }

        break;
      }
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

#if !ENABLED (JERRY_ES2015)
  while (context_p->stack_top_uint8 != PARSER_OBJECT_PROPERTY_START)
  {
    parser_stack_pop (context_p, NULL, 3);
  }

  parser_stack_pop_uint8 (context_p);
#endif /* !ENABLED (JERRY_ES2015) */
} /* parser_parse_object_literal */

/**
 * Parse function literal.
 */
static void
parser_parse_function_expression (parser_context_t *context_p, /**< context */
                                  uint32_t status_flags) /**< function status flags */
{
  int literals = 0;
  uint16_t literal1 = 0;
  uint16_t literal2 = 0;
  uint16_t function_literal_index;
  int32_t function_name_index = -1;

  if (status_flags & PARSER_IS_FUNC_EXPRESSION)
  {
#if ENABLED (JERRY_DEBUGGER)
    parser_line_counter_t debugger_line = context_p->token.line;
    parser_line_counter_t debugger_column = context_p->token.column;
#endif /* ENABLED (JERRY_DEBUGGER) */

    if (!lexer_check_next_character (context_p, LIT_CHAR_LEFT_PAREN))
    {
      lexer_next_token (context_p);

      if (context_p->token.type != LEXER_LITERAL
          || context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
      {
        parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
      }

      parser_flush_cbc (context_p);

      lexer_construct_literal_object (context_p, &context_p->token.lit_location, LEXER_STRING_LITERAL);

#if ENABLED (JERRY_DEBUGGER)
      if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
      {
        jerry_debugger_send_string (JERRY_DEBUGGER_FUNCTION_NAME,
                                    JERRY_DEBUGGER_NO_SUBTYPE,
                                    context_p->lit_object.literal_p->u.char_p,
                                    context_p->lit_object.literal_p->prop.length);

        /* Reset token position for the function. */
        context_p->token.line = debugger_line;
        context_p->token.column = debugger_column;
      }
#endif /* ENABLED (JERRY_DEBUGGER) */

      if (context_p->token.literal_is_reserved
          || context_p->lit_object.type != LEXER_LITERAL_OBJECT_ANY)
      {
        status_flags |= PARSER_HAS_NON_STRICT_ARG;
      }

      function_name_index = context_p->lit_object.index;
    }
  }

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

  function_literal_index = lexer_construct_function_object (context_p, status_flags);

  JERRY_ASSERT (context_p->last_cbc_opcode == PARSER_CBC_UNAVAILABLE);

  if (literals == 1)
  {
    context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
    context_p->last_cbc.literal_index = literal1;
    context_p->last_cbc.value = function_literal_index;
  }
  else if (literals == 2)
  {
    context_p->last_cbc_opcode = CBC_PUSH_THREE_LITERALS;
    context_p->last_cbc.literal_index = literal1;
    context_p->last_cbc.value = literal2;
    context_p->last_cbc.third_literal_index = function_literal_index;
  }
  else
  {
    parser_emit_cbc_literal (context_p,
                             CBC_PUSH_LITERAL,
                             function_literal_index);

    if (function_name_index != -1)
    {
      context_p->last_cbc_opcode = PARSER_TO_EXT_OPCODE (CBC_EXT_PUSH_NAMED_FUNC_EXPRESSION);
      context_p->last_cbc.value = (uint16_t) function_name_index;
    }
  }

  context_p->last_cbc.literal_type = LEXER_FUNCTION_LITERAL;
  context_p->last_cbc.literal_object_type = LEXER_LITERAL_OBJECT_ANY;
} /* parser_parse_function_expression */

#if ENABLED (JERRY_ES2015)

/**
 * Parse template literal.
 */
static void
parser_parse_template_literal (parser_context_t *context_p) /**< context */
{
  bool is_empty_head = true;

  if (context_p->token.lit_location.length > 0)
  {
    is_empty_head = false;

    lexer_construct_literal_object (context_p,
                                    &context_p->token.lit_location,
                                    context_p->token.lit_location.type);

    parser_emit_cbc_literal_from_token (context_p, CBC_PUSH_LITERAL);
  }

  lexer_next_token (context_p);
  parser_parse_expression (context_p, PARSE_EXPR);

  if (context_p->token.type != LEXER_RIGHT_BRACE)
  {
    parser_raise_error (context_p, PARSER_ERR_RIGHT_BRACE_EXPECTED);
  }

  if (!is_empty_head)
  {
    if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
    {
      context_p->last_cbc_opcode = CBC_ADD_TWO_LITERALS;
    }
    else if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
    {
      context_p->last_cbc_opcode = CBC_ADD_RIGHT_LITERAL;
    }
    else
    {
      parser_emit_cbc (context_p, CBC_ADD);
    }
  }

  context_p->source_p--;
  context_p->column--;
  lexer_parse_string (context_p);

  if (is_empty_head || context_p->token.lit_location.length > 0)
  {
    lexer_construct_literal_object (context_p,
                                    &context_p->token.lit_location,
                                    context_p->token.lit_location.type);

    if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
    {
      context_p->last_cbc_opcode = CBC_ADD_TWO_LITERALS;
      context_p->last_cbc.value = context_p->lit_object.index;
      context_p->last_cbc.literal_type = context_p->token.lit_location.type;
      context_p->last_cbc.literal_object_type = context_p->lit_object.type;
    }
    else
    {
      parser_emit_cbc_literal_from_token (context_p, CBC_ADD_RIGHT_LITERAL);
    }
  }

  while (context_p->source_p[-1] != LIT_CHAR_GRAVE_ACCENT)
  {
    lexer_next_token (context_p);
    parser_parse_expression (context_p, PARSE_EXPR);

    if (context_p->token.type != LEXER_RIGHT_BRACE)
    {
      parser_raise_error (context_p, PARSER_ERR_RIGHT_BRACE_EXPECTED);
    }

    if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
    {
      context_p->last_cbc_opcode = CBC_ADD_RIGHT_LITERAL;
    }
    else
    {
      parser_emit_cbc (context_p, CBC_ADD);
    }

    context_p->source_p--;
    context_p->column--;
    lexer_parse_string (context_p);

    if (context_p->token.lit_location.length > 0)
    {
      lexer_construct_literal_object (context_p,
                                      &context_p->token.lit_location,
                                      context_p->token.lit_location.type);

      parser_emit_cbc_literal_from_token (context_p, CBC_ADD_RIGHT_LITERAL);
    }
  }

  return;
} /* parser_parse_template_literal */

#endif /* ENABLED (JERRY_ES2015) */

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
#if ENABLED (JERRY_ES2015)
      if (context_p->next_scanner_info_p->source_p == context_p->source_p)
      {
        JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_FUNCTION);
        break;
      }
#endif /* ENABLED (JERRY_ES2015) */
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
#if ENABLED (JERRY_ES2015)
    case LEXER_TEMPLATE_LITERAL:
    {
      if (context_p->source_p[-1] != LIT_CHAR_GRAVE_ACCENT)
      {
        parser_parse_template_literal (context_p);
        break;
      }

      /* The string is a normal string literal. */
      /* FALLTHRU */
    }
#endif /* ENABLED (JERRY_ES2015) */
    case LEXER_LITERAL:
    {
#if ENABLED (JERRY_ES2015)
      if (context_p->next_scanner_info_p->source_p == context_p->source_p)
      {
        JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_FUNCTION);

        parser_parse_function_expression (context_p,
                                          PARSER_IS_FUNCTION | PARSER_IS_ARROW_FUNCTION);
        return;
      }
#endif /* ENABLED (JERRY_ES2015) */

      uint8_t type = context_p->token.lit_location.type;

      if (type == LEXER_IDENT_LITERAL || type == LEXER_STRING_LITERAL)
      {
        lexer_construct_literal_object (context_p,
                                        &context_p->token.lit_location,
                                        context_p->token.lit_location.type);

#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
        if ((context_p->status_flags & PARSER_MODULE_STORE_IDENT)
            && type == LEXER_IDENT_LITERAL)
        {
          context_p->module_identifier_lit_p = context_p->lit_object.literal_p;
          context_p->status_flags &= (uint32_t) ~(PARSER_MODULE_STORE_IDENT);
        }
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */
      }
      else if (type == LEXER_NUMBER_LITERAL)
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

          parser_emit_cbc_push_number (context_p, is_negative_number);
          break;
        }
      }

      cbc_opcode_t opcode = CBC_PUSH_LITERAL;

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

      parser_emit_cbc_literal_from_token (context_p, (uint16_t) opcode);
      break;
    }
    case LEXER_KEYW_FUNCTION:
    {
      parser_parse_function_expression (context_p,
                                        PARSER_IS_FUNCTION | PARSER_IS_FUNC_EXPRESSION | PARSER_IS_CLOSURE);
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

      uint16_t literal_index = (uint16_t) (context_p->literal_count - 1);

      if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
      {
        context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
        context_p->last_cbc.value = literal_index;
      }
      else if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
      {
        context_p->last_cbc_opcode = CBC_PUSH_THREE_LITERALS;
        context_p->last_cbc.third_literal_index = literal_index;
      }
      else
      {
        parser_emit_cbc_literal (context_p, CBC_PUSH_LITERAL, literal_index);
      }

      context_p->last_cbc.literal_type = LEXER_REGEXP_LITERAL;
      context_p->last_cbc.literal_object_type = LEXER_LITERAL_OBJECT_ANY;
      break;
    }
    case LEXER_KEYW_THIS:
    {
#if ENABLED (JERRY_ES2015)
      if (PARSER_IS_CLASS_CONSTRUCTOR_SUPER (context_p->status_flags))
      {
        if (context_p->status_flags & PARSER_CLASS_IMPLICIT_SUPER)
        {
          parser_emit_cbc (context_p, CBC_PUSH_THIS);
        }
        else
        {
          parser_emit_cbc_ext (context_p, CBC_EXT_PUSH_CONSTRUCTOR_THIS);
        }
      }
      else
      {
#endif /* ENABLED (JERRY_ES2015) */
        parser_emit_cbc (context_p, CBC_PUSH_THIS);
#if ENABLED (JERRY_ES2015)
      }
#endif /* ENABLED (JERRY_ES2015) */
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
#if ENABLED (JERRY_ES2015)
    case LEXER_KEYW_CLASS:
    {
      parser_parse_class (context_p, false);
      return;
    }
    case LEXER_KEYW_SUPER:
    {
      if ((lexer_check_next_character (context_p, LIT_CHAR_DOT)
            || lexer_check_next_character (context_p, LIT_CHAR_LEFT_SQUARE))
          && context_p->status_flags & (PARSER_CLASS_HAS_SUPER))
      {
        if (!LEXER_IS_BINARY_LVALUE_TOKEN (context_p->stack_top_uint8))
        {
          context_p->status_flags |= PARSER_CLASS_SUPER_PROP_REFERENCE;
        }

        if (context_p->status_flags & PARSER_CLASS_CONSTRUCTOR)
        {
          parser_emit_cbc_ext (context_p, CBC_EXT_PUSH_CONSTRUCTOR_SUPER_PROP);
          break;
        }

        if (context_p->status_flags & PARSER_CLASS_IMPLICIT_SUPER)
        {
          parser_emit_cbc_ext (context_p, CBC_EXT_PUSH_STATIC_SUPER);
          break;
        }

        bool is_static = (context_p->status_flags & PARSER_CLASS_STATIC_FUNCTION) != 0;
        parser_emit_cbc_ext (context_p, is_static ? CBC_EXT_PUSH_STATIC_SUPER : CBC_EXT_PUSH_SUPER);
        break;
      }

      if (lexer_check_next_character (context_p, LIT_CHAR_LEFT_PAREN)
          && ((context_p->status_flags & PARSER_CLASS_CONSTRUCTOR_SUPER) == PARSER_CLASS_CONSTRUCTOR_SUPER)
          && !(context_p->status_flags & PARSER_CLASS_IMPLICIT_SUPER))
      {
        parser_emit_cbc_ext (context_p, CBC_EXT_PUSH_CONSTRUCTOR_SUPER);
        break;
      }

      parser_raise_error (context_p, PARSER_ERR_UNEXPECTED_SUPER_REFERENCE);
    }
    case LEXER_LEFT_PAREN:
    {
      JERRY_ASSERT (context_p->next_scanner_info_p->source_p == context_p->source_p
                    && context_p->next_scanner_info_p->type == SCANNER_TYPE_FUNCTION);

      lexer_next_token (context_p);
      parser_parse_function_expression (context_p,
                                        PARSER_IS_FUNCTION | PARSER_IS_ARROW_FUNCTION | PARSER_ARROW_PARSE_ARGS);
      return;
    }
#endif /* ENABLED (JERRY_ES2015) */
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
#if ENABLED (JERRY_ES2015)
  /* Track to see if a property was accessed or not */
  bool property_accessed = false;
#endif /* ENABLED (JERRY_ES2015) */

  /* Parse postfix part of a primary expression. */
  while (true)
  {
#if ENABLED (JERRY_ES2015)
    if (context_p->token.type == LEXER_DOT || context_p->token.type == LEXER_LEFT_SQUARE)
    {
      if (property_accessed)
      {
        /**
         * In the case of "super.prop1.prop2(...)" the second property access should not
         * generate a super prop call thus the 'PARSER_CLASS_SUPER_PROP_REFERENCE' flags should be removed.
         *
         * Similar case:  "super[propname].prop2(...)"
         */
        context_p->status_flags &= (uint32_t) ~PARSER_CLASS_SUPER_PROP_REFERENCE;
      }
      property_accessed = true;
    }
#endif /* ENABLED (JERRY_ES2015) */

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
                      && context_p->lit_object.literal_p->type == LEXER_STRING_LITERAL);
        context_p->token.lit_location.type = LEXER_STRING_LITERAL;

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

        if (PARSER_IS_MUTABLE_PUSH_LITERAL (context_p->last_cbc_opcode))
        {
          context_p->last_cbc_opcode = PARSER_PUSH_LITERAL_TO_PUSH_PROP_LITERAL (context_p->last_cbc_opcode);
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
              && context_p->last_cbc.literal_object_type == LEXER_LITERAL_OBJECT_EVAL
              && context_p->last_cbc.literal_type == LEXER_IDENT_LITERAL)
          {
            is_eval = true;
          }

          if (PARSER_IS_PUSH_PROP (context_p->last_cbc_opcode))
          {
            opcode = CBC_CALL_PROP;
            context_p->last_cbc_opcode = PARSER_PUSH_PROP_TO_PUSH_PROP_REFERENCE (context_p->last_cbc_opcode);
          }
#if ENABLED (JERRY_ES2015)
          else if (context_p->last_cbc_opcode == PARSER_TO_EXT_OPCODE (CBC_EXT_PUSH_CONSTRUCTOR_SUPER))
          {
            opcode = PARSER_TO_EXT_OPCODE (CBC_EXT_SUPER_CALL);
          }
#endif /* ENABLED (JERRY_ES2015) */
          else if (JERRY_UNLIKELY ((context_p->status_flags & (PARSER_INSIDE_WITH | PARSER_RESOLVE_BASE_FOR_CALLS))
                                   && PARSER_IS_PUSH_LITERAL (context_p->last_cbc_opcode)
                                   && context_p->last_cbc.literal_type == LEXER_IDENT_LITERAL))
          {
            opcode = CBC_CALL_PROP;
            parser_emit_ident_reference (context_p, CBC_PUSH_IDENT_REFERENCE);
            parser_emit_cbc_ext (context_p, CBC_EXT_RESOLVE_BASE);
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
#if ENABLED (JERRY_ES2015)
          if (context_p->status_flags & PARSER_CLASS_HAS_SUPER)
          {
            parser_flush_cbc (context_p);
            context_p->last_cbc_opcode = PARSER_TO_EXT_OPCODE (CBC_EXT_CLASS_EVAL);
            context_p->last_cbc.value = PARSER_GET_CLASS_ECMA_PARSE_OPTS (context_p->status_flags);
          }
          else
          {
#endif /* ENABLED (JERRY_ES2015) */
            parser_emit_cbc (context_p, CBC_EVAL);
#if ENABLED (JERRY_ES2015)
          }
#endif /* ENABLED (JERRY_ES2015) */
        }

#if ENABLED (JERRY_ES2015)
        if ((context_p->status_flags & PARSER_CLASS_SUPER_PROP_REFERENCE) && opcode == CBC_CALL_PROP)
        {
          parser_emit_cbc_ext (context_p, CBC_EXT_SUPER_PROP_CALL);
          context_p->status_flags &= (uint32_t) ~PARSER_CLASS_SUPER_PROP_REFERENCE;
        }
#endif /* ENABLED (JERRY_ES2015) */

        if (call_arguments <= 1)
        {
          if (opcode == CBC_CALL)
          {
            parser_emit_cbc (context_p, (uint16_t) (CBC_CALL0 + (call_arguments * 6)));
            continue;
          }
          if (opcode == CBC_CALL_PROP)
          {
            parser_emit_cbc (context_p, (uint16_t) (CBC_CALL0_PROP + (call_arguments * 6)));
            continue;
          }
          if (opcode == CBC_NEW)
          {
            parser_emit_cbc (context_p, (uint16_t) (CBC_NEW0 + call_arguments));
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

        if (!(context_p->token.flags & LEXER_WAS_NEWLINE)
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
          parser_emit_ident_reference (context_p, CBC_TYPEOF_IDENT);
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

      parser_check_invalid_assign (context_p);

      uint16_t literal_index;

      if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
      {
        literal_index = context_p->last_cbc.literal_index;
        context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
      }
      else if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
      {
        literal_index = context_p->last_cbc.value;
        context_p->last_cbc_opcode = CBC_PUSH_LITERAL;
      }
      else
      {
        JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_THREE_LITERALS);
        literal_index = context_p->last_cbc.third_literal_index;
        context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
      }

      parser_stack_push_uint16 (context_p, literal_index);
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
#if ENABLED (JERRY_ES2015)
        if (context_p->status_flags & PARSER_CLASS_SUPER_PROP_REFERENCE)
        {
          parser_emit_cbc_ext (context_p, CBC_EXT_SUPER_PROP_ASSIGN);
          parser_flush_cbc (context_p);
        }
        context_p->status_flags &= (uint32_t) ~PARSER_CLASS_SUPER_PROP_REFERENCE;
#endif /* ENABLED (JERRY_ES2015) */
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
      parser_check_invalid_assign (context_p);

      parser_emit_ident_reference (context_p, CBC_PUSH_IDENT_REFERENCE);
    }
    else if (PARSER_IS_PUSH_PROP (context_p->last_cbc_opcode))
    {
      context_p->last_cbc_opcode = PARSER_PUSH_PROP_TO_PUSH_PROP_REFERENCE (context_p->last_cbc_opcode);
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

      if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL
          && opcode == CBC_ASSIGN_SET_IDENT)
      {
        JERRY_ASSERT (CBC_ARGS_EQ (CBC_ASSIGN_LITERAL_SET_IDENT,
                                   CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2));
        context_p->last_cbc.value = parser_stack_pop_uint16 (context_p);
        context_p->last_cbc_opcode = CBC_ASSIGN_LITERAL_SET_IDENT;
        continue;
      }

      if (cbc_flags[opcode] & CBC_HAS_LITERAL_ARG)
      {
        uint16_t index = parser_stack_pop_uint16 (context_p);
        parser_emit_cbc_literal (context_p, (uint16_t) opcode, index);

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
      parser_stack_push_uint8 (context_p, CBC_ASSIGN);
      parser_stack_push_uint8 (context_p, LEXER_ASSIGN);
      parser_stack_push_uint8 (context_p, lexer_convert_binary_lvalue_token_to_binary (token));
      continue;
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

      if (PARSER_IS_PUSH_NUMBER (context_p->last_cbc_opcode))
      {
        lexer_convert_push_number_to_push_literal (context_p);
      }

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
    parser_emit_cbc (context_p, (uint16_t) opcode);
  }
} /* parser_process_binary_opcodes */

/**
 * Process ternary expression.
 */
static void
parser_process_ternary_expression (parser_context_t *context_p) /**< context */
{
  JERRY_ASSERT (context_p->token.type == LEXER_QUESTION_MARK);

  cbc_opcode_t opcode = CBC_BRANCH_IF_FALSE_FORWARD;
  parser_branch_t cond_branch;
  parser_branch_t uncond_branch;

  parser_push_result (context_p);

  if (context_p->last_cbc_opcode == CBC_LOGICAL_NOT)
  {
    context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
    opcode = CBC_BRANCH_IF_TRUE_FORWARD;
  }

  parser_emit_cbc_forward_branch (context_p, (uint16_t) opcode, &cond_branch);

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
} /* parser_process_ternary_expression */

/**
 * Process expression sequence.
 */
static void
parser_process_expression_sequence (parser_context_t *context_p) /**< context */
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
} /* parser_process_expression_sequence */

/**
 * Process group expression.
 */
static void
parser_process_group_expression (parser_context_t *context_p, /**< context */
                                 size_t *grouping_level_p) /**< grouping level */
{
  JERRY_ASSERT (*grouping_level_p > 0);
  (*grouping_level_p)--;

  if (context_p->stack_top_uint8 == LEXER_COMMA_SEP_LIST)
  {
    parser_push_result (context_p);
    parser_flush_cbc (context_p);
  }

  parser_stack_pop_uint8 (context_p);
  lexer_next_token (context_p);
} /* parser_process_group_expression */

/**
 * Parse block expression.
 */
void
parser_parse_block_expression (parser_context_t *context_p, /**< context */
                               int options) /**< option flags */
{
  parser_parse_expression (context_p, options | PARSE_EXPR_NO_PUSH_RESULT);

  if (CBC_NO_RESULT_OPERATION (context_p->last_cbc_opcode))
  {
    JERRY_ASSERT (CBC_SAME_ARGS (context_p->last_cbc_opcode, context_p->last_cbc_opcode + 2));
    PARSER_PLUS_EQUAL_U16 (context_p->last_cbc_opcode, 2);
    parser_flush_cbc (context_p);
  }
  else
  {
    parser_emit_cbc (context_p, CBC_POP_BLOCK);
  }
} /* parser_parse_block_expression */

/**
 * Parse expression statement.
 */
void
parser_parse_expression_statement (parser_context_t *context_p, /**< context */
                                   int options) /**< option flags */
{
  parser_parse_expression (context_p, options | PARSE_EXPR_NO_PUSH_RESULT);

  if (!CBC_NO_RESULT_OPERATION (context_p->last_cbc_opcode))
  {
    parser_emit_cbc (context_p, CBC_POP);
  }
} /* parser_parse_expression_statement */

/**
 * Parse expression.
 */
void
parser_parse_expression (parser_context_t *context_p, /**< context */
                         int options) /**< option flags */
{
  size_t grouping_level = 0;

  parser_stack_push_uint8 (context_p, LEXER_EXPRESSION_START);

#if ENABLED (JERRY_ES2015)
  /* Parsing a new expression:
   *  So save, remove, and at the end restore the super prop reference indicator.
   *
   * If this is not done, it is possible to carry the flag over to the next expression.
   */
  bool has_super_ref = (context_p->status_flags & PARSER_CLASS_SUPER_PROP_REFERENCE);
  context_p->status_flags &= (uint32_t) ~PARSER_CLASS_SUPER_PROP_REFERENCE;
#endif

  if (options & PARSE_EXPR_HAS_LITERAL)
  {
    JERRY_ASSERT (context_p->last_cbc_opcode == CBC_PUSH_LITERAL);
    goto process_unary_expression;
  }

  while (true)
  {
    parser_parse_unary_expression (context_p, &grouping_level);

    while (true)
    {
process_unary_expression:
      parser_process_unary_expression (context_p);

      uint8_t min_prec_treshold = 0;

      if (LEXER_IS_BINARY_OP_TOKEN (context_p->token.type))
      {
        min_prec_treshold = parser_binary_precedence_table[context_p->token.type - LEXER_FIRST_BINARY_OP];

        /* Check for BINARY_LVALUE tokens + LEXER_LOGICAL_OR + LEXER_LOGICAL_AND */
        if (min_prec_treshold <= PARSER_RIGHT_TO_LEFT_ORDER_MAX_PRECEDENCE
            && min_prec_treshold != PARSER_RIGHT_TO_LEFT_ORDER_TERNARY_PRECEDENCE)
        {
          /* Right-to-left evaluation order. */
          min_prec_treshold++;
        }
      }

      parser_process_binary_opcodes (context_p, min_prec_treshold);

      if (context_p->token.type == LEXER_RIGHT_PAREN
          && (context_p->stack_top_uint8 == LEXER_LEFT_PAREN
              || context_p->stack_top_uint8 == LEXER_COMMA_SEP_LIST))
      {
        parser_process_group_expression (context_p, &grouping_level);
        continue;
      }

      if (context_p->token.type == LEXER_QUESTION_MARK)
      {
        parser_process_ternary_expression (context_p);
        continue;
      }
      break;
    }

    if (JERRY_UNLIKELY (context_p->token.type == LEXER_COMMA)
                        && (!(options & PARSE_EXPR_NO_COMMA) || grouping_level > 0))
    {
      parser_process_expression_sequence (context_p);
      continue;
    }

    if (LEXER_IS_BINARY_OP_TOKEN (context_p->token.type))
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

  if (!(options & PARSE_EXPR_NO_PUSH_RESULT))
  {
    parser_push_result (context_p);
  }

#if ENABLED (JERRY_ES2015)
  /* Restore the super prop ref flag. */
  if (has_super_ref)
  {
    context_p->status_flags |= (uint32_t) PARSER_CLASS_SUPER_PROP_REFERENCE;
  }
#endif /* ENABLED (JERRY_ES2015) */
} /* parser_parse_expression */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_PARSER) */

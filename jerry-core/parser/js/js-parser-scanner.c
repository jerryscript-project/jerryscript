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
#include "lit-char-helpers.h"

#ifndef JERRY_DISABLE_JS_PARSER

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup jsparser JavaScript
 * @{
 *
 * \addtogroup jsparser_scanner Scanner
 * @{
 */

/**
 * Scan mode types types.
 */
typedef enum
{
  SCAN_MODE_PRIMARY_EXPRESSION,            /**< scanning primary expression */
  SCAN_MODE_PRIMARY_EXPRESSION_AFTER_NEW,  /**< scanning primary expression after new */
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
  SCAN_MODE_ARROW_FUNCTION,                /**< arrow function might follows */
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
  SCAN_MODE_POST_PRIMARY_EXPRESSION,       /**< scanning post primary expression */
  SCAN_MODE_PRIMARY_EXPRESSION_END,        /**< scanning primary expression end */
  SCAN_MODE_STATEMENT,                     /**< scanning statement */
  SCAN_MODE_FUNCTION_ARGUMENTS,            /**< scanning function arguments */
  SCAN_MODE_PROPERTY_NAME,                 /**< scanning property name */
#if ENABLED (JERRY_ES2015_CLASS)
  SCAN_MODE_CLASS_DECLARATION,             /**< scanning class declaration */
  SCAN_MODE_CLASS_METHOD,                  /**< scanning class method */
#endif /* ENABLED (JERRY_ES2015_CLASS) */
} scan_modes_t;

/**
 * Scan stack mode types types.
 */
typedef enum
{
  SCAN_STACK_HEAD,                         /**< head */
  SCAN_STACK_PAREN_EXPRESSION,             /**< parent expression group */
  SCAN_STACK_PAREN_STATEMENT,              /**< parent statement group */
  SCAN_STACK_COLON_EXPRESSION,             /**< colon expression group */
  SCAN_STACK_COLON_STATEMENT,              /**< colon statement group */
  SCAN_STACK_SQUARE_BRACKETED_EXPRESSION,  /**< square bracketed expression group */
  SCAN_STACK_OBJECT_LITERAL,               /**< object literal group */
  SCAN_STACK_BLOCK_STATEMENT,              /**< block statement group */
  SCAN_STACK_BLOCK_EXPRESSION,             /**< block expression group */
  SCAN_STACK_BLOCK_PROPERTY,               /**< block property group */
#if ENABLED (JERRY_ES2015_OBJECT_INITIALIZER)
  SCAN_STACK_COMPUTED_PROPERTY,            /**< computed property name */
#endif /* ENABLED (JERRY_ES2015_OBJECT_INITIALIZER) */
#if ENABLED (JERRY_ES2015_TEMPLATE_STRINGS)
  SCAN_STACK_TEMPLATE_STRING,              /**< template string */
#endif /* ENABLED (JERRY_ES2015_TEMPLATE_STRINGS) */
#if ENABLED (JERRY_ES2015_CLASS)
  SCAN_STACK_CLASS,                        /**< class language element */
  SCAN_STACK_CLASS_EXTENDS,                /**< class extends expression */
#endif /* ENABLED (JERRY_ES2015_CLASS) */
#if ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER)
  SCAN_STACK_FUNCTION_PARAMETERS,          /**< function parameter initializer */
#endif /* ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER) */
} scan_stack_modes_t;

/**
 * Scan primary expression.
 *
 * @return true for continue, false for break
 */
static bool
parser_scan_primary_expression (parser_context_t *context_p, /**< context */
                                lexer_token_type_t type, /**< current token type */
                                scan_stack_modes_t stack_top, /**< current stack top */
                                scan_modes_t *mode) /**< scan mode */
{
  switch (type)
  {
    case LEXER_KEYW_NEW:
    {
      *mode = SCAN_MODE_PRIMARY_EXPRESSION_AFTER_NEW;
      break;
    }
    case LEXER_DIVIDE:
    case LEXER_ASSIGN_DIVIDE:
    {
      lexer_construct_regexp_object (context_p, true);
      *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_KEYW_FUNCTION:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_EXPRESSION);
      *mode = SCAN_MODE_FUNCTION_ARGUMENTS;
      break;
    }
    case LEXER_LEFT_PAREN:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_PAREN_EXPRESSION);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_LEFT_SQUARE:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_SQUARE_BRACKETED_EXPRESSION);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_LEFT_BRACE:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_OBJECT_LITERAL);
      *mode = SCAN_MODE_PROPERTY_NAME;
      return true;
    }
#if ENABLED (JERRY_ES2015_TEMPLATE_STRINGS)
    case LEXER_TEMPLATE_LITERAL:
    {
      if (context_p->source_p[-1] != LIT_CHAR_GRAVE_ACCENT)
      {
        parser_stack_push_uint8 (context_p, SCAN_STACK_TEMPLATE_STRING);
        *mode = SCAN_MODE_PRIMARY_EXPRESSION;
        break;
      }

      /* The string is a normal string literal. */
      /* FALLTHRU */
    }
#endif /* ENABLED (JERRY_ES2015_TEMPLATE_STRINGS) */
    case LEXER_LITERAL:
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
    {
      bool is_ident = (context_p->token.lit_location.type == LEXER_IDENT_LITERAL);

      *mode = (is_ident ? SCAN_MODE_ARROW_FUNCTION
                        : SCAN_MODE_POST_PRIMARY_EXPRESSION);
      break;
    }
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
    case LEXER_KEYW_THIS:
    case LEXER_LIT_TRUE:
    case LEXER_LIT_FALSE:
    case LEXER_LIT_NULL:
    {
      *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      break;
    }
#if ENABLED (JERRY_ES2015_CLASS)
    case LEXER_KEYW_CLASS:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_EXPRESSION);
      *mode = SCAN_MODE_CLASS_DECLARATION;
      break;
    }
#endif /* ENABLED (JERRY_ES2015_CLASS) */
    case LEXER_RIGHT_SQUARE:
    {
      if (stack_top != SCAN_STACK_SQUARE_BRACKETED_EXPRESSION)
      {
        parser_raise_error (context_p, PARSER_ERR_PRIMARY_EXP_EXPECTED);
      }
      parser_stack_pop_uint8 (context_p);
      *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_COMMA:
    {
      if (stack_top != SCAN_STACK_SQUARE_BRACKETED_EXPRESSION)
      {
        parser_raise_error (context_p, PARSER_ERR_PRIMARY_EXP_EXPECTED);
      }
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_RIGHT_PAREN:
    {
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
      *mode = SCAN_MODE_ARROW_FUNCTION;
#else /* !ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
      *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */

      if (stack_top == SCAN_STACK_PAREN_STATEMENT)
      {
        *mode = SCAN_MODE_STATEMENT;
      }
      else if (stack_top != SCAN_STACK_PAREN_EXPRESSION)
      {
        parser_raise_error (context_p, PARSER_ERR_PRIMARY_EXP_EXPECTED);
      }

      parser_stack_pop_uint8 (context_p);
      break;
    }
    case LEXER_SEMICOLON:
    {
      /* Needed by for (;;) statements. */
      if (stack_top != SCAN_STACK_PAREN_STATEMENT)
      {
        parser_raise_error (context_p, PARSER_ERR_PRIMARY_EXP_EXPECTED);
      }
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      break;
    }
    default:
    {
      parser_raise_error (context_p, PARSER_ERR_PRIMARY_EXP_EXPECTED);
    }
  }
  return false;
} /* parser_scan_primary_expression */

/**
 * Scan the tokens after the primary expression.
 *
 * @return true for break, false for fall through
 */
static bool
parser_scan_post_primary_expression (parser_context_t *context_p, /**< context */
                                     lexer_token_type_t type, /**< current token type */
                                     scan_modes_t *mode) /**< scan mode */
{
  switch (type)
  {
    case LEXER_DOT:
    {
      lexer_scan_identifier (context_p, false);
      return true;
    }
    case LEXER_LEFT_PAREN:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_PAREN_EXPRESSION);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return true;
    }
    case LEXER_LEFT_SQUARE:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_SQUARE_BRACKETED_EXPRESSION);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return true;
    }
    case LEXER_INCREASE:
    case LEXER_DECREASE:
    {
      if (!(context_p->token.flags & LEXER_WAS_NEWLINE))
      {
        *mode = SCAN_MODE_PRIMARY_EXPRESSION_END;
        return true;
      }
      /* FALLTHRU */
    }
    default:
    {
      break;
    }
  }

  return false;
} /* parser_scan_post_primary_expression */

/**
 * Scan the tokens after the primary expression.
 *
 * @return true for continue, false for break
 */
static bool
parser_scan_primary_expression_end (parser_context_t *context_p, /**< context */
                                    lexer_token_type_t type, /**< current token type */
                                    scan_stack_modes_t stack_top, /**< current stack top */
                                    lexer_token_type_t end_type, /**< terminator token type */
                                    scan_modes_t *mode) /**< scan mode */
{
  switch (type)
  {
    case LEXER_QUESTION_MARK:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_COLON_EXPRESSION);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return false;
    }
    case LEXER_COMMA:
    {
      if (stack_top == SCAN_STACK_OBJECT_LITERAL)
      {
        *mode = SCAN_MODE_PROPERTY_NAME;
        return true;
      }
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return false;
    }
    case LEXER_COLON:
    {
      if (stack_top == SCAN_STACK_COLON_EXPRESSION
          || stack_top == SCAN_STACK_COLON_STATEMENT)
      {
        if (stack_top == SCAN_STACK_COLON_EXPRESSION)
        {
          *mode = SCAN_MODE_PRIMARY_EXPRESSION;
        }
        else
        {
          *mode = SCAN_MODE_STATEMENT;
        }
        parser_stack_pop_uint8 (context_p);
        return false;
      }
      /* FALLTHRU */
    }
    default:
    {
      break;
    }
  }

  if (LEXER_IS_BINARY_OP_TOKEN (type)
      || (type == LEXER_SEMICOLON && stack_top == SCAN_STACK_PAREN_STATEMENT))
  {
    *mode = SCAN_MODE_PRIMARY_EXPRESSION;
    return false;
  }

  if ((type == LEXER_RIGHT_SQUARE && stack_top == SCAN_STACK_SQUARE_BRACKETED_EXPRESSION)
      || (type == LEXER_RIGHT_PAREN && stack_top == SCAN_STACK_PAREN_EXPRESSION)
#if ENABLED (JERRY_ES2015_CLASS)
      || (type == LEXER_LEFT_BRACE && stack_top == SCAN_STACK_CLASS_EXTENDS)
#endif /* ENABLED (JERRY_ES2015_CLASS) */
      || (type == LEXER_RIGHT_BRACE && stack_top == SCAN_STACK_OBJECT_LITERAL))
  {
    parser_stack_pop_uint8 (context_p);
    *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
    if (type == LEXER_RIGHT_PAREN)
    {
      *mode = SCAN_MODE_ARROW_FUNCTION;
    }
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
#if ENABLED (JERRY_ES2015_CLASS)
    if (stack_top == SCAN_STACK_CLASS_EXTENDS)
    {
      *mode = SCAN_MODE_CLASS_METHOD;
    }
#endif /* ENABLED (JERRY_ES2015_CLASS) */
    return false;
  }

#if ENABLED (JERRY_ES2015_TEMPLATE_STRINGS)
  if (type == LEXER_RIGHT_BRACE && stack_top == SCAN_STACK_TEMPLATE_STRING)
  {
    context_p->source_p--;
    context_p->column--;
    lexer_parse_string (context_p);

    if (context_p->source_p[-1] != LIT_CHAR_GRAVE_ACCENT)
    {
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
    }
    else
    {
      parser_stack_pop_uint8 (context_p);
      *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
    }
    return false;
  }
#endif /* ENABLED (JERRY_ES2015_TEMPLATE_STRINGS) */

  *mode = SCAN_MODE_STATEMENT;
  if (type == LEXER_RIGHT_PAREN && stack_top == SCAN_STACK_PAREN_STATEMENT)
  {
    parser_stack_pop_uint8 (context_p);
    return false;
  }

#if ENABLED (JERRY_ES2015_OBJECT_INITIALIZER)
  if (context_p->token.type == LEXER_RIGHT_SQUARE && stack_top == SCAN_STACK_COMPUTED_PROPERTY)
  {
    lexer_next_token (context_p);

    parser_stack_pop_uint8 (context_p);
    stack_top = (scan_stack_modes_t) context_p->stack_top_uint8;

    if (stack_top == SCAN_STACK_BLOCK_PROPERTY)
    {
      if (context_p->token.type != LEXER_LEFT_PAREN)
      {
        parser_raise_error (context_p, PARSER_ERR_ARGUMENT_LIST_EXPECTED);
      }

      *mode = SCAN_MODE_FUNCTION_ARGUMENTS;
      return true;
    }

    JERRY_ASSERT (stack_top == SCAN_STACK_OBJECT_LITERAL);

    if (context_p->token.type == LEXER_LEFT_PAREN)
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_PROPERTY);
      *mode = SCAN_MODE_FUNCTION_ARGUMENTS;
      return true;
    }

    if (context_p->token.type != LEXER_COLON)
    {
      parser_raise_error (context_p, PARSER_ERR_COLON_EXPECTED);
    }

    *mode = SCAN_MODE_PRIMARY_EXPRESSION;
    return false;
  }
#endif /* ENABLED (JERRY_ES2015_OBJECT_INITIALIZER) */

#if ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER)
  if (context_p->token.type == LEXER_RIGHT_PAREN && stack_top == SCAN_STACK_FUNCTION_PARAMETERS)
  {
    lexer_next_token (context_p);

    parser_stack_pop_uint8 (context_p);

    if (context_p->token.type != LEXER_LEFT_BRACE)
    {
      parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_EXPECTED);
    }
    *mode = SCAN_MODE_STATEMENT;
    return false;
  }
#endif /* ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER) */

  /* Check whether we can enter to statement mode. */
  if (stack_top != SCAN_STACK_BLOCK_STATEMENT
      && stack_top != SCAN_STACK_BLOCK_EXPRESSION
      && stack_top != SCAN_STACK_BLOCK_PROPERTY
#if ENABLED (JERRY_ES2015_CLASS)
      && stack_top != SCAN_STACK_CLASS
#endif /* ENABLED (JERRY_ES2015_CLASS) */
      && !(stack_top == SCAN_STACK_HEAD && end_type == LEXER_SCAN_SWITCH))
  {
    parser_raise_error (context_p, PARSER_ERR_INVALID_EXPRESSION);
  }

  if (type == LEXER_RIGHT_BRACE
      || (context_p->token.flags & LEXER_WAS_NEWLINE))
  {
    return true;
  }

  if (type != LEXER_SEMICOLON)
  {
    parser_raise_error (context_p, PARSER_ERR_INVALID_EXPRESSION);
  }

  return false;
} /* parser_scan_primary_expression_end */

/**
 * Scan statements.
 *
 * @return true for continue, false for break
 */
static bool
parser_scan_statement (parser_context_t *context_p, /**< context */
                       lexer_token_type_t type, /**< current token type */
                       scan_stack_modes_t stack_top, /**< current stack top */
                       scan_modes_t *mode) /**< scan mode */
{
  switch (type)
  {
    case LEXER_SEMICOLON:
    case LEXER_KEYW_ELSE:
    case LEXER_KEYW_DO:
    case LEXER_KEYW_TRY:
    case LEXER_KEYW_FINALLY:
    case LEXER_KEYW_DEBUGGER:
    {
      return false;
    }
    case LEXER_KEYW_IF:
    case LEXER_KEYW_WHILE:
    case LEXER_KEYW_WITH:
    case LEXER_KEYW_SWITCH:
    case LEXER_KEYW_CATCH:
    {
      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_LEFT_PAREN)
      {
        parser_raise_error (context_p, PARSER_ERR_LEFT_PAREN_EXPECTED);
      }

      parser_stack_push_uint8 (context_p, SCAN_STACK_PAREN_STATEMENT);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return false;
    }
    case LEXER_KEYW_FOR:
    {
      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_LEFT_PAREN)
      {
        parser_raise_error (context_p, PARSER_ERR_LEFT_PAREN_EXPECTED);
      }

      lexer_next_token (context_p);
      parser_stack_push_uint8 (context_p, SCAN_STACK_PAREN_STATEMENT);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;

      if (context_p->token.type == LEXER_KEYW_VAR)
      {
        return false;
      }
      return true;
    }
    case LEXER_KEYW_VAR:
    case LEXER_KEYW_THROW:
    {
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return false;
    }
    case LEXER_KEYW_RETURN:
    {
      lexer_next_token (context_p);
      if (!(context_p->token.flags & LEXER_WAS_NEWLINE)
          && context_p->token.type != LEXER_SEMICOLON
          && context_p->token.type != LEXER_RIGHT_BRACE)
      {
        *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      }
      return true;
    }
    case LEXER_KEYW_BREAK:
    case LEXER_KEYW_CONTINUE:
    {
      lexer_next_token (context_p);
      if (!(context_p->token.flags & LEXER_WAS_NEWLINE)
          && context_p->token.type == LEXER_LITERAL
          && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
      {
        return false;
      }
      return true;
    }
    case LEXER_KEYW_DEFAULT:
    {
      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_COLON)
      {
        parser_raise_error (context_p, PARSER_ERR_COLON_EXPECTED);
      }
      return false;
    }
    case LEXER_KEYW_CASE:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_COLON_STATEMENT);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return false;
    }
    case LEXER_RIGHT_BRACE:
    {
      if (stack_top == SCAN_STACK_BLOCK_STATEMENT
          || stack_top == SCAN_STACK_BLOCK_EXPRESSION
#if ENABLED (JERRY_ES2015_CLASS)
          || stack_top == SCAN_STACK_CLASS
#endif /* ENABLED (JERRY_ES2015_CLASS) */
          || stack_top == SCAN_STACK_BLOCK_PROPERTY)
      {
        parser_stack_pop_uint8 (context_p);

        if (stack_top == SCAN_STACK_BLOCK_EXPRESSION)
        {
          *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
        }
#if ENABLED (JERRY_ES2015_CLASS)
        else if (stack_top == SCAN_STACK_CLASS)
        {
          *mode = SCAN_MODE_CLASS_METHOD;
        }
#endif /* ENABLED (JERRY_ES2015_CLASS) */
        else if (stack_top == SCAN_STACK_BLOCK_PROPERTY)
        {
          *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
          lexer_next_token (context_p);
          if (context_p->token.type != LEXER_COMMA
              && context_p->token.type != LEXER_RIGHT_BRACE)
          {
            parser_raise_error (context_p, PARSER_ERR_OBJECT_ITEM_SEPARATOR_EXPECTED);
          }
          return true;
        }
        return false;
      }
      break;
    }
    case LEXER_LEFT_BRACE:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_STATEMENT);
      return false;
    }
    case LEXER_KEYW_FUNCTION:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_STATEMENT);
      *mode = SCAN_MODE_FUNCTION_ARGUMENTS;
      return false;
    }
#if ENABLED (JERRY_ES2015_CLASS)
    case LEXER_KEYW_CLASS:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_STATEMENT);
      *mode = SCAN_MODE_CLASS_DECLARATION;
      return false;
    }
#endif /* ENABLED (JERRY_ES2015_CLASS) */
    default:
    {
      break;
    }
  }

  *mode = SCAN_MODE_PRIMARY_EXPRESSION;

  if (type == LEXER_LITERAL
      && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
  {
    lexer_next_token (context_p);
    if (context_p->token.type == LEXER_COLON)
    {
      *mode = SCAN_MODE_STATEMENT;
      return false;
    }
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
    *mode = SCAN_MODE_ARROW_FUNCTION;
#else /* !ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
    *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
  }

  return true;
} /* parser_scan_statement */

/**
 * Pre-scan for token(s).
 */
void
parser_scan_until (parser_context_t *context_p, /**< context */
                   lexer_range_t *range_p, /**< destination range */
                   lexer_token_type_t end_type) /**< terminator token type */
{
  scan_modes_t mode;
  lexer_token_type_t end_type_b = end_type;

  range_p->source_p = context_p->source_p;
  range_p->source_end_p = context_p->source_p;
  range_p->line = context_p->line;
  range_p->column = context_p->column;

  mode = SCAN_MODE_PRIMARY_EXPRESSION;

  if (end_type == LEXER_KEYW_CASE)
  {
    end_type = LEXER_SCAN_SWITCH;
    end_type_b = LEXER_SCAN_SWITCH;
    mode = SCAN_MODE_STATEMENT;
  }
  else
  {
    lexer_next_token (context_p);

#if ENABLED (JERRY_ES2015_FOR_OF)
    lexer_token_type_t for_in_of_token = LEXER_FOR_IN_OF;
#else /* !ENABLED (JERRY_ES2015_FOR_OF) */
    lexer_token_type_t for_in_of_token = LEXER_KEYW_IN;
#endif /* ENABLED (JERRY_ES2015_FOR_OF) */

    if (end_type == for_in_of_token)
    {
      end_type_b = LEXER_SEMICOLON;
      if (context_p->token.type == LEXER_KEYW_VAR)
      {
        lexer_next_token (context_p);
      }
    }
  }

  parser_stack_push_uint8 (context_p, SCAN_STACK_HEAD);

  while (true)
  {
    lexer_token_type_t type = (lexer_token_type_t) context_p->token.type;
    scan_stack_modes_t stack_top = (scan_stack_modes_t) context_p->stack_top_uint8;

    if (type == LEXER_EOS)
    {
      parser_raise_error (context_p, PARSER_ERR_EXPRESSION_EXPECTED);
    }

    if (stack_top == SCAN_STACK_HEAD)
    {
      if (type == end_type || type == end_type_b)
      {
        parser_stack_pop_uint8 (context_p);
        return;
      }

#if ENABLED (JERRY_ES2015_FOR_OF)
      if (end_type == LEXER_FOR_IN_OF)
      {
        if (type == LEXER_KEYW_IN)
        {
          parser_stack_pop_uint8 (context_p);
          context_p->token.type = LEXER_KEYW_IN;
          return;
        }
        else if (type == LEXER_LITERAL && lexer_compare_raw_identifier_to_current (context_p, "of", 2))
        {
          parser_stack_pop_uint8 (context_p);
          context_p->token.type = LEXER_LITERAL_OF;
          return;
        }
      }
#endif /* ENABLED (JERRY_ES2015_FOR_OF) */
    }

    switch (mode)
    {
      case SCAN_MODE_PRIMARY_EXPRESSION:
      {
        if (type == LEXER_ADD
            || type == LEXER_SUBTRACT
            || LEXER_IS_UNARY_OP_TOKEN (type))
        {
          break;
        }
        /* FALLTHRU */
      }
      case SCAN_MODE_PRIMARY_EXPRESSION_AFTER_NEW:
      {
        if (parser_scan_primary_expression (context_p, type, stack_top, &mode))
        {
          continue;
        }
        break;
      }
#if ENABLED (JERRY_ES2015_CLASS)
      case SCAN_MODE_CLASS_DECLARATION:
      {
        if (context_p->token.type == LEXER_LITERAL && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
        {
          lexer_next_token (context_p);
        }

        if (context_p->token.type == LEXER_KEYW_EXTENDS)
        {
          parser_stack_push_uint8 (context_p, SCAN_STACK_CLASS_EXTENDS);
          mode = SCAN_MODE_PRIMARY_EXPRESSION;
          break;
        }
        else if (context_p->token.type != LEXER_LEFT_BRACE)
        {
          parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_EXPECTED);
        }

        mode = SCAN_MODE_CLASS_METHOD;
        break;
      }
      case SCAN_MODE_CLASS_METHOD:
      {
        if (type == LEXER_SEMICOLON)
        {
          break;
        }

        if (type == LEXER_RIGHT_BRACE
            && (stack_top == SCAN_STACK_BLOCK_STATEMENT
                || stack_top == SCAN_STACK_BLOCK_EXPRESSION))
        {
          mode = (stack_top == SCAN_STACK_BLOCK_EXPRESSION) ? SCAN_MODE_PRIMARY_EXPRESSION_END : SCAN_MODE_STATEMENT;
          parser_stack_pop_uint8 (context_p);
          break;
        }

        if (lexer_compare_raw_identifier_to_current (context_p, "static", 6))
        {
          lexer_next_token (context_p);
        }

        if (lexer_compare_raw_identifier_to_current (context_p, "get", 3)
            || lexer_compare_raw_identifier_to_current (context_p, "set", 3))
        {
          lexer_next_token (context_p);
        }

        parser_stack_push_uint8 (context_p, SCAN_STACK_CLASS);
        mode = SCAN_MODE_FUNCTION_ARGUMENTS;
        continue;
      }
#endif /* ENABLED (JERRY_ES2015_CLASS) */
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
      case SCAN_MODE_ARROW_FUNCTION:
      {
        if (type == LEXER_ARROW)
        {
          lexer_next_token (context_p);

          if (context_p->token.type == LEXER_LEFT_BRACE)
          {
            parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_EXPRESSION);
            mode = SCAN_MODE_STATEMENT;
          }
          else
          {
            mode = SCAN_MODE_PRIMARY_EXPRESSION;
            range_p->source_end_p = context_p->source_p;
            continue;
          }
          break;
        }
        mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
        /* FALLTHRU */
      }
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
      case SCAN_MODE_POST_PRIMARY_EXPRESSION:
      {
        if (parser_scan_post_primary_expression (context_p, type, &mode))
        {
          break;
        }
        /* FALLTHRU */
      }
      case SCAN_MODE_PRIMARY_EXPRESSION_END:
      {
        if (parser_scan_primary_expression_end (context_p, type, stack_top, end_type, &mode))
        {
          continue;
        }
        break;
      }
      case SCAN_MODE_STATEMENT:
      {
        if (end_type == LEXER_SCAN_SWITCH
            && stack_top == SCAN_STACK_HEAD
            && (type == LEXER_KEYW_DEFAULT || type == LEXER_KEYW_CASE || type == LEXER_RIGHT_BRACE))
        {
          parser_stack_pop_uint8 (context_p);
          return;
        }

        if (parser_scan_statement (context_p, type, stack_top, &mode))
        {
          continue;
        }
        break;
      }
      case SCAN_MODE_FUNCTION_ARGUMENTS:
      {
#if ENABLED (JERRY_ES2015_CLASS)
        JERRY_ASSERT (stack_top == SCAN_STACK_BLOCK_STATEMENT
                      || stack_top == SCAN_STACK_BLOCK_EXPRESSION
                      || stack_top == SCAN_STACK_CLASS
                      || stack_top == SCAN_STACK_BLOCK_PROPERTY);
#else /* !ENABLED (JERRY_ES2015_CLASS) */
        JERRY_ASSERT (stack_top == SCAN_STACK_BLOCK_STATEMENT
                      || stack_top == SCAN_STACK_BLOCK_EXPRESSION
                      || stack_top == SCAN_STACK_BLOCK_PROPERTY);
#endif /* ENABLED (JERRY_ES2015_CLASS) */

        if (context_p->token.type == LEXER_LITERAL
            && (context_p->token.lit_location.type == LEXER_IDENT_LITERAL
                || context_p->token.lit_location.type == LEXER_STRING_LITERAL
                || context_p->token.lit_location.type == LEXER_NUMBER_LITERAL))
        {
          lexer_next_token (context_p);
        }

        if (context_p->token.type != LEXER_LEFT_PAREN)
        {
          parser_raise_error (context_p, PARSER_ERR_ARGUMENT_LIST_EXPECTED);
        }
        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_RIGHT_PAREN)
        {
          while (true)
          {
#if ENABLED (JERRY_ES2015_FUNCTION_REST_PARAMETER)
            if (context_p->token.type == LEXER_THREE_DOTS)
            {
              lexer_next_token (context_p);
            }
#endif /* ENABLED (JERRY_ES2015_FUNCTION_REST_PARAMETER) */

            if (context_p->token.type != LEXER_LITERAL
                || context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
            {
              parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
            }
            lexer_next_token (context_p);

            if (context_p->token.type != LEXER_COMMA)
            {
              break;
            }
            lexer_next_token (context_p);
          }
        }

#if ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER)
        if (context_p->token.type == LEXER_ASSIGN)
        {
          parser_stack_push_uint8 (context_p, SCAN_STACK_FUNCTION_PARAMETERS);
          mode = SCAN_MODE_PRIMARY_EXPRESSION;
          break;
        }
#endif /* ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER) */

        if (context_p->token.type != LEXER_RIGHT_PAREN)
        {
          parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
        }

        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_LEFT_BRACE)
        {
          parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_EXPECTED);
        }
        mode = SCAN_MODE_STATEMENT;
        break;
      }
      case SCAN_MODE_PROPERTY_NAME:
      {
        JERRY_ASSERT (stack_top == SCAN_STACK_OBJECT_LITERAL);

        lexer_scan_identifier (context_p, true);

#if ENABLED (JERRY_ES2015_OBJECT_INITIALIZER)
        if (context_p->token.type == LEXER_LEFT_SQUARE)
        {
          parser_stack_push_uint8 (context_p, SCAN_STACK_COMPUTED_PROPERTY);
          mode = SCAN_MODE_PRIMARY_EXPRESSION;
          break;
        }
#endif /* ENABLED (JERRY_ES2015_OBJECT_INITIALIZER) */

        if (context_p->token.type == LEXER_RIGHT_BRACE)
        {
          parser_stack_pop_uint8 (context_p);
          mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
          break;
        }

        if (context_p->token.type == LEXER_PROPERTY_GETTER
            || context_p->token.type == LEXER_PROPERTY_SETTER)
        {
          lexer_next_token (context_p);

          parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_PROPERTY);

#if ENABLED (JERRY_ES2015_OBJECT_INITIALIZER)
          if (context_p->token.type == LEXER_LEFT_SQUARE)
          {
            parser_stack_push_uint8 (context_p, SCAN_STACK_COMPUTED_PROPERTY);
            mode = SCAN_MODE_PRIMARY_EXPRESSION;
            break;
          }
#endif /* ENABLED (JERRY_ES2015_OBJECT_INITIALIZER) */

          if (context_p->token.type != LEXER_LITERAL)
          {
            parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
          }

          mode = SCAN_MODE_FUNCTION_ARGUMENTS;
          continue;
        }

        lexer_next_token (context_p);

#if ENABLED (JERRY_ES2015_OBJECT_INITIALIZER)
        if (context_p->token.type == LEXER_LEFT_PAREN)
        {
          parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_PROPERTY);
          mode = SCAN_MODE_FUNCTION_ARGUMENTS;
          continue;
        }

        if (context_p->token.type == LEXER_COMMA)
        {
          continue;
        }

        if (context_p->token.type == LEXER_RIGHT_BRACE)
        {
          parser_stack_pop_uint8 (context_p);
          mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
          break;
        }
#endif /* ENABLED (JERRY_ES2015_OBJECT_INITIALIZER) */

        if (context_p->token.type != LEXER_COLON)
        {
          parser_raise_error (context_p, PARSER_ERR_COLON_EXPECTED);
        }

        mode = SCAN_MODE_PRIMARY_EXPRESSION;
        break;
      }
    }

    range_p->source_end_p = context_p->source_p;
    lexer_next_token (context_p);
  }
} /* parser_scan_until */

/**
 * @}
 * @}
 * @}
 */

#endif /* !JERRY_DISABLE_JS_PARSER */

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

#if ENABLED (JERRY_PARSER)

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
  SCAN_STACK_WHILE_START,                  /**< start of "while" iterator */
  SCAN_STACK_FOR_START,                    /**< start of "for" iterator */
  SCAN_STACK_FOR_CONDITION,                /**< condition part of "for" iterator */
  SCAN_STACK_FOR_EXPRESSION,               /**< expression part of "for" iterator */
  SCAN_STACK_SWITCH_EXPRESSION,            /**< expression part of "switch" statement */
  SCAN_STACK_SWITCH_BLOCK,                 /**< block part of "switch" statement */
  SCAN_STACK_COLON_EXPRESSION,             /**< colon expression group */
  SCAN_STACK_CASE_STATEMENT,               /**< colon statement group */
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
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
  SCAN_STACK_ARROW_EXPRESSION,             /**< (possible) arrow function */
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
#if ENABLED (JERRY_ES2015_CLASS)
  SCAN_STACK_CLASS_FUNCTION,               /**< class function expression */
  SCAN_STACK_CLASS_EXTENDS,                /**< class extends expression */
#endif /* ENABLED (JERRY_ES2015_CLASS) */
#if ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER)
  SCAN_STACK_FUNCTION_PARAMETERS,          /**< function parameter initializer */
#endif /* ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER) */
} scan_stack_modes_t;

/**
 * Generic descriptor which stores only the start position.
 */
typedef struct
{
  const uint8_t *source_p; /**< start source byte */
} scanner_source_start_t;

/**
 * For statement descriptor.
 */
typedef struct
{
  union
  {
    const uint8_t *source_p; /**< start source byte */
    scanner_for_info_t *for_info_p; /**< for info */
  } u;
} scanner_for_statement_t;

/**
 * Switch statement descriptor.
 */
typedef struct
{
  scanner_case_info_t **last_case_p; /**< last case info */
} scanner_switch_statement_t;

/**
 * Scanner context.
 */
typedef struct
{
  uint8_t mode; /**< scanner mode */
  scanner_switch_statement_t active_switch_statement; /**< currently active switch statement */
} scanner_context_t;

#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)

static void
scanner_process_arrow (parser_context_t *context_p, /**< context */
                       scanner_context_t *scanner_context_p) /**< scanner context */
{
  scanner_source_start_t source_start;

  parser_stack_pop_uint8 (context_p);
  parser_stack_pop (context_p, &source_start, sizeof (scanner_source_start_t));

  lexer_next_token (context_p);

  if (context_p->token.type != LEXER_ARROW
      || (context_p->token.flags & LEXER_WAS_NEWLINE))
  {
    scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
    return;
  }

  scanner_info_t *info_p = scanner_insert_info (context_p, source_start.source_p, sizeof (scanner_info_t));
  info_p->type = SCANNER_TYPE_ARROW;

  scanner_context_p->mode = SCAN_MODE_ARROW_FUNCTION;
} /* scanner_process_arrow */

#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */

/**
 * Scan primary expression.
 *
 * @return true for continue, false for break
 */
static bool
scanner_scan_primary_expression (parser_context_t *context_p, /**< context */
                                 scanner_context_t *scanner_context_p, /* scanner context */
                                 lexer_token_type_t type, /**< current token type */
                                 scan_stack_modes_t stack_top) /**< current stack top */
{
  switch (type)
  {
    case LEXER_KEYW_NEW:
    {
      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION_AFTER_NEW;
      break;
    }
    case LEXER_DIVIDE:
    case LEXER_ASSIGN_DIVIDE:
    {
      lexer_construct_regexp_object (context_p, true);
      scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_KEYW_FUNCTION:
    {
      lexer_next_token (context_p);
      if (context_p->token.type == LEXER_LITERAL
          && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
      {
        lexer_next_token (context_p);
      }

      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_EXPRESSION);
      scanner_context_p->mode = SCAN_MODE_FUNCTION_ARGUMENTS;
      return true;
    }
    case LEXER_LEFT_PAREN:
    {
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
      scanner_source_start_t source_start;
      source_start.source_p = context_p->source_p;

      parser_stack_push (context_p, &source_start, sizeof (scanner_source_start_t));
      parser_stack_push_uint8 (context_p, SCAN_STACK_ARROW_EXPRESSION);
#else
      parser_stack_push_uint8 (context_p, SCAN_STACK_PAREN_EXPRESSION);
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_LEFT_SQUARE:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_SQUARE_BRACKETED_EXPRESSION);
      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_LEFT_BRACE:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_OBJECT_LITERAL);
      scanner_context_p->mode = SCAN_MODE_PROPERTY_NAME;
      return true;
    }
#if ENABLED (JERRY_ES2015_TEMPLATE_STRINGS)
    case LEXER_TEMPLATE_LITERAL:
    {
      if (context_p->source_p[-1] != LIT_CHAR_GRAVE_ACCENT)
      {
        parser_stack_push_uint8 (context_p, SCAN_STACK_TEMPLATE_STRING);
        scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
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

      scanner_context_p->mode = (is_ident ? SCAN_MODE_ARROW_FUNCTION
                                          : SCAN_MODE_POST_PRIMARY_EXPRESSION);
      break;
    }
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
    case LEXER_KEYW_THIS:
    case LEXER_KEYW_SUPER:
    case LEXER_LIT_TRUE:
    case LEXER_LIT_FALSE:
    case LEXER_LIT_NULL:
    {
      scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      break;
    }
#if ENABLED (JERRY_ES2015_CLASS)
    case LEXER_KEYW_CLASS:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_EXPRESSION);
      scanner_context_p->mode = SCAN_MODE_CLASS_DECLARATION;
      break;
    }
#endif /* ENABLED (JERRY_ES2015_CLASS) */
    case LEXER_RIGHT_SQUARE:
    {
      if (stack_top != SCAN_STACK_SQUARE_BRACKETED_EXPRESSION)
      {
        scanner_raise_error (context_p);
      }
      parser_stack_pop_uint8 (context_p);
      scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_COMMA:
    {
      if (stack_top != SCAN_STACK_SQUARE_BRACKETED_EXPRESSION)
      {
        scanner_raise_error (context_p);
      }
      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_RIGHT_PAREN:
    {
      switch (stack_top)
      {
        case SCAN_STACK_PAREN_STATEMENT:
        {
          scanner_context_p->mode = SCAN_MODE_STATEMENT;
          break;
        }
        case SCAN_STACK_PAREN_EXPRESSION:
        {
          scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
          break;
        }
        case SCAN_STACK_FOR_EXPRESSION:
        {
          scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION_END;
          return true;
        }
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
        case SCAN_STACK_ARROW_EXPRESSION:
        {
          scanner_process_arrow (context_p, scanner_context_p);
          return true;
        }
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
        default:
        {
          scanner_raise_error (context_p);
        }
      }

      parser_stack_pop_uint8 (context_p);
      break;
    }
    default:
    {
      scanner_raise_error (context_p);
    }
  }
  return false;
} /* scanner_scan_primary_expression */

/**
 * Scan the tokens after the primary expression.
 *
 * @return true for break, false for fall through
 */
static bool
scanner_scan_post_primary_expression (parser_context_t *context_p, /**< context */
                                      scanner_context_t *scanner_context_p, /**< scanner context */
                                      lexer_token_type_t type) /**< current token type */
{
  switch (type)
  {
    case LEXER_DOT:
    {
      lexer_scan_identifier (context_p, LEXER_SCAN_IDENT_NO_OPTS);
      return true;
    }
    case LEXER_LEFT_PAREN:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_PAREN_EXPRESSION);
      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return true;
    }
    case LEXER_LEFT_SQUARE:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_SQUARE_BRACKETED_EXPRESSION);
      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return true;
    }
    case LEXER_INCREASE:
    case LEXER_DECREASE:
    {
      if (!(context_p->token.flags & LEXER_WAS_NEWLINE))
      {
        scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION_END;
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
} /* scanner_scan_post_primary_expression */

/**
 * Scan the tokens after the primary expression.
 *
 * @return true for continue, false for break
 */
static bool
scanner_scan_primary_expression_end (parser_context_t *context_p, /**< context */
                                     scanner_context_t *scanner_context_p, /**< scanner context */
                                     lexer_token_type_t type, /**< current token type */
                                     scan_stack_modes_t stack_top) /**< current stack top */
{
  switch (type)
  {
    case LEXER_QUESTION_MARK:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_COLON_EXPRESSION);
      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return false;
    }
    case LEXER_COMMA:
    {
      if (stack_top == SCAN_STACK_OBJECT_LITERAL)
      {
        scanner_context_p->mode = SCAN_MODE_PROPERTY_NAME;
        return true;
      }
      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return false;
    }
    case LEXER_COLON:
    {
      if (stack_top == SCAN_STACK_COLON_EXPRESSION)
      {
        scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
        parser_stack_pop_uint8 (context_p);
        return false;
      }

      if (stack_top != SCAN_STACK_CASE_STATEMENT)
      {
        break;
      }

      scanner_source_start_t source_start;

      parser_stack_pop_uint8 (context_p);
      parser_stack_pop (context_p, &source_start, sizeof (scanner_source_start_t));

      scanner_location_info_t *location_info_p;
      location_info_p = (scanner_location_info_t *) scanner_insert_info (context_p,
                                                                         source_start.source_p,
                                                                         sizeof (scanner_location_info_t));
      location_info_p->info.type = SCANNER_TYPE_CASE;

      scanner_get_location (&location_info_p->location, context_p);

      scanner_context_p->mode = SCAN_MODE_STATEMENT;
      return false;
    }
    default:
    {
      break;
    }
  }

  if (LEXER_IS_BINARY_OP_TOKEN (type)
      && (type != LEXER_KEYW_IN || stack_top != SCAN_STACK_FOR_START))
  {
    scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
    return false;
  }

  switch (stack_top)
  {
    case SCAN_STACK_HEAD:
    case SCAN_STACK_SWITCH_BLOCK:
    case SCAN_STACK_BLOCK_STATEMENT:
    case SCAN_STACK_BLOCK_EXPRESSION:
    case SCAN_STACK_BLOCK_PROPERTY:
#if ENABLED (JERRY_ES2015_CLASS)
    case SCAN_STACK_CLASS_FUNCTION:
#endif /* ENABLED (JERRY_ES2015_CLASS) */
    {
      scanner_context_p->mode = SCAN_MODE_STATEMENT;

      if (type == LEXER_RIGHT_BRACE
          || (context_p->token.flags & LEXER_WAS_NEWLINE))
      {
        return true;
      }

      if (type == LEXER_SEMICOLON)
      {
        return false;
      }
      break;
    }
    case SCAN_STACK_PAREN_EXPRESSION:
    {
      if (type != LEXER_RIGHT_PAREN)
      {
        break;
      }
      scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      parser_stack_pop_uint8 (context_p);
      return false;
    }
    case SCAN_STACK_PAREN_STATEMENT:
    {
      if (type != LEXER_RIGHT_PAREN)
      {
        break;
      }

      scanner_context_p->mode = SCAN_MODE_STATEMENT;
      parser_stack_pop_uint8 (context_p);
      return false;
    }
    case SCAN_STACK_WHILE_START:
    {
      if (type != LEXER_RIGHT_PAREN)
      {
        break;
      }

      scanner_source_start_t source_start;

      parser_stack_pop_uint8 (context_p);
      parser_stack_pop (context_p, &source_start, sizeof (scanner_source_start_t));

      scanner_location_info_t *location_info_p;
      location_info_p = (scanner_location_info_t *) scanner_insert_info (context_p,
                                                                         source_start.source_p,
                                                                         sizeof (scanner_location_info_t));
      location_info_p->info.type = SCANNER_TYPE_WHILE;

      scanner_get_location (&location_info_p->location, context_p);

      scanner_context_p->mode = SCAN_MODE_STATEMENT;
      return false;
    }
    case SCAN_STACK_FOR_START:
    {
      if (type == LEXER_KEYW_IN
#if ENABLED (JERRY_ES2015_FOR_OF)
          || lexer_compare_literal_to_identifier (context_p, "of", 2)
#endif /* ENABLED (JERRY_ES2015_FOR_OF) */
          || false)
      {
        scanner_for_statement_t for_statement;

        parser_stack_pop_uint8 (context_p);
        parser_stack_pop (context_p, &for_statement, sizeof (scanner_for_statement_t));

        scanner_location_info_t *location_info;
        location_info = (scanner_location_info_t *) scanner_insert_info (context_p,
                                                                         for_statement.u.source_p,
                                                                         sizeof (scanner_location_info_t));
#if ENABLED (JERRY_ES2015_FOR_OF)
        location_info->info.type = (type == LEXER_KEYW_IN) ? SCANNER_TYPE_FOR_IN : SCANNER_TYPE_FOR_OF;
#else /* !ENABLED (JERRY_ES2015_FOR_OF) */
        location_info->info.type = SCANNER_TYPE_FOR_IN;
#endif /* ENABLED (JERRY_ES2015_FOR_OF) */

        scanner_get_location (&location_info->location, context_p);

        parser_stack_push_uint8 (context_p, SCAN_STACK_PAREN_STATEMENT);
        scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
        return false;
      }

      if (type != LEXER_SEMICOLON)
      {
        break;
      }

      scanner_for_statement_t for_statement;

      parser_stack_pop_uint8 (context_p);
      parser_stack_pop (context_p, NULL, sizeof (scanner_for_statement_t));

      for_statement.u.source_p = context_p->source_p;
      parser_stack_push (context_p, &for_statement, sizeof (scanner_for_statement_t));
      parser_stack_push_uint8 (context_p, SCAN_STACK_FOR_CONDITION);

      lexer_next_token (context_p);

      if (context_p->token.type != LEXER_SEMICOLON)
      {
        scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
        return true;
      }

      type = LEXER_SEMICOLON;
      /* FALLTHRU */
    }
    case SCAN_STACK_FOR_CONDITION:
    {
      if (type != LEXER_SEMICOLON)
      {
        break;
      }

      scanner_for_statement_t for_statement;

      parser_stack_pop_uint8 (context_p);
      parser_stack_pop (context_p, &for_statement, sizeof (scanner_for_statement_t));

      scanner_for_info_t *for_info_p;
      for_info_p = (scanner_for_info_t *) scanner_insert_info (context_p,
                                                               for_statement.u.source_p,
                                                               sizeof (scanner_for_info_t));
      for_info_p->info.type = SCANNER_TYPE_FOR;

      scanner_get_location (&for_info_p->expression_location, context_p);
      for_info_p->end_location.source_p = NULL;

      for_statement.u.for_info_p = for_info_p;

      parser_stack_push (context_p, &for_statement, sizeof (scanner_for_statement_t));
      parser_stack_push_uint8 (context_p, SCAN_STACK_FOR_EXPRESSION);

      lexer_next_token (context_p);

      if (context_p->token.type != LEXER_RIGHT_PAREN)
      {
        scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
        return true;
      }

      type = LEXER_RIGHT_PAREN;
      /* FALLTHRU */
    }
    case SCAN_STACK_FOR_EXPRESSION:
    {
      if (type != LEXER_RIGHT_PAREN)
      {
        break;
      }

      scanner_for_statement_t for_statement;

      parser_stack_pop_uint8 (context_p);
      parser_stack_pop (context_p, &for_statement, sizeof (scanner_for_statement_t));

      scanner_get_location (&for_statement.u.for_info_p->end_location, context_p);

      scanner_context_p->mode = SCAN_MODE_STATEMENT;
      return false;
    }
    case SCAN_STACK_SWITCH_EXPRESSION:
    {
      if (type != LEXER_RIGHT_PAREN)
      {
        break;
      }

      lexer_next_token (context_p);

      if (context_p->token.type != LEXER_LEFT_BRACE)
      {
        break;
      }

      parser_stack_pop_uint8 (context_p);

      scanner_switch_statement_t switch_statement = scanner_context_p->active_switch_statement;
      parser_stack_push (context_p, &switch_statement, sizeof (scanner_switch_statement_t));
      parser_stack_push_uint8 (context_p, SCAN_STACK_SWITCH_BLOCK);

      scanner_switch_info_t *switch_info_p;
      switch_info_p = (scanner_switch_info_t *) scanner_insert_info (context_p,
                                                                     context_p->source_p,
                                                                     sizeof (scanner_switch_info_t));
      switch_info_p->info.type = SCANNER_TYPE_SWITCH;
      switch_info_p->case_p = NULL;
      scanner_context_p->active_switch_statement.last_case_p = &switch_info_p->case_p;

      lexer_next_token (context_p);

      if (context_p->token.type != LEXER_RIGHT_BRACE
          && context_p->token.type != LEXER_KEYW_CASE
          && context_p->token.type != LEXER_KEYW_DEFAULT)
      {
        break;
      }

      scanner_context_p->mode = SCAN_MODE_STATEMENT;
      return true;
    }
    case SCAN_STACK_SQUARE_BRACKETED_EXPRESSION:
    {
      if (type != LEXER_RIGHT_SQUARE)
      {
        break;
      }
      scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      parser_stack_pop_uint8 (context_p);
      return false;
    }
    case SCAN_STACK_OBJECT_LITERAL:
    {
      if (type != LEXER_RIGHT_BRACE)
      {
        break;
      }
      scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      parser_stack_pop_uint8 (context_p);
      return false;
    }
#if ENABLED (JERRY_ES2015_OBJECT_INITIALIZER)
    case SCAN_STACK_COMPUTED_PROPERTY:
    {
      if (type != LEXER_RIGHT_SQUARE)
      {
        break;
      }

      lexer_next_token (context_p);

      parser_stack_pop_uint8 (context_p);
      stack_top = (scan_stack_modes_t) context_p->stack_top_uint8;

      if (stack_top == SCAN_STACK_BLOCK_PROPERTY
          || stack_top == SCAN_STACK_CLASS_FUNCTION)
      {
        scanner_context_p->mode = SCAN_MODE_FUNCTION_ARGUMENTS;
        return true;
      }

      JERRY_ASSERT (stack_top == SCAN_STACK_OBJECT_LITERAL);

      if (context_p->token.type == LEXER_LEFT_PAREN)
      {
        parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_PROPERTY);
        scanner_context_p->mode = SCAN_MODE_FUNCTION_ARGUMENTS;
        return true;
      }

      if (context_p->token.type != LEXER_COLON)
      {
        scanner_raise_error (context_p);
      }

      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return false;
    }
#endif /* ENABLED (JERRY_ES2015_OBJECT_INITIALIZER) */
#if ENABLED (JERRY_ES2015_TEMPLATE_STRINGS)
    case SCAN_STACK_TEMPLATE_STRING:
    {
      if (type != LEXER_RIGHT_BRACE)
      {
        break;
      }

      context_p->source_p--;
      context_p->column--;
      lexer_parse_string (context_p);

      if (context_p->source_p[-1] != LIT_CHAR_GRAVE_ACCENT)
      {
        scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      }
      else
      {
        parser_stack_pop_uint8 (context_p);
        scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      }
      return false;
    }
#endif /* ENABLED (JERRY_ES2015_TEMPLATE_STRINGS) */
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
    case SCAN_STACK_ARROW_EXPRESSION:
    {
      if (type != LEXER_RIGHT_PAREN)
      {
        break;
      }

      scanner_process_arrow (context_p, scanner_context_p);
      return true;
    }
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
#if ENABLED (JERRY_ES2015_CLASS)
    case SCAN_STACK_CLASS_EXTENDS:
    {
      if (type != LEXER_LEFT_BRACE)
      {
        break;
      }
      scanner_context_p->mode = SCAN_MODE_CLASS_METHOD;
      parser_stack_pop_uint8 (context_p);
      return true;
    }
#endif /* ENABLED (JERRY_ES2015_CLASS) */
#if ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER)
    case SCAN_STACK_FUNCTION_PARAMETERS:
    {
      if (type != LEXER_RIGHT_PAREN)
      {
        break;
      }

      lexer_next_token (context_p);

      if (context_p->token.type != LEXER_LEFT_BRACE)
      {
        scanner_raise_error (context_p);
      }

      scanner_context_p->mode = SCAN_MODE_STATEMENT;
      parser_stack_pop_uint8 (context_p);
      return false;
    }
#endif /* ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER) */
    default:
    {
      break;
    }
  }

  scanner_raise_error (context_p);
  return false;
} /* scanner_scan_primary_expression_end */

/**
 * Scan statements.
 *
 * @return true for continue, false for break
 */
static bool
scanner_scan_statement (parser_context_t *context_p, /**< context */
                        scanner_context_t *scanner_context_p, /**< scanner context */
                        lexer_token_type_t type, /**< current token type */
                        scan_stack_modes_t stack_top) /**< current stack top */
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
    case LEXER_KEYW_WITH:
    case LEXER_KEYW_SWITCH:
    case LEXER_KEYW_CATCH:
    {
      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_LEFT_PAREN)
      {
        scanner_raise_error (context_p);
      }

      parser_stack_push_uint8 (context_p, type != LEXER_KEYW_SWITCH ? SCAN_STACK_PAREN_STATEMENT
                                                                    : SCAN_STACK_SWITCH_EXPRESSION);
      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return false;
    }
    case LEXER_KEYW_WHILE:
    {
      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_LEFT_PAREN)
      {
        scanner_raise_error (context_p);
      }

      scanner_source_start_t source_start;
      source_start.source_p = context_p->source_p;

      parser_stack_push (context_p, &source_start, sizeof (scanner_source_start_t));
      parser_stack_push_uint8 (context_p, SCAN_STACK_WHILE_START);

      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return false;
    }
    case LEXER_KEYW_FOR:
    {
      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_LEFT_PAREN)
      {
        scanner_raise_error (context_p);
      }

      scanner_for_statement_t for_statement;
      for_statement.u.source_p = context_p->source_p;

      parser_stack_push (context_p, &for_statement, sizeof (scanner_for_statement_t));
      parser_stack_push_uint8 (context_p, SCAN_STACK_FOR_START);

      lexer_next_token (context_p);

      if (context_p->token.type == LEXER_SEMICOLON)
      {
        scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION_END;
        return true;
      }

      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      if (context_p->token.type == LEXER_KEYW_VAR)
      {
        return false;
      }
      return true;
    }
    case LEXER_KEYW_VAR:
    case LEXER_KEYW_THROW:
    {
      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return false;
    }
    case LEXER_KEYW_RETURN:
    {
      lexer_next_token (context_p);
      if (!(context_p->token.flags & LEXER_WAS_NEWLINE)
          && context_p->token.type != LEXER_SEMICOLON
          && context_p->token.type != LEXER_RIGHT_BRACE)
      {
        scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
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
    case LEXER_KEYW_CASE:
    case LEXER_KEYW_DEFAULT:
    {
      if (stack_top != SCAN_STACK_SWITCH_BLOCK)
      {
        scanner_raise_error (context_p);
      }

      scanner_case_info_t *case_info_p;
      case_info_p = (scanner_case_info_t *) scanner_malloc (context_p, sizeof (scanner_case_info_t));

      *(scanner_context_p->active_switch_statement.last_case_p) = case_info_p;
      scanner_context_p->active_switch_statement.last_case_p = &case_info_p->next_p;

      case_info_p->next_p = NULL;
      scanner_get_location (&case_info_p->location, context_p);

      if (type == LEXER_KEYW_DEFAULT)
      {
        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_COLON)
        {
          scanner_raise_error (context_p);
        }

        return false;
      }

      scanner_source_start_t source_start;
      source_start.source_p = context_p->source_p;

      parser_stack_push (context_p, &source_start, sizeof (scanner_source_start_t));
      parser_stack_push_uint8 (context_p, SCAN_STACK_CASE_STATEMENT);

      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return false;
    }
    case LEXER_RIGHT_BRACE:
    {
      switch (stack_top)
      {
        case SCAN_STACK_SWITCH_BLOCK:
        {
          scanner_switch_statement_t switch_statement;

          parser_stack_pop_uint8 (context_p);
          parser_stack_pop (context_p, &switch_statement, sizeof (scanner_switch_statement_t));

          scanner_context_p->active_switch_statement = switch_statement;

          JERRY_ASSERT (scanner_context_p->mode == SCAN_MODE_STATEMENT);
          return false;
        }
        case SCAN_STACK_BLOCK_STATEMENT:
        {
          JERRY_ASSERT (scanner_context_p->mode == SCAN_MODE_STATEMENT);
          break;
        }
        case SCAN_STACK_BLOCK_EXPRESSION:
        {
          scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
          break;
        }
        case SCAN_STACK_BLOCK_PROPERTY:
        {
          lexer_next_token (context_p);
          if (context_p->token.type != LEXER_COMMA
              && context_p->token.type != LEXER_RIGHT_BRACE)
          {
            scanner_raise_error (context_p);
          }

          scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
          parser_stack_pop_uint8 (context_p);
          return true;
        }
#if ENABLED (JERRY_ES2015_CLASS)
        case SCAN_STACK_CLASS_FUNCTION:
        {
          scanner_context_p->mode = SCAN_MODE_CLASS_METHOD;
          parser_stack_pop_uint8 (context_p);
          return true;
        }
#endif /* ENABLED (JERRY_ES2015_CLASS) */
        default:
        {
          scanner_raise_error (context_p);
        }
      }

      parser_stack_pop_uint8 (context_p);
      return false;
    }
    case LEXER_LEFT_BRACE:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_STATEMENT);
      return false;
    }
    case LEXER_KEYW_FUNCTION:
    {
      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_LITERAL
          || context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
      {
        scanner_raise_error (context_p);
      }

      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_STATEMENT);
      scanner_context_p->mode = SCAN_MODE_FUNCTION_ARGUMENTS;
      return false;
    }
#if ENABLED (JERRY_ES2015_CLASS)
    case LEXER_KEYW_CLASS:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_STATEMENT);
      scanner_context_p->mode = SCAN_MODE_CLASS_DECLARATION;
      return false;
    }
#endif /* ENABLED (JERRY_ES2015_CLASS) */
#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
    case LEXER_KEYW_IMPORT:
    {
      if (stack_top != SCAN_STACK_HEAD)
      {
        scanner_raise_error (context_p);
      }

      lexer_next_token (context_p);

      if (context_p->token.type == LEXER_LITERAL
          && context_p->token.lit_location.type == LEXER_STRING_LITERAL)
      {
        return false;
      }

      bool parse_imports = true;

      if (context_p->token.type == LEXER_LITERAL
          && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
      {
        lexer_next_token (context_p);

        if (context_p->token.type == LEXER_COMMA)
        {
          lexer_next_token (context_p);
        }
        else
        {
          parse_imports = false;
        }
      }

      if (parse_imports)
      {
        if (context_p->token.type == LEXER_MULTIPLY)
        {
          lexer_next_token (context_p);
          if (!lexer_compare_literal_to_identifier (context_p, "as", 2))
          {
            scanner_raise_error (context_p);
          }

          lexer_next_token (context_p);

          if (context_p->token.type != LEXER_LITERAL
              && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
          {
            scanner_raise_error (context_p);
          }

          lexer_next_token (context_p);
        }
        else if (context_p->token.type == LEXER_LEFT_BRACE)
        {
          lexer_next_token (context_p);

          while (context_p->token.type != LEXER_RIGHT_BRACE)
          {
            if (context_p->token.type != LEXER_LITERAL
                || context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
            {
              scanner_raise_error (context_p);
            }

            lexer_next_token (context_p);

            if (lexer_compare_literal_to_identifier (context_p, "as", 2))
            {
              lexer_next_token (context_p);

              if (context_p->token.type != LEXER_LITERAL
                  && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
              {
                scanner_raise_error (context_p);
              }

              lexer_next_token (context_p);
            }

            if (context_p->token.type != LEXER_RIGHT_BRACE)
            {
              if (context_p->token.type != LEXER_COMMA)
              {
                scanner_raise_error (context_p);
              }

              lexer_next_token (context_p);
            }
          }

          lexer_next_token (context_p);
        }
        else
        {
          scanner_raise_error (context_p);
        }
      }

      if (!lexer_compare_literal_to_identifier (context_p, "from", 4))
      {
        scanner_raise_error (context_p);
      }

      lexer_next_token (context_p);

      if (context_p->token.type != LEXER_LITERAL
          && context_p->token.lit_location.type != LEXER_STRING_LITERAL)
      {
        scanner_raise_error (context_p);
      }

      return false;
    }
    case LEXER_KEYW_EXPORT:
    {
      if (stack_top != SCAN_STACK_HEAD)
      {
        scanner_raise_error (context_p);
      }

      lexer_next_token (context_p);

      if (context_p->token.type == LEXER_KEYW_DEFAULT)
      {
        lexer_next_token (context_p);

        if (context_p->token.type == LEXER_KEYW_FUNCTION)
        {
          lexer_next_token (context_p);
          if (context_p->token.type == LEXER_LITERAL
              && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
          {
            lexer_next_token (context_p);
          }

          parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_EXPRESSION);
          scanner_context_p->mode = SCAN_MODE_FUNCTION_ARGUMENTS;
          return true;
        }
#if ENABLED (JERRY_ES2015_CLASS)
        if (context_p->token.type == LEXER_KEYW_CLASS)
        {
          return true;
        }
#endif /* ENABLED (JERRY_ES2015_CLASS) */

        scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
        return true;
      }

      if (context_p->token.type == LEXER_MULTIPLY)
      {
        lexer_next_token (context_p);
        if (!lexer_compare_literal_to_identifier (context_p, "from", 4))
        {
          scanner_raise_error (context_p);
        }

        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_LITERAL
            && context_p->token.lit_location.type == LEXER_STRING_LITERAL)
        {
          scanner_raise_error (context_p);
        }

        return false;
      }

      if (context_p->token.type == LEXER_LEFT_BRACE)
      {
        lexer_next_token (context_p);

        while (context_p->token.type != LEXER_RIGHT_BRACE)
        {
          if (context_p->token.type != LEXER_LITERAL
              || context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
          {
            scanner_raise_error (context_p);
          }

          lexer_next_token (context_p);

          if (lexer_compare_literal_to_identifier (context_p, "as", 2))
          {
            lexer_next_token (context_p);

            if (context_p->token.type != LEXER_LITERAL
                && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
            {
              scanner_raise_error (context_p);
            }

            lexer_next_token (context_p);
          }

          if (context_p->token.type != LEXER_RIGHT_BRACE)
          {
            if (context_p->token.type != LEXER_COMMA)
            {
              scanner_raise_error (context_p);
            }

            lexer_next_token (context_p);
          }
        }

        lexer_next_token (context_p);

        if (!lexer_compare_literal_to_identifier (context_p, "from", 4))
        {
          return true;
        }

        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_LITERAL
            && context_p->token.lit_location.type == LEXER_STRING_LITERAL)
        {
          scanner_raise_error (context_p);
        }

        return false;
      }

      return true;
    }
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */
    default:
    {
      break;
    }
  }

  scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;

  if (type == LEXER_LITERAL
      && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
  {
    lexer_next_token (context_p);
    if (context_p->token.type == LEXER_COLON)
    {
      scanner_context_p->mode = SCAN_MODE_STATEMENT;
      return false;
    }
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
    scanner_context_p->mode = SCAN_MODE_ARROW_FUNCTION;
#else /* !ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
    scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
  }

  return true;
} /* scanner_scan_statement */

/**
 * Scan the whole source code.
 */
void
scanner_scan_all (parser_context_t *context_p) /**< context */
{
  scanner_context_t scanner_context;

#if ENABLED (JERRY_PARSER_DUMP_BYTE_CODE)
  const uint8_t *source_start_p = context_p->source_p;

  if (context_p->is_show_opcodes)
  {
    JERRY_DEBUG_MSG ("\n--- Scanning start ---\n\n");
  }
#endif /* ENABLED (JERRY_PARSER_DUMP_BYTE_CODE) */

  scanner_context.mode = SCAN_MODE_STATEMENT;
  scanner_context.active_switch_statement.last_case_p = NULL;

  parser_stack_init (context_p);

  PARSER_TRY (context_p->try_buffer)
  {
    parser_stack_push_uint8 (context_p, SCAN_STACK_HEAD);
    lexer_next_token (context_p);

    while (true)
    {
      lexer_token_type_t type = (lexer_token_type_t) context_p->token.type;
      scan_stack_modes_t stack_top = (scan_stack_modes_t) context_p->stack_top_uint8;

      if (type == LEXER_EOS)
      {
        if (stack_top == SCAN_STACK_HEAD)
        {
          break;
        }
        scanner_raise_error (context_p);
      }

      switch (scanner_context.mode)
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
          if (scanner_scan_primary_expression (context_p, &scanner_context, type, stack_top))
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
            scanner_context.mode = SCAN_MODE_PRIMARY_EXPRESSION;
            break;
          }
          else if (context_p->token.type != LEXER_LEFT_BRACE)
          {
            scanner_raise_error (context_p);
          }

          scanner_context.mode = SCAN_MODE_CLASS_METHOD;
          /* FALLTHRU */
        }
        case SCAN_MODE_CLASS_METHOD:
        {
          JERRY_ASSERT (stack_top == SCAN_STACK_BLOCK_STATEMENT || stack_top == SCAN_STACK_BLOCK_EXPRESSION);

          lexer_scan_identifier (context_p, LEXER_SCAN_CLASS_PROPERTY);

          if (context_p->token.type == LEXER_SEMICOLON)
          {
            break;
          }

          if (context_p->token.type == LEXER_RIGHT_BRACE)
          {
            scanner_context.mode = (stack_top == SCAN_STACK_BLOCK_EXPRESSION ? SCAN_MODE_PRIMARY_EXPRESSION_END
                                                                             : SCAN_MODE_STATEMENT);
            parser_stack_pop_uint8 (context_p);
            break;
          }

          if (lexer_compare_literal_to_identifier (context_p, "static", 6))
          {
            lexer_scan_identifier (context_p, LEXER_SCAN_CLASS_PROPERTY);
          }

          if (lexer_compare_literal_to_identifier (context_p, "get", 3)
              || lexer_compare_literal_to_identifier (context_p, "set", 3))
          {
            lexer_scan_identifier (context_p, LEXER_SCAN_CLASS_PROPERTY);
          }

          parser_stack_push_uint8 (context_p, SCAN_STACK_CLASS_FUNCTION);

#if ENABLED (JERRY_ES2015_OBJECT_INITIALIZER)
          if (context_p->token.type == LEXER_LEFT_SQUARE)
          {
            parser_stack_push_uint8 (context_p, SCAN_STACK_COMPUTED_PROPERTY);
            scanner_context.mode = SCAN_MODE_PRIMARY_EXPRESSION;
            break;
          }
#endif /* ENABLED (JERRY_ES2015_OBJECT_INITIALIZER) */

          lexer_next_token (context_p);
          scanner_context.mode = SCAN_MODE_FUNCTION_ARGUMENTS;
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
              scanner_context.mode = SCAN_MODE_STATEMENT;
            }
            else
            {
              scanner_context.mode = SCAN_MODE_PRIMARY_EXPRESSION;
              continue;
            }
            break;
          }
          scanner_context.mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
          /* FALLTHRU */
        }
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
        case SCAN_MODE_POST_PRIMARY_EXPRESSION:
        {
          if (scanner_scan_post_primary_expression (context_p, &scanner_context, type))
          {
            break;
          }
          /* FALLTHRU */
        }
        case SCAN_MODE_PRIMARY_EXPRESSION_END:
        {
          if (scanner_scan_primary_expression_end (context_p, &scanner_context, type, stack_top))
          {
            continue;
          }
          break;
        }
        case SCAN_MODE_STATEMENT:
        {
          if (scanner_scan_statement (context_p, &scanner_context, type, stack_top))
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
                        || stack_top == SCAN_STACK_CLASS_FUNCTION
                        || stack_top == SCAN_STACK_BLOCK_PROPERTY);
#else /* !ENABLED (JERRY_ES2015_CLASS) */
          JERRY_ASSERT (stack_top == SCAN_STACK_BLOCK_STATEMENT
                        || stack_top == SCAN_STACK_BLOCK_EXPRESSION
                        || stack_top == SCAN_STACK_BLOCK_PROPERTY);
#endif /* ENABLED (JERRY_ES2015_CLASS) */

          if (type != LEXER_LEFT_PAREN)
          {
            scanner_raise_error (context_p);
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
                scanner_raise_error (context_p);
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
            scanner_context.mode = SCAN_MODE_PRIMARY_EXPRESSION;
            break;
          }
#endif /* ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER) */

          if (context_p->token.type != LEXER_RIGHT_PAREN)
          {
            scanner_raise_error (context_p);
          }

          lexer_next_token (context_p);

          if (context_p->token.type != LEXER_LEFT_BRACE)
          {
            scanner_raise_error (context_p);
          }
          scanner_context.mode = SCAN_MODE_STATEMENT;
          break;
        }
        case SCAN_MODE_PROPERTY_NAME:
        {
          JERRY_ASSERT (stack_top == SCAN_STACK_OBJECT_LITERAL);

          lexer_scan_identifier (context_p, LEXER_SCAN_IDENT_PROPERTY);

#if ENABLED (JERRY_ES2015_OBJECT_INITIALIZER)
          if (context_p->token.type == LEXER_LEFT_SQUARE)
          {
            parser_stack_push_uint8 (context_p, SCAN_STACK_COMPUTED_PROPERTY);
            scanner_context.mode = SCAN_MODE_PRIMARY_EXPRESSION;
            break;
          }
#endif /* ENABLED (JERRY_ES2015_OBJECT_INITIALIZER) */

          if (context_p->token.type == LEXER_RIGHT_BRACE)
          {
            parser_stack_pop_uint8 (context_p);
            scanner_context.mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
            break;
          }

          if (context_p->token.type == LEXER_PROPERTY_GETTER
              || context_p->token.type == LEXER_PROPERTY_SETTER)
          {
            lexer_scan_identifier (context_p, LEXER_SCAN_IDENT_PROPERTY | LEXER_SCAN_IDENT_NO_KEYW);

            parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_PROPERTY);

#if ENABLED (JERRY_ES2015_OBJECT_INITIALIZER)
            if (context_p->token.type == LEXER_LEFT_SQUARE)
            {
              parser_stack_push_uint8 (context_p, SCAN_STACK_COMPUTED_PROPERTY);
              scanner_context.mode = SCAN_MODE_PRIMARY_EXPRESSION;
              break;
            }
#endif /* ENABLED (JERRY_ES2015_OBJECT_INITIALIZER) */

            if (context_p->token.type != LEXER_LITERAL)
            {
              scanner_raise_error (context_p);
            }

            scanner_context.mode = SCAN_MODE_FUNCTION_ARGUMENTS;
            break;
          }

          lexer_next_token (context_p);

#if ENABLED (JERRY_ES2015_OBJECT_INITIALIZER)
          if (context_p->token.type == LEXER_LEFT_PAREN)
          {
            parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_PROPERTY);
            scanner_context.mode = SCAN_MODE_FUNCTION_ARGUMENTS;
            continue;
          }

          if (context_p->token.type == LEXER_COMMA)
          {
            continue;
          }

          if (context_p->token.type == LEXER_RIGHT_BRACE)
          {
            parser_stack_pop_uint8 (context_p);
            scanner_context.mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
            break;
          }
#endif /* ENABLED (JERRY_ES2015_OBJECT_INITIALIZER) */

          if (context_p->token.type != LEXER_COLON)
          {
            scanner_raise_error (context_p);
          }

          scanner_context.mode = SCAN_MODE_PRIMARY_EXPRESSION;
          break;
        }
      }

      lexer_next_token (context_p);
    }
  }
  PARSER_CATCH
  {
    /* Ignore the errors thrown by the lexer. */
    if (context_p->error != PARSER_ERR_OUT_OF_MEMORY)
    {
      context_p->error = PARSER_ERR_NO_ERROR;
    }
  }
  PARSER_TRY_END

  scanner_reverse_info_list (context_p);

#if ENABLED (JERRY_PARSER_DUMP_BYTE_CODE)
  if (context_p->is_show_opcodes)
  {
    scanner_info_t *info_p = context_p->next_scanner_info_p;

    while (info_p->type != SCANNER_TYPE_END)
    {
      const char *name_p = NULL;
      bool print_location = false;

      switch (info_p->type)
      {
        case SCANNER_TYPE_WHILE:
        {
          name_p = "WHILE";
          print_location = true;
          break;
        }
        case SCANNER_TYPE_FOR:
        {
          scanner_for_info_t *for_info_p = (scanner_for_info_t *) info_p;
          JERRY_DEBUG_MSG ("  FOR: source:%d expression:%d[%d:%d] end:%d[%d:%d]\n",
                           (int) (for_info_p->info.source_p - source_start_p),
                           (int) (for_info_p->expression_location.source_p - source_start_p),
                           (int) for_info_p->expression_location.line,
                           (int) for_info_p->expression_location.column,
                           (int) (for_info_p->end_location.source_p - source_start_p),
                           (int) for_info_p->end_location.line,
                           (int) for_info_p->end_location.column);
          break;
        }
        case SCANNER_TYPE_FOR_IN:
        {
          name_p = "FOR-IN";
          print_location = true;
          break;
        }
#if ENABLED (JERRY_ES2015_FOR_OF)
        case SCANNER_TYPE_FOR_OF:
        {
          name_p = "FOR-OF";
          print_location = true;
          break;
        }
#endif /* ENABLED (JERRY_ES2015_FOR_OF) */
        case SCANNER_TYPE_SWITCH:
        {
          JERRY_DEBUG_MSG ("  SWITCH: source:%d\n",
                           (int) (info_p->source_p - source_start_p));

          scanner_case_info_t *current_case_p = ((scanner_switch_info_t *) info_p)->case_p;

          while (current_case_p != NULL)
          {
            JERRY_DEBUG_MSG ("    CASE: location:%d[%d:%d]\n",
                             (int) (current_case_p->location.source_p - source_start_p),
                             (int) current_case_p->location.line,
                             (int) current_case_p->location.column);

            current_case_p = current_case_p->next_p;
          }
          break;
        }
        case SCANNER_TYPE_CASE:
        {
          name_p = "CASE";
          print_location = true;
          break;
        }
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
        case SCANNER_TYPE_ARROW:
        {
          JERRY_DEBUG_MSG ("  ARROW: source:%d\n", (int) (info_p->source_p - source_start_p));
          break;
        }
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
      }

      if (print_location)
      {
        scanner_location_info_t *location_info_p = (scanner_location_info_t *) info_p;
        JERRY_DEBUG_MSG ("  %s: source:%d location:%d[%d:%d]\n",
                         name_p,
                         (int) (location_info_p->info.source_p - source_start_p),
                         (int) (location_info_p->location.source_p - source_start_p),
                         (int) location_info_p->location.line,
                         (int) location_info_p->location.column);
      }

      info_p = info_p->next_p;
    }

    JERRY_DEBUG_MSG ("\n--- Scanning end ---\n\n");
  }
#endif /* ENABLED (JERRY_PARSER_DUMP_BYTE_CODE) */

  parser_stack_free (context_p);
} /* scanner_scan_all */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_PARSER) */

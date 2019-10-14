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
#include "js-scanner-internal.h"
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
  SCAN_MODE_POST_PRIMARY_EXPRESSION,       /**< scanning post primary expression */
  SCAN_MODE_PRIMARY_EXPRESSION_END,        /**< scanning primary expression end */
  SCAN_MODE_STATEMENT,                     /**< scanning statement */
  SCAN_MODE_STATEMENT_OR_TERMINATOR,       /**< scanning statement or statement end */
  SCAN_MODE_STATEMENT_END,                 /**< scanning statement end */
  SCAN_MODE_VAR_STATEMENT,                 /**< scanning var statement */
  SCAN_MODE_FUNCTION_ARGUMENTS,            /**< scanning function arguments */
#if ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER)
  SCAN_MODE_CONTINUE_FUNCTION_ARGUMENTS,   /**< continue scanning function arguments */
#endif /* ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER) */
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
  SCAN_STACK_SCRIPT,                       /**< script */
  SCAN_STACK_EVAL_FUNCTION,                /**< evaluated function */
  SCAN_STACK_BLOCK_STATEMENT,              /**< block statement group */
  SCAN_STACK_FUNCTION_STATEMENT,           /**< function statement */
  SCAN_STACK_FUNCTION_EXPRESSION,          /**< function expression */
  SCAN_STACK_FUNCTION_PROPERTY,            /**< function expression in an object literal or class */
  SCAN_STACK_SWITCH_BLOCK,                 /**< block part of "switch" statement */
  SCAN_STACK_IF_STATEMENT,                 /**< statement part of "if" statements */
  SCAN_STACK_DO_STATEMENT,                 /**< statement part of "do" statements */
  SCAN_STACK_DO_EXPRESSION,                /**< expression part of "do" statements */
  SCAN_STACK_WHILE_EXPRESSION,             /**< expression part of "while" iterator */
  SCAN_STACK_PAREN_EXPRESSION,             /**< expression in brackets */
  SCAN_STACK_STATEMENT_WITH_EXPR,          /**< statement which starts with expression enclosed in brackets */
  /* The SCANNER_IS_FOR_START macro needs to be updated when the following constants are reordered. */
  SCAN_STACK_VAR,                          /**< var statement */
  SCAN_STACK_FOR_VAR_START,                /**< start of "for" iterator with var statement */
  SCAN_STACK_FOR_START,                    /**< start of "for" iterator */
  SCAN_STACK_FOR_CONDITION,                /**< condition part of "for" iterator */
  SCAN_STACK_FOR_EXPRESSION,               /**< expression part of "for" iterator */
  SCAN_STACK_SWITCH_EXPRESSION,            /**< expression part of "switch" statement */
  SCAN_STACK_CASE_STATEMENT,               /**< case statement inside a switch statement */
  SCAN_STACK_COLON_EXPRESSION,             /**< expression between a question mark and colon */
  SCAN_STACK_TRY_STATEMENT,                /**< try statement */
  SCAN_STACK_CATCH_STATEMENT,              /**< catch statement */
  SCAN_STACK_SQUARE_BRACKETED_EXPRESSION,  /**< square bracketed expression group */
  SCAN_STACK_OBJECT_LITERAL,               /**< object literal group */
#if ENABLED (JERRY_ES2015_OBJECT_INITIALIZER)
  SCAN_STACK_COMPUTED_PROPERTY,            /**< computed property name */
#endif /* ENABLED (JERRY_ES2015_OBJECT_INITIALIZER) */
#if ENABLED (JERRY_ES2015_TEMPLATE_STRINGS)
  SCAN_STACK_TEMPLATE_STRING,              /**< template string */
#endif /* ENABLED (JERRY_ES2015_TEMPLATE_STRINGS) */
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
  SCAN_STACK_ARROW_ARGUMENTS,              /**< might be arguments of an arrow function */
  SCAN_STACK_ARROW_EXPRESSION,             /**< expression body of an arrow function */
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
#if ENABLED (JERRY_ES2015_CLASS)
  SCAN_STACK_CLASS_STATEMENT,              /**< class statement */
  SCAN_STACK_CLASS_EXPRESSION,             /**< class expression */
  SCAN_STACK_CLASS_EXTENDS,                /**< class extends expression */
#endif /* ENABLED (JERRY_ES2015_CLASS) */
#if ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER)
  SCAN_STACK_FUNCTION_PARAMETERS,          /**< function parameter initializer */
#endif /* ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER) */
} scan_stack_modes_t;

/**
 * Checks whether the stack top is a for statement start.
 */
#define SCANNER_IS_FOR_START(stack_top) \
  ((stack_top) >= SCAN_STACK_FOR_VAR_START && (stack_top) <= SCAN_STACK_FOR_START)

/**
 * Scan mode types types.
 */
typedef enum
{
  SCAN_NEXT_TOKEN, /**< get next token after return */
  SCAN_KEEP_TOKEN, /**< keep the current token after return */
} scan_return_types_t;

/**
 * Checks whether token type is "of".
 */
#if ENABLED (JERRY_ES2015_FOR_OF)
#define SCANNER_IDENTIFIER_IS_OF() (lexer_compare_literal_to_identifier (context_p, "of", 2))
#else
#define SCANNER_IDENTIFIER_IS_OF() (false)
#endif /* ENABLED (JERRY_ES2015_FOR_OF) */

#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)

/**
 * Init scanning the body of an arrow function.
 */
static void
scanner_check_arrow_body (parser_context_t *context_p, /**< context */
                          scanner_context_t *scanner_context_p) /**< scanner context */
{
  lexer_next_token (context_p);

  if (context_p->token.type != LEXER_LEFT_BRACE)
  {
    scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
    parser_stack_push_uint8 (context_p, SCAN_STACK_ARROW_EXPRESSION);
    return;
  }

  lexer_next_token (context_p);
  scanner_context_p->mode = SCAN_MODE_STATEMENT_OR_TERMINATOR;
  parser_stack_push_uint8 (context_p, SCAN_STACK_FUNCTION_EXPRESSION);
} /* scanner_check_arrow_body */

/**
 * Process arrow function with argument list.
 */
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

  scanner_check_arrow_body (context_p, scanner_context_p);
} /* scanner_process_arrow */

/**
 * Process arrow function with a single argument.
 */
static void
scanner_process_simple_arrow (parser_context_t *context_p, /**< context */
                              scanner_context_t *scanner_context_p, /**< scanner context */
                              const uint8_t *source_p) /**< identifier end position */
{
  scanner_info_t *info_p = scanner_insert_info (context_p, source_p, sizeof (scanner_info_t));
  info_p->type = SCANNER_TYPE_ARROW;

  /* Skip the => token, which size is two. */
  context_p->source_p += 2;
  PARSER_PLUS_EQUAL_LC (context_p->column, 2);
  context_p->token.flags = (uint8_t) (context_p->token.flags & ~LEXER_NO_SKIP_SPACES);

  scanner_check_arrow_body (context_p, scanner_context_p);
} /* scanner_process_simple_arrow */

#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */

/**
 * Scan primary expression.
 *
 * @return SCAN_NEXT_TOKEN to read the next token, or SCAN_KEEP_TOKEN to do nothing
 */
static scan_return_types_t
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

      parser_stack_push_uint8 (context_p, SCAN_STACK_FUNCTION_EXPRESSION);
      scanner_context_p->mode = SCAN_MODE_FUNCTION_ARGUMENTS;
      return SCAN_KEEP_TOKEN;
    }
    case LEXER_LEFT_PAREN:
    {
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
      scanner_source_start_t source_start;
      source_start.source_p = context_p->source_p;

      parser_stack_push (context_p, &source_start, sizeof (scanner_source_start_t));
      parser_stack_push_uint8 (context_p, SCAN_STACK_ARROW_ARGUMENTS);
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
      return SCAN_KEEP_TOKEN;
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
      const uint8_t *source_p = context_p->source_p;

      if (context_p->token.lit_location.type == LEXER_IDENT_LITERAL
          && lexer_check_arrow (context_p))
      {
        scanner_process_simple_arrow (context_p, scanner_context_p, source_p);
        return SCAN_KEEP_TOKEN;
      }
      /* FALLTHRU */
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
      parser_stack_push_uint8 (context_p, SCAN_STACK_CLASS_EXPRESSION);
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
      if (stack_top == SCAN_STACK_PAREN_EXPRESSION)
      {
        scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
        parser_stack_pop_uint8 (context_p);
        break;
      }
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
      if (stack_top == SCAN_STACK_ARROW_ARGUMENTS)
      {
        scanner_process_arrow (context_p, scanner_context_p);
        return SCAN_KEEP_TOKEN;
      }
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
      /* FALLTHRU */
    }
    default:
    {
      scanner_raise_error (context_p);
    }
  }
  return SCAN_NEXT_TOKEN;
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
 * @return SCAN_NEXT_TOKEN to read the next token, or SCAN_KEEP_TOKEN to do nothing
 */
static scan_return_types_t
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
      return SCAN_NEXT_TOKEN;
    }
    case LEXER_COMMA:
    {
      switch (stack_top)
      {
        case SCAN_STACK_OBJECT_LITERAL:
        {
          scanner_context_p->mode = SCAN_MODE_PROPERTY_NAME;
          return SCAN_KEEP_TOKEN;
        }
        case SCAN_STACK_VAR:
        case SCAN_STACK_FOR_VAR_START:
        {
          scanner_context_p->mode = SCAN_MODE_VAR_STATEMENT;
          return SCAN_NEXT_TOKEN;
        }
        case SCAN_STACK_COLON_EXPRESSION:
        {
          scanner_raise_error (context_p);
          break;
        }
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
        case SCAN_STACK_ARROW_EXPRESSION:
        {
          break;
        }
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
#if ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER)
        case SCAN_STACK_FUNCTION_PARAMETERS:
        {
          scanner_context_p->mode = SCAN_MODE_CONTINUE_FUNCTION_ARGUMENTS;
          parser_stack_pop_uint8 (context_p);
          return SCAN_NEXT_TOKEN;
        }
#endif /* ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER) */
        default:
        {
          scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
          return SCAN_NEXT_TOKEN;
        }
      }
      break;
    }
    default:
    {
      break;
    }
  }

  if (LEXER_IS_BINARY_OP_TOKEN (type)
      && (type != LEXER_KEYW_IN || !SCANNER_IS_FOR_START (stack_top)))
  {
    scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
    return SCAN_NEXT_TOKEN;
  }

  switch (stack_top)
  {
    case SCAN_STACK_VAR:
    {
      parser_stack_pop_uint8 (context_p);
      return SCAN_KEEP_TOKEN;
    }
    case SCAN_STACK_PAREN_EXPRESSION:
    {
      if (type != LEXER_RIGHT_PAREN)
      {
        break;
      }
      scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      parser_stack_pop_uint8 (context_p);
      return SCAN_NEXT_TOKEN;
    }
    case SCAN_STACK_STATEMENT_WITH_EXPR:
    {
      if (type != LEXER_RIGHT_PAREN)
      {
        break;
      }

      scanner_context_p->mode = SCAN_MODE_STATEMENT;
      parser_stack_pop_uint8 (context_p);
      return SCAN_NEXT_TOKEN;
    }
    case SCAN_STACK_DO_EXPRESSION:
    {
      if (type != LEXER_RIGHT_PAREN)
      {
        break;
      }

      scanner_context_p->mode = SCAN_MODE_STATEMENT_END;
      return SCAN_NEXT_TOKEN;
    }
    case SCAN_STACK_WHILE_EXPRESSION:
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
      return SCAN_NEXT_TOKEN;
    }
    case SCAN_STACK_FOR_VAR_START:
    case SCAN_STACK_FOR_START:
    {
      if (type == LEXER_KEYW_IN || SCANNER_IDENTIFIER_IS_OF ())
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

        parser_stack_push_uint8 (context_p, SCAN_STACK_STATEMENT_WITH_EXPR);
        scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
        return SCAN_NEXT_TOKEN;
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
        return SCAN_KEEP_TOKEN;
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
        return SCAN_KEEP_TOKEN;
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
      return SCAN_NEXT_TOKEN;
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

      scanner_context_p->mode = SCAN_MODE_STATEMENT_OR_TERMINATOR;
      return SCAN_KEEP_TOKEN;
    }
    case SCAN_STACK_CASE_STATEMENT:
    {
      if (type != LEXER_COLON)
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

      scanner_context_p->mode = SCAN_MODE_STATEMENT_OR_TERMINATOR;
      return SCAN_NEXT_TOKEN;
    }
    case SCAN_STACK_COLON_EXPRESSION:
    {
      if (type != LEXER_COLON)
      {
        break;
      }

      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      parser_stack_pop_uint8 (context_p);
      return SCAN_NEXT_TOKEN;
    }
    case SCAN_STACK_SQUARE_BRACKETED_EXPRESSION:
    {
      if (type != LEXER_RIGHT_SQUARE)
      {
        break;
      }
      scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      parser_stack_pop_uint8 (context_p);
      return SCAN_NEXT_TOKEN;
    }
    case SCAN_STACK_OBJECT_LITERAL:
    {
      if (type != LEXER_RIGHT_BRACE)
      {
        break;
      }
      scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      parser_stack_pop_uint8 (context_p);
      return SCAN_NEXT_TOKEN;
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

      if (stack_top == SCAN_STACK_FUNCTION_PROPERTY)
      {
        scanner_context_p->mode = SCAN_MODE_FUNCTION_ARGUMENTS;
        return SCAN_KEEP_TOKEN;
      }

      JERRY_ASSERT (stack_top == SCAN_STACK_OBJECT_LITERAL);

      if (context_p->token.type == LEXER_LEFT_PAREN)
      {
        parser_stack_push_uint8 (context_p, SCAN_STACK_FUNCTION_PROPERTY);
        scanner_context_p->mode = SCAN_MODE_FUNCTION_ARGUMENTS;
        return SCAN_KEEP_TOKEN;
      }

      if (context_p->token.type != LEXER_COLON)
      {
        scanner_raise_error (context_p);
      }

      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return SCAN_NEXT_TOKEN;
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
      return SCAN_NEXT_TOKEN;
    }
#endif /* ENABLED (JERRY_ES2015_TEMPLATE_STRINGS) */
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
    case SCAN_STACK_ARROW_ARGUMENTS:
    {
      if (type != LEXER_RIGHT_PAREN)
      {
        break;
      }

      scanner_process_arrow (context_p, scanner_context_p);
      return SCAN_KEEP_TOKEN;
    }
    case SCAN_STACK_ARROW_EXPRESSION:
    {
      parser_stack_pop_uint8 (context_p);
      scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      return SCAN_KEEP_TOKEN;
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
      return SCAN_KEEP_TOKEN;
    }
#endif /* ENABLED (JERRY_ES2015_CLASS) */
#if ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER)
    case SCAN_STACK_FUNCTION_PARAMETERS:
    {
      parser_stack_pop_uint8 (context_p);

      if (type != LEXER_RIGHT_PAREN
          && (type != LEXER_EOS || context_p->stack_top_uint8 != SCAN_STACK_EVAL_FUNCTION))
      {
        break;
      }

      scanner_context_p->mode = SCAN_MODE_CONTINUE_FUNCTION_ARGUMENTS;
      return SCAN_KEEP_TOKEN;
    }
#endif /* ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER) */
    default:
    {
      scanner_context_p->mode = SCAN_MODE_STATEMENT_END;
      return SCAN_KEEP_TOKEN;
    }
  }

  scanner_raise_error (context_p);
  return SCAN_NEXT_TOKEN;
} /* scanner_scan_primary_expression_end */

/**
 * Scan statements.
 *
 * @return SCAN_NEXT_TOKEN to read the next token, or SCAN_KEEP_TOKEN to do nothing
 */
static scan_return_types_t
scanner_scan_statement (parser_context_t *context_p, /**< context */
                        scanner_context_t *scanner_context_p, /**< scanner context */
                        lexer_token_type_t type, /**< current token type */
                        scan_stack_modes_t stack_top) /**< current stack top */
{
  switch (type)
  {
    case LEXER_SEMICOLON:
    {
      scanner_context_p->mode = SCAN_MODE_STATEMENT_END;
      return SCAN_KEEP_TOKEN;
    }
    case LEXER_LEFT_BRACE:
    {
      scanner_context_p->mode = SCAN_MODE_STATEMENT_OR_TERMINATOR;
      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_STATEMENT);
      return SCAN_NEXT_TOKEN;
    }
    case LEXER_KEYW_DO:
    {
      scanner_context_p->mode = SCAN_MODE_STATEMENT;
      parser_stack_push_uint8 (context_p, SCAN_STACK_DO_STATEMENT);
      return SCAN_NEXT_TOKEN;
    }
    case LEXER_KEYW_TRY:
    {
      lexer_next_token (context_p);

      if (context_p->token.type != LEXER_LEFT_BRACE)
      {
        scanner_raise_error (context_p);
      }

      scanner_context_p->mode = SCAN_MODE_STATEMENT_OR_TERMINATOR;
      parser_stack_push_uint8 (context_p, SCAN_STACK_TRY_STATEMENT);
      return SCAN_NEXT_TOKEN;
    }
    case LEXER_KEYW_DEBUGGER:
    {
      scanner_context_p->mode = SCAN_MODE_STATEMENT_END;
      return SCAN_NEXT_TOKEN;
    }
    case LEXER_KEYW_IF:
    case LEXER_KEYW_WITH:
    case LEXER_KEYW_SWITCH:
    {
      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_LEFT_PAREN)
      {
        scanner_raise_error (context_p);
      }

      uint8_t mode = SCAN_STACK_STATEMENT_WITH_EXPR;

      if (type == LEXER_KEYW_IF)
      {
        parser_stack_push_uint8 (context_p, SCAN_STACK_IF_STATEMENT);
      }
      else if (type == LEXER_KEYW_SWITCH)
      {
        mode = SCAN_STACK_SWITCH_EXPRESSION;
      }

      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      parser_stack_push_uint8 (context_p, mode);
      return SCAN_NEXT_TOKEN;
    }
    case LEXER_KEYW_WHILE:
    {
      lexer_next_token (context_p);

      if (context_p->token.type != LEXER_LEFT_PAREN)
      {
        scanner_raise_error (context_p);
      }

      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;

      scanner_source_start_t source_start;
      source_start.source_p = context_p->source_p;

      parser_stack_push (context_p, &source_start, sizeof (scanner_source_start_t));
      parser_stack_push_uint8 (context_p, SCAN_STACK_WHILE_EXPRESSION);
      return SCAN_NEXT_TOKEN;
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
      uint8_t stack_mode = SCAN_STACK_FOR_START;

      lexer_next_token (context_p);
      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;

      switch (context_p->token.type)
      {
        case LEXER_SEMICOLON:
        {
          scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION_END;
          break;
        }
        case LEXER_KEYW_VAR:
        {
          scanner_context_p->mode = SCAN_MODE_VAR_STATEMENT;
          stack_mode = SCAN_STACK_FOR_VAR_START;
          break;
        }
      }

      parser_stack_push (context_p, &for_statement, sizeof (scanner_for_statement_t));
      parser_stack_push_uint8 (context_p, stack_mode);
      return (stack_mode == SCAN_STACK_FOR_START) ? SCAN_KEEP_TOKEN : SCAN_NEXT_TOKEN;
    }
    case LEXER_KEYW_VAR:
    {
      scanner_context_p->mode = SCAN_MODE_VAR_STATEMENT;
      parser_stack_push_uint8 (context_p, SCAN_STACK_VAR);
      return SCAN_NEXT_TOKEN;
    }
    case LEXER_KEYW_THROW:
    {
      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return SCAN_NEXT_TOKEN;
    }
    case LEXER_KEYW_RETURN:
    {
      lexer_next_token (context_p);

      if (!(context_p->token.flags & LEXER_WAS_NEWLINE)
          && context_p->token.type != LEXER_SEMICOLON
          && context_p->token.type != LEXER_RIGHT_BRACE)
      {
        scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
        return SCAN_KEEP_TOKEN;
      }

      scanner_context_p->mode = SCAN_MODE_STATEMENT_END;
      return SCAN_KEEP_TOKEN;
    }
    case LEXER_KEYW_BREAK:
    case LEXER_KEYW_CONTINUE:
    {
      lexer_next_token (context_p);
      scanner_context_p->mode = SCAN_MODE_STATEMENT_END;

      if (!(context_p->token.flags & LEXER_WAS_NEWLINE)
          && context_p->token.type == LEXER_LITERAL
          && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
      {
        return SCAN_NEXT_TOKEN;
      }
      return SCAN_KEEP_TOKEN;
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

        scanner_context_p->mode = SCAN_MODE_STATEMENT_OR_TERMINATOR;
        return SCAN_NEXT_TOKEN;
      }

      scanner_source_start_t source_start;
      source_start.source_p = context_p->source_p;

      parser_stack_push (context_p, &source_start, sizeof (scanner_source_start_t));
      parser_stack_push_uint8 (context_p, SCAN_STACK_CASE_STATEMENT);

      scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return SCAN_NEXT_TOKEN;
    }
    case LEXER_KEYW_FUNCTION:
    {
      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_LITERAL
          || context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
      {
        scanner_raise_error (context_p);
      }

      scanner_context_p->mode = SCAN_MODE_FUNCTION_ARGUMENTS;
      parser_stack_push_uint8 (context_p, SCAN_STACK_FUNCTION_STATEMENT);
      return SCAN_NEXT_TOKEN;
    }
#if ENABLED (JERRY_ES2015_CLASS)
    case LEXER_KEYW_CLASS:
    {
      scanner_context_p->mode = SCAN_MODE_CLASS_DECLARATION;
      parser_stack_push_uint8 (context_p, SCAN_STACK_CLASS_STATEMENT);
      return SCAN_NEXT_TOKEN;
    }
#endif /* ENABLED (JERRY_ES2015_CLASS) */
#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
    case LEXER_KEYW_IMPORT:
    {
      if (stack_top != SCAN_STACK_SCRIPT)
      {
        scanner_raise_error (context_p);
      }

      scanner_context_p->mode = SCAN_MODE_STATEMENT_END;
      lexer_next_token (context_p);

      if (context_p->token.type == LEXER_LITERAL
          && context_p->token.lit_location.type == LEXER_STRING_LITERAL)
      {
        return SCAN_NEXT_TOKEN;
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

      return SCAN_NEXT_TOKEN;
    }
    case LEXER_KEYW_EXPORT:
    {
      if (stack_top != SCAN_STACK_SCRIPT)
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

          parser_stack_push_uint8 (context_p, SCAN_STACK_FUNCTION_STATEMENT);
          scanner_context_p->mode = SCAN_MODE_FUNCTION_ARGUMENTS;
          return SCAN_KEEP_TOKEN;
        }
#if ENABLED (JERRY_ES2015_CLASS)
        if (context_p->token.type == LEXER_KEYW_CLASS)
        {
          scanner_context_p->mode = SCAN_MODE_STATEMENT;
          return SCAN_KEEP_TOKEN;
        }
#endif /* ENABLED (JERRY_ES2015_CLASS) */

        scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
        return SCAN_KEEP_TOKEN;
      }

      scanner_context_p->mode = SCAN_MODE_STATEMENT_END;

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

        return SCAN_NEXT_TOKEN;
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
          return SCAN_KEEP_TOKEN;
        }

        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_LITERAL
            && context_p->token.lit_location.type == LEXER_STRING_LITERAL)
        {
          scanner_raise_error (context_p);
        }

        return SCAN_NEXT_TOKEN;
      }

      scanner_context_p->mode = SCAN_MODE_STATEMENT;
      return SCAN_KEEP_TOKEN;
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
    if (JERRY_UNLIKELY (lexer_check_next_character (context_p, LIT_CHAR_COLON)))
    {
      lexer_next_token (context_p);
      JERRY_ASSERT (context_p->token.type == LEXER_COLON);
      scanner_context_p->mode = SCAN_MODE_STATEMENT;
      return SCAN_NEXT_TOKEN;
    }

    JERRY_ASSERT (context_p->token.flags & LEXER_NO_SKIP_SPACES);

#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
    /* The colon needs to be checked first because the parser also checks
     * it first, and this check skips the spaces which affects source_p. */
    if (JERRY_UNLIKELY (lexer_check_arrow (context_p)))
    {
      scanner_process_simple_arrow (context_p, scanner_context_p, context_p->source_p);
      return SCAN_KEEP_TOKEN;
    }
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */

    scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
    return SCAN_NEXT_TOKEN;
  }

  return SCAN_KEEP_TOKEN;
} /* scanner_scan_statement */

/**
 * Scan statement terminator.
 *
 * @return SCAN_NEXT_TOKEN to read the next token, or SCAN_KEEP_TOKEN to do nothing
 */
static scan_return_types_t
scanner_scan_statement_end (parser_context_t *context_p, /**< context */
                            scanner_context_t *scanner_context_p, /**< scanner context */
                            lexer_token_type_t type) /**< current token type */
{
  bool terminator_found = false;

  if (type == LEXER_SEMICOLON)
  {
    lexer_next_token (context_p);
    terminator_found = true;
  }

  while (true)
  {
    type = (lexer_token_type_t) context_p->token.type;

    switch (context_p->stack_top_uint8)
    {
      case SCAN_STACK_SCRIPT:
      case SCAN_STACK_EVAL_FUNCTION:
      {
        if (type == LEXER_EOS)
        {
          return SCAN_NEXT_TOKEN;
        }
        break;
      }
      case SCAN_STACK_BLOCK_STATEMENT:
#if ENABLED (JERRY_ES2015_CLASS)
      case SCAN_STACK_CLASS_STATEMENT:
#endif /* ENABLED (JERRY_ES2015_CLASS) */
      case SCAN_STACK_FUNCTION_STATEMENT:
      {
        if (type != LEXER_RIGHT_BRACE)
        {
          break;
        }

        terminator_found = true;
        parser_stack_pop_uint8 (context_p);
        lexer_next_token (context_p);
        continue;
      }
      case SCAN_STACK_FUNCTION_EXPRESSION:
      {
        if (type != LEXER_RIGHT_BRACE)
        {
          break;
        }

        scanner_context_p->mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
        parser_stack_pop_uint8 (context_p);
        return SCAN_NEXT_TOKEN;
      }
      case SCAN_STACK_FUNCTION_PROPERTY:
      {
        if (type != LEXER_RIGHT_BRACE)
        {
          break;
        }

        parser_stack_pop_uint8 (context_p);

#if ENABLED (JERRY_ES2015_CLASS)
        if (context_p->stack_top_uint8 == SCAN_STACK_CLASS_STATEMENT
            || context_p->stack_top_uint8 == SCAN_STACK_CLASS_EXPRESSION)
        {
          scanner_context_p->mode = SCAN_MODE_CLASS_METHOD;
          return SCAN_KEEP_TOKEN;
        }
#endif /* ENABLED (JERRY_ES2015_CLASS) */

        JERRY_ASSERT (context_p->stack_top_uint8 == SCAN_STACK_OBJECT_LITERAL);

        lexer_next_token (context_p);

        if (context_p->token.type == LEXER_RIGHT_BRACE)
        {
          scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION_END;
          return SCAN_KEEP_TOKEN;
        }

        if (context_p->token.type != LEXER_COMMA)
        {
          scanner_raise_error (context_p);
        }

        scanner_context_p->mode = SCAN_MODE_PROPERTY_NAME;
        return SCAN_KEEP_TOKEN;
      }
      case SCAN_STACK_SWITCH_BLOCK:
      {
        if (type != LEXER_RIGHT_BRACE)
        {
          break;
        }

        scanner_switch_statement_t switch_statement;

        parser_stack_pop_uint8 (context_p);
        parser_stack_pop (context_p, &switch_statement, sizeof (scanner_switch_statement_t));

        scanner_context_p->active_switch_statement = switch_statement;

        terminator_found = true;
        lexer_next_token (context_p);
        continue;
      }
      case SCAN_STACK_IF_STATEMENT:
      {
        parser_stack_pop_uint8 (context_p);

        if (type == LEXER_KEYW_ELSE
            && (terminator_found || (context_p->token.flags & LEXER_WAS_NEWLINE)))
        {
          scanner_context_p->mode = SCAN_MODE_STATEMENT;
          return SCAN_NEXT_TOKEN;
        }
        continue;
      }
      case SCAN_STACK_DO_STATEMENT:
      {
        parser_stack_pop_uint8 (context_p);

        if (type != LEXER_KEYW_WHILE
            || (!terminator_found && !(context_p->token.flags & LEXER_WAS_NEWLINE)))
        {
          scanner_raise_error (context_p);
        }

        lexer_next_token (context_p);
        if (context_p->token.type != LEXER_LEFT_PAREN)
        {
          scanner_raise_error (context_p);
        }

        parser_stack_push_uint8 (context_p, SCAN_STACK_DO_EXPRESSION);
        scanner_context_p->mode = SCAN_MODE_PRIMARY_EXPRESSION;
        return SCAN_NEXT_TOKEN;
      }
      case SCAN_STACK_DO_EXPRESSION:
      {
        parser_stack_pop_uint8 (context_p);
        terminator_found = true;
        continue;
      }
      default:
      {
        JERRY_ASSERT (context_p->stack_top_uint8 == SCAN_STACK_TRY_STATEMENT
                      || context_p->stack_top_uint8 == SCAN_STACK_CATCH_STATEMENT);

        if (type != LEXER_RIGHT_BRACE)
        {
          break;
        }

        uint8_t stack_top = context_p->stack_top_uint8;
        parser_stack_pop_uint8 (context_p);
        lexer_next_token (context_p);

        /* A finally statement is optional after a try or catch statement. */
        if (context_p->token.type == LEXER_KEYW_FINALLY)
        {
          lexer_next_token (context_p);

          if (context_p->token.type != LEXER_LEFT_BRACE)
          {
            scanner_raise_error (context_p);
          }

          parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_STATEMENT);
          scanner_context_p->mode = SCAN_MODE_STATEMENT_OR_TERMINATOR;
          return SCAN_NEXT_TOKEN;
        }

        if (stack_top == SCAN_STACK_CATCH_STATEMENT)
        {
          terminator_found = true;
          continue;
        }

        /* A catch statement must be present after a try statement unless a finally is provided. */
        if (context_p->token.type != LEXER_KEYW_CATCH)
        {
          scanner_raise_error (context_p);
        }

        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_LEFT_PAREN)
        {
          scanner_raise_error (context_p);
        }

        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_LITERAL
            || context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
        {
          scanner_raise_error (context_p);
        }

        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_RIGHT_PAREN)
        {
          scanner_raise_error (context_p);
        }

        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_LEFT_BRACE)
        {
          scanner_raise_error (context_p);
        }

        parser_stack_push_uint8 (context_p, SCAN_STACK_CATCH_STATEMENT);
        scanner_context_p->mode = SCAN_MODE_STATEMENT_OR_TERMINATOR;
        return SCAN_NEXT_TOKEN;
      }
    }

    if (!terminator_found && !(context_p->token.flags & LEXER_WAS_NEWLINE))
    {
      scanner_raise_error (context_p);
    }

    scanner_context_p->mode = SCAN_MODE_STATEMENT;
    return SCAN_KEEP_TOKEN;
  }
} /* scanner_scan_statement_end */

/**
 * Scan the whole source code.
 */
void
scanner_scan_all (parser_context_t *context_p, /**< context */
                  const uint8_t *arg_list_p, /**< function argument list */
                  const uint8_t *arg_list_end_p, /**< end of argument list */
                  const uint8_t *source_p, /**< valid UTF-8 source code */
                  const uint8_t *source_end_p) /**< end of source code */
{
  scanner_context_t scanner_context;

#if ENABLED (JERRY_PARSER_DUMP_BYTE_CODE)
  if (context_p->is_show_opcodes)
  {
    JERRY_DEBUG_MSG ("\n--- Scanning start ---\n\n");
  }
#endif /* ENABLED (JERRY_PARSER_DUMP_BYTE_CODE) */

  scanner_context.active_switch_statement.last_case_p = NULL;

  parser_stack_init (context_p);

  PARSER_TRY (context_p->try_buffer)
  {
    context_p->line = 1;
    context_p->column = 1;

    if (arg_list_p == NULL)
    {
      context_p->source_p = source_p;
      context_p->source_end_p = source_end_p;

      scanner_context.mode = SCAN_MODE_STATEMENT_OR_TERMINATOR;
      parser_stack_push_uint8 (context_p, SCAN_STACK_SCRIPT);

      lexer_next_token (context_p);
    }
    else
    {
      context_p->source_p = arg_list_p;
      context_p->source_end_p = arg_list_end_p;
      scanner_context.mode = SCAN_MODE_FUNCTION_ARGUMENTS;
      parser_stack_push_uint8 (context_p, SCAN_STACK_EVAL_FUNCTION);

      /* Faking the first token. */
      context_p->token.type = LEXER_LEFT_PAREN;
    }

    while (true)
    {
      lexer_token_type_t type = (lexer_token_type_t) context_p->token.type;
      scan_stack_modes_t stack_top = (scan_stack_modes_t) context_p->stack_top_uint8;

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
          if (scanner_scan_primary_expression (context_p, &scanner_context, type, stack_top) != SCAN_NEXT_TOKEN)
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
          JERRY_ASSERT (stack_top == SCAN_STACK_CLASS_STATEMENT || stack_top == SCAN_STACK_CLASS_EXPRESSION);

          lexer_skip_empty_statements (context_p);
          lexer_scan_identifier (context_p, LEXER_SCAN_CLASS_PROPERTY);

          if (context_p->token.type == LEXER_RIGHT_BRACE)
          {
            if (stack_top == SCAN_STACK_CLASS_STATEMENT)
            {
              /* The token is kept to disallow consuming a semicolon after it. */
              scanner_context.mode = SCAN_MODE_STATEMENT_END;
              continue;
            }

            scanner_context.mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
            parser_stack_pop_uint8 (context_p);
            break;
          }

          if (lexer_compare_literal_to_identifier (context_p, "static", 6))
          {
            lexer_scan_identifier (context_p, LEXER_SCAN_CLASS_PROPERTY);
          }

          parser_stack_push_uint8 (context_p, SCAN_STACK_FUNCTION_PROPERTY);
          scanner_context.mode = SCAN_MODE_FUNCTION_ARGUMENTS;

          if (lexer_compare_literal_to_identifier (context_p, "get", 3)
              || lexer_compare_literal_to_identifier (context_p, "set", 3))
          {
            lexer_scan_identifier (context_p, LEXER_SCAN_CLASS_PROPERTY | LEXER_SCAN_CLASS_LEFT_PAREN);

            if (context_p->token.type == LEXER_LEFT_PAREN)
            {
              continue;
            }
          }

#if ENABLED (JERRY_ES2015_OBJECT_INITIALIZER)
          if (context_p->token.type == LEXER_LEFT_SQUARE)
          {
            parser_stack_push_uint8 (context_p, SCAN_STACK_COMPUTED_PROPERTY);
            scanner_context.mode = SCAN_MODE_PRIMARY_EXPRESSION;
            break;
          }
#endif /* ENABLED (JERRY_ES2015_OBJECT_INITIALIZER) */

          lexer_next_token (context_p);
          continue;
        }
#endif /* ENABLED (JERRY_ES2015_CLASS) */
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
          if (scanner_scan_primary_expression_end (context_p, &scanner_context, type, stack_top) != SCAN_NEXT_TOKEN)
          {
            continue;
          }
          break;
        }
        case SCAN_MODE_STATEMENT_OR_TERMINATOR:
        {
          if (type == LEXER_RIGHT_BRACE || type == LEXER_EOS)
          {
            scanner_context.mode = SCAN_MODE_STATEMENT_END;
            continue;
          }
          /* FALLTHRU */
        }
        case SCAN_MODE_STATEMENT:
        {
          if (scanner_scan_statement (context_p, &scanner_context, type, stack_top) != SCAN_NEXT_TOKEN)
          {
            continue;
          }
          break;
        }
        case SCAN_MODE_STATEMENT_END:
        {
          if (scanner_scan_statement_end (context_p, &scanner_context, type) != SCAN_NEXT_TOKEN)
          {
            continue;
          }

          if (context_p->token.type == LEXER_EOS)
          {
            goto scan_completed;
          }

          break;
        }
        case SCAN_MODE_VAR_STATEMENT:
        {
          if (type != LEXER_LITERAL
              || context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
          {
            scanner_raise_error (context_p);
          }

          lexer_next_token (context_p);

          switch (context_p->token.type)
          {
            case LEXER_ASSIGN:
            {
              scanner_context.mode = SCAN_MODE_PRIMARY_EXPRESSION;
              /* FALLTHRU */
            }
            case LEXER_COMMA:
            {
              lexer_next_token (context_p);
              continue;
            }
          }

          if (SCANNER_IS_FOR_START (stack_top))
          {
            if (context_p->token.type != LEXER_SEMICOLON
                && context_p->token.type != LEXER_KEYW_IN
                && !SCANNER_IDENTIFIER_IS_OF ())
            {
              scanner_raise_error (context_p);
            }

            scanner_context.mode = SCAN_MODE_PRIMARY_EXPRESSION_END;
            continue;
          }

          JERRY_ASSERT (stack_top == SCAN_STACK_VAR);

          scanner_context.mode = SCAN_MODE_STATEMENT_END;
          parser_stack_pop_uint8 (context_p);
          continue;
        }
        case SCAN_MODE_FUNCTION_ARGUMENTS:
        {
          JERRY_ASSERT (stack_top == SCAN_STACK_EVAL_FUNCTION
                        || stack_top == SCAN_STACK_FUNCTION_STATEMENT
                        || stack_top == SCAN_STACK_FUNCTION_EXPRESSION
                        || stack_top == SCAN_STACK_FUNCTION_PROPERTY);

          if (type != LEXER_LEFT_PAREN)
          {
            scanner_raise_error (context_p);
          }
          lexer_next_token (context_p);

#if ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER)
          /* FALLTHRU */
        }
        case SCAN_MODE_CONTINUE_FUNCTION_ARGUMENTS:
        {
#endif /* ENABLED (JERRY_ES2015_FUNCTION_PARAMETER_INITIALIZER) */

          if (context_p->token.type != LEXER_RIGHT_PAREN && context_p->token.type != LEXER_EOS)
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

          if (context_p->token.type == LEXER_EOS && stack_top == SCAN_STACK_EVAL_FUNCTION)
          {
            /* End of argument parsing. */
            scanner_info_t *scanner_info_p = (scanner_info_t *) scanner_malloc (context_p, sizeof (scanner_info_t));
            scanner_info_p->next_p = context_p->next_scanner_info_p;
            scanner_info_p->source_p = NULL;
            scanner_info_p->type = SCANNER_TYPE_END_ARGUMENTS;

            context_p->next_scanner_info_p = scanner_info_p;
            context_p->source_p = source_p;
            context_p->source_end_p = source_end_p;
            context_p->line = 1;
            context_p->column = 1;

            scanner_context.mode = SCAN_MODE_STATEMENT_OR_TERMINATOR;
            lexer_next_token (context_p);
            continue;
          }

          if (context_p->token.type != LEXER_RIGHT_PAREN)
          {
            scanner_raise_error (context_p);
          }

          lexer_next_token (context_p);

          if (context_p->token.type != LEXER_LEFT_BRACE)
          {
            scanner_raise_error (context_p);
          }
          scanner_context.mode = SCAN_MODE_STATEMENT_OR_TERMINATOR;
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

            parser_stack_push_uint8 (context_p, SCAN_STACK_FUNCTION_PROPERTY);

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

          JERRY_ASSERT (context_p->token.type == LEXER_LITERAL);

#if ENABLED (JERRY_ES2015_OBJECT_INITIALIZER)
          parser_line_counter_t start_line = context_p->token.line;
          parser_line_counter_t start_column = context_p->token.column;
          bool is_ident = (context_p->token.lit_location.type == LEXER_IDENT_LITERAL);
#endif /* ENABLED (JERRY_ES2015_OBJECT_INITIALIZER) */

          lexer_next_token (context_p);

#if ENABLED (JERRY_ES2015_OBJECT_INITIALIZER)
          if (context_p->token.type == LEXER_LEFT_PAREN)
          {
            parser_stack_push_uint8 (context_p, SCAN_STACK_FUNCTION_PROPERTY);
            scanner_context.mode = SCAN_MODE_FUNCTION_ARGUMENTS;
            continue;
          }

          if (is_ident
              && (context_p->token.type == LEXER_COMMA || context_p->token.type == LEXER_RIGHT_BRACE))
          {
            context_p->source_p = context_p->token.lit_location.char_p;
            context_p->line = start_line;
            context_p->column = start_column;

            lexer_next_token (context_p);

            JERRY_ASSERT (context_p->token.type != LEXER_LITERAL
                          || context_p->token.lit_location.type == LEXER_IDENT_LITERAL);

            if (context_p->token.type != LEXER_LITERAL)
            {
              scanner_raise_error (context_p);
            }

            lexer_next_token (context_p);

            if (context_p->token.type == LEXER_COMMA)
            {
              continue;
            }

            JERRY_ASSERT (context_p->token.type == LEXER_RIGHT_BRACE);

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

scan_completed:
    if (context_p->stack_top_uint8 != SCAN_STACK_SCRIPT
        && context_p->stack_top_uint8 != SCAN_STACK_EVAL_FUNCTION)
    {
      scanner_raise_error (context_p);
    }

#ifndef JERRY_NDEBUG
    context_p->status_flags |= PARSER_SCANNING_SUCCESSFUL;
#endif /* !JERRY_NDEBUG */
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
    const uint8_t *source_start_p = (arg_list_p == NULL) ? source_p : arg_list_p;

    while (info_p->type != SCANNER_TYPE_END)
    {
      const char *name_p = NULL;
      bool print_location = false;

      switch (info_p->type)
      {
        case SCANNER_TYPE_END_ARGUMENTS:
        {
          JERRY_DEBUG_MSG ("  END_ARGUMENTS\n");
          source_start_p = source_p;
          break;
        }
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

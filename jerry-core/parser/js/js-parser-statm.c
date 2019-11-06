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
 * \addtogroup jsparser_stmt Statement parser
 * @{
 */

/**
 * @{
 * Strict mode string literal in directive prologues
 */
#define PARSER_USE_STRICT_LITERAL  "use strict"
#define PARSER_USE_STRICT_LENGTH   10
/** @} */

/**
 * Parser statement types.
 *
 * When a new statement is added, the following
 * functions may need to be updated as well:
 *
 *  - parser_statement_length()
 *  - parser_parse_break_statement()
 *  - parser_parse_continue_statement()
 *  - parser_free_jumps()
 *  - 'case LEXER_RIGHT_BRACE:' in parser_parse_statements()
 *  - 'if (context_p->token.type == LEXER_RIGHT_BRACE)' in parser_parse_statements()
 *  - 'switch (context_p->stack_top_uint8)' in parser_parse_statements()
 */
typedef enum
{
  PARSER_STATEMENT_START,
  PARSER_STATEMENT_BLOCK,
#if ENABLED (JERRY_ES2015)
  PARSER_STATEMENT_BLOCK_CONTEXT,
#endif /* ENABLED (JERRY_ES2015) */
  PARSER_STATEMENT_LABEL,
  PARSER_STATEMENT_IF,
  PARSER_STATEMENT_ELSE,
  /* From switch -> for-in : break target statements */
  PARSER_STATEMENT_SWITCH,
  PARSER_STATEMENT_SWITCH_NO_DEFAULT,
#if ENABLED (JERRY_ES2015)
  PARSER_STATEMENT_SWITCH_BLOCK_CONTEXT,
#endif /* ENABLED (JERRY_ES2015) */
  /* From do-while -> for->in : continue target statements */
  PARSER_STATEMENT_DO_WHILE,
  PARSER_STATEMENT_WHILE,
  PARSER_STATEMENT_FOR,
  /* From for->in -> try : instructions with context
   * Break and continue uses another instruction form
   * when crosses their borders. */
  PARSER_STATEMENT_FOR_IN,
#if ENABLED (JERRY_ES2015)
  PARSER_STATEMENT_FOR_OF,
#endif /* ENABLED (JERRY_ES2015) */
  PARSER_STATEMENT_WITH,
  PARSER_STATEMENT_TRY,
} parser_statement_type_t;

#if !ENABLED (JERRY_ES2015)

/**
 * Get the expected depth of a function call.
 */
#define JERRY_GET_EXPECTED_DEPTH(context_p) 0

#else /* ENABLED (JERRY_ES2015) */

/**
 * Get the expected depth of a function call.
 */
#define JERRY_GET_EXPECTED_DEPTH(context_p) \
  (((context_p)->status_flags & PARSER_INSIDE_BLOCK) ? PARSER_BLOCK_CONTEXT_STACK_ALLOCATION : 0)

/**
 * Block statement.
 */
typedef struct
{
  uint16_t scope_stack_top;               /**< preserved top of scope stack */
  uint16_t scope_stack_reg_top;           /**< preserved top register of scope stack */
} parser_block_statement_t;

/**
 * Context of block statement.
 */
typedef struct
{
  parser_branch_t branch;                 /**< branch to the end */
} parser_block_context_t;

#endif /* !ENABLED (JERRY_ES2015) */

/**
 * Loop statement.
 */
typedef struct
{
  parser_branch_node_t *branch_list_p;    /**< list of breaks and continues targeting this statement */
} parser_loop_statement_t;

/**
 * Label statement.
 */
typedef struct
{
  lexer_lit_location_t label_ident;       /**< name of the label */
  parser_branch_node_t *break_list_p;     /**< list of breaks targeting this label */
} parser_label_statement_t;

/**
 * If/else statement.
 */
typedef struct
{
  parser_branch_t branch;                 /**< branch to the end */
} parser_if_else_statement_t;

/**
 * Switch statement.
 */
typedef struct
{
  parser_branch_t default_branch;         /**< branch to the default case */
  parser_branch_node_t *branch_list_p;    /**< branches of case statements */
#if ENABLED (JERRY_ES2015)
  uint16_t scope_stack_top;               /**< preserved top of scope stack */
  uint16_t scope_stack_reg_top;           /**< preserved top register of scope stack */
#endif /* ENABLED (JERRY_ES2015) */
} parser_switch_statement_t;

/**
 * Do-while statement.
 */
typedef struct
{
  uint32_t start_offset;                  /**< start byte code offset */
} parser_do_while_statement_t;

/**
 * While statement.
 */
typedef struct
{
  parser_branch_t branch;                 /**< branch to the end */
  scanner_location_t condition_location;  /**< condition part */
  uint32_t start_offset;                  /**< start byte code offset */
} parser_while_statement_t;

/**
 * For statement.
 */
typedef struct
{
  parser_branch_t branch;                 /**< branch to the end */
  scanner_location_t condition_location;  /**< condition part */
  scanner_location_t expression_location; /**< expression part */
  uint32_t start_offset;                  /**< start byte code offset */
} parser_for_statement_t;

/**
 * For-in statement.
 */
typedef struct
{
  parser_branch_t branch;                 /**< branch to the end */
  uint32_t start_offset;                  /**< start byte code offset */
} parser_for_in_of_statement_t;

/**
 * With statement.
 */
typedef struct
{
  parser_branch_t branch;                 /**< branch to the end */
} parser_with_statement_t;

/**
 * Lexer token types.
 */
typedef enum
{
  parser_try_block,                       /**< try block */
  parser_catch_block,                     /**< catch block */
  parser_finally_block,                   /**< finally block */
} parser_try_block_type_t;

/**
 * Try statement.
 */
typedef struct
{
  parser_try_block_type_t type;           /**< current block type */
  uint16_t scope_stack_top;               /**< current top of scope stack */
  uint16_t scope_stack_reg_top;           /**< current top register of scope stack */
  parser_branch_t branch;                 /**< branch to the end of the current block */
} parser_try_statement_t;

/**
 * Returns the data consumed by a statement. It can be used
 * to skip undesired frames on the stack during frame search.
 *
 * @return size consumed by a statement.
 */
static inline size_t
parser_statement_length (uint8_t type) /**< type of statement */
{
  static const uint8_t statement_lengths[] =
  {
#if ENABLED (JERRY_ES2015)
    /* PARSER_STATEMENT_BLOCK */
    (uint8_t) (sizeof (parser_block_statement_t) + 1),
    /* PARSER_STATEMENT_BLOCK_CONTEXT */
    (uint8_t) (sizeof (parser_block_statement_t) + sizeof (parser_block_context_t) + 1),
#else /* !ENABLED (JERRY_ES2015) */
    /* PARSER_STATEMENT_BLOCK */
    1,
#endif /* ENABLED (JERRY_ES2015) */
    /* PARSER_STATEMENT_LABEL */
    (uint8_t) (sizeof (parser_label_statement_t) + 1),
    /* PARSER_STATEMENT_IF */
    (uint8_t) (sizeof (parser_if_else_statement_t) + 1),
    /* PARSER_STATEMENT_ELSE */
    (uint8_t) (sizeof (parser_if_else_statement_t) + 1),
    /* PARSER_STATEMENT_SWITCH */
    (uint8_t) (sizeof (parser_switch_statement_t) + sizeof (parser_loop_statement_t) + 1),
    /* PARSER_STATEMENT_SWITCH_NO_DEFAULT */
    (uint8_t) (sizeof (parser_switch_statement_t) + sizeof (parser_loop_statement_t) + 1),
#if ENABLED (JERRY_ES2015)
    /* PARSER_STATEMENT_SWITCH_BLOCK_CONTEXT */
    (uint8_t) (sizeof (parser_block_context_t) + 1),
#endif /* ENABLED (JERRY_ES2015) */
    /* PARSER_STATEMENT_DO_WHILE */
    (uint8_t) (sizeof (parser_do_while_statement_t) + sizeof (parser_loop_statement_t) + 1),
    /* PARSER_STATEMENT_WHILE */
    (uint8_t) (sizeof (parser_while_statement_t) + sizeof (parser_loop_statement_t) + 1),
    /* PARSER_STATEMENT_FOR */
    (uint8_t) (sizeof (parser_for_statement_t) + sizeof (parser_loop_statement_t) + 1),
    /* PARSER_STATEMENT_FOR_IN */
    (uint8_t) (sizeof (parser_for_in_of_statement_t) + sizeof (parser_loop_statement_t) + 1),
#if ENABLED (JERRY_ES2015)
    /* PARSER_STATEMENT_FOR_OF */
    (uint8_t) (sizeof (parser_for_in_of_statement_t) + sizeof (parser_loop_statement_t) + 1),
#endif /* ENABLED (JERRY_ES2015) */
    /* PARSER_STATEMENT_WITH */
    (uint8_t) (sizeof (parser_with_statement_t) + 1),
    /* PARSER_STATEMENT_TRY */
    (uint8_t) (sizeof (parser_try_statement_t) + 1),
  };

  JERRY_ASSERT (type >= PARSER_STATEMENT_BLOCK && type <= PARSER_STATEMENT_TRY);

  return statement_lengths[type - PARSER_STATEMENT_BLOCK];
} /* parser_statement_length */

/**
 * Initialize stack iterator.
 */
static inline void
parser_stack_iterator_init (parser_context_t *context_p, /**< context */
                            parser_stack_iterator_t *iterator) /**< iterator */
{
  iterator->current_p = context_p->stack.first_p;
  iterator->current_position = context_p->stack.last_position;
} /* parser_stack_iterator_init */

/**
 * Read the next byte from the stack.
 *
 * @return byte
 */
static inline uint8_t
parser_stack_iterator_read_uint8 (parser_stack_iterator_t *iterator) /**< iterator */
{
  JERRY_ASSERT (iterator->current_position > 0 && iterator->current_position <= PARSER_STACK_PAGE_SIZE);
  return iterator->current_p->bytes[iterator->current_position - 1];
} /* parser_stack_iterator_read_uint8 */

/**
 * Change last byte of the stack.
 */
static inline void
parser_stack_change_last_uint8 (parser_context_t *context_p, /**< context */
                                uint8_t new_value) /**< new value */
{
  parser_mem_page_t *page_p = context_p->stack.first_p;

  JERRY_ASSERT (page_p != NULL
                && context_p->stack_top_uint8 == page_p->bytes[context_p->stack.last_position - 1]);

  page_p->bytes[context_p->stack.last_position - 1] = new_value;
  context_p->stack_top_uint8 = new_value;
} /* parser_stack_change_last_uint8 */

/**
 * Parse expression enclosed in parens.
 */
static inline void
parser_parse_enclosed_expr (parser_context_t *context_p) /**< context */
{
  lexer_next_token (context_p);

  if (context_p->token.type != LEXER_LEFT_PAREN)
  {
    parser_raise_error (context_p, PARSER_ERR_LEFT_PAREN_EXPECTED);
  }

  lexer_next_token (context_p);
  parser_parse_expression (context_p, PARSE_EXPR);

  if (context_p->token.type != LEXER_RIGHT_PAREN)
  {
    parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
  }
  lexer_next_token (context_p);
} /* parser_parse_enclosed_expr */

/**
 * Parse var statement.
 */
static void
parser_parse_var_statement (parser_context_t *context_p) /**< context */
{
  JERRY_ASSERT (context_p->token.type == LEXER_KEYW_VAR
                || context_p->token.type == LEXER_KEYW_LET
                || context_p->token.type == LEXER_KEYW_CONST);

#if ENABLED (JERRY_ES2015)
  uint8_t declaration_type = context_p->token.type;
#endif /* ENABLED (JERRY_ES2015) */

  while (true)
  {
    lexer_expect_identifier (context_p, LEXER_IDENT_LITERAL);
    JERRY_ASSERT (context_p->token.type == LEXER_LITERAL
                  && context_p->token.lit_location.type == LEXER_IDENT_LITERAL);

#if ENABLED (JERRY_DEBUGGER) || ENABLED (JERRY_LINE_INFO)
    parser_line_counter_t ident_line_counter = context_p->token.line;
#endif /* ENABLED (JERRY_DEBUGGER) || ENABLED (JERRY_LINE_INFO) */

#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
    if (context_p->status_flags & PARSER_MODULE_STORE_IDENT)
    {
      context_p->module_identifier_lit_p = context_p->lit_object.literal_p;
      context_p->status_flags &= (uint32_t) ~(PARSER_MODULE_STORE_IDENT);
    }
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */

#if ENABLED (JERRY_ES2015)
    if (context_p->next_scanner_info_p->source_p == context_p->source_p)
    {
      JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_ERR_REDECLARED);
      parser_raise_error (context_p, PARSER_ERR_VARIABLE_REDECLARED);
    }
#endif /* ENABLED (JERRY_ES2015) */

    lexer_next_token (context_p);

    if (context_p->token.type == LEXER_ASSIGN)
    {
#if ENABLED (JERRY_DEBUGGER)
      if ((JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
          && ident_line_counter != context_p->last_breakpoint_line)
      {
        parser_emit_cbc (context_p, CBC_BREAKPOINT_DISABLED);
        parser_flush_cbc (context_p);

        parser_append_breakpoint_info (context_p, JERRY_DEBUGGER_BREAKPOINT_LIST, ident_line_counter);

        context_p->last_breakpoint_line = ident_line_counter;
      }
#endif /* ENABLED (JERRY_DEBUGGER) */

#if ENABLED (JERRY_LINE_INFO)
      if (ident_line_counter != context_p->last_line_info_line)
      {
        parser_emit_line_info (context_p, ident_line_counter, false);
      }
#endif /* ENABLED (JERRY_LINE_INFO) */

#if ENABLED (JERRY_ES2015)
      if (declaration_type != LEXER_KEYW_VAR
          && context_p->lit_object.index < PARSER_REGISTER_START)
      {
        uint16_t index = context_p->lit_object.index;

        lexer_next_token (context_p);
        parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);
        parser_emit_cbc_literal (context_p, CBC_ASSIGN_LET_CONST, index);
      }
      else
      {
#endif /* ENABLED (JERRY_ES2015) */
        parser_emit_cbc_literal_from_token (context_p, CBC_PUSH_LITERAL);
        parser_parse_expression_statement (context_p, PARSE_EXPR_NO_COMMA | PARSE_EXPR_HAS_LITERAL);
#if ENABLED (JERRY_ES2015)
      }
#endif /* ENABLED (JERRY_ES2015) */
    }
#if ENABLED (JERRY_ES2015)
    else if (declaration_type == LEXER_KEYW_LET)
    {
      parser_emit_cbc (context_p, CBC_PUSH_UNDEFINED);

      uint16_t index = context_p->lit_object.index;
      parser_emit_cbc_literal (context_p,
                               index >= PARSER_REGISTER_START ? CBC_MOV_IDENT : CBC_ASSIGN_LET_CONST,
                               index);
    }
    else if (declaration_type == LEXER_KEYW_CONST)
    {
      parser_raise_error (context_p, PARSER_ERR_MISSING_ASSIGN_AFTER_CONST);
    }
#endif /* ENABLED (JERRY_ES2015) */

    if (context_p->token.type != LEXER_COMMA)
    {
      break;
    }
  }
} /* parser_parse_var_statement */

/**
 * Parse function statement.
 */
static void
parser_parse_function_statement (parser_context_t *context_p) /**< context */
{
  uint32_t status_flags;
  lexer_literal_t *literal_p;

  JERRY_ASSERT (context_p->token.type == LEXER_KEYW_FUNCTION);

#if ENABLED (JERRY_DEBUGGER)
  parser_line_counter_t debugger_line = context_p->token.line;
  parser_line_counter_t debugger_column = context_p->token.column;
#endif /* ENABLED (JERRY_DEBUGGER) */

  lexer_expect_identifier (context_p, LEXER_NEW_IDENT_LITERAL);
  JERRY_ASSERT (context_p->token.type == LEXER_LITERAL
                && context_p->token.lit_location.type == LEXER_IDENT_LITERAL);

#if ENABLED (JERRY_ES2015)
  if (context_p->next_scanner_info_p->source_p == context_p->source_p)
  {
    JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_ERR_REDECLARED);
    parser_raise_error (context_p, PARSER_ERR_VARIABLE_REDECLARED);
  }
#endif /* ENABLED (JERRY_ES2015) */

  if (context_p->lit_object.type == LEXER_LITERAL_OBJECT_ARGUMENTS)
  {
    context_p->status_flags |= PARSER_ARGUMENTS_NOT_NEEDED;
  }

#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
  if (context_p->status_flags & PARSER_MODULE_STORE_IDENT)
  {
    context_p->module_identifier_lit_p = context_p->lit_object.literal_p;
    context_p->status_flags &= (uint32_t) ~(PARSER_MODULE_STORE_IDENT);
  }
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */

  status_flags = PARSER_IS_FUNCTION | PARSER_IS_CLOSURE;
  if (context_p->lit_object.type != LEXER_LITERAL_OBJECT_ANY)
  {
    JERRY_ASSERT (context_p->lit_object.type == LEXER_LITERAL_OBJECT_EVAL
                  || context_p->lit_object.type == LEXER_LITERAL_OBJECT_ARGUMENTS);
    status_flags |= PARSER_HAS_NON_STRICT_ARG;
  }

#if ENABLED (JERRY_DEBUGGER)
  if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
  {
    lexer_literal_t *name_p = context_p->lit_object.literal_p;
    jerry_debugger_send_string (JERRY_DEBUGGER_FUNCTION_NAME,
                                JERRY_DEBUGGER_NO_SUBTYPE,
                                name_p->u.char_p,
                                name_p->prop.length);

    /* Reset token position for the function. */
    context_p->token.line = debugger_line;
    context_p->token.column = debugger_column;
  }
#endif /* ENABLED (JERRY_DEBUGGER) */

  JERRY_ASSERT (context_p->scope_stack_top >= 2);
  parser_scope_stack *scope_stack_p = context_p->scope_stack_p + context_p->scope_stack_top - 2;

  uint16_t literal_index = context_p->lit_object.index;

  while (literal_index != scope_stack_p->map_from)
  {
    scope_stack_p--;

    JERRY_ASSERT (scope_stack_p >= context_p->scope_stack_p);
  }

  JERRY_ASSERT (scope_stack_p[1].map_from == PARSER_SCOPE_STACK_FUNC);

  literal_p = PARSER_GET_LITERAL ((size_t) scope_stack_p[1].map_to);

  JERRY_ASSERT ((literal_p->type == LEXER_UNUSED_LITERAL || literal_p->type == LEXER_FUNCTION_LITERAL)
                && literal_p->status_flags == 0);

  ecma_compiled_code_t *compiled_code_p = parser_parse_function (context_p, status_flags);

  if (literal_p->type == LEXER_FUNCTION_LITERAL)
  {
    ecma_bytecode_deref (literal_p->u.bytecode_p);
  }

  literal_p->u.bytecode_p = compiled_code_p;
  literal_p->type = LEXER_FUNCTION_LITERAL;

  lexer_next_token (context_p);
} /* parser_parse_function_statement */

/**
 * Parse if statement (starting part).
 */
static void
parser_parse_if_statement_start (parser_context_t *context_p) /**< context */
{
  parser_if_else_statement_t if_statement;

  parser_parse_enclosed_expr (context_p);

  parser_emit_cbc_forward_branch (context_p,
                                  CBC_BRANCH_IF_FALSE_FORWARD,
                                  &if_statement.branch);

  parser_stack_push (context_p, &if_statement, sizeof (parser_if_else_statement_t));
  parser_stack_push_uint8 (context_p, PARSER_STATEMENT_IF);
  parser_stack_iterator_init (context_p, &context_p->last_statement);
} /* parser_parse_if_statement_start */

/**
 * Parse if statement (ending part).
 *
 * @return true  - if parsing an 'else' statement
 *         false - otherwise
 */
static bool
parser_parse_if_statement_end (parser_context_t *context_p) /**< context */
{
  parser_if_else_statement_t if_statement;
  parser_if_else_statement_t else_statement;
  parser_stack_iterator_t iterator;

  JERRY_ASSERT (context_p->stack_top_uint8 == PARSER_STATEMENT_IF);

  if (context_p->token.type != LEXER_KEYW_ELSE)
  {
    parser_stack_pop_uint8 (context_p);
    parser_stack_pop (context_p, &if_statement, sizeof (parser_if_else_statement_t));
    parser_stack_iterator_init (context_p, &context_p->last_statement);

    parser_set_branch_to_current_position (context_p, &if_statement.branch);

    return false;
  }

  parser_stack_change_last_uint8 (context_p, PARSER_STATEMENT_ELSE);
  parser_stack_iterator_init (context_p, &iterator);
  parser_stack_iterator_skip (&iterator, 1);
  parser_stack_iterator_read (&iterator, &if_statement, sizeof (parser_if_else_statement_t));

  parser_emit_cbc_forward_branch (context_p,
                                  CBC_JUMP_FORWARD,
                                  &else_statement.branch);

  parser_set_branch_to_current_position (context_p, &if_statement.branch);

  parser_stack_iterator_write (&iterator, &else_statement, sizeof (parser_if_else_statement_t));

  lexer_next_token (context_p);
  return true;
} /* parser_parse_if_statement_end */

/**
 * Parse with statement (starting part).
 */
static void
parser_parse_with_statement_start (parser_context_t *context_p) /**< context */
{
  parser_with_statement_t with_statement;

  if (context_p->status_flags & PARSER_IS_STRICT)
  {
    parser_raise_error (context_p, PARSER_ERR_WITH_NOT_ALLOWED);
  }

  parser_parse_enclosed_expr (context_p);

#ifndef JERRY_NDEBUG
  PARSER_PLUS_EQUAL_U16 (context_p->context_stack_depth, PARSER_WITH_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

  context_p->status_flags |= PARSER_INSIDE_WITH | PARSER_LEXICAL_ENV_NEEDED;
  parser_emit_cbc_ext_forward_branch (context_p,
                                      CBC_EXT_WITH_CREATE_CONTEXT,
                                      &with_statement.branch);

  parser_stack_push (context_p, &with_statement, sizeof (parser_with_statement_t));
  parser_stack_push_uint8 (context_p, PARSER_STATEMENT_WITH);
  parser_stack_iterator_init (context_p, &context_p->last_statement);
} /* parser_parse_with_statement_start */

/**
 * Parse with statement (ending part).
 */
static void
parser_parse_with_statement_end (parser_context_t *context_p) /**< context */
{
  parser_with_statement_t with_statement;
  parser_stack_iterator_t iterator;

  JERRY_ASSERT (context_p->status_flags & PARSER_INSIDE_WITH);

  parser_stack_pop_uint8 (context_p);
  parser_stack_pop (context_p, &with_statement, sizeof (parser_with_statement_t));
  parser_stack_iterator_init (context_p, &context_p->last_statement);

  parser_flush_cbc (context_p);
  PARSER_MINUS_EQUAL_U16 (context_p->stack_depth, PARSER_WITH_CONTEXT_STACK_ALLOCATION);
#ifndef JERRY_NDEBUG
  PARSER_MINUS_EQUAL_U16 (context_p->context_stack_depth, PARSER_WITH_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

  parser_emit_cbc (context_p, CBC_CONTEXT_END);
  parser_set_branch_to_current_position (context_p, &with_statement.branch);

  parser_stack_iterator_init (context_p, &iterator);

  while (true)
  {
    uint8_t type = parser_stack_iterator_read_uint8 (&iterator);

    if (type == PARSER_STATEMENT_START)
    {
      context_p->status_flags &= (uint32_t) ~PARSER_INSIDE_WITH;
      return;
    }

    if (type == PARSER_STATEMENT_WITH)
    {
      return;
    }

    parser_stack_iterator_skip (&iterator, parser_statement_length (type));
  }
} /* parser_parse_with_statement_end */

#if ENABLED (JERRY_ES2015)
/**
 * Parse super class context like a with statement (starting part).
 */
void
parser_parse_super_class_context_start (parser_context_t *context_p) /**< context */
{
  JERRY_ASSERT (context_p->token.type == LEXER_KEYW_EXTENDS
                || (context_p->status_flags & PARSER_CLASS_HAS_SUPER));
  parser_with_statement_t with_statement;

  if (context_p->token.type == LEXER_KEYW_EXTENDS)
  {
    lexer_next_token (context_p);

    /* NOTE: Currently there is no proper way to check whether the currently parsed expression
       is a valid lefthand-side expression or not, so we do not throw syntax error and parse
       the class extending value as an expression. */
    parser_parse_expression (context_p, PARSE_EXPR | PARSE_EXPR_NO_COMMA);
  }
  else
  {
    JERRY_ASSERT (context_p->status_flags & PARSER_CLASS_HAS_SUPER);
    parser_emit_cbc (context_p, CBC_PUSH_NULL);
    context_p->status_flags |= PARSER_CLASS_IMPLICIT_SUPER;
  }

#ifndef JERRY_NDEBUG
  PARSER_PLUS_EQUAL_U16 (context_p->context_stack_depth, PARSER_SUPER_CLASS_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

  context_p->status_flags |= PARSER_CLASS_HAS_SUPER;
  parser_emit_cbc_ext_forward_branch (context_p,
                                      CBC_EXT_SUPER_CLASS_CREATE_CONTEXT,
                                      &with_statement.branch);

  parser_stack_push (context_p, &with_statement, sizeof (parser_with_statement_t));
  parser_stack_push_uint8 (context_p, PARSER_STATEMENT_WITH);
} /* parser_parse_super_class_context_start */

/**
 * Parse super class context like a with statement (ending part).
 */
void
parser_parse_super_class_context_end (parser_context_t *context_p) /**< context */
{
  parser_with_statement_t with_statement;
  parser_stack_pop_uint8 (context_p);
  parser_stack_pop (context_p, &with_statement, sizeof (parser_with_statement_t));

  parser_flush_cbc (context_p);
  PARSER_MINUS_EQUAL_U16 (context_p->stack_depth, PARSER_SUPER_CLASS_CONTEXT_STACK_ALLOCATION);
#ifndef JERRY_NDEBUG
  PARSER_MINUS_EQUAL_U16 (context_p->context_stack_depth, PARSER_SUPER_CLASS_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

  parser_emit_cbc_ext (context_p, CBC_EXT_CLASS_EXPR_CONTEXT_END);
  parser_set_branch_to_current_position (context_p, &with_statement.branch);
} /* parser_parse_super_class_context_end */
#endif /* ENABLED (JERRY_ES2015) */

/**
 * Parse do-while statement (ending part).
 */
static void
parser_parse_do_while_statement_end (parser_context_t *context_p) /**< context */
{
  parser_loop_statement_t loop;

  JERRY_ASSERT (context_p->stack_top_uint8 == PARSER_STATEMENT_DO_WHILE);

  if (context_p->token.type != LEXER_KEYW_WHILE)
  {
    parser_raise_error (context_p, PARSER_ERR_WHILE_EXPECTED);
  }

  parser_stack_iterator_t iterator;
  parser_stack_iterator_init (context_p, &iterator);

  parser_stack_iterator_skip (&iterator, 1);
  parser_stack_iterator_read (&iterator, &loop, sizeof (parser_loop_statement_t));

  parser_set_continues_to_current_position (context_p, loop.branch_list_p);

  JERRY_ASSERT (context_p->next_scanner_info_p->source_p != context_p->source_p);

  parser_parse_enclosed_expr (context_p);

  if (context_p->last_cbc_opcode != CBC_PUSH_FALSE)
  {
    cbc_opcode_t opcode = CBC_BRANCH_IF_TRUE_BACKWARD;
    if (context_p->last_cbc_opcode == CBC_LOGICAL_NOT)
    {
      context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
      opcode = CBC_BRANCH_IF_FALSE_BACKWARD;
    }
    else if (context_p->last_cbc_opcode == CBC_PUSH_TRUE)
    {
      context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
      opcode = CBC_JUMP_BACKWARD;
    }

    parser_do_while_statement_t do_while_statement;
    parser_stack_iterator_skip (&iterator, sizeof (parser_loop_statement_t));
    parser_stack_iterator_read (&iterator, &do_while_statement, sizeof (parser_do_while_statement_t));

    parser_emit_cbc_backward_branch (context_p, (uint16_t) opcode, do_while_statement.start_offset);
  }
  else
  {
    context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
  }

  parser_stack_pop (context_p, NULL, 1 + sizeof (parser_loop_statement_t) + sizeof (parser_do_while_statement_t));
  parser_stack_iterator_init (context_p, &context_p->last_statement);

  parser_set_breaks_to_current_position (context_p, loop.branch_list_p);
} /* parser_parse_do_while_statement_end */

/**
 * Parse while statement (starting part).
 */
static void
parser_parse_while_statement_start (parser_context_t *context_p) /**< context */
{
  parser_while_statement_t while_statement;
  parser_loop_statement_t loop;

  JERRY_ASSERT (context_p->token.type == LEXER_KEYW_WHILE);
  lexer_next_token (context_p);

  if (context_p->token.type != LEXER_LEFT_PAREN)
  {
    parser_raise_error (context_p, PARSER_ERR_LEFT_PAREN_EXPECTED);
  }

  JERRY_ASSERT (context_p->next_scanner_info_p->source_p != context_p->source_p
                || context_p->next_scanner_info_p->type == SCANNER_TYPE_WHILE);

  if (context_p->next_scanner_info_p->source_p != context_p->source_p)
  {
    /* The prescanner couldn't find the end of the while condition. */
    lexer_next_token (context_p);
    parser_parse_expression (context_p, PARSE_EXPR);

    JERRY_ASSERT (context_p->token.type != LEXER_RIGHT_PAREN);
    parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
  }

  parser_emit_cbc_forward_branch (context_p, CBC_JUMP_FORWARD, &while_statement.branch);

  JERRY_ASSERT (context_p->last_cbc_opcode == PARSER_CBC_UNAVAILABLE);

  while_statement.start_offset = context_p->byte_code_size;
  scanner_get_location (&while_statement.condition_location, context_p);

  scanner_set_location (context_p, &((scanner_location_info_t *) context_p->next_scanner_info_p)->location);
  scanner_release_next (context_p, sizeof (scanner_location_info_t));
  scanner_seek (context_p);
  lexer_next_token (context_p);

  loop.branch_list_p = NULL;

  parser_stack_push (context_p, &while_statement, sizeof (parser_while_statement_t));
  parser_stack_push (context_p, &loop, sizeof (parser_loop_statement_t));
  parser_stack_push_uint8 (context_p, PARSER_STATEMENT_WHILE);
  parser_stack_iterator_init (context_p, &context_p->last_statement);
} /* parser_parse_while_statement_start */

/**
 * Parse while statement (ending part).
 */
static void JERRY_ATTR_NOINLINE
parser_parse_while_statement_end (parser_context_t *context_p) /**< context */
{
  parser_while_statement_t while_statement;
  parser_loop_statement_t loop;
  lexer_token_t current_token;
  scanner_location_t location;
  cbc_opcode_t opcode;

  JERRY_ASSERT (context_p->stack_top_uint8 == PARSER_STATEMENT_WHILE);

  parser_stack_iterator_t iterator;
  parser_stack_iterator_init (context_p, &iterator);

  parser_stack_iterator_skip (&iterator, 1);
  parser_stack_iterator_read (&iterator, &loop, sizeof (parser_loop_statement_t));
  parser_stack_iterator_skip (&iterator, sizeof (parser_loop_statement_t));
  parser_stack_iterator_read (&iterator, &while_statement, sizeof (parser_while_statement_t));

  scanner_get_location (&location, context_p);
  current_token = context_p->token;

  parser_set_branch_to_current_position (context_p, &while_statement.branch);
  parser_set_continues_to_current_position (context_p, loop.branch_list_p);

  scanner_set_location (context_p, &while_statement.condition_location);
  scanner_seek (context_p);
  lexer_next_token (context_p);

  parser_parse_expression (context_p, PARSE_EXPR);
  if (context_p->token.type != LEXER_RIGHT_PAREN)
  {
    parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
  }

  opcode = CBC_BRANCH_IF_TRUE_BACKWARD;
  if (context_p->last_cbc_opcode == CBC_LOGICAL_NOT)
  {
    context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
    opcode = CBC_BRANCH_IF_FALSE_BACKWARD;
  }
  else if (context_p->last_cbc_opcode == CBC_PUSH_TRUE)
  {
    context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
    opcode = CBC_JUMP_BACKWARD;
  }

  parser_stack_pop (context_p, NULL, 1 + sizeof (parser_loop_statement_t) + sizeof (parser_while_statement_t));
  parser_stack_iterator_init (context_p, &context_p->last_statement);

  parser_emit_cbc_backward_branch (context_p, (uint16_t) opcode, while_statement.start_offset);
  parser_set_breaks_to_current_position (context_p, loop.branch_list_p);

  /* Calling scanner_seek is unnecessary because all
   * info blocks inside the while statement should be processed. */
  scanner_set_location (context_p, &location);
  context_p->token = current_token;
} /* parser_parse_while_statement_end */

/**
 * Check whether the opcode is a valid LeftHandSide expression
 * and convert it back to an assignment.
 *
 * @return the compatible assignment opcode
 */
static uint16_t
parser_check_left_hand_side_expression (parser_context_t *context_p, /**< context */
                                        uint16_t opcode) /**< opcode to check */
{
  if (opcode == CBC_PUSH_LITERAL
      && context_p->last_cbc.literal_type == LEXER_IDENT_LITERAL)
  {
    context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
    return CBC_ASSIGN_SET_IDENT;
  }
  else if (opcode == CBC_PUSH_PROP)
  {
    context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
    return CBC_ASSIGN;
  }
  else if (opcode == CBC_PUSH_PROP_LITERAL)
  {
    context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
    return CBC_ASSIGN_PROP_LITERAL;
  }
  else if (opcode == CBC_PUSH_PROP_LITERAL_LITERAL)
  {
    context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
    return CBC_ASSIGN;
  }
  else if (opcode == CBC_PUSH_PROP_THIS_LITERAL)
  {
    context_p->last_cbc_opcode = CBC_PUSH_THIS_LITERAL;
    return CBC_ASSIGN;
  }
  else
  {
    /* Invalid LeftHandSide expression. */
    parser_emit_cbc_ext (context_p, CBC_EXT_THROW_REFERENCE_ERROR);
    return CBC_ASSIGN;
  }

  return opcode;
} /* parser_check_left_hand_side_expression */

/**
 * Parse for statement (starting part).
 */
static void
parser_parse_for_statement_start (parser_context_t *context_p) /**< context */
{
  parser_loop_statement_t loop;

  JERRY_ASSERT (context_p->token.type == LEXER_KEYW_FOR);
  lexer_next_token (context_p);

  if (context_p->token.type != LEXER_LEFT_PAREN)
  {
    parser_raise_error (context_p, PARSER_ERR_LEFT_PAREN_EXPECTED);
  }

  if (context_p->next_scanner_info_p->source_p == context_p->source_p)
  {
    parser_for_in_of_statement_t for_in_of_statement;
    scanner_location_t start_location, end_location;

#if ENABLED (JERRY_ES2015)
    JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_FOR_IN
                  || context_p->next_scanner_info_p->type == SCANNER_TYPE_FOR_OF);

    bool is_for_in = (context_p->next_scanner_info_p->type == SCANNER_TYPE_FOR_IN);
#else /* !ENABLED (JERRY_ES2015) */
    JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_FOR_IN);

    bool is_for_in = true;
#endif /* ENABLED (JERRY_ES2015) */

    scanner_get_location (&start_location, context_p);
    scanner_set_location (context_p, &((scanner_location_info_t *) context_p->next_scanner_info_p)->location);
    /* The length of both 'in' and 'of' is two. */
    const uint8_t *source_end_p = context_p->source_p - 2;

    scanner_release_next (context_p, sizeof (scanner_location_info_t));
    scanner_seek (context_p);
    lexer_next_token (context_p);
    parser_parse_expression (context_p, PARSE_EXPR);

    if (context_p->token.type != LEXER_RIGHT_PAREN)
    {
      parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
    }

#ifndef JERRY_NDEBUG
    PARSER_PLUS_EQUAL_U16 (context_p->context_stack_depth,
                           is_for_in ? PARSER_FOR_IN_CONTEXT_STACK_ALLOCATION
                                     : PARSER_FOR_OF_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

    parser_emit_cbc_ext_forward_branch (context_p,
                                        is_for_in ? CBC_EXT_FOR_IN_CREATE_CONTEXT
                                                  : CBC_EXT_FOR_OF_CREATE_CONTEXT,
                                        &for_in_of_statement.branch);

    JERRY_ASSERT (context_p->last_cbc_opcode == PARSER_CBC_UNAVAILABLE);
    for_in_of_statement.start_offset = context_p->byte_code_size;

    /* The expression parser must not read the 'in' or 'of' tokens. */
    scanner_get_location (&end_location, context_p);
    scanner_set_location (context_p, &start_location);

    const uint8_t *original_source_end_p = context_p->source_end_p;
    context_p->source_end_p = source_end_p;
    scanner_seek (context_p);
    lexer_next_token (context_p);

    if (context_p->token.type == LEXER_KEYW_VAR)
    {
      uint16_t literal_index;

      lexer_expect_identifier (context_p, LEXER_IDENT_LITERAL);
      JERRY_ASSERT (context_p->token.type == LEXER_LITERAL
                    && context_p->token.lit_location.type == LEXER_IDENT_LITERAL);

      literal_index = context_p->lit_object.index;

      lexer_next_token (context_p);

      if (context_p->token.type == LEXER_ASSIGN)
      {
        parser_branch_t branch;

        /* Initialiser is never executed. */
        parser_emit_cbc_forward_branch (context_p, CBC_JUMP_FORWARD, &branch);
        lexer_next_token (context_p);
        parser_parse_expression_statement (context_p, PARSE_EXPR_NO_COMMA);
        parser_set_branch_to_current_position (context_p, &branch);
      }

      parser_emit_cbc_ext (context_p, is_for_in ? CBC_EXT_FOR_IN_GET_NEXT
                                                : CBC_EXT_FOR_OF_GET_NEXT);
      parser_emit_cbc_literal (context_p, CBC_ASSIGN_SET_IDENT, literal_index);
    }
    else
    {
      uint16_t opcode;

      parser_parse_expression (context_p, PARSE_EXPR);

      opcode = context_p->last_cbc_opcode;

      /* The CBC_EXT_FOR_IN_CREATE_CONTEXT flushed the opcode combiner. */
      JERRY_ASSERT (opcode != CBC_PUSH_TWO_LITERALS
                    && opcode != CBC_PUSH_THREE_LITERALS);

      opcode = parser_check_left_hand_side_expression (context_p, opcode);

      parser_emit_cbc_ext (context_p, is_for_in ? CBC_EXT_FOR_IN_GET_NEXT
                                                : CBC_EXT_FOR_OF_GET_NEXT);
      parser_flush_cbc (context_p);

      context_p->last_cbc_opcode = opcode;
    }

    if (context_p->token.type != LEXER_EOS)
    {
#if ENABLED (JERRY_ES2015)
      parser_raise_error (context_p, is_for_in ? PARSER_ERR_IN_EXPECTED : PARSER_ERR_OF_EXPECTED);
#else /* !ENABLED (JERRY_ES2015) */
      parser_raise_error (context_p, PARSER_ERR_IN_EXPECTED);
#endif /* ENABLED (JERRY_ES2015) */
    }

    parser_flush_cbc (context_p);
    scanner_set_location (context_p, &end_location);
    context_p->source_end_p = original_source_end_p;
    lexer_next_token (context_p);

    loop.branch_list_p = NULL;

    parser_stack_push (context_p, &for_in_of_statement, sizeof (parser_for_in_of_statement_t));
    parser_stack_push (context_p, &loop, sizeof (parser_loop_statement_t));
#if ENABLED (JERRY_ES2015)
    parser_stack_push_uint8 (context_p, is_for_in ? PARSER_STATEMENT_FOR_IN
                                                  : PARSER_STATEMENT_FOR_OF);
#else /* !ENABLED (JERRY_ES2015) */
    parser_stack_push_uint8 (context_p, PARSER_STATEMENT_FOR_IN);
#endif /* ENABLED (JERRY_ES2015) */
    parser_stack_iterator_init (context_p, &context_p->last_statement);
    return;
  }

  lexer_next_token (context_p);

  if (context_p->token.type != LEXER_SEMICOLON)
  {
    if (context_p->token.type == LEXER_KEYW_VAR)
    {
      parser_parse_var_statement (context_p);
    }
    else
    {
      parser_parse_expression_statement (context_p, PARSE_EXPR);
    }

    if (context_p->token.type != LEXER_SEMICOLON)
    {
      parser_raise_error (context_p, PARSER_ERR_SEMICOLON_EXPECTED);
    }
  }

  JERRY_ASSERT (context_p->next_scanner_info_p->source_p != context_p->source_p
                || context_p->next_scanner_info_p->type == SCANNER_TYPE_FOR);

  if (context_p->next_scanner_info_p->source_p != context_p->source_p
      || ((scanner_for_info_t *) context_p->next_scanner_info_p)->end_location.source_p == NULL)
  {
    if (context_p->next_scanner_info_p->source_p == context_p->source_p)
    {
      /* Even though the scanning is failed, there might be valid statements
       * inside the for statement which depend on scanner info blocks. */
      scanner_release_next (context_p, sizeof (scanner_for_info_t));
    }

    /* The prescanner couldn't find the second semicolon or the closing paranthesis. */
    lexer_next_token (context_p);
    parser_parse_expression (context_p, PARSE_EXPR);

    if (context_p->token.type != LEXER_SEMICOLON)
    {
      parser_raise_error (context_p, PARSER_ERR_SEMICOLON_EXPECTED);
    }

    lexer_next_token (context_p);
    parser_parse_expression_statement (context_p, PARSE_EXPR);

    JERRY_ASSERT (context_p->token.type != LEXER_RIGHT_PAREN);
    parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
  }

  parser_for_statement_t for_statement;
  scanner_for_info_t *for_info_p = (scanner_for_info_t *) context_p->next_scanner_info_p;

  parser_emit_cbc_forward_branch (context_p, CBC_JUMP_FORWARD, &for_statement.branch);

  JERRY_ASSERT (context_p->last_cbc_opcode == PARSER_CBC_UNAVAILABLE);

  for_statement.start_offset = context_p->byte_code_size;
  scanner_get_location (&for_statement.condition_location, context_p);
  for_statement.expression_location = for_info_p->expression_location;

  scanner_set_location (context_p, &for_info_p->end_location);
  scanner_release_next (context_p, sizeof (scanner_for_info_t));
  scanner_seek (context_p);
  lexer_next_token (context_p);

  loop.branch_list_p = NULL;

  parser_stack_push (context_p, &for_statement, sizeof (parser_for_statement_t));
  parser_stack_push (context_p, &loop, sizeof (parser_loop_statement_t));
  parser_stack_push_uint8 (context_p, PARSER_STATEMENT_FOR);
  parser_stack_iterator_init (context_p, &context_p->last_statement);
} /* parser_parse_for_statement_start */

/**
 * Parse for statement (ending part).
 */
static void JERRY_ATTR_NOINLINE
parser_parse_for_statement_end (parser_context_t *context_p) /**< context */
{
  parser_for_statement_t for_statement;
  parser_loop_statement_t loop;
  lexer_token_t current_token;
  scanner_location_t location;
  cbc_opcode_t opcode;

  JERRY_ASSERT (context_p->stack_top_uint8 == PARSER_STATEMENT_FOR);

  parser_stack_iterator_t iterator;
  parser_stack_iterator_init (context_p, &iterator);

  parser_stack_iterator_skip (&iterator, 1);
  parser_stack_iterator_read (&iterator, &loop, sizeof (parser_loop_statement_t));
  parser_stack_iterator_skip (&iterator, sizeof (parser_loop_statement_t));
  parser_stack_iterator_read (&iterator, &for_statement, sizeof (parser_for_statement_t));

  scanner_get_location (&location, context_p);
  current_token = context_p->token;

  scanner_set_location (context_p, &for_statement.expression_location);
  scanner_seek (context_p);
  lexer_next_token (context_p);

  parser_set_continues_to_current_position (context_p, loop.branch_list_p);

  if (context_p->token.type != LEXER_RIGHT_PAREN)
  {
    parser_parse_expression_statement (context_p, PARSE_EXPR);

    if (context_p->token.type != LEXER_RIGHT_PAREN)
    {
      parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
    }
  }

  parser_set_branch_to_current_position (context_p, &for_statement.branch);

  scanner_set_location (context_p, &for_statement.condition_location);
  scanner_seek (context_p);
  lexer_next_token (context_p);

  if (context_p->token.type != LEXER_SEMICOLON)
  {
    parser_parse_expression (context_p, PARSE_EXPR);

    if (context_p->token.type != LEXER_SEMICOLON)
    {
      parser_raise_error (context_p, PARSER_ERR_SEMICOLON_EXPECTED);
    }

    opcode = CBC_BRANCH_IF_TRUE_BACKWARD;
    if (context_p->last_cbc_opcode == CBC_LOGICAL_NOT)
    {
      context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
      opcode = CBC_BRANCH_IF_FALSE_BACKWARD;
    }
    else if (context_p->last_cbc_opcode == CBC_PUSH_TRUE)
    {
      context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
      opcode = CBC_JUMP_BACKWARD;
    }
  }
  else
  {
    opcode = CBC_JUMP_BACKWARD;
  }

  parser_stack_pop (context_p, NULL, 1 + sizeof (parser_loop_statement_t) + sizeof (parser_for_statement_t));
  parser_stack_iterator_init (context_p, &context_p->last_statement);

  parser_emit_cbc_backward_branch (context_p, (uint16_t) opcode, for_statement.start_offset);
  parser_set_breaks_to_current_position (context_p, loop.branch_list_p);

  /* Calling scanner_seek is unnecessary because all
   * info blocks inside the for statement should be processed. */
  scanner_set_location (context_p, &location);
  context_p->token = current_token;
} /* parser_parse_for_statement_end */

/**
 * Parse switch statement (starting part).
 */
static void JERRY_ATTR_NOINLINE
parser_parse_switch_statement_start (parser_context_t *context_p) /**< context */
{
  parser_switch_statement_t switch_statement;
  parser_loop_statement_t loop;
  parser_stack_iterator_t iterator;
  scanner_location_t start_location;
  bool switch_case_was_found;
  bool default_case_was_found;
  parser_branch_node_t *case_branches_p = NULL;

  JERRY_ASSERT (context_p->token.type == LEXER_KEYW_SWITCH);

  parser_parse_enclosed_expr (context_p);

  if (context_p->token.type != LEXER_LEFT_BRACE)
  {
    parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_EXPECTED);
  }

#if ENABLED (JERRY_ES2015)
  switch_statement.scope_stack_top = context_p->scope_stack_top;
  switch_statement.scope_stack_reg_top = context_p->scope_stack_reg_top;

  if (context_p->next_scanner_info_p->source_p == context_p->source_p - 1)
  {
    JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_BLOCK);

    if (scanner_is_context_needed (context_p))
    {
      parser_block_context_t block_context;

#ifndef JERRY_NDEBUG
      PARSER_PLUS_EQUAL_U16 (context_p->context_stack_depth, PARSER_BLOCK_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

      parser_emit_cbc_forward_branch (context_p,
                                      CBC_BLOCK_CREATE_CONTEXT,
                                      &block_context.branch);
      parser_stack_push (context_p, &block_context, sizeof (parser_block_context_t));
      parser_stack_push_uint8 (context_p, PARSER_STATEMENT_SWITCH_BLOCK_CONTEXT);
    }

    scanner_create_variables (context_p, SCANNER_CREATE_VARS_NO_OPTS);
  }
#endif /* ENABLED (JERRY_ES2015) */

  JERRY_ASSERT (context_p->next_scanner_info_p->source_p == context_p->source_p
                && context_p->next_scanner_info_p->type == SCANNER_TYPE_SWITCH);

  scanner_case_info_t *case_info_p = ((scanner_switch_info_t *) context_p->next_scanner_info_p)->case_p;
  scanner_set_active (context_p);

  if (case_info_p == NULL)
  {
    lexer_next_token (context_p);

    if (context_p->token.type == LEXER_RIGHT_BRACE)
    {
      scanner_release_active (context_p, sizeof (scanner_switch_info_t));

      parser_emit_cbc (context_p, CBC_POP);
      parser_flush_cbc (context_p);

#if ENABLED (JERRY_ES2015)
      parser_block_statement_t block_statement;
      block_statement.scope_stack_top = context_p->scope_stack_top;
      block_statement.scope_stack_reg_top = context_p->scope_stack_reg_top;
      parser_stack_push (context_p, &block_statement, sizeof (parser_block_statement_t));
#endif /* ENABLED (JERRY_ES2015) */

      parser_stack_push_uint8 (context_p, PARSER_STATEMENT_BLOCK);
      parser_stack_iterator_init (context_p, &context_p->last_statement);
      return;
    }

    parser_raise_error (context_p, PARSER_ERR_INVALID_SWITCH);
  }

  scanner_get_location (&start_location, context_p);

  /* The reason of using an iterator is error management. If an error
   * occures, parser_free_jumps() free all data. However, the branches
   * created by parser_emit_cbc_forward_branch_item() would not be freed.
   * To free these branches, the current switch data is always stored
   * on the stack. If any change happens, this data is updated. Updates
   * are done using the iterator. */

  switch_statement.branch_list_p = NULL;
  loop.branch_list_p = NULL;

  parser_stack_push (context_p, &switch_statement, sizeof (parser_switch_statement_t));
  parser_stack_iterator_init (context_p, &iterator);
  parser_stack_push (context_p, &loop, sizeof (parser_loop_statement_t));
  parser_stack_push_uint8 (context_p, PARSER_STATEMENT_SWITCH);
  parser_stack_iterator_init (context_p, &context_p->last_statement);

  switch_case_was_found = false;
  default_case_was_found = false;

#if ENABLED (JERRY_LINE_INFO)
  uint32_t last_line_info_line = context_p->last_line_info_line;
#endif /* ENABLED (JERRY_LINE_INFO) */

  do
  {
    scanner_set_location (context_p, &case_info_p->location);
    scanner_seek (context_p);
    case_info_p = case_info_p->next_p;

    /* The last letter of case and default is 'e' and 't' respectively.  */
    JERRY_ASSERT (context_p->source_p[-1] == LIT_CHAR_LOWERCASE_E
                  || context_p->source_p[-1] == LIT_CHAR_LOWERCASE_T);

    bool is_default = context_p->source_p[-1] == LIT_CHAR_LOWERCASE_T;
    lexer_next_token (context_p);

    if (is_default)
    {
      if (default_case_was_found)
      {
        parser_raise_error (context_p, PARSER_ERR_MULTIPLE_DEFAULTS_NOT_ALLOWED);
      }

      if (context_p->token.type != LEXER_COLON)
      {
        parser_raise_error (context_p, PARSER_ERR_COLON_EXPECTED);
      }

      default_case_was_found = true;
      continue;
    }

    switch_case_was_found = true;

#if ENABLED (JERRY_LINE_INFO)
    if (context_p->token.line != context_p->last_line_info_line)
    {
      parser_emit_line_info (context_p, context_p->token.line, true);
    }
#endif /* ENABLED (JERRY_LINE_INFO) */

    parser_parse_expression (context_p, PARSE_EXPR);

    if (context_p->token.type != LEXER_COLON)
    {
      parser_raise_error (context_p, PARSER_ERR_COLON_EXPECTED);
    }

    uint16_t opcode = CBC_BRANCH_IF_STRICT_EQUAL;

    if (case_info_p == NULL
        || (case_info_p->next_p == NULL && case_info_p->location.source_p[-1] == LIT_CHAR_LOWERCASE_T))
    {
      /* There are no more 'case' statements in the switch. */
      parser_emit_cbc (context_p, CBC_STRICT_EQUAL);
      opcode = CBC_BRANCH_IF_TRUE_FORWARD;
    }

    parser_branch_node_t *new_case_p = parser_emit_cbc_forward_branch_item (context_p, opcode, NULL);

    if (case_branches_p == NULL)
    {
      switch_statement.branch_list_p = new_case_p;
      parser_stack_iterator_write (&iterator, &switch_statement, sizeof (parser_switch_statement_t));
    }
    else
    {
      case_branches_p->next_p = new_case_p;
    }

    case_branches_p = new_case_p;
  }
  while (case_info_p != NULL);

  JERRY_ASSERT (switch_case_was_found || default_case_was_found);

#if ENABLED (JERRY_LINE_INFO)
  context_p->last_line_info_line = last_line_info_line;
#endif /* ENABLED (JERRY_LINE_INFO) */

  if (!switch_case_was_found)
  {
    /* There was no case statement, so the expression result
     * of the switch must be popped from the stack */
    parser_emit_cbc (context_p, CBC_POP);
  }

  parser_emit_cbc_forward_branch (context_p, CBC_JUMP_FORWARD, &switch_statement.default_branch);
  parser_stack_iterator_write (&iterator, &switch_statement, sizeof (parser_switch_statement_t));

  if (!default_case_was_found)
  {
    parser_stack_change_last_uint8 (context_p, PARSER_STATEMENT_SWITCH_NO_DEFAULT);
  }

  scanner_release_switch_cases (((scanner_switch_info_t *) context_p->active_scanner_info_p)->case_p);
  scanner_release_active (context_p, sizeof (scanner_switch_info_t));

  scanner_set_location (context_p, &start_location);
  scanner_seek (context_p);
  lexer_next_token (context_p);
} /* parser_parse_switch_statement_start */

/**
 * Parse try statement (ending part).
 */
static void
parser_parse_try_statement_end (parser_context_t *context_p) /**< context */
{
  parser_try_statement_t try_statement;
  parser_stack_iterator_t iterator;

  JERRY_ASSERT (context_p->stack_top_uint8 == PARSER_STATEMENT_TRY);

  parser_stack_iterator_init (context_p, &iterator);
  parser_stack_iterator_skip (&iterator, 1);
  parser_stack_iterator_read (&iterator, &try_statement, sizeof (parser_try_statement_t));

#if ENABLED (JERRY_ES2015)
  context_p->scope_stack_top = try_statement.scope_stack_top;
  context_p->scope_stack_reg_top = try_statement.scope_stack_reg_top;
#endif /* ENABLED (JERRY_ES2015) */

  lexer_next_token (context_p);

  if (try_statement.type == parser_finally_block)
  {
    parser_flush_cbc (context_p);
    PARSER_MINUS_EQUAL_U16 (context_p->stack_depth, PARSER_TRY_CONTEXT_STACK_ALLOCATION);
#ifndef JERRY_NDEBUG
    PARSER_MINUS_EQUAL_U16 (context_p->context_stack_depth, PARSER_TRY_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

    parser_emit_cbc (context_p, CBC_CONTEXT_END);
    parser_set_branch_to_current_position (context_p, &try_statement.branch);
  }
  else
  {
    parser_set_branch_to_current_position (context_p, &try_statement.branch);

    if (try_statement.type == parser_catch_block)
    {
#if !ENABLED (JERRY_ES2015)
      context_p->scope_stack_top = try_statement.scope_stack_top;
      context_p->scope_stack_reg_top = try_statement.scope_stack_reg_top;
#endif /* !ENABLED (JERRY_ES2015) */

      if (context_p->token.type != LEXER_KEYW_FINALLY)
      {
        parser_flush_cbc (context_p);
        PARSER_MINUS_EQUAL_U16 (context_p->stack_depth, PARSER_TRY_CONTEXT_STACK_ALLOCATION);
#ifndef JERRY_NDEBUG
        PARSER_MINUS_EQUAL_U16 (context_p->context_stack_depth, PARSER_TRY_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

        parser_emit_cbc (context_p, CBC_CONTEXT_END);
        parser_flush_cbc (context_p);

        try_statement.type = parser_finally_block;
      }
    }
    else if (try_statement.type == parser_try_block
             && context_p->token.type != LEXER_KEYW_CATCH
             && context_p->token.type != LEXER_KEYW_FINALLY)
    {
      parser_raise_error (context_p, PARSER_ERR_CATCH_FINALLY_EXPECTED);
    }
  }

  if (try_statement.type == parser_finally_block)
  {
    parser_stack_pop (context_p, NULL, (uint32_t) (sizeof (parser_try_statement_t) + 1));
    parser_stack_iterator_init (context_p, &context_p->last_statement);
    return;
  }

  if (context_p->token.type == LEXER_KEYW_CATCH)
  {
    uint16_t literal_index;

    lexer_next_token (context_p);

    if (context_p->token.type != LEXER_LEFT_PAREN)
    {
      parser_raise_error (context_p, PARSER_ERR_LEFT_PAREN_EXPECTED);
    }

    try_statement.type = parser_catch_block;
    parser_emit_cbc_ext_forward_branch (context_p,
                                        CBC_EXT_CATCH,
                                        &try_statement.branch);

    try_statement.scope_stack_top = context_p->scope_stack_top;
    try_statement.scope_stack_reg_top = context_p->scope_stack_reg_top;

#ifndef JERRY_NDEBUG
    bool block_found = false;
#endif /* !JERRY_NDEBUG */

    if (context_p->next_scanner_info_p->source_p == context_p->source_p)
    {
      JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_BLOCK);
#ifndef JERRY_NDEBUG
      block_found = true;
#endif /* !JERRY_NDEBUG */

      if (scanner_is_context_needed (context_p))
      {
        parser_emit_cbc_ext (context_p, CBC_EXT_TRY_CREATE_ENV);
      }

      scanner_create_variables (context_p, SCANNER_CREATE_VARS_NO_OPTS);
    }

    lexer_expect_identifier (context_p, LEXER_IDENT_LITERAL);
    JERRY_ASSERT (context_p->token.type == LEXER_LITERAL
                  && context_p->token.lit_location.type == LEXER_IDENT_LITERAL);

#ifndef JERRY_NDEBUG
    JERRY_ASSERT (block_found);
#endif /* !JERRY_NDEBUG */

    literal_index = context_p->lit_object.index;

    lexer_next_token (context_p);

    if (context_p->token.type != LEXER_RIGHT_PAREN)
    {
      parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
    }

    lexer_next_token (context_p);

    if (context_p->token.type != LEXER_LEFT_BRACE)
    {
      parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_EXPECTED);
    }

    parser_emit_cbc_literal (context_p, CBC_ASSIGN_SET_IDENT, literal_index);
    parser_flush_cbc (context_p);
  }
  else
  {
    JERRY_ASSERT (context_p->token.type == LEXER_KEYW_FINALLY);

    lexer_next_token (context_p);

    if (context_p->token.type != LEXER_LEFT_BRACE)
    {
      parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_EXPECTED);
    }

    try_statement.type = parser_finally_block;
    parser_emit_cbc_ext_forward_branch (context_p,
                                        CBC_EXT_FINALLY,
                                        &try_statement.branch);

#if ENABLED (JERRY_ES2015)
    if (context_p->next_scanner_info_p->source_p == context_p->source_p)
    {
      JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_BLOCK);

      if (scanner_is_context_needed (context_p))
      {
        parser_emit_cbc_ext (context_p, CBC_EXT_TRY_CREATE_ENV);
      }

      scanner_create_variables (context_p, SCANNER_CREATE_VARS_NO_OPTS);
    }
#endif /* ENABLED (JERRY_ES2015) */
  }

  lexer_next_token (context_p);
  parser_stack_iterator_write (&iterator, &try_statement, sizeof (parser_try_statement_t));
} /* parser_parse_try_statement_end */

/**
 * Parse default statement.
 */
static void
parser_parse_default_statement (parser_context_t *context_p) /**< context */
{
  parser_stack_iterator_t iterator;
  parser_switch_statement_t switch_statement;

  if (context_p->stack_top_uint8 != PARSER_STATEMENT_SWITCH
      && context_p->stack_top_uint8 != PARSER_STATEMENT_SWITCH_NO_DEFAULT)
  {
    parser_raise_error (context_p, PARSER_ERR_DEFAULT_NOT_IN_SWITCH);
  }

  lexer_next_token (context_p);
  /* Already checked in parser_parse_switch_statement_start. */
  JERRY_ASSERT (context_p->token.type == LEXER_COLON);
  lexer_next_token (context_p);

  parser_stack_iterator_init (context_p, &iterator);
  parser_stack_iterator_skip (&iterator, 1 + sizeof (parser_loop_statement_t));
  parser_stack_iterator_read (&iterator, &switch_statement, sizeof (parser_switch_statement_t));

  parser_set_branch_to_current_position (context_p, &switch_statement.default_branch);
} /* parser_parse_default_statement */

/**
 * Parse case statement.
 */
static void
parser_parse_case_statement (parser_context_t *context_p) /**< context */
{
  parser_stack_iterator_t iterator;
  parser_switch_statement_t switch_statement;
  parser_branch_node_t *branch_p;

  if (context_p->stack_top_uint8 != PARSER_STATEMENT_SWITCH
      && context_p->stack_top_uint8 != PARSER_STATEMENT_SWITCH_NO_DEFAULT)
  {
    parser_raise_error (context_p, PARSER_ERR_CASE_NOT_IN_SWITCH);
  }

  if (context_p->next_scanner_info_p->source_p != context_p->source_p)
  {
    lexer_next_token (context_p);

    parser_parse_expression (context_p, PARSE_EXPR);

    JERRY_ASSERT (context_p->token.type != LEXER_COLON);
    parser_raise_error (context_p, PARSER_ERR_COLON_EXPECTED);
  }

  JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_CASE);

  scanner_set_location (context_p, &((scanner_location_info_t *) context_p->next_scanner_info_p)->location);
  scanner_release_next (context_p, sizeof (scanner_location_info_t));
  scanner_seek (context_p);
  lexer_next_token (context_p);

  parser_stack_iterator_init (context_p, &iterator);
  parser_stack_iterator_skip (&iterator, 1 + sizeof (parser_loop_statement_t));
  parser_stack_iterator_read (&iterator, &switch_statement, sizeof (parser_switch_statement_t));

  /* Free memory after the case statement is found. */

  branch_p = switch_statement.branch_list_p;
  JERRY_ASSERT (branch_p != NULL);
  switch_statement.branch_list_p = branch_p->next_p;
  parser_stack_iterator_write (&iterator, &switch_statement, sizeof (parser_switch_statement_t));

  parser_set_branch_to_current_position (context_p, &branch_p->branch);
  parser_free (branch_p, sizeof (parser_branch_node_t));
} /* parser_parse_case_statement */

/**
 * Parse break statement.
 */
static void
parser_parse_break_statement (parser_context_t *context_p) /**< context */
{
  parser_stack_iterator_t iterator;
  cbc_opcode_t opcode = CBC_JUMP_FORWARD;

  lexer_next_token (context_p);
  parser_stack_iterator_init (context_p, &iterator);

  if (!(context_p->token.flags & LEXER_WAS_NEWLINE)
      && context_p->token.type == LEXER_LITERAL
      && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
  {
    /* The label with the same name is searched on the stack. */
    while (true)
    {
      uint8_t type = parser_stack_iterator_read_uint8 (&iterator);
      if (type == PARSER_STATEMENT_START)
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_BREAK_LABEL);
      }

      if (type == PARSER_STATEMENT_FOR_IN
#if ENABLED (JERRY_ES2015)
          || type == PARSER_STATEMENT_FOR_OF
#endif /* ENABLED (JERRY_ES2015) */
          || type == PARSER_STATEMENT_WITH
#if ENABLED (JERRY_ES2015)
          || type == PARSER_STATEMENT_BLOCK_CONTEXT
#endif /* ENABLED (JERRY_ES2015) */
          || type == PARSER_STATEMENT_TRY)
      {
        opcode = CBC_JUMP_FORWARD_EXIT_CONTEXT;
      }

      if (type == PARSER_STATEMENT_LABEL)
      {
        parser_label_statement_t label_statement;

        parser_stack_iterator_skip (&iterator, 1);
        parser_stack_iterator_read (&iterator, &label_statement, sizeof (parser_label_statement_t));

        if (lexer_compare_identifier_to_current (context_p, &label_statement.label_ident))
        {
          label_statement.break_list_p = parser_emit_cbc_forward_branch_item (context_p,
                                                                              (uint16_t) opcode,
                                                                              label_statement.break_list_p);
          parser_stack_iterator_write (&iterator, &label_statement, sizeof (parser_label_statement_t));
          lexer_next_token (context_p);
          return;
        }
        parser_stack_iterator_skip (&iterator, sizeof (parser_label_statement_t));
      }
      else
      {
        parser_stack_iterator_skip (&iterator, parser_statement_length (type));
      }
    }
  }

  /* The first switch or loop statement is searched. */
  while (true)
  {
    uint8_t type = parser_stack_iterator_read_uint8 (&iterator);
    if (type == PARSER_STATEMENT_START)
    {
      parser_raise_error (context_p, PARSER_ERR_INVALID_BREAK);
    }

    if (type == PARSER_STATEMENT_FOR_IN
#if ENABLED (JERRY_ES2015)
        || type == PARSER_STATEMENT_FOR_OF
#endif /* ENABLED (JERRY_ES2015) */
        || type == PARSER_STATEMENT_WITH
#if ENABLED (JERRY_ES2015)
        || type == PARSER_STATEMENT_BLOCK_CONTEXT
#endif /* ENABLED (JERRY_ES2015) */
        || type == PARSER_STATEMENT_TRY)
    {
      opcode = CBC_JUMP_FORWARD_EXIT_CONTEXT;
    }

    if (type == PARSER_STATEMENT_SWITCH
        || type == PARSER_STATEMENT_SWITCH_NO_DEFAULT
        || type == PARSER_STATEMENT_DO_WHILE
        || type == PARSER_STATEMENT_WHILE
        || type == PARSER_STATEMENT_FOR
#if ENABLED (JERRY_ES2015)
        || type == PARSER_STATEMENT_FOR_OF
#endif /* ENABLED (JERRY_ES2015) */
        || type == PARSER_STATEMENT_FOR_IN)
    {
      parser_loop_statement_t loop;

      parser_stack_iterator_skip (&iterator, 1);
      parser_stack_iterator_read (&iterator, &loop, sizeof (parser_loop_statement_t));
      loop.branch_list_p = parser_emit_cbc_forward_branch_item (context_p,
                                                                (uint16_t) opcode,
                                                                loop.branch_list_p);
      parser_stack_iterator_write (&iterator, &loop, sizeof (parser_loop_statement_t));
      return;
    }

    parser_stack_iterator_skip (&iterator, parser_statement_length (type));
  }
} /* parser_parse_break_statement */

/**
 * Parse continue statement.
 */
static void
parser_parse_continue_statement (parser_context_t *context_p) /**< context */
{
  parser_stack_iterator_t iterator;
  cbc_opcode_t opcode = CBC_JUMP_FORWARD;

  lexer_next_token (context_p);
  parser_stack_iterator_init (context_p, &iterator);

  if (!(context_p->token.flags & LEXER_WAS_NEWLINE)
      && context_p->token.type == LEXER_LITERAL
      && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
  {
    parser_stack_iterator_t loop_iterator;
    bool for_in_of_was_seen = false;

    loop_iterator.current_p = NULL;

    /* The label with the same name is searched on the stack. */
    while (true)
    {
      uint8_t type = parser_stack_iterator_read_uint8 (&iterator);

      if (type == PARSER_STATEMENT_START)
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_CONTINUE_LABEL);
      }

      /* Only those labels are checked, whose are label of a loop. */
      if (loop_iterator.current_p != NULL && type == PARSER_STATEMENT_LABEL)
      {
        parser_label_statement_t label_statement;

        parser_stack_iterator_skip (&iterator, 1);
        parser_stack_iterator_read (&iterator, &label_statement, sizeof (parser_label_statement_t));

        if (lexer_compare_identifier_to_current (context_p, &label_statement.label_ident))
        {
          parser_loop_statement_t loop;

          parser_stack_iterator_skip (&loop_iterator, 1);
          parser_stack_iterator_read (&loop_iterator, &loop, sizeof (parser_loop_statement_t));
          loop.branch_list_p = parser_emit_cbc_forward_branch_item (context_p,
                                                                    (uint16_t) opcode,
                                                                    loop.branch_list_p);
          loop.branch_list_p->branch.offset |= CBC_HIGHEST_BIT_MASK;
          parser_stack_iterator_write (&loop_iterator, &loop, sizeof (parser_loop_statement_t));
          lexer_next_token (context_p);
          return;
        }
        parser_stack_iterator_skip (&iterator, sizeof (parser_label_statement_t));
        continue;
      }

#if ENABLED (JERRY_ES2015)
      bool is_for_in_of_statement = (type == PARSER_STATEMENT_FOR_IN) || (type == PARSER_STATEMENT_FOR_OF);
#else /* !ENABLED (JERRY_ES2015) */
      bool is_for_in_of_statement = (type == PARSER_STATEMENT_FOR_IN);
#endif /* ENABLED (JERRY_ES2015) */

      if (type == PARSER_STATEMENT_WITH
#if ENABLED (JERRY_ES2015)
          || type == PARSER_STATEMENT_BLOCK_CONTEXT
#endif /* ENABLED (JERRY_ES2015) */
          || type == PARSER_STATEMENT_TRY
          || for_in_of_was_seen)
      {
        opcode = CBC_JUMP_FORWARD_EXIT_CONTEXT;
      }
      else if (is_for_in_of_statement)
      {
        for_in_of_was_seen = true;
      }

      if (type == PARSER_STATEMENT_DO_WHILE
          || type == PARSER_STATEMENT_WHILE
          || type == PARSER_STATEMENT_FOR
#if ENABLED (JERRY_ES2015)
          || type == PARSER_STATEMENT_FOR_OF
#endif /* ENABLED (JERRY_ES2015) */
          || type == PARSER_STATEMENT_FOR_IN)
      {
        loop_iterator = iterator;
      }
      else
      {
        loop_iterator.current_p = NULL;
      }

      parser_stack_iterator_skip (&iterator, parser_statement_length (type));
    }
  }

  /* The first loop statement is searched. */
  while (true)
  {
    uint8_t type = parser_stack_iterator_read_uint8 (&iterator);
    if (type == PARSER_STATEMENT_START)
    {
      parser_raise_error (context_p, PARSER_ERR_INVALID_CONTINUE);
    }

    if (type == PARSER_STATEMENT_DO_WHILE
        || type == PARSER_STATEMENT_WHILE
        || type == PARSER_STATEMENT_FOR
#if ENABLED (JERRY_ES2015)
        || type == PARSER_STATEMENT_FOR_OF
#endif /* ENABLED (JERRY_ES2015) */
        || type == PARSER_STATEMENT_FOR_IN)
    {
      parser_loop_statement_t loop;

      parser_stack_iterator_skip (&iterator, 1);
      parser_stack_iterator_read (&iterator, &loop, sizeof (parser_loop_statement_t));
      loop.branch_list_p = parser_emit_cbc_forward_branch_item (context_p,
                                                                (uint16_t) opcode,
                                                                loop.branch_list_p);
      loop.branch_list_p->branch.offset |= CBC_HIGHEST_BIT_MASK;
      parser_stack_iterator_write (&iterator, &loop, sizeof (parser_loop_statement_t));
      return;
    }

    if (type == PARSER_STATEMENT_WITH
#if ENABLED (JERRY_ES2015)
        || type == PARSER_STATEMENT_BLOCK_CONTEXT
#endif /* ENABLED (JERRY_ES2015) */
        || type == PARSER_STATEMENT_TRY)
    {
      opcode = CBC_JUMP_FORWARD_EXIT_CONTEXT;
    }

    parser_stack_iterator_skip (&iterator, parser_statement_length (type));
  }
} /* parser_parse_continue_statement */

#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
/**
 * Parse import statement.
 * Note: See 15.2.2
 */
static void
parser_parse_import_statement (parser_context_t *context_p) /**< parser context */
{
  JERRY_ASSERT (context_p->token.type == LEXER_KEYW_IMPORT);

  parser_module_check_request_place (context_p);
  parser_module_context_init ();

  context_p->module_current_node_p = parser_module_create_module_node (context_p);

  lexer_next_token (context_p);

  /* Check for a ModuleSpecifier*/
  if (context_p->token.type != LEXER_LITERAL
      || context_p->token.lit_location.type != LEXER_STRING_LITERAL)
  {
    if (!(context_p->token.type == LEXER_LEFT_BRACE
          || context_p->token.type == LEXER_MULTIPLY
          || (context_p->token.type == LEXER_LITERAL && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)))
    {
      parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_MULTIPLY_LITERAL_EXPECTED);
    }

    if (context_p->token.type == LEXER_LITERAL)
    {
      /* Handle ImportedDefaultBinding */
      lexer_construct_literal_object (context_p, &context_p->token.lit_location, LEXER_IDENT_LITERAL);

      ecma_string_t *local_name_p = ecma_new_ecma_string_from_utf8 (context_p->lit_object.literal_p->u.char_p,
                                                                    context_p->lit_object.literal_p->prop.length);

      if (parser_module_check_duplicate_import (context_p, local_name_p))
      {
        ecma_deref_ecma_string (local_name_p);
        parser_raise_error (context_p, PARSER_ERR_DUPLICATED_IMPORT_BINDING);
      }

      ecma_string_t *import_name_p = ecma_get_magic_string (LIT_MAGIC_STRING_DEFAULT);
      parser_module_add_names_to_node (context_p, import_name_p, local_name_p);

      ecma_deref_ecma_string (local_name_p);
      ecma_deref_ecma_string (import_name_p);

      lexer_next_token (context_p);

      if (context_p->token.type == LEXER_COMMA)
      {
        lexer_next_token (context_p);
        if (context_p->token.type != LEXER_MULTIPLY
            && context_p->token.type != LEXER_LEFT_BRACE)
        {
          parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_MULTIPLY_EXPECTED);
        }
      }
      else if (!lexer_compare_literal_to_identifier (context_p, "from", 4))
      {
        parser_raise_error (context_p, PARSER_ERR_FROM_COMMA_EXPECTED);
      }
    }

    if (context_p->token.type == LEXER_MULTIPLY)
    {
      /* NameSpaceImport*/
      lexer_next_token (context_p);
      if (!lexer_compare_literal_to_identifier (context_p, "as", 2))
      {
        parser_raise_error (context_p, PARSER_ERR_AS_EXPECTED);
      }

      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_LITERAL)
      {
        parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
      }

      lexer_construct_literal_object (context_p, &context_p->token.lit_location, LEXER_IDENT_LITERAL);

      ecma_string_t *local_name_p = ecma_new_ecma_string_from_utf8 (context_p->lit_object.literal_p->u.char_p,
                                                                    context_p->lit_object.literal_p->prop.length);

      if (parser_module_check_duplicate_import (context_p, local_name_p))
      {
        ecma_deref_ecma_string (local_name_p);
        parser_raise_error (context_p, PARSER_ERR_DUPLICATED_IMPORT_BINDING);
      }

      ecma_string_t *import_name_p = ecma_get_magic_string (LIT_MAGIC_STRING_ASTERIX_CHAR);

      parser_module_add_names_to_node (context_p, import_name_p, local_name_p);
      ecma_deref_ecma_string (local_name_p);
      ecma_deref_ecma_string (import_name_p);

      lexer_next_token (context_p);
    }
    else if (context_p->token.type == LEXER_LEFT_BRACE)
    {
      /* Handle NamedImports */
      parser_module_parse_import_clause (context_p);
    }

    if (!lexer_compare_literal_to_identifier (context_p, "from", 4))
    {
      parser_raise_error (context_p, PARSER_ERR_FROM_EXPECTED);
    }
    lexer_next_token (context_p);
  }

  parser_module_handle_module_specifier (context_p);
  parser_module_add_import_node_to_context (context_p);

  context_p->module_current_node_p = NULL;
} /* parser_parse_import_statement */

/**
 * Parse export statement.
 */
static void
parser_parse_export_statement (parser_context_t *context_p) /**< context */
{
  JERRY_ASSERT (context_p->token.type == LEXER_KEYW_EXPORT);

  parser_module_check_request_place (context_p);
  parser_module_context_init ();

  context_p->module_current_node_p = parser_module_create_module_node (context_p);

  lexer_next_token (context_p);
  switch (context_p->token.type)
  {
    case LEXER_KEYW_DEFAULT:
    {
      scanner_location_t location;
      scanner_get_location (&location, context_p);

      context_p->status_flags |= PARSER_MODULE_STORE_IDENT;

      lexer_next_token (context_p);
      if (context_p->token.type == LEXER_KEYW_CLASS)
      {
        context_p->status_flags |= PARSER_MODULE_DEFAULT_CLASS_OR_FUNC;
        parser_parse_class (context_p, true);
      }
      else if (context_p->token.type == LEXER_KEYW_FUNCTION)
      {
        context_p->status_flags |= PARSER_MODULE_DEFAULT_CLASS_OR_FUNC;
        parser_parse_function_statement (context_p);
      }
      else
      {
        /* Assignment expression */
        scanner_set_location (context_p, &location);

        /* 15.2.3.5 Use the synthetic name '*default*' as the identifier. */
        lexer_construct_literal_object (context_p, &lexer_default_literal, lexer_default_literal.type);

        context_p->token.lit_location.type = LEXER_IDENT_LITERAL;
        parser_emit_cbc_literal_from_token (context_p, CBC_PUSH_LITERAL);

        context_p->module_identifier_lit_p = context_p->lit_object.literal_p;

        /* Fake an assignment to the default identifier */
        context_p->token.type = LEXER_ASSIGN;

        parser_parse_expression_statement (context_p, PARSE_EXPR_NO_COMMA | PARSE_EXPR_HAS_LITERAL);
      }

      ecma_string_t *name_p = ecma_new_ecma_string_from_utf8 (context_p->module_identifier_lit_p->u.char_p,
                                                              context_p->module_identifier_lit_p->prop.length);
      ecma_string_t *export_name_p = ecma_get_magic_string (LIT_MAGIC_STRING_DEFAULT);

      if (parser_module_check_duplicate_export (context_p, export_name_p))
      {
        ecma_deref_ecma_string (name_p);
        ecma_deref_ecma_string (export_name_p);
        parser_raise_error (context_p, PARSER_ERR_DUPLICATED_EXPORT_IDENTIFIER);
      }

      parser_module_add_names_to_node (context_p,
                                       export_name_p,
                                       name_p);
      ecma_deref_ecma_string (name_p);
      ecma_deref_ecma_string (export_name_p);
      break;
    }
    case LEXER_MULTIPLY:
    {
      lexer_next_token (context_p);
      if (!lexer_compare_literal_to_identifier (context_p, "from", 4))
      {
        parser_raise_error (context_p, PARSER_ERR_FROM_EXPECTED);
      }

      lexer_next_token (context_p);
      parser_module_handle_module_specifier (context_p);
      break;
    }
    case LEXER_KEYW_VAR:
#if ENABLED (JERRY_ES2015)
    case LEXER_KEYW_LET:
    case LEXER_KEYW_CONST:
#endif /* ENABLED (JERRY_ES2015) */
    {
      context_p->status_flags |= PARSER_MODULE_STORE_IDENT;
      parser_parse_var_statement (context_p);
      ecma_string_t *name_p = ecma_new_ecma_string_from_utf8 (context_p->module_identifier_lit_p->u.char_p,
                                                              context_p->module_identifier_lit_p->prop.length);

      if (parser_module_check_duplicate_export (context_p, name_p))
      {
        ecma_deref_ecma_string (name_p);
        parser_raise_error (context_p, PARSER_ERR_DUPLICATED_EXPORT_IDENTIFIER);
      }

      parser_module_add_names_to_node (context_p,
                                       name_p,
                                       name_p);
      ecma_deref_ecma_string (name_p);
      break;
    }
    case LEXER_KEYW_CLASS:
    {
      context_p->status_flags |= PARSER_MODULE_STORE_IDENT;
      parser_parse_class (context_p, true);
      ecma_string_t *name_p = ecma_new_ecma_string_from_utf8 (context_p->module_identifier_lit_p->u.char_p,
                                                              context_p->module_identifier_lit_p->prop.length);

      if (parser_module_check_duplicate_export (context_p, name_p))
      {
        ecma_deref_ecma_string (name_p);
        parser_raise_error (context_p, PARSER_ERR_DUPLICATED_EXPORT_IDENTIFIER);
      }

      parser_module_add_names_to_node (context_p,
                                       name_p,
                                       name_p);
      ecma_deref_ecma_string (name_p);
      break;
    }
    case LEXER_KEYW_FUNCTION:
    {
      context_p->status_flags |= PARSER_MODULE_STORE_IDENT;
      parser_parse_function_statement (context_p);
      ecma_string_t *name_p = ecma_new_ecma_string_from_utf8 (context_p->module_identifier_lit_p->u.char_p,
                                                              context_p->module_identifier_lit_p->prop.length);

      if (parser_module_check_duplicate_export (context_p, name_p))
      {
        ecma_deref_ecma_string (name_p);
        parser_raise_error (context_p, PARSER_ERR_DUPLICATED_EXPORT_IDENTIFIER);
      }

      parser_module_add_names_to_node (context_p,
                                       name_p,
                                       name_p);
      ecma_deref_ecma_string (name_p);
      break;
    }
    case LEXER_LEFT_BRACE:
    {
      parser_module_parse_export_clause (context_p);

      if (lexer_compare_literal_to_identifier (context_p, "from", 4))
      {
        lexer_next_token (context_p);
        parser_module_handle_module_specifier (context_p);
      }
      break;
    }
    default:
    {
      parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_MULTIPLY_LITERAL_EXPECTED);
      break;
    }
  }

  context_p->status_flags &= (uint32_t) ~(PARSER_MODULE_DEFAULT_CLASS_OR_FUNC | PARSER_MODULE_STORE_IDENT);
  parser_module_add_export_node_to_context (context_p);
  context_p->module_current_node_p = NULL;
} /* parser_parse_export_statement */
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */

/**
 * Parse label statement.
 */
static void
parser_parse_label (parser_context_t *context_p) /**< context */
{
  parser_stack_iterator_t iterator;
  parser_label_statement_t label_statement;

  parser_stack_iterator_init (context_p, &iterator);

  while (true)
  {
    uint8_t type = parser_stack_iterator_read_uint8 (&iterator);
    if (type == PARSER_STATEMENT_START)
    {
      break;
    }

    if (type == PARSER_STATEMENT_LABEL)
    {
      parser_stack_iterator_skip (&iterator, 1);
      parser_stack_iterator_read (&iterator, &label_statement, sizeof (parser_label_statement_t));
      parser_stack_iterator_skip (&iterator, sizeof (parser_label_statement_t));

      if (lexer_compare_identifier_to_current (context_p, &label_statement.label_ident))
      {
        parser_raise_error (context_p, PARSER_ERR_DUPLICATED_LABEL);
      }
    }
    else
    {
      parser_stack_iterator_skip (&iterator, parser_statement_length (type));
    }
  }

  label_statement.label_ident = context_p->token.lit_location;
  label_statement.break_list_p = NULL;
  parser_stack_push (context_p, &label_statement, sizeof (parser_label_statement_t));
  parser_stack_push_uint8 (context_p, PARSER_STATEMENT_LABEL);
  parser_stack_iterator_init (context_p, &context_p->last_statement);
} /* parser_parse_label */

/**
 * Parse statements.
 */
void
parser_parse_statements (parser_context_t *context_p) /**< context */
{
  /* Statement parsing cannot be nested. */
  JERRY_ASSERT (context_p->last_statement.current_p == NULL);
  parser_stack_push_uint8 (context_p, PARSER_STATEMENT_START);
  parser_stack_iterator_init (context_p, &context_p->last_statement);

#if ENABLED (JERRY_DEBUGGER)
  /* Set lexical enviroment for the debugger. */
  if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
  {
    context_p->status_flags |= PARSER_LEXICAL_ENV_NEEDED;
    context_p->last_breakpoint_line = 0;
  }
#endif /* ENABLED (JERRY_DEBUGGER) */

#if ENABLED (JERRY_LINE_INFO) || ENABLED (JERRY_ES2015_MODULE_SYSTEM)
  if (JERRY_CONTEXT (resource_name) != ECMA_VALUE_UNDEFINED)
  {
    parser_emit_cbc_ext (context_p, CBC_EXT_RESOURCE_NAME);
    parser_flush_cbc (context_p);
  }
#endif /* ENABLED (JERRY_LINE_INFO) || ENABLED (JERRY_ES2015_MODULE_SYSTEM) */
#if ENABLED (JERRY_LINE_INFO)
  context_p->last_line_info_line = 0;
#endif /* ENABLED (JERRY_LINE_INFO) */

  while (context_p->token.type == LEXER_LITERAL
         && context_p->token.lit_location.type == LEXER_STRING_LITERAL)
  {
    lexer_lit_location_t lit_location;
    uint32_t status_flags = context_p->status_flags;

    JERRY_ASSERT (context_p->stack_depth == JERRY_GET_EXPECTED_DEPTH (context_p));
#ifndef JERRY_NDEBUG
    JERRY_ASSERT (context_p->context_stack_depth == context_p->stack_depth);
#endif /* !JERRY_NDEBUG */

    lit_location = context_p->token.lit_location;

    if (lit_location.length == PARSER_USE_STRICT_LENGTH
        && !lit_location.has_escape
        && memcmp (PARSER_USE_STRICT_LITERAL, lit_location.char_p, PARSER_USE_STRICT_LENGTH) == 0)
    {
      context_p->status_flags |= PARSER_IS_STRICT;
    }

    lexer_next_token (context_p);

    if (context_p->token.type != LEXER_SEMICOLON
        && context_p->token.type != LEXER_RIGHT_BRACE
        && (!(context_p->token.flags & LEXER_WAS_NEWLINE)
            || LEXER_IS_BINARY_OP_TOKEN (context_p->token.type)
            || context_p->token.type == LEXER_LEFT_PAREN
            || context_p->token.type == LEXER_LEFT_SQUARE
            || context_p->token.type == LEXER_DOT))
    {
      /* The string is part of an expression statement. */
      context_p->status_flags = status_flags;

#if ENABLED (JERRY_DEBUGGER)
      if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
      {
        JERRY_ASSERT (context_p->last_breakpoint_line == 0);

        parser_emit_cbc (context_p, CBC_BREAKPOINT_DISABLED);
        parser_flush_cbc (context_p);

        parser_append_breakpoint_info (context_p, JERRY_DEBUGGER_BREAKPOINT_LIST, context_p->token.line);

        context_p->last_breakpoint_line = context_p->token.line;
      }
#endif /* ENABLED (JERRY_DEBUGGER) */
#if ENABLED (JERRY_LINE_INFO)
      parser_emit_line_info (context_p, context_p->token.line, false);
#endif /* ENABLED (JERRY_LINE_INFO) */

      lexer_construct_literal_object (context_p, &lit_location, LEXER_STRING_LITERAL);
      parser_emit_cbc_literal_from_token (context_p, CBC_PUSH_LITERAL);
      /* The extra_value is used for saving the token. */
      context_p->token.extra_value = context_p->token.type;
      context_p->token.type = LEXER_EXPRESSION_START;
      break;
    }

#if ENABLED (JERRY_PARSER_DUMP_BYTE_CODE)
    if (context_p->is_show_opcodes
        && !(status_flags & PARSER_IS_STRICT)
        && (context_p->status_flags & PARSER_IS_STRICT))
    {
      JERRY_DEBUG_MSG ("  Note: switch to strict mode\n\n");
    }
#endif /* ENABLED (JERRY_PARSER_DUMP_BYTE_CODE) */

    if (context_p->token.type == LEXER_SEMICOLON)
    {
      lexer_next_token (context_p);
    }

    /* The last directive prologue can be the result of the script. */
    if (!(context_p->status_flags & PARSER_IS_FUNCTION)
        && (context_p->token.type != LEXER_LITERAL
            || context_p->token.lit_location.type != LEXER_STRING_LITERAL))
    {
      lexer_construct_literal_object (context_p, &lit_location, LEXER_STRING_LITERAL);
      parser_emit_cbc_literal_from_token (context_p, CBC_PUSH_LITERAL);
      parser_emit_cbc (context_p, CBC_POP_BLOCK);
      parser_flush_cbc (context_p);
    }
  }

  if (context_p->status_flags & PARSER_IS_STRICT
      && context_p->status_flags & PARSER_HAS_NON_STRICT_ARG)
  {
    parser_raise_error (context_p, PARSER_ERR_NON_STRICT_ARG_DEFINITION);
  }

  while (context_p->token.type != LEXER_EOS
         || context_p->stack_top_uint8 != PARSER_STATEMENT_START)
  {
#ifndef JERRY_NDEBUG
    JERRY_ASSERT (context_p->stack_depth == context_p->context_stack_depth);
#endif /* !JERRY_NDEBUG */

#if ENABLED (JERRY_DEBUGGER)
    if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED
        && context_p->token.line != context_p->last_breakpoint_line
        && context_p->token.type != LEXER_SEMICOLON
        && context_p->token.type != LEXER_LEFT_BRACE
        && context_p->token.type != LEXER_RIGHT_BRACE
        && context_p->token.type != LEXER_KEYW_VAR
        && context_p->token.type != LEXER_KEYW_LET
        && context_p->token.type != LEXER_KEYW_CONST
        && context_p->token.type != LEXER_KEYW_FUNCTION
        && context_p->token.type != LEXER_KEYW_CASE
        && context_p->token.type != LEXER_KEYW_DEFAULT)
    {
      parser_emit_cbc (context_p, CBC_BREAKPOINT_DISABLED);
      parser_flush_cbc (context_p);

      parser_append_breakpoint_info (context_p, JERRY_DEBUGGER_BREAKPOINT_LIST, context_p->token.line);

      context_p->last_breakpoint_line = context_p->token.line;
    }
#endif /* ENABLED (JERRY_DEBUGGER) */

#if ENABLED (JERRY_LINE_INFO)
    if (context_p->token.line != context_p->last_line_info_line
        && context_p->token.type != LEXER_SEMICOLON
        && context_p->token.type != LEXER_LEFT_BRACE
        && context_p->token.type != LEXER_RIGHT_BRACE
        && context_p->token.type != LEXER_KEYW_VAR
        && context_p->token.type != LEXER_KEYW_LET
        && context_p->token.type != LEXER_KEYW_CONST
        && context_p->token.type != LEXER_KEYW_FUNCTION
        && context_p->token.type != LEXER_KEYW_CASE
        && context_p->token.type != LEXER_KEYW_DEFAULT)
    {
      parser_emit_line_info (context_p, context_p->token.line, true);
    }
#endif /* ENABLED (JERRY_LINE_INFO) */

    switch (context_p->token.type)
    {
      case LEXER_SEMICOLON:
      {
        break;
      }

      case LEXER_RIGHT_BRACE:
      {
        if (context_p->stack_top_uint8 == PARSER_STATEMENT_LABEL
            || context_p->stack_top_uint8 == PARSER_STATEMENT_IF
            || context_p->stack_top_uint8 == PARSER_STATEMENT_ELSE
            || context_p->stack_top_uint8 == PARSER_STATEMENT_DO_WHILE
            || context_p->stack_top_uint8 == PARSER_STATEMENT_WHILE
            || context_p->stack_top_uint8 == PARSER_STATEMENT_FOR
            || context_p->stack_top_uint8 == PARSER_STATEMENT_FOR_IN
#if ENABLED (JERRY_ES2015)
            || context_p->stack_top_uint8 == PARSER_STATEMENT_FOR_OF
#endif /* ENABLED (JERRY_ES2015) */
            || context_p->stack_top_uint8 == PARSER_STATEMENT_WITH)
        {
          parser_raise_error (context_p, PARSER_ERR_STATEMENT_EXPECTED);
        }
        break;
      }

      case LEXER_LEFT_BRACE:
      {
        uint8_t block_type = PARSER_STATEMENT_BLOCK;

#if ENABLED (JERRY_ES2015)
        parser_block_statement_t block_statement;
        block_statement.scope_stack_top = context_p->scope_stack_top;
        block_statement.scope_stack_reg_top = context_p->scope_stack_reg_top;

        if (context_p->next_scanner_info_p->source_p == context_p->source_p)
        {
          JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_BLOCK);

          if (scanner_is_context_needed (context_p))
          {
            parser_block_context_t block_context;

#ifndef JERRY_NDEBUG
            PARSER_PLUS_EQUAL_U16 (context_p->context_stack_depth, PARSER_BLOCK_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

            parser_emit_cbc_forward_branch (context_p,
                                            CBC_BLOCK_CREATE_CONTEXT,
                                            &block_context.branch);
            parser_stack_push (context_p, &block_context, sizeof (parser_block_context_t));
            block_type = PARSER_STATEMENT_BLOCK_CONTEXT;
          }

          scanner_create_variables (context_p, SCANNER_CREATE_VARS_NO_OPTS);
        }

        parser_stack_push (context_p, &block_statement, sizeof (parser_block_statement_t));
#endif /* ENABLED (JERRY_ES2015) */

        parser_stack_push_uint8 (context_p, block_type);
        parser_stack_iterator_init (context_p, &context_p->last_statement);
        lexer_next_token (context_p);
        continue;
      }

      case LEXER_KEYW_VAR:
#if ENABLED (JERRY_ES2015)
      case LEXER_KEYW_LET:
      case LEXER_KEYW_CONST:
#endif /* ENABLED (JERRY_ES2015) */
      {
        parser_parse_var_statement (context_p);
        break;
      }

#if ENABLED (JERRY_ES2015)
      case LEXER_KEYW_CLASS:
      {
        parser_parse_class (context_p, true);
        goto consume_last_statement;
      }
#endif /* ENABLED (JERRY_ES2015) */

#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
      case LEXER_KEYW_IMPORT:
      {
        parser_parse_import_statement (context_p);
        break;
      }

      case LEXER_KEYW_EXPORT:
      {
        parser_parse_export_statement (context_p);
        break;
      }
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */

      case LEXER_KEYW_FUNCTION:
      {
        parser_parse_function_statement (context_p);
        goto consume_last_statement;
      }

      case LEXER_KEYW_IF:
      {
        parser_parse_if_statement_start (context_p);
        continue;
      }

      case LEXER_KEYW_SWITCH:
      {
        parser_parse_switch_statement_start (context_p);
        continue;
      }

      case LEXER_KEYW_DO:
      {
        parser_do_while_statement_t do_while_statement;
        parser_loop_statement_t loop;

        JERRY_ASSERT (context_p->last_cbc_opcode == PARSER_CBC_UNAVAILABLE);

        do_while_statement.start_offset = context_p->byte_code_size;
        loop.branch_list_p = NULL;

        parser_stack_push (context_p, &do_while_statement, sizeof (parser_do_while_statement_t));
        parser_stack_push (context_p, &loop, sizeof (parser_loop_statement_t));
        parser_stack_push_uint8 (context_p, PARSER_STATEMENT_DO_WHILE);
        parser_stack_iterator_init (context_p, &context_p->last_statement);
        lexer_next_token (context_p);
        continue;
      }

      case LEXER_KEYW_WHILE:
      {
        parser_parse_while_statement_start (context_p);
        continue;
      }

      case LEXER_KEYW_FOR:
      {
        parser_parse_for_statement_start (context_p);
        continue;
      }

      case LEXER_KEYW_WITH:
      {
        parser_parse_with_statement_start (context_p);
        continue;
      }

      case LEXER_KEYW_TRY:
      {
        parser_try_statement_t try_statement;

        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_LEFT_BRACE)
        {
          parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_EXPECTED);
        }

#ifndef JERRY_NDEBUG
        PARSER_PLUS_EQUAL_U16 (context_p->context_stack_depth, PARSER_TRY_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

        try_statement.type = parser_try_block;
        parser_emit_cbc_ext_forward_branch (context_p,
                                            CBC_EXT_TRY_CREATE_CONTEXT,
                                            &try_statement.branch);

#if ENABLED (JERRY_ES2015)
        try_statement.scope_stack_top = context_p->scope_stack_top;
        try_statement.scope_stack_reg_top = context_p->scope_stack_reg_top;

        if (context_p->next_scanner_info_p->source_p == context_p->source_p)
        {
          JERRY_ASSERT (context_p->next_scanner_info_p->type == SCANNER_TYPE_BLOCK);

          if (scanner_is_context_needed (context_p))
          {
            parser_emit_cbc_ext (context_p, CBC_EXT_TRY_CREATE_ENV);
          }

          scanner_create_variables (context_p, SCANNER_CREATE_VARS_NO_OPTS);
        }
#endif /* ENABLED (JERRY_ES2015) */

        parser_stack_push (context_p, &try_statement, sizeof (parser_try_statement_t));
        parser_stack_push_uint8 (context_p, PARSER_STATEMENT_TRY);
        parser_stack_iterator_init (context_p, &context_p->last_statement);
        lexer_next_token (context_p);
        continue;
      }

      case LEXER_KEYW_DEFAULT:
      {
        parser_parse_default_statement (context_p);
        continue;
      }

      case LEXER_KEYW_CASE:
      {
        parser_parse_case_statement (context_p);
        continue;
      }

      case LEXER_KEYW_BREAK:
      {
        parser_parse_break_statement (context_p);
        break;
      }

      case LEXER_KEYW_CONTINUE:
      {
        parser_parse_continue_statement (context_p);
        break;
      }

      case LEXER_KEYW_THROW:
      {
        lexer_next_token (context_p);
        if (context_p->token.flags & LEXER_WAS_NEWLINE)
        {
          parser_raise_error (context_p, PARSER_ERR_EXPRESSION_EXPECTED);
        }
        parser_parse_expression (context_p, PARSE_EXPR);
        parser_emit_cbc (context_p, CBC_THROW);
        break;
      }

      case LEXER_KEYW_RETURN:
      {
        if (!(context_p->status_flags & PARSER_IS_FUNCTION))
        {
          parser_raise_error (context_p, PARSER_ERR_INVALID_RETURN);
        }

        lexer_next_token (context_p);

        if ((context_p->token.flags & LEXER_WAS_NEWLINE)
            || context_p->token.type == LEXER_SEMICOLON
            || context_p->token.type == LEXER_RIGHT_BRACE)
        {
#if ENABLED (JERRY_ES2015)
          if (JERRY_UNLIKELY (PARSER_IS_CLASS_CONSTRUCTOR_SUPER (context_p->status_flags)))
          {
            if (context_p->status_flags & PARSER_CLASS_IMPLICIT_SUPER)
            {
              parser_emit_cbc (context_p, CBC_PUSH_THIS);
            }
            else
            {
              parser_emit_cbc_ext (context_p, CBC_EXT_PUSH_CONSTRUCTOR_THIS);
            }
            parser_emit_cbc (context_p, CBC_RETURN);
          }
          else
          {
#endif /* ENABLED (JERRY_ES2015) */
            parser_emit_cbc (context_p, CBC_RETURN_WITH_BLOCK);
#if ENABLED (JERRY_ES2015)
          }
#endif /* ENABLED (JERRY_ES2015) */
          break;
        }

        parser_parse_expression (context_p, PARSE_EXPR);

        bool return_with_literal = (context_p->last_cbc_opcode == CBC_PUSH_LITERAL);
#if ENABLED (JERRY_ES2015)
        return_with_literal = return_with_literal && !PARSER_IS_CLASS_CONSTRUCTOR_SUPER (context_p->status_flags);
#endif /* ENABLED (JERRY_ES2015) */

        if (return_with_literal)
        {
          context_p->last_cbc_opcode = CBC_RETURN_WITH_LITERAL;
        }
        else
        {
#if ENABLED (JERRY_ES2015)
          if (JERRY_UNLIKELY (PARSER_IS_CLASS_CONSTRUCTOR_SUPER (context_p->status_flags)))
          {
            parser_emit_cbc_ext (context_p, CBC_EXT_CONSTRUCTOR_RETURN);
          }
          else
          {
#endif /* ENABLED (JERRY_ES2015) */
            parser_emit_cbc (context_p, CBC_RETURN);
#if ENABLED (JERRY_ES2015)
          }
#endif /* ENABLED (JERRY_ES2015) */
        }
        break;
      }

      case LEXER_KEYW_DEBUGGER:
      {
#if ENABLED (JERRY_DEBUGGER)
        /* This breakpoint location is not reported to the
         * debugger, so it is impossible to disable it. */
        if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
        {
          parser_emit_cbc (context_p, CBC_BREAKPOINT_ENABLED);
        }
#endif /* ENABLED (JERRY_DEBUGGER) */
        lexer_next_token (context_p);
        break;
      }
      case LEXER_LITERAL:
      {
        if (context_p->token.lit_location.type == LEXER_IDENT_LITERAL
            && lexer_check_next_character (context_p, LIT_CHAR_COLON))
        {
          parser_parse_label (context_p);
          lexer_next_token (context_p);
          JERRY_ASSERT (context_p->token.type == LEXER_COLON);
          lexer_next_token (context_p);
          continue;
        }
        /* FALLTHRU */
      }

      default:
      {
        int options = PARSE_EXPR;

        if (context_p->token.type == LEXER_EXPRESSION_START)
        {
          /* Restore the token type form the extra_value. */
          context_p->token.type = context_p->token.extra_value;
          options |= PARSE_EXPR_HAS_LITERAL;
        }

        if (context_p->status_flags & PARSER_IS_FUNCTION)
        {
          parser_parse_expression_statement (context_p, options);
        }
        else
        {
          parser_parse_block_expression (context_p, options);
        }

        break;
      }
    }

    parser_flush_cbc (context_p);

    if (context_p->token.type == LEXER_RIGHT_BRACE)
    {
      if (context_p->stack_top_uint8 == PARSER_STATEMENT_BLOCK)
      {
        parser_stack_pop_uint8 (context_p);

#if ENABLED (JERRY_ES2015)
        parser_block_statement_t block_statement;

        parser_stack_pop (context_p, &block_statement, sizeof (parser_block_statement_t));
        context_p->scope_stack_top = block_statement.scope_stack_top;
        context_p->scope_stack_reg_top = block_statement.scope_stack_reg_top;
#endif /* ENABLED (JERRY_ES2015) */

        parser_stack_iterator_init (context_p, &context_p->last_statement);
        lexer_next_token (context_p);
      }
#if ENABLED (JERRY_ES2015)
      else if (context_p->stack_top_uint8 == PARSER_STATEMENT_BLOCK_CONTEXT)
      {
        parser_block_statement_t block_statement;
        parser_block_context_t block_context;

        parser_stack_pop_uint8 (context_p);
        parser_stack_pop (context_p, &block_statement, sizeof (parser_block_statement_t));
        parser_stack_pop (context_p, &block_context, sizeof (parser_block_context_t));

        context_p->scope_stack_top = block_statement.scope_stack_top;
        context_p->scope_stack_reg_top = block_statement.scope_stack_reg_top;

        PARSER_MINUS_EQUAL_U16 (context_p->stack_depth, PARSER_BLOCK_CONTEXT_STACK_ALLOCATION);
#ifndef JERRY_NDEBUG
        PARSER_MINUS_EQUAL_U16 (context_p->context_stack_depth, PARSER_BLOCK_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

        parser_emit_cbc (context_p, CBC_CONTEXT_END);
        parser_set_branch_to_current_position (context_p, &block_context.branch);

        parser_stack_iterator_init (context_p, &context_p->last_statement);
        lexer_next_token (context_p);
      }
#endif /* ENABLED (JERRY_ES2015) */
      else if (context_p->stack_top_uint8 == PARSER_STATEMENT_SWITCH
               || context_p->stack_top_uint8 == PARSER_STATEMENT_SWITCH_NO_DEFAULT)
      {
        int has_default = (context_p->stack_top_uint8 == PARSER_STATEMENT_SWITCH);
        parser_loop_statement_t loop;
        parser_switch_statement_t switch_statement;

        parser_stack_pop_uint8 (context_p);
        parser_stack_pop (context_p, &loop, sizeof (parser_loop_statement_t));
        parser_stack_pop (context_p, &switch_statement, sizeof (parser_switch_statement_t));
        parser_stack_iterator_init (context_p, &context_p->last_statement);

#if ENABLED (JERRY_ES2015)
        context_p->scope_stack_top = switch_statement.scope_stack_top;
        context_p->scope_stack_reg_top = switch_statement.scope_stack_reg_top;
#endif /* ENABLED (JERRY_ES2015) */

        JERRY_ASSERT (switch_statement.branch_list_p == NULL);

        if (!has_default)
        {
          parser_set_branch_to_current_position (context_p, &switch_statement.default_branch);
        }

        parser_set_breaks_to_current_position (context_p, loop.branch_list_p);
        lexer_next_token (context_p);

#if ENABLED (JERRY_ES2015)
        if (context_p->stack_top_uint8 == PARSER_STATEMENT_SWITCH_BLOCK_CONTEXT)
        {
          parser_block_context_t block_context;

          parser_stack_pop_uint8 (context_p);
          parser_stack_pop (context_p, &block_context, sizeof (parser_block_context_t));

          PARSER_MINUS_EQUAL_U16 (context_p->stack_depth, PARSER_BLOCK_CONTEXT_STACK_ALLOCATION);
#ifndef JERRY_NDEBUG
          PARSER_MINUS_EQUAL_U16 (context_p->context_stack_depth, PARSER_BLOCK_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

          parser_emit_cbc (context_p, CBC_CONTEXT_END);
          parser_set_branch_to_current_position (context_p, &block_context.branch);

          parser_stack_iterator_init (context_p, &context_p->last_statement);
        }
#endif /* ENABLED (JERRY_ES2015) */
      }
      else if (context_p->stack_top_uint8 == PARSER_STATEMENT_TRY)
      {
        parser_parse_try_statement_end (context_p);
      }
      else if (context_p->stack_top_uint8 == PARSER_STATEMENT_START)
      {
        if (context_p->status_flags & PARSER_IS_CLOSURE)
        {
          parser_stack_pop_uint8 (context_p);
          context_p->last_statement.current_p = NULL;
          JERRY_ASSERT (context_p->stack_depth == 0);
#ifndef JERRY_NDEBUG
          JERRY_ASSERT (context_p->context_stack_depth == 0);
#endif /* !JERRY_NDEBUG */
          /* There is no lexer_next_token here, since the
           * next token belongs to the parent context. */

#if ENABLED (JERRY_ES2015)
          if (JERRY_UNLIKELY (PARSER_IS_CLASS_CONSTRUCTOR_SUPER (context_p->status_flags)))
          {
            if (context_p->status_flags & PARSER_CLASS_IMPLICIT_SUPER)
            {
              parser_emit_cbc (context_p, CBC_PUSH_THIS);
            }
            else
            {
              parser_emit_cbc_ext (context_p, CBC_EXT_PUSH_CONSTRUCTOR_THIS);
            }
            parser_emit_cbc (context_p, CBC_RETURN);
            parser_flush_cbc (context_p);
          }
#endif /* ENABLED (JERRY_ES2015) */
          return;
        }
        parser_raise_error (context_p, PARSER_ERR_INVALID_RIGHT_SQUARE);
      }
    }
    else if (context_p->token.type == LEXER_SEMICOLON)
    {
      lexer_next_token (context_p);
    }
    else if (context_p->token.type != LEXER_EOS
             && !(context_p->token.flags & LEXER_WAS_NEWLINE))
    {
      parser_raise_error (context_p, PARSER_ERR_SEMICOLON_EXPECTED);
    }

consume_last_statement:
    while (true)
    {
      switch (context_p->stack_top_uint8)
      {
        case PARSER_STATEMENT_LABEL:
        {
          parser_label_statement_t label;

          parser_stack_pop_uint8 (context_p);
          parser_stack_pop (context_p, &label, sizeof (parser_label_statement_t));
          parser_stack_iterator_init (context_p, &context_p->last_statement);

          parser_set_breaks_to_current_position (context_p, label.break_list_p);
          continue;
        }

        case PARSER_STATEMENT_IF:
        {
          if (parser_parse_if_statement_end (context_p))
          {
            break;
          }
          continue;
        }

        case PARSER_STATEMENT_ELSE:
        {
          parser_if_else_statement_t else_statement;

          parser_stack_pop_uint8 (context_p);
          parser_stack_pop (context_p, &else_statement, sizeof (parser_if_else_statement_t));
          parser_stack_iterator_init (context_p, &context_p->last_statement);

          parser_set_branch_to_current_position (context_p, &else_statement.branch);
          continue;
        }

        case PARSER_STATEMENT_DO_WHILE:
        {
          parser_parse_do_while_statement_end (context_p);
          if (context_p->token.type == LEXER_SEMICOLON)
          {
            lexer_next_token (context_p);
          }
          continue;
        }

        case PARSER_STATEMENT_WHILE:
        {
          parser_parse_while_statement_end (context_p);
          continue;
        }

        case PARSER_STATEMENT_FOR:
        {
          parser_parse_for_statement_end (context_p);
          continue;
        }

        case PARSER_STATEMENT_FOR_IN:
#if ENABLED (JERRY_ES2015)
        case PARSER_STATEMENT_FOR_OF:
#endif /* ENABLED (JERRY_ES2015) */
        {
          parser_for_in_of_statement_t for_in_of_statement;
          parser_loop_statement_t loop;

#if ENABLED (JERRY_ES2015)
          bool is_for_in = (context_p->stack_top_uint8 == PARSER_STATEMENT_FOR_IN);
#else
          bool is_for_in = true;
#endif /* ENABLED (JERRY_ES2015) */

          parser_stack_pop_uint8 (context_p);
          parser_stack_pop (context_p, &loop, sizeof (parser_loop_statement_t));
          parser_stack_pop (context_p, &for_in_of_statement, sizeof (parser_for_in_of_statement_t));
          parser_stack_iterator_init (context_p, &context_p->last_statement);

          parser_set_continues_to_current_position (context_p, loop.branch_list_p);

          parser_flush_cbc (context_p);
          PARSER_MINUS_EQUAL_U16 (context_p->stack_depth, is_for_in ? PARSER_FOR_IN_CONTEXT_STACK_ALLOCATION
                                                                    : PARSER_FOR_OF_CONTEXT_STACK_ALLOCATION);
#ifndef JERRY_NDEBUG
          PARSER_MINUS_EQUAL_U16 (context_p->context_stack_depth,
                                  is_for_in ? PARSER_FOR_IN_CONTEXT_STACK_ALLOCATION
                                            : PARSER_FOR_OF_CONTEXT_STACK_ALLOCATION);
#endif /* !JERRY_NDEBUG */

          parser_emit_cbc_ext_backward_branch (context_p,
                                               is_for_in ? CBC_EXT_BRANCH_IF_FOR_IN_HAS_NEXT
                                                         : CBC_EXT_BRANCH_IF_FOR_OF_HAS_NEXT,
                                               for_in_of_statement.start_offset);

          parser_set_breaks_to_current_position (context_p, loop.branch_list_p);
          parser_set_branch_to_current_position (context_p, &for_in_of_statement.branch);
          continue;
        }

        case PARSER_STATEMENT_WITH:
        {
          parser_parse_with_statement_end (context_p);
          continue;
        }

        default:
        {
          break;
        }
      }
      break;
    }
  }

  JERRY_ASSERT (context_p->stack_depth == JERRY_GET_EXPECTED_DEPTH (context_p));
#ifndef JERRY_NDEBUG
  JERRY_ASSERT (context_p->context_stack_depth == context_p->stack_depth);
#endif /* !JERRY_NDEBUG */

  parser_stack_pop_uint8 (context_p);
  context_p->last_statement.current_p = NULL;

  if (context_p->status_flags & PARSER_IS_CLOSURE)
  {
    parser_raise_error (context_p, PARSER_ERR_STATEMENT_EXPECTED);
  }
} /* parser_parse_statements */

/**
 * Free jumps stored on the stack if a parse error is occured.
 */
void JERRY_ATTR_NOINLINE
parser_free_jumps (parser_stack_iterator_t iterator) /**< iterator position */
{
  while (true)
  {
    uint8_t type = parser_stack_iterator_read_uint8 (&iterator);
    parser_branch_node_t *branch_list_p = NULL;

    switch (type)
    {
      case PARSER_STATEMENT_START:
      {
        return;
      }

      case PARSER_STATEMENT_LABEL:
      {
        parser_label_statement_t label;

        parser_stack_iterator_skip (&iterator, 1);
        parser_stack_iterator_read (&iterator, &label, sizeof (parser_label_statement_t));
        parser_stack_iterator_skip (&iterator, sizeof (parser_label_statement_t));
        branch_list_p = label.break_list_p;
        break;
      }

      case PARSER_STATEMENT_SWITCH:
      case PARSER_STATEMENT_SWITCH_NO_DEFAULT:
      {
        parser_switch_statement_t switch_statement;
        parser_loop_statement_t loop;

        parser_stack_iterator_skip (&iterator, 1);
        parser_stack_iterator_read (&iterator, &loop, sizeof (parser_loop_statement_t));
        parser_stack_iterator_skip (&iterator, sizeof (parser_loop_statement_t));
        parser_stack_iterator_read (&iterator, &switch_statement, sizeof (parser_switch_statement_t));
        parser_stack_iterator_skip (&iterator, sizeof (parser_switch_statement_t));

        branch_list_p = switch_statement.branch_list_p;
        while (branch_list_p != NULL)
        {
          parser_branch_node_t *next_p = branch_list_p->next_p;
          parser_free (branch_list_p, sizeof (parser_branch_node_t));
          branch_list_p = next_p;
        }
        branch_list_p = loop.branch_list_p;
        break;
      }

      case PARSER_STATEMENT_DO_WHILE:
      case PARSER_STATEMENT_WHILE:
      case PARSER_STATEMENT_FOR:
      case PARSER_STATEMENT_FOR_IN:
#if ENABLED (JERRY_ES2015)
      case PARSER_STATEMENT_FOR_OF:
#endif /* ENABLED (JERRY_ES2015) */
      {
        parser_loop_statement_t loop;

        parser_stack_iterator_skip (&iterator, 1);
        parser_stack_iterator_read (&iterator, &loop, sizeof (parser_loop_statement_t));
        parser_stack_iterator_skip (&iterator, parser_statement_length (type) - 1);
        branch_list_p = loop.branch_list_p;
        break;
      }

      default:
      {
        parser_stack_iterator_skip (&iterator, parser_statement_length (type));
        continue;
      }
    }

    while (branch_list_p != NULL)
    {
      parser_branch_node_t *next_p = branch_list_p->next_p;
      parser_free (branch_list_p, sizeof (parser_branch_node_t));
      branch_list_p = next_p;
    }
  }
} /* parser_free_jumps */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_PARSER) */

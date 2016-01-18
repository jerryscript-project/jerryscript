/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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

#include "bytecode-data.h"
#include "ecma-helpers.h"
#include "jrt-bit-fields.h"
#include "jrt-libc-includes.h"
#include "jsp-mm.h"
#include "opcodes.h"
#include "opcodes-dumper.h"
#include "parser.h"
#include "re-parser.h"
#include "scopes-tree.h"
#include "stack.h"
#include "jsp-early-error.h"
#include "jsp-internal.h"
#include "vm.h"
#include "rcs-records.h"

/**
 * Flag, indicating whether result of expression
 * evaluation should be stored to 'eval result'
 * temporary variable.
 *
 * In other words, the flag indicates whether
 * 'eval result' store code should be dumped.
 *
 * See also:
 *          parse_expression
 */
typedef enum
{
  JSP_EVAL_RET_STORE_NOT_DUMP, /**< do not dump */
  JSP_EVAL_RET_STORE_DUMP, /**< dump */
} jsp_eval_ret_store_t;

static token tok;
static bool parser_show_instrs = false;

#define EMIT_ERROR(type, MESSAGE) PARSE_ERROR(type, MESSAGE, tok.loc)
#define EMIT_ERROR_VARG(type, MESSAGE, ...) PARSE_ERROR_VARG(type, MESSAGE, tok.loc, __VA_ARGS__)

/**
 * Parse procedure
 *
 *  Parse of any Program production (ECMA-262 v5, 14) is performed in two stages:
 *   - preparse stage:
 *      - build tree of scopes (global / eval as root and functions as other nodes)
 *      - gather necessary properties for each scope:
 *          - maximum number of instructions
 *          - maximum number of per-opcode-block unique literals
 *          - flags, indicating whether scope contains `with`, `try`, etc.
 *      - perform syntax checks
 *   - dump stage:
 *      - dump byte-code instructions
 *      - fill idx-to-literal hash table
 *      - perform byte-code optimizations
 *
 * Parser context
 *  See also:
 *           jsp_ctx_t
 *
 * General description
 *  Each parse stage is performed using jsp_parse_source_element_list function with common general flow,
 *  and local branching for actions, specific for particular stage.
 *
 *  Parse of each element is performed in a specific parser state (see also, jsp_state_expr_t and jsp_state_t)
 *  that determines productions expected by parser in the state.
 *
 *  States are located on managed (non-native) parser stack, so the parser is recursive.
 *
 * Expression parse flow
 *  Whenever parser expects an expression, it pushes new state that is initially either JSP_STATE_EXPR_EMPTY or a
 *  particular fixed helper state.
 *
 *  The new state also contains required expression type, indicating what type of expression is currently expected.
 *  For states starting with JSP_STATE_EXPR_EMPTY, it is the expression type that indicates final production,
 *  that is expected to be parsed in the pushed state (AssignmentExpression, LeftHandSideExpression, etc.).
 *  In case of fixed helper states required type is the same as starting expression type.
 *
 *  In each state parser checks current production for whether it can be extended in some way, acceptable by grammar.
 *  If it can be extended, it is extended, possibly with pushing new state for parsing subexpression.
 *  Otherwise, it is just promoted to next (higher / more general) expression type to repeat check with rules,
 *  corresponding to the next type, or finish parse of current expression.
 *  Upon finishing parse, associated with the current expression / helper state, `is_completed` flag of the state is
 *  raised.
 *
 *  Finish of an expression parse is indicated using `is_subexpr_end` local flag in jsp_parse_source_element_list
 *  function. Upon finish, upper state is popped from stack and becomes current, merging the parsed subexpression
 *  into its state.
 *
 * Statements parse flow
 *  Parse of statements is very similar to parse of expressions.
 *
 *  The states are either fixed (like JSP_STATE_STAT_STATEMENT_LIST), or starting from JSP_STATE_STAT_EMPTY
 *  with required type equal to JSP_STATE_STAT_STATEMENT.
 *
 * Source elements list
 *  Source elements parse state is represented with the single type - JSP_STATE_SOURCE_ELEMENTS.
 *
 * Labelled statements
 *  Named labels are pushed as helper states to parser stack and are searched for upon occurence of break or continue
 *  on label.
 *
 *  Targets of simple break / continue are also searched through stack iteration, looking for iterational or switch
 *  statements.
 */
typedef enum __attr_packed___
{
  /* ECMA-262 v5 expression types */
  JSP_STATE_EXPR__BEGIN,

  JSP_STATE_EXPR_EMPTY,               /**< no expression yet (at start) */
  JSP_STATE_EXPR_FUNCTION,            /**< FunctionExpression (11.2.5) */
  JSP_STATE_EXPR_MEMBER,              /**< MemberExpression (11.2) */
  JSP_STATE_EXPR_CALL,                /**< CallExpression (11.2) */
  JSP_STATE_EXPR_LEFTHANDSIDE,        /**< LeftHandSideExpression (11.2) */

  JSP_STATE_EXPR__SIMPLE_BEGIN,

  JSP_STATE_EXPR_UNARY,               /**< UnaryExpression (11.4) */
  JSP_STATE_EXPR_MULTIPLICATIVE,      /**< MultiplicativeExpression (11.5) */
  JSP_STATE_EXPR_ADDITIVE,            /**< AdditiveExpression (11.6) */
  JSP_STATE_EXPR_SHIFT,               /**< ShiftExpression (11.7) */
  JSP_STATE_EXPR_RELATIONAL,          /**< RelationalExpression (11.8) */
  JSP_STATE_EXPR_EQUALITY,            /**< EqualityExpression (11.9) */
  JSP_STATE_EXPR_BITWISE_AND,         /**< BitwiseAndExpression (11.10) */
  JSP_STATE_EXPR_BITWISE_XOR,         /**< BitwiseXorExpression (11.10) */
  JSP_STATE_EXPR_BITWISE_OR,          /**< BitwiseOrExpression (11.10) */

  JSP_STATE_EXPR__SIMPLE_END,

  JSP_STATE_EXPR_LOGICAL_AND,         /**< LogicalAndExpression (11.11) */
  JSP_STATE_EXPR_LOGICAL_OR,          /**< LogicalOrExpression (11.11) */
  JSP_STATE_EXPR_CONDITION,           /**< ConditionalExpression (11.12) */
  JSP_STATE_EXPR_ASSIGNMENT,          /**< AssignmentExpression (11.13) */
  JSP_STATE_EXPR_EXPRESSION,          /**< Expression (11.14) */

  JSP_STATE_EXPR_ARRAY_LITERAL,       /**< ArrayLiteral (11.1.4) */
  JSP_STATE_EXPR_OBJECT_LITERAL,      /**< ObjectLiteral (11.1.5) */

  JSP_STATE_EXPR_DATA_PROP_DECL,      /**< a data property (ObjectLiteral, 11.1.5) */
  JSP_STATE_EXPR_ACCESSOR_PROP_DECL,  /**< an accessor's property getter / setter (ObjectLiteral, 11.1.5) */

  JSP_STATE_EXPR__END,

  JSP_STATE_STAT_EMPTY,              /**< no statement yet (at start) */
  JSP_STATE_STAT_IF_BRANCH_START,    /**< IfStatement branch start */
  JSP_STATE_STAT_IF_BRANCH_END,      /**< IfStatement branch start */
  JSP_STATE_STAT_STATEMENT,          /**< Statement */
  JSP_STATE_STAT_STATEMENT_LIST,     /**< Statement list */
  JSP_STATE_STAT_VAR_DECL,           /**< VariableStatement */
  JSP_STATE_STAT_VAR_DECL_FINISH,
  JSP_STATE_STAT_DO_WHILE,           /**< IterationStatement */
  JSP_STATE_STAT_WHILE,
  JSP_STATE_STAT_FOR_INIT_END,
  JSP_STATE_STAT_FOR_INCREMENT,
  JSP_STATE_STAT_FOR_COND,
  JSP_STATE_STAT_FOR_FINISH,
  JSP_STATE_STAT_FOR_IN,
  JSP_STATE_STAT_FOR_IN_EXPR,
  JSP_STATE_STAT_FOR_IN_FINISH,
  JSP_STATE_STAT_ITER_FINISH,
  JSP_STATE_STAT_SWITCH,
  JSP_STATE_STAT_SWITCH_BRANCH_EXPR,
  JSP_STATE_STAT_SWITCH_BRANCH,
  JSP_STATE_STAT_SWITCH_FINISH,
  JSP_STATE_STAT_TRY,
  JSP_STATE_STAT_CATCH_FINISH,
  JSP_STATE_STAT_FINALLY_FINISH,
  JSP_STATE_STAT_TRY_FINISH,
  JSP_STATE_STAT_WITH,
  JSP_STATE_STAT_EXPRESSION,
  JSP_STATE_STAT_RETURN,
  JSP_STATE_STAT_THROW,

  JSP_STATE_FUNC_DECL_FINISH,

  JSP_STATE_SOURCE_ELEMENTS,

  JSP_STATE_STAT_BLOCK,

  JSP_STATE_STAT_NAMED_LABEL
} jsp_state_expr_t;

typedef struct jsp_state_t
{
  mem_cpointer_t prev_state_cp; /** pointer to previous state on the stack, or MEM_CP_NULL - for bottom stack element */
  mem_cpointer_t next_state_cp; /** pointer to next state on the stack, or MEM_CP_NULL - for top stack element */
  jsp_state_expr_t state; /**< current state */
  jsp_state_expr_t req_state; /**< required state */

  uint8_t is_completed              : 1; /**< the expression parse completed,
                                          *   no more tokens can be added to the expression */
  uint8_t is_list_in_process        : 1; /**< parsing a list, associated with the expression
                                          *   (details depend on current expression type) */
  uint8_t is_no_in_mode             : 1; /**< expression is being parsed in NoIn mode (see also: ECMA-262 v5, 11.8) */
  uint8_t is_fixed_ret_operand      : 1; /**< the expression's evaluation should produce value that should be
                                          *   put to register, specified by operand, specified in state */
  uint8_t is_value_based_reference : 1; /**< flag, indicating whether current state represents evaluated expression
                                         *   that evaluated to a value-based reference */
  uint8_t is_get_value_dumped_for_main_operand : 1;
  uint8_t is_get_value_dumped_for_prop_operand : 1;
  uint8_t is_need_retval            : 1; /**< flag, indicating whether result of the expression's
                                          *   evaluation, if it is value, is used */
  uint8_t is_complex_production     : 1; /**< the expression is being parsed in complex production mode */
  uint8_t var_decl                  : 1; /**< this flag tells that we are parsing VariableStatement, not
                                              VariableDeclarationList or VariableDeclaration inside
                                              IterationStatement */
  uint8_t is_var_decl_no_in         : 1; /**< this flag tells that we are parsing VariableDeclrationNoIn inside
                                              ForIn iteration statement */
  uint8_t was_default               : 1; /**< was default branch seen */
  uint8_t is_default_branch         : 1; /**< marks default branch of switch statement */
  uint8_t is_simply_jumpable_border : 1; /**< flag, indicating whether simple jump could be performed
                                          *   from current statement outside of the statement */
  uint8_t is_dump_eval_ret_store    : 1; /**< expression's result should be stored to eval's return register */
  uint8_t is_stmt_list_control_flow_exit_stmt_occured : 1; /**< flag, indicating whether one of the following statements
                                                            *   occured immediately in the statement list, corresponding
                                                            *   to the statement:
                                                            *     - return
                                                            *     - break
                                                            *     - continue
                                                            *     - throw */

  union u
  {
    u (void)
    {
    }

    struct expression
    {
      union u
      {
        struct
        {
          uint32_t list_length;
          vm_instr_counter_t header_pos; /**< position of a varg header instruction */
          vm_idx_t reg_alloc_saved_state1;
          vm_idx_t reg_alloc_saved_state2;
        } varg_sequence;
        JERRY_STATIC_ASSERT (sizeof (varg_sequence) == 8); // Please, update size if changed

        struct
        {
          jsp_operand_t prop_name;
          bool is_setter;
        } accessor_prop_decl;
        JERRY_STATIC_ASSERT (sizeof (accessor_prop_decl) == 6); // Please, update size if changed

        struct
        {
          vm_instr_counter_t rewrite_chain; /**< chain of jmp instructions to rewrite */
        } logical_and;
        JERRY_STATIC_ASSERT (sizeof (logical_and) == 2); // Please, update size if changed

        struct
        {
          vm_instr_counter_t rewrite_chain; /**< chain of jmp instructions to rewrite */
        } logical_or;
        JERRY_STATIC_ASSERT (sizeof (logical_or) == 2); // Please, update size if changed

        struct
        {
          vm_instr_counter_t conditional_check_pos;
          vm_instr_counter_t jump_to_end_pos;
        } conditional;
        JERRY_STATIC_ASSERT (sizeof (conditional) == 4); // Please, update size if changed
      } u;
      JERRY_STATIC_ASSERT (sizeof (u) == 8); // Please, update size if changed

      jsp_operand_t operand; /**< operand, associated with expression */
      jsp_operand_t prop_name_operand; /**< operand, describing second part of a value-based reference,
                                        *   or empty operand (for Identifier references, values, or constants) */
      jsp_token_type_t token_type; /**< token, related to current and, if binary, to previous expression */
    } expression;
    JERRY_STATIC_ASSERT (sizeof (expression) == 20); // Please, update size if changed

    struct statement
    {
      union u
      {
        struct iterational
        {
          union u
          {
            struct loop_for_in
            {
              union u
              {
                locus iterator_expr_loc;
                locus body_loc;
              } u;

              lit_cpointer_t var_name_lit_cp;
              vm_instr_counter_t header_pos;
            } loop_for_in;
            JERRY_STATIC_ASSERT (sizeof (loop_for_in) == 8); // Please, update size if changed

            struct loop_while
            {
              union u
              {
                locus cond_expr_start_loc;
                locus end_loc;
              } u;

              vm_instr_counter_t next_iter_tgt_pos;
              vm_instr_counter_t jump_to_end_pos;
            } loop_while;
            JERRY_STATIC_ASSERT (sizeof (loop_while) == 8); // Please, update size if changed

            struct loop_do_while
            {
              vm_instr_counter_t next_iter_tgt_pos;
            } loop_do_while;
            JERRY_STATIC_ASSERT (sizeof (loop_do_while) == 2); // Please, update size if changed

            struct loop_for
            {
              union u1
              {
                locus body_loc;
                locus condition_expr_loc;
              } u1;

              union u2
              {
                locus increment_expr_loc;
                locus end_loc;
              } u2;

              vm_instr_counter_t next_iter_tgt_pos;
              vm_instr_counter_t jump_to_end_pos;
            } loop_for;
            JERRY_STATIC_ASSERT (sizeof (loop_for) == 12); // Please, update size if changed
          } u;
          JERRY_STATIC_ASSERT (sizeof (u) == 12); // Please, update size if changed

          vm_instr_counter_t continues_rewrite_chain;
          vm_instr_counter_t continue_tgt_oc;
        } iterational;
        JERRY_STATIC_ASSERT (sizeof (iterational) == 16); // Please, update size if changed

        struct if_statement
        {
          vm_instr_counter_t conditional_check_pos;
          vm_instr_counter_t jump_to_end_pos;
        } if_statement;
        JERRY_STATIC_ASSERT (sizeof (if_statement) == 4); // Please, update size if changed

        struct switch_statement
        {
          jsp_operand_t expr;

          vm_instr_counter_t default_label_oc; /**< MAX_OPCODES - if DefaultClause didn't occur,
                                                *   start of StatementList for the DefaultClause, otherwise */
          vm_instr_counter_t last_cond_check_jmp_oc; /**< position of last clause's check,
                                                      *   of MAX_OPCODES (at start, or after DefaultClause) */
          vm_instr_counter_t skip_check_jmp_oc; /**< position of check for whether next condition check
                                                    *   should be performed or skipped */
          vm_idx_t saved_reg_next;
          vm_idx_t saved_reg_max_for_temps;
        } switch_statement;
        JERRY_STATIC_ASSERT (sizeof (switch_statement) == 12); // Please, update size if changed

        struct with_statement
        {
          vm_instr_counter_t header_pos;
        } with_statement;
        JERRY_STATIC_ASSERT (sizeof (with_statement) == 2); // Please, update size if changed

        struct try_statement
        {
          vm_instr_counter_t try_pos;
          vm_instr_counter_t catch_pos;
          vm_instr_counter_t finally_pos;
        } try_statement;
        JERRY_STATIC_ASSERT (sizeof (try_statement) == 6); // Please, update size if changed
      } u;
      JERRY_STATIC_ASSERT (sizeof (u) == 16); // Please, update size if changed

      vm_instr_counter_t breaks_rewrite_chain;
    } statement;
    JERRY_STATIC_ASSERT (sizeof (statement) == 20); // Please, update size if changed

    struct named_label
    {
      lit_cpointer_t name_cp;
    } named_label;
    JERRY_STATIC_ASSERT (sizeof (named_label) == 2); // Please, update size if changed

    struct source_elements
    {
      uint16_t parent_scope_child_scopes_counter;

      vm_instr_counter_t reg_var_decl_oc;

      scope_type_t parent_scope_type;

      vm_idx_t saved_reg_next;
      vm_idx_t saved_reg_max_for_temps;

      union
      {
        mem_cpointer_t parent_scopes_tree_node_cp;
        mem_cpointer_t parent_bc_header_cp;
      } u;
    } source_elements;
    JERRY_STATIC_ASSERT (sizeof (source_elements) == 10); // Please, update size if changed
  } u;
  JERRY_STATIC_ASSERT (sizeof (u) == 20); // Please, update size if changed
} jsp_state_t;

JERRY_STATIC_ASSERT (sizeof (jsp_state_t) == 28); // Please, update if size is changed

static void jsp_parse_source_element_list (jsp_ctx_t *, scope_type_t);

/**
 * Initialize parser context
 */
void
jsp_init_ctx (jsp_ctx_t *ctx_p, /**< parser context */
              scope_type_t scope_type) /**< starting scope type (SCOPE_TYPE_GLOBAL or SCOPE_TYPE_EVAL) */
{
  JERRY_ASSERT (scope_type == SCOPE_TYPE_GLOBAL || scope_type == SCOPE_TYPE_EVAL);

  ctx_p->mode = PREPARSE;
  ctx_p->scope_type = scope_type;
  ctx_p->state_stack_p = NULL;
  ctx_p->u.preparse_stage.current_scope_p = NULL;
} /* jsp_init_ctx */

/**
 * Check whether current stage is the dump stage
 *
 * @return true / false
 */
bool
jsp_is_dump_mode (jsp_ctx_t *ctx_p) /**< parser context */
{
  return (ctx_p->mode == DUMP);
} /* jsp_is_dump_mode */

/**
 * Switch to dump stage
 */
void
jsp_switch_to_dump_mode (jsp_ctx_t *ctx_p, /**< parser context */
                         scopes_tree root_scope_p) /**< root of scopes tree */
{
  JERRY_ASSERT (ctx_p->mode == PREPARSE);
  JERRY_ASSERT (ctx_p->u.preparse_stage.current_scope_p == NULL);

  ctx_p->mode = DUMP;
  ctx_p->u.dump_stage.current_bc_header_p = NULL;
  ctx_p->u.dump_stage.next_scope_to_dump_p = root_scope_p;
} /* jsp_switch_to_dump_mode */

/**
 * Check whether currently parsed code is strict mode code
 *
 * @return true / false
 */
bool
jsp_is_strict_mode (jsp_ctx_t *ctx_p) /**< parser context */
{
  if (jsp_is_dump_mode (ctx_p))
  {
    bytecode_data_header_t *bc_header_p = jsp_get_current_bytecode_header (ctx_p);

    JERRY_ASSERT (bc_header_p != NULL);

    return bc_header_p->is_strict;
  }
  else
  {
    scopes_tree scope_p = jsp_get_current_scopes_tree_node (ctx_p);

    JERRY_ASSERT (scope_p != NULL);

    return scope_p->strict_mode;
  }
} /* jsp_is_strict_mode */

/**
 * Indicate that currently parsed code is strict mode code
 */
void
jsp_set_strict_mode (jsp_ctx_t *ctx_p) /**< parser context */
{
  if (jsp_is_dump_mode (ctx_p))
  {
    bytecode_data_header_t *bc_header_p = jsp_get_current_bytecode_header (ctx_p);

    JERRY_ASSERT (bc_header_p != NULL && bc_header_p->is_strict);
  }
  else
  {
    scopes_tree scope_p = jsp_get_current_scopes_tree_node (ctx_p);

    JERRY_ASSERT (scope_p != NULL);

    scope_p->strict_mode = true;
  }
} /* jsp_set_strict_mode */

/**
 * Get current scope type
 *
 * @return scope type
 */
scope_type_t
jsp_get_scope_type (jsp_ctx_t *ctx_p) /**< parser context */
{
  return ctx_p->scope_type;
} /* jsp_get_scope_type */

/**
 * Set current scope type
 */
void
jsp_set_scope_type (jsp_ctx_t *ctx_p, /**< parser context */
                    scope_type_t scope_type) /**< scope type */
{
  ctx_p->scope_type = scope_type;
} /* jsp_set_scope_type */

/**
 * Get number of current scope's child scopes, for which parse was already started
 *
 * @return the value
 */
uint16_t
jsp_get_processed_child_scopes_counter (jsp_ctx_t *ctx_p) /**< parser context */
{
  if (jsp_is_dump_mode (ctx_p))
  {
    bytecode_data_header_t *bc_header_p = jsp_get_current_bytecode_header (ctx_p);

    JERRY_ASSERT (ctx_p->processed_child_scopes_counter <= bc_header_p->func_scopes_count);
  }
  else
  {
    scopes_tree scope = jsp_get_current_scopes_tree_node (ctx_p);

    JERRY_ASSERT (ctx_p->processed_child_scopes_counter <= scope->child_scopes_num);
  }

  return ctx_p->processed_child_scopes_counter;
} /* jsp_get_processed_child_scopes_counter */

/**
 * Set number of current scope's child scopes, for which parse was already started
 */
void
jsp_set_processed_child_scopes_counter (jsp_ctx_t *ctx_p, /**< parser context */
                                        uint16_t processed_child_scopes_counter) /**< the new value */
{
  if (jsp_is_dump_mode (ctx_p))
  {
    bytecode_data_header_t *bc_header_p = jsp_get_current_bytecode_header (ctx_p);

    if (bc_header_p == NULL)
    {
      JERRY_ASSERT (processed_child_scopes_counter == 0);
    }
    else
    {
      JERRY_ASSERT (processed_child_scopes_counter <= bc_header_p->func_scopes_count);
    }
  }
  else
  {
    scopes_tree scope = jsp_get_current_scopes_tree_node (ctx_p);

    if (scope == NULL)
    {
      JERRY_ASSERT (processed_child_scopes_counter == 0);
    }
    else
    {
      JERRY_ASSERT (processed_child_scopes_counter <= scope->child_scopes_num);
    }
  }

  ctx_p->processed_child_scopes_counter = processed_child_scopes_counter;
} /* jsp_set_processed_child_scopes_counter */

/**
 * Get and increment number of current scope's child scopes, for which parse was already started
 *
 * @return the value before increment
 */
uint16_t
jsp_get_and_inc_processed_child_scopes_counter (jsp_ctx_t *ctx_p) /**< parser context */
{
  uint16_t counter = jsp_get_processed_child_scopes_counter (ctx_p);

  uint16_t ret = counter++;

  jsp_set_processed_child_scopes_counter (ctx_p, counter);

  return ret;
} /* jsp_get_and_inc_processed_child_scopes_counter */

/**
 * Get next scope to dump
 *
 * Actually, it the function is iterator interface, used for traversing scopes tree during dump stage.
 * It is assumed that node of scopes tree are linked into the list in pre-order traversal order.
 */
scopes_tree
jsp_get_next_scopes_tree_node_to_dump (jsp_ctx_t *ctx_p) /**< parser context */
{
  JERRY_ASSERT (ctx_p->mode == DUMP);

  scopes_tree node = ctx_p->u.dump_stage.next_scope_to_dump_p;
  JERRY_ASSERT (node != NULL);

  ctx_p->u.dump_stage.next_scope_to_dump_p = MEM_CP_GET_POINTER (scopes_tree_int,
                                                                 node->next_scope_cp);

  return node;
} /* jsp_get_next_scopes_tree_node_to_dump */

/**
 * Get current scope (node of scopes tree)
 *
 * Note:
 *      Valid only during preparse stage
 *
 * @return the node
 */
scopes_tree
jsp_get_current_scopes_tree_node (jsp_ctx_t *ctx_p) /**< parser context */
{
  JERRY_ASSERT (ctx_p->mode == PREPARSE);

  return ctx_p->u.preparse_stage.current_scope_p;
} /* jsp_get_current_scopes_tree_node */

/**
 * Set current scope (node of scopes tree)
 *
 * Note:
 *      Valid only during preparse stage
 */
void
jsp_set_current_scopes_tree_node (jsp_ctx_t *ctx_p, /**< parser context */
                                  scopes_tree scope_p) /**< scopes tree node */
{
  JERRY_ASSERT (ctx_p->mode == PREPARSE);
  JERRY_ASSERT (scope_p != ctx_p->u.preparse_stage.current_scope_p);

  ctx_p->u.preparse_stage.current_scope_p = scope_p;
  ctx_p->u.preparse_stage.tmp_lit_set_num = 0;
  jsp_set_processed_child_scopes_counter (ctx_p, 0);
} /* jsp_set_current_scopes_tree_node */

/**
 * Get current scope's byte-code data header
 *
 * Note:
 *      Valid only during dump stage
 *
 * @return the byte-code data header
 */
bytecode_data_header_t *
jsp_get_current_bytecode_header (jsp_ctx_t *ctx_p) /**< parser context */
{
  JERRY_ASSERT (ctx_p->mode == DUMP);

  return ctx_p->u.dump_stage.current_bc_header_p;
} /* jsp_get_current_bytecode_header */

/**
 * Set current scope's byte-code data header
 *
 * Note:
 *      Valid only during dump stage
 */
void
jsp_set_current_bytecode_header (jsp_ctx_t *ctx_p, /**< parser context */
                                 bytecode_data_header_t *bc_header_p) /**< byte-code data header */
{
  JERRY_ASSERT (ctx_p->mode == DUMP);

  ctx_p->u.dump_stage.current_bc_header_p = bc_header_p;
  jsp_set_processed_child_scopes_counter (ctx_p, 0);
} /* jsp_set_current_bytecode_header */

/**
 * Empty temporary literal set
 *
 * Note:
 *      Valid only during preparse stage
 *
 * See also:
 *          Definition of tmp_lit_set in jsp_ctx_t
 */
void
jsp_empty_tmp_literal_set (jsp_ctx_t *ctx_p) /**< parser context */
{
  JERRY_ASSERT (ctx_p->mode == PREPARSE);

  ctx_p->u.preparse_stage.tmp_lit_set_num = 0;
} /* jsp_empty_tmp_literal_set */

/**
 * Take into account the specified literal and adjust temporary literal set and unique literals counter accordingly
 *
 * Note:
 *      Valid only during preparse stage
 *
 * See also:
 *          Definition of tmp_lit_set in jsp_ctx_t
 */
void
jsp_account_next_bytecode_to_literal_reference (jsp_ctx_t *ctx_p, /**< parser context */
                                                lit_cpointer_t lit_cp) /**< literal identifier */
{
  JERRY_ASSERT (ctx_p->mode == PREPARSE);

  scopes_tree scope_p = jsp_get_current_scopes_tree_node (ctx_p);

  uint8_t tmp_hash_table_index;
  for (tmp_hash_table_index = 0;
       tmp_hash_table_index < ctx_p->u.preparse_stage.tmp_lit_set_num;
       tmp_hash_table_index++)
  {
    if (ctx_p->u.preparse_stage.tmp_lit_set[tmp_hash_table_index].packed_value == lit_cp.packed_value)
    {
      break;
    }
  }

  if (tmp_hash_table_index == ctx_p->u.preparse_stage.tmp_lit_set_num)
  {
    scope_p->max_uniq_literals_num++;

    if (ctx_p->u.preparse_stage.tmp_lit_set_num < SCOPE_TMP_LIT_SET_SIZE)
    {
      ctx_p->u.preparse_stage.tmp_lit_set[ctx_p->u.preparse_stage.tmp_lit_set_num++] = lit_cp;
    }
    else
    {
      JERRY_ASSERT (ctx_p->u.preparse_stage.tmp_lit_set_num != 0);

      ctx_p->u.preparse_stage.tmp_lit_set[ctx_p->u.preparse_stage.tmp_lit_set_num - 1u] = lit_cp;
    }
  }
} /* jsp_account_next_bytecode_to_literal_reference */

static bool
token_is (jsp_token_type_t tt)
{
  return (lexer_get_token_type (tok) == tt);
}

static uint16_t
token_data (void)
{
  return tok.uid;
}

/**
 * Get token data as `lit_cpointer_t`
 *
 * @return compressed pointer to token data
 */
static lit_cpointer_t
token_data_as_lit_cp (void)
{
  lit_cpointer_t cp;
  cp.packed_value = tok.uid;

  return cp;
} /* token_data_as_lit_cp */

static void
skip_token (jsp_ctx_t *ctx_p)
{
  tok = lexer_next_token (false, jsp_is_strict_mode (ctx_p));
}

/**
 * In case a regexp token is scanned as a division operator, rescan it
 */
static void
rescan_regexp_token (jsp_ctx_t *ctx_p)
{
  lexer_seek (tok.loc);
  tok = lexer_next_token (true, jsp_is_strict_mode (ctx_p));
} /* rescan_regexp_token */

static void
seek_token (jsp_ctx_t *ctx_p,
            locus loc)
{
  lexer_seek (loc);

  skip_token (ctx_p);
}

static bool
is_keyword (jsp_token_type_t tt)
{
  return (tt >= TOKEN_TYPE__KEYWORD_BEGIN && tt <= TOKEN_TYPE__KEYWORD_END);
}

static void
assert_keyword (jsp_token_type_t kw)
{
  if (!token_is (kw))
  {
    EMIT_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX, "Expected keyword '%s'", lexer_token_type_to_string (kw));
    JERRY_UNREACHABLE ();
  }
}

static void
current_token_must_be_check_and_skip_it (jsp_ctx_t *ctx_p,
                                         jsp_token_type_t tt)
{
  if (!token_is (tt))
  {
    EMIT_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX, "Expected '%s' token", lexer_token_type_to_string (tt));
  }

  skip_token (ctx_p);
}

static void
current_token_must_be (jsp_token_type_t tt)
{
  if (!token_is (tt))
  {
    EMIT_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX, "Expected '%s' token", lexer_token_type_to_string (tt));
  }
}

/**
 * Skip block, defined with braces of specified type
 *
 * Note:
 *      Missing corresponding brace is considered a syntax error
 *
 * Note:
 *      Opening brace of the block to skip should be set as current
 *      token when the routine is called
 */
static void
jsp_skip_braces (jsp_ctx_t *ctx_p, /**< parser context */
                 jsp_token_type_t brace_type) /**< type of the opening brace */
{
  current_token_must_be (brace_type);

  jsp_token_type_t closing_bracket_type;

  if (brace_type == TOK_OPEN_PAREN)
  {
    closing_bracket_type = TOK_CLOSE_PAREN;
  }
  else if (brace_type == TOK_OPEN_BRACE)
  {
    closing_bracket_type = TOK_CLOSE_BRACE;
  }
  else
  {
    JERRY_ASSERT (brace_type == TOK_OPEN_SQUARE);
    closing_bracket_type = TOK_CLOSE_SQUARE;
  }

  skip_token (ctx_p);

  while (!token_is (closing_bracket_type)
         && !token_is (TOK_EOF))
  {
    if (token_is (TOK_OPEN_PAREN)
        || token_is (TOK_OPEN_BRACE)
        || token_is (TOK_OPEN_SQUARE))
    {
      jsp_skip_braces (ctx_p, lexer_get_token_type (tok));
    }

    skip_token (ctx_p);
  }

  current_token_must_be (closing_bracket_type);
} /* jsp_skip_braces */

/**
 * Find next token of specified type before the specified location
 *
 * Note:
 *      If skip_brace_blocks is true, every { should correspond to } brace before search end location,
 *      otherwise a syntax error is raised.
 *
 * @return true - if token was found (in the case, it is the current token,
 *                                    and lexer locus points to it),
 *         false - otherwise (in the case, lexer locus points to end_loc).
 */
static bool
jsp_find_next_token_before_the_locus (jsp_ctx_t *ctx_p, /**< parser context */
                                      jsp_token_type_t token_to_find, /**< token to search for (except TOK_EOF) */
                                      locus end_loc, /**< location to search before */
                                      bool skip_brace_blocks) /**< skip blocks, surrounded with { and } braces */
{
  JERRY_ASSERT (token_to_find != TOK_EOF);

  while (lit_utf8_iterator_pos_cmp (tok.loc, end_loc) < 0)
  {
    if (skip_brace_blocks)
    {
      if (token_is (TOK_OPEN_BRACE))
      {
        jsp_skip_braces (ctx_p, TOK_OPEN_BRACE);

        JERRY_ASSERT (token_is (TOK_CLOSE_BRACE));
        skip_token (ctx_p);

        if (lit_utf8_iterator_pos_cmp (tok.loc, end_loc) >= 0)
        {
          seek_token (ctx_p, end_loc);

          return false;
        }
      }
      else if (token_is (TOK_CLOSE_BRACE))
      {
        EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Unmatched } brace");
      }
    }

    if (token_is (token_to_find))
    {
      return true;
    }
    else
    {
      JERRY_ASSERT (!token_is (TOK_EOF));
    }

    skip_token (ctx_p);
  }

  JERRY_ASSERT (lit_utf8_iterator_pos_cmp (tok.loc, end_loc) == 0);
  return false;
} /* jsp_find_next_token_before_the_locus */

/* property_name
  : Identifier
  | Keyword
  | StringLiteral
  | NumericLiteral
  ;
*/
static jsp_operand_t
parse_property_name (void)
{
  jsp_token_type_t tt = lexer_get_token_type (tok);

  if (is_keyword (tt))
  {
    const char *s = lexer_token_type_to_string (lexer_get_token_type (tok));
    lit_literal_t lit = lit_find_or_create_literal_from_utf8_string ((const lit_utf8_byte_t *) s,
                                                                 (lit_utf8_size_t)strlen (s));
    return jsp_make_string_lit_operand (rcs_cpointer_compress (lit));
  }
  else
  {
    switch (tt)
    {
      case TOK_NAME:
      case TOK_STRING:
      {
        return jsp_make_string_lit_operand (token_data_as_lit_cp ());
      }
      case TOK_NUMBER:
      case TOK_SMALL_INT:
      {
        ecma_number_t num;

        if (lexer_get_token_type (tok) == TOK_NUMBER)
        {
          lit_literal_t num_lit = lit_get_literal_by_cp (token_data_as_lit_cp ());
          JERRY_ASSERT (RCS_RECORD_IS_NUMBER (num_lit));
          num = lit_number_literal_get_number (num_lit);
        }
        else
        {
          num = ((ecma_number_t) token_data ());
        }

        lit_utf8_byte_t buff[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
        lit_utf8_size_t sz = ecma_number_to_utf8_string (num, buff, sizeof (buff));
        JERRY_ASSERT (sz <= ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER);

        lit_literal_t str_lit = lit_find_or_create_literal_from_utf8_string (buff, sz);
        return jsp_make_string_lit_operand (rcs_cpointer_compress (str_lit));
      }
      case TOK_NULL:
      case TOK_BOOL:
      {
        lit_magic_string_id_t id = (token_is (TOK_NULL)
                                    ? LIT_MAGIC_STRING_NULL
                                    : (tok.uid ? LIT_MAGIC_STRING_TRUE : LIT_MAGIC_STRING_FALSE));
        lit_literal_t lit = lit_find_or_create_literal_from_utf8_string (lit_get_magic_string_utf8 (id),
                                                                     lit_get_magic_string_size (id));
        return jsp_make_string_lit_operand (rcs_cpointer_compress (lit));
      }
      default:
      {
        EMIT_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX,
                         "Wrong property name type: %s",
                         lexer_token_type_to_string (lexer_get_token_type (tok)));
      }
    }
  }
}

/**
 * Check for "use strict" in directive prologue
 */
static void
jsp_parse_directive_prologue (jsp_ctx_t *ctx_p)
{
  const locus start_loc = tok.loc;

  /*
   * Check Directive Prologue for Use Strict directive (see ECMA-262 5.1 section 14.1)
   */
  while (token_is (TOK_STRING))
  {
    if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (token_data_as_lit_cp ()), "use strict")
        && lexer_is_no_escape_sequences_in_token_string (tok))
    {
      jsp_set_strict_mode (ctx_p);
      break;
    }

    skip_token (ctx_p);

    if (token_is (TOK_SEMICOLON))
    {
      skip_token (ctx_p);
    }
  }

  seek_token (ctx_p, start_loc);
} /* jsp_parse_directive_prologue */

static void
jsp_state_push (jsp_ctx_t *ctx_p,
                jsp_state_t state)
{
  jsp_state_t *new_top_state_p = (jsp_state_t *) jsp_mm_alloc (sizeof (jsp_state_t));

  *new_top_state_p = state;

  new_top_state_p->next_state_cp = MEM_CP_NULL;
  MEM_CP_SET_POINTER (new_top_state_p->prev_state_cp, ctx_p->state_stack_p);

  if (ctx_p->state_stack_p != NULL)
  {
    MEM_CP_SET_NON_NULL_POINTER (ctx_p->state_stack_p->next_state_cp, new_top_state_p);
  }

  ctx_p->state_stack_p = new_top_state_p;
} /* jsp_state_push */

static jsp_state_t *
jsp_get_prev_state (jsp_state_t *state_p)
{
  return MEM_CP_GET_POINTER (jsp_state_t, state_p->prev_state_cp);
} /* jsp_get_prev_state */

static jsp_state_t *
jsp_get_next_state (jsp_state_t *state_p)
{
  return MEM_CP_GET_NON_NULL_POINTER (jsp_state_t, state_p->next_state_cp);
} /* jsp_get_next_state */


static jsp_state_t *
jsp_state_top (jsp_ctx_t *ctx_p)
{
  JERRY_ASSERT (ctx_p->state_stack_p != NULL);

  return ctx_p->state_stack_p;
} /* jsp_state_top */

static bool
jsp_is_stack_contains_exactly_one_element (jsp_ctx_t *ctx_p)
{
  jsp_state_t *top_state_p = jsp_state_top (ctx_p);

  return (top_state_p != NULL && jsp_get_prev_state (top_state_p) == NULL);
} /* jsp_is_stack_contains_exactly_one_element */

static void
jsp_state_pop (jsp_ctx_t *ctx_p)
{
  jsp_state_t *top_state_p = ctx_p->state_stack_p;
  JERRY_ASSERT (top_state_p != NULL);

  jsp_state_t *prev_state_p = jsp_get_prev_state (top_state_p);

  if (prev_state_p != NULL)
  {
    prev_state_p->next_state_cp = MEM_CP_NULL;
  }

  jsp_mm_free (top_state_p);

  ctx_p->state_stack_p = prev_state_p;
} /* jsp_state_pop */

static void
jsp_stack_init (jsp_ctx_t *ctx_p)
{
  JERRY_ASSERT (ctx_p->state_stack_p == NULL);
} /* jsp_stack_init */

static void
jsp_stack_finalize (jsp_ctx_t *ctx_p)
{
  while (ctx_p->state_stack_p != NULL)
  {
    jsp_state_pop (ctx_p);
  }
} /* jsp_stack_finalize */

static void
jsp_push_new_expr_state (jsp_ctx_t *ctx_p,
                         jsp_state_expr_t expr_type,
                         jsp_state_expr_t req_state,
                         bool in_allowed)
{
  jsp_state_t new_state;

  new_state.state = expr_type;
  new_state.req_state = req_state;
  new_state.u.expression.operand = empty_operand ();
  new_state.u.expression.prop_name_operand = empty_operand ();
  new_state.u.expression.token_type = TOK_EMPTY;

  new_state.is_completed = false;
  new_state.is_list_in_process = false;
  new_state.is_fixed_ret_operand = false;
  new_state.is_value_based_reference = false;
  new_state.is_complex_production = false;
  new_state.var_decl = false;
  new_state.is_var_decl_no_in = false;
  new_state.is_get_value_dumped_for_main_operand = false;
  new_state.is_get_value_dumped_for_prop_operand = false;
  new_state.is_need_retval = true;
  new_state.prev_state_cp = MEM_CP_NULL;
  new_state.next_state_cp = MEM_CP_NULL;
  new_state.was_default = false;
  new_state.is_default_branch = false;
  new_state.is_simply_jumpable_border = false;
  new_state.is_dump_eval_ret_store = false;
  new_state.is_stmt_list_control_flow_exit_stmt_occured = false;

  new_state.is_no_in_mode = !in_allowed;

  jsp_state_push (ctx_p, new_state);
} /* jsp_push_new_expr_state */

static void
jsp_start_statement_parse (jsp_ctx_t *ctx_p,
                           jsp_state_expr_t stat)
{
  jsp_state_t new_state;

  new_state.state = stat;
  new_state.req_state = JSP_STATE_STAT_STATEMENT;

  new_state.u.statement.breaks_rewrite_chain = MAX_OPCODES;

  new_state.is_completed = false;
  new_state.is_list_in_process = false;
  new_state.is_no_in_mode = false;
  new_state.is_fixed_ret_operand = false;
  new_state.is_complex_production = false;
  new_state.var_decl = false;
  new_state.was_default = false;
  new_state.is_default_branch = false;
  new_state.is_simply_jumpable_border = false;
  new_state.is_stmt_list_control_flow_exit_stmt_occured = false;
  new_state.prev_state_cp = MEM_CP_NULL;
  new_state.next_state_cp = MEM_CP_NULL;
  new_state.is_value_based_reference = false;
  new_state.is_get_value_dumped_for_main_operand = false;
  new_state.is_get_value_dumped_for_prop_operand = false;
  new_state.is_need_retval = false;
  new_state.is_var_decl_no_in = false;
  new_state.is_dump_eval_ret_store = false;

  jsp_state_push (ctx_p, new_state);
} /* jsp_start_statement_parse */

static void
jsp_start_parse_source_element_list (jsp_ctx_t *ctx_p,
                                     mem_cpointer_t parent_scope_or_bc_header_cp,
                                     uint16_t parent_scope_processed_scopes_counter,
                                     scope_type_t scope_type)
{
  const scope_type_t parent_scope_type = jsp_get_scope_type (ctx_p);

  jsp_start_statement_parse (ctx_p, JSP_STATE_SOURCE_ELEMENTS);

  jsp_set_scope_type (ctx_p, scope_type);

  jsp_state_t *source_elements_state_p = jsp_state_top (ctx_p);
  source_elements_state_p->req_state = JSP_STATE_SOURCE_ELEMENTS;

  if (jsp_is_dump_mode (ctx_p))
  {
    source_elements_state_p->u.source_elements.u.parent_bc_header_cp = parent_scope_or_bc_header_cp;
  }
  else
  {
    source_elements_state_p->u.source_elements.u.parent_scopes_tree_node_cp = parent_scope_or_bc_header_cp;
  }

  dumper_save_reg_alloc_ctx (ctx_p,
                             &source_elements_state_p->u.source_elements.saved_reg_next,
                             &source_elements_state_p->u.source_elements.saved_reg_max_for_temps);
  source_elements_state_p->u.source_elements.reg_var_decl_oc = dump_reg_var_decl_for_rewrite (ctx_p);
  source_elements_state_p->u.source_elements.parent_scope_type = parent_scope_type;

  source_elements_state_p->u.source_elements.parent_scope_child_scopes_counter = parent_scope_processed_scopes_counter;

  if (scope_type == SCOPE_TYPE_EVAL)
  {
    dump_variable_assignment (ctx_p,
                              jsp_make_reg_operand (VM_REG_SPECIAL_EVAL_RET),
                              jsp_make_simple_value_operand (ECMA_SIMPLE_VALUE_UNDEFINED));
  }
} /* jsp_start_parse_source_element_list */

static jsp_operand_t
jsp_start_parse_function_scope (jsp_ctx_t *ctx_p,
                                jsp_operand_t func_name,
                                bool is_function_expression,
                                size_t *out_formal_parameters_num_p)
{
  jsp_operand_t func = empty_operand ();

  uint16_t index;

  if (jsp_is_dump_mode (ctx_p))
  {
    index = jsp_get_and_inc_processed_child_scopes_counter (ctx_p);
  }
  else
  {
    index = 0u;
  }

  if (is_function_expression)
  {
    func = tmp_operand ();

    vm_idx_t idx1 = (vm_idx_t) jrt_extract_bit_field (index, 0, JERRY_BITSINBYTE);
    vm_idx_t idx2 = (vm_idx_t) jrt_extract_bit_field (index, JERRY_BITSINBYTE, JERRY_BITSINBYTE);

    dump_binary_op (ctx_p,
                    VM_OP_FUNC_EXPR_REF,
                    func,
                    jsp_make_idx_const_operand (idx1),
                    jsp_make_idx_const_operand (idx2));
  }

  mem_cpointer_t parent_scope_or_bc_header_cp;
  uint16_t parent_scope_processed_scopes_counter = jsp_get_processed_child_scopes_counter (ctx_p);

  scopes_tree func_scope_p;

  if (!jsp_is_dump_mode (ctx_p))
  {
    scopes_tree parent_scope_p = jsp_get_current_scopes_tree_node (ctx_p);
    MEM_CP_SET_NON_NULL_POINTER (parent_scope_or_bc_header_cp, parent_scope_p);

    scopes_tree_set_contains_functions (parent_scope_p);

    func_scope_p = scopes_tree_new_scope (parent_scope_p, SCOPE_TYPE_FUNCTION);

    jsp_set_current_scopes_tree_node (ctx_p, func_scope_p);

    scopes_tree_set_strict_mode (func_scope_p, scopes_tree_strict_mode (parent_scope_p));
  }
  else
  {
    bytecode_data_header_t *parent_bc_header_p = jsp_get_current_bytecode_header (ctx_p);
    MEM_CP_SET_NON_NULL_POINTER (parent_scope_or_bc_header_cp, parent_bc_header_p);

    func_scope_p = jsp_get_next_scopes_tree_node_to_dump (ctx_p);

    bc_dump_single_scope (func_scope_p);

    mem_cpointer_t *parent_declarations_p = MEM_CP_GET_POINTER (mem_cpointer_t,
                                                                parent_bc_header_p->declarations_cp);
    parent_declarations_p[index] = func_scope_p->bc_header_cp;

    bytecode_data_header_t *func_bc_header_p = MEM_CP_GET_NON_NULL_POINTER (bytecode_data_header_t,
                                                                            func_scope_p->bc_header_cp);

    jsp_set_current_bytecode_header (ctx_p, func_bc_header_p);

    scopes_tree_free_scope (func_scope_p);
  }

  /* parse formal parameters list */
  size_t formal_parameters_num = 0;

  current_token_must_be_check_and_skip_it (ctx_p, TOK_OPEN_PAREN);

  JERRY_ASSERT (jsp_is_empty_operand (func_name)
                || jsp_is_string_lit_operand (func_name));

  varg_list_type vlt = is_function_expression ? VARG_FUNC_EXPR : VARG_FUNC_DECL;

  vm_instr_counter_t varg_header_pos = dump_varg_header_for_rewrite (ctx_p, vlt, func, func_name);

  locus formal_parameters_list_loc = tok.loc;

  while (!token_is (TOK_CLOSE_PAREN))
  {
    current_token_must_be (TOK_NAME);
    jsp_operand_t formal_parameter_name = jsp_make_string_lit_operand (token_data_as_lit_cp ());
    skip_token (ctx_p);

    dump_varg (ctx_p, formal_parameter_name);

    formal_parameters_num++;

    if (token_is (TOK_COMMA))
    {
      skip_token (ctx_p);
    }
    else
    {
      current_token_must_be (TOK_CLOSE_PAREN);
    }
  }

  skip_token (ctx_p);

  rewrite_varg_header_set_args_count (ctx_p, formal_parameters_num, varg_header_pos);

  current_token_must_be_check_and_skip_it (ctx_p, TOK_OPEN_BRACE);

  jsp_parse_directive_prologue (ctx_p);

  if (!jsp_is_dump_mode (ctx_p))
  {
    jsp_early_error_check_for_eval_and_arguments_in_strict_mode (func_name, jsp_is_strict_mode (ctx_p), tok.loc);
  }

  if (jsp_is_strict_mode (ctx_p))
  {
    locus body_loc = tok.loc;

    seek_token (ctx_p, formal_parameters_list_loc);

    /* Check duplication of formal parameters names */
    while (!token_is (TOK_CLOSE_PAREN))
    {
      current_token_must_be (TOK_NAME);

      lit_literal_t current_parameter_name_lit = lit_get_literal_by_cp (token_data_as_lit_cp ());
      locus current_parameter_loc = tok.loc;

      skip_token (ctx_p);

      if (token_is (TOK_COMMA))
      {
        skip_token (ctx_p);
      }

      if (!jsp_is_dump_mode (ctx_p))
      {
        jsp_early_error_emit_error_on_eval_and_arguments (current_parameter_name_lit,
                                                          current_parameter_loc);
      }

      while (!token_is (TOK_CLOSE_PAREN))
      {
        current_token_must_be (TOK_NAME);

        if (lit_utf8_iterator_pos_cmp (tok.loc, current_parameter_loc) != 0)
        {
          lit_literal_t iter_parameter_name_lit = lit_get_literal_by_cp (token_data_as_lit_cp ());

          if (lit_literal_equal_type (current_parameter_name_lit, iter_parameter_name_lit))
          {
            PARSE_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX,
                              "Duplication of literal '%s' in FormalParameterList is not allowed in strict mode",
                              tok.loc, lit_literal_to_str_internal_buf (iter_parameter_name_lit));
          }
        }

        skip_token (ctx_p);

        if (token_is (TOK_COMMA))
        {
          skip_token (ctx_p);
        }
        else
        {
          current_token_must_be (TOK_CLOSE_PAREN);
        }
      }

      seek_token (ctx_p, current_parameter_loc);

      JERRY_ASSERT (lit_utf8_iterator_pos_cmp (tok.loc, current_parameter_loc) == 0);
      skip_token (ctx_p);

      if (token_is (TOK_COMMA))
      {
        skip_token (ctx_p);
      }
      else
      {
        current_token_must_be (TOK_CLOSE_PAREN);
      }
    }

    seek_token (ctx_p, body_loc);
  }

  if (out_formal_parameters_num_p != NULL)
  {
    *out_formal_parameters_num_p = formal_parameters_num;
  }

  jsp_start_parse_source_element_list (ctx_p,
                                       parent_scope_or_bc_header_cp,
                                       parent_scope_processed_scopes_counter,
                                       SCOPE_TYPE_FUNCTION);

  return func;
}

static void
jsp_finish_parse_function_scope (jsp_ctx_t *ctx_p)
{
  current_token_must_be_check_and_skip_it (ctx_p, TOK_CLOSE_BRACE);
}

static void
dump_get_value_for_state_if_const (jsp_ctx_t *ctx_p,
                                   jsp_state_t *state_p)
{
  // todo: pass to dumper
  (void) ctx_p;

  JERRY_ASSERT (state_p->state > JSP_STATE_EXPR__BEGIN
                && state_p->state < JSP_STATE_EXPR__END);
  JERRY_ASSERT (!state_p->is_value_based_reference);

  if (jsp_is_string_lit_operand (state_p->u.expression.operand)
      || jsp_is_number_lit_operand (state_p->u.expression.operand)
      || jsp_is_regexp_lit_operand (state_p->u.expression.operand)
      || jsp_is_smallint_operand (state_p->u.expression.operand)
      || jsp_is_simple_value_operand (state_p->u.expression.operand))
  {
    jsp_operand_t reg = tmp_operand ();

    dump_variable_assignment (ctx_p, reg, state_p->u.expression.operand);

    state_p->u.expression.operand = reg;
  }
}

static bool
dump_get_value_for_state_if_ref (jsp_ctx_t *ctx_p,
                                 jsp_state_t *state_p,
                                 bool is_dump_for_value_based_refs_only,
                                 bool is_check_only)
{
  // TODO: pass to dumper
  (void) ctx_p;

  jsp_operand_t obj = state_p->u.expression.operand;
  jsp_operand_t prop_name = state_p->u.expression.prop_name_operand;
  bool is_value_based_reference = state_p->is_value_based_reference;

  jsp_operand_t val = jsp_make_uninitialized_operand ();

  bool is_dump = false;

  if (state_p->state == JSP_STATE_EXPR_LEFTHANDSIDE)
  {
    if (is_value_based_reference)
    {
      if (!state_p->is_get_value_dumped_for_main_operand)
      {
        JERRY_ASSERT (!state_p->is_get_value_dumped_for_prop_operand);
        /* FIXME:
             if (jsp_is_identifier_operand (obj))
             {
             is_dump = true;

             if (!is_check_only)
             {
             jsp_operand_t general_reg = tmp_operand ();

             dump_variable_assignment (ctx_p, general_reg, obj);

             state_p->u.expression.operand = general_reg;
             state_p->is_get_value_dumped_for_main_operand = true;
             }
             }

             if (jsp_is_identifier_operand (prop_name))
             {
             is_dump = true;

             if (!is_check_only)
             {
             jsp_operand_t general_reg = tmp_operand ();

             dump_variable_assignment (ctx_p, general_reg, prop_name);

             state_p->u.expression.prop_name_operand = general_reg;
             state_p->is_get_value_dumped_for_prop_operand = true;
             }
             }
         */
      }
    }
  }
  else
  {
    if (is_value_based_reference)
    {
      is_dump = true;

      JERRY_ASSERT (!jsp_is_empty_operand (prop_name));

      if (!is_check_only)
      {
        jsp_operand_t reg = tmp_operand ();

        dump_prop_getter (ctx_p, reg, obj, prop_name);

        val = reg;

        state_p->is_get_value_dumped_for_main_operand = true;
        state_p->is_get_value_dumped_for_prop_operand = true;
      }
    }
    else if (!is_dump_for_value_based_refs_only
             && (jsp_is_identifier_operand (obj) || jsp_is_this_operand (obj)))
    {
      if (!state_p->is_get_value_dumped_for_main_operand)
      {
        is_dump = true;

        if (!is_check_only)
        {
          jsp_operand_t general_reg = tmp_operand ();

          dump_variable_assignment (ctx_p, general_reg, obj);

          val = general_reg;

          state_p->is_get_value_dumped_for_main_operand = true;
        }
      }
    }

    if (!is_check_only && is_dump)
    {
      JERRY_ASSERT (state_p->is_get_value_dumped_for_main_operand);
      JERRY_ASSERT (!state_p->is_value_based_reference || state_p->is_get_value_dumped_for_prop_operand);

      state_p->u.expression.operand = val;
      state_p->u.expression.prop_name_operand = empty_operand ();
      state_p->is_value_based_reference = false;
    }
  }

  return is_dump;
}

static void
dump_get_value_for_prev_states (jsp_ctx_t *ctx_p,
                                jsp_state_t *state_p)
{
  jsp_state_t *dump_state_border_for_p = jsp_get_prev_state (state_p);
  jsp_state_t *first_state_to_dump_for_p = state_p;

  while (dump_state_border_for_p != NULL)
  {
    if (dump_state_border_for_p->state > JSP_STATE_EXPR__BEGIN
        && dump_state_border_for_p->req_state < JSP_STATE_EXPR__END
        && (!dump_state_border_for_p->is_get_value_dumped_for_main_operand
            || !dump_state_border_for_p->is_get_value_dumped_for_prop_operand))
    {
      first_state_to_dump_for_p = dump_state_border_for_p;
      dump_state_border_for_p = jsp_get_prev_state (dump_state_border_for_p);
    }
    else
    {
      break;
    }
  }

  JERRY_ASSERT (first_state_to_dump_for_p != NULL);

  jsp_state_t *state_iter_p = first_state_to_dump_for_p;

  while (state_iter_p != state_p)
  {
    dump_get_value_for_state_if_ref (ctx_p, state_iter_p, false, false);

    state_iter_p = jsp_get_next_state (state_iter_p);
  }
}

static void
dump_get_value_if_ref (jsp_ctx_t *ctx_p,
                       jsp_state_t *state_p,
                       bool is_dump_for_value_based_refs_only)
{
  if (dump_get_value_for_state_if_ref (ctx_p, state_p, is_dump_for_value_based_refs_only, true))
  {
    dump_get_value_for_prev_states (ctx_p, state_p);

    dump_get_value_for_state_if_ref (ctx_p, state_p, is_dump_for_value_based_refs_only, false);
  }
}

static vm_instr_counter_t
jsp_start_call_dump (jsp_ctx_t *ctx_p,
                     jsp_state_t *state_p,
                     vm_idx_t *out_state_p)
{
  bool is_value_based_reference = state_p->is_value_based_reference;

  opcode_call_flags_t call_flags = OPCODE_CALL_FLAGS__EMPTY;

  jsp_operand_t obj, val;

  if (is_value_based_reference)
  {
    obj = state_p->u.expression.operand;
    jsp_operand_t prop_name = state_p->u.expression.prop_name_operand;

    if (jsp_is_identifier_operand (obj)
        && !state_p->is_get_value_dumped_for_main_operand)
    {
      jsp_operand_t reg = tmp_operand ();

      dump_variable_assignment (ctx_p, reg, obj);

      obj = reg;

      state_p->is_get_value_dumped_for_main_operand = true;
    }

    val = tmp_operand ();
    dump_prop_getter (ctx_p, val, obj, prop_name);

    state_p->is_get_value_dumped_for_prop_operand = true;

    call_flags = (opcode_call_flags_t) (call_flags | OPCODE_CALL_FLAGS_HAVE_THIS_ARG);

    /*
     * Presence of explicit 'this' argument implies that this is not Direct call to Eval
     *
     * See also:
     *          ECMA-262 v5, 15.2.2.1
     */
  }
  else
  {
    dump_get_value_for_state_if_const (ctx_p, state_p);

    obj = state_p->u.expression.operand;

    if (dumper_is_eval_literal (obj))
    {
      call_flags = (opcode_call_flags_t) (call_flags | OPCODE_CALL_FLAGS_DIRECT_CALL_TO_EVAL_FORM);
    }

    /*
     * Note:
     *      If function is called through Identifier, then the obj should be an Identifier reference,
     *      not register variable.
     *      Otherwise, if function is called immediately, without reference (for example, through anonymous
     *      function expression), the obj should be a register variable.
     *
     * See also:
     *          vm_helper_call_get_call_flags_and_this_arg
     */
    val = obj;

    state_p->is_get_value_dumped_for_main_operand = true;
  }


  vm_idx_t reg_alloc_saved_state = dumper_save_reg_alloc_counter (ctx_p);

  jsp_operand_t ret = tmp_operand ();
  vm_instr_counter_t varg_header_pos = dump_varg_header_for_rewrite (ctx_p, VARG_CALL_EXPR, ret, val);

  *out_state_p = dumper_save_reg_alloc_counter (ctx_p);

  dumper_restore_reg_alloc_counter (ctx_p, reg_alloc_saved_state);

  if (call_flags != OPCODE_CALL_FLAGS__EMPTY)
  {
    if (call_flags & OPCODE_CALL_FLAGS_HAVE_THIS_ARG)
    {
      dump_call_additional_info (ctx_p, call_flags, obj);
    }
    else
    {
      dump_call_additional_info (ctx_p, call_flags, empty_operand ());
    }
  }

  state_p->u.expression.operand = ret;
  state_p->u.expression.prop_name_operand = empty_operand ();
  state_p->is_value_based_reference = false;

  return varg_header_pos;
} /* jsp_start_call_dump */

static void
jsp_finish_call_dump (jsp_ctx_t *ctx_p,
                      uint32_t args_num,
                      vm_instr_counter_t header_pos,
                      vm_idx_t reg_alloc_saved_state)
{
  (void) ctx_p;

  dumper_restore_reg_alloc_counter (ctx_p, reg_alloc_saved_state);

  rewrite_varg_header_set_args_count (ctx_p, args_num, header_pos);
} /* jsp_finish_call_dump */

static vm_instr_counter_t __attr_unused___
jsp_start_construct_dump (jsp_ctx_t *ctx_p,
                          jsp_state_t *state_p,
                          vm_idx_t *out_state_p)
{
  dump_get_value_if_ref (ctx_p, state_p, true);
  dump_get_value_for_state_if_const (ctx_p, state_p);

  state_p->is_get_value_dumped_for_main_operand = true;

  vm_idx_t reg_alloc_saved_state = dumper_save_reg_alloc_counter (ctx_p);

  jsp_operand_t ret = tmp_operand ();
  vm_instr_counter_t pos = dump_varg_header_for_rewrite (ctx_p,
                                                         VARG_CONSTRUCT_EXPR,
                                                         ret,
                                                         state_p->u.expression.operand);
  state_p->u.expression.operand = ret;

  *out_state_p = dumper_save_reg_alloc_counter (ctx_p);

  dumper_restore_reg_alloc_counter (ctx_p, reg_alloc_saved_state);

  return pos;
} /* jsp_start_construct_dump */

static void
jsp_finish_construct_dump (jsp_ctx_t *ctx_p,
                           uint32_t args_num,
                           vm_instr_counter_t header_pos,
                           vm_idx_t reg_alloc_saved_state)
{
  // TODO: pass to dumper
  (void) ctx_p;

  dumper_restore_reg_alloc_counter (ctx_p, reg_alloc_saved_state);

  rewrite_varg_header_set_args_count (ctx_p, args_num, header_pos);
} /* jsp_finish_construct_dump */

static void
jsp_add_nested_jump_to_rewrite_chain (jsp_ctx_t *ctx_p,
                                      vm_instr_counter_t *in_out_rewrite_chain_p)
{
  // TODO: pass to dumper
  (void) ctx_p;

  *in_out_rewrite_chain_p = dump_simple_or_nested_jump_for_rewrite (ctx_p, true, false, false,
                                                                    empty_operand (),
                                                                    *in_out_rewrite_chain_p);
}

static void
jsp_add_simple_jump_to_rewrite_chain (jsp_ctx_t *ctx_p,
                                      vm_instr_counter_t *in_out_rewrite_chain_p)
{
  // TODO: pass to dumper
  (void) ctx_p;

  *in_out_rewrite_chain_p = dump_simple_or_nested_jump_for_rewrite (ctx_p, false, false, false,
                                                                    empty_operand (),
                                                                    *in_out_rewrite_chain_p);
}

static void
jsp_add_conditional_jump_to_rewrite_chain (jsp_ctx_t *ctx_p,
                                           vm_instr_counter_t *in_out_rewrite_chain_p,
                                           bool is_inverted_condition,
                                           jsp_operand_t condition)
{
  // TODO: pass to dumper
  (void) ctx_p;

  *in_out_rewrite_chain_p = dump_simple_or_nested_jump_for_rewrite (ctx_p, false, true, is_inverted_condition,
                                                                    condition,
                                                                    *in_out_rewrite_chain_p);
}

static uint32_t
jsp_rewrite_jumps_chain (jsp_ctx_t *ctx_p,
                         vm_instr_counter_t *rewrite_chain_p,
                         vm_instr_counter_t target_oc)
{
  // TODO: pass to dumper
  (void) ctx_p;

  uint32_t count = 0;

  while (*rewrite_chain_p != MAX_OPCODES)
  {
    count++;

    *rewrite_chain_p = rewrite_simple_or_nested_jump_and_get_next (ctx_p,
                                                                   *rewrite_chain_p,
                                                                   target_oc);
  }

  return count;
} /* jsp_rewrite_jumps_chain */

static bool
jsp_is_assignment_expression_end (jsp_state_t *current_state_p)
{
  jsp_token_type_t tt = lexer_get_token_type (tok);

  JERRY_ASSERT (current_state_p->state == JSP_STATE_EXPR_UNARY
                || current_state_p->state == JSP_STATE_EXPR_MULTIPLICATIVE
                || current_state_p->state == JSP_STATE_EXPR_ADDITIVE
                || current_state_p->state == JSP_STATE_EXPR_SHIFT
                || current_state_p->state == JSP_STATE_EXPR_RELATIONAL
                || current_state_p->state == JSP_STATE_EXPR_EQUALITY
                || current_state_p->state == JSP_STATE_EXPR_BITWISE_AND
                || current_state_p->state == JSP_STATE_EXPR_BITWISE_XOR
                || current_state_p->state == JSP_STATE_EXPR_BITWISE_OR);

  if ((tt >= TOKEN_TYPE__MULTIPLICATIVE_BEGIN
       && tt <= TOKEN_TYPE__MULTIPLICATIVE_END)
      || (tt >= TOKEN_TYPE__ADDITIVE_BEGIN
          && tt <= TOKEN_TYPE__ADDITIVE_END)
      || (tt >= TOKEN_TYPE__SHIFT_BEGIN
          && tt <= TOKEN_TYPE__SHIFT_END)
      || (tt >= TOKEN_TYPE__RELATIONAL_BEGIN
          && tt <= TOKEN_TYPE__RELATIONAL_END)
      || (tt >= TOKEN_TYPE__EQUALITY_BEGIN
          && tt <= TOKEN_TYPE__EQUALITY_END)
      || (tt == TOK_AND)
      || (tt == TOK_XOR)
      || (tt == TOK_OR)
      || (tt == TOK_DOUBLE_AND)
      || (tt == TOK_DOUBLE_OR)
      || (tt == TOK_QUERY))
  {
    return false;
  }

  return true;
}

static bool
jsp_dump_unary_op (jsp_ctx_t *ctx_p,
                   jsp_state_t *state_p,
                   jsp_state_t *substate_p)
{
  vm_op_t opcode;

  bool is_combined_with_assignment = false;

  jsp_state_t *prev_state_p = jsp_get_prev_state (state_p);
  JERRY_ASSERT (prev_state_p != NULL);

  bool is_try_combine_with_assignment = (prev_state_p->state == JSP_STATE_EXPR_LEFTHANDSIDE
                                         && prev_state_p->u.expression.token_type == TOK_EQ
                                         && !prev_state_p->is_value_based_reference
                                         && !prev_state_p->is_need_retval
                                         && jsp_is_assignment_expression_end (state_p));

  jsp_token_type_t tt = state_p->u.expression.token_type;
  state_p->u.expression.token_type = TOK_EMPTY;

  bool is_dump_simple_assignment = false;
  bool is_dump_undefined_or_boolean_true = true; /* the value is not valid until is_dump_simple_assignment is set */

  switch (tt)
  {
    case TOK_DOUBLE_PLUS:
    case TOK_DOUBLE_MINUS:
    {
      if (!substate_p->is_value_based_reference)
      {
        if (!jsp_is_dump_mode (ctx_p))
        {
          if (jsp_is_identifier_operand (substate_p->u.expression.operand))
          {
            jsp_early_error_check_for_eval_and_arguments_in_strict_mode (substate_p->u.expression.operand,
                                                                         jsp_is_strict_mode (ctx_p),
                                                                         tok.loc);
          }
          else
          {
            PARSE_ERROR (JSP_EARLY_ERROR_REFERENCE, "Invalid left-hand-side expression", tok.loc);
          }
        }
      }

      opcode = (tt == TOK_DOUBLE_PLUS ? VM_OP_PRE_INCR : VM_OP_PRE_DECR);
      break;
    }
    case TOK_PLUS:
    {
      dump_get_value_if_ref (ctx_p, substate_p, true);

      opcode = VM_OP_UNARY_PLUS;
      break;
    }
    case TOK_MINUS:
    {
      dump_get_value_if_ref (ctx_p, substate_p, true);

      opcode = VM_OP_UNARY_MINUS;
      break;
    }
    case TOK_COMPL:
    {
      dump_get_value_if_ref (ctx_p, substate_p, true);

      opcode = VM_OP_B_NOT;
      break;
    }
    case TOK_NOT:
    {
      dump_get_value_if_ref (ctx_p, substate_p, true);

      opcode = VM_OP_LOGICAL_NOT;
      break;
    }
    case TOK_KW_DELETE:
    {
      if (substate_p->is_value_based_reference)
      {
        opcode = VM_OP_DELETE_PROP;
      }
      else if (jsp_is_identifier_operand (substate_p->u.expression.operand))
      {
        if (!jsp_is_dump_mode (ctx_p))
        {
          jsp_early_error_check_delete (jsp_is_strict_mode (ctx_p), tok.loc);
        }

        opcode = VM_OP_DELETE_VAR;
      }
      else
      {
        opcode = VM_OP__COUNT /* invalid opcode as it will not be used */;

        is_dump_simple_assignment = true;
        is_dump_undefined_or_boolean_true = false; /* dump boolean true value */
      }
      break;
    }
    case TOK_KW_VOID:
    {
      dump_get_value_if_ref (ctx_p, substate_p, false);

      opcode = VM_OP__COUNT /* invalid opcode as it will not be used */;

      is_dump_simple_assignment = true;
      is_dump_undefined_or_boolean_true = true; /* dump undefined value */
      break;
    }
    default:
    {
      dump_get_value_if_ref (ctx_p, substate_p, true);

      JERRY_ASSERT (tt == TOK_KW_TYPEOF);

      opcode = VM_OP_TYPEOF;
      break;
    }
  }

  jsp_operand_t dst = tmp_operand ();

  if (!substate_p->is_value_based_reference)
  {
    dump_get_value_for_state_if_const (ctx_p, substate_p);
  }

  if (dump_get_value_for_state_if_ref (ctx_p, substate_p, false, true))
  {
    dump_get_value_for_prev_states (ctx_p, substate_p);
  }

  if (is_try_combine_with_assignment
      && (is_dump_simple_assignment
          || !substate_p->is_value_based_reference
          || opcode == VM_OP_DELETE_PROP))
  {
    dst = prev_state_p->u.expression.operand;

    is_combined_with_assignment = true;
  }

  if (is_dump_simple_assignment)
  {
    if (is_dump_undefined_or_boolean_true)
    {
      dump_variable_assignment (ctx_p, dst, jsp_make_simple_value_operand (ECMA_SIMPLE_VALUE_UNDEFINED));
    }
    else
    {
      dump_variable_assignment (ctx_p, dst, jsp_make_simple_value_operand (ECMA_SIMPLE_VALUE_TRUE));
    }
  }
  else
  {
    if (substate_p->is_value_based_reference)
    {
      if (opcode == VM_OP_DELETE_PROP)
      {
        dump_binary_op (ctx_p,
                        VM_OP_DELETE_PROP,
                        dst,
                        substate_p->u.expression.operand,
                        substate_p->u.expression.prop_name_operand);
      }
      else
      {
        JERRY_ASSERT (opcode == VM_OP_PRE_INCR || opcode == VM_OP_PRE_DECR);

        jsp_operand_t reg = tmp_operand ();

        dump_prop_getter (ctx_p, reg, substate_p->u.expression.operand, substate_p->u.expression.prop_name_operand);

        dump_unary_op (ctx_p, opcode, reg, reg);

        dump_prop_setter (ctx_p, substate_p->u.expression.operand, substate_p->u.expression.prop_name_operand, reg);

        dst = reg;
      }
    }
    else
    {
      JERRY_ASSERT (!substate_p->is_value_based_reference);

      dump_unary_op (ctx_p, opcode, dst, substate_p->u.expression.operand);
    }
  }

  JERRY_ASSERT (!state_p->is_value_based_reference);
  state_p->u.expression.operand = dst;

  return is_combined_with_assignment;
}

static bool
jsp_dump_binary_op (jsp_ctx_t *ctx_p,
                    jsp_state_t *state_p,
                    jsp_state_t *substate_p)
{
  vm_op_t opcode;

  bool is_combined_with_assignment = false;

  jsp_state_t *prev_state_p = jsp_get_prev_state (state_p);
  JERRY_ASSERT (prev_state_p != NULL);

  bool is_try_combine_with_assignment = (prev_state_p->state == JSP_STATE_EXPR_LEFTHANDSIDE
                                         && prev_state_p->u.expression.token_type == TOK_EQ
                                         && !prev_state_p->is_value_based_reference
                                         && !prev_state_p->is_need_retval
                                         && jsp_is_assignment_expression_end (state_p));

  jsp_token_type_t tt = state_p->u.expression.token_type;
  state_p->u.expression.token_type = TOK_EMPTY;

  switch (tt)
  {
    case TOK_MULT:
    {
      opcode = VM_OP_MULTIPLICATION;
      break;
    }
    case TOK_DIV:
    {
      opcode = VM_OP_DIVISION;
      break;
    }
    case TOK_MOD:
    {
      opcode = VM_OP_REMAINDER;
      break;
    }
    case TOK_PLUS:
    {
      opcode = VM_OP_ADDITION;
      break;
    }
    case TOK_MINUS:
    {
      opcode = VM_OP_SUBSTRACTION;
      break;
    }
    case TOK_LSHIFT:
    {
      opcode = VM_OP_B_SHIFT_LEFT;
      break;
    }
    case TOK_RSHIFT:
    {
      opcode = VM_OP_B_SHIFT_RIGHT;
      break;
    }
    case TOK_RSHIFT_EX:
    {
      opcode = VM_OP_B_SHIFT_URIGHT;
      break;
    }
    case TOK_LESS:
    {
      opcode = VM_OP_LESS_THAN;
      break;
    }
    case TOK_GREATER:
    {
      opcode = VM_OP_GREATER_THAN;
      break;
    }
    case TOK_LESS_EQ:
    {
      opcode = VM_OP_LESS_OR_EQUAL_THAN;
      break;
    }
    case TOK_GREATER_EQ:
    {
      opcode = VM_OP_GREATER_OR_EQUAL_THAN;
      break;
    }
    case TOK_KW_INSTANCEOF:
    {
      opcode = VM_OP_INSTANCEOF;
      break;
    }
    case TOK_KW_IN:
    {
      opcode = VM_OP_IN;
      break;
    }
    case TOK_DOUBLE_EQ:
    {
      opcode = VM_OP_EQUAL_VALUE;
      break;
    }
    case TOK_NOT_EQ:
    {
      opcode = VM_OP_NOT_EQUAL_VALUE;
      break;
    }
    case TOK_TRIPLE_EQ:
    {
      opcode = VM_OP_EQUAL_VALUE_TYPE;
      break;
    }
    case TOK_NOT_DOUBLE_EQ:
    {
      opcode = VM_OP_NOT_EQUAL_VALUE_TYPE;
      break;
    }
    case TOK_AND:
    {
      opcode = VM_OP_B_AND;
      break;
    }
    case TOK_XOR:
    {
      opcode = VM_OP_B_XOR;
      break;
    }
    default:
    {
      JERRY_ASSERT (tt == TOK_OR);

      opcode = VM_OP_B_OR;
      break;
    }
  }

  jsp_operand_t dst, op1, op2;

  if (!state_p->is_value_based_reference)
  {
    dump_get_value_for_state_if_const (ctx_p, state_p);
  }

  if (!substate_p->is_value_based_reference)
  {
    dump_get_value_for_state_if_const (ctx_p, substate_p);
  }

  if (!state_p->is_value_based_reference
      && !substate_p->is_value_based_reference)
  {
    if (dump_get_value_for_state_if_ref (ctx_p, state_p, false, true)
        || dump_get_value_for_state_if_ref (ctx_p, substate_p, false, true))
    {
      dump_get_value_for_prev_states (ctx_p, state_p);
    }

    op1 = state_p->u.expression.operand;
    op2 = substate_p->u.expression.operand;

    if (is_try_combine_with_assignment)
    {
      dst = prev_state_p->u.expression.operand;

      is_combined_with_assignment = true;
    }
    else if (jsp_is_register_operand (op1))
    {
      dst = op1;
    }
    else
    {
      dst = tmp_operand ();
    }
  }
  else
  {
    dump_get_value_if_ref (ctx_p, state_p, true);
    dump_get_value_if_ref (ctx_p, substate_p, true);

    if (is_try_combine_with_assignment)
    {
      dst = prev_state_p->u.expression.operand;

      is_combined_with_assignment = true;
    }
    else
    {
      dst = state_p->u.expression.operand;
    }

    op1 = state_p->u.expression.operand;
    op2 = substate_p->u.expression.operand;
  }

  JERRY_ASSERT (!state_p->is_value_based_reference);
  state_p->u.expression.operand = dst;

  dump_binary_op (ctx_p, opcode, dst, op1, op2);

  return is_combined_with_assignment;
}

static lit_cpointer_t
jsp_get_prop_name_after_dot (void)
{
  if (token_is (TOK_NAME))
  {
    return token_data_as_lit_cp ();
  }
  else if (is_keyword (lexer_get_token_type (tok)))
  {
    const char *s = lexer_token_type_to_string (lexer_get_token_type (tok));
    lit_literal_t lit = lit_find_or_create_literal_from_utf8_string ((lit_utf8_byte_t *) s,
                                                                 (lit_utf8_size_t) strlen (s));
    if (lit == NULL)
    {
      EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Expected identifier");
    }

    return rcs_cpointer_compress (lit);
  }
  else if (token_is (TOK_BOOL) || token_is (TOK_NULL))
  {
    lit_magic_string_id_t id = (token_is (TOK_NULL)
                                ? LIT_MAGIC_STRING_NULL
                                : (tok.uid ? LIT_MAGIC_STRING_TRUE : LIT_MAGIC_STRING_FALSE));
    lit_literal_t lit = lit_find_or_create_literal_from_utf8_string (lit_get_magic_string_utf8 (id),
                                                                 lit_get_magic_string_size (id));

    return rcs_cpointer_compress (lit);
  }
  else
  {
    EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Expected identifier");
  }
} /* jsp_get_prop_name_after_dot */

#ifdef CONFIG_PARSER_ENABLE_PARSE_TIME_BYTE_CODE_OPTIMIZER
/**
 * Try to perform move (allocation) of variables' values on registers
 *
 * Note:
 *      The optimization is only applied to functions
 *
 * @return number of instructions removed from function's header
 */
static void
jsp_try_move_vars_to_regs (jsp_ctx_t *ctx_p,
                           jsp_state_t *state_p)
{
  // TODO: pass to dumper
  (void) ctx_p;

  JERRY_ASSERT (state_p->state == JSP_STATE_SOURCE_ELEMENTS);

  bytecode_data_header_t *bc_header_p = jsp_get_current_bytecode_header (ctx_p);

  bool may_replace_vars_with_regs = bc_header_p->is_vars_and_args_to_regs_possible;

  if (may_replace_vars_with_regs)
  {
    /* no subscopes, as no function declarations / eval etc. in the scope */
    JERRY_ASSERT (bc_header_p->func_scopes_count == 0);

    vm_instr_counter_t instr_pos = 0u;

    const vm_instr_counter_t header_oc = instr_pos++;
    op_meta header_opm = dumper_get_op_meta (ctx_p, header_oc);
    JERRY_ASSERT (header_opm.op.op_idx == VM_OP_FUNC_EXPR_N || header_opm.op.op_idx == VM_OP_FUNC_DECL_N);

    uint32_t args_num;

    if (header_opm.op.op_idx == VM_OP_FUNC_EXPR_N)
    {
      args_num = header_opm.op.data.func_expr_n.arg_list;
    }
    else
    {
      args_num = header_opm.op.data.func_decl_n.arg_list;
    }

    dumper_start_move_of_vars_to_regs (ctx_p);

    mem_cpointer_t *declarations_p = MEM_CP_GET_POINTER (mem_cpointer_t, bc_header_p->declarations_cp);
    lit_cpointer_t *var_declarations_p = (lit_cpointer_t *) (declarations_p + bc_header_p->func_scopes_count);

    /* remove declarations of variables with names equal to an argument's name */
    uint32_t var_decl_index = 0;
    while (var_decl_index < bc_header_p->var_decls_count)
    {
      lit_cpointer_t var_name_lit_cp = var_declarations_p[var_decl_index];

      op_meta var_decl_opm;
      var_decl_opm.op.op_idx = VM_OP_VAR_DECL;
      var_decl_opm.lit_id[0] = var_name_lit_cp;
      var_decl_opm.lit_id[1] = NOT_A_LITERAL;
      var_decl_opm.lit_id[2] = NOT_A_LITERAL;

      bool is_removed = false;

      for (vm_instr_counter_t arg_index = instr_pos;
           arg_index < instr_pos + args_num;
           arg_index++)
      {
        op_meta meta_opm = dumper_get_op_meta (ctx_p, arg_index);
        JERRY_ASSERT (meta_opm.op.op_idx == VM_OP_META);

        if (meta_opm.lit_id[1].packed_value == var_name_lit_cp.packed_value)
        {
          var_declarations_p[var_decl_index] = NOT_A_LITERAL;

          is_removed = true;
          break;
        }
      }

      if (!is_removed)
      {
        if (dumper_try_replace_identifier_name_with_reg (ctx_p, bc_header_p, &var_decl_opm))
        {
          var_declarations_p[var_decl_index] = NOT_A_LITERAL;
        }
      }

      var_decl_index++;
    }

    if (dumper_start_move_of_args_to_regs (ctx_p, args_num))
    {
      bc_header_p->is_args_moved_to_regs = true;
      bc_header_p->is_no_lex_env = true;

      dumper_rewrite_op_meta (ctx_p, header_oc, header_opm);

      /*
       * Mark duplicated arguments names as empty,
       * leaving only last declaration for each duplicated
       * argument name
       */
      for (vm_instr_counter_t arg1_index = instr_pos;
           arg1_index < instr_pos + args_num;
           arg1_index++)
      {
        op_meta meta_opm1 = dumper_get_op_meta (ctx_p, arg1_index);
        JERRY_ASSERT (meta_opm1.op.op_idx == VM_OP_META);

        for (vm_instr_counter_t arg2_index = (vm_instr_counter_t) (arg1_index + 1u);
             arg2_index < instr_pos + args_num;
             arg2_index++)
        {
          op_meta meta_opm2 = dumper_get_op_meta (ctx_p, arg2_index);
          JERRY_ASSERT (meta_opm2.op.op_idx == VM_OP_META);

          if (meta_opm1.lit_id[1].packed_value == meta_opm2.lit_id[1].packed_value)
          {
            meta_opm1.op.data.meta.data_1 = VM_IDX_EMPTY;
            meta_opm1.lit_id[1] = NOT_A_LITERAL;

            dumper_rewrite_op_meta (ctx_p, arg1_index, meta_opm1);

            break;
          }
        }
      }

      for (vm_instr_counter_t arg_instr_pos = instr_pos;
           arg_instr_pos < instr_pos + args_num;
           arg_instr_pos++)
      {
        op_meta meta_opm = dumper_get_op_meta (ctx_p, arg_instr_pos);
        JERRY_ASSERT (meta_opm.op.op_idx == VM_OP_META);

        opcode_meta_type meta_type = (opcode_meta_type) meta_opm.op.data.meta.type;
        JERRY_ASSERT (meta_type == OPCODE_META_TYPE_VARG);
        if (meta_opm.op.data.meta.data_1 == VM_IDX_EMPTY)
        {
          JERRY_ASSERT (meta_opm.lit_id[1].packed_value == NOT_A_LITERAL.packed_value);

          dumper_alloc_reg_for_unused_arg (ctx_p);
        }
        else
        {
          /* the varg specifies argument name, and so should be a string literal */
          JERRY_ASSERT (meta_opm.lit_id[1].packed_value != NOT_A_LITERAL.packed_value);

          bool is_replaced = dumper_try_replace_identifier_name_with_reg (ctx_p, bc_header_p, &meta_opm);
          JERRY_ASSERT (is_replaced);
        }
      }
    }
  }
}
#endif /* CONFIG_PARSER_ENABLE_PARSE_TIME_BYTE_CODE_OPTIMIZER */

static void
parse_expression_inside_parens_begin (jsp_ctx_t *ctx_p)
{
  current_token_must_be_check_and_skip_it (ctx_p, TOK_OPEN_PAREN);
}

static void
parse_expression_inside_parens_end (jsp_ctx_t *ctx_p)
{
  current_token_must_be_check_and_skip_it (ctx_p, TOK_CLOSE_PAREN);
}

static jsp_state_t *
jsp_find_unnamed_label (jsp_ctx_t *ctx_p,
                        bool is_label_for_break,
                        bool *out_is_simply_jumpable_p)
{
  *out_is_simply_jumpable_p = true;

  jsp_state_t *state_p = jsp_state_top (ctx_p);
  while (state_p != NULL)
  {
    if (state_p->state == JSP_STATE_SOURCE_ELEMENTS)
    {
      break;
    }

    if (state_p->is_simply_jumpable_border)
    {
      *out_is_simply_jumpable_p = false;
    }

    bool is_iterational_stmt = (state_p->state == JSP_STATE_STAT_WHILE
                                || state_p->state == JSP_STATE_STAT_DO_WHILE
                                || state_p->state == JSP_STATE_STAT_FOR_IN
                                || state_p->state == JSP_STATE_STAT_FOR_IN_EXPR
                                || state_p->state == JSP_STATE_STAT_FOR_IN_FINISH
                                || state_p->state == JSP_STATE_STAT_FOR_INCREMENT);
    bool is_switch_stmt = (state_p->state == JSP_STATE_STAT_SWITCH_BRANCH_EXPR);

    if ((is_switch_stmt && is_label_for_break) || is_iterational_stmt)
    {
      return state_p;
    }

    state_p = jsp_get_prev_state (state_p);
  }

  return NULL;
}

static jsp_state_t *
jsp_find_named_label (jsp_ctx_t *ctx_p,
                      lit_cpointer_t name_cp,
                      bool *out_is_simply_jumpable_p)
{
  *out_is_simply_jumpable_p = true;

  jsp_state_t *state_p = jsp_state_top (ctx_p);
  jsp_state_t *last_non_named_label_state_p = NULL;

  while (state_p != NULL)
  {
    if (state_p->state == JSP_STATE_SOURCE_ELEMENTS)
    {
      break;
    }

    if (state_p->is_simply_jumpable_border)
    {
      *out_is_simply_jumpable_p = false;
    }

    if (state_p->state != JSP_STATE_STAT_NAMED_LABEL)
    {
      last_non_named_label_state_p = state_p;
    }
    else if (state_p->u.named_label.name_cp.packed_value == name_cp.packed_value)
    {
      return last_non_named_label_state_p;
    }

    state_p = jsp_get_prev_state (state_p);
  }

  return NULL;
}

static void
insert_semicolon (jsp_ctx_t *ctx_p)
{
  bool is_new_line_occured = lexer_is_preceded_by_newlines (tok);
  bool is_close_brace_or_eof = (token_is (TOK_CLOSE_BRACE) || token_is (TOK_EOF));

  if (!is_new_line_occured && !is_close_brace_or_eof)
  {
    if (token_is (TOK_SEMICOLON))
    {
      skip_token (ctx_p);
    }
    else if (!token_is (TOK_EOF))
    {
      EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Expected either ';' or newline token");
    }
  }
}


#define JSP_COMPLETE_STATEMENT_PARSE() \
do \
{ \
  JERRY_ASSERT (substate_p == NULL); \
  state_p->state = JSP_STATE_STAT_STATEMENT; \
  JERRY_ASSERT (!state_p->is_completed); \
  dumper_new_statement (ctx_p); \
} \
while (0)

#define JSP_PUSH_STATE_AND_STATEMENT_PARSE(s) \
do \
{ \
  state_p->state = (s); \
  jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_EMPTY); \
  dumper_new_statement (ctx_p); \
} \
while (0)

#define JSP_FINISH_SUBEXPR() \
  JERRY_ASSERT (substate_p == jsp_state_top (ctx_p)); \
  substate_p = NULL; \
  jsp_state_pop (ctx_p); \
  JERRY_ASSERT (state_p == jsp_state_top (ctx_p));

#define JSP_ASSIGNMENT_EXPR_COMBINE() \
  jsp_state_pop (ctx_p); \
  state_p = jsp_state_top (ctx_p); \
  JERRY_ASSERT (state_p->state == JSP_STATE_EXPR_LEFTHANDSIDE); \
  JERRY_ASSERT (!state_p->is_need_retval); \
  state_p->state = JSP_STATE_EXPR_ASSIGNMENT; \
  state_p->u.expression.operand = empty_operand (); \
  state_p->u.expression.token_type = TOK_EMPTY; \
  state_p->is_completed = true;

/**
 * Parse source element list
 */
static void __attr_noinline___
jsp_parse_source_element_list (jsp_ctx_t *ctx_p,
                               scope_type_t scope_type)
{
  jsp_start_parse_source_element_list (ctx_p, MEM_CP_NULL, 0, scope_type);

  while (true)
  {
    bool is_source_elements_list_end = false;

    bool is_stmt_list_end = false, is_stmt_list_control_flow_exit_stmt_occured = false;

    bool is_subexpr_end = false;

    jsp_state_t *state_p = jsp_state_top (ctx_p), *substate_p = NULL;

    if (state_p->state == state_p->req_state && state_p->is_completed)
    {
      if (jsp_is_stack_contains_exactly_one_element (ctx_p))
      {
        JERRY_ASSERT (state_p->state == JSP_STATE_SOURCE_ELEMENTS);

        jsp_state_pop (ctx_p);

        break;
      }
      else
      {
        is_subexpr_end = (state_p->state > JSP_STATE_EXPR__BEGIN && state_p->state < JSP_STATE_EXPR__END);

        if (is_subexpr_end)
        {
          substate_p = state_p;
          state_p = jsp_get_prev_state (state_p);
        }
        else
        {
          if (state_p->req_state == JSP_STATE_SOURCE_ELEMENTS)
          {
            is_source_elements_list_end = true;
          }
          else if (state_p->req_state == JSP_STATE_STAT_STATEMENT_LIST)
          {
            is_stmt_list_end = true;

            is_stmt_list_control_flow_exit_stmt_occured = state_p->is_stmt_list_control_flow_exit_stmt_occured;
          }

          JERRY_ASSERT (state_p->is_completed);
          jsp_state_pop (ctx_p);

          state_p = jsp_state_top (ctx_p);
        }
      }
    }
    else
    {
      /*
       * Any expression production, if parse of the production is not stopped because required
       * production type was reached, eventually becomes Expression production.
       *
       * Because there are no any expression production higher than Expression,
       * its invalid to reach Expression production type if required production is lower
       * (i.e. is not Expression production type).
       */
      JERRY_ASSERT (!(state_p->state == JSP_STATE_EXPR_EXPRESSION && state_p->req_state != JSP_STATE_EXPR_EXPRESSION));
    }

    const bool in_allowed = !state_p->is_no_in_mode;

    if (state_p->state == JSP_STATE_SOURCE_ELEMENTS)
    {
      scope_type_t scope_type = jsp_get_scope_type (ctx_p);

      jsp_token_type_t end_token_type = (scope_type == SCOPE_TYPE_FUNCTION) ? TOK_CLOSE_BRACE : TOK_EOF;

      if (token_is (end_token_type))
      {
#ifdef CONFIG_PARSER_ENABLE_PARSE_TIME_BYTE_CODE_OPTIMIZER
        if (jsp_is_dump_mode (ctx_p))
        {
          jsp_try_move_vars_to_regs (ctx_p, state_p);
        }
        else
        {
          scopes_tree scope_p = jsp_get_current_scopes_tree_node (ctx_p);

          JERRY_ASSERT (!scope_p->is_vars_and_args_to_regs_possible);

          /*
           * We don't try to perform replacement of local variables with registers for global code, eval code.
           *
           * For global and eval code the replacement can be connected with side effects,
           * that currently can only be figured out in runtime. For example, a variable
           * can be redefined as accessor property of the Global object.
           */
          bool is_vars_and_args_to_regs_possible = (jsp_get_scope_type (ctx_p) == SCOPE_TYPE_FUNCTION
                                                    && !scope_p->ref_eval /* 'eval' can reference variables in a way,
                                                                           * that can't be figured out through static
                                                                           * analysis */
                                                    && !scope_p->ref_arguments /* 'arguments' variable, if declared,
                                                                                * should not be moved to a register,
                                                                                * as it is currently declared in
                                                                                * function's lexical environment
                                                                                * (generally, the problem is the same,
                                                                                *   as with function's arguments) */
                                                    && !scope_p->contains_with /* 'with' create new lexical environment
                                                                                *  and so can change way identifier
                                                                                *  is evaluated */
                                                    && !scope_p->contains_try /* same for 'catch' */
                                                    && !scope_p->contains_delete /* 'delete' handles variable's names,
                                                                                  * not values */
                                                    && !scope_p->contains_functions); /* nested functions
                                                                                       * can reference variables
                                                                                       * of current function */

          scope_p->is_vars_and_args_to_regs_possible = is_vars_and_args_to_regs_possible;
        }
#endif /* CONFIG_PARSER_ENABLE_PARSE_TIME_BYTE_CODE_OPTIMIZER */

        rewrite_reg_var_decl (ctx_p, state_p->u.source_elements.reg_var_decl_oc);

        if (scope_type == SCOPE_TYPE_FUNCTION)
        {
          dump_ret (ctx_p);

          if (jsp_is_dump_mode (ctx_p))
          {
            bytecode_data_header_t *bc_header_p = jsp_get_current_bytecode_header (ctx_p);

            if (parser_show_instrs)
            {
              bc_print_instrs (bc_header_p);
            }

            mem_cpointer_t parent_bc_header_cp = state_p->u.source_elements.u.parent_bc_header_cp;
            bytecode_data_header_t *parent_bc_header_p = MEM_CP_GET_NON_NULL_POINTER (bytecode_data_header_t,
                                                                                      parent_bc_header_cp);

            jsp_set_current_bytecode_header (ctx_p, parent_bc_header_p);
          }
          else
          {
            scopes_tree parent_scope_p = MEM_CP_GET_POINTER (scopes_tree_int,
                                                             state_p->u.source_elements.u.parent_scopes_tree_node_cp);

            jsp_set_current_scopes_tree_node (ctx_p, parent_scope_p);
          }
        }
        else if (scope_type == SCOPE_TYPE_GLOBAL)
        {
          dump_ret (ctx_p);
        }
        else
        {
          JERRY_ASSERT (scope_type == SCOPE_TYPE_EVAL);

          dump_retval (ctx_p, jsp_make_reg_operand (VM_REG_SPECIAL_EVAL_RET));
        }

        jsp_set_scope_type (ctx_p, state_p->u.source_elements.parent_scope_type);
        jsp_set_processed_child_scopes_counter (ctx_p,
                                                state_p->u.source_elements.parent_scope_child_scopes_counter);

        dumper_restore_reg_alloc_ctx (ctx_p,
                                      state_p->u.source_elements.saved_reg_next,
                                      state_p->u.source_elements.saved_reg_max_for_temps,
                                      true);

        state_p->is_completed = true;
      }
      else
      {
        JSP_PUSH_STATE_AND_STATEMENT_PARSE (JSP_STATE_SOURCE_ELEMENTS);
      }
    }
    else if (state_p->state == JSP_STATE_EXPR_EMPTY)
    {
      rescan_regexp_token (ctx_p);

      /* no subexpressions can occur, as expression parse is just started */
      JERRY_ASSERT (!is_subexpr_end);
      JERRY_ASSERT (!state_p->is_completed);

      jsp_token_type_t tt = lexer_get_token_type (tok);
      if ((tt >= TOKEN_TYPE__UNARY_BEGIN
           && tt <= TOKEN_TYPE__UNARY_END)
          || tt == TOK_KW_DELETE
          || tt == TOK_KW_VOID
          || tt == TOK_KW_TYPEOF)
      {
        /* UnaryExpression */
        state_p->state = JSP_STATE_EXPR_UNARY;
        state_p->u.expression.token_type = tt;

        if (!jsp_is_dump_mode (ctx_p))
        {
          if (tt == TOK_KW_DELETE)
          {
            scopes_tree_set_contains_delete (jsp_get_current_scopes_tree_node (ctx_p));
          }
        }

        jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_UNARY, in_allowed);
      }
      else if (token_is (TOK_KW_FUNCTION))
      {
        /* FunctionExpression */
        state_p->state = JSP_STATE_EXPR_FUNCTION;
      }
      else if (token_is (TOK_OPEN_SQUARE))
      {
        dump_get_value_for_prev_states (ctx_p, state_p);

        /* ArrayLiteral */
        vm_idx_t reg_alloc_saved_state = dumper_save_reg_alloc_counter (ctx_p);

        state_p->u.expression.operand = tmp_operand ();
        vm_instr_counter_t varg_header_pos = dump_varg_header_for_rewrite (ctx_p,
                                                                           VARG_ARRAY_DECL,
                                                                           state_p->u.expression.operand,
                                                                           empty_operand ());

        vm_idx_t reg_alloc_saved_state1 = dumper_save_reg_alloc_counter (ctx_p);

        dumper_restore_reg_alloc_counter (ctx_p, reg_alloc_saved_state);

        state_p->state = JSP_STATE_EXPR_ARRAY_LITERAL;
        state_p->is_list_in_process = true;
        state_p->u.expression.u.varg_sequence.list_length = 0;
        state_p->u.expression.u.varg_sequence.reg_alloc_saved_state1 = reg_alloc_saved_state1;
        state_p->u.expression.u.varg_sequence.header_pos = varg_header_pos;
      }
      else if (token_is (TOK_OPEN_BRACE))
      {
        dump_get_value_for_prev_states (ctx_p, state_p);

        /* ObjectLiteral */
        vm_idx_t reg_alloc_saved_state = dumper_save_reg_alloc_counter (ctx_p);

        state_p->u.expression.operand = tmp_operand ();
        vm_instr_counter_t varg_header_pos = dump_varg_header_for_rewrite (ctx_p,
                                                                           VARG_OBJ_DECL,
                                                                           state_p->u.expression.operand,
                                                                           empty_operand ());

        vm_idx_t reg_alloc_saved_state1 = dumper_save_reg_alloc_counter (ctx_p);

        dumper_restore_reg_alloc_counter (ctx_p, reg_alloc_saved_state);

        if (!jsp_is_dump_mode (ctx_p))
        {
          jsp_early_error_start_checking_of_prop_names ();
        }

        state_p->state = JSP_STATE_EXPR_OBJECT_LITERAL;
        state_p->is_list_in_process = true;
        state_p->u.expression.u.varg_sequence.list_length = 0;
        state_p->u.expression.u.varg_sequence.reg_alloc_saved_state1 = reg_alloc_saved_state1;
        state_p->u.expression.u.varg_sequence.header_pos = varg_header_pos;
      }
      else
      {
        /* MemberExpression (PrimaryExpression is immediately promoted to MemberExpression) */
        state_p->state = JSP_STATE_EXPR_MEMBER;

        switch (lexer_get_token_type (tok))
        {
          case TOK_OPEN_PAREN:
          {
            state_p->u.expression.token_type = TOK_OPEN_PAREN;

            jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, true);

            break;
          }
          case TOK_KW_THIS:
          {
            state_p->u.expression.operand = jsp_make_this_operand ();
            break;
          }
          case TOK_KW_NEW:
          {
            state_p->state = JSP_STATE_EXPR_MEMBER;
            state_p->u.expression.token_type = TOK_KW_NEW;

            jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_MEMBER, true);
            break;
          }
          case TOK_NAME:
          {
            if (!jsp_is_dump_mode (ctx_p))
            {
              if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (token_data_as_lit_cp ()), "arguments"))
              {
                scopes_tree_set_arguments_used (jsp_get_current_scopes_tree_node (ctx_p));
              }

              if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (token_data_as_lit_cp ()), "eval"))
              {
                scopes_tree_set_eval_used (jsp_get_current_scopes_tree_node (ctx_p));
              }
            }

            state_p->u.expression.operand = jsp_make_identifier_operand (token_data_as_lit_cp ());

            break;
          }
          case TOK_REGEXP:
          {
            state_p->u.expression.operand = jsp_make_regexp_lit_operand (token_data_as_lit_cp ());
            break;
          }
          case TOK_NULL:
          {
            state_p->u.expression.operand = jsp_make_simple_value_operand (ECMA_SIMPLE_VALUE_NULL);
            break;
          }
          case TOK_BOOL:
          {
            ecma_simple_value_t simple_value = (bool) token_data () ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE;
            state_p->u.expression.operand = jsp_make_simple_value_operand (simple_value);
            break;
          }
          case TOK_SMALL_INT:
          {
            state_p->u.expression.operand = jsp_make_smallint_operand ((uint8_t) token_data ());
            break;
          }
          case TOK_NUMBER:
          {
            state_p->u.expression.operand = jsp_make_number_lit_operand (token_data_as_lit_cp ());
            break;
          }
          case TOK_STRING:
          {
            state_p->u.expression.operand = jsp_make_string_lit_operand (token_data_as_lit_cp ());
            break;
          }
          default:
          {
            EMIT_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX,
                             "Unknown token %s",
                             lexer_token_type_to_string (lexer_get_token_type (tok)));
          }
        }
      }

      skip_token (ctx_p);
    }
    else if (state_p->state == JSP_STATE_EXPR_FUNCTION)
    {
      JERRY_ASSERT (!state_p->is_completed);

      if (is_source_elements_list_end)
      {
        jsp_finish_parse_function_scope (ctx_p);

        state_p->state = JSP_STATE_EXPR_MEMBER;
      }
      else
      {
        jsp_operand_t name = empty_operand ();

        if (token_is (TOK_NAME))
        {
          name = jsp_make_string_lit_operand (token_data_as_lit_cp ());
          skip_token (ctx_p);
        }

        state_p->u.expression.operand = jsp_start_parse_function_scope (ctx_p, name, true, NULL);
      }
    }
    else if (state_p->state == JSP_STATE_EXPR_DATA_PROP_DECL)
    {
      JERRY_ASSERT (!state_p->is_completed);

      if (is_subexpr_end)
      {
        JERRY_ASSERT (substate_p->state == JSP_STATE_EXPR_ASSIGNMENT);

        dump_get_value_if_ref (ctx_p, substate_p, true);
        dump_get_value_for_state_if_const (ctx_p, substate_p);

        jsp_operand_t prop_name = state_p->u.expression.operand;
        jsp_operand_t value = substate_p->u.expression.operand;

        JERRY_ASSERT (jsp_is_string_lit_operand (prop_name));

        dump_prop_name_and_value (ctx_p, prop_name, value);

        if (!jsp_is_dump_mode (ctx_p))
        {
          jsp_early_error_add_prop_name (prop_name, PROP_DATA);
        }

        state_p->is_completed = true;

        JSP_FINISH_SUBEXPR ();
      }
      else
      {
        JERRY_ASSERT (jsp_is_empty_operand (state_p->u.expression.operand));
        state_p->u.expression.operand = parse_property_name ();
        skip_token (ctx_p);

        JERRY_ASSERT (token_is (TOK_COLON));
        skip_token (ctx_p);

        jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_ASSIGNMENT, true);
      }
    }
    else if (state_p->state == JSP_STATE_EXPR_ACCESSOR_PROP_DECL)
    {
      JERRY_ASSERT (!state_p->is_completed);

      if (is_source_elements_list_end)
      {
        jsp_finish_parse_function_scope (ctx_p);

        jsp_operand_t prop_name = state_p->u.expression.u.accessor_prop_decl.prop_name;
        jsp_operand_t func = state_p->u.expression.operand;
        bool is_setter = state_p->u.expression.u.accessor_prop_decl.is_setter;

        if (is_setter)
        {
          dump_prop_setter_decl (ctx_p, prop_name, func);
        }
        else
        {
          dump_prop_getter_decl (ctx_p, prop_name, func);
        }

        state_p->is_completed = true;
      }
      else
      {
        bool is_setter;

        current_token_must_be (TOK_NAME);

        lit_cpointer_t lit_cp = token_data_as_lit_cp ();

        skip_token (ctx_p);

        if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (lit_cp), "get"))
        {
          is_setter = false;
        }
        else if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (lit_cp), "set"))
        {
          is_setter = true;
        }
        else
        {
          EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Invalid property declaration");
        }

        jsp_operand_t prop_name = parse_property_name ();
        skip_token (ctx_p);

        if (!jsp_is_dump_mode (ctx_p))
        {
          jsp_early_error_add_prop_name (prop_name, is_setter ? PROP_SET : PROP_GET);
        }

        size_t formal_parameters_num;
        const jsp_operand_t func = jsp_start_parse_function_scope (ctx_p,
                                                                   empty_operand (),
                                                                   true,
                                                                   &formal_parameters_num);

        size_t req_num_of_formal_parameters = is_setter ? 1 : 0;

        if (req_num_of_formal_parameters != formal_parameters_num)
        {
          EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Invalid number of formal parameters");
        }

        JERRY_ASSERT (jsp_is_empty_operand (state_p->u.expression.operand));
        state_p->u.expression.operand = func;

        state_p->u.expression.u.accessor_prop_decl.prop_name = prop_name;
        state_p->u.expression.u.accessor_prop_decl.is_setter = is_setter;
      }
    }
    else if (state_p->state == JSP_STATE_EXPR_OBJECT_LITERAL)
    {
      JERRY_ASSERT (!state_p->is_completed);
      JERRY_ASSERT (state_p->is_list_in_process);

      if (is_subexpr_end)
      {
        JERRY_ASSERT (substate_p->state == JSP_STATE_EXPR_DATA_PROP_DECL
                      || substate_p->state == JSP_STATE_EXPR_ACCESSOR_PROP_DECL);

        state_p->u.expression.u.varg_sequence.list_length++;

        dumper_restore_reg_alloc_counter (ctx_p, state_p->u.expression.u.varg_sequence.reg_alloc_saved_state2);

        if (token_is (TOK_COMMA))
        {
          skip_token (ctx_p);
        }
        else
        {
          current_token_must_be (TOK_CLOSE_BRACE);
        }

        JSP_FINISH_SUBEXPR ();
      }

      if (token_is (TOK_CLOSE_BRACE))
      {
        if (!jsp_is_dump_mode (ctx_p))
        {
          jsp_early_error_check_for_duplication_of_prop_names (jsp_is_strict_mode (ctx_p), tok.loc);
        }

        skip_token (ctx_p);

        uint32_t list_len = state_p->u.expression.u.varg_sequence.list_length;
        vm_instr_counter_t header_pos = state_p->u.expression.u.varg_sequence.header_pos;

        rewrite_varg_header_set_args_count (ctx_p, list_len, header_pos);

        dumper_restore_reg_alloc_counter (ctx_p, state_p->u.expression.u.varg_sequence.reg_alloc_saved_state1);

        state_p->state = JSP_STATE_EXPR_MEMBER;
        state_p->is_list_in_process = false;
      }
      else
      {
        state_p->u.expression.u.varg_sequence.reg_alloc_saved_state2 = dumper_save_reg_alloc_counter (ctx_p);

        locus start_pos = tok.loc;
        skip_token (ctx_p);

        jsp_state_expr_t expr_type;
        if (token_is (TOK_COLON))
        {
          expr_type = JSP_STATE_EXPR_DATA_PROP_DECL;
        }
        else
        {
          expr_type = JSP_STATE_EXPR_ACCESSOR_PROP_DECL;
        }

        seek_token (ctx_p, start_pos);

        jsp_push_new_expr_state (ctx_p, expr_type, expr_type, true);
      }
    }
    else if (state_p->state == JSP_STATE_EXPR_ARRAY_LITERAL)
    {
      JERRY_ASSERT (!state_p->is_completed);
      JERRY_ASSERT (state_p->is_list_in_process);

      if (is_subexpr_end)
      {
        dump_get_value_if_ref (ctx_p, substate_p, true);
        dump_get_value_for_state_if_const (ctx_p, substate_p);

        dump_varg (ctx_p, substate_p->u.expression.operand);

        JSP_FINISH_SUBEXPR ();

        state_p->u.expression.u.varg_sequence.list_length++;

        dumper_restore_reg_alloc_counter (ctx_p, state_p->u.expression.u.varg_sequence.reg_alloc_saved_state2);

        if (token_is (TOK_COMMA))
        {
          skip_token (ctx_p);
        }
        else
        {
          current_token_must_be (TOK_CLOSE_SQUARE);
        }
      }
      else
      {
        if (token_is (TOK_CLOSE_SQUARE))
        {
          skip_token (ctx_p);

          uint32_t list_len = state_p->u.expression.u.varg_sequence.list_length;
          vm_instr_counter_t header_pos = state_p->u.expression.u.varg_sequence.header_pos;

          rewrite_varg_header_set_args_count (ctx_p, list_len, header_pos);

          dumper_restore_reg_alloc_counter (ctx_p, state_p->u.expression.u.varg_sequence.reg_alloc_saved_state1);

          state_p->state = JSP_STATE_EXPR_MEMBER;
          state_p->is_list_in_process = false;
        }
        else if (token_is (TOK_COMMA))
        {
          while (token_is (TOK_COMMA))
          {
            skip_token (ctx_p);

            vm_idx_t reg_alloc_saved_state = dumper_save_reg_alloc_counter (ctx_p);

            jsp_operand_t reg = tmp_operand ();
            dump_variable_assignment (ctx_p,
                                      reg,
                                      jsp_make_simple_value_operand (ECMA_SIMPLE_VALUE_ARRAY_HOLE));
            dump_varg (ctx_p, reg);

            state_p->u.expression.u.varg_sequence.list_length++;

            dumper_restore_reg_alloc_counter (ctx_p, reg_alloc_saved_state);
          }
        }
        else
        {
          state_p->u.expression.u.varg_sequence.reg_alloc_saved_state2 = dumper_save_reg_alloc_counter (ctx_p);

          jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_ASSIGNMENT, true);
        }
      }
    }
    else if (state_p->state == JSP_STATE_EXPR_MEMBER)
    {
      if (state_p->is_completed)
      {
        if (token_is (TOK_OPEN_PAREN))
        {
          state_p->state = JSP_STATE_EXPR_CALL;
          state_p->is_completed = false;

          /* propagate to CallExpression */
          state_p->state = JSP_STATE_EXPR_CALL;
        }
        else
        {
          /* propagate to LeftHandSideExpression */
          state_p->state = JSP_STATE_EXPR_LEFTHANDSIDE;
          JERRY_ASSERT (state_p->is_completed);
        }
      }
      else
      {
        if (is_subexpr_end)
        {
          if (state_p->is_list_in_process)
          {
            JERRY_ASSERT (state_p->u.expression.token_type == TOK_KW_NEW);
            JERRY_ASSERT (substate_p->state == JSP_STATE_EXPR_ASSIGNMENT);

            dump_get_value_if_ref (ctx_p, substate_p, true);
            dump_get_value_for_state_if_const (ctx_p, substate_p);

            dump_varg (ctx_p, substate_p->u.expression.operand);

            JSP_FINISH_SUBEXPR ();

            dumper_restore_reg_alloc_counter (ctx_p, state_p->u.expression.u.varg_sequence.reg_alloc_saved_state2);

            state_p->u.expression.u.varg_sequence.list_length++;

            if (token_is (TOK_CLOSE_PAREN))
            {
              state_p->u.expression.token_type = TOK_EMPTY;
              state_p->is_list_in_process = false;

              uint32_t list_len = state_p->u.expression.u.varg_sequence.list_length;
              vm_instr_counter_t header_pos = state_p->u.expression.u.varg_sequence.header_pos;

              jsp_finish_construct_dump (ctx_p,
                                         list_len,
                                         header_pos,
                                         state_p->u.expression.u.varg_sequence.reg_alloc_saved_state1);
            }
            else
            {
              current_token_must_be (TOK_COMMA);

              jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_ASSIGNMENT, true);

              state_p->u.expression.u.varg_sequence.reg_alloc_saved_state2 = dumper_save_reg_alloc_counter (ctx_p);
            }

            skip_token (ctx_p);
          }
          else if (state_p->u.expression.token_type == TOK_OPEN_PAREN)
          {
            JERRY_ASSERT (jsp_is_empty_operand (state_p->u.expression.operand));

            state_p->u.expression.operand = substate_p->u.expression.operand;
            state_p->u.expression.prop_name_operand = substate_p->u.expression.prop_name_operand;
            state_p->is_value_based_reference = substate_p->is_value_based_reference;

            state_p->u.expression.token_type = TOK_EMPTY;

            JSP_FINISH_SUBEXPR ();

            current_token_must_be_check_and_skip_it (ctx_p, TOK_CLOSE_PAREN);
          }
          else if (state_p->u.expression.token_type == TOK_KW_NEW)
          {
            JERRY_ASSERT (substate_p->state == JSP_STATE_EXPR_MEMBER);
            JERRY_ASSERT (jsp_is_empty_operand (state_p->u.expression.operand));
            JERRY_ASSERT (!jsp_is_empty_operand (substate_p->u.expression.operand));

            state_p->u.expression.operand = substate_p->u.expression.operand;
            state_p->u.expression.prop_name_operand = substate_p->u.expression.prop_name_operand;
            state_p->is_value_based_reference = substate_p->is_value_based_reference;

            JSP_FINISH_SUBEXPR ();

            bool is_arg_list_implicit = true;
            bool is_arg_list_empty = true;

            if (token_is (TOK_OPEN_PAREN))
            {
              skip_token (ctx_p);

              is_arg_list_implicit = false;

              if (token_is (TOK_CLOSE_PAREN))
              {
                skip_token (ctx_p);
              }
              else
              {
                is_arg_list_empty = false;
              }
            }

            if (!is_arg_list_implicit && !is_arg_list_empty)
            {
              dump_get_value_for_prev_states (ctx_p, state_p);

              vm_idx_t reg_alloc_saved_state1;
              vm_instr_counter_t header_pos = jsp_start_construct_dump (ctx_p, state_p, &reg_alloc_saved_state1);

              state_p->is_list_in_process = true;
              state_p->u.expression.u.varg_sequence.list_length = 0;
              state_p->u.expression.u.varg_sequence.header_pos = header_pos;
              state_p->u.expression.u.varg_sequence.reg_alloc_saved_state1 = reg_alloc_saved_state1;
              state_p->u.expression.u.varg_sequence.reg_alloc_saved_state2 = dumper_save_reg_alloc_counter (ctx_p);

              jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_ASSIGNMENT, true);
            }
            else
            {
              vm_idx_t reg_alloc_saved_state1;
              vm_instr_counter_t header_pos = jsp_start_construct_dump (ctx_p, state_p, &reg_alloc_saved_state1);

              state_p->u.expression.token_type = TOK_EMPTY;

              if (is_arg_list_implicit)
              {
                state_p->state = JSP_STATE_EXPR_MEMBER;
                state_p->is_completed = true;
              }

              jsp_finish_construct_dump (ctx_p, 0, header_pos, reg_alloc_saved_state1);
              state_p->u.expression.prop_name_operand = empty_operand ();
              state_p->is_value_based_reference = false;
            }
          }
          else
          {
            JERRY_ASSERT (state_p->u.expression.token_type == TOK_OPEN_SQUARE);
            state_p->u.expression.token_type = TOK_EMPTY;

            current_token_must_be_check_and_skip_it (ctx_p, TOK_CLOSE_SQUARE);

            dump_get_value_if_ref (ctx_p, substate_p, true);
            dump_get_value_for_state_if_const (ctx_p, substate_p);

            state_p->u.expression.prop_name_operand = substate_p->u.expression.operand;
            state_p->is_value_based_reference = true;

            JSP_FINISH_SUBEXPR ();
          }
        }
        else if (token_is (TOK_OPEN_SQUARE))
        {
          skip_token (ctx_p);

          state_p->u.expression.token_type = TOK_OPEN_SQUARE;

          dump_get_value_if_ref (ctx_p, state_p, true);
          dump_get_value_for_state_if_const (ctx_p, state_p);

          jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, true);
        }
        else if (token_is (TOK_DOT))
        {
          skip_token (ctx_p);

          lit_cpointer_t prop_name = jsp_get_prop_name_after_dot ();
          skip_token (ctx_p);

          dump_get_value_if_ref (ctx_p, state_p, true);
          dump_get_value_for_state_if_const (ctx_p, state_p);

          state_p->u.expression.prop_name_operand = tmp_operand ();
          dump_variable_assignment (ctx_p,
                                    state_p->u.expression.prop_name_operand,
                                    jsp_make_string_lit_operand (prop_name));

          state_p->is_value_based_reference = true;
        }
        else
        {
          state_p->is_completed = true;
        }
      }
    }
    else if (state_p->state == JSP_STATE_EXPR_CALL)
    {
      JERRY_ASSERT (!state_p->is_completed);

      if (is_subexpr_end)
      {
        if (state_p->is_list_in_process)
        {
          JERRY_ASSERT (substate_p->state == JSP_STATE_EXPR_ASSIGNMENT);

          dump_get_value_if_ref (ctx_p, substate_p, true);
          dump_get_value_for_state_if_const (ctx_p, substate_p);

          dump_varg (ctx_p, substate_p->u.expression.operand);

          JSP_FINISH_SUBEXPR ();

          dumper_restore_reg_alloc_counter (ctx_p, state_p->u.expression.u.varg_sequence.reg_alloc_saved_state2);

          state_p->u.expression.u.varg_sequence.list_length++;

          if (token_is (TOK_CLOSE_PAREN))
          {
            state_p->is_list_in_process = false;

            uint32_t list_len = state_p->u.expression.u.varg_sequence.list_length;
            vm_instr_counter_t header_pos = state_p->u.expression.u.varg_sequence.header_pos;

            jsp_finish_call_dump (ctx_p,
                                  list_len,
                                  header_pos,
                                  state_p->u.expression.u.varg_sequence.reg_alloc_saved_state1);
            state_p->u.expression.prop_name_operand = empty_operand ();
            state_p->is_value_based_reference = false;
          }
          else
          {
            current_token_must_be (TOK_COMMA);

            jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_ASSIGNMENT, true);

            state_p->u.expression.u.varg_sequence.reg_alloc_saved_state2 = dumper_save_reg_alloc_counter (ctx_p);
          }
        }
        else
        {
          JERRY_ASSERT (state_p->u.expression.token_type == TOK_OPEN_SQUARE);
          state_p->u.expression.token_type = TOK_EMPTY;

          current_token_must_be (TOK_CLOSE_SQUARE);

          dump_get_value_if_ref (ctx_p, substate_p, true);
          dump_get_value_for_state_if_const (ctx_p, substate_p);

          state_p->u.expression.prop_name_operand = substate_p->u.expression.operand;
          state_p->is_value_based_reference = true;

          JSP_FINISH_SUBEXPR ();
        }

        skip_token (ctx_p);
      }
      else
      {
        if (token_is (TOK_OPEN_PAREN))
        {
          skip_token (ctx_p);

          dump_get_value_for_prev_states (ctx_p, state_p);

          vm_idx_t reg_alloc_saved_state1;

          vm_instr_counter_t header_pos = jsp_start_call_dump (ctx_p, state_p, &reg_alloc_saved_state1);

          if (token_is (TOK_CLOSE_PAREN))
          {
            skip_token (ctx_p);

            jsp_finish_call_dump (ctx_p, 0, header_pos, reg_alloc_saved_state1);
            state_p->u.expression.prop_name_operand = empty_operand ();
            state_p->is_value_based_reference = false;
          }
          else
          {
            state_p->is_list_in_process = true;
            state_p->u.expression.u.varg_sequence.list_length = 0;
            state_p->u.expression.u.varg_sequence.header_pos = header_pos;
            state_p->u.expression.u.varg_sequence.reg_alloc_saved_state1 = reg_alloc_saved_state1;
            state_p->u.expression.u.varg_sequence.reg_alloc_saved_state2 = dumper_save_reg_alloc_counter (ctx_p);

            jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_ASSIGNMENT, true);
          }
        }
        else if (token_is (TOK_OPEN_SQUARE))
        {
          skip_token (ctx_p);

          state_p->u.expression.token_type = TOK_OPEN_SQUARE;
          dump_get_value_if_ref (ctx_p, state_p, true);
          dump_get_value_for_state_if_const (ctx_p, state_p);

          jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, true);
        }
        else if (token_is (TOK_DOT))
        {
          skip_token (ctx_p);

          lit_cpointer_t prop_name = jsp_get_prop_name_after_dot ();
          skip_token (ctx_p);

          dump_get_value_if_ref (ctx_p, state_p, true);
          dump_get_value_for_state_if_const (ctx_p, state_p);

          state_p->u.expression.prop_name_operand = tmp_operand ();
          dump_variable_assignment (ctx_p,
                                    state_p->u.expression.prop_name_operand,
                                    jsp_make_string_lit_operand (prop_name));

          state_p->is_value_based_reference = true;
        }
        else
        {
          state_p->state = JSP_STATE_EXPR_LEFTHANDSIDE;
          state_p->is_completed = true;
        }
      }
    }
    else if (state_p->state == JSP_STATE_EXPR_LEFTHANDSIDE)
    {
      JERRY_ASSERT (state_p->is_completed);

      if (is_subexpr_end)
      {
        jsp_token_type_t tt = state_p->u.expression.token_type;
        JERRY_ASSERT (tt >= TOKEN_TYPE__ASSIGNMENTS_BEGIN && tt <= TOKEN_TYPE__ASSIGNMENTS_END);

        dump_get_value_for_prev_states (ctx_p, state_p);
        dump_get_value_if_ref (ctx_p, substate_p, true);

        if (tt == TOK_EQ)
        {
          if (state_p->is_need_retval)
          {
            dump_get_value_if_ref (ctx_p, substate_p, false);
            dump_get_value_for_state_if_const (ctx_p, substate_p);
          }

          if (state_p->is_value_based_reference)
          {
            dump_get_value_for_state_if_const (ctx_p, substate_p);

            dump_prop_setter (ctx_p,
                              state_p->u.expression.operand,
                              state_p->u.expression.prop_name_operand,
                              substate_p->u.expression.operand);

            state_p->u.expression.prop_name_operand = empty_operand ();
            state_p->is_value_based_reference = false;
          }
          else
          {
            dump_variable_assignment (ctx_p, state_p->u.expression.operand, substate_p->u.expression.operand);
          }

          state_p->u.expression.operand = substate_p->u.expression.operand;

          if (state_p->is_need_retval)
          {
            JERRY_ASSERT (jsp_is_register_operand (state_p->u.expression.operand));
          }
          else
          {
            state_p->u.expression.operand = empty_operand ();
          }
        }
        else
        {
          vm_op_t opcode;

          if (tt == TOK_MULT_EQ)
          {
            opcode = VM_OP_MULTIPLICATION;
          }
          else if (tt == TOK_DIV_EQ)
          {
            opcode = VM_OP_DIVISION;
          }
          else if (tt == TOK_MOD_EQ)
          {
            opcode = VM_OP_REMAINDER;
          }
          else if (tt == TOK_PLUS_EQ)
          {
            opcode = VM_OP_ADDITION;
          }
          else if (tt == TOK_MINUS_EQ)
          {
            opcode = VM_OP_SUBSTRACTION;
          }
          else if (tt == TOK_LSHIFT_EQ)
          {
            opcode = VM_OP_B_SHIFT_LEFT;
          }
          else if (tt == TOK_RSHIFT_EQ)
          {
            opcode = VM_OP_B_SHIFT_RIGHT;
          }
          else if (tt == TOK_RSHIFT_EX_EQ)
          {
            opcode = VM_OP_B_SHIFT_URIGHT;
          }
          else if (tt == TOK_AND_EQ)
          {
            opcode = VM_OP_B_AND;
          }
          else if (tt == TOK_XOR_EQ)
          {
            opcode = VM_OP_B_XOR;
          }
          else
          {
            JERRY_ASSERT (tt == TOK_OR_EQ);

            opcode = VM_OP_B_OR;
          }

          dump_get_value_for_state_if_const (ctx_p, substate_p);

          if (state_p->is_value_based_reference)
          {
            jsp_operand_t reg = tmp_operand ();

            dump_prop_getter (ctx_p, reg, state_p->u.expression.operand, state_p->u.expression.prop_name_operand);

            dump_binary_op (ctx_p, opcode, reg, reg, substate_p->u.expression.operand);

            dump_prop_setter (ctx_p,
                              state_p->u.expression.operand,
                              state_p->u.expression.prop_name_operand,
                              reg);

            state_p->u.expression.operand = reg;
            state_p->u.expression.prop_name_operand = empty_operand ();
            state_p->is_value_based_reference = false;
          }
          else if (state_p->is_need_retval)
          {
            jsp_operand_t reg = tmp_operand ();

            dump_binary_op (ctx_p, opcode, reg, state_p->u.expression.operand, substate_p->u.expression.operand);

            dump_variable_assignment (ctx_p, state_p->u.expression.operand, reg);

            state_p->u.expression.operand = reg;
          }
          else
          {
            dump_binary_op (ctx_p, opcode, state_p->u.expression.operand,
                            state_p->u.expression.operand,
                            substate_p->u.expression.operand);

            state_p->u.expression.operand = empty_operand ();
          }
        }

        state_p->state = JSP_STATE_EXPR_ASSIGNMENT;
        state_p->u.expression.token_type = TOK_EMPTY;
        state_p->is_completed = true;

        JSP_FINISH_SUBEXPR ();
      }
      else
      {
        JERRY_ASSERT (state_p->u.expression.token_type == TOK_EMPTY);

        if (token_is (TOK_DOUBLE_PLUS)
            && !lexer_is_preceded_by_newlines (tok))
        {
          const jsp_operand_t reg = tmp_operand ();

          if (state_p->is_value_based_reference)
          {
            const jsp_operand_t val = tmp_operand ();

            dump_prop_getter (ctx_p, val, state_p->u.expression.operand, state_p->u.expression.prop_name_operand);

            dump_unary_op (ctx_p, VM_OP_POST_INCR, reg, val);

            dump_prop_setter (ctx_p, state_p->u.expression.operand, state_p->u.expression.prop_name_operand, val);

            state_p->u.expression.operand = reg;
            state_p->u.expression.prop_name_operand = empty_operand ();
            state_p->is_value_based_reference = false;
          }
          else if (jsp_is_identifier_operand (state_p->u.expression.operand))
          {
            if (!jsp_is_dump_mode (ctx_p))
            {
              jsp_early_error_check_for_eval_and_arguments_in_strict_mode (state_p->u.expression.operand,
                                                                           jsp_is_strict_mode (ctx_p),
                                                                           tok.loc);
            }

            dump_unary_op (ctx_p, VM_OP_POST_INCR, reg, state_p->u.expression.operand);

            state_p->u.expression.operand = reg;
          }
          else
          {
            PARSE_ERROR (JSP_EARLY_ERROR_REFERENCE, "Invalid left-hand-side expression", tok.loc);
          }

          state_p->state = JSP_STATE_EXPR_UNARY;
          JERRY_ASSERT (state_p->is_completed);

          skip_token (ctx_p);
        }
        else if (token_is (TOK_DOUBLE_MINUS)
                 && !lexer_is_preceded_by_newlines (tok))
        {
          const jsp_operand_t reg = tmp_operand ();

          if (state_p->is_value_based_reference)
          {
            const jsp_operand_t val = tmp_operand ();

            dump_prop_getter (ctx_p, val, state_p->u.expression.operand, state_p->u.expression.prop_name_operand);

            dump_unary_op (ctx_p, VM_OP_POST_DECR, reg, val);

            dump_prop_setter (ctx_p, state_p->u.expression.operand, state_p->u.expression.prop_name_operand, val);

            state_p->u.expression.operand = reg;
            state_p->u.expression.prop_name_operand = empty_operand ();
            state_p->is_value_based_reference = false;
          }
          else if (jsp_is_identifier_operand (state_p->u.expression.operand))
          {
            if (!jsp_is_dump_mode (ctx_p))
            {
              jsp_early_error_check_for_eval_and_arguments_in_strict_mode (state_p->u.expression.operand,
                                                                           jsp_is_strict_mode (ctx_p),
                                                                           tok.loc);
            }

            dump_unary_op (ctx_p, VM_OP_POST_DECR, reg, state_p->u.expression.operand);

            state_p->u.expression.operand = reg;
          }
          else
          {
            PARSE_ERROR (JSP_EARLY_ERROR_REFERENCE, "Invalid left-hand-side expression", tok.loc);
          }

          state_p->state = JSP_STATE_EXPR_UNARY;
          JERRY_ASSERT (state_p->is_completed);

          skip_token (ctx_p);
        }
        else
        {
          jsp_token_type_t tt = lexer_get_token_type (tok);

          if (tt >= TOKEN_TYPE__ASSIGNMENTS_BEGIN && tt <= TOKEN_TYPE__ASSIGNMENTS_END)
          {
            if (!state_p->is_value_based_reference)
            {
              if (!jsp_is_dump_mode (ctx_p))
              {
                if (!jsp_is_identifier_operand (state_p->u.expression.operand))
                {
                  PARSE_ERROR (JSP_EARLY_ERROR_REFERENCE, "Invalid left-hand-side expression", tok.loc);
                }
                else
                {
                  jsp_early_error_check_for_eval_and_arguments_in_strict_mode (state_p->u.expression.operand,
                                                                               jsp_is_strict_mode (ctx_p),
                                                                               tok.loc);
                }
              }
            }

            /* skip the assignment operator */
            skip_token (ctx_p);
            state_p->u.expression.token_type = tt;

            jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_ASSIGNMENT, in_allowed);
          }
          else
          {
            state_p->state = JSP_STATE_EXPR_UNARY;
          }
        }
      }
    }
    else if (state_p->state > JSP_STATE_EXPR__SIMPLE_BEGIN
             && state_p->state < JSP_STATE_EXPR__SIMPLE_END)
    {
      JERRY_STATIC_ASSERT (JSP_STATE_EXPR_MULTIPLICATIVE == JSP_STATE_EXPR_UNARY + 1u);
      JERRY_STATIC_ASSERT (JSP_STATE_EXPR_ADDITIVE == JSP_STATE_EXPR_MULTIPLICATIVE + 1u);
      JERRY_STATIC_ASSERT (JSP_STATE_EXPR_SHIFT == JSP_STATE_EXPR_ADDITIVE + 1u);
      JERRY_STATIC_ASSERT (JSP_STATE_EXPR_RELATIONAL == JSP_STATE_EXPR_SHIFT + 1u);
      JERRY_STATIC_ASSERT (JSP_STATE_EXPR_EQUALITY == JSP_STATE_EXPR_RELATIONAL + 1u);
      JERRY_STATIC_ASSERT (JSP_STATE_EXPR_BITWISE_AND == JSP_STATE_EXPR_EQUALITY + 1u);
      JERRY_STATIC_ASSERT (JSP_STATE_EXPR_BITWISE_XOR == JSP_STATE_EXPR_BITWISE_AND + 1u);
      JERRY_STATIC_ASSERT (JSP_STATE_EXPR_BITWISE_OR == JSP_STATE_EXPR_BITWISE_XOR + 1u);

      if (state_p->is_completed)
      {
        if (state_p->state == JSP_STATE_EXPR_BITWISE_OR)
        {
          state_p->state = JSP_STATE_EXPR_LOGICAL_AND;

          state_p->u.expression.u.logical_and.rewrite_chain = MAX_OPCODES;
        }
        else
        {
          JERRY_ASSERT (state_p->state != state_p->req_state);
          JERRY_ASSERT (state_p->state == JSP_STATE_EXPR_UNARY);

          state_p->state = (jsp_state_expr_t) (state_p->state + 1u);
        }

        state_p->is_completed = false;
      }
      else if (is_subexpr_end)
      {
        bool is_combined_with_assignment;

        if (state_p->state == JSP_STATE_EXPR_UNARY)
        {
          is_combined_with_assignment = jsp_dump_unary_op (ctx_p, state_p, substate_p);
        }
        else
        {
          is_combined_with_assignment = jsp_dump_binary_op (ctx_p, state_p, substate_p);
        }

        JSP_FINISH_SUBEXPR ();

        if (is_combined_with_assignment)
        {
          JSP_ASSIGNMENT_EXPR_COMBINE ();
        }
      }
      else
      {
        JERRY_ASSERT (!state_p->is_completed);


        jsp_state_expr_t new_state, subexpr_type;

        bool is_simple = true;

        jsp_token_type_t tt = lexer_get_token_type (tok);

        if (tt >= TOKEN_TYPE__MULTIPLICATIVE_BEGIN && tt <= TOKEN_TYPE__MULTIPLICATIVE_END)
        {
          JERRY_ASSERT (state_p->state >= JSP_STATE_EXPR_UNARY && state_p->state <= JSP_STATE_EXPR_MULTIPLICATIVE);

          new_state = JSP_STATE_EXPR_MULTIPLICATIVE;
          subexpr_type = JSP_STATE_EXPR_UNARY;
        }
        else if (tt >= TOKEN_TYPE__ADDITIVE_BEGIN && tt <= TOKEN_TYPE__ADDITIVE_END)
        {
          JERRY_ASSERT (state_p->state >= JSP_STATE_EXPR_UNARY && state_p->state <= JSP_STATE_EXPR_ADDITIVE);

          new_state = JSP_STATE_EXPR_ADDITIVE;
          subexpr_type = JSP_STATE_EXPR_MULTIPLICATIVE;
        }
        else if (tt >= TOKEN_TYPE__SHIFT_BEGIN && tt <= TOKEN_TYPE__SHIFT_END)
        {
          JERRY_ASSERT (state_p->state >= JSP_STATE_EXPR_UNARY && state_p->state <= JSP_STATE_EXPR_SHIFT);

          new_state = JSP_STATE_EXPR_SHIFT;
          subexpr_type = JSP_STATE_EXPR_ADDITIVE;
        }
        else if ((tt >= TOKEN_TYPE__RELATIONAL_BEGIN && tt <= TOKEN_TYPE__RELATIONAL_END)
                 || tt == TOK_KW_INSTANCEOF
                 || (in_allowed && tt == TOK_KW_IN))
        {
          JERRY_ASSERT (state_p->state >= JSP_STATE_EXPR_UNARY && state_p->state <= JSP_STATE_EXPR_RELATIONAL);

          new_state = JSP_STATE_EXPR_RELATIONAL;
          subexpr_type = JSP_STATE_EXPR_SHIFT;
        }
        else if (tt >= TOKEN_TYPE__EQUALITY_BEGIN && tt <= TOKEN_TYPE__EQUALITY_END)
        {
          JERRY_ASSERT (state_p->state >= JSP_STATE_EXPR_UNARY && state_p->state <= JSP_STATE_EXPR_EQUALITY);

          new_state = JSP_STATE_EXPR_EQUALITY;
          subexpr_type = JSP_STATE_EXPR_RELATIONAL;
        }
        else if (tt == TOK_AND)
        {
          JERRY_ASSERT (state_p->state >= JSP_STATE_EXPR_UNARY && state_p->state <= JSP_STATE_EXPR_BITWISE_AND);

          new_state = JSP_STATE_EXPR_BITWISE_AND;
          subexpr_type = JSP_STATE_EXPR_EQUALITY;
        }
        else if (tt == TOK_XOR)
        {
          JERRY_ASSERT (state_p->state >= JSP_STATE_EXPR_UNARY && state_p->state <= JSP_STATE_EXPR_BITWISE_XOR);

          new_state = JSP_STATE_EXPR_BITWISE_XOR;
          subexpr_type = JSP_STATE_EXPR_BITWISE_AND;
        }
        else if (tt == TOK_OR)
        {
          JERRY_ASSERT (state_p->state >= JSP_STATE_EXPR_UNARY && state_p->state <= JSP_STATE_EXPR_BITWISE_OR);

          new_state = JSP_STATE_EXPR_BITWISE_OR;
          subexpr_type = JSP_STATE_EXPR_BITWISE_XOR;
        }
        else
        {
          is_simple = false;
        }

        if (!is_simple && state_p->req_state >= JSP_STATE_EXPR_LOGICAL_AND)
        {
          state_p->state = JSP_STATE_EXPR_LOGICAL_AND;
          state_p->u.expression.u.logical_and.rewrite_chain = MAX_OPCODES;
        }
        else
        {
          JERRY_ASSERT (is_simple || state_p->req_state < JSP_STATE_EXPR__SIMPLE_END);

          if (!is_simple || state_p->req_state < new_state)
          {
            state_p->state = state_p->req_state;

            state_p->is_completed = true;
          }
          else
          {
            skip_token (ctx_p);

            state_p->state = new_state;
            state_p->u.expression.token_type = tt;

            jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, subexpr_type, in_allowed);
          }
        }
      }
    }
    else if (state_p->state == JSP_STATE_EXPR_LOGICAL_AND)
    {
      if (state_p->is_completed)
      {
        /* propagate to LogicalOrExpression */
        state_p->state = JSP_STATE_EXPR_LOGICAL_OR;

        state_p->u.expression.u.logical_or.rewrite_chain = MAX_OPCODES;

        state_p->is_completed = false;
      }
      else
      {
        if (is_subexpr_end)
        {
          dump_get_value_if_ref (ctx_p, state_p, true);
          dump_get_value_if_ref (ctx_p, substate_p, true);

          JERRY_ASSERT (state_p->u.expression.token_type == TOK_DOUBLE_AND);

          JERRY_ASSERT (jsp_is_register_operand (state_p->u.expression.operand));

          dump_variable_assignment (ctx_p, state_p->u.expression.operand, substate_p->u.expression.operand);

          state_p->u.expression.token_type = TOK_EMPTY;

          JSP_FINISH_SUBEXPR ();
        }
        else
        {
          JERRY_ASSERT (state_p->u.expression.token_type == TOK_EMPTY);

          if (token_is (TOK_DOUBLE_AND))
          {
            /* ECMA-262 v5, 11.11 (complex LogicalAndExpression) */
            skip_token (ctx_p);

            /*
             * FIXME:
             *       Consider eliminating COMPLEX_PRODUCTION flag through implementing establishing a general operand
             *       management for expressions
             */
            if (!state_p->is_complex_production)
            {
              state_p->is_complex_production = true;

              state_p->u.expression.u.logical_and.rewrite_chain = MAX_OPCODES;

              JERRY_ASSERT (!state_p->is_fixed_ret_operand);

              jsp_operand_t ret = tmp_operand ();

              dump_get_value_if_ref (ctx_p, state_p, true);

              dump_variable_assignment (ctx_p, ret, state_p->u.expression.operand);

              state_p->is_fixed_ret_operand = true;
              state_p->u.expression.operand = ret;
            }

            JERRY_ASSERT (state_p->is_complex_production);

            jsp_add_conditional_jump_to_rewrite_chain (ctx_p, &state_p->u.expression.u.logical_and.rewrite_chain,
                                                       true, state_p->u.expression.operand);

            state_p->u.expression.token_type = TOK_DOUBLE_AND;

            jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_BITWISE_OR, in_allowed);
          }
          else
          {
            /* end of LogicalAndExpression */
            JERRY_ASSERT (state_p->u.expression.token_type == TOK_EMPTY);

            jsp_rewrite_jumps_chain (ctx_p, &state_p->u.expression.u.logical_and.rewrite_chain,
                                     dumper_get_current_instr_counter (ctx_p));

            state_p->is_complex_production = false;

            state_p->is_completed = true;
          }
        }
      }
    }
    else if (state_p->state == JSP_STATE_EXPR_LOGICAL_OR)
    {
      if (state_p->is_completed)
      {
        /* switching to ConditionalExpression */
        if (token_is (TOK_QUERY))
        {
          state_p->state = JSP_STATE_EXPR_CONDITION;
          state_p->is_completed = false;

          /* ECMA-262 v5, 11.12 */
          skip_token (ctx_p);

          dump_get_value_if_ref (ctx_p, state_p, true);
          dump_get_value_for_state_if_const (ctx_p, state_p);

          vm_instr_counter_t conditional_check_pos = dump_conditional_check_for_rewrite (ctx_p,
                                                                                         state_p->u.expression.operand);
          state_p->u.expression.u.conditional.conditional_check_pos = conditional_check_pos;

          state_p->u.expression.token_type = TOK_QUERY;

          if (!state_p->is_fixed_ret_operand)
          {
            state_p->is_fixed_ret_operand = true;
            state_p->u.expression.operand = tmp_operand ();
          }

          jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_ASSIGNMENT, true);
        }
        else
        {
          /* just propagate */
          state_p->state = JSP_STATE_EXPR_ASSIGNMENT;
          JERRY_ASSERT (state_p->is_completed);
        }
      }
      else
      {
        if (is_subexpr_end)
        {
          dump_get_value_if_ref (ctx_p, substate_p, true);

          JERRY_ASSERT (state_p->u.expression.token_type == TOK_DOUBLE_OR);

          JERRY_ASSERT (jsp_is_register_operand (state_p->u.expression.operand));
          dump_variable_assignment (ctx_p, state_p->u.expression.operand, substate_p->u.expression.operand);

          state_p->u.expression.token_type = TOK_EMPTY;

          JSP_FINISH_SUBEXPR ();
        }
        else
        {
          JERRY_ASSERT (state_p->u.expression.token_type == TOK_EMPTY);

          if (token_is (TOK_DOUBLE_OR))
          {
            /* ECMA-262 v5, 11.11 (complex LogicalOrExpression) */
            skip_token (ctx_p);

            /*
             * FIXME:
             *       Consider eliminating COMPLEX_PRODUCTION flag through implementing establishing a general operand
             *       management for expressions
             */
            if (!state_p->is_complex_production)
            {
              state_p->is_complex_production = true;

              state_p->u.expression.u.logical_or.rewrite_chain = MAX_OPCODES;

              jsp_operand_t ret;

              if (state_p->is_fixed_ret_operand)
              {
                JERRY_ASSERT (jsp_is_register_operand (state_p->u.expression.operand));

                ret = state_p->u.expression.operand;
              }
              else
              {
                ret = tmp_operand ();

                dump_get_value_if_ref (ctx_p, state_p, true);

                dump_variable_assignment (ctx_p, ret, state_p->u.expression.operand);

                state_p->is_fixed_ret_operand = true;

                state_p->u.expression.operand = ret;
              }
            }

            JERRY_ASSERT (state_p->is_complex_production);

            jsp_add_conditional_jump_to_rewrite_chain (ctx_p, &state_p->u.expression.u.logical_or.rewrite_chain,
                                                       false, state_p->u.expression.operand);

            state_p->u.expression.token_type = TOK_DOUBLE_OR;

            jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_LOGICAL_AND, in_allowed);
          }
          else
          {
            /* end of LogicalOrExpression */
            JERRY_ASSERT (state_p->u.expression.token_type == TOK_EMPTY);

            jsp_rewrite_jumps_chain (ctx_p, &state_p->u.expression.u.logical_or.rewrite_chain,
                                     dumper_get_current_instr_counter (ctx_p));

            state_p->is_complex_production = false;

            state_p->is_completed = true;
          }
        }
      }
    }
    else if (state_p->state == JSP_STATE_EXPR_ASSIGNMENT)
    {
      /* assignment production can't be continued at the point */
      JERRY_ASSERT (!is_subexpr_end);

      JERRY_ASSERT (state_p->is_completed);
      JERRY_ASSERT (state_p->u.expression.token_type == TOK_EMPTY);

      /* 'assignment expression' production can't be continued with an operator,
       *  so just propagating it to 'expression' production */
      state_p->is_completed = false;
      state_p->state = JSP_STATE_EXPR_EXPRESSION;
    }
    else if (state_p->state == JSP_STATE_EXPR_CONDITION)
    {
      JERRY_ASSERT (!state_p->is_completed);
      JERRY_ASSERT (is_subexpr_end);

      /* ECMA-262 v5, 11.12 */
      if (state_p->u.expression.token_type == TOK_QUERY)
      {
        current_token_must_be_check_and_skip_it (ctx_p, TOK_COLON);

        JERRY_ASSERT (state_p->is_fixed_ret_operand);
        JERRY_ASSERT (jsp_is_register_operand (state_p->u.expression.operand));
        JERRY_ASSERT (substate_p->state == JSP_STATE_EXPR_ASSIGNMENT);

        dump_get_value_if_ref (ctx_p, substate_p, true);

        dump_variable_assignment (ctx_p, state_p->u.expression.operand, substate_p->u.expression.operand);

        JSP_FINISH_SUBEXPR ();

        state_p->u.expression.u.conditional.jump_to_end_pos = dump_jump_to_end_for_rewrite (ctx_p);

        rewrite_conditional_check (ctx_p, state_p->u.expression.u.conditional.conditional_check_pos);

        state_p->u.expression.token_type = TOK_COLON;

        jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_ASSIGNMENT, in_allowed);
      }
      else
      {
        JERRY_ASSERT (state_p->u.expression.token_type == TOK_COLON);

        JERRY_ASSERT (state_p->is_fixed_ret_operand);
        JERRY_ASSERT (jsp_is_register_operand (state_p->u.expression.operand));
        JERRY_ASSERT (substate_p->state == JSP_STATE_EXPR_ASSIGNMENT);

        dump_get_value_if_ref (ctx_p, substate_p, true);

        dump_variable_assignment (ctx_p, state_p->u.expression.operand, substate_p->u.expression.operand);

        JSP_FINISH_SUBEXPR ();

        rewrite_jump_to_end (ctx_p, state_p->u.expression.u.conditional.jump_to_end_pos);

        state_p->u.expression.token_type = TOK_EMPTY;
        state_p->is_fixed_ret_operand = false;

        state_p->state = JSP_STATE_EXPR_ASSIGNMENT;
        state_p->is_completed = true;
      }
    }
    else if (state_p->state == JSP_STATE_EXPR_EXPRESSION)
    {
      /* ECMA-262 v5, 11.14 */

      if (is_subexpr_end)
      {
        JERRY_ASSERT (state_p->u.expression.token_type == TOK_COMMA);

        dump_get_value_if_ref (ctx_p, substate_p, false);

        if (state_p->is_need_retval)
        {
          /*
           * The operand should be already evaluated
           *
           * See also:
           *          11.14, step 2
           */
          JERRY_ASSERT (!state_p->is_value_based_reference
                        && !jsp_is_identifier_operand (state_p->u.expression.operand));

          state_p->u.expression.operand = substate_p->u.expression.operand;
        }

        JSP_FINISH_SUBEXPR ();
      }
      else
      {
        JERRY_ASSERT (!state_p->is_completed);

        if (token_is (TOK_COMMA))
        {
          skip_token (ctx_p);

          JERRY_ASSERT (!token_is (TOK_COMMA));

          state_p->u.expression.token_type = TOK_COMMA;

          /* ECMA-262 v5, 11.14, step 2 */
          dump_get_value_if_ref (ctx_p, state_p, false);

          jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_ASSIGNMENT, in_allowed);
        }
        else
        {
          if (state_p->is_need_retval)
          {
            if (!state_p->is_value_based_reference)
            {
              dump_get_value_for_state_if_const (ctx_p, state_p);
            }
          }

          state_p->is_completed = true;
        }
      }
    }
    else if (state_p->state == JSP_STATE_STAT_EMPTY)
    {
      dumper_new_statement (ctx_p);

      if (token_is (TOK_KW_IF)) /* IfStatement */
      {
        skip_token (ctx_p);

        parse_expression_inside_parens_begin (ctx_p);

        state_p->state = JSP_STATE_STAT_IF_BRANCH_START;

        jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, in_allowed);
      }
      else if (token_is (TOK_OPEN_BRACE))
      {
        skip_token (ctx_p);

        state_p->state = JSP_STATE_STAT_BLOCK;
        jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_STATEMENT_LIST);
        jsp_state_top (ctx_p)->req_state = JSP_STATE_STAT_STATEMENT_LIST;
      }
      else if (token_is (TOK_KW_VAR))
      {
        state_p->state = JSP_STATE_STAT_VAR_DECL;
        state_p->var_decl = true;
      }
      else if (token_is (TOK_KW_DO)
               || token_is (TOK_KW_WHILE)
               || token_is (TOK_KW_FOR))
      {
        state_p->u.statement.u.iterational.continues_rewrite_chain = MAX_OPCODES;
        state_p->u.statement.u.iterational.continue_tgt_oc = MAX_OPCODES;

        if (token_is (TOK_KW_DO))
        {
          vm_instr_counter_t next_iter_tgt_pos = dumper_set_next_iteration_target (ctx_p);
          state_p->u.statement.u.iterational.u.loop_do_while.next_iter_tgt_pos = next_iter_tgt_pos;
          skip_token (ctx_p);

          JSP_PUSH_STATE_AND_STATEMENT_PARSE (JSP_STATE_STAT_DO_WHILE);
        }
        else if (token_is (TOK_KW_WHILE))
        {
          skip_token (ctx_p);

          state_p->u.statement.u.iterational.u.loop_while.u.cond_expr_start_loc = tok.loc;
          jsp_skip_braces (ctx_p, TOK_OPEN_PAREN);

          state_p->u.statement.u.iterational.u.loop_while.jump_to_end_pos = dump_jump_to_end_for_rewrite (ctx_p);

          state_p->u.statement.u.iterational.u.loop_while.next_iter_tgt_pos = dumper_set_next_iteration_target (ctx_p);

          skip_token (ctx_p);

          JSP_PUSH_STATE_AND_STATEMENT_PARSE (JSP_STATE_STAT_WHILE);
        }
        else
        {
          current_token_must_be_check_and_skip_it (ctx_p, TOK_KW_FOR);

          current_token_must_be (TOK_OPEN_PAREN);

          locus for_open_paren_loc, for_body_statement_loc;

          for_open_paren_loc = tok.loc;

          jsp_skip_braces (ctx_p, TOK_OPEN_PAREN);
          skip_token (ctx_p);

          for_body_statement_loc = tok.loc;

          seek_token (ctx_p, for_open_paren_loc);

          bool is_plain_for = jsp_find_next_token_before_the_locus (ctx_p,
                                                                    TOK_SEMICOLON,
                                                                    for_body_statement_loc,
                                                                    true);
          seek_token (ctx_p, for_open_paren_loc);

          if (is_plain_for)
          {
            state_p->u.statement.u.iterational.u.loop_for.u1.body_loc = for_body_statement_loc;

            current_token_must_be_check_and_skip_it (ctx_p, TOK_OPEN_PAREN);

            // Initializer
            if (token_is (TOK_KW_VAR))
            {
              state_p->state = JSP_STATE_STAT_FOR_INIT_END;
              jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_VAR_DECL);
            }
            else
            {
              if (!token_is (TOK_SEMICOLON))
              {
                jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, false);

                jsp_state_top (ctx_p)->is_need_retval = false;
              }
              else
              {
                // Initializer is empty
              }
              state_p->state = JSP_STATE_STAT_FOR_INIT_END;
            }
          }
          else
          {
            current_token_must_be_check_and_skip_it (ctx_p, TOK_OPEN_PAREN);

            // Save Iterator location
            state_p->u.statement.u.iterational.u.loop_for_in.u.iterator_expr_loc = tok.loc;

            while (lit_utf8_iterator_pos_cmp (tok.loc, for_body_statement_loc) < 0)
            {
              if (jsp_find_next_token_before_the_locus (ctx_p,
                                                        TOK_KW_IN,
                                                        for_body_statement_loc,
                                                        true))
              {
                break;
              }
              else
              {
                EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Invalid for statement");
              }
            }

            JERRY_ASSERT (token_is (TOK_KW_IN));
            skip_token (ctx_p);

            // Collection
            jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, true);
            state_p->state = JSP_STATE_STAT_FOR_IN;
          }
        }
      }
      else if (token_is (TOK_KW_SWITCH))
      {
        skip_token (ctx_p);

        parse_expression_inside_parens_begin (ctx_p);
        jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, true);
        state_p->state = JSP_STATE_STAT_SWITCH;
      }
      else if (token_is (TOK_SEMICOLON))
      {
        skip_token (ctx_p);

        JSP_COMPLETE_STATEMENT_PARSE ();
      }
      else if (token_is (TOK_KW_CONTINUE)
               || token_is (TOK_KW_BREAK))
      {
        bool is_break = token_is (TOK_KW_BREAK);

        skip_token (ctx_p);

        jsp_state_t *prev_state_p = jsp_get_prev_state (state_p);

        if (prev_state_p->state == JSP_STATE_STAT_STATEMENT_LIST)
        {
          prev_state_p->is_stmt_list_control_flow_exit_stmt_occured = true;
        }

        jsp_state_t *labelled_stmt_p;
        bool is_simply_jumpable = true;

        if (!lexer_is_preceded_by_newlines (tok) && token_is (TOK_NAME))
        {
          /* break or continue on a label */
          labelled_stmt_p = jsp_find_named_label (ctx_p, token_data_as_lit_cp (), &is_simply_jumpable);

          if (labelled_stmt_p == NULL)
          {
            EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Label not found");
          }

          skip_token (ctx_p);
        }
        else
        {
          labelled_stmt_p = jsp_find_unnamed_label (ctx_p, is_break, &is_simply_jumpable);

          if (labelled_stmt_p == NULL)
          {
            if (is_break)
            {
              EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "No corresponding statement for the break");
            }
            else
            {
              EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "No corresponding statement for the continue");
            }
          }
        }

        insert_semicolon (ctx_p);

        JERRY_ASSERT (labelled_stmt_p != NULL);

        vm_instr_counter_t *rewrite_chain_p;
        if (is_break)
        {
          rewrite_chain_p = &labelled_stmt_p->u.statement.breaks_rewrite_chain;
        }
        else
        {
          rewrite_chain_p = &labelled_stmt_p->u.statement.u.iterational.continues_rewrite_chain;
        }

        if (is_simply_jumpable)
        {
          jsp_add_simple_jump_to_rewrite_chain (ctx_p, rewrite_chain_p);
        }
        else
        {
          jsp_add_nested_jump_to_rewrite_chain (ctx_p, rewrite_chain_p);
        }

        JSP_COMPLETE_STATEMENT_PARSE ();
      }
      else if (token_is (TOK_KW_RETURN))
      {
        if (jsp_get_scope_type (ctx_p) != SCOPE_TYPE_FUNCTION)
        {
          EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Return is illegal");
        }

        jsp_state_t *prev_state_p = jsp_get_prev_state (state_p);

        if (prev_state_p->state == JSP_STATE_STAT_STATEMENT_LIST)
        {
          prev_state_p->is_stmt_list_control_flow_exit_stmt_occured = true;
        }

        skip_token (ctx_p);

        if (!token_is (TOK_SEMICOLON)
            && !lexer_is_preceded_by_newlines (tok)
            && !token_is (TOK_CLOSE_BRACE))
        {
          jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, true);
          state_p->state = JSP_STATE_STAT_RETURN;
        }
        else
        {
          dump_ret (ctx_p);
          JSP_COMPLETE_STATEMENT_PARSE ();
        }
      }
      else if (token_is (TOK_KW_TRY))
      {
        skip_token (ctx_p);

        if (!jsp_is_dump_mode (ctx_p))
        {
          scopes_tree_set_contains_try (jsp_get_current_scopes_tree_node (ctx_p));
        }

        state_p->u.statement.u.try_statement.try_pos = dump_try_for_rewrite (ctx_p);

        current_token_must_be_check_and_skip_it (ctx_p, TOK_OPEN_BRACE);

        state_p->is_simply_jumpable_border = true;

        state_p->state = JSP_STATE_STAT_TRY;
        jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_BLOCK);
        jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_STATEMENT_LIST);
        jsp_state_top (ctx_p)->req_state = JSP_STATE_STAT_STATEMENT_LIST;
      }
      else if (token_is (TOK_KW_WITH))
      {
        skip_token (ctx_p);

        if (jsp_is_strict_mode (ctx_p))
        {
          EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "'with' expression is not allowed in strict mode.");
        }

        parse_expression_inside_parens_begin (ctx_p);
        jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, true);
        state_p->state = JSP_STATE_STAT_WITH;
      }
      else if (token_is (TOK_KW_THROW))
      {
        skip_token (ctx_p);

        jsp_state_t *prev_state_p = jsp_get_prev_state (state_p);

        if (prev_state_p->state == JSP_STATE_STAT_STATEMENT_LIST)
        {
          prev_state_p->is_stmt_list_control_flow_exit_stmt_occured = true;
        }

        jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, true);
        state_p->state = JSP_STATE_STAT_THROW;
      }
      else if (token_is (TOK_KW_FUNCTION))
      {
        skip_token (ctx_p);

        current_token_must_be (TOK_NAME);

        const jsp_operand_t func_name = jsp_make_string_lit_operand (token_data_as_lit_cp ());
        skip_token (ctx_p);

        state_p->state = JSP_STATE_FUNC_DECL_FINISH;

        jsp_start_parse_function_scope (ctx_p, func_name, false,  NULL);
      }
      else
      {
        bool is_expression = true;

        if (token_is (TOK_NAME))  // LabelledStatement or ExpressionStatement
        {
          const token temp = tok;
          skip_token (ctx_p);
          if (token_is (TOK_COLON))
          {
            skip_token (ctx_p);

            lit_cpointer_t name_cp;
            name_cp.packed_value = temp.uid;

            bool is_simply_jumpable;
            jsp_state_t *named_label_state_p = jsp_find_named_label (ctx_p, name_cp, &is_simply_jumpable);

            if (named_label_state_p != NULL)
            {
              EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Label is duplicated");
            }

            state_p->state = JSP_STATE_STAT_NAMED_LABEL;
            state_p->u.named_label.name_cp = name_cp;

            jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_EMPTY);

            is_expression = false;
          }
          else
          {
            seek_token (ctx_p, temp.loc);
          }
        }

        if (is_expression)
        {
          jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, true);

          if (jsp_get_scope_type (ctx_p) != SCOPE_TYPE_EVAL)
          {
            jsp_state_top (ctx_p)->is_need_retval = false;
          }

          state_p->state = JSP_STATE_STAT_EXPRESSION;
        }
      }
    }
    else if (state_p->state == JSP_STATE_STAT_BLOCK)
    {
      JERRY_ASSERT (is_stmt_list_end);

      jsp_state_t *prev_state_p = jsp_get_prev_state (state_p);

      if (prev_state_p->state == JSP_STATE_STAT_STATEMENT_LIST)
      {
        prev_state_p->is_stmt_list_control_flow_exit_stmt_occured = is_stmt_list_control_flow_exit_stmt_occured;
      }

      current_token_must_be_check_and_skip_it (ctx_p, TOK_CLOSE_BRACE);

      JSP_COMPLETE_STATEMENT_PARSE ();
    }
    else if (state_p->state == JSP_STATE_STAT_IF_BRANCH_START)
    {
      if (is_subexpr_end) // Finished parsing condition
      {
        parse_expression_inside_parens_end (ctx_p);

        dump_get_value_if_ref (ctx_p, substate_p, true);
        dump_get_value_for_state_if_const (ctx_p, substate_p);

        jsp_operand_t cond = substate_p->u.expression.operand;

        state_p->u.statement.u.if_statement.conditional_check_pos = dump_conditional_check_for_rewrite (ctx_p, cond);

        JSP_FINISH_SUBEXPR ();

        JSP_PUSH_STATE_AND_STATEMENT_PARSE (JSP_STATE_STAT_IF_BRANCH_START);
      }
      else
      {
        if (token_is (TOK_KW_ELSE))
        {
          skip_token (ctx_p);

          state_p->u.statement.u.if_statement.jump_to_end_pos = dump_jump_to_end_for_rewrite (ctx_p);
          rewrite_conditional_check (ctx_p, state_p->u.statement.u.if_statement.conditional_check_pos);

          JSP_PUSH_STATE_AND_STATEMENT_PARSE (JSP_STATE_STAT_IF_BRANCH_END);
        }
        else
        {
          rewrite_conditional_check (ctx_p, state_p->u.statement.u.if_statement.conditional_check_pos);

          JSP_COMPLETE_STATEMENT_PARSE ();
        }
      }
    }
    else if (state_p->state == JSP_STATE_STAT_IF_BRANCH_END)
    {
      rewrite_jump_to_end (ctx_p, state_p->u.statement.u.if_statement.jump_to_end_pos);

      JSP_COMPLETE_STATEMENT_PARSE ();
    }
    else if (state_p->state == JSP_STATE_STAT_STATEMENT_LIST)
    {
      if (is_subexpr_end)
      {
        JSP_FINISH_SUBEXPR ();
      }

      while (token_is (TOK_SEMICOLON))
      {
        skip_token (ctx_p);
      }

      if (token_is (TOK_CLOSE_BRACE)
          || (token_is (TOK_KW_CASE) || token_is (TOK_KW_DEFAULT)))
      {
        state_p->is_completed = true;
      }
      else
      {
        jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_EMPTY);
      }
    }
    else if (state_p->state == JSP_STATE_STAT_VAR_DECL)
    {
      skip_token (ctx_p);

      locus name_loc = tok.loc;

      current_token_must_be (TOK_NAME);

      const lit_cpointer_t lit_cp = token_data_as_lit_cp ();
      const jsp_operand_t name = jsp_make_string_lit_operand (lit_cp);

      skip_token (ctx_p);

      if (!jsp_is_dump_mode (ctx_p))
      {
        jsp_early_error_check_for_eval_and_arguments_in_strict_mode (name, jsp_is_strict_mode (ctx_p), name_loc);
      }

      if (!jsp_is_dump_mode (ctx_p))
      {
        if (!scopes_tree_variable_declaration_exists (jsp_get_current_scopes_tree_node (ctx_p), lit_cp))
        {
          dump_variable_declaration (ctx_p, lit_cp);
        }
      }

      if (token_is (TOK_EQ))
      {
        seek_token (ctx_p, name_loc);

        jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_ASSIGNMENT, true);

        jsp_state_top (ctx_p)->is_need_retval = false;
      }

      state_p->state = JSP_STATE_STAT_VAR_DECL_FINISH;
    }
    else if (state_p->state == JSP_STATE_STAT_VAR_DECL_FINISH)
    {
      if (is_subexpr_end)
      {
        JSP_FINISH_SUBEXPR ();
      }

      if (!token_is (TOK_COMMA))
      {
        JSP_COMPLETE_STATEMENT_PARSE ();

        if (state_p->var_decl)
        {
          insert_semicolon (ctx_p);
        }
      }
      else
      {
        state_p->state = JSP_STATE_STAT_VAR_DECL;
      }
    }
    else if (state_p->state == JSP_STATE_STAT_DO_WHILE)
    {
      if (is_subexpr_end)
      {
        parse_expression_inside_parens_end (ctx_p);

        dump_get_value_if_ref (ctx_p, substate_p, true);
        dump_get_value_for_state_if_const (ctx_p, substate_p);

        const jsp_operand_t cond = substate_p->u.expression.operand;

        JSP_FINISH_SUBEXPR ();

        dump_continue_iterations_check (ctx_p,
                                        state_p->u.statement.u.iterational.u.loop_do_while.next_iter_tgt_pos,
                                        cond);

        state_p->state = JSP_STATE_STAT_ITER_FINISH;
      }
      else
      {
        assert_keyword (TOK_KW_WHILE);
        skip_token (ctx_p);

        JERRY_ASSERT (state_p->u.statement.u.iterational.continue_tgt_oc == MAX_OPCODES);
        state_p->u.statement.u.iterational.continue_tgt_oc = dumper_get_current_instr_counter (ctx_p);

        parse_expression_inside_parens_begin (ctx_p);
        jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, true);
      }
    }
    else if (state_p->state == JSP_STATE_STAT_WHILE)
    {
      if (is_subexpr_end)
      {
        parse_expression_inside_parens_end (ctx_p);

        dump_get_value_if_ref (ctx_p, substate_p, true);
        dump_get_value_for_state_if_const (ctx_p, substate_p);

        const jsp_operand_t cond = substate_p->u.expression.operand;

        JSP_FINISH_SUBEXPR ();

        dump_continue_iterations_check (ctx_p, state_p->u.statement.u.iterational.u.loop_while.next_iter_tgt_pos, cond);

        seek_token (ctx_p, state_p->u.statement.u.iterational.u.loop_while.u.end_loc);

        state_p->state = JSP_STATE_STAT_ITER_FINISH;
      }
      else
      {
        JERRY_ASSERT (state_p->u.statement.u.iterational.continue_tgt_oc == MAX_OPCODES);
        state_p->u.statement.u.iterational.continue_tgt_oc = dumper_get_current_instr_counter (ctx_p);

        rewrite_jump_to_end (ctx_p, state_p->u.statement.u.iterational.u.loop_while.jump_to_end_pos);

        const locus end_loc = tok.loc;

        seek_token (ctx_p, state_p->u.statement.u.iterational.u.loop_while.u.cond_expr_start_loc);

        state_p->u.statement.u.iterational.u.loop_while.u.end_loc = end_loc;

        parse_expression_inside_parens_begin (ctx_p);
        jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, true);
      }
    }
    else if (state_p->state == JSP_STATE_STAT_FOR_INIT_END)
    {
      if (is_subexpr_end)
      {
        JSP_FINISH_SUBEXPR ();
      }

      // Jump -> ConditionCheck
      state_p->u.statement.u.iterational.u.loop_for.jump_to_end_pos = dump_jump_to_end_for_rewrite (ctx_p);

      state_p->u.statement.u.iterational.u.loop_for.next_iter_tgt_pos = dumper_set_next_iteration_target (ctx_p);

      current_token_must_be_check_and_skip_it (ctx_p, TOK_SEMICOLON);

      locus for_body_statement_loc = state_p->u.statement.u.iterational.u.loop_for.u1.body_loc;

      // Save Condition locus
      state_p->u.statement.u.iterational.u.loop_for.u1.condition_expr_loc = tok.loc;

      if (!jsp_find_next_token_before_the_locus (ctx_p,
                                                 TOK_SEMICOLON,
                                                 for_body_statement_loc,
                                                 true))
      {
        EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Invalid for statement");
      }

      current_token_must_be_check_and_skip_it (ctx_p, TOK_SEMICOLON);

      // Save Increment locus
      state_p->u.statement.u.iterational.u.loop_for.u2.increment_expr_loc = tok.loc;

      // Body
      seek_token (ctx_p, for_body_statement_loc);

      JSP_PUSH_STATE_AND_STATEMENT_PARSE (JSP_STATE_STAT_FOR_INCREMENT);
    }
    else if (state_p->state == JSP_STATE_STAT_FOR_INCREMENT)
    {
      if (is_subexpr_end)
      {
        JSP_FINISH_SUBEXPR ();

        state_p->state = JSP_STATE_STAT_FOR_COND;
      }
      else
      {
        // Save LoopEnd locus
        const locus loop_end_loc = tok.loc;

        // Setup ContinueTarget
        JERRY_ASSERT (state_p->u.statement.u.iterational.continue_tgt_oc == MAX_OPCODES);
        state_p->u.statement.u.iterational.continue_tgt_oc = dumper_get_current_instr_counter (ctx_p);

        // Increment
        seek_token (ctx_p, state_p->u.statement.u.iterational.u.loop_for.u2.increment_expr_loc);

        state_p->u.statement.u.iterational.u.loop_for.u2.end_loc = loop_end_loc;

        if (!token_is (TOK_CLOSE_PAREN))
        {
          jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, true);

          jsp_state_top (ctx_p)->is_need_retval = false;
        }
        else
        {
          state_p->state = JSP_STATE_STAT_FOR_COND;
        }
      }
    }
    else if (state_p->state == JSP_STATE_STAT_FOR_COND)
    {
      if (is_subexpr_end)
      {
        dump_get_value_if_ref (ctx_p, substate_p, true);
        dump_get_value_for_state_if_const (ctx_p, substate_p);

        jsp_operand_t cond = substate_p->u.expression.operand;

        JSP_FINISH_SUBEXPR ();

        dump_continue_iterations_check (ctx_p, state_p->u.statement.u.iterational.u.loop_for.next_iter_tgt_pos, cond);

        state_p->state = JSP_STATE_STAT_FOR_FINISH;
      }
      else
      {
        current_token_must_be (TOK_CLOSE_PAREN);

        // Setup ConditionCheck
        rewrite_jump_to_end (ctx_p, state_p->u.statement.u.iterational.u.loop_for.jump_to_end_pos);

        // Condition
        seek_token (ctx_p, state_p->u.statement.u.iterational.u.loop_for.u1.condition_expr_loc);

        if (token_is (TOK_SEMICOLON))
        {
          dump_continue_iterations_check (ctx_p,
                                          state_p->u.statement.u.iterational.u.loop_for.next_iter_tgt_pos,
                                          empty_operand ());
          state_p->state = JSP_STATE_STAT_FOR_FINISH;
        }
        else
        {
          jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, true);
        }
      }
    }
    else if (state_p->state == JSP_STATE_STAT_FOR_FINISH)
    {
      seek_token (ctx_p, state_p->u.statement.u.iterational.u.loop_for.u2.end_loc);

      state_p->state = JSP_STATE_STAT_ITER_FINISH;
    }
    else if (state_p->state == JSP_STATE_STAT_FOR_IN)
    {
      current_token_must_be_check_and_skip_it (ctx_p, TOK_CLOSE_PAREN);

      JERRY_ASSERT (is_subexpr_end);

      locus body_loc = tok.loc;

      // Dump for-in instruction
      dump_get_value_if_ref (ctx_p, substate_p, true);
      dump_get_value_for_state_if_const (ctx_p, substate_p);

      jsp_operand_t collection = substate_p->u.expression.operand;

      JSP_FINISH_SUBEXPR ();

      state_p->u.statement.u.iterational.u.loop_for_in.header_pos = dump_for_in_for_rewrite (ctx_p, collection);

      // Dump assignment VariableDeclarationNoIn / LeftHandSideExpression <- VM_REG_SPECIAL_FOR_IN_PROPERTY_NAME
      seek_token (ctx_p, state_p->u.statement.u.iterational.u.loop_for_in.u.iterator_expr_loc);

      if (token_is (TOK_KW_VAR))
      {
        skip_token (ctx_p);

        locus name_loc = tok.loc;

        current_token_must_be (TOK_NAME);

        const lit_cpointer_t lit_cp = token_data_as_lit_cp ();
        const jsp_operand_t name = jsp_make_string_lit_operand (lit_cp);

        skip_token (ctx_p);

        if (!jsp_is_dump_mode (ctx_p))
        {
          jsp_early_error_check_for_eval_and_arguments_in_strict_mode (name, jsp_is_strict_mode (ctx_p), name_loc);

          if (!scopes_tree_variable_declaration_exists (jsp_get_current_scopes_tree_node (ctx_p), lit_cp))
          {
            dump_variable_declaration (ctx_p, lit_cp);
          }
        }

        if (token_is (TOK_EQ))
        {
          seek_token (ctx_p, name_loc);

          jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_ASSIGNMENT, false);
        }
        state_p->is_var_decl_no_in = true;
        state_p->u.statement.u.iterational.u.loop_for_in.var_name_lit_cp = lit_cp;
      }
      else
      {
        state_p->is_var_decl_no_in = false;
        state_p->u.statement.u.iterational.u.loop_for_in.var_name_lit_cp = NOT_A_LITERAL;
        jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_LEFTHANDSIDE, true);
      }

      // Body
      state_p->u.statement.u.iterational.u.loop_for_in.u.body_loc = body_loc;

      state_p->state = JSP_STATE_STAT_FOR_IN_EXPR;
    }
    else if (state_p->state == JSP_STATE_STAT_FOR_IN_EXPR)
    {
      current_token_must_be (TOK_KW_IN);

      seek_token (ctx_p, state_p->u.statement.u.iterational.u.loop_for_in.u.body_loc);

      jsp_operand_t for_in_special_reg = jsp_make_reg_operand (VM_REG_SPECIAL_FOR_IN_PROPERTY_NAME);

      if (!state_p->is_var_decl_no_in)
      {
        JERRY_ASSERT (is_subexpr_end);

        if (substate_p->is_value_based_reference)
        {
          dump_prop_setter (ctx_p,
                            substate_p->u.expression.operand,
                            substate_p->u.expression.prop_name_operand,
                            for_in_special_reg);
        }
        else
        {
          dump_variable_assignment (ctx_p, substate_p->u.expression.operand, for_in_special_reg);
        }

        JSP_FINISH_SUBEXPR ();
      }
      else
      {
        JERRY_ASSERT (!is_subexpr_end);

        lit_cpointer_t var_name_lit_cp = state_p->u.statement.u.iterational.u.loop_for_in.var_name_lit_cp;

        dump_variable_assignment (ctx_p, jsp_make_identifier_operand (var_name_lit_cp), for_in_special_reg);
      }

      state_p->is_simply_jumpable_border = true;

      JSP_PUSH_STATE_AND_STATEMENT_PARSE (JSP_STATE_STAT_FOR_IN_FINISH);
    }
    else if (state_p->state == JSP_STATE_STAT_FOR_IN_FINISH)
    {
      // Save LoopEnd locus
      const locus loop_end_loc = tok.loc;

      // Setup ContinueTarget
      JERRY_ASSERT (state_p->u.statement.u.iterational.continue_tgt_oc == MAX_OPCODES);
      state_p->u.statement.u.iterational.continue_tgt_oc = dumper_get_current_instr_counter (ctx_p);

      // Write position of for-in end to for_in instruction
      rewrite_for_in (ctx_p, state_p->u.statement.u.iterational.u.loop_for_in.header_pos);

      // Dump meta (OPCODE_META_TYPE_END_FOR_IN)
      dump_for_in_end (ctx_p);

      seek_token (ctx_p, loop_end_loc);

      state_p->state = JSP_STATE_STAT_ITER_FINISH;
    }
    else if (state_p->state == JSP_STATE_STAT_ITER_FINISH)
    {
      JSP_COMPLETE_STATEMENT_PARSE ();

      jsp_rewrite_jumps_chain (ctx_p,
                               &state_p->u.statement.u.iterational.continues_rewrite_chain,
                               state_p->u.statement.u.iterational.continue_tgt_oc);
    }
    else if (state_p->state == JSP_STATE_STAT_SWITCH)
    {
      JERRY_ASSERT (is_subexpr_end);

      parse_expression_inside_parens_end (ctx_p);

      dump_get_value_if_ref (ctx_p, substate_p, false);
      dump_get_value_for_state_if_const (ctx_p, substate_p);

      jsp_operand_t switch_expr = substate_p->u.expression.operand;

      JSP_FINISH_SUBEXPR ();

      current_token_must_be_check_and_skip_it (ctx_p, TOK_OPEN_BRACE);

      state_p->state = JSP_STATE_STAT_SWITCH_BRANCH_EXPR;

      state_p->u.statement.u.switch_statement.expr = switch_expr;
      state_p->u.statement.u.switch_statement.default_label_oc = MAX_OPCODES;
      state_p->u.statement.u.switch_statement.last_cond_check_jmp_oc = MAX_OPCODES;
      state_p->u.statement.u.switch_statement.skip_check_jmp_oc = MAX_OPCODES;

      dumper_save_reg_alloc_ctx (ctx_p,
                                 &state_p->u.statement.u.switch_statement.saved_reg_next,
                                 &state_p->u.statement.u.switch_statement.saved_reg_max_for_temps);
    }
    else if (state_p->state == JSP_STATE_STAT_SWITCH_BRANCH_EXPR)
    {
      if (is_subexpr_end)
      {
        /* finished parse of an Expression in CaseClause */
        dump_get_value_if_ref (ctx_p, substate_p, true);
        dump_get_value_for_state_if_const (ctx_p, substate_p);

        jsp_operand_t case_expr = substate_p->u.expression.operand;

        JSP_FINISH_SUBEXPR ();

        current_token_must_be_check_and_skip_it (ctx_p, TOK_COLON);

        jsp_operand_t switch_expr = state_p->u.statement.u.switch_statement.expr;
        jsp_operand_t condition_reg = tmp_operand ();

        if (jsp_is_register_operand (case_expr))
        {
          /* reuse the register */
          condition_reg = case_expr;
        }

        dump_binary_op (ctx_p, VM_OP_EQUAL_VALUE_TYPE, condition_reg, switch_expr, case_expr);

        jsp_add_conditional_jump_to_rewrite_chain (ctx_p,
                                                   &state_p->u.statement.u.switch_statement.last_cond_check_jmp_oc,
                                                   true,
                                                   condition_reg);

        uint32_t num = jsp_rewrite_jumps_chain (ctx_p,
                                                &state_p->u.statement.u.switch_statement.skip_check_jmp_oc,
                                                dumper_get_current_instr_counter (ctx_p));
        JERRY_ASSERT (num <= 1);

        jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_STATEMENT_LIST);
        jsp_state_top (ctx_p)->req_state = JSP_STATE_STAT_STATEMENT_LIST;
      }
      else if (token_is (TOK_CLOSE_BRACE))
      {
        skip_token (ctx_p);

        vm_instr_counter_t last_cond_check_tgt_oc;

        if (state_p->u.statement.u.switch_statement.default_label_oc != MAX_OPCODES)
        {
          last_cond_check_tgt_oc = state_p->u.statement.u.switch_statement.default_label_oc;
        }
        else
        {
          last_cond_check_tgt_oc = dumper_get_current_instr_counter (ctx_p);
        }

        uint32_t num = jsp_rewrite_jumps_chain (ctx_p,
                                                &state_p->u.statement.u.switch_statement.last_cond_check_jmp_oc,
                                                last_cond_check_tgt_oc);
        JERRY_ASSERT (num <= 1);

        JSP_COMPLETE_STATEMENT_PARSE ();
      }
      else
      {
        if (is_stmt_list_end
            && !is_stmt_list_control_flow_exit_stmt_occured
            && token_is (TOK_KW_CASE))
        {
          jsp_add_simple_jump_to_rewrite_chain (ctx_p,
                                                &state_p->u.statement.u.switch_statement.skip_check_jmp_oc);
        }

        if (token_is (TOK_KW_CASE) || token_is (TOK_KW_DEFAULT))
        {
          if (!is_stmt_list_end /* no StatementList[opt] occured in the SwitchStatement yet,
                                 * so no conditions were checked for now and the DefaultClause
                                 * should be jumped over */
              && token_is (TOK_KW_DEFAULT))
          {
            /* first clause is DefaultClause */
            JERRY_ASSERT (state_p->u.statement.u.switch_statement.default_label_oc == MAX_OPCODES);

            /* the check is unconditional as it is jump over DefaultClause */
            jsp_add_simple_jump_to_rewrite_chain (ctx_p,
                                                  &state_p->u.statement.u.switch_statement.last_cond_check_jmp_oc);
          }

          if (token_is (TOK_KW_CASE))
          {
            skip_token (ctx_p);

            uint32_t num = jsp_rewrite_jumps_chain (ctx_p,
                                                    &state_p->u.statement.u.switch_statement.last_cond_check_jmp_oc,
                                                    dumper_get_current_instr_counter (ctx_p));
            JERRY_ASSERT (num <= 1);

            dumper_restore_reg_alloc_ctx (ctx_p,
                                          state_p->u.statement.u.switch_statement.saved_reg_next,
                                          state_p->u.statement.u.switch_statement.saved_reg_max_for_temps,
                                          false);

            jsp_push_new_expr_state (ctx_p, JSP_STATE_EXPR_EMPTY, JSP_STATE_EXPR_EXPRESSION, true);
          }
          else
          {
            JERRY_ASSERT (token_is (TOK_KW_DEFAULT));
            skip_token (ctx_p);

            if (state_p->u.statement.u.switch_statement.default_label_oc != MAX_OPCODES)
            {
              EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Duplication of 'default' clause");
            }

            state_p->u.statement.u.switch_statement.default_label_oc = dumper_get_current_instr_counter (ctx_p);

            uint32_t num = jsp_rewrite_jumps_chain (ctx_p,
                                                    &state_p->u.statement.u.switch_statement.skip_check_jmp_oc,
                                                    dumper_get_current_instr_counter (ctx_p));
            JERRY_ASSERT (num <= 1);

            current_token_must_be_check_and_skip_it (ctx_p, TOK_COLON);

            jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_STATEMENT_LIST);
            jsp_state_top (ctx_p)->req_state = JSP_STATE_STAT_STATEMENT_LIST;
          }
        }
        else
        {
          EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Expected 'case' or' 'default' clause");
        }
      }
    }
    else if (state_p->state == JSP_STATE_STAT_TRY)
    {
      rewrite_try (ctx_p, state_p->u.statement.u.try_statement.try_pos);

      if (!token_is (TOK_KW_CATCH)
          && !token_is (TOK_KW_FINALLY))
      {
        EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Expected either 'catch' or 'finally' token");
      }

      if (token_is (TOK_KW_CATCH))
      {
        skip_token (ctx_p);

        current_token_must_be_check_and_skip_it (ctx_p, TOK_OPEN_PAREN);

        current_token_must_be (TOK_NAME);

        const jsp_operand_t name = jsp_make_string_lit_operand (token_data_as_lit_cp ());

        if (!jsp_is_dump_mode (ctx_p))
        {
          jsp_early_error_check_for_eval_and_arguments_in_strict_mode (name, jsp_is_strict_mode (ctx_p), tok.loc);
        }

        skip_token (ctx_p);

        current_token_must_be_check_and_skip_it (ctx_p, TOK_CLOSE_PAREN);

        state_p->u.statement.u.try_statement.catch_pos = dump_catch_for_rewrite (ctx_p, name);

        current_token_must_be_check_and_skip_it (ctx_p, TOK_OPEN_BRACE);

        state_p->state = JSP_STATE_STAT_CATCH_FINISH;

        JERRY_ASSERT (state_p->is_simply_jumpable_border);

        jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_BLOCK);
        jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_STATEMENT_LIST);
        jsp_state_top (ctx_p)->req_state = JSP_STATE_STAT_STATEMENT_LIST;
      }
      else
      {
        JERRY_ASSERT (token_is (TOK_KW_FINALLY));
        skip_token (ctx_p);

        state_p->u.statement.u.try_statement.finally_pos = dump_finally_for_rewrite (ctx_p);

        current_token_must_be_check_and_skip_it (ctx_p, TOK_OPEN_BRACE);

        state_p->state = JSP_STATE_STAT_FINALLY_FINISH;

        JERRY_ASSERT (state_p->is_simply_jumpable_border);

        jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_BLOCK);
        jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_STATEMENT_LIST);
        jsp_state_top (ctx_p)->req_state = JSP_STATE_STAT_STATEMENT_LIST;
      }
    }
    else if (state_p->state == JSP_STATE_STAT_CATCH_FINISH)
    {
      rewrite_catch (ctx_p, state_p->u.statement.u.try_statement.catch_pos);

      if (token_is (TOK_KW_FINALLY))
      {
        skip_token (ctx_p);
        state_p->u.statement.u.try_statement.finally_pos = dump_finally_for_rewrite (ctx_p);

        current_token_must_be_check_and_skip_it (ctx_p, TOK_OPEN_BRACE);

        state_p->state = JSP_STATE_STAT_FINALLY_FINISH;

        JERRY_ASSERT (state_p->is_simply_jumpable_border);

        jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_BLOCK);
        jsp_start_statement_parse (ctx_p, JSP_STATE_STAT_STATEMENT_LIST);
        jsp_state_top (ctx_p)->req_state = JSP_STATE_STAT_STATEMENT_LIST;
      }
      else
      {
        state_p->state = JSP_STATE_STAT_TRY_FINISH;
      }
    }
    else if (state_p->state == JSP_STATE_STAT_FINALLY_FINISH)
    {
      rewrite_finally (ctx_p, state_p->u.statement.u.try_statement.finally_pos);

      state_p->state = JSP_STATE_STAT_TRY_FINISH;
    }
    else if (state_p->state == JSP_STATE_STAT_TRY_FINISH)
    {
      dump_end_try_catch_finally (ctx_p);

      JSP_COMPLETE_STATEMENT_PARSE ();
    }
    else if (state_p->state == JSP_STATE_STAT_WITH)
    {
      if (is_subexpr_end)
      {
        parse_expression_inside_parens_end (ctx_p);

        dump_get_value_if_ref (ctx_p, substate_p, true);
        dump_get_value_for_state_if_const (ctx_p, substate_p);

        const jsp_operand_t expr = substate_p->u.expression.operand;

        JSP_FINISH_SUBEXPR ();

        if (!jsp_is_dump_mode (ctx_p))
        {
          scopes_tree_set_contains_with (jsp_get_current_scopes_tree_node (ctx_p));
        }

        state_p->is_simply_jumpable_border = true;

        state_p->u.statement.u.with_statement.header_pos = dump_with_for_rewrite (ctx_p, expr);

        JSP_PUSH_STATE_AND_STATEMENT_PARSE (JSP_STATE_STAT_WITH);
      }
      else
      {
        rewrite_with (ctx_p, state_p->u.statement.u.with_statement.header_pos);
        dump_with_end (ctx_p);

        JSP_COMPLETE_STATEMENT_PARSE ();
      }
    }
    else if (state_p->state == JSP_STATE_FUNC_DECL_FINISH)
    {
      JERRY_ASSERT (is_source_elements_list_end);
      jsp_finish_parse_function_scope (ctx_p);

      JSP_COMPLETE_STATEMENT_PARSE ();
    }
    else if (state_p->state == JSP_STATE_STAT_NAMED_LABEL)
    {
      jsp_state_pop (ctx_p);
    }
    else if (state_p->state == JSP_STATE_STAT_EXPRESSION)
    {
      JERRY_ASSERT (is_subexpr_end);
      insert_semicolon (ctx_p);

      dump_get_value_if_ref (ctx_p, substate_p, false);

      if (jsp_get_scope_type (ctx_p) == SCOPE_TYPE_EVAL)
      {
        JERRY_ASSERT (substate_p->is_need_retval);

        dump_variable_assignment (ctx_p,
                                  jsp_make_reg_operand (VM_REG_SPECIAL_EVAL_RET),
                                  substate_p->u.expression.operand);
      }

      JSP_FINISH_SUBEXPR ();

      JSP_COMPLETE_STATEMENT_PARSE ();
    }
    else if (state_p->state == JSP_STATE_STAT_RETURN)
    {
      JERRY_ASSERT (is_subexpr_end);

      dump_get_value_if_ref (ctx_p, substate_p, true);
      dump_get_value_for_state_if_const (ctx_p, substate_p);

      const jsp_operand_t op = substate_p->u.expression.operand;

      JSP_FINISH_SUBEXPR ();

      dump_retval (ctx_p, op);

      insert_semicolon (ctx_p);

      JSP_COMPLETE_STATEMENT_PARSE ();
    }
    else if (state_p->state == JSP_STATE_STAT_THROW)
    {
      JERRY_ASSERT (is_subexpr_end);

      dump_get_value_if_ref (ctx_p, substate_p, true);
      dump_get_value_for_state_if_const (ctx_p, substate_p);

      const jsp_operand_t op = substate_p->u.expression.operand;

      JSP_FINISH_SUBEXPR ();

      dump_throw (ctx_p, op);

      insert_semicolon (ctx_p);

      JSP_COMPLETE_STATEMENT_PARSE ();
    }
    else
    {
      JERRY_ASSERT (state_p->state == JSP_STATE_STAT_STATEMENT);
      JERRY_ASSERT (!state_p->is_completed);

      vm_instr_counter_t break_tgt_oc = dumper_get_current_instr_counter (ctx_p);
      jsp_rewrite_jumps_chain (ctx_p,
                               &state_p->u.statement.breaks_rewrite_chain,
                               break_tgt_oc);

      state_p->is_completed = true;
    }

    JERRY_ASSERT (substate_p == NULL);
  }
} /* jsp_parse_source_element_list */

/**
 * Parse program
 *
 * program
 *  : LT!* source_element_list LT!* EOF!
 *  ;
 *
 * @return true - if parse finished successfully (no SyntaxError was raised);
 *         false - otherwise.
 */
static jsp_status_t
parser_parse_program (const jerry_api_char_t *source_p, /**< source code buffer */
                      size_t source_size, /**< source code size in bytes */
                      bool in_eval, /**< flag indicating if we are parsing body of eval code */
                      bool is_strict, /**< flag, indicating whether current code
                                       *   inherited strict mode from code of an outer scope */
                      const bytecode_data_header_t **out_bytecode_data_p, /**< out: generated byte-code array
                                                                           *  (in case there were no syntax errors) */
                      bool *out_contains_functions_p) /**< out: optional (can be NULL, if the output is not needed)
                                                       *        flag, indicating whether the compiled byte-code
                                                       *        contains a function declaration / expression */
{
  JERRY_ASSERT (out_bytecode_data_p != NULL);

  const scope_type_t scope_type = (in_eval ? SCOPE_TYPE_EVAL : SCOPE_TYPE_GLOBAL);

#ifndef JERRY_NDEBUG
  volatile bool is_pre_parse_finished = false;
#endif /* !JERRY_NDEBUG */

  jsp_status_t status;

  jsp_mm_init ();

  jsp_ctx_t ctx;
  jsp_init_ctx (&ctx, scope_type);

  jsp_stack_init (&ctx);

  dumper_init (&ctx, parser_show_instrs);
  jsp_early_error_init ();

  scopes_tree_init ();

  scopes_tree scope = scopes_tree_new_scope (NULL, scope_type);
  scopes_tree_set_strict_mode (scope, is_strict);

  jsp_set_current_scopes_tree_node (&ctx, scope);

  jmp_buf *jsp_early_error_label_p = jsp_early_error_get_early_error_longjmp_label ();
  int r = setjmp (*jsp_early_error_label_p);

  if (r == 0)
  {
    /*
     * Note:
     *      Operations that could raise an early error can be performed only during execution of the block.
     */
    lexer_init (source_p, source_size, parser_show_instrs);
    skip_token (&ctx);

    locus start_loc = tok.loc;

    jsp_parse_directive_prologue (&ctx);

    jsp_parse_source_element_list (&ctx, scope_type);
    JERRY_ASSERT (token_is (TOK_EOF));

    jsp_set_current_scopes_tree_node (&ctx, NULL);

    scopes_tree_finish_build ();

#ifndef JERRY_NDEBUG
    is_pre_parse_finished = true;
#endif /* !JERRY_NDEBUG */

    jsp_early_error_free ();

    if (out_contains_functions_p != NULL)
    {
      *out_contains_functions_p = scope->contains_functions;
    }

    jsp_switch_to_dump_mode (&ctx, scope);

    scopes_tree scope_to_dump_p = jsp_get_next_scopes_tree_node_to_dump (&ctx);
    JERRY_ASSERT (scope_to_dump_p == scope);

    bytecode_data_header_t *root_bc_header_p = bc_dump_single_scope (scope);

    scopes_tree_free_scope (scope);

    bc_register_root_bytecode_header (root_bc_header_p);

    *out_bytecode_data_p = root_bc_header_p;

    jsp_set_current_bytecode_header (&ctx, root_bc_header_p);

    seek_token (&ctx, start_loc);

    jsp_parse_source_element_list (&ctx, scope_type);

    jsp_set_current_bytecode_header (&ctx, NULL);

    jsp_stack_finalize (&ctx);

    JERRY_ASSERT (jsp_get_scope_type (&ctx) == scope_type);

    if (parser_show_instrs)
    {
      lit_dump_literals ();
      bc_print_instrs (root_bc_header_p);
    }

    scopes_tree_finalize ();

    status = JSP_STATUS_OK;
  }
  else
  {
    /* SyntaxError handling */

#ifndef JERRY_NDEBUG
    JERRY_ASSERT (!is_pre_parse_finished);
#endif /* !JERRY_NDEBUG */

    *out_bytecode_data_p = NULL;

    jsp_mm_free_all ();

    jsp_early_error_t type = jsp_early_error_get_type ();

    if (type == JSP_EARLY_ERROR_SYNTAX)
    {
      status = JSP_STATUS_SYNTAX_ERROR;
    }
    else
    {
      JERRY_ASSERT (type == JSP_EARLY_ERROR_REFERENCE);

      status = JSP_STATUS_REFERENCE_ERROR;
    }
  }

  jsp_mm_finalize ();

  return status;
} /* parser_parse_program */

/**
 * Parse source script
 *
 * @return true - if parse finished successfully (no SyntaxError were raised);
 *         false - otherwise.
 */
jsp_status_t
parser_parse_script (const jerry_api_char_t *source, /**< source script */
                     size_t source_size, /**< source script size it bytes */
                     const bytecode_data_header_t **out_bytecode_data_p) /**< out: generated byte-code array
                                                                          *  (in case there were no syntax errors) */
{
  return parser_parse_program (source, source_size, false, false, out_bytecode_data_p, NULL);
} /* parser_parse_script */

/**
 * Parse string passed to eval() call
 *
 * @return true - if parse finished successfully (no SyntaxError were raised);
 *         false - otherwise.
 */
jsp_status_t
parser_parse_eval (const jerry_api_char_t *source, /**< string passed to eval() */
                   size_t source_size, /**< string size in bytes */
                   bool is_strict, /**< flag, indicating whether eval is called
                                    *   from strict code in direct mode */
                   const bytecode_data_header_t **out_bytecode_data_p, /**< out: generated byte-code array
                                                                        *  (in case there were no syntax errors) */
                   bool *out_contains_functions_p) /**< out: flag, indicating whether the compiled byte-code
                                                    *        contains a function declaration / expression */
{
  JERRY_ASSERT (out_contains_functions_p != NULL);

  return parser_parse_program (source,
                               source_size,
                               true,
                               is_strict,
                               out_bytecode_data_p,
                               out_contains_functions_p);
} /* parser_parse_eval */

/**
 * Tell parser whether to dump bytecode
 */
void
parser_set_show_instrs (bool show_instrs) /**< flag indicating whether to dump bytecode */
{
  parser_show_instrs = show_instrs;
} /* parser_set_show_instrs */

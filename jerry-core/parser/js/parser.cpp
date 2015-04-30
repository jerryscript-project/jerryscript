/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#include <stdarg.h>

#include "jrt-libc-includes.h"
#include "parser.h"
#include "opcodes.h"
#include "serializer.h"
#include "vm.h"
#include "stack.h"
#include "hash-table.h"
#include "opcodes-native-call.h"
#include "scopes-tree.h"
#include "ecma-helpers.h"
#include "syntax-errors.h"
#include "opcodes-dumper.h"
#include "serializer.h"

#define NESTING_ITERATIONAL 1
#define NESTING_SWITCH      2
#define NESTING_FUNCTION    3

static token tok;

enum
{
  nestings_global_size
};
STATIC_STACK (nestings, uint8_t)

enum
{
  scopes_global_size
};
STATIC_STACK (scopes, scopes_tree)

#define EMIT_ERROR(MESSAGE) PARSE_ERROR(MESSAGE, tok.loc)
#define EMIT_SORRY(MESSAGE) PARSE_SORRY(MESSAGE, tok.loc)
#define EMIT_ERROR_VARG(MESSAGE, ...) PARSE_ERROR_VARG(MESSAGE, tok.loc, __VA_ARGS__)

#define NESTING_TO_STRING(I) (I == NESTING_FUNCTION \
                                ? "function" \
                                : I == NESTING_ITERATIONAL \
                                  ? "iterational" \
                                  : I == NESTING_SWITCH \
                                    ? "switch" : "unknown")

#define OPCODE_IS(OP, ID) (OP.op_idx == __op__idx_##ID)

static operand parse_expression (bool);
static void parse_statement (void);
static operand parse_assignment_expression (bool);
static void parse_source_element_list (bool);
static operand parse_argument_list (varg_list_type, operand, uint8_t *, operand *);
static void process_keyword_names (void);
static void skip_braces (void);
static void skip_parens (void);

static void
push_nesting (uint8_t nesting_type)
{
  STACK_PUSH (nestings, nesting_type);
}

static void
pop_nesting (uint8_t nesting_type)
{
  JERRY_ASSERT (STACK_HEAD (nestings, 1) == nesting_type);
  STACK_DROP (nestings, 1);
}

static void
must_be_inside_but_not_in (uint8_t not_in, uint8_t insides_count, ...)
{
  va_list insides_list;
  if (STACK_SIZE(nestings) == 0)
  {
    EMIT_ERROR ("Shall be inside a nesting");
  }

  va_start (insides_list, insides_count);
  uint8_t *insides = (uint8_t*) mem_heap_alloc_block (insides_count, MEM_HEAP_ALLOC_SHORT_TERM);
  for (uint8_t i = 0; i < insides_count; i++)
  {
    insides[i] = (uint8_t) va_arg (insides_list, int);
  }
  va_end (insides_list);

  for (uint8_t i = (uint8_t) STACK_SIZE (nestings); i != 0; i--)
  {
    for (uint8_t j = 0; j < insides_count; j++)
    {
      if (insides[j] == STACK_ELEMENT (nestings, i - 1))
      {
        mem_heap_free_block (insides);
        return;
      }
    }
    if (STACK_ELEMENT (nestings, i - 1) == not_in)
    {
      EMIT_ERROR_VARG ("Shall not be inside a '%s' nesting", NESTING_TO_STRING(not_in));
    }
  }
  EMIT_ERROR ("Shall be inside a nesting");
}

static bool
token_is (token_type tt)
{
  return tok.type == tt;
}

static literal_index_t
token_data (void)
{
  return tok.uid;
}

static void
skip_token (void)
{
  tok = lexer_next_token ();
}

static void
assert_keyword (keyword kw)
{
  if (!token_is (TOK_KEYWORD) || token_data () != kw)
  {
    EMIT_ERROR_VARG ("Expected keyword '%s'", lexer_keyword_to_string (kw));
    JERRY_UNREACHABLE ();
  }
}

static bool
is_keyword (keyword kw)
{
  return token_is (TOK_KEYWORD) && token_data () == kw;
}

static void
current_token_must_be (token_type tt)
{
  if (!token_is (tt))
  {
    EMIT_ERROR_VARG ("Expected '%s' token", lexer_token_type_to_string (tt));
  }
}

static void
skip_newlines (void)
{
  do
  {
    skip_token ();
  }
  while (token_is (TOK_NEWLINE));
}

static void
next_token_must_be (token_type tt)
{
  skip_token ();
  if (!token_is (tt))
  {
    EMIT_ERROR_VARG ("Expected '%s' token", lexer_token_type_to_string (tt));
  }
}

static void
token_after_newlines_must_be (token_type tt)
{
  skip_newlines ();
  if (!token_is (tt))
  {
    EMIT_ERROR_VARG ("Expected '%s' token", lexer_token_type_to_string (tt));
  }
}

static void
token_after_newlines_must_be_keyword (keyword kw)
{
  skip_newlines ();
  if (!is_keyword (kw))
  {
    EMIT_ERROR_VARG ("Expected keyword '%s'", lexer_keyword_to_string (kw));
  }
}

static bool
is_strict_mode (void)
{
  return scopes_tree_strict_mode (STACK_TOP (scopes));
}

/* property_name
  : Identifier
  | Keyword
  | StringLiteral
  | NumericLiteral
  ;
*/
static operand
parse_property_name (void)
{
  switch (tok.type)
  {
    case TOK_NAME:
    case TOK_STRING:
    case TOK_NUMBER:
    {
      return literal_operand (token_data ());
    }
    case TOK_SMALL_INT:
    {
      const literal lit = create_literal_from_num ((ecma_number_t) token_data ());
      lexer_add_keyword_or_numeric_literal_if_not_present (lit);
      const literal_index_t lit_id = lexer_lookup_literal_uid (lit);
      return literal_operand (lit_id);
    }
    case TOK_KEYWORD:
    {
      const literal lit = create_literal_from_str_compute_len (lexer_keyword_to_string ((keyword) token_data ()));
      lexer_add_keyword_or_numeric_literal_if_not_present (lit);
      const literal_index_t lit_id = lexer_lookup_literal_uid (lit);
      return literal_operand (lit_id);
    }
    default:
    {
      EMIT_ERROR_VARG ("Wrong property name type: %s", lexer_token_type_to_string (tok.type));
    }
  }
}

/* property_name_and_value
  : property_name LT!* ':' LT!* assignment_expression
  ; */
static void
parse_property_name_and_value (void)
{
  const operand name = parse_property_name ();
  token_after_newlines_must_be (TOK_COLON);
  skip_newlines ();
  const operand value = parse_assignment_expression (true);
  dump_prop_name_and_value (name, value);
  syntax_add_prop_name (name, PROP_DATA);
}

/* property_assignment
  : property_name_and_value
  | get LT!* property_name LT!* '(' LT!* ')' LT!* '{' LT!* function_body LT!* '}'
  | set LT!* property_name LT!* '(' identifier ')' LT!* '{' LT!* function_body LT!* '}'
  ; */
static void
parse_property_assignment (void)
{
  STACK_DECLARE_USAGE (nestings);

  if (token_is (TOK_NAME))
  {
    bool is_setter;

    if (literal_equal_type_s (lexer_get_literal_by_id (token_data ()), "get"))
    {
      is_setter = false;
    }
    else if (literal_equal_type_s (lexer_get_literal_by_id (token_data ()), "set"))
    {
      is_setter = true;
    }
    else
    {
      parse_property_name_and_value ();

      goto cleanup;
    }

    const token temp = tok;
    skip_newlines ();
    if (token_is (TOK_COLON))
    {
      lexer_save_token (tok);
      tok = temp;

      parse_property_name_and_value ();

      goto cleanup;
    }

    const operand name = parse_property_name ();
    syntax_add_prop_name (name, is_setter ? PROP_SET : PROP_GET);

    skip_newlines ();
    const operand func = parse_argument_list (VARG_FUNC_EXPR, name, NULL, NULL);

    dump_function_end_for_rewrite ();

    const bool is_strict = scopes_tree_strict_mode (STACK_TOP (scopes));
    scopes_tree_set_strict_mode (STACK_TOP (scopes), false);

    token_after_newlines_must_be (TOK_OPEN_BRACE);
    skip_newlines ();
    push_nesting (NESTING_FUNCTION);
    parse_source_element_list (false);
    pop_nesting (NESTING_FUNCTION);
    token_after_newlines_must_be (TOK_CLOSE_BRACE);

    scopes_tree_set_strict_mode (STACK_TOP (scopes), is_strict);

    dump_ret ();
    rewrite_function_end (VARG_FUNC_EXPR);
    if (is_setter)
    {
      dump_prop_setter_decl (name, func);
    }
    else
    {
      dump_prop_getter_decl (name, func);
    }
  }
  else
  {
    parse_property_name_and_value ();
  }

cleanup:
  STACK_CHECK_USAGE (nestings);
}

/** Parse list of identifiers, assigment expressions or properties, splitted by comma.
    For each ALT dumps appropriate bytecode. Uses OBJ during dump if neccesary.
    Result tmp. */
static operand
parse_argument_list (varg_list_type vlt, operand obj, uint8_t *args_count, operand *this_arg)
{
  token_type close_tt = TOK_CLOSE_PAREN;
  uint8_t args_num = 0;

  switch (vlt)
  {
    case VARG_FUNC_DECL:
    case VARG_FUNC_EXPR:
    {
      syntax_start_checking_of_vargs ();
      /* FALLTHRU */
    }
    case VARG_CONSTRUCT_EXPR:
    {
      current_token_must_be (TOK_OPEN_PAREN);
      dump_varg_header_for_rewrite (vlt, obj);
      break;
    }
    case VARG_CALL_EXPR:
    {
      current_token_must_be (TOK_OPEN_PAREN);
      if (dumper_is_intrinsic (obj))
      {
        break;
      }
      if (this_arg != NULL && this_arg->type == OPERAND_LITERAL)
      {
        *this_arg = dump_variable_assignment_res (*this_arg);
      }
      dump_varg_header_for_rewrite (vlt, obj);
      break;
    }
    case VARG_ARRAY_DECL:
    {
      current_token_must_be (TOK_OPEN_SQUARE);
      close_tt = TOK_CLOSE_SQUARE;
      dump_varg_header_for_rewrite (vlt, obj);
      break;
    }
    case VARG_OBJ_DECL:
    {
      current_token_must_be (TOK_OPEN_BRACE);
      close_tt = TOK_CLOSE_BRACE;
      dump_varg_header_for_rewrite (vlt, obj);
      syntax_start_checking_of_prop_names ();
      break;
    }
  }
  if (vlt == VARG_CALL_EXPR && this_arg != NULL && !operand_is_empty (*this_arg))
  {
    dump_this_arg (*this_arg);
    args_num++;
  }

  skip_newlines ();
  while (!token_is (close_tt))
  {
    operand op;
    switch (vlt)
    {
      case VARG_FUNC_DECL:
      case VARG_FUNC_EXPR:
      {
        current_token_must_be (TOK_NAME);
        op = literal_operand (token_data ());
        syntax_add_varg (op);
        syntax_check_for_eval_and_arguments_in_strict_mode (op, is_strict_mode (), tok.loc);
        break;
      }
      case VARG_ARRAY_DECL:
      {
        if (token_is (TOK_COMMA))
        {
          op = dump_undefined_assignment_res ();
          dump_varg (op);
          args_num++;
          skip_newlines ();
          continue;
        }
        /* FALLTHRU  */
      }
      case VARG_CONSTRUCT_EXPR:
      {
        op = parse_assignment_expression (true);
        break;
      }
      case VARG_CALL_EXPR:
      {
        op = parse_assignment_expression (true);
        if (dumper_is_intrinsic (obj))
        {
          operand res = dump_intrinsic (obj, op);
          token_after_newlines_must_be (close_tt);
          return res;
        }
        break;
      }
      case VARG_OBJ_DECL:
      {
        parse_property_assignment ();
        break;
      }
    }

    /* In case of obj_decl prop is already dumped.  */
    if (vlt != VARG_OBJ_DECL)
    {
      dump_varg (op);
    }
    args_num++;

    skip_newlines ();
    if (!token_is (TOK_COMMA))
    {
      current_token_must_be (close_tt);
      break;
    }

    skip_newlines ();
  }

  if (args_count != NULL)
  {
    *args_count = args_num;
  }

  operand res;
  switch (vlt)
  {
    case VARG_FUNC_DECL:
    case VARG_FUNC_EXPR:
    {
      syntax_check_for_syntax_errors_in_formal_param_list (is_strict_mode (), tok.loc);
      res = rewrite_varg_header_set_args_count (args_num);
      break;
    }
    case VARG_CONSTRUCT_EXPR:
    case VARG_ARRAY_DECL:
    case VARG_CALL_EXPR:
    {
      /* Intrinsics are already processed.  */
      res = rewrite_varg_header_set_args_count (args_num);
      break;
    }
    case VARG_OBJ_DECL:
    {
      syntax_check_for_duplication_of_prop_names (is_strict_mode (), tok.loc);
      res = rewrite_varg_header_set_args_count (args_num);
      break;
    }
  }
  return res;
}

/* function_declaration
  : 'function' LT!* Identifier LT!*
    '(' (LT!* Identifier (LT!* ',' LT!* Identifier)*) ? LT!* ')' LT!* function_body
  ;

   function_body
  : '{' LT!* sourceElements LT!* '}' */
static void
parse_function_declaration (void)
{
  STACK_DECLARE_USAGE (scopes);
  STACK_DECLARE_USAGE (nestings);

  assert_keyword (KW_FUNCTION);

  token_after_newlines_must_be (TOK_NAME);
  const operand name = literal_operand (token_data ());

  skip_newlines ();
  STACK_PUSH (scopes, scopes_tree_init (STACK_TOP (scopes)));
  serializer_set_scope (STACK_TOP (scopes));
  scopes_tree_set_strict_mode (STACK_TOP (scopes), scopes_tree_strict_mode (STACK_HEAD (scopes, 2)));
  lexer_set_strict_mode (scopes_tree_strict_mode (STACK_TOP (scopes)));
  parse_argument_list (VARG_FUNC_DECL, name, NULL, NULL);

  dump_function_end_for_rewrite ();

  token_after_newlines_must_be (TOK_OPEN_BRACE);

  skip_newlines ();
  push_nesting (NESTING_FUNCTION);
  parse_source_element_list (false);
  pop_nesting (NESTING_FUNCTION);

  next_token_must_be (TOK_CLOSE_BRACE);

  dump_ret ();
  rewrite_function_end (VARG_FUNC_DECL);

  STACK_DROP (scopes, 1);
  serializer_set_scope (STACK_TOP (scopes));
  lexer_set_strict_mode (scopes_tree_strict_mode (STACK_TOP (scopes)));

  STACK_CHECK_USAGE (scopes);
  STACK_CHECK_USAGE (nestings);
}

/* function_expression
  : 'function' LT!* Identifier? LT!* '(' formal_parameter_list? LT!* ')' LT!* function_body
  ; */
static operand
parse_function_expression (void)
{
  STACK_DECLARE_USAGE (nestings)

  assert_keyword (KW_FUNCTION);

  operand res;

  skip_newlines ();
  if (token_is (TOK_NAME))
  {
    const operand name = literal_operand (token_data ());
    skip_newlines ();
    res = parse_argument_list (VARG_FUNC_EXPR, name, NULL, NULL);
  }
  else
  {
    lexer_save_token (tok);
    skip_newlines ();
    res = parse_argument_list (VARG_FUNC_EXPR, empty_operand (), NULL, NULL);
  }

  dump_function_end_for_rewrite ();

  token_after_newlines_must_be (TOK_OPEN_BRACE);
  skip_newlines ();
  push_nesting (NESTING_FUNCTION);
  parse_source_element_list (false);
  pop_nesting (NESTING_FUNCTION);
  next_token_must_be (TOK_CLOSE_BRACE);

  dump_ret ();
  rewrite_function_end (VARG_FUNC_EXPR);

  STACK_CHECK_USAGE (nestings);

  return res;
}

/* array_literal
  : '[' LT!* assignment_expression? (LT!* ',' (LT!* assignment_expression)?)* LT!* ']' LT!*
  ; */
static operand
parse_array_literal (void)
{
  return parse_argument_list (VARG_ARRAY_DECL, empty_operand (), NULL, NULL);
}

/* object_literal
  : '{' LT!* property_assignment (LT!* ',' LT!* property_assignment)* LT!* '}'
  ; */
static operand
parse_object_literal (void)
{
  return parse_argument_list (VARG_OBJ_DECL, empty_operand (), NULL, NULL);
}

/* literal
  : 'null'
  | 'true'
  | 'false'
  | number_literal
  | string_literal
  ; */
static operand
parse_literal (void)
{
  switch (tok.type)
  {
    case TOK_NUMBER: return dump_number_assignment_res (token_data ());
    case TOK_STRING: return dump_string_assignment_res (token_data ());
    case TOK_NULL: return dump_null_assignment_res ();
    case TOK_BOOL: return dump_boolean_assignment_res ((bool) token_data ());
    case TOK_SMALL_INT: return dump_smallint_assignment_res ((idx_t) token_data ());
    default:
    {
      EMIT_ERROR ("Expected literal");
    }
  }
}

/* primary_expression
  : 'this'
  | Identifier
  | literal
  | 'undefined'
  | '[' LT!* array_literal LT!* ']'
  | '{' LT!* object_literal LT!* '}'
  | '(' LT!* expression LT!* ')'
  ; */
static operand
parse_primary_expression (void)
{
  if (is_keyword (KW_THIS))
  {
    return dump_this_res ();
  }

  switch (tok.type)
  {
    case TOK_NULL:
    case TOK_BOOL:
    case TOK_SMALL_INT:
    case TOK_NUMBER:
    case TOK_STRING: return parse_literal ();
    case TOK_NAME: return literal_operand (token_data ());
    case TOK_OPEN_SQUARE: return parse_array_literal ();
    case TOK_OPEN_BRACE: return parse_object_literal ();
    case TOK_OPEN_PAREN:
    {
      skip_newlines ();
      if (!token_is (TOK_CLOSE_PAREN))
      {
        operand res = parse_expression (true);
        token_after_newlines_must_be (TOK_CLOSE_PAREN);
        return res;
      }
      /* FALLTHRU */
    }
    default:
    {
      EMIT_ERROR_VARG ("Unknown token %s", lexer_token_type_to_string (tok.type));
    }
  }
}

/* member_expression
  : (primary_expression | function_expression | 'new' LT!* member_expression (LT!* '(' LT!* arguments? LT!* ')')
      (LT!* member_expression_suffix)*
  ;

   arguments
  : assignment_expression (LT!* ',' LT!* assignment_expression)*)?
  ;

   member_expression_suffix
  : index_suffix
  | property_reference_suffix
  ;

   index_suffix
  : '[' LT!* expression LT!* ']'
  ;

   property_reference_suffix
  : '.' LT!* Identifier
  ; */
static operand
parse_member_expression (operand *this_arg, operand *prop_gl)
{
  operand expr;
  if (is_keyword (KW_FUNCTION))
  {
    expr = parse_function_expression ();
  }
  else if (is_keyword (KW_NEW))
  {
    skip_newlines ();
    expr = parse_member_expression (this_arg, prop_gl);

    skip_newlines ();
    if (token_is (TOK_OPEN_PAREN))
    {
      expr = parse_argument_list (VARG_CONSTRUCT_EXPR, expr, NULL, NULL);
    }
    else
    {
      lexer_save_token (tok);
      dump_varg_header_for_rewrite (VARG_CONSTRUCT_EXPR, expr);
      expr = rewrite_varg_header_set_args_count (0);
    }
  }
  else
  {
    expr = parse_primary_expression ();
  }

  skip_newlines ();
  while (token_is (TOK_OPEN_SQUARE) || token_is (TOK_DOT))
  {
    operand prop = empty_operand ();

    if (token_is (TOK_OPEN_SQUARE))
    {
      skip_newlines ();
      prop = parse_expression (true);
      next_token_must_be (TOK_CLOSE_SQUARE);
    }
    else if (token_is (TOK_DOT))
    {
      skip_newlines ();
      if (token_is (TOK_NAME))
      {
        prop = dump_string_assignment_res (token_data ());
      }
      else if (token_is (TOK_KEYWORD))
      {
        const literal lit = create_literal_from_str_compute_len (lexer_keyword_to_string ((keyword) token_data ()));
        const literal_index_t lit_id = lexer_lookup_literal_uid (lit);
        if (lit_id == INVALID_LITERAL)
        {
          EMIT_ERROR ("Expected identifier");
        }
        prop = dump_string_assignment_res (lit_id);
      }
      else
      {
        EMIT_ERROR ("Expected identifier");
      }
    }
    skip_newlines ();

    if (this_arg)
    {
      *this_arg = expr;
    }
    if (prop_gl)
    {
      *prop_gl = prop;
    }
    expr = dump_prop_getter_res (expr, prop);
  }

  lexer_save_token (tok);
  return expr;
}

/* call_expression
  : member_expression LT!* arguments (LT!* call_expression_suffix)*
  ;

   call_expression_suffix
  : arguments
  | index_suffix
  | property_reference_suffix
  ;

   arguments
  : '(' LT!* assignment_expression LT!* (',' LT!* assignment_expression * LT!*)* ')'
  ; */
static operand
parse_call_expression (operand *this_arg_gl, operand *prop_gl)
{
  operand this_arg = empty_operand ();
  operand expr = parse_member_expression (&this_arg, prop_gl);
  operand prop;

  skip_newlines ();
  if (!token_is (TOK_OPEN_PAREN))
  {
    lexer_save_token (tok);
    if (this_arg_gl != NULL)
    {
      *this_arg_gl = this_arg;
    }
    return expr;
  }

  expr = parse_argument_list (VARG_CALL_EXPR, expr, NULL, &this_arg);
  this_arg = empty_operand ();

  skip_newlines ();
  while (token_is (TOK_OPEN_PAREN) || token_is (TOK_OPEN_SQUARE)
         || token_is (TOK_DOT))
  {
    if (tok.type == TOK_OPEN_PAREN)
    {
      expr = parse_argument_list (VARG_CALL_EXPR, expr, NULL, &this_arg);
      skip_newlines ();
    }
    else
    {
      this_arg = expr;
      if (tok.type == TOK_OPEN_SQUARE)
      {
        skip_newlines ();
        prop = parse_expression (true);
        next_token_must_be (TOK_CLOSE_SQUARE);
      }
      else if (tok.type == TOK_DOT)
      {
        token_after_newlines_must_be (TOK_NAME);
        prop = dump_string_assignment_res (token_data ());
      }
      expr = dump_prop_getter_res (expr, prop);
      skip_newlines ();
    }
  }
  lexer_save_token (tok);
  if (this_arg_gl != NULL)
  {
    *this_arg_gl = this_arg;
  }
  if (prop_gl != NULL)
  {
    *prop_gl = prop;
  }
  return expr;
}

/* left_hand_side_expression
  : call_expression
  | new_expression
  ; */
static operand
parse_left_hand_side_expression (operand *this_arg, operand *prop)
{
  return parse_call_expression (this_arg, prop);
}

/* postfix_expression
  : left_hand_side_expression ('++' | '--')?
  ; */
static operand
parse_postfix_expression (void)
{
  operand this_arg = empty_operand (), prop = empty_operand ();
  operand expr = parse_left_hand_side_expression (&this_arg, &prop);

  if (lexer_prev_token ().type == TOK_NEWLINE)
  {
    return expr;
  }

  syntax_check_for_eval_and_arguments_in_strict_mode (expr, is_strict_mode (), tok.loc);

  skip_token ();
  if (token_is (TOK_DOUBLE_PLUS))
  {
    const operand res = dump_post_increment_res (expr);
    if (!operand_is_empty (this_arg) && !operand_is_empty (prop))
    {
      dump_prop_setter (this_arg, prop, expr);
    }
    expr = res;
  }
  else if (token_is (TOK_DOUBLE_MINUS))
  {
    const operand res = dump_post_decrement_res (expr);
    if (!operand_is_empty (this_arg) && !operand_is_empty (prop))
    {
      dump_prop_setter (this_arg, prop, expr);
    }
    expr = res;
  }
  else
  {
    lexer_save_token (tok);
  }

  return expr;
}

/* unary_expression
  : postfix_expression
  | ('delete' | 'void' | 'typeof' | '++' | '--' | '+' | '-' | '~' | '!') unary_expression
  ; */
static operand
parse_unary_expression (operand *this_arg_gl, operand *prop_gl)
{
  operand expr, this_arg = empty_operand (), prop = empty_operand ();
  switch (tok.type)
  {
    case TOK_DOUBLE_PLUS:
    {
      skip_newlines ();
      expr = parse_unary_expression (&this_arg, &prop);
      syntax_check_for_eval_and_arguments_in_strict_mode (expr, is_strict_mode (), tok.loc);
      expr = dump_pre_increment_res (expr);
      if (!operand_is_empty (this_arg) && !operand_is_empty (prop))
      {
        dump_prop_setter (this_arg, prop, expr);
      }
      break;
    }
    case TOK_DOUBLE_MINUS:
    {
      skip_newlines ();
      expr = parse_unary_expression (&this_arg, &prop);
      syntax_check_for_eval_and_arguments_in_strict_mode (expr, is_strict_mode (), tok.loc);
      expr = dump_pre_decrement_res (expr);
      if (!operand_is_empty (this_arg) && !operand_is_empty (prop))
      {
        dump_prop_setter (this_arg, prop, expr);
      }
      break;
    }
    case TOK_PLUS:
    {
      skip_newlines ();
      expr = parse_unary_expression (NULL, NULL);
      expr = dump_unary_plus_res (expr);
      break;
    }
    case TOK_MINUS:
    {
      skip_newlines ();
      expr = parse_unary_expression (NULL, NULL);
      expr = dump_unary_minus_res (expr);
      break;
    }
    case TOK_COMPL:
    {
      skip_newlines ();
      expr = parse_unary_expression (NULL, NULL);
      expr = dump_bitwise_not_res (expr);
      break;
    }
    case TOK_NOT:
    {
      skip_newlines ();
      expr = parse_unary_expression (NULL, NULL);
      expr = dump_logical_not_res (expr);
      break;
    }
    case TOK_KEYWORD:
    {
      if (is_keyword (KW_DELETE))
      {
        skip_newlines ();
        expr = parse_unary_expression (NULL, NULL);
        expr = dump_delete_res (expr, is_strict_mode (), tok.loc);
        break;
      }
      else if (is_keyword (KW_VOID))
      {
        skip_newlines ();
        expr = parse_unary_expression (NULL, NULL);
        expr = dump_variable_assignment_res (expr);
        dump_undefined_assignment (expr);
        break;
      }
      else if (is_keyword (KW_TYPEOF))
      {
        skip_newlines ();
        expr = parse_unary_expression (NULL, NULL);
        expr = dump_typeof_res (expr);
        break;
      }
      /* FALLTHRU.  */
    }
    default:
    {
      expr = parse_postfix_expression ();
    }
  }

  if (this_arg_gl != NULL)
  {
    *this_arg_gl = this_arg;
  }
  if (prop_gl != NULL)
  {
    *prop_gl = prop;
  }

  return expr;
}

static operand
dump_assignment_of_lhs_if_literal (operand lhs)
{
  if (lhs.type == OPERAND_LITERAL)
  {
    lhs = dump_variable_assignment_res (lhs);
  }
  return lhs;
}

/* multiplicative_expression
  : unary_expression (LT!* ('*' | '/' | '%') LT!* unary_expression)*
  ; */
static operand
parse_multiplicative_expression (void)
{
  operand expr = parse_unary_expression (NULL, NULL);

  skip_newlines ();
  while (true)
  {
    switch (tok.type)
    {
      case TOK_MULT:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_multiplication_res (expr, parse_unary_expression (NULL, NULL));
        break;
      }
      case TOK_DIV:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_division_res (expr, parse_unary_expression (NULL, NULL));
        break;
      }
      case TOK_MOD:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_remainder_res (expr, parse_unary_expression (NULL, NULL));
        break;
      }
      default:
      {
        lexer_save_token (tok);
        goto done;
      }
    }
    skip_newlines ();
  }
done:
  return expr;
}

/* additive_expression
  : multiplicative_expression (LT!* ('+' | '-') LT!* multiplicative_expression)*
  ; */
static operand
parse_additive_expression (void)
{
  operand expr = parse_multiplicative_expression ();

  skip_newlines ();
  while (true)
  {
    switch (tok.type)
    {
      case TOK_PLUS:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_addition_res (expr, parse_multiplicative_expression ());
        break;
      }
      case TOK_MINUS:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_substraction_res (expr, parse_multiplicative_expression ());
        break;
      }
      default:
      {
        lexer_save_token (tok);
        goto done;
      }
    }
    skip_newlines ();
  }
done:
  return expr;
}

/* shift_expression
  : additive_expression (LT!* ('<<' | '>>' | '>>>') LT!* additive_expression)*
  ; */
static operand
parse_shift_expression (void)
{
  operand expr = parse_additive_expression ();

  skip_newlines ();
  while (true)
  {
    switch (tok.type)
    {
      case TOK_LSHIFT:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_left_shift_res (expr, parse_additive_expression ());
        break;
      }
      case TOK_RSHIFT:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_right_shift_res (expr, parse_additive_expression ());
        break;
      }
      case TOK_RSHIFT_EX:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_right_shift_ex_res (expr, parse_additive_expression ());
        break;
      }
      default:
      {
        lexer_save_token (tok);
        goto done;
      }
    }
    skip_newlines ();
  }
done:
  return expr;
}

/* relational_expression
  : shift_expression (LT!* ('<' | '>' | '<=' | '>=' | 'instanceof' | 'in') LT!* shift_expression)*
  ; */
static operand
parse_relational_expression (bool in_allowed)
{
  operand expr = parse_shift_expression ();

  skip_newlines ();
  while (true)
  {
    switch (tok.type)
    {
      case TOK_LESS:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_less_than_res (expr, parse_shift_expression ());
        break;
      }
      case TOK_GREATER:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_greater_than_res (expr, parse_shift_expression ());
        break;
      }
      case TOK_LESS_EQ:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_less_or_equal_than_res (expr, parse_shift_expression ());
        break;
      }
      case TOK_GREATER_EQ:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_greater_or_equal_than_res (expr, parse_shift_expression ());
        break;
      }
      case TOK_KEYWORD:
      {
        if (is_keyword (KW_INSTANCEOF))
        {
          expr = dump_assignment_of_lhs_if_literal (expr);
          skip_newlines ();
          expr = dump_instanceof_res (expr, parse_shift_expression ());
          break;
        }
        else if (is_keyword (KW_IN))
        {
          if (in_allowed)
          {
            expr = dump_assignment_of_lhs_if_literal (expr);
            skip_newlines ();
            expr = dump_in_res (expr, parse_shift_expression ());
            break;
          }
        }
        /* FALLTHROUGH */
      }
      default:
      {
        lexer_save_token (tok);
        goto done;
      }
    }
    skip_newlines ();
  }
done:
  return expr;
}

/* equality_expression
  : relational_expression (LT!* ('==' | '!=' | '===' | '!==') LT!* relational_expression)*
  ; */
static operand
parse_equality_expression (bool in_allowed)
{
  operand expr = parse_relational_expression (in_allowed);

  skip_newlines ();
  while (true)
  {
    switch (tok.type)
    {
      case TOK_DOUBLE_EQ:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_equal_value_res (expr, parse_relational_expression (in_allowed));
        break;
      }
      case TOK_NOT_EQ:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_not_equal_value_res (expr, parse_relational_expression (in_allowed));
        break;
      }
      case TOK_TRIPLE_EQ:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_equal_value_type_res (expr, parse_relational_expression (in_allowed));
        break;
      }
      case TOK_NOT_DOUBLE_EQ:
      {
        expr = dump_assignment_of_lhs_if_literal (expr);
        skip_newlines ();
        expr = dump_not_equal_value_type_res (expr, parse_relational_expression (in_allowed));
        break;
      }
      default:
      {
        lexer_save_token (tok);
        goto done;
      }
    }
    skip_newlines ();
  }
done:
  return expr;
}

/* bitwise_and_expression
  : equality_expression (LT!* '&' LT!* equality_expression)*
  ; */
static operand
parse_bitwise_and_expression (bool in_allowed)
{
  operand expr = parse_equality_expression (in_allowed);
  skip_newlines ();
  while (true)
  {
    if (tok.type == TOK_AND)
    {
      expr = dump_assignment_of_lhs_if_literal (expr);
      skip_newlines ();
      expr = dump_bitwise_and_res (expr, parse_equality_expression (in_allowed));
    }
    else
    {
      lexer_save_token (tok);
      goto done;
    }
    skip_newlines ();
  }
done:
  return expr;
}

/* bitwise_xor_expression
  : bitwise_and_expression (LT!* '^' LT!* bitwise_and_expression)*
  ; */
static operand
parse_bitwise_xor_expression (bool in_allowed)
{
  operand expr = parse_bitwise_and_expression (in_allowed);
  skip_newlines ();
  while (true)
  {
    if (tok.type == TOK_XOR)
    {
      expr = dump_assignment_of_lhs_if_literal (expr);
      skip_newlines ();
      expr = dump_bitwise_xor_res (expr, parse_bitwise_and_expression (in_allowed));
    }
    else
    {
      lexer_save_token (tok);
      goto done;
    }
    skip_newlines ();
  }
done:
  return expr;
}

/* bitwise_or_expression
  : bitwise_xor_expression (LT!* '|' LT!* bitwise_xor_expression)*
  ; */
static operand
parse_bitwise_or_expression (bool in_allowed)
{
  operand expr = parse_bitwise_xor_expression (in_allowed);
  skip_newlines ();
  while (true)
  {
    if (tok.type == TOK_OR)
    {
      expr = dump_assignment_of_lhs_if_literal (expr);
      skip_newlines ();
      expr = dump_bitwise_or_res (expr, parse_bitwise_xor_expression (in_allowed));
    }
    else
    {
      lexer_save_token (tok);
      goto done;
    }
    skip_newlines ();
  }
done:
  return expr;
}

/* logical_and_expression
  : bitwise_or_expression (LT!* '&&' LT!* bitwise_or_expression)*
  ; */
static operand
parse_logical_and_expression (bool in_allowed)
{
  operand expr = parse_bitwise_or_expression (in_allowed), tmp;
  skip_newlines ();
  if (token_is (TOK_DOUBLE_AND))
  {
    tmp = dump_variable_assignment_res (expr);
    start_dumping_logical_and_checks ();
    dump_logical_and_check_for_rewrite (tmp);
  }
  else
  {
    lexer_save_token (tok);
    return expr;
  }
  while (token_is (TOK_DOUBLE_AND))
  {
    skip_newlines ();
    expr = parse_bitwise_or_expression (in_allowed);
    dump_variable_assignment (tmp, expr);
    skip_newlines ();
    if (token_is (TOK_DOUBLE_AND))
    {
      dump_logical_and_check_for_rewrite (tmp);
    }
  }
  lexer_save_token (tok);
  rewrite_logical_and_checks ();
  return tmp;
}

/* logical_or_expression
  : logical_and_expression (LT!* '||' LT!* logical_and_expression)*
  ; */
static operand
parse_logical_or_expression (bool in_allowed)
{
  operand expr = parse_logical_and_expression (in_allowed), tmp;
  skip_newlines ();
  if (token_is (TOK_DOUBLE_OR))
  {
    tmp = dump_variable_assignment_res (expr);
    start_dumping_logical_or_checks ();
    dump_logical_or_check_for_rewrite (tmp);
  }
  else
  {
    lexer_save_token (tok);
    return expr;
  }
  while (token_is (TOK_DOUBLE_OR))
  {
    skip_newlines ();
    expr = parse_logical_and_expression (in_allowed);
    dump_variable_assignment (tmp, expr);
    skip_newlines ();
    if (token_is (TOK_DOUBLE_OR))
    {
      dump_logical_or_check_for_rewrite (tmp);
    }
  }
  lexer_save_token (tok);
  rewrite_logical_or_checks ();
  return tmp;
}

/* conditional_expression
  : logical_or_expression (LT!* '?' LT!* assignment_expression LT!* ':' LT!* assignment_expression)?
  ; */
static operand
parse_conditional_expression (bool in_allowed, bool *is_conditional)
{
  operand expr = parse_logical_or_expression (in_allowed);
  skip_newlines ();
  if (token_is (TOK_QUERY))
  {
    dump_conditional_check_for_rewrite (expr);
    skip_newlines ();
    expr = parse_assignment_expression (in_allowed);
    operand tmp = dump_variable_assignment_res (expr);
    token_after_newlines_must_be (TOK_COLON);
    dump_jump_to_end_for_rewrite ();
    rewrite_conditional_check ();
    skip_newlines ();
    expr = parse_assignment_expression (in_allowed);
    dump_variable_assignment (tmp, expr);
    rewrite_jump_to_end ();
    if (is_conditional != NULL)
    {
      *is_conditional = true;
    }
    return tmp;
  }
  else
  {
    lexer_save_token (tok);
    return expr;
  }
}

/* assignment_expression
  : conditional_expression
  | left_hand_side_expression LT!* assignment_operator LT!* assignment_expression
  ; */
static operand
parse_assignment_expression (bool in_allowed)
{
  bool is_conditional = false;
  operand expr = parse_conditional_expression (in_allowed, &is_conditional);
  if (is_conditional)
  {
    return expr;
  }
  syntax_check_for_eval_and_arguments_in_strict_mode (expr, is_strict_mode (), tok.loc);
  skip_newlines ();
  switch (tok.type)
  {
    case TOK_EQ:
    {
      skip_newlines ();
      start_dumping_assignment_expression ();
      const operand assign_expr = parse_assignment_expression (in_allowed);
      expr = dump_prop_setter_or_variable_assignment_res (expr, assign_expr);
      break;
    }
    case TOK_MULT_EQ:
    {
      skip_newlines ();
      start_dumping_assignment_expression ();
      const operand assign_expr = parse_assignment_expression (in_allowed);
      expr = dump_prop_setter_or_multiplication_res (expr, assign_expr);
      break;
    }
    case TOK_DIV_EQ:
    {
      skip_newlines ();
      start_dumping_assignment_expression ();
      const operand assign_expr = parse_assignment_expression (in_allowed);
      expr = dump_prop_setter_or_division_res (expr, assign_expr);
      break;
    }
    case TOK_MOD_EQ:
    {
      skip_newlines ();
      start_dumping_assignment_expression ();
      const operand assign_expr = parse_assignment_expression (in_allowed);
      expr = dump_prop_setter_or_remainder_res (expr, assign_expr);
      break;
    }
    case TOK_PLUS_EQ:
    {
      skip_newlines ();
      start_dumping_assignment_expression ();
      const operand assign_expr = parse_assignment_expression (in_allowed);
      expr = dump_prop_setter_or_addition_res (expr, assign_expr);
      break;
    }
    case TOK_MINUS_EQ:
    {
      skip_newlines ();
      start_dumping_assignment_expression ();
      const operand assign_expr = parse_assignment_expression (in_allowed);
      expr = dump_prop_setter_or_substraction_res (expr, assign_expr);
      break;
    }
    case TOK_LSHIFT_EQ:
    {
      skip_newlines ();
      start_dumping_assignment_expression ();
      const operand assign_expr = parse_assignment_expression (in_allowed);
      expr = dump_prop_setter_or_left_shift_res (expr, assign_expr);
      break;
    }
    case TOK_RSHIFT_EQ:
    {
      skip_newlines ();
      start_dumping_assignment_expression ();
      const operand assign_expr = parse_assignment_expression (in_allowed);
      expr = dump_prop_setter_or_right_shift_res (expr, assign_expr);
      break;
    }
    case TOK_RSHIFT_EX_EQ:
    {
      skip_newlines ();
      start_dumping_assignment_expression ();
      const operand assign_expr = parse_assignment_expression (in_allowed);
      expr = dump_prop_setter_or_right_shift_ex_res (expr, assign_expr);
      break;
    }
    case TOK_AND_EQ:
    {
      skip_newlines ();
      start_dumping_assignment_expression ();
      const operand assign_expr = parse_assignment_expression (in_allowed);
      expr = dump_prop_setter_or_bitwise_and_res (expr, assign_expr);
      break;
    }
    case TOK_XOR_EQ:
    {
      skip_newlines ();
      start_dumping_assignment_expression ();
      const operand assign_expr = parse_assignment_expression (in_allowed);
      expr = dump_prop_setter_or_bitwise_xor_res (expr, assign_expr);
      break;
    }
    case TOK_OR_EQ:
    {
      skip_newlines ();
      start_dumping_assignment_expression ();
      const operand assign_expr = parse_assignment_expression (in_allowed);
      expr = dump_prop_setter_or_bitwise_or_res (expr, assign_expr);
      break;
    }
    default:
    {
      lexer_save_token (tok);
      break;
    }
  }
  return expr;
}

/* expression
  : assignment_expression (LT!* ',' LT!* assignment_expression)*
  ;
 */
static operand
parse_expression (bool in_allowed)
{
  operand expr = parse_assignment_expression (in_allowed);

  while (true)
  {
    skip_newlines ();
    if (token_is (TOK_COMMA))
    {
      skip_newlines ();
      expr = parse_assignment_expression (in_allowed);
    }
    else
    {
      lexer_save_token (tok);
      break;
    }
  }
  return expr;
}

/* variable_declaration
  : Identifier LT!* initialiser?
  ;
   initialiser
  : '=' LT!* assignment_expression
  ; */
static void
parse_variable_declaration (void)
{
  current_token_must_be (TOK_NAME);
  const operand name = literal_operand (token_data ());

  skip_newlines ();
  if (token_is (TOK_EQ))
  {
    skip_newlines ();
    const operand expr = parse_assignment_expression (true);
    dump_variable_assignment (name, expr);
  }
  else
  {
    lexer_save_token (tok);
  }
}

/* variable_declaration_list
  : variable_declaration
    (LT!* ',' LT!* variable_declaration)*
  ; */
static void
parse_variable_declaration_list (bool *several_decls)
{
  while (true)
  {
    parse_variable_declaration ();

    skip_newlines ();
    if (!token_is (TOK_COMMA))
    {
      lexer_save_token (tok);
      return;
    }

    skip_newlines ();
    if (several_decls)
    {
      *several_decls = true;
    }
  }
}

static void
parse_plain_for (void)
{
  /* Represent loop like

     for (i = 0; i < 10; i++) {
       body;
     }

     as

  assign i, 0
  jmp_down %cond
%body
  body
  post_incr i
%cond
  less_than i, 10
  is_true_jmp_up %body

  */

  dump_jump_to_end_for_rewrite ();

  // Skip till body
  JERRY_ASSERT (token_is (TOK_SEMICOLON));
  skip_newlines ();
  const locus cond_loc = tok.loc;
  while (!token_is (TOK_SEMICOLON))
  {
    skip_newlines ();
  }
  skip_newlines ();
  const locus incr_loc = tok.loc;
  while (!token_is (TOK_CLOSE_PAREN))
  {
    skip_newlines ();
  }

  start_collecting_continues ();
  start_collecting_breaks ();
  dumper_set_next_interation_target ();

  // Parse body
  skip_newlines ();
  push_nesting (NESTING_ITERATIONAL);
  parse_statement ();
  pop_nesting (NESTING_ITERATIONAL);

  const locus end_loc = tok.loc;

  dumper_set_continue_target ();

  lexer_seek (incr_loc);
  skip_token ();
  if (!token_is (TOK_CLOSE_PAREN))
  {
    parse_expression (true);
  }

  rewrite_jump_to_end ();

  lexer_seek (cond_loc);
  skip_token ();
  if (token_is (TOK_SEMICOLON))
  {
    dump_continue_iterations_check (empty_operand ());
  }
  else
  {
    const operand cond = parse_expression (true);
    dump_continue_iterations_check (cond);
  }

  dumper_set_break_target ();
  rewrite_breaks ();
  rewrite_continues ();

  lexer_seek (end_loc);
  skip_token ();
  if (tok.type != TOK_CLOSE_BRACE)
  {
    lexer_save_token (tok);
  }
}

static void
parse_for_in (void)
{
  EMIT_SORRY ("'for in' loops are not supported yet");
}

/* for_statement
  : 'for' LT!* '(' (LT!* for_statement_initialiser_part)? LT!* ';'
    (LT!* expression)? LT!* ';' (LT!* expression)? LT!* ')' LT!* statement
  ;

   for_statement_initialiser_part
  : expression
  | 'var' LT!* variable_declaration_list
  ;

   for_in_statement
  : 'for' LT!* '(' LT!* for_in_statement_initialiser_part LT!* 'in'
    LT!* expression LT!* ')' LT!* statement
  ;

   for_in_statement_initialiser_part
  : left_hand_side_expression
  | 'var' LT!* variable_declaration
  ;*/

static void
parse_for_or_for_in_statement (void)
{
  assert_keyword (KW_FOR);
  token_after_newlines_must_be (TOK_OPEN_PAREN);

  skip_newlines ();
  if (token_is (TOK_SEMICOLON))
  {
    parse_plain_for ();
    return;
  }
  /* Both for_statement_initialiser_part and for_in_statement_initialiser_part
     contains 'var'. Check it first.  */
  if (is_keyword (KW_VAR))
  {
    bool several_decls = false;
    skip_newlines ();
    parse_variable_declaration_list (&several_decls);
    if (several_decls)
    {
      token_after_newlines_must_be (TOK_SEMICOLON);
      parse_plain_for ();
      return;
    }
    else
    {
      skip_newlines ();
      if (token_is (TOK_SEMICOLON))
      {
        parse_plain_for ();
        return;
      }
      else if (is_keyword (KW_IN))
      {
        parse_for_in ();
        return;
      }
      else
      {
        EMIT_ERROR ("Expected either ';' or 'in' token");
      }
    }
  }

  /* expression contains left_hand_side_expression.  */
  parse_expression (false);

  skip_newlines ();
  if (token_is (TOK_SEMICOLON))
  {
    parse_plain_for ();
    return;
  }
  else if (is_keyword (KW_IN))
  {
    parse_for_in ();
    return;
  }
  else
  {
    EMIT_ERROR ("Expected either ';' or 'in' token");
  }
}

static operand
parse_expression_inside_parens (void)
{
  token_after_newlines_must_be (TOK_OPEN_PAREN);
  skip_newlines ();
  const operand res = parse_expression (true);
  token_after_newlines_must_be (TOK_CLOSE_PAREN);
  return res;
}

/* statement_list
  : statement (LT!* statement)*
  ; */
static void
parse_statement_list (void)
{
  while (true)
  {
    parse_statement ();

    skip_newlines ();
    while (token_is (TOK_SEMICOLON))
    {
      skip_newlines ();
    }
    if (token_is (TOK_CLOSE_BRACE))
    {
      lexer_save_token (tok);
      break;
    }
    if (is_keyword (KW_CASE) || is_keyword (KW_DEFAULT))
    {
      lexer_save_token (tok);
      break;
    }
  }
}

/* if_statement
  : 'if' LT!* '(' LT!* expression LT!* ')' LT!* statement (LT!* 'else' LT!* statement)?
  ; */
static void
parse_if_statement (void)
{
  assert_keyword (KW_IF);

  const operand cond = parse_expression_inside_parens ();
  dump_conditional_check_for_rewrite (cond);

  skip_newlines ();
  parse_statement ();

  skip_newlines ();
  if (is_keyword (KW_ELSE))
  {
    dump_jump_to_end_for_rewrite ();
    rewrite_conditional_check ();

    skip_newlines ();
    parse_statement ();

    rewrite_jump_to_end ();
  }
  else
  {
    lexer_save_token (tok);
    rewrite_conditional_check ();
  }
}

/* do_while_statement
  : 'do' LT!* statement LT!* 'while' LT!* '(' expression ')' (LT | ';')!
  ; */
static void
parse_do_while_statement (void)
{
  assert_keyword (KW_DO);

  start_collecting_continues ();
  start_collecting_breaks ();

  dumper_set_next_interation_target ();

  skip_newlines ();
  push_nesting (NESTING_ITERATIONAL);
  parse_statement ();
  pop_nesting (NESTING_ITERATIONAL);

  dumper_set_continue_target ();

  token_after_newlines_must_be_keyword (KW_WHILE);
  const operand cond = parse_expression_inside_parens ();
  dump_continue_iterations_check (cond);

  dumper_set_break_target ();

  rewrite_breaks ();
  rewrite_continues ();
}

/* while_statement
  : 'while' LT!* '(' LT!* expression LT!* ')' LT!* statement
  ; */
static void
parse_while_statement (void)
{
  assert_keyword (KW_WHILE);

  token_after_newlines_must_be (TOK_OPEN_PAREN);
  const locus cond_loc = tok.loc;
  skip_parens ();

  dump_jump_to_end_for_rewrite ();

  dumper_set_next_interation_target ();

  start_collecting_continues ();
  start_collecting_breaks ();

  skip_newlines ();
  push_nesting (NESTING_ITERATIONAL);
  parse_statement ();
  pop_nesting (NESTING_ITERATIONAL);

  dumper_set_continue_target ();

  rewrite_jump_to_end ();

  const locus end_loc = tok.loc;
  lexer_seek (cond_loc);
  const operand cond = parse_expression_inside_parens ();
  dump_continue_iterations_check (cond);

  dumper_set_break_target ();

  rewrite_breaks ();
  rewrite_continues ();

  lexer_seek (end_loc);
  skip_token ();
}

/* with_statement
  : 'with' LT!* '(' LT!* expression LT!* ')' LT!* statement
  ; */
static void
parse_with_statement (void)
{
  assert_keyword (KW_WITH);
  if (is_strict_mode ())
  {
    EMIT_ERROR ("'with' expression is not allowed in strict mode.");
  }
  const operand expr = parse_expression_inside_parens ();
  dump_with (expr);
  skip_newlines ();
  parse_statement ();
  dump_with_end ();
}

static void
skip_case_clause_body (void)
{
  while (!is_keyword (KW_CASE) && !is_keyword (KW_DEFAULT) && !token_is (TOK_CLOSE_BRACE))
  {
    skip_newlines ();
    if (token_is (TOK_OPEN_BRACE))
    {
      skip_braces ();
    }
  }
}

/* switch_statement
  : 'switch' LT!* '(' LT!* expression LT!* ')' LT!* '{' LT!* case_block LT!* '}'
  ;
   case_block
  : '{' LT!* case_clause* LT!* '}'
  | '{' LT!* case_clause* LT!* default_clause LT!* case_clause* LT!* '}'
  ;
   case_clause
  : 'case' LT!* expression LT!* ':' LT!* statement*
  ; */
static void
parse_switch_statement (void)
{
  assert_keyword (KW_SWITCH);

  const operand switch_expr = parse_expression_inside_parens ();
  token_after_newlines_must_be (TOK_OPEN_BRACE);

  start_dumping_case_clauses ();
  const locus start_loc = tok.loc;
  bool was_default = false;
  // First, generate table of jumps
  skip_newlines ();
  while (is_keyword (KW_CASE) || is_keyword (KW_DEFAULT))
  {
    if (is_keyword (KW_CASE))
    {
      skip_newlines ();
      const operand case_expr = parse_expression (true);
      next_token_must_be (TOK_COLON);
      dump_case_clause_check_for_rewrite (switch_expr, case_expr);
      skip_newlines ();
      skip_case_clause_body ();
    }
    else if (is_keyword (KW_DEFAULT))
    {
      if (was_default)
      {
        EMIT_ERROR ("Duplication of 'default' clause");
      }
      was_default = true;
      token_after_newlines_must_be (TOK_COLON);
      skip_newlines ();
      skip_case_clause_body ();
    }
  }
  current_token_must_be (TOK_CLOSE_BRACE);

  if (was_default)
  {
    dump_default_clause_check_for_rewrite ();
  }

  lexer_seek (start_loc);
  next_token_must_be (TOK_OPEN_BRACE);

  start_collecting_breaks ();
  push_nesting (NESTING_SWITCH);
  // Second, parse case clauses' bodies and rewrite jumps
  skip_newlines ();
  while (is_keyword (KW_CASE) || is_keyword (KW_DEFAULT))
  {
    if (is_keyword (KW_CASE))
    {
      while (!token_is (TOK_COLON))
      {
        skip_newlines ();
      }
      rewrite_case_clause ();
      skip_newlines ();
      if (is_keyword (KW_CASE) || is_keyword (KW_DEFAULT))
      {
        continue;
      }
      parse_statement_list ();
    }
    else if (is_keyword (KW_DEFAULT))
    {
      token_after_newlines_must_be (TOK_COLON);
      skip_newlines ();
      if (is_keyword (KW_CASE) || is_keyword (KW_DEFAULT))
      {
        continue;
      }
      parse_statement_list ();
      continue;
    }
    skip_newlines ();
  }
  current_token_must_be (TOK_CLOSE_BRACE);
  skip_token ();
  pop_nesting (NESTING_SWITCH);

  // Finally, dump 'finally' jump
  if (was_default)
  {
    rewrite_default_clause ();
  }

  dumper_set_break_target ();
  rewrite_breaks ();
  finish_dumping_case_clauses ();
}

/* catch_clause
  : 'catch' LT!* '(' LT!* Identifier LT!* ')' LT!* '{' LT!* statement_list LT!* '}'
  ; */
static void
parse_catch_clause (void)
{
  assert_keyword (KW_CATCH);

  token_after_newlines_must_be (TOK_OPEN_PAREN);
  token_after_newlines_must_be (TOK_NAME);
  const operand exception = literal_operand (token_data ());
  syntax_check_for_eval_and_arguments_in_strict_mode (exception, is_strict_mode (), tok.loc);
  token_after_newlines_must_be (TOK_CLOSE_PAREN);

  dump_catch_for_rewrite (exception);

  token_after_newlines_must_be (TOK_OPEN_BRACE);
  skip_newlines ();
  parse_statement_list ();
  next_token_must_be (TOK_CLOSE_BRACE);

  rewrite_catch ();
}

/* finally_clause
  : 'finally' LT!* '{' LT!* statement_list LT!* '}'
  ; */
static void
parse_finally_clause (void)
{
  assert_keyword (KW_FINALLY);

  dump_finally_for_rewrite ();

  token_after_newlines_must_be (TOK_OPEN_BRACE);
  skip_newlines ();
  parse_statement_list ();
  next_token_must_be (TOK_CLOSE_BRACE);

  rewrite_finally ();
}

/* try_statement
  : 'try' LT!* '{' LT!* statement_list LT!* '}' LT!* (finally_clause | catch_clause (LT!* finally_clause)?)
  ; */
static void
parse_try_statement (void)
{
  assert_keyword (KW_TRY);

  dump_try_for_rewrite ();

  token_after_newlines_must_be (TOK_OPEN_BRACE);
  skip_newlines ();
  parse_statement_list ();
  next_token_must_be (TOK_CLOSE_BRACE);

  rewrite_try ();

  token_after_newlines_must_be (TOK_KEYWORD);
  if (is_keyword (KW_CATCH))
  {
    parse_catch_clause ();

    skip_newlines ();
    if (is_keyword (KW_FINALLY))
    {
      parse_finally_clause ();
    }
    else
    {
      lexer_save_token (tok);
    }
  }
  else if (is_keyword (KW_FINALLY))
  {
    parse_finally_clause ();
  }
  else
  {
    EMIT_ERROR ("Expected either 'catch' or 'finally' token");
  }

  dump_end_try_catch_finally ();
}

static void
insert_semicolon (void)
{
  // We cannot use tok, since we may use lexer_save_token
  skip_token ();
  if (token_is (TOK_NEWLINE) || lexer_prev_token ().type == TOK_NEWLINE)
  {
    lexer_save_token (tok);
    return;
  }
  if (token_is (TOK_CLOSE_BRACE))
  {
    lexer_save_token (tok);
    return;
  }
  else if (!token_is (TOK_SEMICOLON))
  {
    EMIT_ERROR ("Expected either ';' or newline token");
  }
}

/* statement
  : statement_block
  | variable_statement
  | empty_statement
  | if_statement
  | iteration_statement
  | continue_statement
  | break_statement
  | return_statement
  | with_statement
  | labelled_statement
  | switch_statement
  | throw_statement
  | try_statement
  | expression_statement
  ;

   statement_block
  : '{' LT!* statement_list? LT!* '}'
  ;

   variable_statement
  : 'var' LT!* variable_declaration_list (LT | ';')!
  ;

   empty_statement
  : ';'
  ;

   expression_statement
  : expression (LT | ';')!
  ;

   iteration_statement
  : do_while_statement
  | while_statement
  | for_statement
  | for_in_statement
  ;

   continue_statement
  : 'continue' Identifier? (LT | ';')!
  ;

   break_statement
  : 'break' Identifier? (LT | ';')!
  ;

   return_statement
  : 'return' expression? (LT | ';')!
  ;

   switchStatement
  : 'switch' LT!* '(' LT!* expression LT!* ')' LT!* caseBlock
  ;

   throw_statement
  : 'throw' expression (LT | ';')!
  ;

   try_statement
  : 'try' LT!* '{' LT!* statement_list LT!* '}' LT!* (finally_clause | catch_clause (LT!* finally_clause)?)
  ;*/
static void
parse_statement (void)
{
  dumper_new_statement ();

  if (token_is (TOK_CLOSE_BRACE))
  {
    lexer_save_token (tok);
    return;
  }
  if (token_is (TOK_OPEN_BRACE))
  {
    skip_newlines ();
    if (!token_is (TOK_CLOSE_BRACE))
    {
      parse_statement_list ();
      next_token_must_be (TOK_CLOSE_BRACE);
    }
    return;
  }
  if (is_keyword (KW_VAR))
  {
    skip_newlines ();
    parse_variable_declaration_list (NULL);
    return;
  }
  if (is_keyword (KW_FUNCTION))
  {
    parse_function_declaration ();
    return;
  }
  if (token_is (TOK_SEMICOLON))
  {
    return;
  }
  if (is_keyword (KW_CASE) || is_keyword (KW_DEFAULT))
  {
    return;
  }
  if (is_keyword (KW_IF))
  {
    parse_if_statement ();
    return;
  }
  if (is_keyword (KW_DO))
  {
    parse_do_while_statement ();
    return;
  }
  if (is_keyword (KW_WHILE))
  {
    parse_while_statement ();
    return;
  }
  if (is_keyword (KW_FOR))
  {
    parse_for_or_for_in_statement ();
    return;
  }
  if (is_keyword (KW_CONTINUE))
  {
    must_be_inside_but_not_in (NESTING_FUNCTION, 1, NESTING_ITERATIONAL);
    dump_continue_for_rewrite ();
    return;
  }
  if (is_keyword (KW_BREAK))
  {
    must_be_inside_but_not_in (NESTING_FUNCTION, 2, NESTING_ITERATIONAL, NESTING_SWITCH);
    dump_break_for_rewrite ();
    return;
  }
  if (is_keyword (KW_RETURN))
  {
    skip_token ();
    if (!token_is (TOK_SEMICOLON) && !token_is (TOK_NEWLINE))
    {
      const operand op = parse_expression (true);
      dump_retval (op);
      insert_semicolon ();
      return;
    }
    else
    {
      dump_ret ();
      return;
    }
  }
  if (is_keyword (KW_WITH))
  {
    parse_with_statement ();
    return;
  }
  if (is_keyword (KW_SWITCH))
  {
    parse_switch_statement ();
    return;
  }
  if (is_keyword (KW_THROW))
  {
    skip_token ();
    const operand op = parse_expression (true);
    insert_semicolon ();
    dump_throw (op);
    return;
  }
  if (is_keyword (KW_TRY))
  {
    parse_try_statement ();
    return;
  }
  if (token_is (TOK_NAME))
  {
    const token temp = tok;
    skip_newlines ();
    if (token_is (TOK_COLON))
    {
      // STMT_LABELLED;
      EMIT_SORRY ("Labelled statements are not supported yet");
    }
    else
    {
      lexer_save_token (tok);
      tok = temp;
      parse_expression (true);
      skip_newlines ();
      if (!token_is (TOK_SEMICOLON))
      {
        lexer_save_token (tok);
      }
      return;
    }
  }
  else
  {
    parse_expression (true);
    skip_newlines ();
    if (!token_is (TOK_SEMICOLON))
    {
      lexer_save_token (tok);
    }
    return;
  }
}

/* source_element
  : function_declaration
  | statement
  ; */
static void
parse_source_element (void)
{
  if (is_keyword (KW_FUNCTION))
  {
    parse_function_declaration ();
  }
  else
  {
    parse_statement ();
  }
}

static void
skip_optional_name_and_parens (void)
{
  if (token_is (TOK_NAME))
  {
    token_after_newlines_must_be (TOK_OPEN_PAREN);
  }
  else
  {
    current_token_must_be (TOK_OPEN_PAREN);
  }

  while (!token_is (TOK_CLOSE_PAREN))
  {
    skip_newlines ();
  }
}

static void process_keyword_names ()
{
  if (token_is (TOK_KEYWORD))
  {
    keyword kw = (keyword) token_data ();
    skip_newlines ();
    if (token_is (TOK_COLON))
    {
      lexer_add_keyword_or_numeric_literal_if_not_present (
        create_literal_from_str_compute_len (lexer_keyword_to_string (kw)));
    }
    else
    {
      lexer_save_token (tok);
    }
  }
  else if (token_is (TOK_NAME))
  {
    if (literal_equal_type_s (lexer_get_literal_by_id (token_data ()), "get")
        || literal_equal_type_s (lexer_get_literal_by_id (token_data ()), "set"))
    {
      skip_newlines ();
      if (token_is (TOK_KEYWORD))
      {
        keyword kw = (keyword) token_data ();
        skip_newlines ();
        if (token_is (TOK_OPEN_PAREN))
        {
          lexer_add_keyword_or_numeric_literal_if_not_present (
            create_literal_from_str_compute_len (lexer_keyword_to_string (kw)));
        }
        else
        {
          lexer_save_token (tok);
        }
      }
      else
      {
        lexer_save_token (tok);
      }
    }
  }
}

static void
skip_braces (void)
{
  current_token_must_be (TOK_OPEN_BRACE);

  uint8_t nesting_level = 1;
  while (nesting_level > 0)
  {
    skip_newlines ();
    if (token_is (TOK_OPEN_BRACE))
    {
      nesting_level++;
    }
    else if (token_is (TOK_CLOSE_BRACE))
    {
      nesting_level--;
    }
    else
    {
      process_keyword_names ();
    }
  }
}

static void
skip_function (void)
{
  skip_newlines ();
  skip_optional_name_and_parens ();
  skip_newlines ();
  skip_braces ();
}

static void
skip_squares (void)
{
  current_token_must_be (TOK_OPEN_SQUARE);

  uint8_t nesting_level = 1;
  while (nesting_level > 0)
  {
    skip_newlines ();
    if (token_is (TOK_OPEN_SQUARE))
    {
      nesting_level++;
    }
    else if (token_is (TOK_CLOSE_SQUARE))
    {
      nesting_level--;
    }
  }
}

static void
skip_parens (void)
{
  current_token_must_be (TOK_OPEN_PAREN);

  uint8_t nesting_level = 1;
  while (nesting_level > 0)
  {
    skip_newlines ();
    if (token_is (TOK_OPEN_PAREN))
    {
      nesting_level++;
    }
    else if (token_is (TOK_CLOSE_PAREN))
    {
      nesting_level--;
    }
  }
}

static bool
var_declared (literal_index_t var_id)
{
  return dumper_variable_declaration_exists (var_id);
}

static void
preparse_var_decls (void)
{
  assert_keyword (KW_VAR);

  skip_newlines ();
  while (!token_is (TOK_NEWLINE) && !token_is (TOK_SEMICOLON) && !is_keyword (KW_IN))
  {
    if (token_is (TOK_NAME))
    {
      if (!var_declared (token_data ()))
      {
        syntax_check_for_eval_and_arguments_in_strict_mode (literal_operand (token_data ()),
                                                            is_strict_mode (),
                                                            tok.loc);
        dump_variable_declaration (token_data ());
      }
      skip_token ();
      continue;
    }
    else if (token_is (TOK_EQ))
    {
      while (!token_is (TOK_COMMA) && !token_is (TOK_NEWLINE) && !token_is (TOK_SEMICOLON))
      {
        if (is_keyword (KW_FUNCTION))
        {
          skip_function ();
        }
        else if (token_is (TOK_OPEN_BRACE))
        {
          skip_braces ();
        }
        else if (token_is (TOK_OPEN_SQUARE))
        {
          skip_squares ();
        }
        else if (token_is (TOK_OPEN_PAREN))
        {
          skip_parens ();
        }
        skip_token ();
      }
    }
    else if (!token_is (TOK_COMMA))
    {
      EMIT_ERROR ("Expected ','");
    }
    else
    {
      skip_token ();
      continue;
    }
  }
}

static void
preparse_scope (bool is_global)
{
  const locus start_loc = tok.loc;
  const token_type end_tt = is_global ? TOK_EOF : TOK_CLOSE_BRACE;

  if (token_is (TOK_STRING) && literal_equal_s (lexer_get_literal_by_id (token_data ()), "use strict"))
  {
    scopes_tree_set_strict_mode (STACK_TOP (scopes), true);
    dump_strict_mode_header ();
  }

  lexer_set_strict_mode (scopes_tree_strict_mode (STACK_TOP (scopes)));

  dump_reg_var_decl_for_rewrite ();

  size_t nesting_level = 0;
  while (nesting_level > 0 || !token_is (end_tt))
  {
    if (token_is (TOK_OPEN_BRACE))
    {
      nesting_level++;
    }
    else if (token_is (TOK_CLOSE_BRACE))
    {
      nesting_level--;
    }
    else if (is_keyword (KW_VAR))
    {
      preparse_var_decls ();
    }
    else if (is_keyword (KW_FUNCTION))
    {
      skip_function ();
    }
    else if (token_is (TOK_OPEN_BRACE))
    {
      skip_braces ();
    }
    else
    {
      process_keyword_names ();
    }
    skip_newlines ();
  }

  if (start_loc != tok.loc)
  {
    lexer_seek (start_loc);
  }
  else
  {
    lexer_save_token (tok);
  }
}

/* source_element_list
  : source_element (LT!* source_element)*
  ; */
static void
parse_source_element_list (bool is_global)
{
  dumper_new_scope ();
  preparse_scope (is_global);

  skip_newlines ();
  while (!token_is (TOK_EOF) && !token_is (TOK_CLOSE_BRACE))
  {
    parse_source_element ();
    skip_newlines ();
  }
  lexer_save_token (tok);
  rewrite_reg_var_decl ();
  dumper_finish_scope ();
}

/* program
  : LT!* source_element_list LT!* EOF!
  ; */
void
parser_parse_program (void)
{
  STACK_PUSH (scopes, scopes_tree_init (NULL));
  serializer_set_scope (STACK_TOP (scopes));
  lexer_set_strict_mode (scopes_tree_strict_mode (STACK_TOP (scopes)));

  skip_newlines ();
  parse_source_element_list (true);

  skip_newlines ();
  JERRY_ASSERT (token_is (TOK_EOF));
  dump_exit ();

  serializer_dump_literals (lexer_get_literals (), lexer_get_literals_count ());
  serializer_merge_scopes_into_bytecode ();
  serializer_set_scope (NULL);
  serializer_set_strings_buffer (lexer_get_strings_cache ());

  scopes_tree_free (STACK_TOP (scopes));
  STACK_DROP (scopes, 1);
}

void
parser_init (const char *source, size_t source_size, bool show_opcodes)
{
  lexer_init (source, source_size, show_opcodes);
  serializer_set_show_opcodes (show_opcodes);
  dumper_init ();
  syntax_init ();

  STACK_INIT (nestings);
  STACK_INIT (scopes);
}

void
parser_free (void)
{
  STACK_FREE (nestings);
  STACK_FREE (scopes);

  syntax_free ();
  dumper_free ();
  lexer_free ();
}

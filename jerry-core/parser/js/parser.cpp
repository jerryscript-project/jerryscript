/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#include "ecma-helpers.h"
#include "hash-table.h"
#include "jrt-libc-includes.h"
#include "jsp-label.h"
#include "jsp-mm.h"
#include "opcodes.h"
#include "opcodes-dumper.h"
#include "parser.h"
#include "re-parser.h"
#include "scopes-tree.h"
#include "serializer.h"
#include "stack.h"
#include "jsp-early-error.h"
#include "vm.h"

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
static bool inside_eval = false;
static bool inside_function = false;
static bool parser_show_instrs = false;
/**
 * flag, indicating that code contains function declarations or function expressions
 */
static bool code_contains_functions = false;

enum
{
  scopes_global_size
};
STATIC_STACK (scopes, scopes_tree)

#define EMIT_ERROR(type, MESSAGE) PARSE_ERROR(type, MESSAGE, tok.loc)
#define EMIT_ERROR_VARG(type, MESSAGE, ...) PARSE_ERROR_VARG(type, MESSAGE, tok.loc, __VA_ARGS__)

static operand parse_expression (bool, jsp_eval_ret_store_t);
static void parse_statement (jsp_label_t *outermost_stmt_label_p);
static operand parse_assignment_expression (bool);
static void parse_source_element_list (bool);
static operand parse_argument_list (varg_list_type, operand, operand *);

static bool
token_is (token_type tt)
{
  return tok.type == tt;
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
skip_token (void)
{
  tok = lexer_next_token ();
}

static void
assert_keyword (keyword kw)
{
  if (!token_is (TOK_KEYWORD) || token_data () != kw)
  {
    EMIT_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX, "Expected keyword '%s'", lexer_keyword_to_string (kw));
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
    EMIT_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX, "Expected '%s' token", lexer_token_type_to_string (tt));
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
    EMIT_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX, "Expected '%s' token", lexer_token_type_to_string (tt));
  }
}

static void
token_after_newlines_must_be (token_type tt)
{
  skip_newlines ();
  if (!token_is (tt))
  {
    EMIT_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX, "Expected '%s' token", lexer_token_type_to_string (tt));
  }
}

static void
token_after_newlines_must_be_keyword (keyword kw)
{
  skip_newlines ();
  if (!is_keyword (kw))
  {
    EMIT_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX, "Expected keyword '%s'", lexer_keyword_to_string (kw));
  }
}

static bool
is_strict_mode (void)
{
  return scopes_tree_strict_mode (STACK_TOP (scopes));
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
jsp_skip_braces (token_type brace_type) /**< type of the opening brace */
{
  current_token_must_be (brace_type);

  token_type closing_bracket_type;

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

  skip_newlines ();

  while (!token_is (closing_bracket_type)
         && !token_is (TOK_EOF))
  {
    if (token_is (TOK_OPEN_PAREN)
        || token_is (TOK_OPEN_BRACE)
        || token_is (TOK_OPEN_SQUARE))
    {
      jsp_skip_braces (tok.type);
    }

    skip_newlines ();
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
jsp_find_next_token_before_the_locus (token_type token_to_find, /**< token to search for
                                                                 *   (except TOK_NEWLINE and TOK_EOF) */
                                      locus end_loc, /**< location to search before */
                                      bool skip_brace_blocks) /**< skip blocks, surrounded with { and } braces */
{
  JERRY_ASSERT (token_to_find != TOK_NEWLINE
                && token_to_find != TOK_EOF);

  while (lit_utf8_iterator_pos_cmp (tok.loc, end_loc) < 0)
  {
    if (skip_brace_blocks)
    {
      if (token_is (TOK_OPEN_BRACE))
      {
        jsp_skip_braces (TOK_OPEN_BRACE);

        JERRY_ASSERT (token_is (TOK_CLOSE_BRACE));
        skip_newlines ();

        if (lit_utf8_iterator_pos_cmp (tok.loc, end_loc) >= 0)
        {
          lexer_seek (end_loc);
          tok = lexer_next_token ();

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

    skip_newlines ();
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
static operand
parse_property_name (void)
{
  switch (tok.type)
  {
    case TOK_NAME:
    case TOK_STRING:
    case TOK_NUMBER:
    {
      return literal_operand (token_data_as_lit_cp ());
    }
    case TOK_SMALL_INT:
    {
      literal_t lit = lit_find_or_create_literal_from_num ((ecma_number_t) token_data ());
      return literal_operand (lit_cpointer_t::compress (lit));
    }
    case TOK_KEYWORD:
    {
      const char *s = lexer_keyword_to_string ((keyword) token_data ());
      literal_t lit = lit_find_or_create_literal_from_utf8_string ((const lit_utf8_byte_t *) s,
                                                                   (lit_utf8_size_t)strlen (s));
      return literal_operand (lit_cpointer_t::compress (lit));
    }
    case TOK_NULL:
    case TOK_BOOL:
    {
      lit_magic_string_id_t id = (token_is (TOK_NULL)
                                  ? LIT_MAGIC_STRING_NULL
                                  : (tok.uid ? LIT_MAGIC_STRING_TRUE : LIT_MAGIC_STRING_FALSE));
      literal_t lit = lit_find_or_create_literal_from_utf8_string (lit_get_magic_string_utf8 (id),
                                                                   lit_get_magic_string_size (id));
      return literal_operand (lit_cpointer_t::compress (lit));
    }
    default:
    {
      EMIT_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX, "Wrong property name type: %s", lexer_token_type_to_string (tok.type));
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
  jsp_early_error_add_prop_name (name, PROP_DATA);
}

/* property_assignment
  : property_name_and_value
  | get LT!* property_name LT!* '(' LT!* ')' LT!* '{' LT!* function_body LT!* '}'
  | set LT!* property_name LT!* '(' identifier ')' LT!* '{' LT!* function_body LT!* '}'
  ; */
static void
parse_property_assignment (void)
{
  if (token_is (TOK_NAME))
  {
    bool is_setter;

    if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (token_data_as_lit_cp ()), "get"))
    {
      is_setter = false;
    }
    else if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (token_data_as_lit_cp ()), "set"))
    {
      is_setter = true;
    }
    else
    {
      parse_property_name_and_value ();

      return;
    }

    const token temp = tok;
    skip_newlines ();
    if (token_is (TOK_COLON))
    {
      lexer_save_token (tok);
      tok = temp;

      parse_property_name_and_value ();

      return;
    }

    code_contains_functions = true;

    STACK_DECLARE_USAGE (scopes);

    const operand name = parse_property_name ();
    jsp_early_error_add_prop_name (name, is_setter ? PROP_SET : PROP_GET);

    STACK_PUSH (scopes, scopes_tree_init (NULL));
    serializer_set_scope (STACK_TOP (scopes));
    scopes_tree_set_strict_mode (STACK_TOP (scopes), scopes_tree_strict_mode (STACK_HEAD (scopes, 2)));
    lexer_set_strict_mode (scopes_tree_strict_mode (STACK_TOP (scopes)));

    skip_newlines ();
    const operand func = parse_argument_list (VARG_FUNC_EXPR, empty_operand (), NULL);

    dump_function_end_for_rewrite ();

    token_after_newlines_must_be (TOK_OPEN_BRACE);
    skip_newlines ();

    bool was_in_function = inside_function;
    inside_function = true;

    jsp_label_t *masked_label_set_p = jsp_label_mask_set ();

    parse_source_element_list (false);

    jsp_label_restore_set (masked_label_set_p);

    token_after_newlines_must_be (TOK_CLOSE_BRACE);

    dump_ret ();
    rewrite_function_end ();

    inside_function = was_in_function;

    scopes_tree fe_scope_tree = STACK_TOP (scopes);

    STACK_DROP (scopes, 1);
    serializer_set_scope (STACK_TOP (scopes));
    lexer_set_strict_mode (scopes_tree_strict_mode (STACK_TOP (scopes)));

    serializer_dump_subscope (fe_scope_tree);
    scopes_tree_free (fe_scope_tree);

    STACK_CHECK_USAGE (scopes);

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
}

/** Parse list of identifiers, assigment expressions or properties, splitted by comma.
    For each ALT dumps appropriate bytecode. Uses OBJ during dump if neccesary.
    Result tmp. */
static operand
parse_argument_list (varg_list_type vlt, operand obj, operand *this_arg_p)
{
  token_type close_tt = TOK_CLOSE_PAREN;
  size_t args_num = 0;

  JERRY_ASSERT (!(vlt != VARG_CALL_EXPR && this_arg_p != NULL));

  switch (vlt)
  {
    case VARG_FUNC_DECL:
    case VARG_FUNC_EXPR:
    case VARG_CONSTRUCT_EXPR:
    {
      current_token_must_be (TOK_OPEN_PAREN);
      dump_varg_header_for_rewrite (vlt, obj);
      break;
    }
    case VARG_CALL_EXPR:
    {
      current_token_must_be (TOK_OPEN_PAREN);

      opcode_call_flags_t call_flags = OPCODE_CALL_FLAGS__EMPTY;

      operand this_arg = empty_operand ();
      if (this_arg_p != NULL
          && !operand_is_empty (*this_arg_p))
      {
        call_flags = (opcode_call_flags_t) (call_flags | OPCODE_CALL_FLAGS_HAVE_THIS_ARG);

        if (this_arg_p->type == OPERAND_LITERAL)
        {
          /*
           * FIXME:
           *       Base of CallExpression should be evaluated only once during evaluation of CallExpression
           *
           * See also:
           *          Evaluation of MemberExpression (ECMA-262 v5, 11.2.1)
           */
          this_arg = dump_variable_assignment_res (*this_arg_p);
        }
        else
        {
          this_arg = *this_arg_p;
        }

        /*
         * Presence of explicit 'this' argument implies that it is not Direct call to Eval
         *
         * See also:
         *          ECMA-262 v5, 15.2.2.1
         */
      }
      else if (dumper_is_eval_literal (obj))
      {
        call_flags = (opcode_call_flags_t) (call_flags | OPCODE_CALL_FLAGS_DIRECT_CALL_TO_EVAL_FORM);
      }
      else
      {
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
      }

      dump_varg_header_for_rewrite (vlt, obj);

      if (call_flags != OPCODE_CALL_FLAGS__EMPTY)
      {
        if (call_flags & OPCODE_CALL_FLAGS_HAVE_THIS_ARG)
        {
          JERRY_ASSERT (!operand_is_empty (this_arg));
          dump_call_additional_info (call_flags, this_arg);
        }
        else
        {
          dump_call_additional_info (call_flags, empty_operand ());
        }
      }

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
      jsp_early_error_start_checking_of_prop_names ();
      break;
    }
  }

  skip_newlines ();
  while (!token_is (close_tt))
  {
    dumper_start_varg_code_sequence ();

    operand op;

    if (vlt == VARG_FUNC_DECL
        || vlt == VARG_FUNC_EXPR)
    {
      current_token_must_be (TOK_NAME);
      op = literal_operand (token_data_as_lit_cp ());
      jsp_early_error_add_varg (op);
      jsp_early_error_check_for_eval_and_arguments_in_strict_mode (op, is_strict_mode (), tok.loc);
      dump_varg (op);
      skip_newlines ();
    }
    else if (vlt == VARG_CONSTRUCT_EXPR
             || vlt == VARG_CALL_EXPR)
    {
      op = parse_assignment_expression (true);
      dump_varg (op);
      skip_newlines ();
    }
    else if (vlt == VARG_ARRAY_DECL)
    {
      if (token_is (TOK_COMMA))
      {
        op = dump_undefined_assignment_res ();
        dump_varg (op);
      }
      else
      {
        op = parse_assignment_expression (true);
        dump_varg (op);
        skip_newlines ();
      }
    }
    else
    {
      JERRY_ASSERT (vlt == VARG_OBJ_DECL);

      parse_property_assignment ();
      skip_newlines ();
    }

    if (token_is (TOK_COMMA))
    {
      skip_newlines ();
    }
    else
    {
      current_token_must_be (close_tt);
    }

    args_num++;

    dumper_finish_varg_code_sequence ();
  }

  operand res;
  switch (vlt)
  {
    case VARG_FUNC_DECL:
    case VARG_FUNC_EXPR:
    {
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
      jsp_early_error_check_for_duplication_of_prop_names (is_strict_mode (), tok.loc);
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

  code_contains_functions = true;

  assert_keyword (KW_FUNCTION);

  jsp_label_t *masked_label_set_p = jsp_label_mask_set ();

  token_after_newlines_must_be (TOK_NAME);
  const operand name = literal_operand (token_data_as_lit_cp ());

  jsp_early_error_check_for_eval_and_arguments_in_strict_mode (name, is_strict_mode (), tok.loc);

  skip_newlines ();
  STACK_PUSH (scopes, scopes_tree_init (STACK_TOP (scopes)));
  serializer_set_scope (STACK_TOP (scopes));
  scopes_tree_set_strict_mode (STACK_TOP (scopes), scopes_tree_strict_mode (STACK_HEAD (scopes, 2)));
  lexer_set_strict_mode (scopes_tree_strict_mode (STACK_TOP (scopes)));

  jsp_early_error_start_checking_of_vargs ();
  parse_argument_list (VARG_FUNC_DECL, name, NULL);

  dump_function_end_for_rewrite ();

  token_after_newlines_must_be (TOK_OPEN_BRACE);
  skip_newlines ();

  bool was_in_function = inside_function;
  inside_function = true;

  parse_source_element_list (false);

  next_token_must_be (TOK_CLOSE_BRACE);

  dump_ret ();
  rewrite_function_end ();

  inside_function = was_in_function;

  jsp_early_error_check_for_syntax_errors_in_formal_param_list (is_strict_mode (), tok.loc);

  STACK_DROP (scopes, 1);
  serializer_set_scope (STACK_TOP (scopes));
  lexer_set_strict_mode (scopes_tree_strict_mode (STACK_TOP (scopes)));

  jsp_label_restore_set (masked_label_set_p);

  STACK_CHECK_USAGE (scopes);
}

/* function_expression
  : 'function' LT!* Identifier? LT!* '(' formal_parameter_list? LT!* ')' LT!* function_body
  ; */
static operand
parse_function_expression (void)
{
  STACK_DECLARE_USAGE (scopes);
  assert_keyword (KW_FUNCTION);

  code_contains_functions = true;

  operand res;

  jsp_early_error_start_checking_of_vargs ();

  STACK_PUSH (scopes, scopes_tree_init (NULL));
  serializer_set_scope (STACK_TOP (scopes));
  scopes_tree_set_strict_mode (STACK_TOP (scopes), scopes_tree_strict_mode (STACK_HEAD (scopes, 2)));
  lexer_set_strict_mode (scopes_tree_strict_mode (STACK_TOP (scopes)));

  skip_newlines ();
  if (token_is (TOK_NAME))
  {
    const operand name = literal_operand (token_data_as_lit_cp ());
    jsp_early_error_check_for_eval_and_arguments_in_strict_mode (name, is_strict_mode (), tok.loc);

    skip_newlines ();
    res = parse_argument_list (VARG_FUNC_EXPR, name, NULL);
  }
  else
  {
    lexer_save_token (tok);
    skip_newlines ();
    res = parse_argument_list (VARG_FUNC_EXPR, empty_operand (), NULL);
  }

  dump_function_end_for_rewrite ();

  token_after_newlines_must_be (TOK_OPEN_BRACE);
  skip_newlines ();

  bool was_in_function = inside_function;
  inside_function = true;

  jsp_label_t *masked_label_set_p = jsp_label_mask_set ();

  parse_source_element_list (false);

  jsp_label_restore_set (masked_label_set_p);


  next_token_must_be (TOK_CLOSE_BRACE);

  dump_ret ();
  rewrite_function_end ();

  inside_function = was_in_function;

  jsp_early_error_check_for_syntax_errors_in_formal_param_list (is_strict_mode (), tok.loc);

  serializer_set_scope (STACK_HEAD (scopes, 2));
  serializer_dump_subscope (STACK_TOP (scopes));
  scopes_tree_free (STACK_TOP (scopes));
  STACK_DROP (scopes, 1);
  lexer_set_strict_mode (scopes_tree_strict_mode (STACK_TOP (scopes)));

  STACK_CHECK_USAGE (scopes);
  return res;
} /* parse_function_expression */

/* array_literal
  : '[' LT!* assignment_expression? (LT!* ',' (LT!* assignment_expression)?)* LT!* ']' LT!*
  ; */
static operand
parse_array_literal (void)
{
  return parse_argument_list (VARG_ARRAY_DECL, empty_operand (), NULL);
}

/* object_literal
  : '{' LT!* property_assignment (LT!* ',' LT!* property_assignment)* LT!* '}'
  ; */
static operand
parse_object_literal (void)
{
  return parse_argument_list (VARG_OBJ_DECL, empty_operand (), NULL);
}

/* literal
  : 'null'
  | 'true'
  | 'false'
  | number_literal
  | string_literal
  | regexp_literal
  ; */
static operand
parse_literal (void)
{
  switch (tok.type)
  {
    case TOK_NUMBER: return dump_number_assignment_res (token_data_as_lit_cp ());
    case TOK_STRING: return dump_string_assignment_res (token_data_as_lit_cp ());
    case TOK_REGEXP: return dump_regexp_assignment_res (token_data_as_lit_cp ());
    case TOK_NULL: return dump_null_assignment_res ();
    case TOK_BOOL: return dump_boolean_assignment_res ((bool) token_data ());
    case TOK_SMALL_INT: return dump_smallint_assignment_res ((idx_t) token_data ());
    default:
    {
      EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Expected literal");
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
    case TOK_REGEXP:
    case TOK_STRING: return parse_literal ();
    case TOK_NAME:
    {
      if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (token_data_as_lit_cp ()), "arguments"))
      {
        scopes_tree_set_arguments_used (STACK_TOP (scopes));
      }
      if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (token_data_as_lit_cp ()), "eval"))
      {
        scopes_tree_set_eval_used (STACK_TOP (scopes));
      }
      return literal_operand (token_data_as_lit_cp ());
    }
    case TOK_OPEN_SQUARE: return parse_array_literal ();
    case TOK_OPEN_BRACE: return parse_object_literal ();
    case TOK_OPEN_PAREN:
    {
      skip_newlines ();
      if (!token_is (TOK_CLOSE_PAREN))
      {
        operand res = parse_expression (true, JSP_EVAL_RET_STORE_NOT_DUMP);
        token_after_newlines_must_be (TOK_CLOSE_PAREN);
        return res;
      }
      /* FALLTHRU */
    }
    default:
    {
      EMIT_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX, "Unknown token %s", lexer_token_type_to_string (tok.type));
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
      expr = parse_argument_list (VARG_CONSTRUCT_EXPR, expr, NULL);
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
      prop = parse_expression (true, JSP_EVAL_RET_STORE_NOT_DUMP);
      next_token_must_be (TOK_CLOSE_SQUARE);
    }
    else if (token_is (TOK_DOT))
    {
      skip_newlines ();
      if (token_is (TOK_NAME))
      {
        prop = dump_string_assignment_res (token_data_as_lit_cp ());
      }
      else if (token_is (TOK_KEYWORD))
      {
        const char *s = lexer_keyword_to_string ((keyword) token_data ());
        literal_t lit = lit_find_or_create_literal_from_utf8_string ((lit_utf8_byte_t *) s,
                                                                     (lit_utf8_size_t) strlen (s));
        if (lit == NULL)
        {
          EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Expected identifier");
        }
        prop = dump_string_assignment_res (lit_cpointer_t::compress (lit));
      }
      else if (token_is (TOK_BOOL) || token_is (TOK_NULL))
      {
        lit_magic_string_id_t id = (token_is (TOK_NULL)
                                    ? LIT_MAGIC_STRING_NULL
                                    : (tok.uid ? LIT_MAGIC_STRING_TRUE : LIT_MAGIC_STRING_FALSE));
        literal_t lit = lit_find_or_create_literal_from_utf8_string (lit_get_magic_string_utf8 (id),
                                                                     lit_get_magic_string_size (id));
        prop = dump_string_assignment_res (lit_cpointer_t::compress (lit));
      }
      else
      {
        EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Expected identifier");
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

  expr = parse_argument_list (VARG_CALL_EXPR, expr, &this_arg);
  this_arg = empty_operand ();

  skip_newlines ();
  while (token_is (TOK_OPEN_PAREN) || token_is (TOK_OPEN_SQUARE)
         || token_is (TOK_DOT))
  {
    if (tok.type == TOK_OPEN_PAREN)
    {
      expr = parse_argument_list (VARG_CALL_EXPR, expr, &this_arg);
      skip_newlines ();
    }
    else
    {
      this_arg = expr;
      if (tok.type == TOK_OPEN_SQUARE)
      {
        skip_newlines ();
        prop = parse_expression (true, JSP_EVAL_RET_STORE_NOT_DUMP);
        next_token_must_be (TOK_CLOSE_SQUARE);
      }
      else if (tok.type == TOK_DOT)
      {
        token_after_newlines_must_be (TOK_NAME);
        prop = dump_string_assignment_res (token_data_as_lit_cp ());
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
parse_postfix_expression (operand *out_this_arg_gl_p, /**< out: if expression evaluates to object-based
                                                       *          reference - the reference's base;
                                                       *        otherwise - empty operand */
                          operand *out_prop_gl_p) /**< out: if expression evaluates to object-based
                                                   *          reference - the reference's name;
                                                   *        otherwise - empty operand */
{
  operand this_arg = empty_operand (), prop = empty_operand ();
  operand expr = parse_left_hand_side_expression (&this_arg, &prop);

  if (lexer_prev_token ().type == TOK_NEWLINE)
  {
    return expr;
  }

  skip_token ();
  if (token_is (TOK_DOUBLE_PLUS))
  {
    jsp_early_error_check_for_eval_and_arguments_in_strict_mode (expr, is_strict_mode (), tok.loc);

    const operand res = dump_post_increment_res (expr);
    if (!operand_is_empty (this_arg) && !operand_is_empty (prop))
    {
      dump_prop_setter (this_arg, prop, expr);
    }
    expr = res;
  }
  else if (token_is (TOK_DOUBLE_MINUS))
  {
    jsp_early_error_check_for_eval_and_arguments_in_strict_mode (expr, is_strict_mode (), tok.loc);

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

  if (out_this_arg_gl_p != NULL)
  {
    *out_this_arg_gl_p = this_arg;
  }

  if (out_prop_gl_p != NULL)
  {
    *out_prop_gl_p = prop;
  }

  return expr;
} /* parse_postfix_expression */

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
      jsp_early_error_check_for_eval_and_arguments_in_strict_mode (expr, is_strict_mode (), tok.loc);
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
      jsp_early_error_check_for_eval_and_arguments_in_strict_mode (expr, is_strict_mode (), tok.loc);
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
      expr = parse_postfix_expression (&this_arg, &prop);
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

  skip_newlines ();

  token_type tt = tok.type;

  if (tt == TOK_EQ
      || tt == TOK_MULT_EQ
      || tt == TOK_DIV_EQ
      || tt == TOK_MOD_EQ
      || tt == TOK_PLUS_EQ
      || tt == TOK_MINUS_EQ
      || tt == TOK_LSHIFT_EQ
      || tt == TOK_RSHIFT_EQ
      || tt == TOK_RSHIFT_EX_EQ
      || tt == TOK_AND_EQ
      || tt == TOK_XOR_EQ
      || tt == TOK_OR_EQ)
  {
    jsp_early_error_check_for_eval_and_arguments_in_strict_mode (expr, is_strict_mode (), tok.loc);
    skip_newlines ();
    start_dumping_assignment_expression ();
    const operand assign_expr = parse_assignment_expression (in_allowed);

    if (tt == TOK_EQ)
    {
      expr = dump_prop_setter_or_variable_assignment_res (expr, assign_expr);
    }
    else if (tt == TOK_MULT_EQ)
    {
      expr = dump_prop_setter_or_multiplication_res (expr, assign_expr);
    }
    else if (tt == TOK_DIV_EQ)
    {
      expr = dump_prop_setter_or_division_res (expr, assign_expr);
    }
    else if (tt == TOK_MOD_EQ)
    {
      expr = dump_prop_setter_or_remainder_res (expr, assign_expr);
    }
    else if (tt == TOK_PLUS_EQ)
    {
      expr = dump_prop_setter_or_addition_res (expr, assign_expr);
    }
    else if (tt == TOK_MINUS_EQ)
    {
      expr = dump_prop_setter_or_substraction_res (expr, assign_expr);
    }
    else if (tt == TOK_LSHIFT_EQ)
    {
      expr = dump_prop_setter_or_left_shift_res (expr, assign_expr);
    }
    else if (tt == TOK_RSHIFT_EQ)
    {
      expr = dump_prop_setter_or_right_shift_res (expr, assign_expr);
    }
    else if (tt == TOK_RSHIFT_EX_EQ)
    {
      expr = dump_prop_setter_or_right_shift_ex_res (expr, assign_expr);
    }
    else if (tt == TOK_AND_EQ)
    {
      expr = dump_prop_setter_or_bitwise_and_res (expr, assign_expr);
    }
    else if (tt == TOK_XOR_EQ)
    {
      expr = dump_prop_setter_or_bitwise_xor_res (expr, assign_expr);
    }
    else
    {
      JERRY_ASSERT (tt == TOK_OR_EQ);
      expr = dump_prop_setter_or_bitwise_or_res (expr, assign_expr);
    }
  }
  else
  {
    lexer_save_token (tok);
  }

  return expr;
}

/**
 * Parse an expression
 *
 * expression
 *  : assignment_expression (LT!* ',' LT!* assignment_expression)*
 *  ;
 *
 * @return operand which holds result of expression
 */
static operand
parse_expression (bool in_allowed, /**< flag indicating if 'in' is allowed inside expression to parse */
                  jsp_eval_ret_store_t dump_eval_ret_store) /**< flag indicating if 'eval'
                                                             *   result code store should be dumped */
{
  operand expr = parse_assignment_expression (in_allowed);

  while (true)
  {
    skip_newlines ();
    if (token_is (TOK_COMMA))
    {
      dump_assignment_of_lhs_if_literal (expr);

      skip_newlines ();
      expr = parse_assignment_expression (in_allowed);
    }
    else
    {
      lexer_save_token (tok);
      break;
    }
  }

  if (inside_eval
      && dump_eval_ret_store == JSP_EVAL_RET_STORE_DUMP
      && !inside_function)
  {
    dump_variable_assignment (eval_ret_operand () , expr);
  }

  return expr;
} /* parse_expression */

/* variable_declaration
  : Identifier LT!* initialiser?
  ;
   initialiser
  : '=' LT!* assignment_expression
  ; */
static operand
parse_variable_declaration (void)
{
  current_token_must_be (TOK_NAME);
  const operand name = literal_operand (token_data_as_lit_cp ());

  if (!dumper_variable_declaration_exists (token_data_as_lit_cp ()))
  {
    jsp_early_error_check_for_eval_and_arguments_in_strict_mode (literal_operand (token_data_as_lit_cp ()),
                                                                 is_strict_mode (),
                                                                 tok.loc);

    dump_variable_declaration (token_data_as_lit_cp ());
  }

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

  return name;
} /* parse_variable_declaration */

/* variable_declaration_list
  : variable_declaration
    (LT!* ',' LT!* variable_declaration)*
  ; */
static void
parse_variable_declaration_list (void)
{
  JERRY_ASSERT (is_keyword (KW_VAR));

  while (true)
  {
    skip_newlines ();

    parse_variable_declaration ();

    skip_newlines ();
    if (!token_is (TOK_COMMA))
    {
      lexer_save_token (tok);
      break;
    }
  }
}

/**
 * Parse for statement
 *
 * See also:
 *          ECMA-262 v5, 12.6.3
 *
 * Note:
 *      Syntax:
 *               Initializer                      Condition     Increment     Body      LoopEnd
 *       - for ([ExpressionNoIn];                [Expression]; [Expression]) Statement
 *       - for (var VariableDeclarationListNoIn; [Expression]; [Expression]) Statement
 *
 * Note:
 *      Layout of generated byte-code is the following:
 *                        Initializer ([ExpressionNoIn] / VariableDeclarationListNoIn)
 *                        Jump -> ConditionCheck
 *        NextIteration:
 *                        Body (Statement)
 *        ContinueTarget:
 *                        Increment ([Expression])
 *        ConditionCheck:
 *                        Condition ([Expression])
 *                        If Condition is evaluted to true, jump -> NextIteration
 */
static void
jsp_parse_for_statement (jsp_label_t *outermost_stmt_label_p, /**< outermost (first) label, corresponding to
                                                               *   the statement (or NULL, if there are no named
                                                               *   labels associated with the statement) */
                         locus for_body_statement_loc) /**< locus of loop body statement */
{
  current_token_must_be (TOK_OPEN_PAREN);
  skip_newlines ();

  // Initializer
  if (is_keyword (KW_VAR))
  {
    parse_variable_declaration_list ();
    skip_token ();
  }
  else if (!token_is (TOK_SEMICOLON))
  {
    parse_expression (false, JSP_EVAL_RET_STORE_NOT_DUMP);
    skip_token ();
  }
  else
  {
    // Initializer is empty
  }

  // Jump -> ConditionCheck
  dump_jump_to_end_for_rewrite ();

  dumper_set_next_interation_target ();

  current_token_must_be (TOK_SEMICOLON);
  skip_token ();

  // Save Condition locus
  const locus condition_loc = tok.loc;

  if (!jsp_find_next_token_before_the_locus (TOK_SEMICOLON,
                                             for_body_statement_loc,
                                             true))
  {
    EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Invalid for statement");
  }

  current_token_must_be (TOK_SEMICOLON);
  skip_token ();

  // Save Increment locus
  const locus increment_loc = tok.loc;

  // Body
  lexer_seek (for_body_statement_loc);
  skip_newlines ();

  parse_statement (NULL);

  // Save LoopEnd locus
  const locus loop_end_loc = tok.loc;

  // Setup ContinueTarget
  jsp_label_setup_continue_target (outermost_stmt_label_p,
                                   serializer_get_current_instr_counter ());

  // Increment
  lexer_seek (increment_loc);
  skip_newlines ();

  if (!token_is (TOK_CLOSE_PAREN))
  {
    parse_expression (true, JSP_EVAL_RET_STORE_NOT_DUMP);
  }

  current_token_must_be (TOK_CLOSE_PAREN);

  // Setup ConditionCheck
  rewrite_jump_to_end ();

  // Condition
  lexer_seek (condition_loc);
  skip_newlines ();

  if (token_is (TOK_SEMICOLON))
  {
    dump_continue_iterations_check (empty_operand ());
  }
  else
  {
    operand cond = parse_expression (true, JSP_EVAL_RET_STORE_NOT_DUMP);
    dump_continue_iterations_check (cond);
  }

  lexer_seek (loop_end_loc);
  skip_newlines ();
  if (tok.type != TOK_CLOSE_BRACE)
  {
    lexer_save_token (tok);
  }
} /* jsp_parse_for_statement */

/**
 * Parse VariableDeclarationNoIn / LeftHandSideExpression (iterator part) of for-in statement
 *
 * See also:
 *          jsp_parse_for_in_statement
 *
 * @return true - if iterator consists of base and property name,
 *         false - otherwise, iterator consists of an identifier name (without base).
 */
static bool
jsp_parse_for_in_statement_iterator (operand *base_p, /**< out: base value of member expression, if any,
                                                       *        empty operand - otherwise */
                                     operand *identifier_p) /**< out: property name (if base value is not empty),
                                                             *        identifier - otherwise */
{
  JERRY_ASSERT (base_p != NULL);
  JERRY_ASSERT (identifier_p != NULL);

  if (is_keyword (KW_VAR))
  {
    skip_newlines ();

    *base_p = empty_operand ();
    *identifier_p = parse_variable_declaration ();

    return false;
  }
  else
  {
    operand base, identifier;

    /*
     * FIXME:
     *       Remove evaluation of last part of identifier chain
     */
    operand i = parse_left_hand_side_expression (&base, &identifier);

    if (operand_is_empty (base))
    {
      *base_p = empty_operand ();
      *identifier_p = i;

      return false;
    }
    else
    {
      *base_p = base;
      *identifier_p = identifier;

      return true;
    }
  }
} /* jsp_parse_for_in_statement_iterator */

/**
 * Parse for-in statement
 *
 * See also:
 *          ECMA-262 v5, 12.6.4
 *
 * Note:
 *      Syntax:
 *                     Iterator                 Collection   Body     LoopEnd
 *       - for (    LeftHandSideExpression  in Expression) Statement
 *       - for (var VariableDeclarationNoIn in Expression) Statement
 *
 * Note:
 *      Layout of generate byte-code is the following:
 *                        tmp <- Collection (Expression)
 *                        for_in instruction (tmp, instruction counter of for-in end mark)
 *                         {
 *                          Assignment of OPCODE_REG_SPECIAL_FOR_IN_PROPERTY_NAME to
 *                          Iterator (VariableDeclarationNoIn / LeftHandSideExpression)
 *                         }
 *                         Body (Statement)
 *        ContinueTarget:
 *                        meta (OPCODE_META_TYPE_END_FOR_IN)
 */
static void
jsp_parse_for_in_statement (jsp_label_t *outermost_stmt_label_p, /**< outermost (first) label,
                                                                  *   corresponding to the statement
                                                                  *   (or NULL, if there are no name
                                                                  *   labels associated with the statement) */
                            locus for_body_statement_loc) /**< locus of loop body statement */
{
  bool is_raised = jsp_label_raise_nested_jumpable_border ();

  current_token_must_be (TOK_OPEN_PAREN);
  skip_newlines ();

  // Save Iterator location
  locus iterator_loc = tok.loc;

  while (lit_utf8_iterator_pos_cmp (tok.loc, for_body_statement_loc) < 0)
  {
    if (jsp_find_next_token_before_the_locus (TOK_KEYWORD,
                                              for_body_statement_loc,
                                              true))
    {
      if (is_keyword (KW_IN))
      {
        break;
      }
      else
      {
        skip_token ();
      }
    }
    else
    {
      EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Invalid for statement");
    }
  }

  JERRY_ASSERT (is_keyword (KW_IN));
  skip_newlines ();

  // Collection
  operand collection = parse_expression (true, JSP_EVAL_RET_STORE_NOT_DUMP);
  current_token_must_be (TOK_CLOSE_PAREN);
  skip_token ();

  // Dump for-in instruction
  vm_instr_counter_t for_in_oc = dump_for_in_for_rewrite (collection);

  // Dump assignment VariableDeclarationNoIn / LeftHandSideExpression <- OPCODE_REG_SPECIAL_FOR_IN_PROPERTY_NAME
  lexer_seek (iterator_loc);
  tok = lexer_next_token ();

  operand iterator_base, iterator_identifier, for_in_special_reg;
  for_in_special_reg = jsp_create_operand_for_in_special_reg ();

  if (jsp_parse_for_in_statement_iterator (&iterator_base, &iterator_identifier))
  {
    dump_prop_setter (iterator_base, iterator_identifier, for_in_special_reg);
  }
  else
  {
    JERRY_ASSERT (operand_is_empty (iterator_base));
    dump_variable_assignment (iterator_identifier, for_in_special_reg);
  }

  // Body
  lexer_seek (for_body_statement_loc);
  tok = lexer_next_token ();

  parse_statement (NULL);

  // Save LoopEnd locus
  const locus loop_end_loc = tok.loc;

  // Setup ContinueTarget
  jsp_label_setup_continue_target (outermost_stmt_label_p,
                                   serializer_get_current_instr_counter ());

  // Write position of for-in end to for_in instruction
  rewrite_for_in (for_in_oc);

  // Dump meta (OPCODE_META_TYPE_END_FOR_IN)
  dump_for_in_end ();

  lexer_seek (loop_end_loc);
  tok = lexer_next_token ();
  if (tok.type != TOK_CLOSE_BRACE)
  {
    lexer_save_token (tok);
  }

  if (is_raised)
  {
    jsp_label_remove_nested_jumpable_border ();
  }
} /* jsp_parse_for_in_statement */

/**
 * Parse for/for-in statements
 *
 * See also:
 *          ECMA-262 v5, 12.6.3 and 12.6.4
 */
static void
jsp_parse_for_or_for_in_statement (jsp_label_t *outermost_stmt_label_p) /**< outermost (first) label,
                                                                         *   corresponding to the statement
                                                                         *   (or NULL, if there are no name
                                                                         *   labels associated with the statement) */
{
  assert_keyword (KW_FOR);
  token_after_newlines_must_be (TOK_OPEN_PAREN);

  locus for_open_paren_loc, for_body_statement_loc;

  for_open_paren_loc = tok.loc;

  jsp_skip_braces (TOK_OPEN_PAREN);
  skip_newlines ();

  for_body_statement_loc = tok.loc;

  lexer_seek (for_open_paren_loc);
  tok = lexer_next_token ();

  bool is_plain_for = jsp_find_next_token_before_the_locus (TOK_SEMICOLON,
                                                            for_body_statement_loc,
                                                            true);
  lexer_seek (for_open_paren_loc);
  tok = lexer_next_token ();

  if (is_plain_for)
  {
    jsp_parse_for_statement (outermost_stmt_label_p, for_body_statement_loc);
  }
  else
  {
    jsp_parse_for_in_statement (outermost_stmt_label_p, for_body_statement_loc);
  }
} /* jsp_parse_for_or_for_in_statement */

static operand
parse_expression_inside_parens (void)
{
  token_after_newlines_must_be (TOK_OPEN_PAREN);
  skip_newlines ();
  const operand res = parse_expression (true, JSP_EVAL_RET_STORE_NOT_DUMP);
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
    parse_statement (NULL);

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
  parse_statement (NULL);

  skip_newlines ();
  if (is_keyword (KW_ELSE))
  {
    dump_jump_to_end_for_rewrite ();
    rewrite_conditional_check ();

    skip_newlines ();
    parse_statement (NULL);

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
parse_do_while_statement (jsp_label_t *outermost_stmt_label_p) /**< outermost (first) label, corresponding to
                                                                *   the statement (or NULL, if there are no named
                                                                *   labels associated with the statement) */
{
  assert_keyword (KW_DO);

  dumper_set_next_interation_target ();

  skip_newlines ();
  parse_statement (NULL);

  jsp_label_setup_continue_target (outermost_stmt_label_p,
                                   serializer_get_current_instr_counter ());

  token_after_newlines_must_be_keyword (KW_WHILE);
  const operand cond = parse_expression_inside_parens ();
  dump_continue_iterations_check (cond);
}

/* while_statement
  : 'while' LT!* '(' LT!* expression LT!* ')' LT!* statement
  ; */
static void
parse_while_statement (jsp_label_t *outermost_stmt_label_p) /**< outermost (first) label, corresponding to
                                                             *   the statement (or NULL, if there are no named
                                                             *   labels associated with the statement) */
{
  assert_keyword (KW_WHILE);

  token_after_newlines_must_be (TOK_OPEN_PAREN);
  const locus cond_loc = tok.loc;
  jsp_skip_braces (TOK_OPEN_PAREN);

  dump_jump_to_end_for_rewrite ();

  dumper_set_next_interation_target ();

  skip_newlines ();
  parse_statement (NULL);

  jsp_label_setup_continue_target (outermost_stmt_label_p,
                                   serializer_get_current_instr_counter ());

  rewrite_jump_to_end ();

  const locus end_loc = tok.loc;
  lexer_seek (cond_loc);
  const operand cond = parse_expression_inside_parens ();
  dump_continue_iterations_check (cond);

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
    EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "'with' expression is not allowed in strict mode.");
  }
  const operand expr = parse_expression_inside_parens ();

  bool is_raised = jsp_label_raise_nested_jumpable_border ();

  vm_instr_counter_t with_begin_oc = dump_with_for_rewrite (expr);
  skip_newlines ();
  parse_statement (NULL);
  rewrite_with (with_begin_oc);
  dump_with_end ();

  if (is_raised)
  {
    jsp_label_remove_nested_jumpable_border ();
  }
}

static void
skip_case_clause_body (void)
{
  while (!is_keyword (KW_CASE)
         && !is_keyword (KW_DEFAULT)
         && !token_is (TOK_CLOSE_BRACE))
  {
    if (token_is (TOK_OPEN_BRACE))
    {
      jsp_skip_braces (TOK_OPEN_BRACE);
    }
    skip_newlines ();
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
      const operand case_expr = parse_expression (true, JSP_EVAL_RET_STORE_NOT_DUMP);
      next_token_must_be (TOK_COLON);
      dump_case_clause_check_for_rewrite (switch_expr, case_expr);
      skip_newlines ();
      skip_case_clause_body ();
    }
    else if (is_keyword (KW_DEFAULT))
    {
      if (was_default)
      {
        EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Duplication of 'default' clause");
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

  jsp_label_t label;
  jsp_label_push (&label,
                  JSP_LABEL_TYPE_UNNAMED_BREAKS,
                  TOKEN_EMPTY_INITIALIZER);

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
      rewrite_default_clause ();
      if (is_keyword (KW_CASE))
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

  jsp_label_rewrite_jumps_and_pop (&label,
                                   serializer_get_current_instr_counter ());

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
  const operand exception = literal_operand (token_data_as_lit_cp ());
  jsp_early_error_check_for_eval_and_arguments_in_strict_mode (exception, is_strict_mode (), tok.loc);
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

  bool is_raised = jsp_label_raise_nested_jumpable_border ();

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
    EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Expected either 'catch' or 'finally' token");
  }

  dump_end_try_catch_finally ();

  if (is_raised)
  {
    jsp_label_remove_nested_jumpable_border ();
  }
}

static void
insert_semicolon (void)
{
  // We cannot use tok, since we may use lexer_save_token
  skip_token ();

  bool is_new_line_occured = (token_is (TOK_NEWLINE) || lexer_prev_token ().type == TOK_NEWLINE);
  bool is_close_brace_or_eof = (token_is (TOK_CLOSE_BRACE) || token_is (TOK_EOF));

  if (is_new_line_occured || is_close_brace_or_eof)
  {
    lexer_save_token (tok);
  }
  else if (!token_is (TOK_SEMICOLON) && !token_is (TOK_EOF))
  {
    EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Expected either ';' or newline token");
  }
}

/**
 * iteration_statement
 *  : do_while_statement
 *  | while_statement
 *  | for_statement
 *  | for_in_statement
 *  ;
 */
static void
parse_iterational_statement (jsp_label_t *outermost_named_stmt_label_p) /**< outermost (first) named label,
                                                                         *   corresponding to the statement
                                                                         *   (or NULL, if there are no named
                                                                         *    labels associated with the statement) */
{
  jsp_label_t label;
  jsp_label_push (&label,
                  (jsp_label_type_flag_t) (JSP_LABEL_TYPE_UNNAMED_BREAKS | JSP_LABEL_TYPE_UNNAMED_CONTINUES),
                  TOKEN_EMPTY_INITIALIZER);

  jsp_label_t *outermost_stmt_label_p = (outermost_named_stmt_label_p != NULL ? outermost_named_stmt_label_p : &label);

  if (is_keyword (KW_DO))
  {
    parse_do_while_statement (outermost_stmt_label_p);
  }
  else if (is_keyword (KW_WHILE))
  {
    parse_while_statement (outermost_stmt_label_p);
  }
  else
  {
    JERRY_ASSERT (is_keyword (KW_FOR));
    jsp_parse_for_or_for_in_statement (outermost_stmt_label_p);
  }

  jsp_label_rewrite_jumps_and_pop (&label,
                                   serializer_get_current_instr_counter ());
} /* parse_iterational_statement */

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
parse_statement (jsp_label_t *outermost_stmt_label_p) /**< outermost (first) label, corresponding to the statement
                                                       *   (or NULL, if there are no named labels associated
                                                       *    with the statement) */
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
    parse_variable_declaration_list ();
    if (token_is (TOK_SEMICOLON))
    {
      skip_newlines ();
    }
    else
    {
      insert_semicolon ();
    }
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
  if (is_keyword (KW_DO)
      || is_keyword (KW_WHILE)
      || is_keyword (KW_FOR))
  {
    parse_iterational_statement (outermost_stmt_label_p);
    return;
  }
  if (is_keyword (KW_CONTINUE)
      || is_keyword (KW_BREAK))
  {
    bool is_break = is_keyword (KW_BREAK);

    skip_token ();

    jsp_label_t *label_p;
    bool is_simply_jumpable = true;
    if (token_is (TOK_NAME))
    {
      /* break or continue on a label */
      label_p = jsp_label_find (JSP_LABEL_TYPE_NAMED, tok, &is_simply_jumpable);

      if (label_p == NULL)
      {
        EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Label not found");
      }
    }
    else if (is_break)
    {
      label_p = jsp_label_find (JSP_LABEL_TYPE_UNNAMED_BREAKS,
                                TOKEN_EMPTY_INITIALIZER,
                                &is_simply_jumpable);

      if (label_p == NULL)
      {
        EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "No corresponding statement for the break");
      }
    }
    else
    {
      JERRY_ASSERT (!is_break);

      label_p = jsp_label_find (JSP_LABEL_TYPE_UNNAMED_CONTINUES,
                                TOKEN_EMPTY_INITIALIZER,
                                &is_simply_jumpable);

      if (label_p == NULL)
      {
        EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "No corresponding statement for the continue");
      }
    }

    JERRY_ASSERT (label_p != NULL);

    jsp_label_add_jump (label_p, is_simply_jumpable, is_break);

    return;
  }
  if (is_keyword (KW_RETURN))
  {
    if (!inside_function)
    {
      EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Return is illegal");
    }

    skip_token ();
    if (!token_is (TOK_SEMICOLON) && !token_is (TOK_NEWLINE))
    {
      const operand op = parse_expression (true, JSP_EVAL_RET_STORE_NOT_DUMP);
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
    const operand op = parse_expression (true, JSP_EVAL_RET_STORE_NOT_DUMP);
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
      skip_newlines ();

      jsp_label_t *label_p = jsp_label_find (JSP_LABEL_TYPE_NAMED, temp, NULL);
      if (label_p != NULL)
      {
        EMIT_ERROR (JSP_EARLY_ERROR_SYNTAX, "Label is duplicated");
      }

      jsp_label_t label;
      jsp_label_push (&label, JSP_LABEL_TYPE_NAMED, temp);

      parse_statement (outermost_stmt_label_p != NULL ? outermost_stmt_label_p : &label);

      jsp_label_rewrite_jumps_and_pop (&label,
                                       serializer_get_current_instr_counter ());
    }
    else
    {
      lexer_save_token (tok);
      tok = temp;
      operand expr = parse_expression (true, JSP_EVAL_RET_STORE_DUMP);
      dump_assignment_of_lhs_if_literal (expr);
      insert_semicolon ();
    }
  }
  else
  {
    parse_expression (true, JSP_EVAL_RET_STORE_DUMP);
    insert_semicolon ();
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
    parse_statement (NULL);
  }
}

/**
 * Check for "use strict" in directive prologue
 */
static void
check_directive_prologue_for_use_strict ()
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
      scopes_tree_set_strict_mode (STACK_TOP (scopes), true);
      lexer_set_strict_mode (scopes_tree_strict_mode (STACK_TOP (scopes)));
      break;
    }

    skip_newlines ();

    if (token_is (TOK_SEMICOLON))
    {
      skip_newlines ();
    }
  }

  if (lit_utf8_iterator_pos_cmp (start_loc, tok.loc) != 0)
  {
    lexer_seek (start_loc);
  }
  else
  {
    lexer_save_token (tok);
  }
} /* check_directive_prologue_for_use_strict */

/**
 * Parse source element list
 *
 *  source_element_list
 *   : source_element (LT!* source_element)*
 *   ;
 */
static void
parse_source_element_list (bool is_global) /**< flag, indicating that we parsing global context */
{
  const token_type end_tt = is_global ? TOK_EOF : TOK_CLOSE_BRACE;

  dumper_new_scope ();

  vm_instr_counter_t scope_code_flags_oc = dump_scope_code_flags_for_rewrite ();

  check_directive_prologue_for_use_strict ();

  dump_reg_var_decl_for_rewrite ();

  if (inside_eval
      && !inside_function)
  {
    dump_undefined_assignment (eval_ret_operand ());
  }

  skip_newlines ();
  while (!token_is (TOK_EOF) && !token_is (TOK_CLOSE_BRACE))
  {
    parse_source_element ();
    skip_newlines ();
  }

  if (!token_is (end_tt))
  {
    PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "Unexpected token", tok.loc);
  }

  lexer_save_token (tok);

  opcode_scope_code_flags_t scope_flags = OPCODE_SCOPE_CODE_FLAGS__EMPTY;

  scopes_tree fe_scope_tree = STACK_TOP (scopes);
  if (fe_scope_tree->strict_mode)
  {
    scope_flags = (opcode_scope_code_flags_t) (scope_flags | OPCODE_SCOPE_CODE_FLAGS_STRICT);
  }

  if (!fe_scope_tree->ref_arguments)
  {
    scope_flags = (opcode_scope_code_flags_t) (scope_flags | OPCODE_SCOPE_CODE_FLAGS_NOT_REF_ARGUMENTS_IDENTIFIER);
  }

  if (!fe_scope_tree->ref_eval)
  {
    scope_flags = (opcode_scope_code_flags_t) (scope_flags | OPCODE_SCOPE_CODE_FLAGS_NOT_REF_EVAL_IDENTIFIER);
  }
  rewrite_scope_code_flags (scope_code_flags_oc, scope_flags);

  rewrite_reg_var_decl ();
  dumper_finish_scope ();
} /* parse_source_element_list */

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
                      bool in_function, /**< flag indicating if we are parsing body of a function */
                      bool in_eval, /**< flag indicating if we are parsing body of eval code */
                      bool is_strict, /**< flag, indicating whether current code
                                       *   inherited strict mode from code of an outer scope */
                      const vm_instr_t **out_instrs_p) /**< out: generated byte-code array
                                                        *  (in case there were no syntax errors) */
{
  JERRY_ASSERT (out_instrs_p != NULL);

  inside_function = in_function;
  inside_eval = in_eval;
  code_contains_functions = false;

#ifndef JERRY_NDEBUG
  volatile bool is_parse_finished = false;
#endif /* !JERRY_NDEBUG */

  jsp_status_t status;

  jsp_mm_init ();
  jsp_label_init ();

  serializer_set_show_instrs (parser_show_instrs);
  dumper_init ();
  jsp_early_error_init ();

  STACK_INIT (scopes);
  STACK_PUSH (scopes, scopes_tree_init (NULL));
  serializer_set_scope (STACK_TOP (scopes));
  scopes_tree_set_strict_mode (STACK_TOP (scopes), is_strict);

  jmp_buf *jsp_early_error_label_p = jsp_early_error_get_early_error_longjmp_label ();
  int r = setjmp (*jsp_early_error_label_p);

  if (r == 0)
  {
    /*
     * Note:
     *      Operations that could raise an early error can be performed only during execution of the block.
     */

    lexer_init (source_p, source_size, parser_show_instrs);
    lexer_set_strict_mode (scopes_tree_strict_mode (STACK_TOP (scopes)));

    skip_newlines ();
    parse_source_element_list (true);

    skip_newlines ();
    JERRY_ASSERT (token_is (TOK_EOF));

    if (in_function)
    {
      dump_ret ();
    }
    else if (inside_eval)
    {
      dump_retval (eval_ret_operand ());
    }
    else
    {
      dump_ret ();
    }

#ifndef JERRY_NDEBUG
    is_parse_finished = true;
#endif /* !JERRY_NDEBUG */

    jsp_early_error_free ();

    *out_instrs_p = serializer_merge_scopes_into_bytecode ();

    dumper_free ();

    serializer_set_scope (NULL);
    scopes_tree_free (STACK_TOP (scopes));
    STACK_DROP (scopes, 1);
    STACK_FREE (scopes);

    status = JSP_STATUS_OK;
  }
  else
  {
    /* SyntaxError handling */

#ifndef JERRY_NDEBUG
    JERRY_ASSERT (!is_parse_finished);
#endif /* !JERRY_NDEBUG */

    *out_instrs_p = NULL;

    jsp_label_remove_all_labels ();
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

  jsp_label_finalize ();
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
                     const vm_instr_t **out_instrs_p) /**< out: generated byte-code array
                                                       *  (in case there were no syntax errors) */
{
  return parser_parse_program (source, source_size, false, false, false, out_instrs_p);
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
                   const vm_instr_t **out_instrs_p) /**< out: generated byte-code array
                                                     *  (in case there were no syntax errors) */
{
  return parser_parse_program (source, source_size, false, true, is_strict, out_instrs_p);
} /* parser_parse_eval */

/**
 * Parse a function created via new Function call
 *
 * NOTE: Array of arguments should contain at least one element.
 *       In case of new Function() call without parameters, empty string should be passed as argument to this
 *       function.
 *
 * @return true - if parse finished successfully (no SyntaxError were raised);
 *         false - otherwise.
 */
jsp_status_t
parser_parse_new_function (const jerry_api_char_t **params, /**< array of arguments of new Function (p1, p2, ..., pn,
                                                             *                                       body) call */
                           const size_t *params_size, /**< sizes of arguments strings */
                           size_t params_count, /**< total number of arguments passed to new Function (...) */
                           const vm_instr_t **out_instrs_p) /**< out: generated byte-code array
                                                             *  (in case there were no syntax errors) */
{
  // Process arguments
  JERRY_ASSERT (params_count > 0);
  for (size_t i = 0; i < params_count - 1; ++i)
  {
    FIXME ("check parameter's name for syntax errors");
    lit_find_or_create_literal_from_utf8_string ((lit_utf8_byte_t *) params[i], (lit_utf8_size_t) params_size[i]);
  }
  return parser_parse_program (params[params_count - 1],
                               params_size[params_count - 1],
                               true,
                               false,
                               false,
                               out_instrs_p);
} /* parser_parse_new_function */

/**
 * Indicates whether code contains functions
 *
 * @return true/false
 */
bool
parser_is_code_contains_functions ()
{
  return code_contains_functions;
} /* parser_is_code_contains_functions */

/**
 * Tell parser whether to dump bytecode
 */
void
parser_set_show_instrs (bool show_instrs) /**< flag indicating whether to dump bytecode */
{
  parser_show_instrs = show_instrs;
} /* parser_set_show_instrs */

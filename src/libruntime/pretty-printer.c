/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#include "jerry-libc.h"
#include "pretty-printer.h"

static int intendation;
static bool was_function_expression;
static bool was_subexpression;

static statement_type prev_stmt;

void
pp_reset (void)
{
  prev_stmt = STMT_EOF;
  intendation = 0;
}

void
pp_token (token tok)
{
  switch (tok.type)
    {
       case TOK_NAME:
         __printf ("IDENTIFIER (%s)\n", tok.data.name);
         break;

       case TOK_STRING:
         __printf ("STRING (%s)\n", tok.data.str);
         break;

       case TOK_KEYWORD:
         pp_keyword (tok.data.kw);
         break;

       case TOK_INT:
         __printf ("INTEGER (%d)\n", tok.data.num);
         break;

       case TOK_FLOAT:
         __printf ("FLOAT (%f)\n", tok.data.fp_num);
         break;

       case TOK_NULL:
         __printf ("NULL (null)\n");
         break;

       case TOK_BOOL:
         if (tok.data.is_true)
           __printf ("BOOL (true)\n");
         else
           __printf ("BOOL (false)\n");
         break;


       case TOK_OPEN_BRACE:
         __printf ("PUNC ({)\n");
         break;

       case TOK_CLOSE_BRACE:
         __printf ("PUNC (})\n");
         break;

       case TOK_OPEN_PAREN:
         __printf ("PUNC (()\n");
         break;

       case TOK_CLOSE_PAREN:
         __printf ("PUNC ())\n");
         break;

       case TOK_OPEN_SQUARE:
         __printf ("PUNC ([)\n");
         break;

       case TOK_CLOSE_SQUARE:
         __printf ("PUNC (])\n");
         break;


       case TOK_DOT:
         __printf ("PUNC (.)\n");
         break;

       case TOK_SEMICOLON:
         __printf ("PUNC (;)\n");
         break;

       case TOK_COMMA:
         __printf ("PUNC (,)\n");
         break;

       case TOK_LESS:
         __printf ("PUNC (<)\n");
         break;

       case TOK_GREATER:
         __printf ("PUNC (>)\n");
         break;

       case TOK_LESS_EQ:
         __printf ("PUNC (<=)\n");
         break;


       case TOK_GREATER_EQ:
         __printf ("PUNC (>=)\n");
         break;

       case TOK_DOUBLE_EQ:
         __printf ("PUNC (==)\n");
         break;

       case TOK_NOT_EQ:
         __printf ("PUNC (!=)\n");
         break;

       case TOK_TRIPLE_EQ:
         __printf ("PUNC (===)\n");
         break;

       case TOK_NOT_DOUBLE_EQ:
         __printf ("PUNC (!==)\n");
         break;


       case TOK_PLUS:
         __printf ("PUNC (+)\n");
         break;

       case TOK_MINUS:
         __printf ("PUNC (-)\n");
         break;

       case TOK_MULT:
         __printf ("PUNC (*)\n");
         break;

       case TOK_MOD:
         __printf ("PUNC (%%)\n");
         break;

       case TOK_DOUBLE_PLUS:
         __printf ("PUNC (++)\n");
         break;

       case TOK_DOUBLE_MINUS:
         __printf ("PUNC (--)\n");
         break;


       case TOK_LSHIFT:
         __printf ("PUNC (<<)\n");
         break;

       case TOK_RSHIFT:
         __printf ("PUNC (>>)\n");
         break;

       case TOK_RSHIFT_EX:
         __printf ("PUNC (>>>)\n");
         break;

       case TOK_AND:
         __printf ("PUNC (&)\n");
         break;

       case TOK_OR:
         __printf ("PUNC (|)\n");
         break;

       case TOK_XOR:
         __printf ("PUNC (^)\n");
         break;


       case TOK_NOT:
         __printf ("PUNC (!)\n");
         break;

       case TOK_COMPL:
         __printf ("PUNC (~)\n");
         break;

       case TOK_DOUBLE_AND:
         __printf ("PUNC (&&)\n");
         break;

       case TOK_DOUBLE_OR:
         __printf ("PUNC (||)\n");
         break;

       case TOK_QUERY:
         __printf ("PUNC (?)\n");
         break;

       case TOK_COLON:
         __printf ("PUNC (:)\n");
         break;


       case TOK_EQ:
         __printf ("PUNC (=)\n");
         break;

       case TOK_PLUS_EQ:
         __printf ("PUNC (+=)\n");
         break;

       case TOK_MINUS_EQ:
         __printf ("PUNC (-=)\n");
         break;

       case TOK_MULT_EQ:
         __printf ("PUNC (*=)\n");
         break;

       case TOK_MOD_EQ:
         __printf ("PUNC (%%=)\n");
         break;

       case TOK_LSHIFT_EQ:
         __printf ("PUNC (<<=)\n");
         break;


       case TOK_RSHIFT_EQ:
         __printf ("PUNC (>>=)\n");
         break;

       case TOK_RSHIFT_EX_EQ:
         __printf ("PUNC (>>>=)\n");
         break;

       case TOK_AND_EQ:
         __printf ("PUNC (&=)\n");
         break;

       case TOK_OR_EQ:
         __printf ("PUNC (|=)\n");
         break;

       case TOK_XOR_EQ:
         __printf ("PUNC (^=)\n");
         break;


       case TOK_DIV:
         __printf ("PUNC (/)\n");
         break;

       case TOK_DIV_EQ:
         __printf ("PUNC (/=)\n");
         break;

       case TOK_NEWLINE:
         __printf ("NEWLINE\n");
         break;

       default:
         JERRY_UNREACHABLE ();

    }
}

void
pp_keyword (keyword kw)
{
  switch (kw)
  {
    case KW_NONE:
      JERRY_UNREACHABLE ();
      break;

    case KW_RESERVED:
      __printf ("KEYWORD RESERVED\n");
      break;


    case KW_BREAK:
      __printf ("KEYWORD (break)\n");
      break;

    case KW_CASE:
      __printf ("KEYWORD (case)\n");
      break;

    case KW_CATCH:
      __printf ("KEYWORD (catch)\n");
      break;

    case KW_CONTINUE:
      __printf ("KEYWORD (continue)\n");
      break;

    case KW_DEBUGGER:
      __printf ("KEYWORD (debugger)\n");
      break;

    case KW_DEFAULT:
      __printf ("KEYWORD (default)\n");
      break;

    case KW_DELETE:
      __printf ("KEYWORD (delete)\n");
      break;


    case KW_DO:
      __printf ("KEYWORD (do)\n");
      break;

    case KW_ELSE:
      __printf ("KEYWORD (else)\n");
      break;

    case KW_FINALLY:
      __printf ("KEYWORD (finally)\n");
      break;

    case KW_FOR:
      __printf ("KEYWORD (for)\n");
      break;

    case KW_FUNCTION:
      __printf ("KEYWORD (function)\n");
      break;

    case KW_IF:
      __printf ("KEYWORD (if)\n");
      break;

    case KW_IN:
      __printf ("KEYWORD (in)\n");
      break;


    case KW_INSTANCEOF:
      __printf ("KEYWORD (instanceof)\n");
      break;

    case KW_NEW:
      __printf ("KEYWORD (new)\n");
      break;

    case KW_RETURN:
      __printf ("KEYWORD (return)\n");
      break;

    case KW_SWITCH:
      __printf ("KEYWORD (switch)\n");
      break;

    case KW_THIS:
      __printf ("KEYWORD (this)\n");
      break;

    case KW_THROW:
      __printf ("KEYWORD (throw)\n");
      break;

    case KW_TRY:
      __printf ("KEYWORD (try)\n");
      break;


    case KW_TYPEOF:
      __printf ("KEYWORD (typeof)\n");
      break;

    case KW_VAR:
      __printf ("KEYWORD (var)\n");
      break;

    case KW_VOID:
      __printf ("KEYWORD (void)\n");
      break;

    case KW_WHILE:
      __printf ("KEYWORD (while)\n");
      break;

    case KW_WITH:
      __printf ("KEYWORD (with)\n");
      break;

    default:
      JERRY_UNREACHABLE ();

  }
}

static void
intend (void)
{
  for (int i = 0; i < intendation; i++)
    __putchar (' '); 
}

static void
pp_formal_parameter_list (formal_parameter_list param_list)
{
  int i;

  for (i = 0; i < MAX_PARAMS; i++)
    {
      if (param_list.names[i] == NULL)
        break;
      if (i != 0)
        __printf (", ");
      __printf ("%s", param_list.names[i]);
    }
}

static void
pp_function_declaration (function_declaration func_decl)
{
  __printf ("function ");
  if (func_decl.name)
    __printf ("%s ", func_decl.name);
  __putchar ('(');
  pp_formal_parameter_list (func_decl.params);
  __printf (") ");
  was_function_expression = true;
}

static void
pp_literal (literal lit)
{
  switch (lit.type)
  {
    case LIT_NULL:
      __printf ("null");
      break;

    case LIT_BOOL:
      __printf ("%s", lit.data.is_true ? "true" : "false");
      break;

    case LIT_INT:
      __printf ("%d", lit.data.num);
      break;

    case LIT_STR:
      __printf ("\"%s\"", lit.data.str);
      break;

    default:
      JERRY_UNREACHABLE ();
  }
}

static void
pp_operand (operand op)
{
  JERRY_ASSERT (!is_operand_empty (op));
  if (op.is_literal)
    pp_literal (op.data.lit);
  else
    __printf ("%s", op.data.name);
}

static void
pp_operand_list (operand_list list)
{
  int i;

  for (i = 0; i < MAX_PARAMS; i++)
    {
      if (is_operand_empty (list.ops[i]))
        break;
      if (i != 0)
        __printf (", ");
      pp_operand (list.ops[i]);
    }
}

static void
pp_property (property prop)
{
  JERRY_ASSERT (!is_property_empty (prop));
  pp_operand (prop.name);
  __printf (" : ");
  pp_operand (prop.value);
}

static void
pp_property_list (property_list prop_list)
{
  int i;

  for (i = 0; i < MAX_PROPERTIES; i++)
    {
      if (is_property_empty (prop_list.props[i]))
        break;
      if (i != 0)
        __printf (", ");
      pp_property (prop_list.props[i]);
    }
}

static void
pp_call_expression (call_expression expr)
{
  JERRY_ASSERT (expr.name);
  __printf ("%s (", expr.name);
  pp_operand_list (expr.args);
  __printf (")\n");
}

static void
dump_two_operands (operand_pair pair, const char *operation)
{
  pp_operand (pair.op1);
  __printf ("%s", operation);
  pp_operand (pair.op2);
  __putchar ('\n');
}

static void
dump_unary (operand op, const char *operation)
{
  __printf ("%s", operation);
  pp_operand (op);
  __putchar ('\n');
}

static void
dump_postfix (operand op, const char *operation)
{
  pp_operand (op);
  __printf ("%s\n", operation);
}

static void
pp_assignment_expression (assignment_expression expr)
{
  if (expr.oper != AO_NONE && expr.var)
    __printf ("%s", expr.var);

  switch (expr.oper)
  {
    case AO_NONE:
      break;

    case AO_EQ:
      __printf (" = ");
      break;

    case AO_MULT_EQ:
      __printf (" *= ");
      break;

    case AO_DIV_EQ:
      __printf (" ?= ");
      break;

    case AO_MOD_EQ:
      __printf (" %%= ");
      break;

    case AO_PLUS_EQ:
      __printf (" += ");
      break;

    case AO_MINUS_EQ:
      __printf (" -= ");
      break;

    case AO_LSHIFT_EQ:
      __printf (" <<= ");
      break;

    case AO_RSHIFT_EQ:
      __printf (" >>= ");
      break;

    case AO_RSHIFT_EX_EQ:
      __printf (" >>>= ");
      break;

    case AO_AND_EQ:
      __printf (" &= ");
      break;

    case AO_XOR_EQ:
      __printf (" ^= ");
      break;

    case AO_OR_EQ:
      __printf (" |= ");
      break;

    default:
      JERRY_UNREACHABLE ();
  }

  switch (expr.type)
  {
    case ET_NONE:
      pp_operand (expr.data.ops.op1);
      JERRY_ASSERT (is_operand_empty (expr.data.ops.op2));
      return;

    case ET_LOGICAL_OR:
      dump_two_operands (expr.data.ops, " || ");
      return;

    case ET_LOGICAL_AND:
      dump_two_operands (expr.data.ops, " && ");
      return;

    case ET_BITWISE_OR:
      dump_two_operands (expr.data.ops, " | ");
      return;

    case ET_BITWISE_XOR:
      dump_two_operands (expr.data.ops, " ^ ");
      return;

    case ET_BITWISE_AND:
      dump_two_operands (expr.data.ops, " & ");
      return;

    case ET_DOUBLE_EQ:
      dump_two_operands (expr.data.ops, " == ");
      return;

    case ET_NOT_EQ:
      dump_two_operands (expr.data.ops, " != ");
      return;

    case ET_TRIPLE_EQ:
      dump_two_operands (expr.data.ops, " === ");
      return;

    case ET_NOT_DOUBLE_EQ:
      dump_two_operands (expr.data.ops, " !== ");
      return;

    case ET_LESS:
      dump_two_operands (expr.data.ops, " < ");
      return;

    case ET_GREATER:
      dump_two_operands (expr.data.ops, " > ");
      return;

    case ET_LESS_EQ:
      dump_two_operands (expr.data.ops, " <= ");
      return;

    case ET_GREATER_EQ:
      dump_two_operands (expr.data.ops, " <= ");
      return;

    case ET_INSTANCEOF:
      dump_two_operands (expr.data.ops, " instanceof ");
      return;

    case ET_IN:
      dump_two_operands (expr.data.ops, " in ");
      return;

    case ET_LSHIFT:
      dump_two_operands (expr.data.ops, " << ");
      return;

    case ET_RSHIFT:
      dump_two_operands (expr.data.ops, " >> ");
      return;

    case ET_RSHIFT_EX:
      dump_two_operands (expr.data.ops, " >>> ");
      return;

    case ET_PLUS:
      dump_two_operands (expr.data.ops, " + ");
      return;

    case ET_MINUS:
      dump_two_operands (expr.data.ops, " - ");
      return;

    case ET_MULT:
      dump_two_operands (expr.data.ops, " * ");
      return;

    case ET_DIV:
      dump_two_operands (expr.data.ops, " / ");
      return;

    case ET_MOD:
      dump_two_operands (expr.data.ops, " %% ");
      return;

    case ET_UNARY_DELETE:
      dump_unary (expr.data.ops.op1, "delete ");
      JERRY_ASSERT (is_operand_empty (expr.data.ops.op2));
      return;

    case ET_UNARY_VOID:
      dump_unary (expr.data.ops.op1, "void ");
      JERRY_ASSERT (is_operand_empty (expr.data.ops.op2));
      return;

    case ET_UNARY_TYPEOF:
      dump_unary (expr.data.ops.op1, "typeof ");
      JERRY_ASSERT (is_operand_empty (expr.data.ops.op2));
      return;

    case ET_UNARY_INCREMENT:
      dump_unary (expr.data.ops.op1, "++");
      JERRY_ASSERT (is_operand_empty (expr.data.ops.op2));
      return;

    case ET_UNARY_DECREMENT:
      dump_unary (expr.data.ops.op1, "--");
      JERRY_ASSERT (is_operand_empty (expr.data.ops.op2));
      return;

    case ET_UNARY_PLUS:
      dump_unary (expr.data.ops.op1, "+");
      JERRY_ASSERT (is_operand_empty (expr.data.ops.op2));
      return;

    case ET_UNARY_MINUS:
      dump_unary (expr.data.ops.op1, "-");
      JERRY_ASSERT (is_operand_empty (expr.data.ops.op2));
      return;

    case ET_UNARY_COMPL:
      dump_unary (expr.data.ops.op1, "~");
      JERRY_ASSERT (is_operand_empty (expr.data.ops.op2));
      return;

    case ET_UNARY_NOT:
      dump_unary (expr.data.ops.op1, "!");
      JERRY_ASSERT (is_operand_empty (expr.data.ops.op2));
      return;

    case ET_POSTFIX_INCREMENT:
      dump_postfix (expr.data.ops.op1, "++");
      JERRY_ASSERT (is_operand_empty (expr.data.ops.op2));
      return;

    case ET_POSTFIX_DECREMENT:
      dump_postfix (expr.data.ops.op1, "--");
      JERRY_ASSERT (is_operand_empty (expr.data.ops.op2));
      return;

    case ET_CALL:
      pp_call_expression (expr.data.call_expr);
      return;

    case ET_NEW:
      __printf ("new ");
      pp_operand (expr.data.ops.op1);
      JERRY_ASSERT (is_operand_empty (expr.data.ops.op2));
      __putchar ('\n');
      return;

    case ET_INDEX:
      pp_operand (expr.data.ops.op1);
      __putchar ('.');
      pp_operand (expr.data.ops.op2);
      __putchar ('\n');
      return;

    case ET_PROP_REF:
      pp_operand (expr.data.ops.op1);
      __putchar ('[');
      pp_operand (expr.data.ops.op2);
      __printf ("]\n");
      return;

    case ET_OBJECT:
      __putchar ('{');
      pp_property_list (expr.data.obj_lit);
      __printf ("}\n");
      return;

    case ET_FUNCTION:
      pp_function_declaration (expr.data.func_expr);
      return;

    case ET_ARRAY:
      __putchar ('[');
      pp_operand_list (expr.data.arr_lit);
      __printf ("]\n");
      return;

    case ET_SUBEXPRESSION:
      __putchar ('(');
      return;

    case ET_LITERAL:
    case ET_IDENTIFIER:  
      pp_operand (expr.data.ops.op1);
      JERRY_ASSERT (is_operand_empty (expr.data.ops.op2));
      __putchar ('\n');
      return;

    default:
      JERRY_UNREACHABLE ();
  }
}

static void
pp_expression (expression_list expr_list)
{
  int i;

  for (i = 0; i < MAX_EXPRS; i++)
    {
      if (is_expression_empty (expr_list.exprs[i]))
        break;
      if (i != 0)
        __printf (", ");
      pp_assignment_expression (expr_list.exprs[i]);
    }

  if (was_subexpression && !was_function_expression)
    {
      __putchar (')');
      was_subexpression = false;
    }
}

static void
pp_variable_declaration (variable_declaration var_decl)
{
  __printf ("%s", var_decl.name);
  if (!is_expression_empty (var_decl.assign_expr))
    {
      __printf (" = ");
      pp_assignment_expression (var_decl.assign_expr);
    }
}

static void
pp_variable_declaration_list (variable_declaration_list decl_list)
{
  int i;

  __printf ("var ");

  for (i = 0; i < MAX_DECLS; i++)
    {
      if (is_variable_declaration_empty (decl_list.decls[i]))
        break;
      if (i != 0)
        __printf (", ");
      pp_variable_declaration (decl_list.decls[i]);
    }
}

static void
pp_for_in_statement_initializer_part (for_in_statement_initializer_part init)
{
  if (init.is_decl)
    {
      __printf ("var ");
      pp_variable_declaration (init.data.decl);
    }
  else if (!is_expression_empty (init.data.left_hand_expr))
    pp_assignment_expression (init.data.left_hand_expr);
}

static void
pp_for_in_statement (for_in_statement for_in_stmt)
{
  __printf ("for (");
  pp_for_in_statement_initializer_part (for_in_stmt.init);
  __printf (" in ");
  pp_expression (for_in_stmt.list_expr);
  __printf (") ");
}

static void
pp_for_statement_initialiser_part (for_statement_initialiser_part init)
{
  if (init.is_decl)
    pp_variable_declaration_list (init.data.decl_list);
  else
    pp_expression (init.data.expr);
}

static void
pp_for_statement (for_statement for_stmt)
{
  __printf ("for (");
  pp_for_statement_initialiser_part (for_stmt.init);
  __printf ("; ");
  if (is_expression_empty (for_stmt.limit))
    pp_assignment_expression (for_stmt.limit);
  __printf ("; ");
  if (is_expression_empty (for_stmt.incr))
    pp_assignment_expression (for_stmt.incr);
  __printf (") ");
}

static void
pp_for_or_for_in_statement (for_or_for_in_statement for_or_for_in_stmt)
{
  if (for_or_for_in_stmt.is_for_in)
    pp_for_in_statement (for_or_for_in_stmt.data.for_in_stmt);
  else
    pp_for_statement (for_or_for_in_stmt.data.for_stmt);
}

void
pp_statement (statement stmt)
{
  was_function_expression = false;
  was_subexpression = false;

  if (prev_stmt == STMT_BLOCK_END)
    {
      if (stmt.type == STMT_EMPTY)
        {
          __printf (";\n");
          prev_stmt = stmt.type;
          return;
        }
      else
        __putchar ('\n');
    }

  switch (stmt.type)
  {
    case STMT_BLOCK_START:
      __printf ("{\n");
      intendation += 2;
      break;

    case STMT_BLOCK_END:
      intendation -= 2;
      intend ();
      __printf ("}");
      break;

    case STMT_VARIABLE:
      intend ();
      pp_variable_declaration_list (stmt.data.var_stmt);
      break;

    case STMT_EMPTY:
      __printf (";\n");
      break;

    case STMT_IF:
      intend ();
      __printf ("if (");
      pp_expression (stmt.data.expr);
      __printf (") ");
      break;

    case STMT_ELSE:
      intend ();
      __printf ("else ");
      break;

    case STMT_ELSE_IF:
      intend ();
      __printf ("else if(");
      pp_expression (stmt.data.expr);
      __printf (") ");
      break;

    case STMT_DO:
      intend ();
      __printf ("do ");
      break;

    case STMT_WHILE:
      intend ();
      __printf ("while (");
      pp_expression (stmt.data.expr);
      __printf (") ");
      break;

    case STMT_FOR_OR_FOR_IN:
      intend ();
      pp_for_or_for_in_statement (stmt.data.for_stmt);
      break;

    case STMT_CONTINUE:
      intend ();
      __printf ("continue\n");
      break;

    case STMT_BREAK:
      intend ();
      __printf ("break\n");
      break;

    case STMT_RETURN:
      intend ();
      __printf ("return ");
      pp_expression (stmt.data.expr);
      if (!was_function_expression)
        __printf (";\n");
      break;

    case STMT_WITH:
      intend ();
      __printf ("with (");
      pp_expression (stmt.data.expr);
      __printf (") ");
      break;

    case STMT_LABELLED:
      intend ();
      __printf ("%s:\n", stmt.data.name);
      break;

    case STMT_SWITCH:
      intend ();
      __printf ("switch (");
      pp_expression (stmt.data.expr);
      __printf (") ");
      break;

    case STMT_CASE:
      intend ();
      __printf ("case ");
      pp_expression (stmt.data.expr);
      __printf (":\n");
      break;

    case STMT_THROW:
      intend ();
      __printf ("throw ");
      pp_expression (stmt.data.expr);
      __printf (";\n");
      break;

    case STMT_TRY:
      intend ();
      __printf ("try ");
      break;

    case STMT_CATCH:
      intend ();
      __printf ("catch (");
      pp_expression (stmt.data.expr);
      __printf (") ");
      break;

    case STMT_FINALLY:
      intend ();
      __printf ("finally ");
      break;

    case STMT_EXPRESSION:
      intend ();
      pp_expression (stmt.data.expr);
      break;

    case STMT_SUBEXPRESSION_END:
      __putchar (')');
      break;

    case STMT_FUNCTION:
      intend ();
      pp_function_declaration (stmt.data.fun_decl);
      break;

    default:
      JERRY_UNREACHABLE ();
  }

  prev_stmt = stmt.type;
}

void pp_finish (void)
{
  if (prev_stmt == STMT_BLOCK_END)
    __putchar ('\n');
}

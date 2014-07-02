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

#include "pretty-printer.h"
#include "error.h"
#include "globals.h"

static int intendation;
static bool was_function_expression;
static bool was_subexpression;

static statement_type prev_stmt;

static void pp_expression (expression *);
static void pp_assignment_expression (assignment_expression *);
static void pp_member_expression (member_expression *);

void
pp_reset ()
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
      printf ("IDENTIFIER (%s)\n", tok.data.name);
      break;

    case TOK_STRING:
      printf ("STRING (%s)\n", tok.data.str);
      break;

    case TOK_KEYWORD:
      pp_keyword (tok.data.kw);
      break;

    case TOK_INT:
      printf ("INTEGER (%d)\n", tok.data.num);
      break;

    case TOK_FLOAT:
      printf ("FLOAT (%f)\n", tok.data.fp_num);
      break;

    case TOK_NULL:
      printf ("NULL (null)\n");
      break;

    case TOK_BOOL:
      if (tok.data.is_true)
        printf ("BOOL (true)\n");
      else
        printf ("BOOL (false)\n");
      break;


    case TOK_OPEN_BRACE:
      printf ("PUNC ({)\n");
      break;

    case TOK_CLOSE_BRACE:
      printf ("PUNC (})\n");
      break;

    case TOK_OPEN_PAREN:
      printf ("PUNC (()\n");
      break;

    case TOK_CLOSE_PAREN:
      printf ("PUNC ())\n");
      break;

    case TOK_OPEN_SQUARE:
      printf ("PUNC ([)\n");
      break;

    case TOK_CLOSE_SQUARE:
      printf ("PUNC (])\n");
      break;


    case TOK_DOT:
      printf ("PUNC (.)\n");
      break;

    case TOK_SEMICOLON:
      printf ("PUNC (;)\n");
      break;

    case TOK_COMMA:
      printf ("PUNC (,)\n");
      break;

    case TOK_LESS:
      printf ("PUNC (<)\n");
      break;

    case TOK_GREATER:
      printf ("PUNC (>)\n");
      break;

    case TOK_LESS_EQ:
      printf ("PUNC (<=)\n");
      break;


    case TOK_GREATER_EQ:
      printf ("PUNC (>=)\n");
      break;

    case TOK_DOUBLE_EQ:
      printf ("PUNC (==)\n");
      break;

    case TOK_NOT_EQ:
      printf ("PUNC (!=)\n");
      break;

    case TOK_TRIPLE_EQ:
      printf ("PUNC (===)\n");
      break;

    case TOK_NOT_DOUBLE_EQ:
      printf ("PUNC (!==)\n");
      break;


    case TOK_PLUS:
      printf ("PUNC (+)\n");
      break;

    case TOK_MINUS:
      printf ("PUNC (-)\n");
      break;

    case TOK_MULT:
      printf ("PUNC (*)\n");
      break;

    case TOK_MOD:
      printf ("PUNC (%%)\n");
      break;

    case TOK_DOUBLE_PLUS:
      printf ("PUNC (++)\n");
      break;

    case TOK_DOUBLE_MINUS:
      printf ("PUNC (--)\n");
      break;


    case TOK_LSHIFT:
      printf ("PUNC (<<)\n");
      break;

    case TOK_RSHIFT:
      printf ("PUNC (>>)\n");
      break;

    case TOK_RSHIFT_EX:
      printf ("PUNC (>>>)\n");
      break;

    case TOK_AND:
      printf ("PUNC (&)\n");
      break;

    case TOK_OR:
      printf ("PUNC (|)\n");
      break;

    case TOK_XOR:
      printf ("PUNC (^)\n");
      break;


    case TOK_NOT:
      printf ("PUNC (!)\n");
      break;

    case TOK_COMPL:
      printf ("PUNC (~)\n");
      break;

    case TOK_DOUBLE_AND:
      printf ("PUNC (&&)\n");
      break;

    case TOK_DOUBLE_OR:
      printf ("PUNC (||)\n");
      break;

    case TOK_QUERY:
      printf ("PUNC (?)\n");
      break;

    case TOK_COLON:
      printf ("PUNC (:)\n");
      break;


    case TOK_EQ:
      printf ("PUNC (=)\n");
      break;

    case TOK_PLUS_EQ:
      printf ("PUNC (+=)\n");
      break;

    case TOK_MINUS_EQ:
      printf ("PUNC (-=)\n");
      break;

    case TOK_MULT_EQ:
      printf ("PUNC (*=)\n");
      break;

    case TOK_MOD_EQ:
      printf ("PUNC (%%=)\n");
      break;

    case TOK_LSHIFT_EQ:
      printf ("PUNC (<<=)\n");
      break;


    case TOK_RSHIFT_EQ:
      printf ("PUNC (>>=)\n");
      break;

    case TOK_RSHIFT_EX_EQ:
      printf ("PUNC (>>>=)\n");
      break;

    case TOK_AND_EQ:
      printf ("PUNC (&=)\n");
      break;

    case TOK_OR_EQ:
      printf ("PUNC (|=)\n");
      break;

    case TOK_XOR_EQ:
      printf ("PUNC (^=)\n");
      break;


    case TOK_DIV:
      printf ("PUNC (/)\n");
      break;

    case TOK_DIV_EQ:
      printf ("PUNC (/=)\n");
      break;

    case TOK_NEWLINE:
      printf ("NEWLINE\n");
      break;

    default:
      unreachable ();

    }
}

void
pp_keyword (keyword kw)
{
  switch (kw)
    {
    case KW_NONE:
      unreachable ();
      break;

    case KW_RESERVED:
      printf ("KEYWORD RESERVED\n");
      break;


    case KW_BREAK:
      printf ("KEYWORD (break)\n");
      break;

    case KW_CASE:
      printf ("KEYWORD (case)\n");
      break;

    case KW_CATCH:
      printf ("KEYWORD (catch)\n");
      break;

    case KW_CONTINUE:
      printf ("KEYWORD (continue)\n");
      break;

    case KW_DEBUGGER:
      printf ("KEYWORD (debugger)\n");
      break;

    case KW_DEFAULT:
      printf ("KEYWORD (default)\n");
      break;

    case KW_DELETE:
      printf ("KEYWORD (delete)\n");
      break;


    case KW_DO:
      printf ("KEYWORD (do)\n");
      break;

    case KW_ELSE:
      printf ("KEYWORD (else)\n");
      break;

    case KW_FINALLY:
      printf ("KEYWORD (finally)\n");
      break;

    case KW_FOR:
      printf ("KEYWORD (for)\n");
      break;

    case KW_FUNCTION:
      printf ("KEYWORD (function)\n");
      break;

    case KW_IF:
      printf ("KEYWORD (if)\n");
      break;

    case KW_IN:
      printf ("KEYWORD (in)\n");
      break;


    case KW_INSTANCEOF:
      printf ("KEYWORD (instanceof)\n");
      break;

    case KW_NEW:
      printf ("KEYWORD (new)\n");
      break;

    case KW_RETURN:
      printf ("KEYWORD (return)\n");
      break;

    case KW_SWITCH:
      printf ("KEYWORD (switch)\n");
      break;

    case KW_THIS:
      printf ("KEYWORD (this)\n");
      break;

    case KW_THROW:
      printf ("KEYWORD (throw)\n");
      break;

    case KW_TRY:
      printf ("KEYWORD (try)\n");
      break;


    case KW_TYPEOF:
      printf ("KEYWORD (typeof)\n");
      break;

    case KW_VAR:
      printf ("KEYWORD (var)\n");
      break;

    case KW_VOID:
      printf ("KEYWORD (void)\n");
      break;

    case KW_WHILE:
      printf ("KEYWORD (while)\n");
      break;

    case KW_WITH:
      printf ("KEYWORD (with)\n");
      break;

    default:
      unreachable ();

    }
}

static void
intend ()
{
  for (int i = 0; i < intendation; i++)
    putchar (' '); 
}

static void
pp_formal_parameter_list (formal_parameter_list *param_list)
{
  formal_parameter_list *list = param_list;
  assert (param_list);
  while (list)
    {
      printf ("%s", list->name);
      if (list->next)
        printf (", ");
      list = list->next;
    }
}

static void
pp_function_declaration (function_declaration *func_decl)
{
  assert (func_decl);
  printf ("function ");
  if (func_decl->name)
    printf ("%s ", func_decl->name);
  putchar ('(');
  if (func_decl->params)
    pp_formal_parameter_list (func_decl->params);
  printf (") ");
  was_function_expression = true;
}

static void
pp_literal (literal *lit)
{
  assert (lit);
  switch (lit->type)
    {
    case LIT_NULL:
      printf ("null");
      break;

    case LIT_BOOL:
      printf ("%s", lit->data.is_true ? "true" : "false");
      break;

    case LIT_INT:
      printf ("%d", lit->data.num);
      break;

    case LIT_STR:
      printf ("\"%s\"", lit->data.str);
      break;

    default:
      unreachable ();
    }
}

static void
pp_property_name (property_name *name)
{
  assert (name);
  switch (name->type)
    {
    case PN_NAME:
      printf ("%s", name->data.name);
      break;

    case PN_STRING:
      printf ("%s", name->data.str);
      break;

    case PN_NUM:
      printf ("%d", name->data.num);
      break;

    default:
      unreachable ();
    }
}

static void
pp_property_name_and_value (property_name_and_value *nav)
{
  assert (nav);
  pp_property_name (nav->name);
  printf (" : ");
  pp_assignment_expression (nav->assign_expr);
}

static void
pp_property_name_and_value_list (property_name_and_value_list *nav_list)
{
  property_name_and_value_list *list = nav_list;
  assert (nav_list);
  while (list)
    {
      pp_property_name_and_value (list->nav);
      if (list->next)
        printf (", ");
      list = list->next;
    }
}

static void
pp_primary_expression (primary_expression *primary_expr)
{
  assert (primary_expr);
  switch (primary_expr->type)
    {
    case PE_THIS:
      printf ("this");
      break;

    case PE_NAME:
      printf ("%s", primary_expr->data.name);
      break;

    case PE_LITERAL:
      pp_literal (primary_expr->data.lit);
      break;

    case PE_ARRAY:
      putchar ('[');
      if (primary_expr->data.array_lit)
        pp_expression (primary_expr->data.array_lit);
      putchar (']');
      break;

    case PE_OBJECT:
      putchar ('{');
      if (primary_expr->data.object_lit)
        pp_property_name_and_value_list (primary_expr->data.object_lit);
      putchar ('}');
      break;

    case PE_EXPR:
      putchar ('(');
      if (primary_expr->data.expr)
        pp_expression (primary_expr->data.expr);
      was_subexpression = true;
      break;

    default:
      unreachable ();
    }
}

static void
pp_member_expression_with_arguments (member_expression_with_arguments *member_expr)
{
  assert (member_expr);
  printf ("new ");
  pp_member_expression (member_expr->member_expr);
  if (member_expr->args)
    {
      putchar (' ');
      pp_expression (member_expr->args);
    }
}

static void
pp_member_expression_suffix (member_expression_suffix *suffix)
{
  assert (suffix);
  switch (suffix->type)
    {
    case MES_INDEX:
      putchar ('[');
      pp_expression (suffix->data.index_expr);
      putchar (']');
      break;

    case MES_PROPERTY:
      putchar ('.');
      printf ("%s", suffix->data.name);
      break;

    default:
      unreachable ();
    }
}

static void 
pp_member_expression_suffix_list (member_expression_suffix_list *suffix_list) 
{ 
  member_expression_suffix_list *list = suffix_list; 
  assert (suffix_list); 
  while (list) 
    { 
      pp_member_expression_suffix (list->suffix); 
      list = list->next; 
    } 
}

static void
pp_member_expression (member_expression *member_expr)
{
  assert (member_expr);
  switch (member_expr->type)
    {
    case ME_PRIMARY:
      pp_primary_expression (member_expr->data.primary_expr);
      break;

    case ME_FUNCTION:
      pp_function_declaration (member_expr->data.function_expr);
      break;

    case ME_ARGS:
      pp_member_expression_with_arguments (member_expr->data.args);
      break;

    default:
      unreachable ();
    }
  if (member_expr->suffix_list)
    pp_member_expression_suffix_list (member_expr->suffix_list);
}

static void
pp_call_expression_suffix (call_expression_suffix *suffix)
{
  assert (suffix);
  switch (suffix->type)
    {
    case CAS_ARGS:
      putchar ('(');
      pp_expression (suffix->data.args);
      putchar (')');
      break;

    case CAS_INDEX:
      putchar ('[');
      pp_expression (suffix->data.index_expr);
      putchar (']');
      break;

    case CAS_PROPERTY:
      putchar ('.');
      printf ("%s", suffix->data.name);
      break;

    default:
      unreachable ();
    }
}

static void
pp_call_expression_suffix_list (call_expression_suffix_list *suffix_list)
{
  call_expression_suffix_list *list = suffix_list;
  assert (suffix_list);
  while (list)
    {
      pp_call_expression_suffix (list->suffix);
      list = list->next;
    }
}

static void
pp_call_expression (call_expression *call_expr)
{
  assert (call_expr);
  pp_member_expression (call_expr->member_expr);
  printf (" (");
  if (call_expr->args)
    pp_expression (call_expr->args);
  printf (")");
  if (call_expr->suffix_list)
    pp_call_expression_suffix_list (call_expr->suffix_list);
}

static void
pp_new_expression (new_expression *new_expr)
{
  assert (new_expr);
  switch (new_expr->type)
    {
    case NE_MEMBER:
      pp_member_expression (new_expr->data.member_expr);
      break;

    case NE_NEW:
      printf ("new ");
      pp_new_expression (new_expr->data.new_expr);
      break;

      unreachable ();
    }
}

static void 
pp_left_hand_side_expression (left_hand_side_expression *left_expr)
{
  assert (left_expr);
  switch (left_expr->type)
    {
    case LHSE_CALL:
      pp_call_expression (left_expr->data.call_expr);
      break;

    case LHSE_NEW:
      pp_new_expression (left_expr->data.new_expr);
      break;

    default:
      unreachable ();
    }
}

static void
pp_postfix_expression (postfix_expression *postfix_expr)
{
  assert (postfix_expr);
  pp_left_hand_side_expression (postfix_expr->expr);
  switch (postfix_expr->type)
    {
    case PE_INCREMENT:
      printf ("++");
      break;

    case PE_DECREMENT:
      printf ("--");
      break;

    default:
      break;
    }
}

static void
pp_unary_expression (unary_expression *unary_expr)
{
  assert (unary_expr);
  if (unary_expr->type != UE_POSTFIX)
    putchar ('(');
  switch (unary_expr->type)
    {
    case UE_POSTFIX:
      pp_postfix_expression (unary_expr->data.postfix_expr);
      break;

    case UE_DELETE: 
      printf ("delete ");
      pp_unary_expression (unary_expr->data.unary_expr);
      break;

    case UE_VOID: 
      printf ("void ");
      pp_unary_expression (unary_expr->data.unary_expr);
      break;

    case UE_TYPEOF: 
      printf ("typeof ");
      pp_unary_expression (unary_expr->data.unary_expr);
      break;

    case UE_INCREMENT: 
      printf ("++");
      pp_unary_expression (unary_expr->data.unary_expr);
      break;

    case UE_DECREMENT: 
      printf ("--");
      pp_unary_expression (unary_expr->data.unary_expr);
      break;

    case UE_PLUS: 
      printf ("+");
      pp_unary_expression (unary_expr->data.unary_expr);
      break;

    case UE_MINUS: 
      printf ("-");
      pp_unary_expression (unary_expr->data.unary_expr);
      break;

    case UE_COMPL: 
      printf ("~");
      pp_unary_expression (unary_expr->data.unary_expr);
      break;

    case UE_NOT: 
      printf ("!");
      pp_unary_expression (unary_expr->data.unary_expr);
      break;

    default:
      unreachable ();
    }
  if (unary_expr->type != UE_POSTFIX)
    putchar (')');
}

static void 
pp_multiplicative_expression_list (multiplicative_expression_list *expr_list) 
{ 
  multiplicative_expression_list *list = expr_list; 
  assert (expr_list); 
  if (expr_list->next) 
    putchar ('('); 
  while (list) 
    { 
      pp_unary_expression (list->unary_expr); 
      if (list->next) 
        {
          switch (list->type)
            {
            case ME_MULT:
              printf (" * ");
              break;

            case ME_DIV:
              printf (" / ");
              break;

            case ME_MOD:
              printf (" %% ");
              break;

            default:
              unreachable ();
            }
        }
      list = list->next; 
    } 
  if (expr_list->next) 
    putchar (')'); 
}

static void 
pp_additive_expression_list (additive_expression_list *expr_list) 
{ 
  additive_expression_list *list = expr_list; 
  assert (expr_list); 
  if (expr_list->next) 
    putchar ('('); 
  while (list) 
    { 
      pp_multiplicative_expression_list (list->mult_expr); 
      if (list->next) 
        {
          switch (list->type)
            {
            case AE_PLUS:
              printf (" + ");
              break;

            case AE_MINUS:
              printf (" - ");
              break;

            default:
              unreachable ();
            }
        }
      list = list->next; 
    } 
  if (expr_list->next) 
    putchar (')'); 
}

static void 
pp_shift_expression_list (shift_expression_list *expr_list) 
{ 
  shift_expression_list *list = expr_list; 
  assert (expr_list); 
  if (expr_list->next) 
    putchar ('('); 
  while (list) 
    { 
      pp_additive_expression_list (list->add_expr); 
      if (list->next) 
        {
          switch (list->type)
            {
            case SE_LSHIFT:
              printf (" << ");
              break;

            case SE_RSHIFT:
              printf (" >> ");
              break;

            case SE_RSHIFT_EX:
              printf (" >>> ");
              break;

            default:
              unreachable ();
            }
        }
      list = list->next; 
    } 
  if (expr_list->next) 
    putchar (')'); 
}

static void 
pp_relational_expression_list (relational_expression_list *expr_list) 
{ 
  relational_expression_list *list = expr_list; 
  assert (expr_list); 
  if (expr_list->next) 
    putchar ('('); 
  while (list) 
    { 
      pp_shift_expression_list (list->shift_expr); 
      if (list->next) 
        {
          switch (list->type)
            {
            case RE_LESS:
              printf (" < ");
              break;

            case RE_GREATER:
              printf (" > ");
              break;

            case RE_LESS_EQ:
              printf (" <= ");
              break;

            case RE_GREATER_EQ:
              printf (" >= ");
              break;

            case RE_INSTANCEOF:
              printf (" instanceof ");
              break;

            case RE_IN:
              printf (" in ");
              break;

            default:
              unreachable ();
            }
        }
      list = list->next; 
    } 
  if (expr_list->next) 
    putchar (')'); 
}

static void 
pp_equality_expression_list (equality_expression_list *expr_list) 
{ 
  equality_expression_list *list = expr_list; 
  assert (expr_list); 
  if (expr_list->next) 
    putchar ('('); 
  while (list) 
    { 
      pp_relational_expression_list (list->rel_expr); 
      if (list->next) 
        {
          switch (list->type)
            {
            case EE_DOUBLE_EQ:
              printf (" == ");
              break;

            case EE_NOT_EQ:
              printf (" != ");
              break;

            case EE_TRIPLE_EQ:
              printf (" === ");
              break;

            case EE_NOT_DOUBLE_EQ:
              printf (" !== ");
              break;

            default:
              unreachable ();
            }
        }
      list = list->next; 
    } 
  if (expr_list->next) 
    putchar (')'); 
}

static void 
pp_bitwise_and_expression_list (bitwise_and_expression_list *expr_list) 
{ 
  bitwise_and_expression_list *list = expr_list; 
  assert (expr_list); 
  if (expr_list->next) 
    putchar ('('); 
  while (list) 
    { 
      pp_equality_expression_list (list->eq_expr); 
      if (list->next) 
        printf (" & "); 
      list = list->next; 
    } 
  if (expr_list->next) 
    putchar (')'); 
}

static void 
pp_bitwise_xor_expression_list (bitwise_xor_expression_list *expr_list) 
{ 
  bitwise_xor_expression_list *list = expr_list; 
  assert (expr_list); 
  if (expr_list->next) 
    putchar ('('); 
  while (list) 
    { 
      pp_bitwise_and_expression_list (list->and_expr); 
      if (list->next) 
        printf (" ^ "); 
      list = list->next; 
    } 
  if (expr_list->next) 
    putchar (')'); 
}

static void 
pp_bitwise_or_expression_list (bitwise_or_expression_list *expr_list) 
{ 
  bitwise_or_expression_list *list = expr_list; 
  assert (expr_list); 
  if (expr_list->next) 
    putchar ('('); 
  while (list) 
    { 
      pp_bitwise_xor_expression_list (list->xor_expr); 
      if (list->next) 
        printf (" | "); 
      list = list->next; 
    } 
  if (expr_list->next) 
    putchar (')'); 
}

static void 
pp_logical_and_expression_list (logical_and_expression_list *expr_list) 
{ 
  logical_and_expression_list *list = expr_list; 
  assert (expr_list); 
  if (expr_list->next) 
    putchar ('('); 
  while (list) 
    { 
      pp_bitwise_or_expression_list (list->or_expr); 
      if (list->next) 
        printf (" && "); 
      list = list->next; 
    } 
  if (expr_list->next) 
    putchar (')'); 
}

static void 
pp_logical_or_expression_list (logical_or_expression_list *expr_list) 
{ 
  logical_or_expression_list *list = expr_list; 
  assert (expr_list); 
  if (expr_list->next) 
    putchar ('('); 
  while (list) 
    { 
      pp_logical_and_expression_list (list->and_expr); 
      if (list->next) 
        printf (" || "); 
      list = list->next; 
    } 
  if (expr_list->next) 
    putchar (')'); 
}

static void
pp_conditional_expression (conditional_expression *cond_expr)
{
  assert (cond_expr);
  pp_logical_or_expression_list (cond_expr->or_expr);
  if (cond_expr->then_expr)
    {
      printf (" ? ");
      pp_assignment_expression (cond_expr->then_expr);
    }
  if (cond_expr->else_expr)
    {
      printf (" : ");
      pp_assignment_expression (cond_expr->else_expr);
    }
}

static void
pp_assignment_expression (assignment_expression *assign_expr)
{
  assert (assign_expr);
  switch (assign_expr->type)
    {
    case AE_COND:
      pp_conditional_expression (assign_expr->data.cond_expr);
      return;

    case AE_EQ:
      pp_left_hand_side_expression (assign_expr->data.s.left_hand_expr);
      printf (" = ");
      pp_assignment_expression (assign_expr->data.s.assign_expr);
      return;

    case AE_MULT_EQ:
      pp_left_hand_side_expression (assign_expr->data.s.left_hand_expr);
      printf (" *= ");
      pp_assignment_expression (assign_expr->data.s.assign_expr);
      return;

    case AE_DIV_EQ:
      pp_left_hand_side_expression (assign_expr->data.s.left_hand_expr);
      printf (" /= ");
      pp_assignment_expression (assign_expr->data.s.assign_expr);
      return;

    case AE_MOD_EQ:
      pp_left_hand_side_expression (assign_expr->data.s.left_hand_expr);
      printf (" %%= ");
      pp_assignment_expression (assign_expr->data.s.assign_expr);
      return;

    case AE_PLUS_EQ:
      pp_left_hand_side_expression (assign_expr->data.s.left_hand_expr);
      printf (" += ");
      pp_assignment_expression (assign_expr->data.s.assign_expr);
      return;

    case AE_MINUS_EQ:
      pp_left_hand_side_expression (assign_expr->data.s.left_hand_expr);
      printf (" -= ");
      pp_assignment_expression (assign_expr->data.s.assign_expr);
      return;

    case AE_LSHIFT_EQ:
      pp_left_hand_side_expression (assign_expr->data.s.left_hand_expr);
      printf (" <<= ");
      pp_assignment_expression (assign_expr->data.s.assign_expr);
      return;

    case AE_RSHIFT_EQ:
      pp_left_hand_side_expression (assign_expr->data.s.left_hand_expr);
      printf (" >>= ");
      pp_assignment_expression (assign_expr->data.s.assign_expr);
      return;

    case AE_RSHIFT_EX_EQ:
      pp_left_hand_side_expression (assign_expr->data.s.left_hand_expr);
      printf (" >>>= ");
      pp_assignment_expression (assign_expr->data.s.assign_expr);
      return;

    case AE_AND_EQ:
      pp_left_hand_side_expression (assign_expr->data.s.left_hand_expr);
      printf (" &= ");
      pp_assignment_expression (assign_expr->data.s.assign_expr);
      return;

    case AE_OR_EQ:
      pp_left_hand_side_expression (assign_expr->data.s.left_hand_expr);
      printf (" |= ");
      pp_assignment_expression (assign_expr->data.s.assign_expr);
      return;

    case AE_XOR_EQ:
      pp_left_hand_side_expression (assign_expr->data.s.left_hand_expr);
      printf (" ^= ");
      pp_assignment_expression (assign_expr->data.s.assign_expr);
      return;

    default:
      unreachable ();
    }
}

static void
pp_expression (expression_list *expr_list)
{
  expression_list *list = expr_list;
  assert (expr_list);
  while (list)
    {
      pp_assignment_expression (list->assign_expr);
      if (list->next)
        printf (", ");
      list = list->next;
    }

  if (was_subexpression && !was_function_expression)
    {
      putchar (')');
      was_subexpression = false;
    }
}

static void
pp_variable_declaration (variable_declaration *var_decl)
{
  assert (var_decl);
  printf ("%s", var_decl->name);
  if (var_decl->ass_expr)
    {
      printf (" = ");
      pp_assignment_expression (var_decl->ass_expr);
    }
}

static void
pp_variable_declaration_list (variable_declaration_list *decl_list)
{
  variable_declaration_list *list = decl_list;
  assert (decl_list);
  printf ("var ");
  while (list)
    {
      pp_variable_declaration (list->var_decl);
      if (list->next)
        printf (", ");
      list = list->next;
    }
}

static void
pp_for_in_statement_initializer_part (for_in_statement_initializer_part *init)
{
  assert (init);
  if (init->is_decl)
    {
      printf ("var ");
      pp_variable_declaration (init->data.decl);
    }
  else
    pp_left_hand_side_expression (init->data.left_hand_expr);
}

static void
pp_for_in_statement (for_in_statement *for_in_stmt)
{
  assert (for_in_stmt);
  printf ("for (");
  pp_for_in_statement_initializer_part (for_in_stmt->init);
  printf (" in ");
  pp_expression (for_in_stmt->list_expr);
  printf (") ");
}

static void
pp_for_statement_initialiser_part (for_statement_initialiser_part *init)
{
  assert (init);
  if (init->is_decl)
    pp_variable_declaration_list (init->data.decl_list);
  else
    pp_expression (init->data.expr);
}

static void
pp_for_statement (for_statement *for_stmt)
{
  assert (for_stmt);
  printf ("for (");
  if (for_stmt->init)
    pp_for_statement_initialiser_part (for_stmt->init);
  printf ("; ");
  if (for_stmt->limit)
    pp_expression (for_stmt->limit);
  printf ("; ");
  if (for_stmt->incr)
    pp_expression (for_stmt->incr);
  printf (") ");
}

static void
pp_for_or_for_in_statement (for_or_for_in_statement *for_or_for_in_stmt)
{
  assert (for_or_for_in_stmt);
  if (for_or_for_in_stmt->is_for_in)
    pp_for_in_statement (for_or_for_in_stmt->data.for_in_stmt);
  else
    pp_for_statement (for_or_for_in_stmt->data.for_stmt);
}

void
pp_statement (statement *stmt)
{
  was_function_expression = false;
  was_subexpression = false;
  assert (stmt);

  if (prev_stmt == STMT_BLOCK_END)
    {
      if (stmt->type == STMT_EMPTY)
        {
          printf (";\n");
          prev_stmt = stmt->type;
          return;
        }
      else
        putchar ('\n');
    }

  switch (stmt->type)
    {
    case STMT_BLOCK_START:
      printf ("{\n");
      intendation += 2;
      break;

    case STMT_BLOCK_END:
      intendation -= 2;
      intend ();
      printf ("}");
      break;

    case STMT_VARIABLE:
      intend ();
      pp_variable_declaration_list (stmt->data.var_stmt);
      break;

    case STMT_EMPTY:
      printf (";\n");
      break;

    case STMT_IF:
      intend ();
      printf ("if (");
      pp_expression (stmt->data.expr);
      printf (") ");
      break;

    case STMT_ELSE:
      intend ();
      printf ("else ");
      break;

    case STMT_ELSE_IF:
      intend ();
      printf ("else if(");
      pp_expression (stmt->data.expr);
      printf (") ");
      break;

    case STMT_DO:
      intend ();
      printf ("do ");
      break;

    case STMT_WHILE:
      intend ();
      printf ("while (");
      pp_expression (stmt->data.expr);
      printf (") ");
      break;

    case STMT_FOR_OR_FOR_IN:
      intend ();
      pp_for_or_for_in_statement (stmt->data.for_stmt);
      break;

    case STMT_CONTINUE:
      intend ();
      printf ("continue\n");
      break;

    case STMT_BREAK:
      intend ();
      printf ("break\n");
      break;

    case STMT_RETURN:
      intend ();
      printf ("return ");
      if (stmt->data.expr)
        pp_expression (stmt->data.expr);
      if (!was_function_expression)
        printf (";\n");
      break;

    case STMT_WITH:
      intend ();
      printf ("with (");
      pp_expression (stmt->data.expr);
      printf (") ");
      break;

    case STMT_LABELLED:
      intend ();
      printf ("%s:\n", stmt->data.name);
      break;

    case STMT_SWITCH:
      intend ();
      printf ("switch (");
      pp_expression (stmt->data.expr);
      printf (") ");
      break;

    case STMT_CASE:
      intend ();
      printf ("case ");
      pp_expression (stmt->data.expr);
      printf (":\n");
      break;

    case STMT_THROW:
      intend ();
      printf ("throw ");
      pp_expression (stmt->data.expr);
      printf (";\n");
      break;

    case STMT_TRY:
      intend ();
      printf ("try ");
      break;

    case STMT_CATCH:
      intend ();
      printf ("catch (");
      pp_expression (stmt->data.expr);
      printf (") ");
      break;

    case STMT_FINALLY:
      intend ();
      printf ("finally ");
      break;

    case STMT_EXPRESSION:
      intend ();
      pp_expression (stmt->data.expr);
      break;

    case STMT_SUBEXPRESSION_END:
      putchar (')');
      break;

    case STMT_FUNCTION:
      intend ();
      pp_function_declaration (stmt->data.fun_decl);
      break;

    default:
      unreachable ();
    }

  prev_stmt = stmt->type;
}

void pp_finish ()
{
  if (prev_stmt == STMT_BLOCK_END)
    putchar ('\n');
}

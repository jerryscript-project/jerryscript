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
#include "lexer.h"
#include "parser.h"

bool
is_formal_parameter_list_empty (formal_parameter_list list)
{
  return list.names[0] == NULL;
}

bool
is_operand_empty (operand op)
{
  return op.is_literal == false && op.data.none == NULL;
}

bool
is_operand_list_empty (operand_list list)
{
  return is_operand_empty (list.ops[0]);
}

bool
is_property_empty (property prop)
{
  return is_operand_empty (prop.name) && is_operand_empty (prop.value);
}

bool
is_property_list_empty (property_list list)
{
  return is_property_empty (list.props[0]);
}

bool
is_expression_empty (assignment_expression expr)
{
  return expr.oper == AO_NONE && expr.type == ET_NONE && expr.data.none == NULL;
}

bool
is_variable_declaration_empty (variable_declaration var_decl)
{
  return var_decl.name == NULL && is_expression_empty (var_decl.assign_expr);
}

bool
is_statement_null (statement stmt)
{
  return stmt.type == STMT_NULL && stmt.data.none == NULL;
}


static token tok;

#ifdef __HOST
FILE *debug_file;
#endif

static expression parse_expression (void);
static assignment_expression parse_assigment_expression (void);

typedef enum
{
  SCOPE_GLOBAL        = 0,
  SCOPE_IF            = 1u << 0,
  SCOPE_BLOCK         = 1u << 1,
  SCOPE_DO            = 1u << 2,
  SCOPE_WHILE         = 1u << 3,
  SCOPE_FOR           = 1u << 4,
  SCOPE_LOOP          = SCOPE_WHILE | SCOPE_FOR | SCOPE_DO,
  SCOPE_WITH          = 1u << 5,
  SCOPE_SWITCH        = 1u << 6,
  SCOPE_CASE          = 1u << 7,
  SCOPE_ELSE          = 1u << 8,
  SCOPE_TRY           = 1u << 9,
  SCOPE_CATCH         = 1u << 10,
  SCOPE_FINALLY       = 1u << 11,
  SCOPE_FUNCTION      = 1u << 12,
  SCOPE_SUBEXPRESSION = 1u << 13
}
scope_type;

typedef struct scope
{
  scope_type type;
  bool was_stmt;
}
scope;

#define MAX_SCOPES 10

static scope current_scopes[MAX_SCOPES];

static unsigned int scope_index;

static void
scope_must_be (unsigned int scopes)
{
  size_t i;

  for (i = 0; i < scope_index; i++)
    {
      if (scopes & current_scopes[i].type)
        return;
    }
  parser_fatal (ERR_PARSER);
}

static void
current_scope_must_be (unsigned int scopes)
{
  if (scopes & current_scopes[scope_index - 1].type)
    return;
  parser_fatal (ERR_PARSER);
}

static inline void
current_scope_must_be_global (void)
{
  if (scope_index != 1)
    parser_fatal (ERR_PARSER);
}

static void
push_scope (int type)
{
#ifdef __HOST
  __fprintf (debug_file, "push_scope: 0x%x\n", type);
#endif
  current_scopes[scope_index++] = (scope) { .type = type, .was_stmt = false };
}

static void
pop_scope (void)
{
#ifdef __HOST
  __fprintf (debug_file, "pop_scope: 0x%x\n", current_scopes[scope_index - 1].type);
#endif
  scope_index--;
}

static void
assert_keyword (keyword kw)
{
  if (tok.type != TOK_KEYWORD || tok.data.kw != kw)
    {
#ifdef __HOST
      __printf ("assert_keyword: 0x%x\n", kw);
#endif
      JERRY_UNREACHABLE ();
    }
} 

static bool
is_keyword (keyword kw)
{
  return tok.type == TOK_KEYWORD && tok.data.kw == kw;
}

static void
current_token_must_be(token_type tt) 
{
  if (tok.type != tt) 
    {
#ifdef __HOST
      __printf ("current_token_must_be: 0x%x\n", tt);
#endif
      parser_fatal (ERR_PARSER); 
    }
}

static void
skip_newlines (void)
{
  tok = lexer_next_token ();
  while (tok.type == TOK_NEWLINE)
    tok = lexer_next_token ();
}

static void
next_token_must_be (token_type tt)
{
  tok = lexer_next_token ();
  if (tok.type != tt)
    {
#ifdef __HOST
      __printf ("next_token_must_be: 0x%x\n", tt);
#endif
      parser_fatal (ERR_PARSER);
    }
}

static void
token_after_newlines_must_be (token_type tt)
{
  skip_newlines ();
  if (tok.type != tt)
    parser_fatal (ERR_PARSER);
}

static inline void
token_after_newlines_must_be_keyword (keyword kw)
{
  skip_newlines ();
  if (!is_keyword (kw))
    parser_fatal (ERR_PARSER);
}

static void
insert_semicolon (void)
{
  tok = lexer_next_token ();
  if (tok.type != TOK_NEWLINE && tok.type != TOK_SEMICOLON)
    parser_fatal (ERR_PARSER);
}

/* formal_parameter_list
	: LT!* Identifier (LT!* ',' LT!* Identifier)*
	; */
static formal_parameter_list
parse_formal_parameter_list (void)
{
  int i;
  formal_parameter_list res;

  current_token_must_be (TOK_NAME);

  for (i = 0; i < MAX_PARAMS; i++)
    {
      res.names[i] = tok.data.name;
      
      skip_newlines ();
      if (tok.type != TOK_COMMA)
        {
          lexer_save_token (tok);

          if (i != MAX_PARAMS - 1)
            res.names[i + 1] = NULL;
          break;
        }
    }
  return res;
}

/* function_declaration
	: 'function' LT!* Identifier LT!* 
	  '(' formal_parameter_list? LT!* ')' LT!* function_body
	;

   function_body
	: '{' LT!* sourceElements LT!* '}' */
static function_declaration
parse_function_declaration (void)
{
  function_declaration res;

  assert_keyword  (KW_FUNCTION);

  token_after_newlines_must_be (TOK_NAME);
  res.name = tok.data.name;

  token_after_newlines_must_be (TOK_OPEN_PAREN);

  skip_newlines ();
  if (tok.type != TOK_CLOSE_PAREN)
    {
      res.params = parse_formal_parameter_list ();
      next_token_must_be (TOK_CLOSE_PAREN);
    }
  else
    res.params = empty_formal_parameter_list;

  return res;
}

/* function_expression
	: 'function' LT!* Identifier? LT!* '(' formal_parameter_list? LT!* ')' LT!* function_body
	; */
static function_expression
parse_function_expression (void)
{
  function_expression res;

  assert_keyword  (KW_FUNCTION);

  skip_newlines ();
  if (tok.type == TOK_NAME)
    {
      res.name = tok.data.name;
      skip_newlines ();
    }
  else
    res.name = NULL;

  current_token_must_be (TOK_OPEN_PAREN);

  skip_newlines ();
  if (tok.type != TOK_CLOSE_PAREN)
    {
      res.params = parse_formal_parameter_list ();
      next_token_must_be (TOK_CLOSE_PAREN);
    }
  else
    res.params = empty_formal_parameter_list;

  push_scope (SCOPE_FUNCTION);

  return res;
}

static literal
parse_literal (void)
{
  literal res;

  switch (tok.type)
  {
    case TOK_NULL:
      res.type = LIT_NULL;
      res.data.none = NULL;
      return res;

    case TOK_BOOL:
      res.type = LIT_BOOL;
      res.data.is_true = tok.data.is_true;
      return res;

    case TOK_INT:
      res.type = LIT_INT;
      res.data.num = tok.data.num;
      return res;

    case TOK_STRING:
      res.type = LIT_STR;
      res.data.str = tok.data.str;
      return res;

    default:
      JERRY_UNREACHABLE ();
  }
}

static operand
parse_operand (void)
{
  operand res;

  switch (tok.type)
  {
    case TOK_NULL:
    case TOK_BOOL:
    case TOK_INT:
    case TOK_STRING:
      res.is_literal = true;
      res.data.lit = parse_literal ();
      return res;

    case TOK_NAME:
      res.is_literal = false;
      res.data.name = tok.data.name;
      return res;

    default:
      return empty_operand;
  }
}

/* arguments
  : operand LT!* ( ',' LT!* operand * LT!* )* 
  ;*/
static argument_list
parse_argument_list (void)
{
  argument_list res;
  int i;

  for (i = 0; i < MAX_PARAMS; i++)
    {
      res.ops[i] = parse_operand ();

      skip_newlines ();
      if (tok.type != TOK_COMMA)
        {
          lexer_save_token (tok);

          if (i != MAX_PARAMS - 1)
            res.ops[i + 1] = empty_operand;
          break;
        }
      
      skip_newlines ();
    }
  return res;
}

/* call_expression
  : identifier LT!* '(' LT!* arguments * LT!* ')' LT!* 
  ;*/
static call_expression
parse_call_expression (void)
{
  call_expression res;

  current_token_must_be (TOK_NAME);
  res.name = tok.data.name;

  token_after_newlines_must_be (TOK_OPEN_PAREN);

  skip_newlines ();
  if (tok.type != TOK_CLOSE_PAREN)
    {
      res.args = parse_argument_list ();
      next_token_must_be (TOK_CLOSE_PAREN);
    }
  else
    res.args = empty_operand_list;

  skip_newlines ();
  if (tok.type == TOK_OPEN_PAREN || tok.type == TOK_OPEN_SQUARE 
      || tok.type == TOK_DOT)
    JERRY_UNREACHABLE ();
  else
    lexer_save_token (tok);

  return res;
}

/* array_literal
  : [ arguments ]
  ; */
static array_literal 
parse_array_literal (void)
{
  array_literal res;

  current_token_must_be (TOK_OPEN_SQUARE);

  skip_newlines ();
  if (tok.type != TOK_CLOSE_SQUARE)
    {
      res = parse_argument_list ();
      next_token_must_be (TOK_CLOSE_SQUARE);
    }
  else
    res = empty_operand_list;

  return res;
}

/* property_name
  : Identifier
  | StringLiteral
  | NumericLiteral
  ; */
static inline property_name
parse_property_name (void)
{
  switch (tok.type)
  {
    case TOK_NAME:
    case TOK_STRING:
    case TOK_INT:
      return parse_operand ();
    
    default:
      JERRY_UNREACHABLE ();
  }
}

/* property_name_and_value
  : property_name LT!* ':' LT!* operand
  ; */
static property
parse_property (void)
{
  property res;

  res.name = parse_property_name ();
  token_after_newlines_must_be (TOK_COLON);

  skip_newlines ();
  res.value = parse_operand ();

  return res;
}

/* object_literal
  : LT!* property (LT!* ',' LT!* property)* LT!* 
  ; */
static object_literal
parse_object_literal (void)
{
  object_literal res;
  int i;

  for (i = 0; i < MAX_PROPERTIES; i++)
    { 
      res.props[i] = parse_property ();

      skip_newlines ();
      if (tok.type != TOK_COMMA)
        {
          lexer_save_token (tok);

          if (i != MAX_PROPERTIES - 1)
            res.props[i + 1] = empty_property;
          break;
        }
    }

  return res;
}

static void
parse_unary_expression (assignment_expression *res, expression_type type)
{
  res->type = type;
  token_after_newlines_must_be (TOK_NAME);
  res->data.ops.op1 = parse_operand ();
  res->data.ops.op2 = empty_operand;
}

static assignment_expression
parse_assigment_expression (void)
{
  assignment_expression res;

  if (tok.type != TOK_NAME)
    goto parse_operands;

  current_token_must_be (TOK_NAME);

  token saved_token = tok;

  skip_newlines ();
  if (tok.type == TOK_OPEN_PAREN)
    {
      lexer_save_token (tok);
      tok = saved_token;
      return (assignment_expression)
        {
          .oper = AO_NONE,
          .type = ET_CALL,
          .var = NULL,
          .data.call_expr = parse_call_expression ()
        };
    }

  res.var = saved_token.data.name;

  // Assume that this is assignment
  skip_newlines ();
  switch (tok.type)
    {  
      case TOK_EQ:
        res.oper = AO_EQ;
        break;
      case TOK_MULT_EQ:
        res.oper = AO_MULT_EQ;
        break;
      case TOK_DIV_EQ:
        res.oper = AO_DIV_EQ;
        break;
      case TOK_MOD_EQ:
        res.oper = AO_MOD_EQ;
        break;
      case TOK_PLUS_EQ:
        res.oper = AO_PLUS_EQ;
        break;
      case TOK_MINUS_EQ:
        res.oper = AO_MINUS_EQ;
        break;
      case TOK_LSHIFT_EQ:
        res.oper = AO_LSHIFT_EQ;
        break;
      case TOK_RSHIFT_EQ:
        res.oper = AO_RSHIFT_EQ;
        break;
      case TOK_RSHIFT_EX_EQ:
        res.oper = AO_RSHIFT_EX_EQ;
        break;
      case TOK_AND_EQ:
        res.oper = AO_AND_EQ;
        break;
      case TOK_XOR_EQ:
        res.oper = AO_XOR_EQ;
        break;
      case TOK_OR_EQ:
        res.oper = AO_OR_EQ;
        break;

    default:
      res.oper = AO_NONE;
      res.data.ops.op1 = (operand) { .is_literal = false, .data.name = res.var };
      res.var = NULL;
      goto parse_operator;
  }

  skip_newlines ();

parse_operands:
  saved_token = tok;
  switch (tok.type)
  {
    case TOK_NAME:
    case TOK_STRING:
    case TOK_INT:
    case TOK_NULL:
    case TOK_BOOL:
      res.data.ops.op1 = parse_operand ();
      break;

    case TOK_OPEN_PAREN:
      res.type = ET_SUBEXPRESSION;
      res.data.none = NULL;
      return res;

    case TOK_OPEN_BRACE:
      res.type = ET_OBJECT;
      res.data.obj_lit = parse_object_literal ();
      next_token_must_be (TOK_CLOSE_BRACE);
      return res;

    case TOK_OPEN_SQUARE:
      res.type = ET_ARRAY;
      res.data.arr_lit = parse_array_literal ();
      next_token_must_be (TOK_CLOSE_SQUARE);
      return res;

    case TOK_DOUBLE_PLUS:
      parse_unary_expression (&res, ET_UNARY_INCREMENT);
      return res;

    case TOK_DOUBLE_MINUS:
      parse_unary_expression (&res, ET_UNARY_DECREMENT);
      return res;

    case TOK_PLUS:
      parse_unary_expression (&res, ET_UNARY_PLUS);
      return res;

    case TOK_MINUS:
      parse_unary_expression (&res, ET_UNARY_MINUS);
      return res;

    case TOK_COMPL:
      parse_unary_expression (&res, ET_UNARY_COMPL);
      return res;

    case TOK_NOT:
      parse_unary_expression (&res, ET_UNARY_NOT);
      return res;

    case TOK_KEYWORD:
      switch (tok.data.kw)
      {  
        case KW_DELETE:
          parse_unary_expression (&res, ET_UNARY_DELETE);
          return res;
        case KW_VOID:
          parse_unary_expression (&res, ET_UNARY_VOID);
          return res;
        case KW_TYPEOF:
          parse_unary_expression (&res, ET_UNARY_TYPEOF);
          return res;

        case KW_FUNCTION:
          res.type = ET_FUNCTION;
          res.data.func_expr = parse_function_expression ();
          return res;

        case KW_NEW:
          parse_unary_expression (&res, ET_NEW);
          return res;

        default:
          JERRY_UNREACHABLE ();
      }

    default:
      JERRY_UNREACHABLE ();
  }

parse_operator:
  skip_newlines ();
  switch (tok.type)
  {
    case TOK_DOUBLE_OR:
      res.type = ET_LOGICAL_OR;
      break;
    case TOK_DOUBLE_AND:
      res.type = ET_LOGICAL_AND;
      break;
    case TOK_OR:
      res.type = ET_BITWISE_OR;
      break;
    case TOK_XOR:
      res.type = ET_BITWISE_XOR;
      break;
    case TOK_AND:
      res.type = ET_BITWISE_AND;
      break;
    case TOK_DOUBLE_EQ:
      res.type = ET_DOUBLE_EQ;
      break;
    case TOK_NOT_EQ:
      res.type = ET_NOT_EQ;
      break;
    case TOK_TRIPLE_EQ:
      res.type = ET_TRIPLE_EQ;
      break;
    case TOK_NOT_DOUBLE_EQ:
      res.type = ET_NOT_DOUBLE_EQ;
      break;
    case TOK_LESS:
      res.type = ET_LESS;
      break;
    case TOK_GREATER:
      res.type = ET_GREATER;
      break;
    case TOK_LESS_EQ:
      res.type = ET_LESS_EQ;
      break;
    case TOK_GREATER_EQ:
      res.type = ET_GREATER_EQ;
      break;
    case TOK_LSHIFT:
      res.type = ET_LSHIFT;
      break;
    case TOK_RSHIFT:
      res.type = ET_RSHIFT;
      break;
    case TOK_RSHIFT_EX:
      res.type = ET_RSHIFT_EX;
      break;
    case TOK_PLUS:
      res.type = ET_PLUS;
      break;
    case TOK_MINUS:
      res.type = ET_MINUS;
      break;
    case TOK_MULT:
      res.type = ET_MULT;
      break;
    case TOK_DIV:
      res.type = ET_DIV;
      break;
    case TOK_MOD:
      res.type = ET_MOD;
      break;

    case TOK_DOUBLE_PLUS:
      res.type = ET_POSTFIX_INCREMENT;
      res.data.ops.op2 = empty_operand;
      return res;

    case TOK_DOUBLE_MINUS:
      res.type = ET_POSTFIX_DECREMENT;
      res.data.ops.op2 = empty_operand;
      return res;

    case TOK_OPEN_PAREN:
      res.type = ET_CALL;
      lexer_save_token (tok);
      tok = saved_token;
      res.data.call_expr = parse_call_expression ();
      return res;

    case TOK_DOT:
      res.type = ET_PROP_REF;
      skip_newlines ();
      res.data.ops.op2 = parse_operand ();
      JERRY_ASSERT (!res.data.ops.op2.is_literal);
      return res;

    case TOK_OPEN_SQUARE:
      res.type = ET_INDEX;
      skip_newlines ();
      res.data.ops.op2 = parse_operand ();
      token_after_newlines_must_be (TOK_CLOSE_SQUARE);
      return res;

    case TOK_KEYWORD:
      switch (tok.data.kw)
      {
        case KW_INSTANCEOF:
          res.type = ET_INSTANCEOF;
          break;
        case KW_IN:
          res.type = ET_IN;
          break;

        default:
          JERRY_UNREACHABLE ();
      }
      break;

    default:
      lexer_save_token (tok);
      res.oper = AO_NONE;
      res.type = ET_NONE; 
      res.data.ops.op2 = empty_operand;
      return res;
  }

  skip_newlines ();
  res.data.ops.op2 = parse_operand ();
  return res;
}

/* expression
  : assignment_expression (LT!* ',' LT!* assignment_expression)*
  ;
  */
static expression
parse_expression (void)
{
  expression res;
  int i;

  for (i = 0; i < MAX_EXPRS; i++)
    {
      res.exprs[i] = parse_assigment_expression ();
      skip_newlines ();
      if (tok.type != TOK_COMMA)
        {
          lexer_save_token (tok);
          if (i != MAX_EXPRS - 1)
            res.exprs[i + 1] = empty_expression;
          break;
        }
    }

  return res;
}

/* variable_declaration(_no_in)
	: Identifier LT!* initialiser(_no_in)?
	;
   initialiser(_no_in)
	: '=' LT!* assignment_expression
	; */
static variable_declaration
parse_variable_declaration (void)
{
  variable_declaration res;

  current_token_must_be (TOK_NAME);
  res.name = tok.data.name;

  skip_newlines ();
  if (tok.type == TOK_EQ)
    {
      skip_newlines ();
      res.assign_expr = parse_assigment_expression ();
    }
  else
    {
      lexer_save_token (tok);
      res.assign_expr = empty_expression;
    }

  return res;
}

/* variable_declaration_list(_no_in)
	: variable_declaration(_no_in) 
	  (LT!* ',' LT!* variable_declaration(_no_in))*
	; */
static variable_declaration_list
parse_variable_declaration_list (void)
{
  variable_declaration_list res;
  int i;

  for (i = 0; i < MAX_DECLS; i++)
    {
      res.decls[i] = parse_variable_declaration ();
      skip_newlines ();
      if (tok.type != TOK_COMMA)
        {
          lexer_save_token (tok);
          if (i != MAX_DECLS - 1)
            res.decls[i + 1] = empty_variable_declaration;
          break;
        }
    }

  return res;
}

/* for_statement
	: 'for' LT!* '(' (LT!* for_statement_initialiser_part)? LT!* ';' 
	  (LT!* expression)? LT!* ';' (LT!* expression)? LT!* ')' LT!* statement
	;

   for_statement_initialiser_part
  : expression_no_in
  | 'var' LT!* variable_declaration_list_no_in
  ;

   for_in_statement
	: 'for' LT!* '(' LT!* for_in_statement_initialiser_part LT!* 'in' 
	  LT!* expression LT!* ')' LT!* statement
	; 

   for_in_statement_initialiser_part
  : left_hand_side_expression
  | 'var' LT!* variable_declaration_no_in
  ;*/

static for_or_for_in_statement
parse_for_or_for_in_statement (void)
{
  for_or_for_in_statement res;
  variable_declaration_list list;
  expression expr;
  expr.exprs[0] = empty_expression;
  list.decls[0] = empty_variable_declaration;

  assert_keyword  (KW_FOR);
  token_after_newlines_must_be (TOK_OPEN_PAREN);

  skip_newlines ();
  if (tok.type == TOK_SEMICOLON)
    goto plain_for;
  /* Both for_statement_initialiser_part and for_in_statement_initialiser_part
     contains 'var'. Check it first.  */
  if (is_keyword (KW_VAR))
    {
      skip_newlines ();
      list = parse_variable_declaration_list ();
      if (!is_variable_declaration_empty (list.decls[1]))
        {
          token_after_newlines_must_be (TOK_SEMICOLON);
          goto plain_for;
        }
      else
        {
          skip_newlines ();
          if (tok.type == TOK_SEMICOLON)
            goto plain_for;
          else if (is_keyword (KW_IN))
            goto for_in;
          else
            parser_fatal (ERR_PARSER);
        }
    }
  JERRY_ASSERT (is_variable_declaration_empty(list.decls[0]));

  /* expression contains left_hand_side_expression.  */
  expr = parse_expression ();

  skip_newlines ();
  if (tok.type == TOK_SEMICOLON)
    goto plain_for;
  else if (is_keyword (KW_IN))
    goto for_in;
  else 
    parser_fatal (ERR_PARSER);

  JERRY_UNREACHABLE ();

plain_for:
  res.is_for_in = false;
  if (!is_variable_declaration_empty(list.decls[0]))
    {
      JERRY_ASSERT (is_expression_empty(expr.exprs[0]));
      res.data.for_stmt.init.is_decl = true;
      res.data.for_stmt.init.data.decl_list = list;
    }
  if (!is_expression_empty(expr.exprs[0]))
    {
      JERRY_ASSERT (is_variable_declaration_empty(list.decls[0]));
      res.data.for_stmt.init.is_decl = false;
      res.data.for_stmt.init.data.expr = expr;
    }

  skip_newlines ();
  if (tok.type == TOK_SEMICOLON)
    res.data.for_stmt.limit = empty_expression;
  else
    {
      res.data.for_stmt.limit = parse_assigment_expression ();
      next_token_must_be (TOK_SEMICOLON);
    }

  skip_newlines ();
  if (tok.type == TOK_CLOSE_PAREN)
    res.data.for_stmt.incr = empty_expression;
  else
    {
      res.data.for_stmt.incr = parse_assigment_expression ();
      next_token_must_be (TOK_CLOSE_PAREN);
    }
  return res;

for_in:
  res.is_for_in = true;
  if (!is_variable_declaration_empty(list.decls[0]))
    {
      JERRY_ASSERT (is_expression_empty (expr.exprs[0]));
      JERRY_ASSERT (is_variable_declaration_empty (list.decls[1]));
      res.data.for_in_stmt.init.is_decl = true;
      res.data.for_in_stmt.init.data.decl = list.decls[0];
    }
  if (!is_expression_empty(expr.exprs[0]))
    { 
      JERRY_ASSERT (is_expression_empty(expr.exprs[0]));
      res.data.for_in_stmt.init.is_decl = false;
      res.data.for_in_stmt.init.data.left_hand_expr = expr.exprs[0];
    }

  skip_newlines ();
  res.data.for_in_stmt.list_expr = parse_expression ();
  token_after_newlines_must_be (TOK_CLOSE_PAREN);

  return res;
}

static void
parse_expression_inside_parens (statement *res)
{
  token_after_newlines_must_be (TOK_OPEN_PAREN);
  skip_newlines ();
  res->data.expr = parse_expression ();
  token_after_newlines_must_be (TOK_CLOSE_PAREN);
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
statement
parser_parse_statement (void)
{
  statement res;
  res.data.none = NULL;

  JERRY_ASSERT (scope_index);

  skip_newlines ();

  if (is_keyword (KW_FINALLY))
    {
      res.type = STMT_FINALLY;
      current_scope_must_be (SCOPE_TRY | SCOPE_CATCH);
      pop_scope ();
      push_scope (SCOPE_FINALLY);
      return res;
    }

  if (current_scopes[scope_index - 1].was_stmt 
      && (current_scopes[scope_index - 1].type & (SCOPE_IF | SCOPE_DO | SCOPE_WITH | SCOPE_SWITCH | SCOPE_ELSE 
                                 | SCOPE_CATCH | SCOPE_FINALLY | SCOPE_FUNCTION | SCOPE_WHILE
                                 | SCOPE_FOR)))
    pop_scope ();

  current_scopes[scope_index - 1].was_stmt = true;

  if (tok.type == TOK_EOF)
    {
      current_scope_must_be_global ();
      res.type = STMT_EOF;
      return res;
    }

  if (current_scopes[scope_index - 1].type == SCOPE_SUBEXPRESSION)
    {
      if (tok.type == TOK_CLOSE_PAREN)
        {
          res.type = STMT_SUBEXPRESSION_END;
          pop_scope ();
          return res;
        }
      res.type = STMT_EXPRESSION;
      res.data.expr = parse_expression ();
      return res;
    }
  if (tok.type == TOK_OPEN_BRACE)
    {
      res.type = STMT_BLOCK_START;
      push_scope (SCOPE_BLOCK);
      return res;
    }
  if (tok.type == TOK_CLOSE_BRACE)
    {
      current_scope_must_be (SCOPE_BLOCK);
      res.type = STMT_BLOCK_END;
      pop_scope ();
      current_scopes[scope_index - 1].was_stmt = true;
      return res;
    }
  if (is_keyword (KW_ELSE))
    {
      current_scope_must_be (SCOPE_IF);
      skip_newlines ();
      if (is_keyword (KW_IF))
        {
          res.type = STMT_ELSE_IF;
          parse_expression_inside_parens (&res);
          return res;
        }
      else
        {
          lexer_save_token (tok);
          res.type = STMT_ELSE;
          pop_scope ();
          push_scope (SCOPE_ELSE);
          return res;
        }
    }
  if (is_keyword (KW_WHILE))
    {
      res.type = STMT_WHILE;
      parse_expression_inside_parens (&res);
      if (current_scopes[scope_index - 1].type == SCOPE_DO)
        {
          insert_semicolon ();
          pop_scope ();
        }
      else
        push_scope (SCOPE_WHILE);
      return res;
    }
  if (is_keyword (KW_CATCH))
    {
      res.type = STMT_CATCH;
      current_scope_must_be (SCOPE_TRY);
      parse_expression_inside_parens (&res);
      pop_scope ();
      push_scope (SCOPE_CATCH);
      return res;
    }
  if (is_keyword (KW_FUNCTION))
    {
      res.type = STMT_FUNCTION;
      res.data.fun_decl = parse_function_declaration ();
      push_scope (SCOPE_FUNCTION);
      return res;
    }
  if (is_keyword (KW_VAR))
    {
      res.type = STMT_VARIABLE;

      skip_newlines ();
      res.data.var_stmt = parse_variable_declaration_list ();
      return res;
    }
  if (tok.type == TOK_SEMICOLON)
    {
      res.type = STMT_EMPTY;
      return res;
    }
  if (is_keyword (KW_IF))
    {
      res.type = STMT_IF;
      parse_expression_inside_parens (&res);
      push_scope (SCOPE_IF);
      return res;
    }
  if (is_keyword (KW_DO))
    {
      res.type = STMT_DO;
      push_scope (SCOPE_DO);
      return res;
    }
  if (is_keyword (KW_FOR))
    {
      res.type = STMT_FOR_OR_FOR_IN;
      res.data.for_stmt = parse_for_or_for_in_statement ();
      push_scope (SCOPE_FOR);
      return res;
    }
  if (is_keyword (KW_CONTINUE))
    {
      scope_must_be (SCOPE_LOOP);
      res.type = STMT_CONTINUE;
      tok = lexer_next_token ();
      if (tok.type == TOK_NAME)
        res.data.name = tok.data.name;
      else
        lexer_save_token (tok);
      insert_semicolon ();
      return res;
    }
  if (is_keyword (KW_BREAK))
    {
      scope_must_be (SCOPE_LOOP | SCOPE_CASE);
      res.type = STMT_BREAK;
      tok = lexer_next_token ();
      if (tok.type == TOK_NAME)
        {
          if (current_scopes[scope_index - 1].type == SCOPE_CASE)
            parser_fatal (ERR_PARSER);
          res.data.name = tok.data.name;
        }
      else
        lexer_save_token (tok);
      insert_semicolon ();
      return res;
    }
  if (is_keyword (KW_RETURN))
    {
      scope_must_be (SCOPE_FUNCTION);
      res.type = STMT_RETURN;
      tok = lexer_next_token ();
      if (tok.type != TOK_SEMICOLON && tok.type != TOK_NEWLINE)
        {
          unsigned int current_scope_index = scope_index;
          res.data.expr = parse_expression ();
          if (current_scope_index == scope_index)
            insert_semicolon ();
        }
      // Eat TOK_SEMICOLON
      return res;
    }
  if (is_keyword (KW_WITH))
    {
      res.type = STMT_WITH;
      parse_expression_inside_parens (&res);
      push_scope (SCOPE_WITH);
      return res;
    }
  if (is_keyword (KW_SWITCH))
    {
      res.type = STMT_SWITCH;
      parse_expression_inside_parens (&res);
      push_scope (SCOPE_SWITCH);
      return res;
    }
  if (is_keyword (KW_THROW))
    {
      res.type = STMT_THROW;
      tok = lexer_next_token ();
      res.data.expr = parse_expression ();
      insert_semicolon ();
      return res;
    }
  if (is_keyword (KW_TRY))
    {
      res.type = STMT_TRY;
      push_scope (SCOPE_TRY);
      return res;
    }
  if (is_keyword (KW_CASE))
    {
      if (current_scopes[scope_index - 1].type == SCOPE_CASE)
        pop_scope ();
      current_scope_must_be (SCOPE_SWITCH);
      skip_newlines ();
      res.data.expr = parse_expression ();
      token_after_newlines_must_be (TOK_SEMICOLON);
      push_scope (SCOPE_CASE);
      return res;
    }
  if (is_keyword (KW_DEFAULT))
    {
      if (current_scopes[scope_index - 1].type == SCOPE_CASE)
        pop_scope ();
      current_scope_must_be (SCOPE_SWITCH);
      token_after_newlines_must_be (TOK_SEMICOLON);
      push_scope (SCOPE_CASE);
      return res;
    }
  if (tok.type == TOK_NAME)
    {
      token saved = tok;
      skip_newlines ();
      if (tok.type == TOK_COLON)
        {
          res.type = STMT_LABELLED;
          res.data.name = saved.data.name;
          return res;
        }
      else
        {
          lexer_save_token (tok);
          tok = saved;
          expression expr = parse_expression ();
          res.type = STMT_EXPRESSION;
          res.data.expr = expr;
          return res;
        }
    }

  expression expr = parse_expression ();
  if (!is_expression_empty (expr.exprs[0]))
    {
      res.type = STMT_EXPRESSION;
      res.data.expr = expr;
      return res;
    }
  else
    {
      lexer_save_token (tok);
      return null_statement;
    }
}

void
parser_init (void)
{
  scope_index = 1;
#ifdef __HOST
  debug_file = __fopen ("parser.log", "w");
#endif
}

void
parser_fatal (jerry_Status_t code)
{
  __printf ("FATAL: %d\n", code);
  lexer_dump_buffer_state ();

  jerry_Exit( code);
}

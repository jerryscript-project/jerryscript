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

#include <stdlib.h>
#include "parser.h"
#include "error.h"
#include "lexer.h"

static token tok;

#ifdef DEBUG
FILE *debug_file;
#endif

static expression *parse_expression (bool);
static assignment_expression *parse_assigment_expression (bool);

#define ALLOCATE(T)	(T*) malloc (sizeof (T))

#define RETURN_NULL_IF_NULL(T) do { if (T == NULL) { free (res); return NULL; } } while (0)

typedef enum
{
  SCOPE_GLOBAL        = 0,
  SCOPE_IF            = 1 << 0,
  SCOPE_BLOCK         = 1 << 1,
  SCOPE_DO            = 1 << 2,
  SCOPE_WHILE         = 1 << 3,
  SCOPE_FOR           = 1 << 4,
  SCOPE_LOOP          = SCOPE_WHILE | SCOPE_FOR | SCOPE_DO,
  SCOPE_WITH          = 1 << 5,
  SCOPE_SWITCH        = 1 << 6,
  SCOPE_CASE          = 1 << 7,
  SCOPE_ELSE          = 1 << 8,
  SCOPE_TRY           = 1 << 9,
  SCOPE_CATCH         = 1 << 10,
  SCOPE_FINALLY       = 1 << 11,
  SCOPE_FUNCTION      = 1 << 12,
  SCOPE_SUBEXPRESSION = 1 << 13
}
scope_type;

typedef struct scope
{
  scope_type type;
  bool was_stmt;

  struct scope *prev;
}
scope;

static scope *current_scope;

static inline void
scope_must_be (int scopes)
{
  scope *sc = current_scope;
  while (sc)
    {
      if (scopes & sc->type)
        return;
      sc = sc->prev;
    }
  fatal (ERR_PARSER);
}

static inline void
current_scope_must_be (int scopes)
{
  if (scopes & current_scope->type)
    return;
  fatal (ERR_PARSER);
}

static inline void
current_scope_must_be_global ()
{
  if (current_scope->prev)
    fatal (ERR_PARSER);
}

static void
push_scope (int type)
{
#ifdef DEBUG
  fprintf (debug_file, "push_scope: 0x%x\n", type);
#endif
  scope *sc = ALLOCATE (scope);
  sc->type = type;
  sc->prev = current_scope;
  sc->was_stmt = false;
  current_scope = sc;
}

static void
pop_scope ()
{
#ifdef DEBUG
  fprintf (debug_file, "pop_scope: 0x%x\n", current_scope->type);
#endif
  scope *sc = current_scope->prev;
  free (current_scope);
  current_scope = sc;
}

static inline void
assert_keyword (keyword kw)
{
  if (tok.type != TOK_KEYWORD || tok.data.kw != kw)
    {
      printf ("assert_keyword: 0x%x\n", kw);
      unreachable ();
    }
} 

static inline bool
is_keyword (keyword kw)
{
  return tok.type == TOK_KEYWORD && tok.data.kw == kw;
}

static inline void
current_token_must_be(token_type tt) 
{
  if (tok.type != tt) 
    {
      printf ("current_token_must_be: 0x%x\n", tt);
      fatal (ERR_PARSER); 
    }
}

static inline void
skip_newlines ()
{
  tok = lexer_next_token ();
  while (tok.type == TOK_NEWLINE)
    tok = lexer_next_token ();
}

static inline void
next_token_must_be (token_type tt)
{
  tok = lexer_next_token ();
  if (tok.type != tt)
    {
      printf ("next_token_must_be: 0x%x\n", tt);
      fatal (ERR_PARSER);
    }
}

static inline void
token_after_newlines_must_be (token_type tt)
{
  skip_newlines ();
  if (tok.type != tt)
    fatal (ERR_PARSER);
}

static inline void
token_after_newlines_must_be_keyword (keyword kw)
{
  skip_newlines ();
  if (!is_keyword (kw))
    fatal (ERR_PARSER);
}

static inline void
fatal_if_null (void * what)
{
  if (!what)
    fatal (ERR_PARSER);
}

static inline void
insert_semicolon ()
{
  tok = lexer_next_token ();
  if (tok.type != TOK_NEWLINE && tok.type != TOK_SEMICOLON)
    fatal (ERR_PARSER);
}

/* formal_parameter_list
	: LT!* Identifier (LT!* ',' LT!* Identifier)*
	; */
static formal_parameter_list *
parse_formal_parameter_list ()
{
  formal_parameter_list *res = ALLOCATE (formal_parameter_list);

  current_token_must_be (TOK_NAME);
  res->name = tok.data.name;

  skip_newlines ();
  if (tok.type == TOK_COMMA)
    {
      skip_newlines ();
      res->next = parse_formal_parameter_list ();
    }
  else
    {
      lexer_save_token (tok);
      res->next = NULL;
    }

  return res;
}

/* function_declaration
	: 'function' LT!* Identifier LT!* 
	  '(' formal_parameter_list? LT!* ')' LT!* function_body
	;

   function_body
	: '{' LT!* sourceElements LT!* '}' */
static function_declaration*
parse_function_declaration ()
{
  function_declaration *res = ALLOCATE (function_declaration);

  assert_keyword  (KW_FUNCTION);

  token_after_newlines_must_be (TOK_NAME);
  res->name = tok.data.name;

  token_after_newlines_must_be (TOK_OPEN_PAREN);

  skip_newlines ();
  if (tok.type != TOK_CLOSE_PAREN)
    {
      res->params = parse_formal_parameter_list ();
      next_token_must_be (TOK_CLOSE_PAREN);
    }
  else
    res->params = NULL;

  return res;
}

/* function_expression
	: 'function' LT!* Identifier? LT!* '(' formal_parameter_list? LT!* ')' LT!* function_body
	; */
static function_expression *
parse_function_expression ()
{
  function_expression *res = ALLOCATE (function_expression);

  assert_keyword  (KW_FUNCTION);

  skip_newlines ();
  if (tok.type == TOK_NAME)
    {
      res->name = tok.data.name;
      skip_newlines ();
    }
  else
    res->name = NULL;

  current_token_must_be (TOK_OPEN_PAREN);

  skip_newlines ();
  if (tok.type != TOK_CLOSE_PAREN)
    {
      res->params = parse_formal_parameter_list ();
      next_token_must_be (TOK_CLOSE_PAREN);
    }
  else
    res->params = NULL;

  push_scope (SCOPE_FUNCTION);

  return res;
}

/* array_literal
  : assignment_expression? (LT!* ',' (LT!* assignment_expression)?)* LT!*
  ; */
static array_literal *
parse_array_literal ()
{
  array_literal *res = NULL;
  assignment_expression *assign_expr = parse_assigment_expression (false);
  if (assign_expr != NULL)
    {
      res = ALLOCATE (array_literal);
      res->assign_expr = assign_expr;
    }
  skip_newlines ();
  if (tok.type == TOK_COMMA)
    {
      if (res == NULL)
        res = ALLOCATE (array_literal);
      skip_newlines ();
      res->next = parse_array_literal ();
    }
  else
    lexer_save_token (tok);

  return res;
}

/* property_name
  : Identifier
  | StringLiteral
  | NumericLiteral
  ; */
static property_name *
parse_property_name ()
{
  property_name *res = ALLOCATE (property_name);
  
  switch (tok.type)
  {
    case TOK_NAME:
      res->type = PN_NAME;
      res->data.name = tok.data.name;
      return res;

    case TOK_STRING:
      res->type = PN_STRING;
      res->data.str = tok.data.str;
      return res;

    case TOK_INT:
      res->type = PN_NUM;
      res->data.num = tok.data.num;
      return res;

    default:
      unreachable ();
  }
}

/* property_name_and_value
  : property_name LT!* ':' LT!* assignment_expression
  ; */
static property_name_and_value *
parse_property_name_and_value ()
{
  property_name_and_value *res = ALLOCATE (property_name_and_value);

  res->name = parse_property_name ();
  assert (res->name);
  token_after_newlines_must_be (TOK_COLON);

  skip_newlines ();
  res->assign_expr = parse_assigment_expression (true);
  fatal_if_null (res->assign_expr);

  return res;
}

/* object_literal
  : LT!* property_name_and_value (LT!* ',' LT!* property_name_and_value)* LT!* 
  ; */
static object_literal *
parse_object_literal ()
{
  object_literal *res = ALLOCATE (object_literal);

  res->nav = parse_property_name_and_value ();
  assert (res->nav);

  skip_newlines ();
  if (tok.type == TOK_COMMA)
    {
      skip_newlines ();
      res->next = parse_object_literal ();
      assert (res->next);
    }
  else
    {
      lexer_save_token (tok);
      res->next = NULL; 
    }
  return res;
}

static literal *
parse_literal ()
{
  literal *res = ALLOCATE (literal);

  switch (tok.type)
  {
    case TOK_NULL:
      res->type = LIT_NULL;
      res->data.data = NULL;
      return res;

    case TOK_BOOL:
      res->type = LIT_BOOL;
      res->data.is_true = tok.data.is_true;
      return res;

    case TOK_INT:
      res->type = LIT_INT;
      res->data.num = tok.data.num;
      return res;

    case TOK_STRING:
      res->type = LIT_STR;
      res->data.str = tok.data.str;
      return res;

    default:
      unreachable ();
  }
}

/* primary_expression
  : 'this'
  | Identifier
  | literal
  | '[' LT!* array_literal LT!* ']'
  | '{' LT!* object_literal LT!* '}'
  | '(' LT!* expression LT!* ')'
  ; */
static primary_expression *
parse_primary_expression ()
{
  primary_expression *res = ALLOCATE (primary_expression);

  if (is_keyword (KW_THIS))
    {
      res->type = PE_THIS;
      res->data.none = NULL;
      return res;
    }
  else if (tok.type == TOK_NAME)
    {
      res->type = PE_NAME;
      res->data.name = tok.data.name;
      return res;
    }
  else if (tok.type == TOK_NULL || tok.type == TOK_BOOL 
          || tok.type == TOK_INT || tok.type == TOK_STRING)
    {
      res->type = PE_LITERAL;
      res->data.lit = parse_literal ();
      assert (res->data.lit);
      return res;
    }
  else if (tok.type == TOK_OPEN_SQUARE)
    {
      res->type = PE_ARRAY;
      skip_newlines ();
      if (tok.type != TOK_CLOSE_SQUARE)
        {
          res->data.array_lit = parse_array_literal ();
          assert (res->data.array_lit);
          token_after_newlines_must_be (TOK_CLOSE_SQUARE);
        }
      else
        res->data.array_lit = NULL;
      return res;
    }
  else if (tok.type == TOK_OPEN_BRACE)
    {
      res->type = PE_OBJECT;
      skip_newlines ();
      if (tok.type != TOK_CLOSE_BRACE)
        {
          res->data.object_lit = parse_object_literal ();
          assert (res->data.object_lit);
          token_after_newlines_must_be (TOK_CLOSE_BRACE);
        }
      else
        res->data.object_lit = NULL;
      return res;
    }
  else if (tok.type == TOK_OPEN_PAREN)
    {
      res->type = PE_EXPR;
      skip_newlines ();
      if (tok.type != TOK_CLOSE_PAREN)
        {
          scope *old_scope;
          push_scope (SCOPE_SUBEXPRESSION);
          old_scope = current_scope;
          
          res->data.expr = parse_expression (false);
          if (current_scope == old_scope)
            {
              next_token_must_be (TOK_CLOSE_PAREN);
              pop_scope ();
            }
          assert (res->data.expr);
        }
      else
        res->data.expr = NULL;

      return res;
    }
  else
    {
      free (res);
      return NULL;
    }
}

/* member_expression_suffix
  : index_suffix
  | property_reference_suffix
  ;
  
   index_suffix
  : '[' LT!* expression LT!* ']'
  ; 
  
   property_reference_suffix
  : '.' LT!* Identifier
  ; */
static member_expression_suffix *
parse_member_expression_suffix ()
{
  member_expression_suffix *res = ALLOCATE (member_expression_suffix);

  if (tok.type == TOK_OPEN_SQUARE)
    {
      res->type = MES_INDEX;

      skip_newlines ();
      res->data.index_expr = parse_expression (true);
      fatal_if_null (res->data.index_expr);
      token_after_newlines_must_be (TOK_CLOSE_SQUARE);
    }
  else if (tok.type == TOK_DOT)
    {
      res->type = MES_PROPERTY;

      skip_newlines ();
      if (tok.type != TOK_NAME)
        fatal (ERR_PARSER);
      res->data.name = tok.data.name;
    }
  else 
    unreachable ();

  return res;
}

/* member_expression_suffix_list 
  : member_expression_suffix +
  ; */
static member_expression_suffix_list *
parse_member_expression_suffix_list ()
{
  member_expression_suffix_list *res = ALLOCATE (member_expression_suffix_list);

  if (tok.type == TOK_OPEN_SQUARE || tok.type == TOK_DOT)
    {
      res->suffix = parse_member_expression_suffix ();
      assert (res->suffix);
    }
  else
    unreachable ();
  
  skip_newlines ();
  if (tok.type == TOK_OPEN_SQUARE || tok.type == TOK_DOT)
    {
      res->next = parse_member_expression_suffix_list ();
      assert (res->next);
    }
  else
    {
      lexer_save_token (tok);
      res->next = NULL;
    }

  return res;
}

/* member_expression
  : (primary_expression | function_expression | 'new' LT!* member_expression (LT!* '(' LT!* arguments? LT!* ')') 
      (LT!* member_expression_suffix)*
  ; 

   arguments
  : assignment_expression (LT!* ',' LT!* assignment_expression)*)?
  ;*/
static member_expression *
parse_member_expression ()
{
  member_expression *res = ALLOCATE (member_expression);

  if (is_keyword (KW_FUNCTION))
    {
      res->type = ME_FUNCTION;
      res->data.function_expr = parse_function_expression ();
      assert (res->data.function_expr);
    }
  else if (is_keyword (KW_NEW))
    {
      member_expression_with_arguments *expr 
          = ALLOCATE (member_expression_with_arguments);
      res->type = ME_ARGS;

      skip_newlines ();
      expr->member_expr = parse_member_expression ();
      fatal_if_null (expr->member_expr);

      token_after_newlines_must_be (TOK_OPEN_PAREN);
      skip_newlines ();
      if (tok.type != TOK_CLOSE_PAREN)
        {
          expr->args = parse_expression (true);
          assert (expr->args);
          token_after_newlines_must_be (TOK_CLOSE_PAREN);
        }
      else
        expr->args = NULL;

      res->data.args = expr;
    }
  else
    {
      res->type = ME_PRIMARY;
      res->data.primary_expr = parse_primary_expression ();
      RETURN_NULL_IF_NULL (res->data.primary_expr);
    }

  skip_newlines ();
  if (tok.type == TOK_OPEN_SQUARE || tok.type == TOK_DOT)
    {
      res->suffix_list = parse_member_expression_suffix_list ();
      assert (res->suffix_list);
    }
  else
    {
      lexer_save_token (tok);
      res->suffix_list = NULL; 
    }

  return res;
}

/* new_expression
  : member_expression
  | 'new' LT!* new_expression
  ; */
static new_expression *
parse_new_expression ()
{
  new_expression *res = ALLOCATE (new_expression);

  if (is_keyword (KW_NEW))
    {
      member_expression *expr = parse_member_expression ();
      if (expr == NULL)
        {
          res->type = NE_NEW;

          skip_newlines ();
          res->data.new_expr = parse_new_expression ();
          assert (res->data.new_expr);
          return res;
        }
      else
        {
          res->type = NE_MEMBER;
          res->data.member_expr = expr;
          return res;
        }
    }
  else
    {
      res->type = NE_MEMBER;
      res->data.member_expr = parse_member_expression ();
      RETURN_NULL_IF_NULL (res->data.member_expr);
      return res;
    }
}

/* call_expression_suffix
  : arguments
  | index_suffix
  | property_reference_suffix
  ; */
static call_expression_suffix *
parse_call_expression_suffix ()
{
  call_expression_suffix *res = ALLOCATE (call_expression_suffix);

  if (tok.type == TOK_OPEN_PAREN)
    {
      res->type = CAS_ARGS;

      skip_newlines ();
      res->data.args = parse_expression (true);
      fatal_if_null (res->data.args);
      token_after_newlines_must_be (TOK_CLOSE_PAREN);

      return res;
    }
  else if (tok.type == TOK_OPEN_SQUARE)
    {
      res->type = CAS_INDEX;

      skip_newlines ();
      res->data.index_expr = parse_expression (true);
      fatal_if_null (res->data.index_expr);
      token_after_newlines_must_be (TOK_CLOSE_SQUARE);

      return res;
    }
  else if (tok.type == TOK_DOT)
    {
      res->type = CAS_PROPERTY;

      token_after_newlines_must_be (TOK_NAME);
      res->data.name = tok.data.name;
      return res;
    }
  else
    unreachable ();
}

/* call_expression_suffix_list
  : call_expression_suffix+
  ; */
static call_expression_suffix_list *
parse_call_expression_suffix_list ()
{
  call_expression_suffix_list *res = ALLOCATE (call_expression_suffix_list);

  if (tok.type == TOK_OPEN_PAREN || tok.type == TOK_OPEN_SQUARE 
      || tok.type == TOK_DOT)
    {
      res->suffix = parse_call_expression_suffix ();
      assert (res->suffix);
    }
  else
    unreachable ();

  skip_newlines ();
  if (tok.type == TOK_OPEN_PAREN || tok.type == TOK_OPEN_SQUARE 
      || tok.type == TOK_DOT)
    {
      res->next = parse_call_expression_suffix_list ();
      assert (res->next);
    }
  else
    {
      lexer_save_token (tok);
      res->next = NULL;
    }

  return res;
}

/* call_expression
  : member_expression LT!* arguments (LT!* call_expression_suffix)*
  ;*/
static call_expression *
parse_call_expression (member_expression *member_expr)
{
  call_expression *res = ALLOCATE (call_expression);

  assert (member_expr);
  res->member_expr = member_expr;

  assert (tok.type == TOK_OPEN_PAREN);
  skip_newlines ();
  if (tok.type != TOK_CLOSE_PAREN)
    {
      res->args = parse_expression (true);
      token_after_newlines_must_be (TOK_CLOSE_PAREN);
    }
  else
    res->args = NULL;

  skip_newlines ();
  if (tok.type == TOK_OPEN_PAREN || tok.type == TOK_OPEN_SQUARE 
      || tok.type == TOK_DOT)
    {
      res->suffix_list = parse_call_expression_suffix_list ();
      assert (res->suffix_list);
    }
  else
    lexer_save_token (tok);

  return res;
}

/* left_hand_side_expression
  : call_expression
  | new_expression
  ; */
static left_hand_side_expression *
parse_left_hand_side_expression ()
{
  left_hand_side_expression *res = ALLOCATE (left_hand_side_expression);

  if (is_keyword (KW_NEW))
    {
      res->type = LHSE_NEW;
      res->data.new_expr = parse_new_expression ();
      assert (res->data.new_expr);
      return res;
    }
  else
    {
      member_expression *member_expr = parse_member_expression ();
      RETURN_NULL_IF_NULL (member_expr);
      skip_newlines ();
      if (tok.type == TOK_OPEN_PAREN)
        {
          res->type = LHSE_CALL;
          res->data.call_expr = parse_call_expression (member_expr);
          assert (res->data.call_expr);
          return res;
        }
      else
        {
          lexer_save_token (tok);
          res->type = LHSE_NEW;
          res->data.new_expr = ALLOCATE (new_expression);
          res->data.new_expr->type = NE_MEMBER;
          res->data.new_expr->data.member_expr = member_expr;
          return res;
        }
    }
}

/* postfix_expression
  : left_hand_side_expression ('++' | '--')?
  ; */
static postfix_expression *
parse_postfix_expression ()
{
  postfix_expression *res = ALLOCATE (postfix_expression);

  res->expr = parse_left_hand_side_expression ();
  RETURN_NULL_IF_NULL (res->expr);

  tok = lexer_next_token ();
  if (tok.type == TOK_DOUBLE_PLUS)
    res->type = PE_INCREMENT;
  else if (tok.type == TOK_DOUBLE_MINUS)
    res->type = PE_DECREMENT;
  else
    {
      lexer_save_token (tok);
      res->type = PE_NONE;
    }

  return res;
}

/* unary_expression
  : postfix_expression
  | ('delete' | 'void' | 'typeof' | '++' | '--' | '+' | '-' | '~' | '!') unary_expression
  ; */
static unary_expression *
parse_unary_expression ()
{
  unary_expression *res = ALLOCATE (unary_expression);

  switch (tok.type)
  {
    case TOK_DOUBLE_PLUS:   res->type = UE_INCREMENT; break;
    case TOK_DOUBLE_MINUS:  res->type = UE_DECREMENT; break;
    case TOK_PLUS:          res->type = UE_PLUS;      break;
    case TOK_MINUS:         res->type = UE_MINUS;     break;
    case TOK_COMPL:         res->type = UE_COMPL;     break;
    case TOK_NOT:           res->type = UE_NOT;       break;
    case TOK_KEYWORD:
      if (is_keyword (KW_DELETE)) { res->type = UE_DELETE; break; }
      if (is_keyword (KW_VOID))   { res->type = UE_VOID;   break; }
      if (is_keyword (KW_TYPEOF)) { res->type = UE_TYPEOF; break; }
      /* FALLTHRU.  */

    default:
      /* It is postfix_expression.  */
      res->type = UE_POSTFIX;
      res->data.postfix_expr = parse_postfix_expression ();
      RETURN_NULL_IF_NULL (res->data.postfix_expr);
      return res;
  }

  skip_newlines ();
  res->data.unary_expr = parse_unary_expression ();
  assert (res->data.unary_expr);
  return res;
}

/* multiplicative_expression_list
  : unary_expression (LT!* ('*' | '/' | '%') LT!* unary_expression)*
  ; */
static multiplicative_expression_list *
parse_multiplicative_expression_list ()
{
  multiplicative_expression_list *res = ALLOCATE (multiplicative_expression_list);

  res->unary_expr = parse_unary_expression ();
  RETURN_NULL_IF_NULL (res->unary_expr);

  skip_newlines ();
  switch (tok.type)
  {
    case TOK_MULT: res->type = ME_MULT; break;
    case TOK_DIV:  res->type = ME_DIV;  break;
    case TOK_MOD:  res->type = ME_MOD;  break;
    default:
      lexer_save_token (tok);
      res->type = ME_NONE;
      res->next = NULL;
      return res;
  }

  skip_newlines ();
  res->next = parse_multiplicative_expression_list ();
  assert (res->next);
  return res;
}

/* additive_expression_list
  : multiplicative_expression_list (LT!* ('+' | '-') LT!* multiplicative_expression_list)*
  ; */
static additive_expression_list * 
parse_additive_expression_list ()
{
  additive_expression_list *res = ALLOCATE (additive_expression_list);

  res->mult_expr = parse_multiplicative_expression_list ();
  RETURN_NULL_IF_NULL (res->mult_expr);

  skip_newlines ();
  switch (tok.type)
  {
    case TOK_PLUS:  res->type = AE_PLUS;  break;
    case TOK_MINUS: res->type = AE_MINUS; break;
    default:
      lexer_save_token (tok);
      res->type = AE_NONE;
      res->next = NULL;
      return res;
  }

  skip_newlines ();
  res->next = parse_additive_expression_list ();
  assert (res->next);
  return res;
}

/* shift_expression_list
  : additive_expression_list (LT!* ('<<' | '>>' | '>>>') LT!* additive_expression_list)*
  ; */
static shift_expression_list *
parse_shift_expression_list ()
{
  shift_expression_list *res = ALLOCATE (shift_expression_list);

  res->add_expr = parse_additive_expression_list ();
  RETURN_NULL_IF_NULL (res->add_expr);

  skip_newlines ();
  switch (tok.type)
  {
    case TOK_LSHIFT:    res->type = SE_LSHIFT;    break;
    case TOK_RSHIFT:    res->type = SE_RSHIFT;    break;
    case TOK_RSHIFT_EX: res->type = SE_RSHIFT_EX; break;
    default:
      lexer_save_token (tok);
      res->type = SE_NONE;
      res->next = NULL;
      return res;
  }

  skip_newlines ();
  res->next = parse_shift_expression_list ();
  assert (res->next);
  return res;
}

/* relational_expression_list
  : shift_expression_list (LT!* ('<' | '>' | '<=' | '>=' | 'instanceof' | 'in') LT!* shift_expression_list)*
  ;

   relational_expression_no_in
  : shift_expression_list (LT!* ('<' | '>' | '<=' | '>=' | 'instanceof') LT!* shift_expression_list)*
  ; */
static relational_expression_list *
parse_relational_expression_list (bool in_allowed)
{
  relational_expression_list *res = ALLOCATE (relational_expression_list);

  res->shift_expr = parse_shift_expression_list ();
  RETURN_NULL_IF_NULL (res->shift_expr);

  skip_newlines ();
  switch (tok.type)
  {
    case TOK_LESS: res->type = RE_LESS; break;
    case TOK_GREATER: res->type = RE_GREATER; break;
    case TOK_LESS_EQ: res->type = RE_LESS_EQ; break;
    case TOK_GREATER_EQ: res->type = RE_GREATER_EQ; break;
    case TOK_KEYWORD:
      if (is_keyword (KW_INSTANCEOF)) 
        { 
          res->type = RE_INSTANCEOF; 
          break; 
        }
      if (is_keyword (KW_IN))
        {
          if (!in_allowed)
            fatal (ERR_PARSER);
          res->type = RE_IN;
          break;
        }
    default:
      lexer_save_token (tok);
      res->type = RE_NONE;
      res->next = NULL;
      return res;
  }

  skip_newlines ();
  res->next = parse_relational_expression_list (in_allowed);
  assert (res->next);
  return res;
}

/* equality_expression_list
  : relational_expression_list (LT!* ('==' | '!=' | '===' | '!==') LT!* relational_expression_list)*
  ;

   equality_expression_no_in
  : relational_expression_no_in (LT!* ('==' | '!=' | '===' | '!==') LT!* relational_expression_no_in)*
  ; */
static equality_expression_list *
parse_equality_expression_list (bool in_allowed)
{
  equality_expression_list *res = ALLOCATE (equality_expression_list);

  res->rel_expr = parse_relational_expression_list (in_allowed);
  RETURN_NULL_IF_NULL (res->rel_expr);

  skip_newlines ();
  switch (tok.type)
  {
    case TOK_DOUBLE_EQ: res->type = EE_DOUBLE_EQ; break;
    case TOK_NOT_EQ: res->type = EE_NOT_EQ; break;
    case TOK_TRIPLE_EQ: res->type = EE_TRIPLE_EQ; break;
    case TOK_NOT_DOUBLE_EQ: res->type = EE_NOT_DOUBLE_EQ; break;
    default:
      lexer_save_token (tok);
      res->type = EE_NONE;
      res->next = NULL;
      return res;
  }

  skip_newlines ();
  res->next = parse_equality_expression_list (in_allowed);
  assert (res->next);
  return res;
}

/* bitwise_and_expression_list
  : equality_expression_list (LT!* '&' LT!* equality_expression_list)*
  ;
  
   bitwise_and_expression_no_in
  : equality_expression_no_in (LT!* '&' LT!* equality_expression_no_in)*
  ; */
static bitwise_and_expression_list *
parse_bitwise_and_expression_list (bool in_allowed)
{
  bitwise_and_expression_list *res = ALLOCATE (bitwise_and_expression_list);

  res->eq_expr = parse_equality_expression_list (in_allowed);
  RETURN_NULL_IF_NULL (res->eq_expr);

  skip_newlines ();
  if (tok.type == TOK_AND)
    {
      skip_newlines ();
      res->next = parse_bitwise_and_expression_list (in_allowed);
      assert (res->next);
      return res;
    }
  else
    {
      lexer_save_token (tok);
      res->next = NULL;
      return res;
    }
}

/* bitwise_xor_expression_list
  : bitwise_and_expression_list (LT!* '^' LT!* bitwise_and_expression_list)*
  ;
  
   bitwise_xor_expression_no_in
  : bitwise_and_expression_no_in (LT!* '^' LT!* bitwise_and_expression_no_in)*
  ; */
static bitwise_xor_expression_list *
parse_bitwise_xor_expression_list (bool in_allowed)
{
  bitwise_xor_expression_list *res = ALLOCATE (bitwise_xor_expression_list);

  res->and_expr = parse_bitwise_and_expression_list (in_allowed);
  RETURN_NULL_IF_NULL (res->and_expr);

  skip_newlines ();
  if (tok.type == TOK_XOR)
    {
      skip_newlines ();
      res->next = parse_bitwise_xor_expression_list (in_allowed);
      assert (res->next);
      return res;
    }
  else
    {
      lexer_save_token (tok);
      res->next = NULL;
      return res;
    }
}

/* bitwise_or_expression_list
  : bitwise_xor_expression_list (LT!* '|' LT!* bitwise_xor_expression_list)*
  ;
  
   bitwise_or_expression_no_in
  : bitwise_xor_expression_no_in (LT!* '|' LT!* bitwise_xor_expression_no_in)*
  ; */
static bitwise_or_expression_list *
parse_bitwise_or_expression_list (bool in_allowed)
{
  bitwise_or_expression_list *res = ALLOCATE (bitwise_or_expression_list);

  res->xor_expr = parse_bitwise_xor_expression_list (in_allowed);
  RETURN_NULL_IF_NULL (res->xor_expr);

  skip_newlines ();
  if (tok.type == TOK_OR)
    {
      skip_newlines ();
      res->next = parse_bitwise_or_expression_list (in_allowed);
      assert (res->next);
      return res;
    }
  else
    {
      lexer_save_token (tok);
      res->next = NULL;
      return res;
    }
}

/* logical_and_expression_list
  : bitwise_or_expression_list (LT!* '&&' LT!* bitwise_or_expression_list)*
  ;
  
   logical_and_expression_no_in
  : bitwise_or_expression_no_in (LT!* '&&' LT!* bitwise_or_expression_no_in)*
  ;*/
static logical_and_expression_list *
parse_logical_and_expression_list (bool in_allowed)
{
  logical_and_expression_list *res = ALLOCATE (logical_and_expression_list);

  res->or_expr = parse_bitwise_or_expression_list (in_allowed);
  RETURN_NULL_IF_NULL (res->or_expr);

  skip_newlines ();
  if (tok.type == TOK_DOUBLE_AND)
    {
      skip_newlines ();
      res->next = parse_logical_and_expression_list (in_allowed);
      assert (res->next);
      return res;
    }
  else
    {
      lexer_save_token (tok);
      res->next = NULL;
      return res;
    }
}

/* logical_or_expression_list
  : logical_and_expression_list (LT!* '||' LT!* logical_and_expression_list)*
  ;
  
   logical_or_expression_no_in
  : logical_and_expression_no_in (LT!* '||' LT!* logical_and_expression_no_in)*
  ; */
static logical_or_expression_list *
parse_logical_or_expression_list (bool in_allowed)
{
  logical_or_expression_list *res = ALLOCATE (logical_or_expression_list);

  res->and_expr = parse_logical_and_expression_list (in_allowed);
  RETURN_NULL_IF_NULL (res->and_expr);

  skip_newlines ();
  if (tok.type == TOK_DOUBLE_OR)
    {
      skip_newlines ();
      res->next = parse_logical_or_expression_list (in_allowed);
      assert (res->next);
      return res;
    }
  else
    {
      lexer_save_token (tok);
      res->next = NULL;
      return res;
    }
}

/* conditional_expression
  : logical_or_expression_list (LT!* '?' LT!* assignment_expression LT!* ':' LT!* assignment_expression)?
  ;

   conditional_expression_no_in
  : logical_or_expression_no_in (LT!* '?' LT!* assignment_expression_no_in LT!* ':' LT!* assignment_expression_no_in)?
  ; */
static conditional_expression *
parse_conditional_expression (bool in_allowed)
{
  conditional_expression *res = ALLOCATE (conditional_expression);

  res->or_expr = parse_logical_or_expression_list (in_allowed);
  RETURN_NULL_IF_NULL (res->or_expr);

  skip_newlines ();
  if (tok.type == TOK_QUERY)
    {
      skip_newlines ();
      res->then_expr = parse_assigment_expression (in_allowed);
      assert (res->then_expr);
      token_after_newlines_must_be (TOK_COLON);

      skip_newlines ();
      res->else_expr = parse_assigment_expression (in_allowed);
      assert (res->else_expr);
      return res;
    }
  else
    {
      lexer_save_token (tok);
      res->then_expr = res->else_expr = NULL;
      return res;
    }
}

/* assignment_expression
  : conditional_expression
  | left_hand_side_expression LT!* assignment_operator LT!* assignment_expression
  ;
  
   assignment_expression_no_in
  : conditional_expression_no_in
  | left_hand_side_expression LT!* assignment_operator LT!* assignment_expression_no_in
  ; 

   assignmentOperator
  : '=' | '*=' | '/=' | '%=' | '+=' | '-=' | '<<=' | '>>=' | '>>>=' | '&=' | '^=' | '|='
  ;*/
static assignment_expression *
parse_assigment_expression (bool in_allowed)
{
  assignment_expression *res = ALLOCATE (assignment_expression);

  /* OK, assignment_expression contains conditional_expression,
     conditional_expression contains logical_or_expression_list,
     logical_or_expression_list contains logical_and_expression_list,
     logical_and_expression_list contains bitwise_or_expression_list,
     bitwise_or_expression_list contains bitwise_xor_expression_list,
     bitwise_xor_expression_list contains bitwise_and_expression_list,
     bitwise_and_expression_list contains equality_expression_list,
     equality_expression_list contains relational_expression_list,
     relational_expression_list contains shift_expression_list,
     shift_expression_list contains additive_expression_list,
     additive_expression_list contains multiplicative_expression_list,
     multiplicative_expression_list contains unary_expression,
     unary_expression contains postfix_expression,
     postfix_expression contains left_hand_side_expression.

     So, we can try parse conditional_expression and check if there is only left_hand_side_expression.  */
  conditional_expression *cond_expr = parse_conditional_expression (in_allowed);
  const logical_or_expression_list *lor_expr;
  const logical_and_expression_list *land_expr;
  const bitwise_or_expression_list *bor_expr;
  const bitwise_xor_expression_list *xor_expr;
  const bitwise_and_expression_list *band_expr;
  const equality_expression_list *eq_expr;
  const relational_expression_list *rel_expr;
  const shift_expression_list *shift_expr;
  const additive_expression_list *add_expr;
  const multiplicative_expression_list *mult_expr;
  const unary_expression *unary_expr;
  const postfix_expression *postfix_expr;
  left_hand_side_expression *left_hand_expr;

  RETURN_NULL_IF_NULL (cond_expr);

  if (cond_expr->then_expr)
    goto cond;
  assert (cond_expr->else_expr == NULL);
  assert (cond_expr->or_expr);

  lor_expr = cond_expr->or_expr;
  if (lor_expr->next)
    goto cond;
  assert (lor_expr->and_expr);

  land_expr = lor_expr->and_expr;
  if (land_expr->next)
    goto cond;
  assert (land_expr->or_expr);

  bor_expr = land_expr->or_expr;
  if (bor_expr->next)
    goto cond;
  assert (bor_expr->xor_expr);

  xor_expr = bor_expr->xor_expr;
  if (xor_expr->next)
    goto cond;
  assert (xor_expr->and_expr);

  band_expr = xor_expr->and_expr;
  if (band_expr->next)
    goto cond;
  assert (band_expr->eq_expr);

  eq_expr = band_expr->eq_expr;
  if (eq_expr->next)
    goto cond;
  assert (eq_expr->type == EE_NONE);
  assert (eq_expr->rel_expr);

  rel_expr = eq_expr->rel_expr;
  if (rel_expr->next)
    goto cond;
  assert (rel_expr->type == RE_NONE);
  assert (rel_expr->shift_expr);

  shift_expr = rel_expr->shift_expr;
  if (shift_expr->next)
    goto cond;
  assert (shift_expr->type == SE_NONE);
  assert (shift_expr->add_expr);

  add_expr = shift_expr->add_expr;
  if (add_expr->next)
    goto cond;
  assert (add_expr->type == AE_NONE);
  assert (add_expr->mult_expr);

  mult_expr = add_expr->mult_expr;
  if (mult_expr->next)
    goto cond;
  assert (mult_expr->type == ME_NONE);
  assert (mult_expr->unary_expr);

  unary_expr = mult_expr->unary_expr;
  if (unary_expr->type != UE_POSTFIX)
    goto cond;
  assert (unary_expr->data.postfix_expr);

  postfix_expr = unary_expr->data.postfix_expr;
  if (postfix_expr->type != PE_NONE)
    goto cond;
  assert (postfix_expr->expr);

  left_hand_expr = postfix_expr->expr;

  skip_newlines ();
  switch (tok.type)
  {
    case TOK_EQ: res->type = AE_EQ; break;
    case TOK_MULT_EQ: res->type = AE_MULT_EQ; break;
    case TOK_DIV_EQ: res->type = AE_DIV_EQ; break;
    case TOK_MOD_EQ: res->type = AE_MOD_EQ; break;
    case TOK_PLUS_EQ: res->type = AE_PLUS_EQ; break;
    case TOK_MINUS_EQ: res->type = AE_MINUS_EQ; break;
    case TOK_LSHIFT_EQ: res->type = AE_LSHIFT_EQ; break;
    case TOK_RSHIFT_EQ: res->type = AE_RSHIFT_EQ; break;
    case TOK_RSHIFT_EX_EQ: res->type = AE_RSHIFT_EX_EQ; break;
    case TOK_AND_EQ: res->type = AE_AND_EQ; break;
    case TOK_OR_EQ: res->type = AE_OR_EQ; break;
    case TOK_XOR_EQ: res->type = AE_XOR_EQ; break;
    default: 
      lexer_save_token (tok); 
      goto cond;
  }
  res->data.s.left_hand_expr = left_hand_expr;

  skip_newlines ();
  res->data.s.assign_expr = parse_assigment_expression (in_allowed);
  assert (res->data.s.assign_expr);
  return res;

cond:
  res->type = AE_COND;
  res->data.cond_expr = cond_expr;
  return res;
}

/* expression
  : assignment_expression (LT!* ',' LT!* assignment_expression)*
  ;
  
   expression_no_in
  : assignment_expression_no_in (LT!* ',' LT!* assignment_expression_no_in)*
  ; */
static expression *
parse_expression (bool in_allowed)
{
  expression *res = ALLOCATE (expression);

  res->assign_expr = parse_assigment_expression (in_allowed);
  RETURN_NULL_IF_NULL (res->assign_expr);

  skip_newlines ();
  if (tok.type == TOK_COMMA)
    {
      skip_newlines ();
      res->next = parse_expression (in_allowed);
    }
  else
    {
      lexer_save_token (tok);
      res->next = NULL;
    }

  return res;
}

/* variable_declaration(_no_in)
	: Identifier LT!* initialiser(_no_in)?
	;
   initialiser(_no_in)
	: '=' LT!* assignment_expression
	; */
static variable_declaration*
parse_variable_declaration (bool in_allowed)
{
  variable_declaration *res = ALLOCATE (variable_declaration);

  current_token_must_be (TOK_NAME);
  res->name = tok.data.name;

  skip_newlines ();
  if (tok.type == TOK_EQ)
    {
      skip_newlines ();
      res->ass_expr = parse_assigment_expression (in_allowed);
    }
  else
    {
      lexer_save_token (tok);
      res->ass_expr = NULL;
    }

  return res;
}

/* variable_declaration_list(_no_in)
	: variable_declaration(_no_in) 
	  (LT!* ',' LT!* variable_declaration(_no_in))*
	; */
static variable_declaration_list *
parse_variable_declaration_list (bool in_allowed)
{
  variable_declaration_list *res = ALLOCATE (variable_declaration_list);;
  variable_declaration *var_decl = parse_variable_declaration (in_allowed);

  fatal_if_null (var_decl);
  res->var_decl = var_decl;
  skip_newlines ();
  if (tok.type == TOK_COMMA)
    {
      skip_newlines ();
      res->next = parse_variable_declaration_list (in_allowed);
      assert (res->next);
    }
  else
    {
      lexer_save_token (tok);
      res->next = NULL;
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

static for_or_for_in_statement *
parse_for_or_for_in_statement ()
{
  for_or_for_in_statement *res = ALLOCATE (for_or_for_in_statement);
  variable_declaration_list *list = NULL;
  expression *expr = NULL;

  assert_keyword  (KW_FOR);
  token_after_newlines_must_be (TOK_OPEN_PAREN);

  skip_newlines ();
  if (tok.type == TOK_SEMICOLON)
    goto plain_for;
  /* Both for_statement_initialiser_part and for_in_statement_initialiser_part
     contains 'var'. Chekc it first.  */
  if (is_keyword (KW_VAR))
    {
      skip_newlines ();
      list = parse_variable_declaration_list (false);
      assert (list);
      if (list->next)
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
            fatal (ERR_PARSER);
        }
    }
  assert (list == NULL);

  /* expression contains left_hand_side_expression.  */
  expr = parse_expression (false);

  skip_newlines ();
  if (tok.type == TOK_SEMICOLON)
    goto plain_for;
  else if (is_keyword (KW_IN))
    goto for_in;
  else 
    fatal (ERR_PARSER);

  unreachable ();

plain_for:
  res->is_for_in = false;
  res->data.for_stmt = ALLOCATE (for_statement);
  if (list)
    {
      assert (expr == NULL);
      res->data.for_stmt->init = ALLOCATE (for_statement_initialiser_part);
      res->data.for_stmt->init->is_decl = true;
      res->data.for_stmt->init->data.decl_list = list;
    }
  if (expr)
    {
      assert (list == NULL);
      res->data.for_stmt->init = ALLOCATE (for_statement_initialiser_part);
      res->data.for_stmt->init->is_decl = false;
      res->data.for_stmt->init->data.expr = expr;
    }

  skip_newlines ();
  if (tok.type == TOK_SEMICOLON)
    res->data.for_stmt->limit = NULL;
  else
    {
      res->data.for_stmt->limit = parse_expression (true);
      assert (res->data.for_stmt->limit);
      next_token_must_be (TOK_SEMICOLON);
    }

  skip_newlines ();
  if (tok.type == TOK_CLOSE_PAREN)
    res->data.for_stmt->incr = NULL;
  else
    {
      res->data.for_stmt->incr = parse_expression (true);
      assert (res->data.for_stmt->incr);
      next_token_must_be (TOK_CLOSE_PAREN);
    }
  return res;

for_in:
  res->is_for_in = true;
  res->data.for_in_stmt = ALLOCATE (for_in_statement);
  if (list)
    {
      assert (expr == NULL);
      assert (list->next == NULL);
      res->data.for_in_stmt->init = ALLOCATE (for_in_statement_initializer_part);
      res->data.for_in_stmt->init->is_decl = true;
      res->data.for_in_stmt->init->data.decl = list->var_decl;
    }
  if (expr)
    { 
      assignment_expression *assign_expr = NULL; 
      conditional_expression *cond_expr = NULL;
      logical_or_expression_list *lor_expr = NULL;
      logical_and_expression_list *land_expr = NULL;
      bitwise_or_expression_list *bor_expr = NULL;
      bitwise_xor_expression_list *xor_expr = NULL;
      bitwise_and_expression_list *band_expr = NULL;
      equality_expression_list *eq_expr = NULL;
      relational_expression_list *rel_expr = NULL;
      shift_expression_list *shift_expr = NULL;
      additive_expression_list *add_expr = NULL;
      multiplicative_expression_list *mult_expr = NULL;
      unary_expression *unary_expr = NULL;
      postfix_expression *postfix_expr = NULL;

      assert (list == NULL);
      assert (expr->next == NULL);

      assign_expr = expr->assign_expr;
      assert (assign_expr->type == AE_COND);
      assert (assign_expr->data.cond_expr);

      cond_expr = assign_expr->data.cond_expr;
      assert (cond_expr->then_expr == NULL);
      assert (cond_expr->else_expr == NULL);
      assert (cond_expr->or_expr);
    
      lor_expr = cond_expr->or_expr;
      assert (lor_expr->next == NULL);
      assert (lor_expr->and_expr);
    
      land_expr = lor_expr->and_expr;
      assert (land_expr->next == NULL);
      assert (land_expr->or_expr);
    
      bor_expr = land_expr->or_expr;
      assert (bor_expr->next == NULL);
      assert (bor_expr->xor_expr);
    
      xor_expr = bor_expr->xor_expr;
      assert (xor_expr->next == NULL);
      assert (xor_expr->and_expr);
    
      band_expr = xor_expr->and_expr;
      assert (band_expr->next == NULL);
      assert (band_expr->eq_expr);
    
      eq_expr = band_expr->eq_expr;
      assert (eq_expr->next == NULL);
      assert (eq_expr->type == EE_NONE);
      assert (eq_expr->rel_expr);
    
      rel_expr = eq_expr->rel_expr;
      assert (rel_expr->next == NULL);
      assert (rel_expr->type == RE_NONE);
      assert (rel_expr->shift_expr);
    
      shift_expr = rel_expr->shift_expr;
      assert (shift_expr->next == NULL);
      assert (shift_expr->type == SE_NONE);
      assert (shift_expr->add_expr);
    
      add_expr = shift_expr->add_expr;
      assert (add_expr->next == NULL);
      assert (add_expr->type == AE_NONE);
      assert (add_expr->mult_expr);
    
      mult_expr = add_expr->mult_expr;
      assert (mult_expr->next == NULL);
      assert (mult_expr->type == ME_NONE);
      assert (mult_expr->unary_expr);
    
      unary_expr = mult_expr->unary_expr;
      assert (unary_expr->type == UE_POSTFIX);
      assert (unary_expr->data.postfix_expr);
    
      postfix_expr = unary_expr->data.postfix_expr;
      assert (postfix_expr->type == PE_NONE);
      assert (postfix_expr->expr);

      res->data.for_in_stmt->init = ALLOCATE (for_in_statement_initializer_part);
      res->data.for_in_stmt->init->is_decl = false;
      res->data.for_in_stmt->init->data.left_hand_expr = postfix_expr->expr;
    }

  assert (res->data.for_in_stmt->init);
  skip_newlines ();
  res->data.for_in_stmt->list_expr = parse_expression (true);
  assert (res->data.for_in_stmt->list_expr);
  token_after_newlines_must_be (TOK_CLOSE_PAREN);

  return res;
}

static inline void
parse_expression_inside_parens (statement *res)
{
  token_after_newlines_must_be (TOK_OPEN_PAREN);
  skip_newlines ();
  res->data.expr = parse_expression (true);
  assert (res->data.expr);
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
statement *
parser_parse_statement ()
{
  statement *res = ALLOCATE (statement);
  res->data.none = NULL;

  skip_newlines ();

  if (is_keyword (KW_FINALLY))
    {
      res->type = STMT_FINALLY;
      current_scope_must_be (SCOPE_TRY | SCOPE_CATCH);
      pop_scope ();
      push_scope (SCOPE_FINALLY);
      return res;
    }

  if (current_scope->was_stmt 
      && (current_scope->type & (SCOPE_IF | SCOPE_DO | SCOPE_WITH | SCOPE_SWITCH | SCOPE_ELSE 
                                 | SCOPE_CATCH | SCOPE_FINALLY | SCOPE_FUNCTION | SCOPE_WHILE
                                 | SCOPE_FOR)))
    pop_scope ();

  current_scope->was_stmt = true;

  if (tok.type == TOK_EOF)
    {
      current_scope_must_be_global ();
      res->type = STMT_EOF;
      return res;
    }

  if (current_scope->type == SCOPE_SUBEXPRESSION)
    {
      if (tok.type == TOK_CLOSE_PAREN)
        {
          res->type = STMT_SUBEXPRESSION_END;
          pop_scope ();
          return res;
        }
      res->type = STMT_EXPRESSION;
      res->data.expr = parse_expression (true);
      assert (res->data.expr);
      return res;
    }
  if (tok.type == TOK_OPEN_BRACE)
    {
      res->type = STMT_BLOCK_START;
      push_scope (SCOPE_BLOCK);
      return res;
    }
  if (tok.type == TOK_CLOSE_BRACE)
    {
      current_scope_must_be (SCOPE_BLOCK);
      res->type = STMT_BLOCK_END;
      pop_scope ();
      current_scope->was_stmt = true;
      return res;
    }
  if (is_keyword (KW_ELSE))
    {
      current_scope_must_be (SCOPE_IF);
      skip_newlines ();
      if (is_keyword (KW_IF))
        {
          res->type = STMT_ELSE_IF;
          parse_expression_inside_parens (res);
          return res;
        }
      else
        {
          lexer_save_token (tok);
          res->type = STMT_ELSE;
          pop_scope ();
          push_scope (SCOPE_ELSE);
          return res;
        }
    }
  if (is_keyword (KW_WHILE))
    {
      res->type = STMT_WHILE;
      parse_expression_inside_parens (res);
      if (current_scope->type == SCOPE_DO)
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
      res->type = STMT_CATCH;
      current_scope_must_be (SCOPE_TRY);
      parse_expression_inside_parens (res);
      pop_scope ();
      push_scope (SCOPE_CATCH);
      return res;
    }
  if (is_keyword (KW_FUNCTION))
    {
      res->type = STMT_FUNCTION;
      res->data.fun_decl = parse_function_declaration ();
      push_scope (SCOPE_FUNCTION);
      return res;
    }
  if (is_keyword (KW_VAR))
    {
      res->type = STMT_VARIABLE;

      skip_newlines ();
      res->data.var_stmt = parse_variable_declaration_list (true);
      return res;
    }
  if (tok.type == TOK_SEMICOLON)
    {
      res->type = STMT_EMPTY;
      return res;
    }
  if (is_keyword (KW_IF))
    {
      res->type = STMT_IF;
      parse_expression_inside_parens (res);
      push_scope (SCOPE_IF);
      return res;
    }
  if (is_keyword (KW_DO))
    {
      res->type = STMT_DO;
      push_scope (SCOPE_DO);
      return res;
    }
  if (is_keyword (KW_FOR))
    {
      res->type = STMT_FOR_OR_FOR_IN;
      res->data.for_stmt = parse_for_or_for_in_statement ();
      assert (res->data.for_stmt);
      push_scope (SCOPE_FOR);
      return res;
    }
  if (is_keyword (KW_CONTINUE))
    {
      scope_must_be (SCOPE_LOOP);
      res->type = STMT_CONTINUE;
      tok = lexer_next_token ();
      if (tok.type == TOK_NAME)
        res->data.name = tok.data.name;
      else
        lexer_save_token (tok);
      insert_semicolon ();
      return res;
    }
  if (is_keyword (KW_BREAK))
    {
      scope_must_be (SCOPE_LOOP | SCOPE_CASE);
      res->type = STMT_BREAK;
      tok = lexer_next_token ();
      if (tok.type == TOK_NAME)
        {
          if (current_scope->type == SCOPE_CASE)
            fatal (ERR_PARSER);
          res->data.name = tok.data.name;
        }
      else
        lexer_save_token (tok);
      insert_semicolon ();
      return res;
    }
  if (is_keyword (KW_RETURN))
    {
      scope_must_be (SCOPE_FUNCTION);
      res->type = STMT_RETURN;
      tok = lexer_next_token ();
      if (tok.type != TOK_SEMICOLON && tok.type != TOK_NEWLINE)
        {
          scope *old_scope = current_scope;
          res->data.expr = parse_expression (false);
          assert (res->data.expr);
          if (old_scope == current_scope)
            insert_semicolon ();
        }
      // Eat TOK_SEMICOLON
      return res;
    }
  if (is_keyword (KW_WITH))
    {
      res->type = STMT_WITH;
      parse_expression_inside_parens (res);
      push_scope (SCOPE_WITH);
      return res;
    }
  if (is_keyword (KW_SWITCH))
    {
      res->type = STMT_SWITCH;
      parse_expression_inside_parens (res);
      push_scope (SCOPE_SWITCH);
      return res;
    }
  if (is_keyword (KW_THROW))
    {
      res->type = STMT_THROW;
      tok = lexer_next_token ();
      res->data.expr = parse_expression (true);
      assert (res->data.expr);
      insert_semicolon ();
      return res;
    }
  if (is_keyword (KW_TRY))
    {
      res->type = STMT_TRY;
      push_scope (SCOPE_TRY);
      return res;
    }
  if (is_keyword (KW_CASE))
    {
      if (current_scope->type == SCOPE_CASE)
        pop_scope ();
      current_scope_must_be (SCOPE_SWITCH);
      skip_newlines ();
      res->data.expr = parse_expression (true);
      assert (res->data.expr);
      token_after_newlines_must_be (TOK_SEMICOLON);
      push_scope (SCOPE_CASE);
      return res;
    }
  if (is_keyword (KW_DEFAULT))
    {
      if (current_scope->type == SCOPE_CASE)
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
          res->type = STMT_LABELLED;
          res->data.name = saved.data.name;
          return res;
        }
      else
        {
          lexer_save_token (tok);
          tok = saved;
          expression *expr = parse_expression (true);
          fatal_if_null (expr);
          res->type = STMT_EXPRESSION;
          res->data.expr = expr;
          return res;
        }
    }

  expression *expr = parse_expression (true);
  if (expr)
    {
      res->type = STMT_EXPRESSION;
      res->data.expr = expr;
      return res;
    }
  else
    {
      lexer_save_token (tok);
      free (res);
      return NULL;
    }
}

void
parser_init ()
{
  current_scope = ALLOCATE (struct scope);
  current_scope->type = SCOPE_GLOBAL;
#ifdef DEBUG
  debug_file = fopen ("parser.log", "w");
#endif
}

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
#include "opcodes.h"
#include "serializer.h"

#define MAX_OPCODES 10
#define INVALID_VALUE 255

static token tok;
static OPCODE opcode, opcodes_buffer[MAX_OPCODES];
static uint8_t current_opcode_in_buffer = 0;
static uint8_t opcode_counter = 0, sub_assignment_exprs = 0;

#ifdef __HOST
_FILE *debug_file;
#endif

static T_IDX parse_expression (void);
static void parse_statement (void);
static T_IDX parse_assignment_expression (void);
static void parse_source_element_list (void);

static T_IDX temp_name, min_temp_name;

static T_IDX
next_temp_name (void)
{
  return temp_name++;
}

static void
reset_temp_name (void)
{
  temp_name = min_temp_name;
}

static void
assert_keyword (keyword kw)
{
  if (tok.type != TOK_KEYWORD || tok.data.kw != kw)
    {
#ifdef __HOST
      __printf ("assert_keyword: %d\n", kw);
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
      __printf ("current_token_must_be: %d\n", tt);
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
      __printf ("next_token_must_be: %d\n", tt);
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

#if 0
static void
insert_semicolon (void)
{
  tok = lexer_next_token ();
  if (tok.type != TOK_NEWLINE && tok.type != TOK_SEMICOLON)
    parser_fatal (ERR_PARSER);
}
#endif

#define NEXT(ID, TYPE) \
  do { skip_newlines (); ID = parse_##TYPE (); } while (0)

#define DUMP_VOID_OPCODE(GETOP) \
  do { opcode=getop_##GETOP (); serializer_dump_opcode (&opcode); opcode_counter++; } while (0)

#define DUMP_OPCODE(GETOP, ...) \
  do { opcode=getop_##GETOP (__VA_ARGS__); serializer_dump_opcode (&opcode); opcode_counter++; } while (0)

#define REWRITE_OPCODE(OC, GETOP, ...) \
  do { opcode=getop_##GETOP (__VA_ARGS__); serializer_rewrite_opcode (OC, &opcode); } while (0)

static T_IDX
integer_zero (void)
{
  T_IDX lhs = next_temp_name ();
  DUMP_OPCODE (assignment, lhs, OPCODE_ARG_TYPE_SMALLINT, 0);
  return lhs;
}

static T_IDX
integer_one (void)
{
  T_IDX lhs = next_temp_name ();
  DUMP_OPCODE (assignment, lhs, OPCODE_ARG_TYPE_SMALLINT, 1);
  return lhs;
}

static void
save_opcode (void)
{
  JERRY_ASSERT (current_opcode_in_buffer < MAX_OPCODES);

  opcodes_buffer[current_opcode_in_buffer++] = opcode;
}

static void
dump_saved_opcodes (void)
{
  uint8_t i;
  for (i = 0; i < current_opcode_in_buffer; i++)
    serializer_dump_opcode (&opcodes_buffer[i]);
  current_opcode_in_buffer = 0;
}

/* property_name
  : Identifier
  | StringLiteral
  | NumericLiteral
  ; */
static T_IDX
parse_property_name (void)
{ 
  switch (tok.type)
  {
    case TOK_NAME:
    case TOK_STRING:
    case TOK_INT:
      return tok.data.uid;

    default:
      JERRY_UNREACHABLE ();
  }
}

/* property_name_and_value
  : property_name LT!* ':' LT!* assignment_expression
  ; */
static T_IDX
parse_property_name_and_value (void)
{
  T_IDX lhs, name, value;

  lhs = next_temp_name ();
  name = parse_property_name ();
  token_after_newlines_must_be (TOK_COLON);
  NEXT (value, assignment_expression);

  DUMP_OPCODE (prop, lhs, name, value);

  return lhs;
}

/* property_assignment
  : property_name_and_value
  | get LT!* property_name LT!* '(' LT!* ')' LT!* '{' LT!* function_body LT!* '}'
  | set LT!* property_name LT!* '(' identifier ')' LT!* '{' LT!* function_body LT!* '}' 
  ; */
static T_IDX
parse_property_assignment (void)
{
  T_IDX lhs, name, arg;

  current_token_must_be (TOK_NAME);

  lhs = next_temp_name ();

  if (!__strcmp ("get", lexer_get_string_by_id (tok.data.uid)))
    {
      NEXT (name, property_name);

      token_after_newlines_must_be (TOK_OPEN_PAREN);
      token_after_newlines_must_be (TOK_CLOSE_PAREN);
      token_after_newlines_must_be (TOK_OPEN_BRACE);

      DUMP_OPCODE (prop_get_decl, lhs, name);

      skip_newlines ();
      parse_source_element_list ();

      token_after_newlines_must_be (TOK_CLOSE_BRACE);
      DUMP_VOID_OPCODE (ret);

      return lhs;
    }
  else if (!__strcmp ("set", lexer_get_string_by_id (tok.data.uid)))
    {
      NEXT (name, property_name);

      token_after_newlines_must_be (TOK_OPEN_PAREN);
      token_after_newlines_must_be (TOK_NAME);
      arg = tok.data.uid;
      token_after_newlines_must_be (TOK_CLOSE_PAREN);
      token_after_newlines_must_be (TOK_OPEN_BRACE);

      DUMP_OPCODE (prop_set_decl, lhs, name, arg);

      skip_newlines ();
      parse_source_element_list ();

      token_after_newlines_must_be (TOK_CLOSE_BRACE);
      DUMP_VOID_OPCODE (ret);

      return lhs;
    }
  else
    return parse_property_name_and_value ();
}

static void
dump_varg_3 (T_IDX current_param, T_IDX params[3])
{
  if (current_param == 3)
    {
      DUMP_OPCODE (varg_3, params[0], params[1], params[2]);
      current_param = 0;
    }
}

static void
dump_varg_end (T_IDX current_param, T_IDX params[3])
{
  switch (current_param)
  {
    case 0:
      DUMP_OPCODE (varg_1_end, params[0]);
      break;
              
    case 1:
      DUMP_OPCODE (varg_2_end, params[0], params[1]);
      break;
              
    case 2:
      DUMP_OPCODE (varg_3_end, params[0], params[1], params[2]);
      break;

    default:
      JERRY_UNREACHABLE ();
  }
}

typedef enum
{
  AL_FUNC_DECL,
  AL_FUNC_EXPR,
  AL_ARRAY_LIT,
  AL_OBJECT_LIT,
  AL_CONSTRUCT_EXPR,
  AL_CALL_EXPR
}
argument_list_type;

/** Parse list of identifiers, assigment expressions or properties, splitted by comma. 
    For each ALT dumps appropriate bytecode. Uses OBJ during dump if neccesary.
    Returns temp var if expression has lhs, or 0 otherwise.  */
static T_IDX
parse_argument_list (argument_list_type alt, T_IDX obj)
{
  token_type open_tt, close_tt;
  T_IDX first_opcode_args_count, 
        lhs = 0, 
        args[3+1/* +1 for stack protector */], 
        current_arg = 0;

  switch (alt)
  {
    case AL_FUNC_DECL:
      open_tt = TOK_OPEN_PAREN;     // Openning token
      close_tt = TOK_CLOSE_PAREN;   // Ending token
      first_opcode_args_count = 2;  // Maximum number of arguments in first opcode
      break;

    case AL_FUNC_EXPR:
    case AL_CONSTRUCT_EXPR:
    case AL_CALL_EXPR:
      open_tt = TOK_OPEN_PAREN;
      close_tt = TOK_CLOSE_PAREN;
      first_opcode_args_count = 1;
      lhs = next_temp_name ();
      break;

    case AL_ARRAY_LIT:
      open_tt = TOK_OPEN_SQUARE;
      close_tt = TOK_CLOSE_SQUARE;
      first_opcode_args_count = 2;
      lhs = next_temp_name ();
      break;

    case AL_OBJECT_LIT:
      open_tt = TOK_OPEN_BRACE;
      close_tt = TOK_CLOSE_BRACE;
      first_opcode_args_count = 2;
      lhs = next_temp_name ();
      break;

    default:
      JERRY_UNREACHABLE ();
  }

  current_token_must_be (open_tt);

  skip_newlines ();
  if (tok.type != close_tt)
    {
      bool is_first_opcode = true;
      while (true)
        {
          if (is_first_opcode)
            { 
              if (current_arg == first_opcode_args_count)
                {
                  switch (alt)
                  {
                    case AL_FUNC_DECL:
                      DUMP_OPCODE (func_decl_n, obj, args[0], args[1]);
                      break;

                    case AL_FUNC_EXPR:
                      DUMP_OPCODE (func_expr_n, lhs, obj, args[0]);
                      break;

                    case AL_ARRAY_LIT:
                      DUMP_OPCODE (array_n, lhs, args[0], args[1]);
                      break;

                    case AL_OBJECT_LIT:
                      DUMP_OPCODE (obj_n, lhs, args[0], args[1]);
                      break;

                    case AL_CONSTRUCT_EXPR:
                      DUMP_OPCODE (construct_n, lhs, obj, args[0]);
                      break;

                    case AL_CALL_EXPR:
                      DUMP_OPCODE (call_n, lhs, obj, args[0]);
                      break;
                    
                    default:
                      JERRY_UNREACHABLE ();
                  }
                  current_arg = 0;
                  is_first_opcode = false;
                }
            }
          else
            dump_varg_3 (current_arg, args);

          switch (alt)
          {
            case AL_FUNC_DECL:
              current_token_must_be (TOK_NAME);
              args[current_arg] = tok.data.uid;
              break;

            case AL_FUNC_EXPR:
            case AL_ARRAY_LIT:
            case AL_CONSTRUCT_EXPR:
            case AL_CALL_EXPR:
              args[current_arg] = parse_assignment_expression ();
              break;

            case AL_OBJECT_LIT:
              args[current_arg] = parse_property_assignment ();
              break;

            default:
              JERRY_UNREACHABLE ();
          }

          skip_newlines ();
          if (tok.type != TOK_COMMA)
            {
              current_token_must_be (close_tt);
              break;
            }

          skip_newlines ();
          current_arg++;
        }

      if (is_first_opcode)
        {
          if (current_arg == 0)
            {
              switch (alt)
              {
                case AL_FUNC_DECL:
                  DUMP_OPCODE (func_decl_1, obj, args[0]);
                  break;

                case AL_FUNC_EXPR:
                  DUMP_OPCODE (func_expr_1, lhs, obj, args[0]);
                  break;

                case AL_ARRAY_LIT:
                  DUMP_OPCODE (array_1, lhs, args[0]);
                  break;

                case AL_OBJECT_LIT:
                  DUMP_OPCODE (obj_1, lhs, args[0]);
                  break;

                case AL_CONSTRUCT_EXPR:
                  DUMP_OPCODE (construct_1, lhs, obj, args[0]);
                  break;

                case AL_CALL_EXPR:
                  DUMP_OPCODE (call_1, lhs, obj, args[0]);
                  break;

                default:
                  JERRY_UNREACHABLE ();
              }
            }
          else if (current_arg == 1)
            {
              switch (alt)
              {
                case AL_FUNC_DECL:
                  DUMP_OPCODE (func_decl_2, obj, args[0], args[1]);
                  break;

                case AL_ARRAY_LIT:
                  DUMP_OPCODE (array_2, lhs, args[0], args[1]);
                  break;

                case AL_OBJECT_LIT:
                  DUMP_OPCODE (obj_2, lhs, args[0], args[1]);
                  break;

                default:
                  JERRY_UNREACHABLE ();
              }
            }
          else
            JERRY_UNREACHABLE ();
        }
      else
        dump_varg_end (current_arg, args);
    }
  else
    {
      switch (alt)
      {
        case AL_FUNC_DECL:
          DUMP_OPCODE (func_decl_0, obj);
          break;

        case AL_FUNC_EXPR:
          DUMP_OPCODE (func_expr_0, lhs, obj);
          break;

        case AL_ARRAY_LIT:
          DUMP_OPCODE (array_0, lhs);
          break;

        case AL_OBJECT_LIT:
          DUMP_OPCODE (obj_0, lhs);
          break;

        case AL_CONSTRUCT_EXPR:
          DUMP_OPCODE (construct_0, lhs, obj);
          break;

        case AL_CALL_EXPR:
          DUMP_OPCODE (call_0, lhs, obj);
          break;

        default:
          JERRY_UNREACHABLE ();
      }
    }

  return lhs;
}

/* function_declaration
	: 'function' LT!* Identifier LT!* 
	  '(' (LT!* Identifier (LT!* ',' LT!* Identifier)* ) ? LT!* ')' LT!* function_body
	;

   function_body
	: '{' LT!* sourceElements LT!* '}' */
static void
parse_function_declaration (void)
{
  T_IDX name;

  assert_keyword (KW_FUNCTION);

  token_after_newlines_must_be (TOK_NAME);

  name = tok.data.uid;

  skip_newlines ();
  parse_argument_list (AL_FUNC_DECL, name);

  token_after_newlines_must_be (TOK_OPEN_BRACE);

  skip_newlines ();
  parse_source_element_list ();

  next_token_must_be (TOK_CLOSE_BRACE);

  DUMP_VOID_OPCODE (ret);
}

/* function_expression
	: 'function' LT!* Identifier? LT!* '(' formal_parameter_list? LT!* ')' LT!* function_body
	; */
static T_IDX
parse_function_expression (void)
{
  T_IDX name, lhs;

  assert_keyword  (KW_FUNCTION);

  skip_newlines ();
  if (tok.type == TOK_NAME)
    name = tok.data.uid;
  else
    {
      lexer_save_token (tok);
      name = next_temp_name ();
    }

  skip_newlines ();
  lhs = parse_argument_list (AL_FUNC_EXPR, name);  

  token_after_newlines_must_be (TOK_OPEN_BRACE);

  skip_newlines ();
  parse_source_element_list ();

  token_after_newlines_must_be (TOK_CLOSE_BRACE);

  DUMP_VOID_OPCODE (ret);

  return lhs;
}

/* array_literal
  : '[' LT!* assignment_expression? (LT!* ',' (LT!* assignment_expression)?)* LT!* ']' LT!* 
  ; */
static T_IDX
parse_array_literal (void)
{
  return parse_argument_list (AL_ARRAY_LIT, 0);
}

/* object_literal
  : '{' LT!* property_assignment (LT!* ',' LT!* property_assignment)* LT!* '}'
  ; */
static T_IDX 
parse_object_literal (void)
{
  return parse_argument_list (AL_OBJECT_LIT, 0);
}

static T_IDX
parse_literal (void)
{
  T_IDX lhs;

  switch (tok.type)
  {
    case TOK_NULL:
      lhs = next_temp_name ();
      DUMP_OPCODE (assignment, lhs, OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_NULL);
      return lhs;

    case TOK_BOOL:
      lhs = next_temp_name ();
      DUMP_OPCODE (assignment, lhs, OPCODE_ARG_TYPE_SIMPLE, 
                   tok.data.uid ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
      return lhs;

    case TOK_INT:
      lhs = next_temp_name ();
      DUMP_OPCODE (assignment, lhs, OPCODE_ARG_TYPE_NUMBER, tok.data.uid);
      return lhs;

    case TOK_STRING:
      lhs = next_temp_name ();
      DUMP_OPCODE (assignment, lhs, OPCODE_ARG_TYPE_STRING, tok.data.uid);
      return lhs;

    default:
      JERRY_UNREACHABLE ();
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
static T_IDX
parse_primary_expression (void)
{
  T_IDX lhs;

  if (is_keyword (KW_THIS))
    {
      lhs = next_temp_name ();
      DUMP_OPCODE (this, lhs);
      return lhs;
    }
  else if (tok.type == TOK_NAME)
    return tok.data.uid;
  else if (tok.type == TOK_NULL || tok.type == TOK_BOOL 
          || tok.type == TOK_INT || tok.type == TOK_STRING)
    return parse_literal ();
  else if (tok.type == TOK_OPEN_SQUARE)
    return parse_array_literal ();
  else if (tok.type == TOK_OPEN_BRACE)
    return parse_object_literal ();
  else if (tok.type == TOK_OPEN_PAREN)
    {
      skip_newlines ();
      if (tok.type != TOK_CLOSE_PAREN)
        {
          lhs = parse_expression ();
          token_after_newlines_must_be (TOK_CLOSE_PAREN);
          return lhs;
        }
    }
  JERRY_UNREACHABLE ();
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
static T_IDX
parse_member_expression (void)
{
  T_IDX lhs, obj, prop;
  if (is_keyword (KW_FUNCTION))
    obj = parse_function_expression ();
  else if (is_keyword (KW_NEW))
    {
      T_IDX member;

      NEXT (member, member_expression);

      obj = parse_argument_list (AL_CONSTRUCT_EXPR, member);
    }
  else
    obj = parse_primary_expression ();

  skip_newlines ();
  while (tok.type == TOK_OPEN_SQUARE || tok.type == TOK_DOT)
    {
      lhs = next_temp_name ();

      if (tok.type == TOK_OPEN_SQUARE)
        {
          NEXT (prop, expression);
          next_token_must_be (TOK_CLOSE_SQUARE);
        }
      else if (tok.type == TOK_DOT)
        {
          skip_newlines ();
          if (tok.type != TOK_NAME)
            parser_fatal (ERR_PARSER);
          prop = tok.data.uid;
        }
      else 
        JERRY_UNREACHABLE ();

      DUMP_OPCODE (prop_access, lhs, obj, prop);
      obj = lhs;
      skip_newlines ();
    }

  lexer_save_token (tok);

  return obj;
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
  : '(' LT!* assignment_expression LT!* ( ',' LT!* assignment_expression * LT!* )* ')'
  ; */
static T_IDX
parse_call_expression (void)
{
  T_IDX lhs, obj, prop;

  obj = parse_member_expression ();

  skip_newlines ();
  if (tok.type != TOK_OPEN_PAREN)
    {
      lexer_save_token (tok);
      return obj;
    }

  lhs = parse_argument_list (AL_CALL_EXPR, obj);
  obj = lhs;

  skip_newlines ();
  while (tok.type == TOK_OPEN_PAREN || tok.type == TOK_OPEN_SQUARE 
         || tok.type == TOK_DOT)
    {
      switch (tok.type)
      {
        case TOK_OPEN_PAREN:
          lhs = parse_argument_list (AL_CALL_EXPR, obj);
          skip_newlines ();
          break;

        case TOK_OPEN_SQUARE:
          NEXT (prop, expression);
          next_token_must_be (TOK_CLOSE_SQUARE);

          DUMP_OPCODE (prop_access, lhs, obj, prop);
          obj = lhs;
          skip_newlines ();
          break;

        case TOK_DOT:
          token_after_newlines_must_be (TOK_NAME);
          prop = tok.data.uid;

          DUMP_OPCODE (prop_access, lhs, obj, prop);
          obj = lhs;
          skip_newlines ();
          break;

        default:
          JERRY_UNREACHABLE ();
      }
    }
  lexer_save_token (tok);

  return obj;
}

/* left_hand_side_expression
  : call_expression
  | new_expression
  ; */
static T_IDX
parse_left_hand_side_expression (void)
{
  return parse_call_expression ();
}

/* postfix_expression
  : left_hand_side_expression ('++' | '--')?
  ; */
static T_IDX
parse_postfix_expression (void)
{
  T_IDX expr = parse_left_hand_side_expression ();

  tok = lexer_next_token ();
  if (tok.type == TOK_DOUBLE_PLUS)
    {
      opcode = getop_addition (expr, expr, integer_one ());
      save_opcode ();
    }
  else if (tok.type == TOK_DOUBLE_MINUS)
    {
      opcode = getop_substraction (expr, expr, integer_one ());
      save_opcode ();
    }
  else
    lexer_save_token (tok);

  return expr;
}

/* unary_expression
  : postfix_expression
  | ('delete' | 'void' | 'typeof' | '++' | '--' | '+' | '-' | '~' | '!') unary_expression
  ; */
static T_IDX
parse_unary_expression (void)
{
  T_IDX expr, lhs;

  switch (tok.type)
  {
    case TOK_DOUBLE_PLUS:
      NEXT (expr, unary_expression);
      DUMP_OPCODE (addition, expr, expr, integer_one ());
      return expr;

    case TOK_DOUBLE_MINUS:
      NEXT (expr, unary_expression);
      DUMP_OPCODE (substraction, expr, expr, integer_one ());
      return expr;

    case TOK_PLUS:
      lhs = next_temp_name ();
      NEXT (expr, unary_expression);
      DUMP_OPCODE (addition, lhs, integer_zero (), expr);
      return lhs;

    case TOK_MINUS:
      lhs = next_temp_name ();
      NEXT (expr, unary_expression);
      DUMP_OPCODE (substraction, lhs, integer_zero (), expr);
      return lhs;

    case TOK_COMPL:
      lhs = next_temp_name ();
      NEXT (expr, unary_expression);
      DUMP_OPCODE (b_not, lhs, expr);
      return lhs;

    case TOK_NOT:
      lhs = next_temp_name ();
      NEXT (expr, unary_expression);
      DUMP_OPCODE (logical_not, lhs, expr);
      return lhs;

    case TOK_KEYWORD:
      if (is_keyword (KW_DELETE)) 
        { 
          lhs = next_temp_name ();
          NEXT (expr, unary_expression);
          DUMP_OPCODE (delete, lhs, expr);
          return lhs;
        }
      if (is_keyword (KW_VOID)) 
        JERRY_UNIMPLEMENTED ();
      if (is_keyword (KW_TYPEOF))
        { 
          lhs = next_temp_name ();
          NEXT (expr, unary_expression);
          DUMP_OPCODE (typeof, lhs, expr);
          return lhs;
        }
      /* FALLTHRU.  */

    default:
      return parse_postfix_expression ();
  }
}

#define DUMP_OF(GETOP, EXPR) \
    lhs = next_temp_name (); \
    NEXT (expr2, EXPR);\
    DUMP_OPCODE (GETOP, lhs, expr1, expr2); \
    expr1 = lhs; \
    break;

/* multiplicative_expression
  : unary_expression (LT!* ('*' | '/' | '%') LT!* unary_expression)*
  ; */
static T_IDX
parse_multiplicative_expression (void)
{
  T_IDX lhs, expr1, expr2;

  expr1 = parse_unary_expression ();

  skip_newlines ();
  while (true)
    {
      switch (tok.type)
      {
        case TOK_MULT: DUMP_OF (multiplication, unary_expression)
        case TOK_DIV: DUMP_OF (division, unary_expression)
        case TOK_MOD: DUMP_OF (remainder, unary_expression)
        
        default:
          lexer_save_token (tok);
          return expr1;
      }

      skip_newlines ();
    }
}

/* additive_expression
  : multiplicative_expression (LT!* ('+' | '-') LT!* multiplicative_expression)*
  ; */
static T_IDX 
parse_additive_expression (void)
{
  T_IDX lhs, expr1, expr2;

  expr1 = parse_multiplicative_expression ();

  skip_newlines ();
  while (true)
    {
      switch (tok.type)
      {
        case TOK_PLUS: DUMP_OF (addition, multiplicative_expression);
        case TOK_MINUS: DUMP_OF (substraction, multiplicative_expression);
        
        default:
          lexer_save_token (tok);
          return expr1;
      }

      skip_newlines ();
    }
}

/* shift_expression
  : additive_expression (LT!* ('<<' | '>>' | '>>>') LT!* additive_expression)*
  ; */
static T_IDX
parse_shift_expression (void)
{
  T_IDX lhs, expr1, expr2;

  expr1 = parse_additive_expression ();

  skip_newlines ();
  while (true)
    {
      switch (tok.type)
      {
        case TOK_LSHIFT: DUMP_OF (b_shift_left, additive_expression)
        case TOK_RSHIFT: DUMP_OF (b_shift_right, additive_expression)
        case TOK_RSHIFT_EX: DUMP_OF (b_shift_uright, additive_expression)
        
        default:
          lexer_save_token (tok);
          return expr1;
      }

      skip_newlines ();
    }
}

/* relational_expression
  : shift_expression (LT!* ('<' | '>' | '<=' | '>=' | 'instanceof' | 'in') LT!* shift_expression)*
  ; */
static T_IDX
parse_relational_expression (void)
{
  T_IDX lhs, expr1, expr2;

  expr1 = parse_shift_expression ();

  skip_newlines ();
  while (true)
    {
      switch (tok.type)
      {
        case TOK_LESS: DUMP_OF (less_than, shift_expression)
        case TOK_GREATER: DUMP_OF (greater_than, shift_expression)
        case TOK_LESS_EQ: DUMP_OF (less_or_equal_than, shift_expression)
        case TOK_GREATER_EQ: DUMP_OF (greater_or_equal_than, shift_expression)
        case TOK_KEYWORD:
          if (is_keyword (KW_INSTANCEOF))
            {
              DUMP_OF (instanceof, shift_expression)
            }
          else if (is_keyword (KW_IN))
            {
              DUMP_OF (in, shift_expression)
            }
          // FALLTHRU
        
        default:
          lexer_save_token (tok);
          return expr1;
      }

      skip_newlines ();
    }
}

/* equality_expression
  : relational_expression (LT!* ('==' | '!=' | '===' | '!==') LT!* relational_expression)*
  ; */
static T_IDX
parse_equality_expression (void)
{
  T_IDX lhs, expr1, expr2;

  expr1 = parse_relational_expression ();

  skip_newlines ();
  while (true)
    {
      switch (tok.type)
      {
        case TOK_DOUBLE_EQ: DUMP_OF (equal_value, relational_expression)
        case TOK_NOT_EQ: DUMP_OF (not_equal_value, relational_expression)
        case TOK_TRIPLE_EQ: DUMP_OF (equal_value_type, relational_expression)
        case TOK_NOT_DOUBLE_EQ: DUMP_OF (not_equal_value_type, relational_expression)

        default:
          lexer_save_token (tok);
          return expr1;
      }

      skip_newlines ();
    }
}

#define PARSE_OF(FUNC, EXPR, TOK_TYPE, GETOP) \
static T_IDX parse_##FUNC (void) { \
  T_IDX lhs, expr1, expr2; \
  expr1 = parse_##EXPR (); \
  skip_newlines (); \
  while (true) \
  { \
    switch (tok.type) \
    { \
      case TOK_##TOK_TYPE: DUMP_OF (GETOP, EXPR) \
      default: lexer_save_token (tok); return expr1; \
    } \
    skip_newlines (); \
  } \
}

/* bitwise_and_expression
  : equality_expression (LT!* '&' LT!* equality_expression)*
  ; */
PARSE_OF (bitwise_and_expression, equality_expression, AND, b_and)

/* bitwise_xor_expression
  : bitwise_and_expression (LT!* '^' LT!* bitwise_and_expression)*
  ; */
PARSE_OF (bitwise_xor_expression, bitwise_and_expression, XOR, b_xor)

/* bitwise_or_expression
  : bitwise_xor_expression (LT!* '|' LT!* bitwise_xor_expression)*
  ; */
PARSE_OF (bitwise_or_expression, bitwise_xor_expression, OR, b_or)

/* logical_and_expression
  : bitwise_or_expression (LT!* '&&' LT!* bitwise_or_expression)*
  ; */
PARSE_OF (logical_and_expression, bitwise_or_expression, DOUBLE_AND, logical_and)

/* logical_or_expression
  : logical_and_expression (LT!* '||' LT!* logical_and_expression)*
  ; */
PARSE_OF (logical_or_expression, logical_and_expression, DOUBLE_OR, logical_or)

/* conditional_expression
  : logical_or_expression (LT!* '?' LT!* assignment_expression LT!* ':' LT!* assignment_expression)?
  ; */
static T_IDX
parse_conditional_expression (bool *was_conditional)
{
  T_IDX expr = parse_logical_or_expression ();

  skip_newlines ();
  if (tok.type == TOK_QUERY)
    {
      T_IDX lhs, jmp_oc, res = next_temp_name ();

      DUMP_OPCODE (is_true_jmp, expr, (uint8_t) (opcode_counter + 2));
      jmp_oc = opcode_counter;
      DUMP_OPCODE (jmp_down, INVALID_VALUE);

      NEXT (lhs, assignment_expression);
      DUMP_OPCODE (assignment, res, OPCODE_ARG_TYPE_VARIABLE, lhs);
      token_after_newlines_must_be (TOK_COLON);

      REWRITE_OPCODE (jmp_oc, jmp_down, (uint8_t) (opcode_counter - jmp_oc));
      jmp_oc = opcode_counter;
      DUMP_OPCODE (jmp_down, INVALID_VALUE);

      NEXT (lhs, assignment_expression);
      DUMP_OPCODE (assignment, res, OPCODE_ARG_TYPE_VARIABLE, lhs);
      REWRITE_OPCODE (jmp_oc, jmp_down, (uint8_t) (opcode_counter - jmp_oc));

      *was_conditional = true;
      return res;
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
static T_IDX
parse_assignment_expression (void)
{
  T_IDX lhs, rhs, reg_var_decl_loc;
  bool was_conditional = false;

  if (!sub_assignment_exprs)
    {
      reset_temp_name ();
      reg_var_decl_loc = opcode_counter;
      DUMP_OPCODE (reg_var_decl, min_temp_name, INVALID_VALUE);
    }

  sub_assignment_exprs++;

  lhs = parse_conditional_expression (&was_conditional);
  if (was_conditional)
    {
      goto end;
    }

  skip_newlines ();
  switch (tok.type)
    {  
      case TOK_EQ:
        NEXT (rhs, assignment_expression);
        DUMP_OPCODE (assignment, lhs, OPCODE_ARG_TYPE_VARIABLE, rhs);
        break;

      case TOK_MULT_EQ:
        NEXT (rhs, assignment_expression);
        DUMP_OPCODE (multiplication, lhs, lhs, rhs);
        break;

      case TOK_DIV_EQ:
        NEXT (rhs, assignment_expression);
        DUMP_OPCODE (division, lhs, lhs, rhs);
        break;
        
      case TOK_MOD_EQ:
        NEXT (rhs, assignment_expression);
        DUMP_OPCODE (remainder, lhs, lhs, rhs);
        break;
        
      case TOK_PLUS_EQ:
        NEXT (rhs, assignment_expression);
        DUMP_OPCODE (addition, lhs, lhs, rhs);
        break;
        
      case TOK_MINUS_EQ:
        NEXT (rhs, assignment_expression);
        DUMP_OPCODE (substraction, lhs, lhs, rhs);
        break;
        
      case TOK_LSHIFT_EQ:
        NEXT (rhs, assignment_expression);
        DUMP_OPCODE (b_shift_left, lhs, lhs, rhs);
        break;
        
      case TOK_RSHIFT_EQ:
        NEXT (rhs, assignment_expression);
        DUMP_OPCODE (b_shift_right, lhs, lhs, rhs);
        break;
        
      case TOK_RSHIFT_EX_EQ:
        NEXT (rhs, assignment_expression);
        DUMP_OPCODE (b_shift_uright, lhs, lhs, rhs);
        break;
        
      case TOK_AND_EQ:
        NEXT (rhs, assignment_expression);
        DUMP_OPCODE (b_and, lhs, lhs, rhs);
        break;
        
      case TOK_XOR_EQ:
        NEXT (rhs, assignment_expression);
        DUMP_OPCODE (b_xor, lhs, lhs, rhs);
        break;
        
      case TOK_OR_EQ:
        NEXT (rhs, assignment_expression);
        DUMP_OPCODE (b_or, lhs, lhs, rhs);
        break;
        
      default:
        lexer_save_token (tok);
  }

end:

  sub_assignment_exprs--;
  if (!sub_assignment_exprs)
    {
      REWRITE_OPCODE (reg_var_decl_loc, reg_var_decl, min_temp_name, (uint8_t) (temp_name - 1));
      dump_saved_opcodes ();
    }
  return lhs;
}

/* expression
  : assignment_expression (LT!* ',' LT!* assignment_expression)*
  ;
  */
static T_IDX
parse_expression (void)
{
  T_IDX expr = parse_assignment_expression ();

  while (true)
    {
      skip_newlines ();
      if (tok.type == TOK_COMMA)
        NEXT (expr, assignment_expression);
      else
        {
          lexer_save_token (tok);
          return expr;
        }
    }
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
  T_IDX name, expr;

  current_token_must_be (TOK_NAME);
  name = tok.data.uid;
  DUMP_OPCODE (var_decl, name);

  skip_newlines ();
  if (tok.type == TOK_EQ)
    {
      NEXT (expr, assignment_expression);
      DUMP_OPCODE (assignment, name, OPCODE_ARG_TYPE_VARIABLE, expr);
    }
  else
    lexer_save_token (tok);
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
      if (tok.type != TOK_COMMA)
        {
          lexer_save_token (tok);
          return;
        }

      skip_newlines ();
      if (several_decls)
        *several_decls = true;
    }
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
  T_IDX stop, cond_oc, body_oc, step_oc, end_oc;

  assert_keyword  (KW_FOR);
  token_after_newlines_must_be (TOK_OPEN_PAREN);

  skip_newlines ();
  if (tok.type == TOK_SEMICOLON)
    goto plain_for;
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

  /* expression contains left_hand_side_expression.  */
  parse_expression ();

  skip_newlines ();
  if (tok.type == TOK_SEMICOLON)
    goto plain_for;
  else if (is_keyword (KW_IN))
    goto for_in;
  else 
    parser_fatal (ERR_PARSER);

  JERRY_UNREACHABLE ();

plain_for:
  /* Represent loop like 

     for (i = 0; i < 10; i++) {
       body;
     }

     as
      
  11   i = #0;
   cond_oc:
  12   tmp1 = #10;
  13   tmp2 = i < tmp1;
   end_oc:
  14   is_false_jmp tmp2, 8 // end of loop
   body_oc:
  15   jmp_down 5 // body
   step_oc:
  16   tmp3 = #1;
  17   tmp4 = i + 1;
  18   i = tmp4;
  19   jmp_up 7; // cond_oc

  20   body

  21   jmp_up 5; // step_oc;
  22   ...
  */
  cond_oc = opcode_counter;
  skip_newlines ();
  if (tok.type != TOK_SEMICOLON)
    {
      stop = parse_assignment_expression ();
      next_token_must_be (TOK_SEMICOLON);
    }
  else
    stop = integer_one ();

  end_oc = opcode_counter;
  DUMP_OPCODE (is_false_jmp, stop, INVALID_VALUE);

  body_oc = opcode_counter;
  DUMP_OPCODE (jmp_down, INVALID_VALUE);

  step_oc = opcode_counter;
  skip_newlines ();
  if (tok.type != TOK_CLOSE_PAREN)
    {
      parse_assignment_expression ();
      next_token_must_be (TOK_CLOSE_PAREN);
    }
  DUMP_OPCODE (jmp_up, (uint8_t) (opcode_counter - cond_oc));
  REWRITE_OPCODE (body_oc, jmp_down, (uint8_t) (opcode_counter - body_oc));

  skip_newlines ();
  parse_statement ();

  DUMP_OPCODE (jmp_up, (uint8_t) (opcode_counter - step_oc));
  REWRITE_OPCODE (end_oc, is_false_jmp, stop, opcode_counter);
  return;

for_in:
  JERRY_UNIMPLEMENTED ();

}

static T_IDX
parse_expression_inside_parens (void)
{
  T_IDX expr;
  token_after_newlines_must_be (TOK_OPEN_PAREN);
  NEXT (expr, expression);
  token_after_newlines_must_be (TOK_CLOSE_PAREN);
  return expr;
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

      tok = lexer_next_token ();
      if (tok.type != TOK_NEWLINE && tok.type != TOK_SEMICOLON)
        {
          lexer_save_token (tok);
          return;
        }

      skip_newlines ();
    }
}

/* if_statement
  : 'if' LT!* '(' LT!* expression LT!* ')' LT!* statement (LT!* 'else' LT!* statement)?
  ; */
static void
parse_if_statement (void)
{
  T_IDX cond, cond_oc;
  assert_keyword (KW_IF);

  cond = parse_expression_inside_parens ();
  cond_oc = opcode_counter;
  DUMP_OPCODE (is_false_jmp, cond, INVALID_VALUE);

  skip_newlines ();
  parse_statement ();

  REWRITE_OPCODE (cond_oc, is_false_jmp, cond, opcode_counter);

  skip_newlines ();
  if (is_keyword (KW_ELSE))
    {
      skip_newlines ();
      parse_statement ();
    }
  else
    lexer_save_token (tok);
}

/* do_while_statement
  : 'do' LT!* statement LT!* 'while' LT!* '(' expression ')' (LT | ';')!
  ; */
static void
parse_do_while_statement (void)
{
  T_IDX cond, loop_oc;

  assert_keyword (KW_DO);

  loop_oc = opcode_counter;

  skip_newlines ();
  parse_statement ();

  token_after_newlines_must_be_keyword (KW_WHILE);
  cond = parse_expression_inside_parens ();
  DUMP_OPCODE (is_true_jmp, cond, loop_oc);
}

/* while_statement
  : 'while' LT!* '(' LT!* expression LT!* ')' LT!* statement
  ; */
static void
parse_while_statement (void)
{
  T_IDX cond, cond_oc, jmp_oc;

  assert_keyword (KW_WHILE);

  cond_oc = opcode_counter;
  cond = parse_expression_inside_parens ();
  jmp_oc = opcode_counter;
  DUMP_OPCODE (is_false_jmp, cond, INVALID_VALUE);

  skip_newlines ();
  parse_statement ();

  DUMP_OPCODE (jmp_up, (uint8_t) (opcode_counter - cond_oc));
  REWRITE_OPCODE (jmp_oc, is_false_jmp, cond, opcode_counter);
}

/* with_statement
  : 'with' LT!* '(' LT!* expression LT!* ')' LT!* statement
  ; */
static void
parse_with_statement (void)
{
  T_IDX expr;
  assert_keyword (KW_WITH);
  expr = parse_expression_inside_parens ();

  DUMP_OPCODE (with, expr); 

  skip_newlines ();
  parse_statement ();

  DUMP_VOID_OPCODE (end_with);
}

/* switch_statement
  : 'switch' LT!* '(' LT!* expression LT!* ')' LT!* '{' LT!* case_block LT!* '}'
  ; */
static void
parse_switch_statement (void)
{
  JERRY_UNIMPLEMENTED ();
}

/* try_statement
  : 'try' LT!* '{' LT!* statement_list LT!* '}' LT!* (finally_clause | catch_clause (LT!* finally_clause)?)
  ;

   catch_clause
  : 'catch' LT!* '(' LT!* Identifier LT!* ')' LT!* '{' LT!* statement_list LT!* '}'
  ;
  
   finally_clause
  : 'finally' LT!* '{' LT!* statement_list LT!* '}'
  ;*/
static void
parse_try_statement (void)
{
  JERRY_UNIMPLEMENTED ();
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
  if (tok.type == TOK_CLOSE_BRACE)
    {
      lexer_save_token (tok);
      return;
    }
  if (tok.type == TOK_OPEN_BRACE)
    {
      skip_newlines ();
      if (tok.type != TOK_CLOSE_BRACE)
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
  if (tok.type == TOK_SEMICOLON)
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
      JERRY_UNIMPLEMENTED ();
    }
  if (is_keyword (KW_BREAK))
    {
      JERRY_UNIMPLEMENTED ();
    }
  if (is_keyword (KW_RETURN))
    {
      T_IDX expr;
      tok = lexer_next_token ();
      if (tok.type != TOK_SEMICOLON)
        {
          expr = parse_expression ();
          DUMP_OPCODE (retval, expr);
        }
      else
        DUMP_VOID_OPCODE (ret);
      return;
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
      JERRY_UNIMPLEMENTED ();
    }
  if (is_keyword (KW_TRY))
    {
      parse_try_statement ();
      return;
    }
  if (tok.type == TOK_NAME)
    {
      token saved = tok;
      skip_newlines ();
      if (tok.type == TOK_COLON)
        {
          // STMT_LABELLED;
          JERRY_UNIMPLEMENTED ();
        }
      else
        {
          lexer_save_token (tok);
          tok = saved;
          parse_expression ();
          return;
        }
    }
  else
    {
      parse_expression ();
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
    parse_function_declaration ();
  else
    parse_statement ();
}

/* source_element_list
  : source_element (LT!* source_element)*
  ; */
static void
parse_source_element_list (void)
{
  while (tok.type != TOK_EOF && tok.type != TOK_CLOSE_BRACE)
    {
      parse_source_element ();
      skip_newlines ();
    }
  lexer_save_token (tok);
}

/* program
  : LT!* source_element_list LT!* EOF!
  ; */
void
parser_parse_program (void)
{
  skip_newlines ();
  parse_source_element_list ();

  skip_newlines ();
  JERRY_ASSERT (tok.type == TOK_EOF);
  DUMP_OPCODE (exitval, 0);
}

void
parser_init (void)
{
  temp_name = min_temp_name = lexer_get_reserved_ids_count ();
#ifdef __HOST
  debug_file = __fopen ("parser.log", "w");
#endif
}

void
parser_fatal (jerry_status_t code)
{
  __printf ("FATAL: %d\n", code);
  lexer_dump_buffer_state ();

  jerry_exit( code);
}

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

#include "bytecode-generator.h"
#include "globals.h"
#include "libcoreint/opcodes.h"
#include "libruntime/serializer.h"
#include "libruntime/jerry-libc.h"

static uint8_t opcode_index;

#define MAX_STACK_SIZE 10

static uint8_t opcode_stack[10];
static uint8_t stack_head;

static void
push_opcode (uint8_t opcode)
{
  JERRY_ASSERT (stack_head < MAX_STACK_SIZE);
  opcode_stack[stack_head++] = opcode;
}

static uint8_t
pop_opcode (void)
{
  return opcode_stack[--stack_head];
}

void
generator_init (void)
{
  opcode_index = 0;
  stack_head = 0;
}

void
generator_dump_strings (const char **strings, uint8_t num)
{
  uint8_t len = num, i;

  for (i = 0; i < num; i++)
    {
      serializer_dump_data (&len, 1);
      len = (uint8_t) (len + __strlen (strings[i]));
    }

  for (i = 0; i < num; i++)
    serializer_dump_data (strings[i], __strlen (strings[i]) + 1);
}

static bool
is_expression (statement stmt)
{
  switch (stmt.type)
  {
    case STMT_IF:
    case STMT_ELSE_IF:
    case STMT_END_DO_WHILE:
    case STMT_WHILE:
    case STMT_RETURN:
    case STMT_WITH:
    case STMT_SWITCH:
    case STMT_CASE:
    case STMT_THROW:
    case STMT_EXPRESSION:
      return true;

    default:
      return false;
  }
}

static bool
is_assignment (statement stmt)
{
  JERRY_ASSERT (is_expression (stmt));
  return stmt.data.expr.oper != AO_NONE;
}

static expression_type
get_expression_type (statement stmt)
{
  JERRY_ASSERT (is_expression (stmt));
  return stmt.data.expr.type;
}

static bool
expression_has_operands (statement stmt)
{
  switch (get_expression_type (stmt))
  {
    case ET_OBJECT:
    case ET_FUNCTION:
    case ET_ARRAY:
    case ET_SUBEXPRESSION:
    case ET_NONE:
      return false;

    default:
      return true;
  }
}

static operand
first_operand (statement stmt)
{
  JERRY_ASSERT (expression_has_operands (stmt));
  return stmt.data.expr.data.ops.op1;
}

static literal
first_operand_as_literal (statement stmt)
{
  operand oper = first_operand (stmt);

  JERRY_ASSERT (oper.is_literal);

  return oper.data.lit;
}

static opcode_arg_type_operand
first_operand_type (statement stmt)
{
  JERRY_ASSERT (expression_has_operands (stmt));
  if (!first_operand (stmt).is_literal)
    {
      return OPCODE_ARG_TYPE_VARIABLE;
    }

  literal lit = first_operand(stmt).data.lit;

  TODO( Small integer -> OPCODE_ARG_TYPE_SMALLINT );

  switch ( lit.type )
    {
    case LIT_NULL:
    case LIT_BOOL:
      return OPCODE_ARG_TYPE_SIMPLE;
    case LIT_INT:
      return OPCODE_ARG_TYPE_NUMBER;
    case LIT_STR:
      return OPCODE_ARG_TYPE_STRING;
    }

  JERRY_UNREACHABLE();
}

static uint8_t
first_operand_id (statement stmt)
{
  JERRY_ASSERT (expression_has_operands (stmt));
  if (first_operand (stmt).is_literal)
    JERRY_UNIMPLEMENTED ();
  return first_operand (stmt).data.name;
}

static operand
second_operand (statement stmt)
{
  JERRY_ASSERT (expression_has_operands (stmt));
  return stmt.data.expr.data.ops.op2;
}

static uint8_t
second_operand_id (statement stmt)
{
  JERRY_ASSERT (expression_has_operands (stmt));
  if (second_operand (stmt).is_literal)
    JERRY_UNIMPLEMENTED ();
  return second_operand (stmt).data.name;
}

static uint8_t
lhs (statement stmt)
{
  JERRY_ASSERT (is_assignment (stmt));

  return stmt.data.expr.var;
}

static void
dump_opcode (OPCODE *opcode)
{
  serializer_dump_data (opcode, sizeof (OPCODE));
  opcode_index++;
}

static assignment_operator
get_assignment_operator (statement stmt)
{
  JERRY_ASSERT (is_assignment (stmt));

  return stmt.data.expr.oper;
}

static OPCODE
generate_triple_address (OPCODE (*getop)(T_IDX, T_IDX, T_IDX), statement stmt)
{
  return (*getop) (lhs (stmt), first_operand_id (stmt), second_operand_id (stmt));
}

void
generator_dump_statement (statement stmt)
{
  OPCODE opcode = getop_nop ();
  JERRY_STATIC_ASSERT (sizeof (OPCODE) <= sizeof (uint32_t));

  switch (stmt.type)
  { 
    case STMT_EMPTY:
      break;

    case STMT_WHILE:
      if (!is_assignment (stmt))
        {
          literal lit;

          switch (get_expression_type (stmt))
          {
            case ET_LITERAL:
              lit = first_operand_as_literal (stmt);
              if (lit.type == LIT_BOOL && lit.data.is_true)
                {
                  opcode = getop_loop_inf ((uint8_t) (opcode_index + 1));
                  push_opcode ((uint8_t) (opcode_index + 1));
                  dump_opcode (&opcode);
                  return;
                }
              else
                JERRY_UNIMPLEMENTED ();
              break;

            default:
              JERRY_UNIMPLEMENTED ();
          }
        }
      else
        JERRY_UNIMPLEMENTED ();
      break;

    case STMT_EXPRESSION:
      if (is_assignment (stmt))
        {
          switch (get_expression_type (stmt))
          {
            case ET_NONE:
              JERRY_UNREACHABLE ();
              break;

            case ET_LOGICAL_OR:
              opcode = generate_triple_address (getop_logical_or, stmt);
              dump_opcode (&opcode);
              break;

            case ET_LOGICAL_AND:
              opcode = generate_triple_address (getop_logical_and, stmt);
              dump_opcode (&opcode);
              break;

            case ET_BITWISE_OR:
              opcode = generate_triple_address (getop_b_or, stmt);
              dump_opcode (&opcode);
              break;

            case ET_BITWISE_XOR:
              opcode = generate_triple_address (getop_b_xor, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_BITWISE_AND:
              opcode = generate_triple_address (getop_b_and, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_DOUBLE_EQ:
              opcode = generate_triple_address (getop_equal_value, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_NOT_EQ:
              opcode = generate_triple_address (getop_not_equal_value, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_TRIPLE_EQ:
              opcode = generate_triple_address (getop_equal_value_type, stmt);
              dump_opcode (&opcode);
              break;

            case ET_NOT_DOUBLE_EQ:
              opcode = generate_triple_address (getop_not_equal_value_type, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_LESS:
              opcode = generate_triple_address (getop_less_than, stmt);
              dump_opcode (&opcode);
              break;

            case ET_GREATER:
              opcode = generate_triple_address (getop_greater_than, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_LESS_EQ:
              opcode = generate_triple_address (getop_less_or_equal_than, stmt);
              dump_opcode (&opcode);
              break;

            case ET_GREATER_EQ:
              opcode = generate_triple_address (getop_greater_or_equal_than, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_INSTANCEOF:
            case ET_IN:
              JERRY_UNIMPLEMENTED ();
              break;

            case ET_LSHIFT:
              opcode = generate_triple_address (getop_b_shift_left, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_RSHIFT:
              opcode = generate_triple_address (getop_b_shift_right, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_RSHIFT_EX:
              opcode = generate_triple_address (getop_b_shift_uright, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_PLUS:
              opcode = generate_triple_address (getop_addition, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_MINUS:
              opcode = generate_triple_address (getop_substraction, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_MULT:
              opcode = generate_triple_address (getop_multiplication, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_DIV:
              opcode = generate_triple_address (getop_division, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_MOD:
              opcode = generate_triple_address (getop_remainder, stmt);
              dump_opcode (&opcode);
              break;
              
            case ET_UNARY_DELETE:
            case ET_UNARY_VOID:
            case ET_UNARY_TYPEOF:
              JERRY_UNIMPLEMENTED ();
              break;

            case ET_UNARY_INCREMENT:
              // opcode = getop_addition (first_operand_id (stmt), first_operand_id (stmt), INTEGER_ONE);
            case ET_UNARY_DECREMENT:
            case ET_UNARY_PLUS:
            case ET_UNARY_MINUS:
            case ET_UNARY_COMPL:
            case ET_UNARY_NOT:
            case ET_POSTFIX_INCREMENT:
            case ET_POSTFIX_DECREMENT:
            case ET_CALL:
            case ET_NEW:
            case ET_INDEX:
            case ET_PROP_REF:
            case ET_OBJECT:
            case ET_FUNCTION:
            case ET_ARRAY:
            case ET_SUBEXPRESSION:
              JERRY_UNIMPLEMENTED ();
              break;

            case ET_LITERAL:
            case ET_IDENTIFIER:
              switch (get_assignment_operator (stmt))
              {
                case AO_NONE:
                  JERRY_UNREACHABLE ();
                  break;

                case AO_EQ:
                  opcode = getop_assignment (lhs (stmt),
                                             first_operand_type(stmt),
                                             first_operand_id (stmt));
                  dump_opcode (&opcode);
                  return;

                default:
                  JERRY_UNIMPLEMENTED ();
              }
              JERRY_UNREACHABLE ();

            default:
              JERRY_UNREACHABLE ();
          }

          if (get_assignment_operator (stmt) != AO_EQ)
            JERRY_UNIMPLEMENTED ();

        }
      else
        {
          call_expression expr = stmt.data.expr.data.call_expr;
          JERRY_ASSERT (!is_operand_list_empty (expr.args));
          if (!is_operand_empty (expr.args.ops[1]))
            JERRY_UNREACHABLE ();
          if (expr.args.ops[0].is_literal)
            JERRY_UNREACHABLE ();
          opcode = getop_call_1 (expr.name, expr.args.ops[0].data.name);
        }
      break;

    case STMT_END_WHILE:
      opcode = getop_jmp (pop_opcode ());
      break;

    default:
      __printf (" generator_dump_statement: %d ", stmt.type);
      JERRY_UNREACHABLE ();
  }
}

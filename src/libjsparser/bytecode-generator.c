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

void
generator_dump_statement (statement stmt)
{
  OPCODE opcode;
  JERRY_STATIC_ASSERT (sizeof (OPCODE) <= sizeof (uint32_t));

  switch (stmt.type)
  { 
    case STMT_EMPTY:
      break;

    case STMT_WHILE:
      TODO (Supports only infinite loops);
      if (stmt.data.expr.oper == AO_NONE && stmt.data.expr.type == ET_NONE)
        {
          operand op = stmt.data.expr.data.ops.op1;
          if (op.is_literal && op.data.lit.type == LIT_BOOL && op.data.lit.data.is_true)
            {
              opcode = getop_loop_inf ((uint8_t) (opcode_index + 1));
              push_opcode ((uint8_t) (opcode_index + 1));
            }
        }
      break;

    case STMT_EXPRESSION:
      TODO (Supports only calls);
      if (stmt.data.expr.oper == AO_NONE)
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

  serializer_dump_data (&opcode, sizeof (OPCODE));
  opcode_index++;
}
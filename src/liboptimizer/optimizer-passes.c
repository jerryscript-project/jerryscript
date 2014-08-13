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

#include "optimizer-passes.h"
#include "opcodes.h"
#include "deserializer.h"
#include "serializer.h"
#include "jerry-libc.h"

#define NAME_TO_ID(op) (__op__idx_##op)

/* Reorder bytecode like

   call_n ...
   assignment ... +
   var_?_end

   to

   assignment ... +
   call_n ...
   var_?_end
 */
static void
optimize_calls (OPCODE *opcodes)
{
  OPCODE *current_opcode;

  for (current_opcode = opcodes;
       current_opcode->op_idx != NAME_TO_ID (exitval);
       current_opcode++)
  {
    if (current_opcode->op_idx == NAME_TO_ID (call_n)
        && (current_opcode + 1)->op_idx == NAME_TO_ID (assignment))
    {
      OPCODE temp = *current_opcode;
      *current_opcode = *(current_opcode + 1);
      *(current_opcode + 1) = temp;
    }
  }
}

/* Move NUMBER opcodes from FROM to TO and adjust opcodes beetwen FROM and TO.  */
void
optimizer_move_opcodes (OPCODE *from, OPCODE *to, uint16_t number)
{
  OPCODE temp[number], *current_opcode;
  uint16_t i;

  if (to == from)
  {
    return;
  }

  for (i = 0; i < number; i++)
  {
    temp[i] = from[i];
  }

  if (to > from)
  {
    if (number <= to - from)
    {
      // Adjust opcodes up
      for (current_opcode = from; current_opcode != to; current_opcode++)
      {
        *current_opcode = *(current_opcode + number);
      }
    }
    else
    {
      optimizer_move_opcodes (from + number, from, (uint16_t) (to - from));
    }
  }
  else
  {
    if (number <= from - to)
    {
      // Adjust opcodes down
      for (current_opcode = from; current_opcode != to; current_opcode--)
      {
        *current_opcode = *(current_opcode - number);
      }
    }
    else
    {
      optimizer_move_opcodes (to, to + number, (uint16_t) (from - to));
    }
  }

  for (i = 0; i < number; i++)
  {
    to[i] = temp[i];
  }
}

static uint16_t
opcode_to_counter (OPCODE *opcode)
{
  JERRY_ASSERT (opcode > (OPCODE *) deserialize_bytecode ());
  return (uint16_t) (opcode - (OPCODE *) deserialize_bytecode ());
}

void
optimizer_adjust_jumps (OPCODE *first_opcode, OPCODE *last_opcode, int16_t value)
{
  OPCODE *current_opcode;

  JERRY_ASSERT (first_opcode <= last_opcode);

  for (current_opcode = first_opcode; current_opcode != last_opcode; current_opcode++)
  {
    if (current_opcode->op_idx == NAME_TO_ID (is_true_jmp))
    {
      /* 19: is_true_jmp 2
         20: var_decl ...

         becomes

         19: var_decl ...
         20: is_true_jmp 2
       */
      if (current_opcode->data.is_true_jmp.opcode >= opcode_to_counter (last_opcode)
          || current_opcode->data.is_true_jmp.opcode < opcode_to_counter (first_opcode) - value)
      {
        continue;
      }

      /* 19: is_true_jmp 20
         20: assignment
         21: var_decl

         becomes

         19: var_decl
         20: is_true_jmp 21
         21: assignment
       */
      if (current_opcode->data.is_true_jmp.opcode >= opcode_to_counter (first_opcode)
          && current_opcode->data.is_true_jmp.opcode <= opcode_to_counter (last_opcode) - value)
      {
        current_opcode->data.is_true_jmp.opcode = (T_IDX) (current_opcode->data.is_true_jmp.opcode + value);
        continue;
      }

      /* 19: is_true_jmp 22
         20: assignment
         21: var_decl
         22: var_decl

         becomes

         19: var_decl
         20: var_decl
         21: is_true_jmp 23
         22: assignment
       */
      if (current_opcode->data.is_true_jmp.opcode < opcode_to_counter (last_opcode))
      {
        current_opcode->data.is_true_jmp.opcode = (T_IDX) opcode_to_counter (last_opcode);
        continue;
      }

      JERRY_UNREACHABLE ();
    }

    if (current_opcode->op_idx == NAME_TO_ID (is_false_jmp))
    {
      /* 19: is_false_jmp 2
         20: var_decl ...

         becomes

         19: var_decl ...
         20: is_false_jmp 2
       */
      if (current_opcode->data.is_false_jmp.opcode >= opcode_to_counter (last_opcode)
          || current_opcode->data.is_false_jmp.opcode < opcode_to_counter (first_opcode) - value)
      {
        continue;
      }

      /* 19: is_false_jmp 20
         20: assignment
         21: var_decl

         becomes

         19: var_decl
         20: is_false_jmp 21
         21: assignment
       */
      if (current_opcode->data.is_false_jmp.opcode >= opcode_to_counter (first_opcode)
          && current_opcode->data.is_false_jmp.opcode <= opcode_to_counter (last_opcode) - value)
      {
        current_opcode->data.is_false_jmp.opcode = (T_IDX) (current_opcode->data.is_false_jmp.opcode + value);
        continue;
      }

      /* 19: is_false_jmp 22
         20: assignment
         21: var_decl
         22: var_decl

         becomes

         19: var_decl
         20: var_decl
         21: is_false_jmp 23
         22: assignment
       */
      if (current_opcode->data.is_false_jmp.opcode < opcode_to_counter (last_opcode))
      {
        current_opcode->data.is_false_jmp.opcode = (T_IDX) opcode_to_counter (last_opcode);
        continue;
      }

      JERRY_UNREACHABLE ();
    }

    if (current_opcode->op_idx == NAME_TO_ID (jmp_down))
    {
      /* 19: jmp_down 1
         20: assignment
         21: var_decl ...

         becomes

         19: var_decl ...
         20: jmp_down 1
         21: assignment
       */
      if (current_opcode->data.jmp_down.opcode_count < last_opcode - current_opcode)
      {
        continue;
      }

      /* 19: jmp_down 3
         20: assignment
         21: var_decl

         becomes

         19: var_decl
         20: jmp_down 2
         21: assignment
       */
      if (current_opcode->data.jmp_down.opcode_count >= last_opcode - current_opcode + value)
      {
        current_opcode->data.jmp_down.opcode_count = (T_IDX) (current_opcode->data.jmp_down.opcode_count - value);
        continue;
      }

      /* 19: jmp_down 3
         20: assignment
         21: var_decl
         22: var_decl

         becomes

         19: var_decl
         20: var_decl
         21: jmp_down 2
         22: assignment
       */
      if (current_opcode->data.jmp_down.opcode_count >= last_opcode - current_opcode
          && current_opcode->data.jmp_down.opcode_count < last_opcode - current_opcode + value)
      {
        current_opcode->data.jmp_down.opcode_count = (T_IDX) (last_opcode - current_opcode);
        continue;
      }

      JERRY_UNREACHABLE ();
    }

    if (current_opcode->op_idx == NAME_TO_ID (jmp_up))
    {
      /* 19: assignment
         20: jmp_up 1
         21: var_decl ...

         becomes

         19: var_decl ...
         20: assignment
         21: jmp_up 1
       */
      if (current_opcode->data.jmp_up.opcode_count < current_opcode - first_opcode)
      {
        continue;
      }

      /* 19: jmp_up 1
         20: assignment
         21: var_decl

         becomes

         19: var_decl
         20: jmp_up 2
         21: assignment
       */
      if (current_opcode->data.jmp_up.opcode_count >= current_opcode - first_opcode)
      {
        current_opcode->data.jmp_up.opcode_count = (T_IDX) (current_opcode->data.jmp_up.opcode_count + value);
        continue;
      }

      JERRY_UNREACHABLE ();
    }
  }
}

/* Reorder scope like
  "use strict"
  func_decl
  var_decl
  other opcodes
 */
void
optimizer_reorder_scope (uint16_t scope_start, uint16_t scope_end)
{
  OPCODE *opcodes = (OPCODE *) deserialize_bytecode ();
  OPCODE *first_opcode = opcodes + scope_start;
  OPCODE *last_opcode = opcodes + scope_end;
  OPCODE *current_opcode, *processed_opcode = first_opcode;
  OPCODE *var_decls_start;

  for (current_opcode = processed_opcode; current_opcode != last_opcode; current_opcode++)
  {
    if (current_opcode->op_idx == NAME_TO_ID (assignment)
        && current_opcode->data.assignment.type_value_right == OPCODE_ARG_TYPE_STRING
        && !__strcmp ("use strict", (char *) deserialize_string_by_id (current_opcode->data.assignment.value_right)))
    {
      optimizer_move_opcodes (current_opcode, processed_opcode, 1);
      optimizer_adjust_jumps (processed_opcode + 1, current_opcode + 1, 1);
      processed_opcode++;
      break;
    }
  }

  for (current_opcode = processed_opcode; current_opcode != last_opcode;)
  {
    if (current_opcode->op_idx == NAME_TO_ID (func_decl_0)
        || current_opcode->op_idx == NAME_TO_ID (func_decl_1)
        || current_opcode->op_idx == NAME_TO_ID (func_decl_2)
        || current_opcode->op_idx == NAME_TO_ID (func_decl_n))
    {
      OPCODE *fun_opcode;
      int16_t value, jmp_offset = 0;
      for (fun_opcode = current_opcode + 1; fun_opcode != last_opcode; fun_opcode++)
      {
        if (fun_opcode->op_idx == NAME_TO_ID (jmp_down))
        {
          jmp_offset = (int16_t) (fun_opcode - current_opcode);
          fun_opcode += fun_opcode->data.jmp_down.opcode_count;
          break;
        }
      }
      JERRY_ASSERT (fun_opcode <= last_opcode);

      value = (int16_t) (fun_opcode - current_opcode);
      optimizer_move_opcodes (current_opcode, processed_opcode, (uint16_t) value);
      // Adjust jumps inside func_decl except end's jmp_down
      optimizer_adjust_jumps (processed_opcode + jmp_offset + 1,
                              processed_opcode + value,
                              (int16_t) (processed_opcode - current_opcode));
      optimizer_adjust_jumps (processed_opcode + value,
                              fun_opcode,
                              value);
      processed_opcode += value;
      current_opcode = fun_opcode;
      continue;
    }
    else
    {
      current_opcode++;
    }
  }

  var_decls_start = processed_opcode;
  for (current_opcode = processed_opcode; current_opcode != last_opcode; current_opcode++)
  {
    if (current_opcode->op_idx == NAME_TO_ID (var_decl))
    {
      // If variable already declared, replace it with nop
      bool was_decl = false;
      if (var_decls_start->op_idx == NAME_TO_ID (var_decl) && var_decls_start != current_opcode)
      {
        OPCODE *var_decls_iterator;
        for (var_decls_iterator = var_decls_start;
             var_decls_iterator != processed_opcode;
             var_decls_iterator++)
        {
          JERRY_ASSERT (var_decls_iterator->op_idx == NAME_TO_ID (var_decl));
          if (var_decls_iterator->data.var_decl.variable_name
              == current_opcode->data.var_decl.variable_name)
          {
            was_decl = true;
            break;
          }
        }
      }

      if (was_decl)
      {
        current_opcode->op_idx = NAME_TO_ID (nop);
      }
      else
      {
        optimizer_move_opcodes (current_opcode, processed_opcode, 1);
        optimizer_adjust_jumps (processed_opcode + 1, current_opcode + 1, 1);
        processed_opcode++;
      }
    }
  }
}

void
optimizer_run_passes (OPCODE* opcodes)
{
  optimize_calls (opcodes);
}

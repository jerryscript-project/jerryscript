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
#include "jerry-libc.h"
#include "deserializer.h"

#define NAME_TO_ID(op) (__op__idx_##op)

#define FIELD(op, field) (opcode.data.op.field)

#define OPCODE_STR(op) \
  #op,

#define OPCODE_SIZE(op) \
  sizeof (struct __op_##op) + 1,

static char* opcode_names[] = {
  OP_LIST (OPCODE_STR)
  ""
};

static uint8_t opcode_sizes[] = {
  OP_LIST (OPCODE_SIZE)
  0
};

void
pp_strings (const char *strings[], uint8_t size)
{
  uint8_t i;
  uint16_t offset = (uint16_t) (size * 2 + 1);

  __printf ("STRINGS %d:\n", size);
  for (i = 0; i < size; i++)
    {
      __printf ("%3d %5d %20s\n", i, offset, strings[i]);
      offset = (uint16_t) (offset + __strlen (strings[i]) + 1);
    }
}

void 
pp_nums (const ecma_number_t nums[], uint8_t size, uint8_t strings_num)
{
  uint8_t i;

  __printf ("NUMS %d:\n", size);
  for (i = 0; i < size; i++)
    {
      __printf ("%3d %7d\n", i + strings_num, (int) nums[i]);
    }
  __printf ("\n");
}

static void
dump_variable (T_IDX id)
{
  if (id >= deserialize_min_temp ())
  {
    __printf ("tmp%d", id);
  }
  else if (deserialize_string_by_id (id))
  {
    __printf ("%s", deserialize_string_by_id (id));
  }
  else
  {
    __printf ("%d", (int) deserialize_num_by_id (id));
  }
}

#define CASE_CONDITIONAL_JUMP(op, string1, field1, string2, field2) \
  case NAME_TO_ID (op): \
    __printf (string1); \
    dump_variable (opcode.data.op.field1); \
    __printf (string2); \
    __printf (" %d;", opcode.data.op.field2); \
    break;

#define CASE_UNCONDITIONAL_JUMP(op, string, oc, oper, field) \
  case NAME_TO_ID (op): \
    __printf (string); \
    __printf (" %d;", oc oper opcode.data.op.field); \
    break;

#define CASE_TRIPLE_ADDRESS(op, lhs, equals, op1, oper, op2) \
  case NAME_TO_ID (op): \
    dump_variable (opcode.data.op.lhs); \
    __printf (" " equals " "); \
    dump_variable (opcode.data.op.op1); \
    __printf (" " oper " "); \
    dump_variable (opcode.data.op.op2); \
    __printf (";"); \
    break;

#define CASE_DOUBLE_ADDRESS(op, lhs, equals, oper, op2) \
  case NAME_TO_ID (op): \
    dump_variable (opcode.data.op.lhs); \
    __printf (" " equals " "); \
    __printf (oper " "); \
    dump_variable (opcode.data.op.op2); \
    __printf (";"); \
    break;

#define CASE_DOUBLE_ADDRESS_POST(op, lhs, equals, op2, oper) \
  case NAME_TO_ID (op): \
    dump_variable (opcode.data.op.lhs); \
    __printf (" " equals " "); \
    dump_variable (opcode.data.op.op2); \
    __printf (" " oper ";"); \
    break;

#define CASE_ASSIGNMENT(op, lhs, equals, op2) \
  case NAME_TO_ID (op): \
    dump_variable (opcode.data.op.lhs); \
    __printf (" " equals " "); \
    if (opcode.data.op.type_value_right == OPCODE_ARG_TYPE_SIMPLE) { \
      switch (opcode.data.op.op2) { \
        case ECMA_SIMPLE_VALUE_NULL: \
          __printf ("null"); \
          break; \
        case ECMA_SIMPLE_VALUE_FALSE: \
          __printf ("false"); \
          break; \
        case ECMA_SIMPLE_VALUE_TRUE: \
          __printf ("true"); \
          break; \
        default: \
          JERRY_UNREACHABLE (); \
      } \
      __printf (": SIMPLE"); \
    } else if (opcode.data.op.type_value_right == OPCODE_ARG_TYPE_STRING) { \
      __printf ("'%s'", deserialize_string_by_id (opcode.data.op.op2)); \
      __printf (": STRING"); \
    } else if (opcode.data.op.type_value_right == OPCODE_ARG_TYPE_NUMBER) {\
      __printf ("%d", deserialize_num_by_id (opcode.data.op.op2)); \
      __printf (": NUMBER"); \
    } else if (opcode.data.op.type_value_right == OPCODE_ARG_TYPE_SMALLINT) {\
      __printf ("%d", opcode.data.op.op2); \
      __printf (": NUMBER"); \
    } else if (opcode.data.op.type_value_right == OPCODE_ARG_TYPE_VARIABLE) {\
      dump_variable (opcode.data.op.op2); \
      __printf (": TYPEOF("); \
      dump_variable (opcode.data.op.op2); \
      __printf (")"); \
    } else { \
      JERRY_UNREACHABLE (); \
    } \
    __printf (";"); \
    break;

#define CASE_VARG_0_NAME_LHS(op, lhs, equals, new, name, start, end) \
  case NAME_TO_ID (op): \
    dump_variable (opcode.data.op.lhs); \
    __printf (" " equals " " new " "); \
    dump_variable (opcode.data.op.name); \
    __printf (start end ";"); \
    break;     

#define CASE_VARG_1_NAME_LHS(op, lhs, equals, new, name, start, arg, end) \
  case NAME_TO_ID (op): \
    dump_variable (opcode.data.op.lhs); \
    __printf (" " equals " " new " "); \
    dump_variable (opcode.data.op.name); \
    __printf (start); \
    dump_variable (opcode.data.op.arg); \
    __printf (end ";"); \
    break;     

#define CASE_VARG_N_NAME_LHS(op, lhs, equals, new, name, start, arg, end) \
  case NAME_TO_ID (op): \
    dump_variable (opcode.data.op.lhs); \
    __printf (" " equals " " new " "); \
    dump_variable (opcode.data.op.name); \
    __printf (start); \
    dump_variable (opcode.data.op.arg); \
    __printf (" ..."); \
    varg_end = end; \
    break;     

#define CASE_VARG_0_NAME(op, new, name, start, end) \
  case NAME_TO_ID (op): \
     __printf (new " "); \
     dump_variable (opcode.data.op.name); \
     __printf (start end ";"); \
     break;     

#define CASE_VARG_1_NAME(op, new, name, start, arg1, end) \
  case NAME_TO_ID (op): \
    __printf (new " "); \
    dump_variable (opcode.data.op.name); \
    __printf (start); \
    dump_variable (opcode.data.op.arg1); \
    __printf (end ";"); \
    break;     

#define CASE_VARG_2_NAME(op, new, name, start, arg1, arg2, end) \
  case NAME_TO_ID (op): \
    __printf (new " "); \
    dump_variable (opcode.data.op.name); \
    __printf (start); \
    dump_variable (opcode.data.op.arg1); \
    __printf (", "); \
    dump_variable (opcode.data.op.arg2); \
    __printf (end ";"); \
    break;     

#define CASE_VARG_N_NAME(op, new, name, start, arg1, arg2, end) \
  case NAME_TO_ID (op): \
    __printf (new " "); \
    dump_variable (opcode.data.op.name); \
    __printf (start); \
    dump_variable (opcode.data.op.arg1); \
    __printf (", "); \
    dump_variable (opcode.data.op.arg2); \
    __printf (" ..."); \
    varg_end = end; \
    break;     

#define CASE_VARG_0_LHS(op, lhs, equals, start, end) \
  case NAME_TO_ID (op): \
    dump_variable (opcode.data.op.lhs); \
    __printf (" " equals " " start); \
    __printf (end ";"); \
    break;     

#define CASE_VARG_1_LHS(op, lhs, equals, start, arg1, end) \
  case NAME_TO_ID (op): \
    dump_variable (opcode.data.op.lhs); \
    __printf (" " equals " " start); \
    dump_variable (opcode.data.op.arg1); \
    __printf (end ";"); \
    break;     

#define CASE_VARG_2_LHS(op, lhs, equals, start, arg1, arg2, end) \
  case NAME_TO_ID (op): \
    dump_variable (opcode.data.op.lhs); \
    __printf (" " equals " " start); \
    dump_variable (opcode.data.op.arg1); \
    __printf (", "); \
    dump_variable (opcode.data.op.arg2); \
    __printf (end ";"); \
    break;     

#define CASE_VARG_N_LHS(op, lhs, equals, start, arg1, arg2, end) \
  case NAME_TO_ID (op): \
    dump_variable (opcode.data.op.lhs); \
    __printf (" " equals " " start); \
    dump_variable (opcode.data.op.arg1); \
    __printf (", "); \
    dump_variable (opcode.data.op.arg2); \
    __printf (" ..."); \
    varg_end = end; \
    break;     

#define CASE_VARG_1_END(op, arg1) \
  case NAME_TO_ID (op): \
    __printf ("... "); \
    dump_variable (opcode.data.op.arg1); \
    __printf ("%s;", varg_end); \
    break;     

#define CASE_VARG_2_END(op, arg1, arg2) \
  case NAME_TO_ID (op): \
    __printf ("... "); \
    dump_variable (opcode.data.op.arg1); \
    __printf (", "); \
    dump_variable (opcode.data.op.arg2); \
    __printf ("%s;", varg_end); \
    break;     

#define CASE_VARG_3_END(op, arg1, arg2, arg3) \
  case NAME_TO_ID (op): \
    __printf ("... "); \
    dump_variable (opcode.data.op.arg1); \
    __printf (", "); \
    dump_variable (opcode.data.op.arg2); \
    __printf (", "); \
    dump_variable (opcode.data.op.arg3); \
    __printf ("%s;", varg_end); \
    break;     

#define CASE_VARG_3(op, arg1, arg2, arg3) \
  case NAME_TO_ID (op): \
    __printf ("... "); \
    dump_variable (opcode.data.op.arg1); \
    __printf (", "); \
    dump_variable (opcode.data.op.arg2); \
    __printf (", "); \
    dump_variable (opcode.data.op.arg3); \
    __printf (" ..."); \
    break;    

#define CASE_EXIT(op, name, field) \
  case NAME_TO_ID (op): \
    __printf (name " "); \
    __printf ("%d;", opcode.data.op.field); \
    break;     

#define CASE_SINGLE_ADDRESS(op, name, field) \
  case NAME_TO_ID (op): \
    __printf (name " "); \
    dump_variable (opcode.data.op.field); \
    __printf (";"); \
    break;     

#define CASE_ZERO_ADDRESS(op, name) \
  case NAME_TO_ID (op): \
    __printf (name ";"); \
    break;

#define CASE_THIS(op, lhs, equals, this) \
  case NAME_TO_ID (op): \
    dump_variable (opcode.data.op.lhs); \
    __printf (" " equals " "); \
    __printf (this ";"); \
    break;

#define CASE_WITH(op, expr) \
  case NAME_TO_ID (op): \
    __printf ("with ("); \
    dump_variable (opcode.data.op.expr); \
    __printf (")"); \
    break;

#define CASE_REG_VAR_DECL(op, min, max) \
  case NAME_TO_ID (op): \
    __printf ("var "); \
    dump_variable (opcode.data.op.min); \
    __printf (" .. "); \
    dump_variable (opcode.data.op.max); \
    __printf (";"); \
    break;

static char *varg_end;

void
pp_opcode (opcode_counter_t oc, OPCODE opcode, bool is_rewrite)
{
  uint8_t i = 1;
  uint8_t opcode_num = opcode.op_idx;

  __printf ("%03d: %20s ", oc, opcode_names[opcode_num]);
  if (opcode_num != NAME_TO_ID (nop) && opcode_num != NAME_TO_ID (ret) 
      && opcode_num != NAME_TO_ID (end_with))
    {
      for (i = 1; i < opcode_sizes[opcode_num]; i++)
        __printf ("%4d ", ((uint8_t*)&opcode)[i]);
    }

  for (; i < 4; i++)
    __printf ("     ");

  __printf ("    // ");

  switch (opcode_num)
  {
    CASE_CONDITIONAL_JUMP (is_true_jmp, "if (", value, ") goto", opcode)
    CASE_CONDITIONAL_JUMP (is_false_jmp, "if (", value, " == false) goto", opcode)

    CASE_UNCONDITIONAL_JUMP (jmp, "goto", 0, +, opcode_idx)
    CASE_UNCONDITIONAL_JUMP (jmp_up, "goto", oc, -, opcode_count)
    CASE_UNCONDITIONAL_JUMP (jmp_down, "goto", oc, +, opcode_count)

    CASE_TRIPLE_ADDRESS (addition, dst, "=", var_left, "+", var_right)
    CASE_TRIPLE_ADDRESS (substraction, dst, "=", var_left, "-", var_right)
    CASE_TRIPLE_ADDRESS (division, dst, "=", var_left, "/", var_right)
    CASE_TRIPLE_ADDRESS (multiplication, dst, "=", var_left, "*", var_right)
    CASE_TRIPLE_ADDRESS (remainder, dst, "=", var_left, "%%", var_right)
    CASE_TRIPLE_ADDRESS (b_shift_left, dst, "=", var_left, "<<", var_right)
    CASE_TRIPLE_ADDRESS (b_shift_right, dst, "=", var_left, ">>", var_right)
    CASE_TRIPLE_ADDRESS (b_shift_uright, dst, "=", var_left, ">>>", var_right)
    CASE_TRIPLE_ADDRESS (b_and, dst, "=", var_left, "&", var_right)
    CASE_TRIPLE_ADDRESS (b_or, dst, "=", var_left, "|", var_right)
    CASE_TRIPLE_ADDRESS (b_xor, dst, "=", var_left, "^", var_right)
    CASE_DOUBLE_ADDRESS (b_not, dst, "=", "~", var_right)
    CASE_TRIPLE_ADDRESS (logical_and, dst, "=", var_left, "&&", var_right)
    CASE_TRIPLE_ADDRESS (logical_or, dst, "=", var_left, "||", var_right)
    CASE_DOUBLE_ADDRESS (logical_not, dst, "=", "!", var_right)
    CASE_TRIPLE_ADDRESS (equal_value, dst, "=", var_left, "==", var_right)
    CASE_TRIPLE_ADDRESS (not_equal_value, dst, "=", var_left, "!=", var_right)
    CASE_TRIPLE_ADDRESS (equal_value_type, dst, "=", var_left, "===", var_right)
    CASE_TRIPLE_ADDRESS (not_equal_value_type, dst, "=", var_left, "!==", var_right)
    CASE_TRIPLE_ADDRESS (less_than, dst, "=", var_left, "<", var_right)
    CASE_TRIPLE_ADDRESS (greater_than, dst, "=", var_left, ">", var_right)
    CASE_TRIPLE_ADDRESS (less_or_equal_than, dst, "=", var_left, "<=", var_right)
    CASE_TRIPLE_ADDRESS (greater_or_equal_than, dst, "=", var_left, ">=", var_right)
    CASE_TRIPLE_ADDRESS (instanceof, dst, "=", var_left, "instanceof", var_right)
    CASE_TRIPLE_ADDRESS (in, dst, "=", var_left, "in", var_right)
    CASE_DOUBLE_ADDRESS_POST (post_incr, dst, "=", var_right, "++")
    CASE_DOUBLE_ADDRESS_POST (post_decr, dst, "=", var_right, "--")
    CASE_DOUBLE_ADDRESS (pre_incr, dst, "=", "++", var_right)
    CASE_DOUBLE_ADDRESS (pre_decr, dst, "=", "--", var_right)
    CASE_ASSIGNMENT (assignment, var_left, "=", value_right)
    CASE_VARG_0_NAME_LHS (call_0, lhs, "=", "", name_lit_idx, "(", ")")
    CASE_VARG_1_NAME_LHS (call_1, lhs, "=", "", name_lit_idx, "(", arg1_lit_idx, ")")
    CASE_VARG_N_NAME_LHS (call_n, lhs, "=", "", name_lit_idx, "(", arg1_lit_idx, ")")
    CASE_VARG_0_NAME_LHS (construct_0, lhs, "=", "new", name_lit_idx, "(", ")")
    CASE_VARG_1_NAME_LHS (construct_1, lhs, "=", "new", name_lit_idx, "(", arg1_lit_idx, ")")
    CASE_VARG_N_NAME_LHS (construct_n, lhs, "=", "new", name_lit_idx, "(", arg1_lit_idx, ")")
    CASE_VARG_0_NAME (func_decl_0, "function", name_lit_idx, "(", ")")
    CASE_VARG_1_NAME (func_decl_1, "function", name_lit_idx, "(", arg1_lit_idx, ")")
    CASE_VARG_2_NAME (func_decl_2, "function", name_lit_idx, "(", arg1_lit_idx, arg2_lit_idx, ")")
    CASE_VARG_N_NAME (func_decl_n, "function", name_lit_idx, "(", arg1_lit_idx, arg2_lit_idx, ")")
    CASE_VARG_0_NAME_LHS (func_expr_0, lhs, "=", "function", name_lit_idx, "(", ")")
    CASE_VARG_1_NAME_LHS (func_expr_1, lhs, "=", "function", name_lit_idx, "(", arg1_lit_idx, ")")
    CASE_VARG_N_NAME_LHS (func_expr_n, lhs, "=", "function", name_lit_idx, "(", arg1_lit_idx, ")")
    CASE_VARG_1_END (varg_1_end, arg1_lit_idx)
    CASE_VARG_2_END (varg_2_end, arg1_lit_idx, arg2_lit_idx)
    CASE_VARG_3_END (varg_3_end, arg1_lit_idx, arg2_lit_idx, arg3_lit_idx)
    CASE_VARG_3 (varg_3, arg1_lit_idx, arg2_lit_idx, arg3_lit_idx);
    CASE_EXIT (exitval, "exit", status_code)
    CASE_SINGLE_ADDRESS (retval, "return", ret_value)
    CASE_ZERO_ADDRESS (ret, "return")
    CASE_ZERO_ADDRESS (end_with, "")
    CASE_ZERO_ADDRESS (nop, "")
    CASE_VARG_0_LHS (array_0, lhs, "=", "[", "]")
    CASE_VARG_1_LHS (array_1, lhs, "=", "[", elem1, "]")
    CASE_VARG_2_LHS (array_2, lhs, "=", "[", elem1, elem2, "]")
    CASE_VARG_N_LHS (array_n, lhs, "=", "[", elem1, elem2, "]")
    CASE_VARG_0_LHS (obj_0, lhs, "=", "{", "}")
    CASE_VARG_1_LHS (obj_1, lhs, "=", "{", arg1, "}")
    CASE_VARG_2_LHS (obj_2, lhs, "=", "{", arg1, arg2, "}")
    CASE_VARG_N_LHS (obj_n, lhs, "=", "{", arg1, arg2, "}")
    CASE_TRIPLE_ADDRESS (prop, lhs, "=", name, ":", value)
    CASE_VARG_1_NAME_LHS (prop_access, lhs, "=", "", obj, "[", prop, "]")
    CASE_VARG_0_NAME_LHS (prop_get_decl, lhs, "=", "get", prop, "[", "]")
    CASE_VARG_1_NAME_LHS (prop_set_decl, lhs, "=", "set", prop, "[", arg, "]")
    CASE_THIS (this, lhs, "=", "this")
    CASE_DOUBLE_ADDRESS (delete, lhs, "=", "delete", obj)
    CASE_DOUBLE_ADDRESS (typeof, lhs, "=", "typeof", obj)
    CASE_WITH (with, expr)
    CASE_VARG_0_NAME (var_decl, "var", variable_name, "", "")
    CASE_REG_VAR_DECL (reg_var_decl, min, max)
  }

  if (is_rewrite)
    __printf (" // REWRITE");

  __printf ("\n");
}

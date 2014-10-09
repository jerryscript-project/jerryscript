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
#include "lexer.h"
#include "deserializer.h"
#include "opcodes-native-call.h"
#include <stdarg.h>

#define NAME_TO_ID(op) (__op__idx_##op)

#define FIELD(op, field) (opcode.data.op.field)

#define __OPCODE_STR(name, arg1, arg2, arg3) \
  #name,

#define __OPCODE_SIZE(name, arg1, arg2, arg3) \
  sizeof (__op_##name) + 1,

static char* opcode_names[] =
{
  OP_LIST (OPCODE_STR)
  ""
};

static uint8_t opcode_sizes[] =
{
  OP_LIST (OPCODE_SIZE)
  0
};

static void
dump_lp (lp_string lp)
{
  for (ecma_length_t i = 0; i < lp.length; i++)
  {
    __putchar (lp.str[i]);
  }
}

void
pp_strings (const lp_string strings[], uint8_t size)
{
  __printf ("STRINGS %d:\n", size);
  for (uint8_t i = 0; i < size; i++)
  {
    __printf ("%3d ", i);
    dump_lp (strings[i]);
    __putchar ('\n');
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

static const char *
var_id_to_string (char *res, idx_t id)
{
  if (id >= lexer_get_reserved_ids_count ())
  {
    __strncpy (res, "tmp", 3);
    if (id / 100 != 0)
    {
      res[3] = (char) (id / 100 + '0');
      res[4] = (char) ((id % 100) / 10 + '0');
      res[5] = (char) (id % 10 + '0');
      return res;
    }
    else if (id / 10 != 0)
    {
      res[3] = (char) (id / 10 + '0');
      res[4] = (char) (id % 10 + '0');
      return res;
    }
    else
    {
      res[3] = (char) (id + '0');
      return res;
    }
  }
  else if (id < lexer_get_strings_count ())
  {
    lp_string str = lexer_get_string_by_id (id);
    __strncpy (res, (char *) str.str, str.length);
    return res;
  }
  else
  {
    int i = 0;
    int num = (int) lexer_get_num_by_id (id);
    int temp = num;
    for (; temp != 0; i++)
    {
      temp /= 10;
    }
    do
    {
      res[i--] = (char) (num % 10 + '0');
      num /= 10;
    }
    while (i >= 0);
    return res;
  }
}

static void
pp_printf (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  while (*format)
  {
    if (*format != '%')
    {
      __putchar (*format);
      format++;
      continue;
    }

    format++;
    switch (*format)
    {
      case 'd':
      {
        __printf ("%d", va_arg (args, int));
        break;
      }
      case 's':
      {
        char res[32] = {'\0'};
        __printf ("%s", var_id_to_string (res, (idx_t) va_arg (args, int)));
        break;
      }
      case '%':
      {
        __printf ("%%");
        break;
      }
      default:
      {
        JERRY_UNREACHABLE ();
      }
    }
    format++;
  }
  va_end (args);
}

#define PP_OP_0(op, format) \
  case NAME_TO_ID(op): pp_printf (format); break;

#define PP_OP_1(op, format, field1) \
  case NAME_TO_ID(op): pp_printf (format, opcode.data.op.field1); break;

#define PP_OP_2(op, format, field1, field2) \
  case NAME_TO_ID(op): pp_printf (format, opcode.data.op.field1, opcode.data.op.field2); break;

#define PP_OP_3(op, format, field1, field2, field3) \
  case NAME_TO_ID(op): pp_printf (format, opcode.data.op.field1, opcode.data.op.field2, opcode.data.op.field3); break;

static int vargs_num = 0;
static int seen_vargs = 0;

void
pp_opcode (opcode_counter_t oc, opcode_t opcode, bool is_rewrite)
{
  uint8_t i = 1;
  uint8_t opcode_id = opcode.op_idx;

  __printf ("%3d: %20s ", oc, opcode_names[opcode_id]);
  if (opcode_id != NAME_TO_ID (nop) && opcode_id != NAME_TO_ID (ret))
  {
    for (i = 1; i < opcode_sizes[opcode_id]; i++)
    {
      __printf ("%4d ", ((uint8_t*) & opcode)[i]);
    }
  }

  for (; i < 4; i++)
  {
    __printf ("     ");
  }

  __printf ("    // ");

  switch (opcode_id)
  {
    PP_OP_3 (addition, "%s = %s + %s;", dst, var_left, var_right);
    PP_OP_3 (substraction, "%s = %s - %s;", dst, var_left, var_right);
    PP_OP_3 (division, "%s = %s - %s;", dst, var_left, var_right);
    PP_OP_3 (multiplication, "%s = %s * %s;", dst, var_left, var_right);
    PP_OP_3 (remainder, "%s = %s %% %s;", dst, var_left, var_right);
    PP_OP_3 (b_shift_left, "%s = %s << %s;", dst, var_left, var_right);
    PP_OP_3 (b_shift_right, "%s = %s >> %s;", dst, var_left, var_right);
    PP_OP_3 (b_shift_uright, "%s = %s >>> %s;", dst, var_left, var_right);
    PP_OP_3 (b_and, "%s = %s & %s;", dst, var_left, var_right);
    PP_OP_3 (b_or, "%s = %s | %s;", dst, var_left, var_right);
    PP_OP_3 (b_xor, "%s = %s ^ %s;", dst, var_left, var_right);
    PP_OP_2 (b_not, "%s = ~ %s;", dst, var_right);
    PP_OP_3 (logical_and, "%s = %s && %s;", dst, var_left, var_right);
    PP_OP_3 (logical_or, "%s = %s || %s;", dst, var_left, var_right);
    PP_OP_2 (logical_not, "%s = ! %s;", dst, var_right);
    PP_OP_3 (equal_value, "%s = %s == %s;", dst, var_left, var_right);
    PP_OP_3 (not_equal_value, "%s = %s != %s;", dst, var_left, var_right);
    PP_OP_3 (equal_value_type, "%s = %s === %s;", dst, var_left, var_right);
    PP_OP_3 (not_equal_value_type, "%s = %s !== %s;", dst, var_left, var_right);
    PP_OP_3 (less_than, "%s = %s < %s;", dst, var_left, var_right);
    PP_OP_3 (greater_than, "%s = %s > %s;", dst, var_left, var_right);
    PP_OP_3 (less_or_equal_than, "%s = %s <= %s;", dst, var_left, var_right);
    PP_OP_3 (greater_or_equal_than, "%s = %s >= %s;", dst, var_left, var_right);
    PP_OP_3 (instanceof, "%s = %s instanceof %s;", dst, var_left, var_right);
    PP_OP_3 (in, "%s = %s in %s;", dst, var_left, var_right);
    PP_OP_2 (post_incr, "%s = %s++;", dst, var_right);
    PP_OP_2 (post_decr, "%s = %s--;", dst, var_right);
    PP_OP_2 (pre_incr, "%s = ++%s;", dst, var_right);
    PP_OP_2 (pre_decr, "%s = --%s;", dst, var_right);
    PP_OP_1 (throw, "throw %s;", var);
    PP_OP_2 (reg_var_decl, "var %s .. %s;", min, max);
    PP_OP_1 (var_decl, "var %s;", variable_name);
    PP_OP_0 (nop, ";");
    PP_OP_1 (exitval, "exit %d;", status_code);
    PP_OP_1 (retval, "return %d;", ret_value);
    PP_OP_0 (ret, "ret;");
    PP_OP_3 (prop_getter, "%s = %s[\"%s\"];", lhs, obj, prop);
    PP_OP_3 (prop_setter, "%s[\"%s\"] = %s;", obj, prop, rhs);
    PP_OP_1 (this, "%s = this;", lhs);
    PP_OP_2 (delete_var, "%s = delete %s;", lhs, name);
    PP_OP_3 (delete_prop, "%s = delete %s[\"%s\"];", lhs, base, name);
    PP_OP_2 (typeof, "%s = typeof %s;", lhs, obj);
    PP_OP_1 (with, "with (%s);", expr);
    case NAME_TO_ID (is_true_jmp_up):
    {
      pp_printf ("if (%s) goto %d;", opcode.data.is_true_jmp_up.value,
                 oc - calc_opcode_counter_from_idx_idx (opcode.data.is_true_jmp_up.opcode_1,
                                                        opcode.data.is_true_jmp_up.opcode_2));
      break;
    }
    case NAME_TO_ID (is_false_jmp_up):
    {
      pp_printf ("if (%s) goto %d;", opcode.data.is_false_jmp_up.value,
                 oc - calc_opcode_counter_from_idx_idx (opcode.data.is_false_jmp_up.opcode_1,
                                                        opcode.data.is_false_jmp_up.opcode_2));
      break;
    }
    case NAME_TO_ID (is_true_jmp_down):
    {
      pp_printf ("if (%s) goto %d;", opcode.data.is_true_jmp_down.value,
                 oc + calc_opcode_counter_from_idx_idx (opcode.data.is_true_jmp_down.opcode_1,
                                                        opcode.data.is_true_jmp_down.opcode_2));
      break;
    }
    case NAME_TO_ID (is_false_jmp_down):
    {
      pp_printf ("if (%s) goto %d;", opcode.data.is_false_jmp_down.value,
                 oc + calc_opcode_counter_from_idx_idx (opcode.data.is_false_jmp_down.opcode_1,
                                                        opcode.data.is_false_jmp_down.opcode_2));
      break;
    }
    case NAME_TO_ID (jmp_up):
    {
      pp_printf ("goto %d;",
                 oc - calc_opcode_counter_from_idx_idx (opcode.data.jmp_up.opcode_1,
                                                        opcode.data.jmp_up.opcode_2));
      break;
    }
    case NAME_TO_ID (jmp_down):
    {
      pp_printf ("goto %d;",
                 oc + calc_opcode_counter_from_idx_idx (opcode.data.jmp_down.opcode_1,
                                                        opcode.data.jmp_down.opcode_2));
      break;
    }
    case NAME_TO_ID (try):
    {
      pp_printf ("try (end: %d);", calc_opcode_counter_from_idx_idx (opcode.data.try.oc_idx_1,
                                                                     opcode.data.try.oc_idx_2));
      break;
    }
    case NAME_TO_ID (assignment):
    {
      pp_printf ("%s = ", opcode.data.assignment.var_left);
      switch (opcode.data.assignment.type_value_right)
      {
        case OPCODE_ARG_TYPE_SIMPLE:
        {
          switch (opcode.data.assignment.value_right)
          {
            case ECMA_SIMPLE_VALUE_NULL: pp_printf ("null"); break;
            case ECMA_SIMPLE_VALUE_FALSE: pp_printf ("false"); break;
            case ECMA_SIMPLE_VALUE_TRUE: pp_printf ("true"); break;
            case ECMA_SIMPLE_VALUE_UNDEFINED: pp_printf ("undefined"); break;
            default: JERRY_UNREACHABLE ();
          }
          pp_printf (": SIMPLE;");
          break;
        }
        case OPCODE_ARG_TYPE_STRING:
        {
          pp_printf ("%s: STRING;", opcode.data.assignment.value_right);
          break;
        }
        case OPCODE_ARG_TYPE_NUMBER:
        {
          pp_printf ("%s: NUMBER;", opcode.data.assignment.value_right);
          break;
        }
        case OPCODE_ARG_TYPE_SMALLINT:
        {
          pp_printf ("%d: SMALLINT;", opcode.data.assignment.value_right);
          break;
        }
        case OPCODE_ARG_TYPE_VARIABLE:
        {
          pp_printf ("%s : TYPEOF(%s);", opcode.data.assignment.value_right, opcode.data.assignment.value_right);
          break;
        }
        default:
        {
          JERRY_UNREACHABLE ();
        }
      }
      break;
    }
    case NAME_TO_ID (call_n):
    {
      if (opcode.data.call_n.arg_list == 0)
      {
        pp_printf ("%s = %s ();", opcode.data.call_n.lhs, opcode.data.call_n.name_lit_idx);
      }
      else
      {
        vargs_num = opcode.data.call_n.arg_list;
        seen_vargs = 0;
      }
      break;
    }
    case NAME_TO_ID (native_call):
    {
      if (opcode.data.native_call.arg_list == 0)
      {
        pp_printf ("%s = ", opcode.data.native_call.lhs);
        switch (opcode.data.native_call.name)
        {
          case OPCODE_NATIVE_CALL_LED_TOGGLE: pp_printf ("LEDToggle ();"); break;
          case OPCODE_NATIVE_CALL_LED_ON: pp_printf ("LEDOn ();"); break;
          case OPCODE_NATIVE_CALL_LED_OFF: pp_printf ("LEDOff ();"); break;
          case OPCODE_NATIVE_CALL_LED_ONCE: pp_printf ("LEDOnce ();"); break;
          case OPCODE_NATIVE_CALL_WAIT: pp_printf ("wait ();"); break;
          case OPCODE_NATIVE_CALL_PRINT: pp_printf ("print ();"); break;
          default: JERRY_UNREACHABLE ();
        }
      }
      else
      {
        vargs_num = opcode.data.native_call.arg_list;
        seen_vargs = 0;
      }
      break;
    }
    case NAME_TO_ID (construct_n):
    {
      if (opcode.data.construct_n.arg_list == 0)
      {
        pp_printf ("%s = new %s;", opcode.data.construct_n.lhs, opcode.data.construct_n.name_lit_idx);
      }
      else
      {
        vargs_num = opcode.data.construct_n.arg_list;
        seen_vargs = 0;
      }
      break;
    }
    case NAME_TO_ID (func_decl_n):
    {
      if (opcode.data.func_decl_n.arg_list == 0)
      {
        pp_printf ("function %s ();", opcode.data.func_decl_n.name_lit_idx);
      }
      else
      {
        vargs_num = opcode.data.func_decl_n.arg_list;
        seen_vargs = 0;
      }
      break;
    }
    case NAME_TO_ID (func_expr_n):
    {
      if (opcode.data.func_expr_n.arg_list == 0)
      {
        pp_printf ("%s = function %s ();", opcode.data.func_expr_n.lhs, opcode.data.func_expr_n.name_lit_idx);
      }
      else
      {
        vargs_num = opcode.data.func_expr_n.arg_list;
        seen_vargs = 0;
      }
      break;
    }
    case NAME_TO_ID (array_decl):
    {
      if (opcode.data.array_decl.list == 0)
      {
        pp_printf ("%s = [];", opcode.data.array_decl.lhs);
      }
      else
      {
        vargs_num = opcode.data.array_decl.list;
        seen_vargs = 0;
      }
      break;
    }
    case NAME_TO_ID (obj_decl):
    {
      if (opcode.data.obj_decl.list == 0)
      {
        pp_printf ("%s = {};", opcode.data.obj_decl.lhs);
      }
      else
      {
        vargs_num = opcode.data.obj_decl.list;
        seen_vargs = 0;
      }
      break;
    }
    case NAME_TO_ID (meta):
    {
      switch (opcode.data.meta.type)
      {
        case OPCODE_META_TYPE_UNDEFINED:
        {
          pp_printf ("unknown meta;");
          break;
        }
        case OPCODE_META_TYPE_THIS_ARG:
        case OPCODE_META_TYPE_VARG:
        case OPCODE_META_TYPE_VARG_PROP_DATA:
        case OPCODE_META_TYPE_VARG_PROP_GETTER:
        case OPCODE_META_TYPE_VARG_PROP_SETTER:
        {
          seen_vargs++;
          if (seen_vargs == vargs_num)
          {
            bool found = false;
            opcode_counter_t start = oc;
            while ((int16_t) start >= 0 && !found)
            {
              start--;
              switch (deserialize_opcode (start).op_idx)
              {
                case NAME_TO_ID (call_n):
                case NAME_TO_ID (native_call):
                case NAME_TO_ID (construct_n):
                case NAME_TO_ID (func_decl_n):
                case NAME_TO_ID (func_expr_n):
                case NAME_TO_ID (array_decl):
                case NAME_TO_ID (obj_decl):
                {
                  found = true;
                  break;
                }
              }
            }
            opcode_t start_op = deserialize_opcode (start);
            switch (start_op.op_idx)
            {
              case NAME_TO_ID (call_n):
              {
                pp_printf ("%s = %s (", start_op.data.call_n.lhs, start_op.data.call_n.name_lit_idx);
                break;
              }
              case NAME_TO_ID (native_call):
              {
                pp_printf ("%s = ", start_op.data.native_call.lhs);
                switch (start_op.data.native_call.name)
                {
                  case OPCODE_NATIVE_CALL_LED_TOGGLE: pp_printf ("LEDToggle ("); break;
                  case OPCODE_NATIVE_CALL_LED_ON: pp_printf ("LEDOn ("); break;
                  case OPCODE_NATIVE_CALL_LED_OFF: pp_printf ("LEDOff ("); break;
                  case OPCODE_NATIVE_CALL_LED_ONCE: pp_printf ("LEDOnce ("); break;
                  case OPCODE_NATIVE_CALL_WAIT: pp_printf ("wait ("); break;
                  case OPCODE_NATIVE_CALL_PRINT: pp_printf ("print ("); break;
                  default: JERRY_UNREACHABLE ();
                }
                break;
              }
              case NAME_TO_ID (construct_n):
              {
                pp_printf ("%s = new %s (", start_op.data.construct_n.lhs, start_op.data.construct_n.name_lit_idx);
                break;
              }
              case NAME_TO_ID (func_decl_n):
              {
                pp_printf ("function %s (", start_op.data.func_decl_n.name_lit_idx);
                break;
              }
              case NAME_TO_ID (func_expr_n):
              {
                pp_printf ("%s = function %s (", start_op.data.func_expr_n.lhs,
                           start_op.data.func_expr_n.name_lit_idx);
                break;
              }
              case NAME_TO_ID (array_decl):
              {
                pp_printf ("%s = [", start_op.data.array_decl.lhs);
                break;
              }
              case NAME_TO_ID (obj_decl):
              {
                pp_printf ("%s = {", start_op.data.obj_decl.lhs);
                break;
              }
              default:
              {
                JERRY_UNREACHABLE ();
              }
            }
            for (opcode_counter_t counter = start; counter <= oc; counter++)
            {
              opcode_t meta_op = deserialize_opcode (counter);
              switch (meta_op.op_idx)
              {
                case NAME_TO_ID (meta):
                {
                  switch (opcode.data.meta.type)
                  {
                    case OPCODE_META_TYPE_THIS_ARG:
                    {
                      pp_printf ("this_arg = %s", meta_op.data.meta.data_1);
                      break;
                    }
                    case OPCODE_META_TYPE_VARG:
                    {
                      pp_printf ("%s", meta_op.data.meta.data_1);
                      break;
                    }
                    case OPCODE_META_TYPE_VARG_PROP_DATA:
                    {
                      pp_printf ("%s:%s", meta_op.data.meta.data_1, meta_op.data.meta.data_2);
                      break;
                    }
                    case OPCODE_META_TYPE_VARG_PROP_GETTER:
                    {
                      pp_printf ("%s = get ();", meta_op.data.meta.data_1);
                      break;
                    }
                    case OPCODE_META_TYPE_VARG_PROP_SETTER:
                    {
                      pp_printf ("%s = set (%s);", meta_op.data.meta.data_1, meta_op.data.meta.data_2);
                      break;
                    }
                    default:
                    {
                      JERRY_UNREACHABLE ();
                    }
                  }
                  if (counter != oc)
                  {
                    pp_printf (", ");
                  }
                  break;
                }
              }
            }
            pp_printf (");");
          }
          break;
        }
        case OPCODE_META_TYPE_END_WITH:
        {
          pp_printf ("end with;");
          break;
        }
        case OPCODE_META_TYPE_FUNCTION_END:
        {
          pp_printf ("function end: %d;", calc_opcode_counter_from_idx_idx (opcode.data.meta.data_1,
                                                                            opcode.data.meta.data_2));
          break;
        }
        case OPCODE_META_TYPE_CATCH:
        {
          pp_printf ("catch end: %d;", calc_opcode_counter_from_idx_idx (opcode.data.meta.data_1,
                                                                         opcode.data.meta.data_2));
          break;
        }
        case OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER:
        {
          pp_printf ("catch (%s);", opcode.data.meta.data_1);
          break;
        }
        case OPCODE_META_TYPE_FINALLY:
        {
          pp_printf ("finally end: %d;", calc_opcode_counter_from_idx_idx (opcode.data.meta.data_1,
                                                                           opcode.data.meta.data_2));
          break;
        }
        case OPCODE_META_TYPE_END_TRY_CATCH_FINALLY:
        {
          pp_printf ("end try");
          break;
        }
        case OPCODE_META_TYPE_STRICT_CODE:
        {
          pp_printf ("use strict;");
          break;
        }
        default:
        {
          JERRY_UNREACHABLE ();
        }
      }
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  if (is_rewrite)
  {
    __printf (" // REWRITE");
  }

  __printf ("\n");
}

/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#ifdef JERRY_ENABLE_PRETTY_PRINTER
#include <stdarg.h>

#include "pretty-printer.h"
#include "jrt-libc-includes.h"
#include "lexer.h"
#include "opcodes-native-call.h"
#include "ecma-helpers.h"
#include "ecma-globals.h"
#include "serializer.h"
#include "lit-literal.h"

#define NAME_TO_ID(op) (__op__idx_##op)

#define __OPCODE_STR(name, arg1, arg2, arg3) \
  #name,

#define __OPCODE_SIZE(name, arg1, arg2, arg3) \
  (uint8_t) (sizeof (__op_##name) + 1),

static const char* opcode_names[] =
{
  OP_LIST (OPCODE_STR)
  ""
};

static uint8_t opcode_sizes[] =
{
  OP_LIST (OPCODE_SIZE)
  0
};

static char buff[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];

static void
clear_temp_buffer (void)
{
  memset (buff, 0, ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER);
}

static const char *
lit_cp_to_str (lit_cpointer_t cp)
{
  literal_t lit = lit_get_literal_by_cp (cp);
  return lit_literal_to_str_internal_buf (lit);
}

static const char *
tmp_id_to_str (idx_t id)
{
  JERRY_ASSERT (id != LITERAL_TO_REWRITE);
  JERRY_ASSERT (id >= 128);
  clear_temp_buffer ();
  strncpy (buff, "tmp", 3);
  if (id / 100 != 0)
  {
    buff[3] = (char) (id / 100 + '0');
    buff[4] = (char) ((id % 100) / 10 + '0');
    buff[5] = (char) (id % 10 + '0');
  }
  else if (id / 10 != 0)
  {
    buff[3] = (char) (id / 10 + '0');
    buff[4] = (char) (id % 10 + '0');
  }
  else
  {
    buff[3] = (char) (id + '0');
  }
  return buff;
}

static const char *
var_to_str (opcode_t opcode, lit_cpointer_t lit_ids[], opcode_counter_t oc, uint8_t current_arg)
{
  raw_opcode raw = *(raw_opcode*) &opcode;
  if (raw.uids[current_arg] == LITERAL_TO_REWRITE)
  {
    if (lit_ids == NULL)
    {
      return "hz";
    }
    JERRY_ASSERT (lit_ids[current_arg - 1].packed_value != MEM_CP_NULL);
    return lit_cp_to_str (lit_ids[current_arg - 1]);
  }
  else if (raw.uids[current_arg] >= 128)
  {
    return tmp_id_to_str (raw.uids[current_arg]);
  }
  else
  {
    return lit_cp_to_str (serializer_get_literal_cp_by_uid (raw.uids[current_arg], NULL, oc));
  }
}

static void
pp_printf (const char *format, opcode_t opcode, lit_cpointer_t lit_ids[], opcode_counter_t oc, uint8_t start_arg)
{
  uint8_t current_arg = start_arg;
  JERRY_ASSERT (current_arg <= 3);
  while (*format)
  {
    if (*format != '%')
    {
      putchar (*format);
      format++;
      continue;
    }

    format++;
    switch (*format)
    {
      case 'd':
      {
        JERRY_ASSERT (current_arg <= 3);
        raw_opcode raw = *(raw_opcode*) &opcode;
        printf ("%d", raw.uids[current_arg]);
        break;
      }
      case 's':
      {
        JERRY_ASSERT (current_arg <= 3);
        printf ("%s", var_to_str (opcode, lit_ids, oc, current_arg));
        break;
      }
      default:
      {
        putchar ('%');
        continue;
      }
    }
    current_arg++;
    format++;
  }
}

#define PP_OP(op_name, format) \
  case NAME_TO_ID (op_name): pp_printf (format, opm.op, opm.lit_id, oc, 1); break;
#define VAR(i) var_to_str (opm.op, opm.lit_id, oc, i)
#define OC(i, j) __extension__({ raw_opcode* raw = (raw_opcode *) &opm.op; \
                                 calc_opcode_counter_from_idx_idx (raw->uids[i], raw->uids[j]); })

static int vargs_num = 0;
static int seen_vargs = 0;

static void
dump_asm (opcode_counter_t oc, opcode_t opcode)
{
  uint8_t i = 0;
  uint8_t opcode_id = opcode.op_idx;
  printf ("%3d: %20s ", oc, opcode_names[opcode_id]);
  if (opcode_id != NAME_TO_ID (nop) && opcode_id != NAME_TO_ID (ret))
  {
    for (i = 1; i < opcode_sizes[opcode_id]; i++)
    {
      printf ("%4d ", ((raw_opcode *) &opcode)->uids[i]);
    }
  }

  for (; i < 4; i++)
  {
    printf ("     ");
  }
}

void
pp_op_meta (const opcode_t *opcodes_p,
            opcode_counter_t oc,
            op_meta opm,
            bool rewrite)
{
  dump_asm (oc, opm.op);
  printf ("    // ");

  switch (opm.op.op_idx)
  {
    PP_OP (addition, "%s = %s + %s;");
    PP_OP (substraction, "%s = %s - %s;");
    PP_OP (division, "%s = %s / %s;");
    PP_OP (multiplication, "%s = %s * %s;");
    PP_OP (remainder, "%s = %s %% %s;");
    PP_OP (unary_minus, "%s = -%s;");
    PP_OP (unary_plus, "%s = +%s;");
    PP_OP (b_shift_left, "%s = %s << %s;");
    PP_OP (b_shift_right, "%s = %s >> %s;");
    PP_OP (b_shift_uright, "%s = %s >>> %s;");
    PP_OP (b_and, "%s = %s & %s;");
    PP_OP (b_or, "%s = %s | %s;");
    PP_OP (b_xor, "%s = %s ^ %s;");
    PP_OP (b_not, "%s = ~ %s;");
    PP_OP (logical_not, "%s = ! %s;");
    PP_OP (equal_value, "%s = %s == %s;");
    PP_OP (not_equal_value, "%s = %s != %s;");
    PP_OP (equal_value_type, "%s = %s === %s;");
    PP_OP (not_equal_value_type, "%s = %s !== %s;");
    PP_OP (less_than, "%s = %s < %s;");
    PP_OP (greater_than, "%s = %s > %s;");
    PP_OP (less_or_equal_than, "%s = %s <= %s;");
    PP_OP (greater_or_equal_than, "%s = %s >= %s;");
    PP_OP (instanceof, "%s = %s instanceof %s;");
    PP_OP (in, "%s = %s in %s;");
    PP_OP (post_incr, "%s = %s++;");
    PP_OP (post_decr, "%s = %s--;");
    PP_OP (pre_incr, "%s = ++%s;");
    PP_OP (pre_decr, "%s = --%s;");
    PP_OP (throw_value, "throw %s;");
    PP_OP (reg_var_decl, "var %s .. %s;");
    PP_OP (var_decl, "var %s;");
    PP_OP (nop, ";");
    PP_OP (retval, "return %s;");
    PP_OP (ret, "ret;");
    PP_OP (prop_getter, "%s = %s[%s];");
    PP_OP (prop_setter, "%s[%s] = %s;");
    PP_OP (this_binding, "%s = this;");
    PP_OP (delete_var, "%s = delete %s;");
    PP_OP (delete_prop, "%s = delete %s.%s;");
    PP_OP (typeof, "%s = typeof %s;");
    PP_OP (with, "with (%s);");
    PP_OP (for_in, "for_in (%s);");
    case NAME_TO_ID (is_true_jmp_up): printf ("if (%s) goto %d;", VAR (1), oc - OC (2, 3)); break;
    case NAME_TO_ID (is_false_jmp_up): printf ("if (%s == false) goto %d;", VAR (1), oc - OC (2, 3)); break;
    case NAME_TO_ID (is_true_jmp_down): printf ("if (%s) goto %d;", VAR (1), oc + OC (2, 3)); break;
    case NAME_TO_ID (is_false_jmp_down): printf ("if (%s == false) goto %d;", VAR (1), oc + OC (2, 3)); break;
    case NAME_TO_ID (jmp_up): printf ("goto %d;", oc - OC (1, 2)); break;
    case NAME_TO_ID (jmp_down): printf ("goto %d;", oc + OC (1, 2)); break;
    case NAME_TO_ID (jmp_break_continue): printf ("goto_nested %d;", oc + OC (1, 2)); break;
    case NAME_TO_ID (try_block): printf ("try (end: %d);", oc + OC (1, 2)); break;
    case NAME_TO_ID (assignment):
    {
      printf ("%s = ", VAR (1));
      switch (opm.op.data.assignment.type_value_right)
      {
        case OPCODE_ARG_TYPE_STRING: printf ("'%s': STRING;", VAR (3)); break;
        case OPCODE_ARG_TYPE_NUMBER: printf ("%s: NUMBER;", VAR (3)); break;
        case OPCODE_ARG_TYPE_NUMBER_NEGATE: printf ("-%s: NUMBER;", VAR (3)); break;
        case OPCODE_ARG_TYPE_SMALLINT: printf ("%d: SMALLINT;", opm.op.data.assignment.value_right); break;
        case OPCODE_ARG_TYPE_SMALLINT_NEGATE: printf ("-%d: SMALLINT;", opm.op.data.assignment.value_right); break;
        case OPCODE_ARG_TYPE_VARIABLE: printf ("%s : TYPEOF(%s);", VAR (3), VAR (3)); break;
        case OPCODE_ARG_TYPE_SIMPLE:
        {
          switch (opm.op.data.assignment.value_right)
          {
            case ECMA_SIMPLE_VALUE_NULL: printf ("null"); break;
            case ECMA_SIMPLE_VALUE_FALSE: printf ("false"); break;
            case ECMA_SIMPLE_VALUE_TRUE: printf ("true"); break;
            case ECMA_SIMPLE_VALUE_UNDEFINED: printf ("undefined"); break;
            case ECMA_SIMPLE_VALUE_UNUSED: printf ("<unused>"); break;
            default: JERRY_UNREACHABLE ();
          }
          printf (": SIMPLE;");
          break;
        }
      }
      break;
    }
    case NAME_TO_ID (call_n):
    {
      vargs_num = opm.op.data.call_n.arg_list;
      seen_vargs = 0;

      break;
    }
    case NAME_TO_ID (native_call):
    {
      if (opm.op.data.native_call.arg_list == 0)
      {
        printf ("%s = ", VAR (1));
        switch (opm.op.data.native_call.name)
        {
          case OPCODE_NATIVE_CALL_LED_TOGGLE: printf ("LEDToggle ();"); break;
          case OPCODE_NATIVE_CALL_LED_ON: printf ("LEDOn ();"); break;
          case OPCODE_NATIVE_CALL_LED_OFF: printf ("LEDOff ();"); break;
          case OPCODE_NATIVE_CALL_LED_ONCE: printf ("LEDOnce ();"); break;
          case OPCODE_NATIVE_CALL_WAIT: printf ("wait ();"); break;
          case OPCODE_NATIVE_CALL_PRINT: printf ("print ();"); break;
          default: JERRY_UNREACHABLE ();
        }
      }
      else
      {
        vargs_num = opm.op.data.native_call.arg_list;
        seen_vargs = 0;
      }
      break;
    }
    case NAME_TO_ID (construct_n):
    {
      if (opm.op.data.construct_n.arg_list == 0)
      {
        pp_printf ("%s = new %s;", opm.op, opm.lit_id, oc, 1);
      }
      else
      {
        vargs_num = opm.op.data.construct_n.arg_list;
        seen_vargs = 0;
      }
      break;
    }
    case NAME_TO_ID (func_decl_n):
    {
      if (opm.op.data.func_decl_n.arg_list == 0)
      {
        printf ("function %s ();", VAR (1));
      }
      else
      {
        vargs_num = opm.op.data.func_decl_n.arg_list;
        seen_vargs = 0;
      }
      break;
    }
    case NAME_TO_ID (func_expr_n):
    {
      if (opm.op.data.func_expr_n.arg_list == 0)
      {
        if (opm.op.data.func_expr_n.name_lit_idx == INVALID_VALUE)
        {
          printf ("%s = function ();", VAR (1));
        }
        else
        {
          pp_printf ("%s = function %s ();", opm.op, opm.lit_id, oc, 1);
        }
      }
      else
      {
        vargs_num = opm.op.data.func_expr_n.arg_list;
        seen_vargs = 0;
      }
      break;
    }
    case NAME_TO_ID (array_decl):
    {
      if (opm.op.data.array_decl.list == 0)
      {
        printf ("%s = [];", VAR (1));
      }
      else
      {
        vargs_num = opm.op.data.array_decl.list;
        seen_vargs = 0;
      }
      break;
    }
    case NAME_TO_ID (obj_decl):
    {
      if (opm.op.data.obj_decl.list == 0)
      {
        printf ("%s = {};", VAR (1));
      }
      else
      {
        vargs_num = opm.op.data.obj_decl.list;
        seen_vargs = 0;
      }
      break;
    }
    case NAME_TO_ID (meta):
    {
      switch (opm.op.data.meta.type)
      {
        case OPCODE_META_TYPE_UNDEFINED:
        {
          printf ("unknown meta;");
          break;
        }
        case OPCODE_META_TYPE_CALL_SITE_INFO:
        case OPCODE_META_TYPE_VARG:
        case OPCODE_META_TYPE_VARG_PROP_DATA:
        case OPCODE_META_TYPE_VARG_PROP_GETTER:
        case OPCODE_META_TYPE_VARG_PROP_SETTER:
        {
          if (opm.op.data.meta.type != OPCODE_META_TYPE_CALL_SITE_INFO)
          {
            seen_vargs++;
          }

          if (seen_vargs == vargs_num)
          {
            bool found = false;
            opcode_counter_t start = oc;
            while ((int16_t) start >= 0 && !found)
            {
              start--;
              switch (serializer_get_opcode (opcodes_p, start).op_idx)
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
            opcode_t start_op = serializer_get_opcode (opcodes_p, start);
            switch (start_op.op_idx)
            {
              case NAME_TO_ID (call_n):
              {
                pp_printf ("%s = %s (", start_op, NULL, start, 1);
                break;
              }
              case NAME_TO_ID (native_call):
              {
                pp_printf ("%s = ", start_op, NULL, start, 1);
                switch (start_op.data.native_call.name)
                {
                  case OPCODE_NATIVE_CALL_LED_TOGGLE: printf ("LEDToggle ("); break;
                  case OPCODE_NATIVE_CALL_LED_ON: printf ("LEDOn ("); break;
                  case OPCODE_NATIVE_CALL_LED_OFF: printf ("LEDOff ("); break;
                  case OPCODE_NATIVE_CALL_LED_ONCE: printf ("LEDOnce ("); break;
                  case OPCODE_NATIVE_CALL_WAIT: printf ("wait ("); break;
                  case OPCODE_NATIVE_CALL_PRINT: printf ("print ("); break;
                  default: JERRY_UNREACHABLE ();
                }
                break;
              }
              case NAME_TO_ID (construct_n):
              {
                pp_printf ("%s = new %s (", start_op, NULL, start, 1);
                break;
              }
              case NAME_TO_ID (func_decl_n):
              {
                pp_printf ("function %s (", start_op, NULL, start, 1);
                break;
              }
              case NAME_TO_ID (func_expr_n):
              {
                if (start_op.data.func_expr_n.name_lit_idx == INVALID_VALUE)
                {
                  pp_printf ("%s = function (", start_op, NULL, start, 1);
                }
                else
                {
                  pp_printf ("%s = function %s (", start_op, NULL, start, 1);
                }
                break;
              }
              case NAME_TO_ID (array_decl):
              {
                pp_printf ("%s = [", start_op, NULL, start, 1);
                break;
              }
              case NAME_TO_ID (obj_decl):
              {
                pp_printf ("%s = {", start_op, NULL, start, 1);
                break;
              }
              default:
              {
                JERRY_UNREACHABLE ();
              }
            }
            for (opcode_counter_t counter = start; counter <= oc; counter++)
            {
              opcode_t meta_op = serializer_get_opcode (opcodes_p, counter);

              switch (meta_op.op_idx)
              {
                case NAME_TO_ID (meta):
                {
                  switch (meta_op.data.meta.type)
                  {
                    case OPCODE_META_TYPE_CALL_SITE_INFO:
                    {
                      opcode_call_flags_t call_flags = (opcode_call_flags_t) meta_op.data.meta.data_1;

                      if (call_flags & OPCODE_CALL_FLAGS_HAVE_THIS_ARG)
                      {
                        pp_printf ("this_arg = %s", meta_op, NULL, counter, 3);
                      }
                      if (call_flags & OPCODE_CALL_FLAGS_DIRECT_CALL_TO_EVAL_FORM)
                      {
                        printf ("['direct call to eval' form]");
                      }

                      break;
                    }
                    case OPCODE_META_TYPE_VARG:
                    {
                      pp_printf ("%s", meta_op, NULL, counter, 2);
                      break;
                    }
                    case OPCODE_META_TYPE_VARG_PROP_DATA:
                    {
                      pp_printf ("%s:%s", meta_op, NULL, counter, 2);
                      break;
                    }
                    case OPCODE_META_TYPE_VARG_PROP_GETTER:
                    {
                      pp_printf ("%s = get %s ();", meta_op, NULL, counter, 2);
                      break;
                    }
                    case OPCODE_META_TYPE_VARG_PROP_SETTER:
                    {
                      pp_printf ("%s = set (%s);", meta_op, NULL, counter, 2);
                      break;
                    }
                    default:
                    {
                      continue;
                    }
                  }
                  if (counter != oc)
                  {
                    printf (", ");
                  }
                  break;
                }
              }
            }
            switch (start_op.op_idx)
            {
              case NAME_TO_ID (array_decl):
              {
                printf ("];");
                break;
              }
              case NAME_TO_ID (obj_decl):
              {
                printf ("};");
                break;
              }
              default:
              {
                printf (");");
              }
            }
          }
          break;
        }
        case OPCODE_META_TYPE_END_WITH:
        {
          printf ("end with;");
          break;
        }
        case OPCODE_META_TYPE_END_FOR_IN:
        {
          printf ("end for-in;");
          break;
        }
        case OPCODE_META_TYPE_FUNCTION_END:
        {
          printf ("function end: %d;", oc + OC (2, 3));
          break;
        }
        case OPCODE_META_TYPE_CATCH:
        {
          printf ("catch end: %d;", oc + OC (2, 3));
          break;
        }
        case OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER:
        {
          printf ("catch (%s);", VAR (2));
          break;
        }
        case OPCODE_META_TYPE_FINALLY:
        {
          printf ("finally end: %d;", oc + OC (2, 3));
          break;
        }
        case OPCODE_META_TYPE_END_TRY_CATCH_FINALLY:
        {
          printf ("end try");
          break;
        }
        case OPCODE_META_TYPE_SCOPE_CODE_FLAGS:
        {
          if (opm.op.data.meta.data_1 != INVALID_VALUE)
          {
            idx_t scope_flags = opm.op.data.meta.data_1;

            if (scope_flags & OPCODE_SCOPE_CODE_FLAGS_STRICT)
            {
              printf ("[use strict] ");
              scope_flags &= (idx_t) ~(OPCODE_SCOPE_CODE_FLAGS_STRICT);
            }
            if (scope_flags & OPCODE_SCOPE_CODE_FLAGS_NOT_REF_ARGUMENTS_IDENTIFIER)
            {
              printf ("[no 'arguments'] ");
              scope_flags &= (idx_t) ~(OPCODE_SCOPE_CODE_FLAGS_NOT_REF_ARGUMENTS_IDENTIFIER);
            }
            if (scope_flags & OPCODE_SCOPE_CODE_FLAGS_NOT_REF_EVAL_IDENTIFIER)
            {
              printf ("[no 'eval'] ");
              scope_flags &= (idx_t) ~(OPCODE_SCOPE_CODE_FLAGS_NOT_REF_EVAL_IDENTIFIER);
            }

            JERRY_ASSERT (scope_flags == 0);
          }
          else
          {
            printf ("[to be rewritten]");
          }

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

  if (rewrite)
  {
    printf (" // REWRITE");
  }

  printf ("\n");
}
#endif /* JERRY_ENABLE_PRETTY_PRINTER */

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

#include "opcodes-dumper.h"

#include "serializer.h"
#include "stack.h"
#include "syntax-errors.h"
#include "opcodes-native-call.h"

static idx_t temp_name, max_temp_name;

#define OPCODE(name) (__op__idx_##name)

enum
{
  U8_global_size
};
STATIC_STACK (U8, uint8_t)

enum
{
  varg_headers_global_size
};
STATIC_STACK (varg_headers, opcode_counter_t)

enum
{
  function_ends_global_size
};
STATIC_STACK (function_ends, opcode_counter_t)

enum
{
  logical_and_checks_global_size
};
STATIC_STACK (logical_and_checks, opcode_counter_t)

enum
{
  logical_or_checks_global_size
};
STATIC_STACK (logical_or_checks, opcode_counter_t)

enum
{
  conditional_checks_global_size
};
STATIC_STACK (conditional_checks, opcode_counter_t)

enum
{
  jumps_to_end_global_size
};
STATIC_STACK (jumps_to_end, opcode_counter_t)

enum
{
  prop_getters_global_size
};
STATIC_STACK (prop_getters, op_meta)

enum
{
  next_iterations_global_size
};
STATIC_STACK (next_iterations, opcode_counter_t)

enum
{
  case_clauses_global_size
};
STATIC_STACK (case_clauses, opcode_counter_t)

enum
{
  tries_global_size
};
STATIC_STACK (tries, opcode_counter_t)

enum
{
  catches_global_size
};
STATIC_STACK (catches, opcode_counter_t)

enum
{
  finallies_global_size
};
STATIC_STACK (finallies, opcode_counter_t)

enum
{
  temp_names_global_size
};
STATIC_STACK (temp_names, idx_t)

enum
{
  reg_var_decls_global_size
};
STATIC_STACK (reg_var_decls, opcode_counter_t)

/**
 * Reset counter of register variables allocator
 * to identifier of first general register
 */
static void
reset_temp_name (void)
{
  temp_name = OPCODE_REG_GENERAL_FIRST;
} /* reset_temp_name */

/**
 * Allocate next register variable
 *
 * @return identifier of the allocated variable
 */
static idx_t
next_temp_name (void)
{
  idx_t next_reg = temp_name++;

  if (next_reg > OPCODE_REG_GENERAL_LAST)
  {
    /*
     * FIXME:
     *       Implement mechanism, allowing reusage of register variables
     */
    PARSE_ERROR ("Not enough register variables", LIT_ITERATOR_POS_ZERO);
  }

  if (max_temp_name < next_reg)
  {
    max_temp_name = next_reg;
  }

  return next_reg;
} /* next_temp_name */

static op_meta
create_op_meta (opcode_t op, lit_cpointer_t lit_id1, lit_cpointer_t lit_id2, lit_cpointer_t lit_id3)
{
  op_meta ret;

  ret.op = op;
  ret.lit_id[0] = lit_id1;
  ret.lit_id[1] = lit_id2;
  ret.lit_id[2] = lit_id3;

  return ret;
}

static op_meta
create_op_meta_000 (opcode_t op)
{
  return create_op_meta (op, NOT_A_LITERAL, NOT_A_LITERAL, NOT_A_LITERAL);
}

static op_meta
create_op_meta_001 (opcode_t op, lit_cpointer_t lit_id)
{
  return create_op_meta (op, NOT_A_LITERAL, NOT_A_LITERAL, lit_id);
}

static op_meta
create_op_meta_010 (opcode_t op, lit_cpointer_t lit_id)
{
  return create_op_meta (op, NOT_A_LITERAL, lit_id, NOT_A_LITERAL);
}

static op_meta
create_op_meta_011 (opcode_t op, lit_cpointer_t lit_id2, lit_cpointer_t lit_id3)
{
  return create_op_meta (op, NOT_A_LITERAL, lit_id2, lit_id3);
}

static op_meta
create_op_meta_100 (opcode_t op, lit_cpointer_t lit_id)
{
  return create_op_meta (op, lit_id, NOT_A_LITERAL, NOT_A_LITERAL);
}

static op_meta
create_op_meta_101 (opcode_t op, lit_cpointer_t lit_id1, lit_cpointer_t lit_id3)
{
  return create_op_meta (op, lit_id1, NOT_A_LITERAL, lit_id3);
}

static op_meta
create_op_meta_110 (opcode_t op, lit_cpointer_t lit_id1, lit_cpointer_t lit_id2)
{
  return create_op_meta (op, lit_id1, lit_id2, NOT_A_LITERAL);
}

static op_meta
create_op_meta_111 (opcode_t op, lit_cpointer_t lit_id1, lit_cpointer_t lit_id2, lit_cpointer_t lit_id3)
{
  return create_op_meta (op, lit_id1, lit_id2, lit_id3);
}

static operand
tmp_operand (void)
{
  operand ret;

  ret.type = OPERAND_TMP;
  ret.data.uid = next_temp_name ();

  return ret;
}

static uint8_t
name_to_native_call_id (operand obj)
{
  if (obj.type != OPERAND_LITERAL)
  {
    return OPCODE_NATIVE_CALL__COUNT;
  }
  if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (obj.data.lit_id), "LEDToggle"))
  {
    return OPCODE_NATIVE_CALL_LED_TOGGLE;
  }
  else if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (obj.data.lit_id), "LEDOn"))
  {
    return OPCODE_NATIVE_CALL_LED_ON;
  }
  else if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (obj.data.lit_id), "LEDOff"))
  {
    return OPCODE_NATIVE_CALL_LED_OFF;
  }
  else if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (obj.data.lit_id), "LEDOnce"))
  {
    return OPCODE_NATIVE_CALL_LED_ONCE;
  }
  else if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (obj.data.lit_id), "wait"))
  {
    return OPCODE_NATIVE_CALL_WAIT;
  }
  else if (lit_literal_equal_type_cstr (lit_get_literal_by_cp (obj.data.lit_id), "print"))
  {
    return OPCODE_NATIVE_CALL_PRINT;
  }
  return OPCODE_NATIVE_CALL__COUNT;
}

static bool
is_native_call (operand obj)
{
  return name_to_native_call_id (obj) < OPCODE_NATIVE_CALL__COUNT;
}

static op_meta
create_op_meta_for_res_and_obj (opcode_t (*getop) (idx_t, idx_t, idx_t), operand *res, operand *obj)
{
  JERRY_ASSERT (obj != NULL);
  JERRY_ASSERT (res != NULL);
  op_meta ret;
  switch (obj->type)
  {
    case OPERAND_TMP:
    {
      switch (res->type)
      {
        case OPERAND_TMP:
        {
          const opcode_t opcode = getop (res->data.uid, obj->data.uid, INVALID_VALUE);
          ret = create_op_meta_000 (opcode);
          break;
        }
        case OPERAND_LITERAL:
        {
          const opcode_t opcode = getop (LITERAL_TO_REWRITE, obj->data.uid, INVALID_VALUE);
          ret = create_op_meta_100 (opcode, res->data.lit_id);
          break;
        }
      }
      break;
    }
    case OPERAND_LITERAL:
    {
      switch (res->type)
      {
        case OPERAND_TMP:
        {
          const opcode_t opcode = getop (res->data.uid, LITERAL_TO_REWRITE, INVALID_VALUE);
          ret = create_op_meta_010 (opcode, obj->data.lit_id);
          break;
        }
        case OPERAND_LITERAL:
        {
          const opcode_t opcode = getop (LITERAL_TO_REWRITE, LITERAL_TO_REWRITE, INVALID_VALUE);
          ret = create_op_meta_110 (opcode, res->data.lit_id, obj->data.lit_id);
          break;
        }
      }
      break;
    }
  }
  return ret;
}

static op_meta
create_op_meta_for_obj (opcode_t (*getop) (idx_t, idx_t), operand *obj)
{
  JERRY_ASSERT (obj != NULL);
  op_meta res;
  switch (obj->type)
  {
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop (obj->data.uid, INVALID_VALUE);
      res = create_op_meta_000 (opcode);
      break;
    }
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop (LITERAL_TO_REWRITE, INVALID_VALUE);
      res = create_op_meta_100 (opcode, obj->data.lit_id);
      break;
    }
  }
  return res;
}

static op_meta
create_op_meta_for_native_call (operand res, operand obj)
{
  JERRY_ASSERT (is_native_call (obj));
  op_meta ret;
  switch (res.type)
  {
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop_native_call (res.data.uid, name_to_native_call_id (obj), INVALID_VALUE);
      ret = create_op_meta_000 (opcode);
      break;
    }
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop_native_call (LITERAL_TO_REWRITE, name_to_native_call_id (obj), INVALID_VALUE);
      ret = create_op_meta_100 (opcode, res.data.lit_id);
      break;
    }
  }
  return ret;
}

static op_meta
create_op_meta_for_vlt (varg_list_type vlt, operand *res, operand *obj)
{
  op_meta ret;
  switch (vlt)
  {
    case VARG_FUNC_EXPR: ret = create_op_meta_for_res_and_obj (getop_func_expr_n, res, obj); break;
    case VARG_CONSTRUCT_EXPR: ret = create_op_meta_for_res_and_obj (getop_construct_n, res, obj); break;
    case VARG_CALL_EXPR:
    {
      JERRY_ASSERT (obj != NULL);
      if (is_native_call (*obj))
      {
        ret = create_op_meta_for_native_call (*res, *obj);
      }
      else
      {
        ret = create_op_meta_for_res_and_obj (getop_call_n, res, obj);
      }
      break;
    }
    case VARG_FUNC_DECL:
    {
      JERRY_ASSERT (res == NULL);
      ret = create_op_meta_for_obj (getop_func_decl_n, obj);
      break;
    }
    case VARG_ARRAY_DECL:
    {
      JERRY_ASSERT (obj == NULL);
      ret = create_op_meta_for_obj (getop_array_decl, res);
      break;
    }
    case VARG_OBJ_DECL:
    {
      JERRY_ASSERT (obj == NULL);
      ret = create_op_meta_for_obj (getop_obj_decl, res);
      break;
    }
  }
  return ret;
}

static void
split_opcode_counter (opcode_counter_t oc, idx_t *id1, idx_t *id2)
{
  JERRY_ASSERT (id1 != NULL);
  JERRY_ASSERT (id2 != NULL);
  *id1 = (idx_t) (oc >> JERRY_BITSINBYTE);
  *id2 = (idx_t) (oc & ((1 << JERRY_BITSINBYTE) - 1));
  JERRY_ASSERT (oc == calc_opcode_counter_from_idx_idx (*id1, *id2));
}

static op_meta
last_dumped_op_meta (void)
{
  return serializer_get_op_meta ((opcode_counter_t) (serializer_get_current_opcode_counter () - 1));
}

static void
dump_single_address (opcode_t (*getop) (idx_t), operand op)
{
  switch (op.type)
  {
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop (LITERAL_TO_REWRITE);
      serializer_dump_op_meta (create_op_meta_100 (opcode, op.data.lit_id));
      break;
    }
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop (op.data.uid);
      serializer_dump_op_meta (create_op_meta_000 (opcode));
      break;
    }
  }
}

static void
dump_double_address (opcode_t (*getop) (idx_t, idx_t), operand res, operand obj)
{
  switch (res.type)
  {
    case OPERAND_LITERAL:
    {
      switch (obj.type)
      {
        case OPERAND_LITERAL:
        {
          const opcode_t opcode = getop (LITERAL_TO_REWRITE, LITERAL_TO_REWRITE);
          serializer_dump_op_meta (create_op_meta_110 (opcode, res.data.lit_id, obj.data.lit_id));
          break;
        }
        case OPERAND_TMP:
        {
          const opcode_t opcode = getop (LITERAL_TO_REWRITE, obj.data.uid);
          serializer_dump_op_meta (create_op_meta_100 (opcode, res.data.lit_id));
          break;
        }
      }
      break;
    }
    case OPERAND_TMP:
    {
      switch (obj.type)
      {
        case OPERAND_LITERAL:
        {
          const opcode_t opcode = getop (res.data.uid, LITERAL_TO_REWRITE);
          serializer_dump_op_meta (create_op_meta_010 (opcode, obj.data.lit_id));
          break;
        }
        case OPERAND_TMP:
        {
          const opcode_t opcode = getop (res.data.uid, obj.data.uid);
          serializer_dump_op_meta (create_op_meta_000 (opcode));
          break;
        }
      }
      break;
    }
  }
}

static void
dump_triple_address (opcode_t (*getop) (idx_t, idx_t, idx_t), operand res, operand lhs, operand rhs)
{
  switch (res.type)
  {
    case OPERAND_LITERAL:
    {
      switch (lhs.type)
      {
        case OPERAND_LITERAL:
        {
          switch (rhs.type)
          {
            case OPERAND_LITERAL:
            {
              const opcode_t opcode = getop (LITERAL_TO_REWRITE, LITERAL_TO_REWRITE, LITERAL_TO_REWRITE);
              serializer_dump_op_meta (create_op_meta_111 (opcode, res.data.lit_id, lhs.data.lit_id, rhs.data.lit_id));
              break;
            }
            case OPERAND_TMP:
            {
              const opcode_t opcode = getop (LITERAL_TO_REWRITE, LITERAL_TO_REWRITE, rhs.data.uid);
              serializer_dump_op_meta (create_op_meta_110 (opcode, res.data.lit_id, lhs.data.lit_id));
              break;
            }
          }
          break;
        }
        case OPERAND_TMP:
        {
          switch (rhs.type)
          {
            case OPERAND_LITERAL:
            {
              const opcode_t opcode = getop (LITERAL_TO_REWRITE, lhs.data.uid, LITERAL_TO_REWRITE);
              serializer_dump_op_meta (create_op_meta_101 (opcode, res.data.lit_id, rhs.data.lit_id));
              break;
            }
            case OPERAND_TMP:
            {
              const opcode_t opcode = getop (LITERAL_TO_REWRITE, lhs.data.uid, rhs.data.uid);
              serializer_dump_op_meta (create_op_meta_100 (opcode, res.data.lit_id));
              break;
            }
          }
          break;
        }
      }
      break;
    }
    case OPERAND_TMP:
    {
      switch (lhs.type)
      {
        case OPERAND_LITERAL:
        {
          switch (rhs.type)
          {
            case OPERAND_LITERAL:
            {
              const opcode_t opcode = getop (res.data.uid, LITERAL_TO_REWRITE, LITERAL_TO_REWRITE);
              serializer_dump_op_meta (create_op_meta_011 (opcode, lhs.data.lit_id, rhs.data.lit_id));
              break;
            }
            case OPERAND_TMP:
            {
              const opcode_t opcode = getop (res.data.uid, LITERAL_TO_REWRITE, rhs.data.uid);
              serializer_dump_op_meta (create_op_meta_010 (opcode, lhs.data.lit_id));
              break;
            }
          }
          break;
        }
        case OPERAND_TMP:
        {
          switch (rhs.type)
          {
            case OPERAND_LITERAL:
            {
              const opcode_t opcode = getop (res.data.uid, lhs.data.uid, LITERAL_TO_REWRITE);
              serializer_dump_op_meta (create_op_meta_001 (opcode, rhs.data.lit_id));
              break;
            }
            case OPERAND_TMP:
            {
              const opcode_t opcode = getop (res.data.uid, lhs.data.uid, rhs.data.uid);
              serializer_dump_op_meta (create_op_meta_000 (opcode));
              break;
            }
          }
          break;
        }
      }
      break;
    }
  }
}

static void
dump_prop_setter_op_meta (op_meta last, operand op)
{
  JERRY_ASSERT (last.op.op_idx == OPCODE (prop_getter));
  switch (op.type)
  {
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop_prop_setter (last.op.data.prop_getter.obj,
                                                 last.op.data.prop_getter.prop,
                                                 LITERAL_TO_REWRITE);
      serializer_dump_op_meta (create_op_meta_111 (opcode, last.lit_id[1], last.lit_id[2], op.data.lit_id));
      break;
    }
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop_prop_setter (last.op.data.prop_getter.obj,
                                                 last.op.data.prop_getter.prop,
                                                 op.data.uid);
      serializer_dump_op_meta (create_op_meta_110 (opcode, last.lit_id[1], last.lit_id[2]));
      break;
    }
  }
}

static operand
create_operand_from_tmp_and_lit (idx_t tmp, lit_cpointer_t lit_id)
{
  if (tmp != LITERAL_TO_REWRITE)
  {
    JERRY_ASSERT (lit_id.packed_value == MEM_CP_NULL);

    operand ret;

    ret.type = OPERAND_TMP;
    ret.data.uid = tmp;

    return ret;
  }
  else
  {
    JERRY_ASSERT (lit_id.packed_value != MEM_CP_NULL);

    operand ret;

    ret.type = OPERAND_LITERAL;
    ret.data.lit_id = lit_id;

    return ret;
  }
}

static operand
dump_triple_address_and_prop_setter_res (void (*dumper) (operand, operand, operand),
                                         op_meta last, operand op)
{
  JERRY_ASSERT (last.op.op_idx == OPCODE (prop_getter));
  const operand obj = create_operand_from_tmp_and_lit (last.op.data.prop_getter.obj, last.lit_id[1]);
  const operand prop = create_operand_from_tmp_and_lit (last.op.data.prop_getter.prop, last.lit_id[2]);
  const operand tmp = dump_prop_getter_res (obj, prop);
  dumper (tmp, tmp, op);
  dump_prop_setter (obj, prop, tmp);
  return tmp;
}

static operand
dump_prop_setter_or_triple_address_res (void (*dumper) (operand, operand, operand),
                                         operand res, operand op)
{
  const op_meta last = STACK_TOP (prop_getters);
  if (last.op.op_idx == OPCODE (prop_getter))
  {
    res = dump_triple_address_and_prop_setter_res (dumper, last, op);
  }
  else
  {
    dumper (res, res, op);
  }
  STACK_DROP (prop_getters, 1);
  return res;
}

static opcode_counter_t
get_diff_from (opcode_counter_t oc)
{
  return (opcode_counter_t) (serializer_get_current_opcode_counter () - oc);
}

operand
empty_operand (void)
{
  operand ret;

  ret.type = OPERAND_TMP;
  ret.data.uid = INVALID_VALUE;

  return ret;
}

operand
literal_operand (lit_cpointer_t lit_cp)
{
  operand ret;

  ret.type = OPERAND_LITERAL;
  ret.data.lit_id = lit_cp;

  return ret;
}

/**
 * Creates operand for eval's return value
 *
 * @return constructed operand
 */
operand
eval_ret_operand (void)
{
  operand ret;

  ret.type = OPERAND_TMP;
  ret.data.uid = EVAL_RET_VALUE;

  return ret;
} /* eval_ret_operand */

/**
 * Creates operand for taking iterator value (next property name)
 * from for-in opcode handler.
 *
 * @return constructed operand
 */
operand
jsp_create_operand_for_in_special_reg (void)
{
  operand ret;

  ret.type = OPERAND_TMP;
  ret.data.uid = OPCODE_REG_SPECIAL_FOR_IN_PROPERTY_NAME;

  return ret;
} /* jsp_create_operand_for_in_special_reg */

bool
operand_is_empty (operand op)
{
  return op.type == OPERAND_TMP && op.data.uid == INVALID_VALUE;
}

void
dumper_new_statement (void)
{
  reset_temp_name ();
}

void
dumper_new_scope (void)
{
  STACK_PUSH (temp_names, temp_name);
  STACK_PUSH (temp_names, max_temp_name);
  reset_temp_name ();
  max_temp_name = temp_name;
}

void
dumper_finish_scope (void)
{
  max_temp_name = STACK_TOP (temp_names);
  STACK_DROP (temp_names, 1);
  temp_name = STACK_TOP (temp_names);
  STACK_DROP (temp_names, 1);
}

/**
 * Handle start of argument preparation instruction sequence generation
 *
 * Note:
 *      Values of registers, allocated for the code sequence, are not used outside of the sequence,
 *      so they can be reused, reducing register pressure.
 *
 *      To reuse the registers, counter of register allocator is saved, and restored then,
 *      after finishing generation of the code sequence, using dumper_finish_varg_code_sequence.
 *
 * FIXME:
 *       Implement general register allocation mechanism
 *
 * See also:
 *          dumper_finish_varg_code_sequence
 */
void
dumper_start_varg_code_sequence (void)
{
  STACK_PUSH (temp_names, temp_name);
} /* dumper_start_varg_code_sequence */

/**
 * Handle finish of argument preparation instruction sequence generation
 *
 * See also:
 *          dumper_start_varg_code_sequence
 */
void
dumper_finish_varg_code_sequence (void)
{
  temp_name = STACK_TOP (temp_names);
  STACK_DROP (temp_names, 1);
} /* dumper_finish_varg_code_sequence */

/**
 * Check that byte-code operand refers to 'eval' string
 *
 * @return true - if specified byte-code operand's type is literal, and value of corresponding
 *                literal is equal to LIT_MAGIC_STRING_EVAL string,
 *         false - otherwise.
 */
bool
dumper_is_eval_literal (operand obj) /**< byte-code operand */
{
  /*
   * FIXME: Switch to corresponding magic string
   */
  bool is_eval_lit = (obj.type == OPERAND_LITERAL
                      && lit_literal_equal_type_cstr (lit_get_literal_by_cp (obj.data.lit_id), "eval"));

  return is_eval_lit;
} /* dumper_is_eval_literal */

void
dump_boolean_assignment (operand op, bool is_true)
{
  switch (op.type)
  {
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop_assignment (LITERAL_TO_REWRITE,
                                                OPCODE_ARG_TYPE_SIMPLE,
                                                is_true ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
      const op_meta om = create_op_meta_100 (opcode, op.data.lit_id);
      serializer_dump_op_meta (om);
      break;
    }
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop_assignment (op.data.uid,
                                                OPCODE_ARG_TYPE_SIMPLE,
                                                is_true ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
      const op_meta om = create_op_meta_000 (opcode);
      serializer_dump_op_meta (om);
      break;
    }
  }
}

operand
dump_boolean_assignment_res (bool is_true)
{
  operand op = tmp_operand ();
  dump_boolean_assignment (op, is_true);
  return op;
}

void
dump_string_assignment (operand op, lit_cpointer_t lit_id)
{
  switch (op.type)
  {
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop_assignment (LITERAL_TO_REWRITE, OPCODE_ARG_TYPE_STRING, LITERAL_TO_REWRITE);
      serializer_dump_op_meta (create_op_meta_101 (opcode, op.data.lit_id, lit_id));
      break;
    }
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop_assignment (op.data.uid, OPCODE_ARG_TYPE_STRING, LITERAL_TO_REWRITE);
      serializer_dump_op_meta (create_op_meta_001 (opcode, lit_id));
      break;
    }
  }
}

operand
dump_string_assignment_res (lit_cpointer_t lit_id)
{
  operand op = tmp_operand ();
  dump_string_assignment (op, lit_id);
  return op;
}

void
dump_number_assignment (operand op, lit_cpointer_t lit_id)
{
  switch (op.type)
  {
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop_assignment (LITERAL_TO_REWRITE, OPCODE_ARG_TYPE_NUMBER, LITERAL_TO_REWRITE);
      serializer_dump_op_meta (create_op_meta_101 (opcode, op.data.lit_id, lit_id));
      break;
    }
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop_assignment (op.data.uid, OPCODE_ARG_TYPE_NUMBER, LITERAL_TO_REWRITE);
      serializer_dump_op_meta (create_op_meta_001 (opcode, lit_id));
      break;
    }
  }
}

operand
dump_number_assignment_res (lit_cpointer_t lit_id)
{
  operand op = tmp_operand ();
  dump_number_assignment (op, lit_id);
  return op;
}

void
dump_regexp_assignment (operand op, lit_cpointer_t lit_id)
{
  switch (op.type)
  {
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop_assignment (LITERAL_TO_REWRITE, OPCODE_ARG_TYPE_REGEXP, LITERAL_TO_REWRITE);
      serializer_dump_op_meta (create_op_meta_101 (opcode, op.data.lit_id, lit_id));
      break;
    }
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop_assignment (op.data.uid, OPCODE_ARG_TYPE_REGEXP, LITERAL_TO_REWRITE);
      serializer_dump_op_meta (create_op_meta_001 (opcode, lit_id));
      break;
    }
  }
}

operand
dump_regexp_assignment_res (lit_cpointer_t lit_id)
{
  operand op = tmp_operand ();
  dump_regexp_assignment (op, lit_id);
  return op;
}

void
dump_smallint_assignment (operand op, idx_t uid)
{
  switch (op.type)
  {
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop_assignment (LITERAL_TO_REWRITE, OPCODE_ARG_TYPE_SMALLINT, uid);
      serializer_dump_op_meta (create_op_meta_100 (opcode, op.data.lit_id));
      break;
    }
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop_assignment (op.data.uid, OPCODE_ARG_TYPE_SMALLINT, uid);
      serializer_dump_op_meta (create_op_meta_000 (opcode));
      break;
    }
  }
}

operand
dump_smallint_assignment_res (idx_t uid)
{
  operand op = tmp_operand ();
  dump_smallint_assignment (op, uid);
  return op;
}

void
dump_undefined_assignment (operand op)
{
  switch (op.type)
  {
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop_assignment (LITERAL_TO_REWRITE,
                                                OPCODE_ARG_TYPE_SIMPLE,
                                                ECMA_SIMPLE_VALUE_UNDEFINED);
      serializer_dump_op_meta (create_op_meta_100 (opcode, op.data.lit_id));
      break;
    }
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop_assignment (op.data.uid, OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_UNDEFINED);
      serializer_dump_op_meta (create_op_meta_000 (opcode));
      break;
    }
  }
}

operand
dump_undefined_assignment_res (void)
{
  operand op = tmp_operand ();
  dump_undefined_assignment (op);
  return op;
}

void
dump_null_assignment (operand op)
{
  switch (op.type)
  {
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop_assignment (LITERAL_TO_REWRITE,
                                                OPCODE_ARG_TYPE_SIMPLE,
                                                ECMA_SIMPLE_VALUE_NULL);
      serializer_dump_op_meta (create_op_meta_100 (opcode, op.data.lit_id));
      break;
    }
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop_assignment (op.data.uid,
                                                OPCODE_ARG_TYPE_SIMPLE,
                                                ECMA_SIMPLE_VALUE_NULL);
      serializer_dump_op_meta (create_op_meta_000 (opcode));
      break;
    }
  }
}

operand
dump_null_assignment_res (void)
{
  operand op = tmp_operand ();
  dump_null_assignment (op);
  return op;
}

void
dump_variable_assignment (operand res, operand var)
{
  switch (res.type)
  {
    case OPERAND_LITERAL:
    {
      switch (var.type)
      {
        case OPERAND_LITERAL:
        {
          const opcode_t opcode = getop_assignment (LITERAL_TO_REWRITE,
                                                    OPCODE_ARG_TYPE_VARIABLE,
                                                    LITERAL_TO_REWRITE);
          serializer_dump_op_meta (create_op_meta_101 (opcode, res.data.lit_id, var.data.lit_id));
          break;
        }
        case OPERAND_TMP:
        {
          const opcode_t opcode = getop_assignment (LITERAL_TO_REWRITE,
                                                    OPCODE_ARG_TYPE_VARIABLE,
                                                    var.data.uid);
          serializer_dump_op_meta (create_op_meta_100 (opcode, res.data.lit_id));
          break;
        }
      }
      break;
    }
    case OPERAND_TMP:
    {
      switch (var.type)
      {
        case OPERAND_LITERAL:
        {
          const opcode_t opcode = getop_assignment (res.data.uid,
                                                    OPCODE_ARG_TYPE_VARIABLE,
                                                    LITERAL_TO_REWRITE);
          serializer_dump_op_meta (create_op_meta_001 (opcode, var.data.lit_id));
          break;
        }
        case OPERAND_TMP:
        {
          const opcode_t opcode = getop_assignment (res.data.uid,
                                                    OPCODE_ARG_TYPE_VARIABLE,
                                                    var.data.uid);
          serializer_dump_op_meta (create_op_meta_000 (opcode));
          break;
        }
      }
      break;
    }
  }
}

operand
dump_variable_assignment_res (operand var)
{
  operand op = tmp_operand ();
  dump_variable_assignment (op, var);
  return op;
}

void
dump_varg_header_for_rewrite (varg_list_type vlt, operand obj)
{
  STACK_PUSH (varg_headers, serializer_get_current_opcode_counter ());
  switch (vlt)
  {
    case VARG_FUNC_EXPR:
    case VARG_CONSTRUCT_EXPR:
    case VARG_CALL_EXPR:
    {
      operand res = empty_operand ();
      serializer_dump_op_meta (create_op_meta_for_vlt (vlt, &res, &obj));
      break;
    }
    case VARG_FUNC_DECL:
    {
      serializer_dump_op_meta (create_op_meta_for_vlt (vlt, NULL, &obj));
      break;
    }
    case VARG_ARRAY_DECL:
    case VARG_OBJ_DECL:
    {
      operand res = empty_operand ();
      serializer_dump_op_meta (create_op_meta_for_vlt (vlt, &res, NULL));
      break;
    }
  }
}

operand
rewrite_varg_header_set_args_count (uint8_t args_count)
{
  op_meta om = serializer_get_op_meta (STACK_TOP (varg_headers));
  switch (om.op.op_idx)
  {
    case OPCODE (func_expr_n):
    case OPCODE (construct_n):
    case OPCODE (call_n):
    case OPCODE (native_call):
    {
      const operand res = tmp_operand ();
      om.op.data.func_expr_n.arg_list = args_count;
      om.op.data.func_expr_n.lhs = res.data.uid;
      serializer_rewrite_op_meta (STACK_TOP (varg_headers), om);
      STACK_DROP (varg_headers, 1);
      return res;
    }
    case OPCODE (func_decl_n):
    {
      om.op.data.func_decl_n.arg_list = args_count;
      serializer_rewrite_op_meta (STACK_TOP (varg_headers), om);
      STACK_DROP (varg_headers, 1);
      return empty_operand ();
    }
    case OPCODE (array_decl):
    case OPCODE (obj_decl):
    {
      const operand res = tmp_operand ();
      om.op.data.obj_decl.list = args_count;
      om.op.data.obj_decl.lhs = res.data.uid;
      serializer_rewrite_op_meta (STACK_TOP (varg_headers), om);
      STACK_DROP (varg_headers, 1);
      return res;
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
}

/**
 * Dump 'meta' instruction of 'call additional information' type,
 * containing call flags and, optionally, 'this' argument
 */
void
dump_call_additional_info (opcode_call_flags_t flags, /**< call flags */
                           operand this_arg) /**< 'this' argument - if flags include OPCODE_CALL_FLAGS_HAVE_THIS_ARG,
                                              *   or empty operand - otherwise */
{
  if (flags & OPCODE_CALL_FLAGS_HAVE_THIS_ARG)
  {
    JERRY_ASSERT (this_arg.type == OPERAND_TMP);
    JERRY_ASSERT (!operand_is_empty (this_arg));
  }
  else
  {
    JERRY_ASSERT (operand_is_empty (this_arg));
  }

  const opcode_t opcode = getop_meta (OPCODE_META_TYPE_CALL_SITE_INFO,
                                      flags,
                                      (idx_t) (flags & OPCODE_CALL_FLAGS_HAVE_THIS_ARG
                                               ? this_arg.data.uid
                                               : INVALID_VALUE));

  serializer_dump_op_meta (create_op_meta_000 (opcode));
} /* dump_call_additional_info */

void
dump_varg (operand op)
{
  switch (op.type)
  {
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop_meta (OPCODE_META_TYPE_VARG, op.data.uid, INVALID_VALUE);
      serializer_dump_op_meta (create_op_meta_000 (opcode));
      return;
    }
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop_meta (OPCODE_META_TYPE_VARG, LITERAL_TO_REWRITE, INVALID_VALUE);
      serializer_dump_op_meta (create_op_meta_010 (opcode, op.data.lit_id));
      return;
    }
  }
}

void
dump_prop_name_and_value (operand name, operand value)
{
  JERRY_ASSERT (name.type == OPERAND_LITERAL);
  literal_t lit = lit_get_literal_by_cp (name.data.lit_id);
  operand tmp;
  if (lit->get_type () == LIT_STR_T
      || lit->get_type () == LIT_MAGIC_STR_T
      || lit->get_type () == LIT_MAGIC_STR_EX_T)
  {
    tmp = dump_string_assignment_res (name.data.lit_id);
  }
  else
  {
    JERRY_ASSERT (lit->get_type () == LIT_NUMBER_T);
    tmp = dump_number_assignment_res (name.data.lit_id);
  }
  switch (value.type)
  {
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop_meta (OPCODE_META_TYPE_VARG_PROP_DATA, tmp.data.uid, LITERAL_TO_REWRITE);
      serializer_dump_op_meta (create_op_meta_001 (opcode, value.data.lit_id));
      break;
    }
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop_meta (OPCODE_META_TYPE_VARG_PROP_DATA, tmp.data.uid, value.data.uid);
      serializer_dump_op_meta (create_op_meta_000 (opcode));
      break;
    }
  }
}

void
dump_prop_getter_decl (operand name, operand func)
{
  JERRY_ASSERT (name.type == OPERAND_LITERAL);
  JERRY_ASSERT (func.type == OPERAND_TMP);
  literal_t lit = lit_get_literal_by_cp (name.data.lit_id);
  operand tmp;
  if (lit->get_type () == LIT_STR_T
      || lit->get_type () == LIT_MAGIC_STR_T
      || lit->get_type () == LIT_MAGIC_STR_EX_T)
  {
    tmp = dump_string_assignment_res (name.data.lit_id);
  }
  else
  {
    JERRY_ASSERT (lit->get_type () == LIT_NUMBER_T);
    tmp = dump_number_assignment_res (name.data.lit_id);
  }
  const opcode_t opcode = getop_meta (OPCODE_META_TYPE_VARG_PROP_GETTER, tmp.data.uid, func.data.uid);
  serializer_dump_op_meta (create_op_meta_000 (opcode));
}

void
dump_prop_setter_decl (operand name, operand func)
{
  JERRY_ASSERT (name.type == OPERAND_LITERAL);
  JERRY_ASSERT (func.type == OPERAND_TMP);
  literal_t lit = lit_get_literal_by_cp (name.data.lit_id);
  operand tmp;
  if (lit->get_type () == LIT_STR_T
      || lit->get_type () == LIT_MAGIC_STR_T
      || lit->get_type () == LIT_MAGIC_STR_EX_T)
  {
    tmp = dump_string_assignment_res (name.data.lit_id);
  }
  else
  {
    JERRY_ASSERT (lit->get_type () == LIT_NUMBER_T);
    tmp = dump_number_assignment_res (name.data.lit_id);
  }
  const opcode_t opcode = getop_meta (OPCODE_META_TYPE_VARG_PROP_SETTER, tmp.data.uid, func.data.uid);
  serializer_dump_op_meta (create_op_meta_000 (opcode));
}

void
dump_prop_getter (operand res, operand obj, operand prop)
{
  dump_triple_address (getop_prop_getter, res, obj, prop);
}

operand
dump_prop_getter_res (operand obj, operand prop)
{
  const operand res = tmp_operand ();
  dump_prop_getter (res, obj, prop);
  return res;
}

void
dump_prop_setter (operand res, operand obj, operand prop)
{
  dump_triple_address (getop_prop_setter, res, obj, prop);
}

void
dump_function_end_for_rewrite (void)
{
  STACK_PUSH (function_ends, serializer_get_current_opcode_counter ());
  const opcode_t opcode = getop_meta (OPCODE_META_TYPE_FUNCTION_END, INVALID_VALUE, INVALID_VALUE);
  serializer_dump_op_meta (create_op_meta_000 (opcode));
}

void
rewrite_function_end (varg_list_type vlt)
{
  opcode_counter_t oc;
  if (vlt == VARG_FUNC_DECL)
  {
    oc = (opcode_counter_t) (get_diff_from (STACK_TOP (function_ends))
                                            + serializer_count_opcodes_in_subscopes ());
  }
  else
  {
    JERRY_ASSERT (vlt == VARG_FUNC_EXPR);
    oc = (opcode_counter_t) (get_diff_from (STACK_TOP (function_ends)));
  }
  idx_t id1, id2;
  split_opcode_counter (oc, &id1, &id2);
  const opcode_t opcode = getop_meta (OPCODE_META_TYPE_FUNCTION_END, id1, id2);
  serializer_rewrite_op_meta (STACK_TOP (function_ends), create_op_meta_000 (opcode));
  STACK_DROP (function_ends, 1);
}

void
dump_this (operand op)
{
  dump_single_address (getop_this_binding, op);
}

operand
dump_this_res (void)
{
  const operand res = tmp_operand ();
  dump_this (res);
  return res;
}

void
dump_post_increment (operand res, operand obj)
{
  dump_double_address (getop_post_incr, res, obj);
}

operand
dump_post_increment_res (operand op)
{
  const operand res = tmp_operand ();
  dump_post_increment (res, op);
  return res;
}

void
dump_post_decrement (operand res, operand obj)
{
  dump_double_address (getop_post_decr, res, obj);
}

operand
dump_post_decrement_res (operand op)
{
  const operand res = tmp_operand ();
  dump_post_decrement (res, op);
  return res;
}

void
dump_pre_increment (operand res, operand obj)
{
  dump_double_address (getop_pre_incr, res, obj);
}

operand
dump_pre_increment_res (operand op)
{
  const operand res = tmp_operand ();
  dump_pre_increment (res, op);
  return res;
}

void
dump_pre_decrement (operand res, operand obj)
{
  dump_double_address (getop_pre_decr, res, obj);
}

operand
dump_pre_decrement_res (operand op)
{
  const operand res = tmp_operand ();
  dump_pre_decrement (res, op);
  return res;
}

void
dump_unary_plus (operand res, operand obj)
{
  dump_double_address (getop_unary_plus, res, obj);
}

operand
dump_unary_plus_res (operand op)
{
  const operand res = tmp_operand ();
  dump_unary_plus (res, op);
  return res;
}

void
dump_unary_minus (operand res, operand obj)
{
  dump_double_address (getop_unary_minus, res, obj);
}

operand
dump_unary_minus_res (operand op)
{
  const operand res = tmp_operand ();
  dump_unary_minus (res, op);
  return res;
}

void
dump_bitwise_not (operand res, operand obj)
{
  dump_double_address (getop_b_not, res, obj);
}

operand
dump_bitwise_not_res (operand op)
{
  const operand res = tmp_operand ();
  dump_bitwise_not (res, op);
  return res;
}

void
dump_logical_not (operand res, operand obj)
{
  dump_double_address (getop_logical_not, res, obj);
}

operand
dump_logical_not_res (operand op)
{
  const operand res = tmp_operand ();
  dump_logical_not (res, op);
  return res;
}

void
dump_delete (operand res, operand op, bool is_strict, locus loc)
{
  switch (op.type)
  {
    case OPERAND_LITERAL:
    {
      literal_t lit = lit_get_literal_by_cp (op.data.lit_id);
      if (lit->get_type () == LIT_STR_T
          || lit->get_type () == LIT_MAGIC_STR_T
          || lit->get_type () == LIT_MAGIC_STR_EX_T)
      {
        syntax_check_delete (is_strict, loc);
        switch (res.type)
        {
          case OPERAND_LITERAL:
          {
            const opcode_t opcode = getop_delete_var (LITERAL_TO_REWRITE, LITERAL_TO_REWRITE);
            serializer_dump_op_meta (create_op_meta_110 (opcode, res.data.lit_id, op.data.lit_id));
            break;
          }
          case OPERAND_TMP:
          {
            const opcode_t opcode = getop_delete_var (res.data.uid, LITERAL_TO_REWRITE);
            serializer_dump_op_meta (create_op_meta_010 (opcode, op.data.lit_id));
            break;
          }
        }
        break;
      }
      else if (lit->get_type ()  == LIT_NUMBER_T)
      {
        dump_boolean_assignment (res, true);
      }
      break;
    }
    case OPERAND_TMP:
    {
      const op_meta last_op_meta = last_dumped_op_meta ();
      switch (last_op_meta.op.op_idx)
      {
        case OPCODE (assignment):
        {
          if (last_op_meta.op.data.assignment.value_right == LITERAL_TO_REWRITE)
          {
            const operand var_op = literal_operand (last_op_meta.lit_id[2]);
            syntax_check_delete (is_strict, loc);
            switch (res.type)
            {
              case OPERAND_LITERAL:
              {
                const opcode_t opcode = getop_delete_var (LITERAL_TO_REWRITE, LITERAL_TO_REWRITE);
                serializer_dump_op_meta (create_op_meta_110 (opcode, res.data.lit_id, var_op.data.lit_id));
                break;
              }
              case OPERAND_TMP:
              {
                const opcode_t opcode = getop_delete_var (res.data.uid, LITERAL_TO_REWRITE);
                serializer_dump_op_meta (create_op_meta_010 (opcode, var_op.data.lit_id));
                break;
              }
            }
          }
          else
          {
            dump_boolean_assignment (res, true);
          }
          break;
        }
        case OPCODE (prop_getter):
        {
          const opcode_counter_t oc = (opcode_counter_t) (serializer_get_current_opcode_counter () - 1);
          serializer_set_writing_position (oc);
          switch (res.type)
          {
            case OPERAND_LITERAL:
            {
              if (last_op_meta.op.data.prop_getter.obj == LITERAL_TO_REWRITE)
              {
                if (last_op_meta.op.data.prop_getter.prop == LITERAL_TO_REWRITE)
                {
                  const opcode_t opcode = getop_delete_prop (LITERAL_TO_REWRITE,
                                                             LITERAL_TO_REWRITE,
                                                             LITERAL_TO_REWRITE);
                  serializer_dump_op_meta (create_op_meta_111 (opcode, res.data.lit_id,
                                                               last_op_meta.lit_id[1],
                                                               last_op_meta.lit_id[2]));
                }
                else
                {
                  const opcode_t opcode = getop_delete_prop (LITERAL_TO_REWRITE,
                                                             LITERAL_TO_REWRITE,
                                                             last_op_meta.op.data.prop_getter.prop);
                  serializer_dump_op_meta (create_op_meta_110 (opcode, res.data.lit_id,
                                                               last_op_meta.lit_id[1]));
                }
              }
              else
              {
                if (last_op_meta.op.data.prop_getter.prop == LITERAL_TO_REWRITE)
                {
                  const opcode_t opcode = getop_delete_prop (LITERAL_TO_REWRITE,
                                                             last_op_meta.op.data.prop_getter.obj,
                                                             LITERAL_TO_REWRITE);
                  serializer_dump_op_meta (create_op_meta_101 (opcode, res.data.lit_id,
                                                               last_op_meta.lit_id[2]));
                }
                else
                {
                  const opcode_t opcode = getop_delete_prop (LITERAL_TO_REWRITE,
                                                             last_op_meta.op.data.prop_getter.obj,
                                                             last_op_meta.op.data.prop_getter.prop);
                  serializer_dump_op_meta (create_op_meta_100 (opcode, res.data.lit_id));
                }
              }
              break;
            }
            case OPERAND_TMP:
            {
              if (last_op_meta.op.data.prop_getter.obj == LITERAL_TO_REWRITE)
              {
                if (last_op_meta.op.data.prop_getter.prop == LITERAL_TO_REWRITE)
                {
                  const opcode_t opcode = getop_delete_prop (res.data.uid,
                                                             LITERAL_TO_REWRITE,
                                                             LITERAL_TO_REWRITE);
                  serializer_dump_op_meta (create_op_meta_011 (opcode, last_op_meta.lit_id[1],
                                                               last_op_meta.lit_id[2]));
                }
                else
                {
                  const opcode_t opcode = getop_delete_prop (res.data.uid,
                                                             LITERAL_TO_REWRITE,
                                                             last_op_meta.op.data.prop_getter.prop);
                  serializer_dump_op_meta (create_op_meta_010 (opcode, last_op_meta.lit_id[1]));
                }
              }
              else
              {
                if (last_op_meta.op.data.prop_getter.prop == LITERAL_TO_REWRITE)
                {
                  const opcode_t opcode = getop_delete_prop (res.data.uid,
                                                             last_op_meta.op.data.prop_getter.obj,
                                                             LITERAL_TO_REWRITE);
                  serializer_dump_op_meta (create_op_meta_001 (opcode, last_op_meta.lit_id[2]));
                }
                else
                {
                  const opcode_t opcode = getop_delete_prop (res.data.uid,
                                                             last_op_meta.op.data.prop_getter.obj,
                                                             last_op_meta.op.data.prop_getter.prop);
                  serializer_dump_op_meta (create_op_meta_000 (opcode));
                }
              }
              break;
            }
          }
          break;
        }
      }
      break;
    }
  }
}

operand
dump_delete_res (operand op, bool is_strict, locus loc)
{
  const operand res = tmp_operand ();
  dump_delete (res, op, is_strict, loc);
  return res;
}

void
dump_typeof (operand res, operand op)
{
  dump_double_address (getop_typeof, res, op);
}

operand
dump_typeof_res (operand op)
{
  const operand res = tmp_operand ();
  dump_typeof (res, op);
  return res;
}

void
dump_multiplication (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_multiplication, res, lhs, rhs);
}

operand
dump_multiplication_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_multiplication (res, lhs, rhs);
  return res;
}

void
dump_division (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_division, res, lhs, rhs);
}

operand
dump_division_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_division (res, lhs, rhs);
  return res;
}

void
dump_remainder (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_remainder, res, lhs, rhs);
}

operand
dump_remainder_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_remainder (res, lhs, rhs);
  return res;
}

void
dump_addition (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_addition, res, lhs, rhs);
}

operand
dump_addition_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_addition (res, lhs, rhs);
  return res;
}

void
dump_substraction (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_substraction, res, lhs, rhs);
}

operand
dump_substraction_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_substraction (res, lhs, rhs);
  return res;
}

void
dump_left_shift (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_b_shift_left, res, lhs, rhs);
}

operand
dump_left_shift_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_left_shift (res, lhs, rhs);
  return res;
}

void
dump_right_shift (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_b_shift_right, res, lhs, rhs);
}

operand
dump_right_shift_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_right_shift (res, lhs, rhs);
  return res;
}

void
dump_right_shift_ex (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_b_shift_uright, res, lhs, rhs);
}

operand
dump_right_shift_ex_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_right_shift_ex (res, lhs, rhs);
  return res;
}

void
dump_less_than (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_less_than, res, lhs, rhs);
}

operand
dump_less_than_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_less_than (res, lhs, rhs);
  return res;
}

void
dump_greater_than (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_greater_than, res, lhs, rhs);
}

operand
dump_greater_than_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_greater_than (res, lhs, rhs);
  return res;
}

void
dump_less_or_equal_than (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_less_or_equal_than, res, lhs, rhs);
}

operand
dump_less_or_equal_than_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_less_or_equal_than (res, lhs, rhs);
  return res;
}

void
dump_greater_or_equal_than (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_greater_or_equal_than, res, lhs, rhs);
}

operand
dump_greater_or_equal_than_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_greater_or_equal_than (res, lhs, rhs);
  return res;
}

void
dump_instanceof (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_instanceof, res, lhs, rhs);
}

operand
dump_instanceof_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_instanceof (res, lhs, rhs);
  return res;
}

void
dump_in (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_in, res, lhs, rhs);
}

operand
dump_in_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_in (res, lhs, rhs);
  return res;
}

void
dump_equal_value (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_equal_value, res, lhs, rhs);
}

operand
dump_equal_value_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_equal_value (res, lhs, rhs);
  return res;
}

void
dump_not_equal_value (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_not_equal_value, res, lhs, rhs);
}

operand
dump_not_equal_value_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_not_equal_value (res, lhs, rhs);
  return res;
}

void
dump_equal_value_type (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_equal_value_type, res, lhs, rhs);
}

operand
dump_equal_value_type_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_equal_value_type (res, lhs, rhs);
  return res;
}

void
dump_not_equal_value_type (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_not_equal_value_type, res, lhs, rhs);
}

operand
dump_not_equal_value_type_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_not_equal_value_type (res, lhs, rhs);
  return res;
}

void
dump_bitwise_and (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_b_and, res, lhs, rhs);
}

operand
dump_bitwise_and_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_bitwise_and (res, lhs, rhs);
  return res;
}

void
dump_bitwise_xor (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_b_xor, res, lhs, rhs);
}

operand
dump_bitwise_xor_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_bitwise_xor (res, lhs, rhs);
  return res;
}

void
dump_bitwise_or (operand res, operand lhs, operand rhs)
{
  dump_triple_address (getop_b_or, res, lhs, rhs);
}

operand
dump_bitwise_or_res (operand lhs, operand rhs)
{
  const operand res = tmp_operand ();
  dump_bitwise_or (res, lhs, rhs);
  return res;
}

void
start_dumping_logical_and_checks (void)
{
  STACK_PUSH (U8, (uint8_t) STACK_SIZE (logical_and_checks));
}

void
dump_logical_and_check_for_rewrite (operand op)
{
  STACK_PUSH (logical_and_checks, serializer_get_current_opcode_counter ());
  switch (op.type)
  {
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop_is_false_jmp_down (LITERAL_TO_REWRITE, INVALID_VALUE, INVALID_VALUE);
      serializer_dump_op_meta (create_op_meta_100 (opcode, op.data.lit_id));
      break;
    }
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop_is_false_jmp_down (op.data.uid, INVALID_VALUE, INVALID_VALUE);
      serializer_dump_op_meta (create_op_meta_000 (opcode));
      break;
    }
  }
}

void
rewrite_logical_and_checks (void)
{
  for (uint8_t i = STACK_TOP (U8); i < STACK_SIZE (logical_and_checks); i++)
  {
    op_meta jmp_op_meta = serializer_get_op_meta (STACK_ELEMENT (logical_and_checks, i));
    JERRY_ASSERT (jmp_op_meta.op.op_idx == OPCODE (is_false_jmp_down));
    idx_t id1, id2;
    split_opcode_counter (get_diff_from (STACK_ELEMENT (logical_and_checks, i)), &id1, &id2);
    jmp_op_meta.op.data.is_false_jmp_down.opcode_1 = id1;
    jmp_op_meta.op.data.is_false_jmp_down.opcode_2 = id2;
    serializer_rewrite_op_meta (STACK_ELEMENT (logical_and_checks, i), jmp_op_meta);
  }
  STACK_DROP (logical_and_checks, STACK_SIZE (logical_and_checks) - STACK_TOP (U8));
  STACK_DROP (U8, 1);
}

void
start_dumping_logical_or_checks (void)
{
  STACK_PUSH (U8, (uint8_t) STACK_SIZE (logical_or_checks));
}

void
dump_logical_or_check_for_rewrite (operand op)
{
  STACK_PUSH (logical_or_checks, serializer_get_current_opcode_counter ());
  switch (op.type)
  {
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop_is_true_jmp_down (LITERAL_TO_REWRITE, INVALID_VALUE, INVALID_VALUE);
      serializer_dump_op_meta (create_op_meta_100 (opcode, op.data.lit_id));
      break;
    }
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop_is_true_jmp_down (op.data.uid, INVALID_VALUE, INVALID_VALUE);
      serializer_dump_op_meta (create_op_meta_000 (opcode));
      break;
    }
  }
}

void
rewrite_logical_or_checks (void)
{
  for (uint8_t i = STACK_TOP (U8); i < STACK_SIZE (logical_or_checks); i++)
  {
    op_meta jmp_op_meta = serializer_get_op_meta (STACK_ELEMENT (logical_or_checks, i));
    JERRY_ASSERT (jmp_op_meta.op.op_idx == OPCODE (is_true_jmp_down));
    idx_t id1, id2;
    split_opcode_counter (get_diff_from (STACK_ELEMENT (logical_or_checks, i)), &id1, &id2);
    jmp_op_meta.op.data.is_true_jmp_down.opcode_1 = id1;
    jmp_op_meta.op.data.is_true_jmp_down.opcode_2 = id2;
    serializer_rewrite_op_meta (STACK_ELEMENT (logical_or_checks, i), jmp_op_meta);
  }
  STACK_DROP (logical_or_checks, STACK_SIZE (logical_or_checks) - STACK_TOP (U8));
  STACK_DROP (U8, 1);
}

void
dump_conditional_check_for_rewrite (operand op)
{
  STACK_PUSH (conditional_checks, serializer_get_current_opcode_counter ());
  switch (op.type)
  {
    case OPERAND_LITERAL:
    {
      const opcode_t opcode = getop_is_false_jmp_down (LITERAL_TO_REWRITE, INVALID_VALUE, INVALID_VALUE);
      serializer_dump_op_meta (create_op_meta_100 (opcode, op.data.lit_id));
      break;
    }
    case OPERAND_TMP:
    {
      const opcode_t opcode = getop_is_false_jmp_down (op.data.uid, INVALID_VALUE, INVALID_VALUE);
      serializer_dump_op_meta (create_op_meta_000 (opcode));
      break;
    }
  }
}

void
rewrite_conditional_check (void)
{
  op_meta jmp_op_meta = serializer_get_op_meta (STACK_TOP (conditional_checks));
  JERRY_ASSERT (jmp_op_meta.op.op_idx == OPCODE (is_false_jmp_down));
  idx_t id1, id2;
  split_opcode_counter (get_diff_from (STACK_TOP (conditional_checks)), &id1, &id2);
  jmp_op_meta.op.data.is_false_jmp_down.opcode_1 = id1;
  jmp_op_meta.op.data.is_false_jmp_down.opcode_2 = id2;
  serializer_rewrite_op_meta (STACK_TOP (conditional_checks), jmp_op_meta);
  STACK_DROP (conditional_checks, 1);
}

void
dump_jump_to_end_for_rewrite (void)
{
  STACK_PUSH (jumps_to_end, serializer_get_current_opcode_counter ());
  const opcode_t opcode = getop_jmp_down (INVALID_VALUE, INVALID_VALUE);
  serializer_dump_op_meta (create_op_meta_000 (opcode));
}

void
rewrite_jump_to_end (void)
{
  op_meta jmp_op_meta = serializer_get_op_meta (STACK_TOP (jumps_to_end));
  JERRY_ASSERT (jmp_op_meta.op.op_idx == OPCODE (jmp_down));
  idx_t id1, id2;
  split_opcode_counter (get_diff_from (STACK_TOP (jumps_to_end)), &id1, &id2);
  jmp_op_meta.op.data.jmp_down.opcode_1 = id1;
  jmp_op_meta.op.data.jmp_down.opcode_2 = id2;
  serializer_rewrite_op_meta (STACK_TOP (jumps_to_end), jmp_op_meta);
  STACK_DROP (jumps_to_end, 1);
}

void
start_dumping_assignment_expression (void)
{
  const op_meta last = last_dumped_op_meta ();
  if (last.op.op_idx == OPCODE (prop_getter))
  {
    serializer_set_writing_position ((opcode_counter_t) (serializer_get_current_opcode_counter () - 1));
  }
  STACK_PUSH (prop_getters, last);
}

operand
dump_prop_setter_or_variable_assignment_res (operand res, operand op)
{
  const op_meta last = STACK_TOP (prop_getters);
  if (last.op.op_idx == OPCODE (prop_getter))
  {
    dump_prop_setter_op_meta (last, op);
  }
  else
  {
    dump_variable_assignment (res, op);
  }
  STACK_DROP (prop_getters, 1);
  return op;
}

operand
dump_prop_setter_or_addition_res (operand res, operand op)
{
  return dump_prop_setter_or_triple_address_res (dump_addition, res, op);
}

operand
dump_prop_setter_or_multiplication_res (operand res, operand op)
{
  return dump_prop_setter_or_triple_address_res (dump_multiplication, res, op);
}

operand
dump_prop_setter_or_division_res (operand res, operand op)
{
  return dump_prop_setter_or_triple_address_res (dump_division, res, op);
}

operand
dump_prop_setter_or_remainder_res (operand res, operand op)
{
  return dump_prop_setter_or_triple_address_res (dump_remainder, res, op);
}

operand
dump_prop_setter_or_substraction_res (operand res, operand op)
{
  return dump_prop_setter_or_triple_address_res (dump_substraction, res, op);
}

operand
dump_prop_setter_or_left_shift_res (operand res, operand op)
{
  return dump_prop_setter_or_triple_address_res (dump_left_shift, res, op);
}

operand
dump_prop_setter_or_right_shift_res (operand res, operand op)
{
  return dump_prop_setter_or_triple_address_res (dump_right_shift, res, op);
}

operand
dump_prop_setter_or_right_shift_ex_res (operand res, operand op)
{
  return dump_prop_setter_or_triple_address_res (dump_right_shift_ex, res, op);
}

operand
dump_prop_setter_or_bitwise_and_res (operand res, operand op)
{
  return dump_prop_setter_or_triple_address_res (dump_bitwise_and, res, op);
}

operand
dump_prop_setter_or_bitwise_xor_res (operand res, operand op)
{
  return dump_prop_setter_or_triple_address_res (dump_bitwise_xor, res, op);
}

operand
dump_prop_setter_or_bitwise_or_res (operand res, operand op)
{
  return dump_prop_setter_or_triple_address_res (dump_bitwise_or, res, op);
}

void
dumper_set_next_interation_target (void)
{
  STACK_PUSH (next_iterations, serializer_get_current_opcode_counter ());
}

void
dump_continue_iterations_check (operand op)
{
  const opcode_counter_t next_iteration_target_diff = (opcode_counter_t) (serializer_get_current_opcode_counter ()
                                                                          - STACK_TOP (next_iterations));
  idx_t id1, id2;
  split_opcode_counter (next_iteration_target_diff, &id1, &id2);
  if (operand_is_empty (op))
  {
    const opcode_t opcode = getop_jmp_up (id1, id2);
    serializer_dump_op_meta (create_op_meta_000 (opcode));
  }
  else
  {
    switch (op.type)
    {
      case OPERAND_LITERAL:
      {
        const opcode_t opcode = getop_is_true_jmp_up (LITERAL_TO_REWRITE, id1, id2);
        serializer_dump_op_meta (create_op_meta_100 (opcode, op.data.lit_id));
        break;
      }
      case OPERAND_TMP:
      {
        const opcode_t opcode = getop_is_true_jmp_up (op.data.uid, id1, id2);
        serializer_dump_op_meta (create_op_meta_000 (opcode));
        break;
      }
    }
  }
  STACK_DROP (next_iterations, 1);
}

/**
 * Dump template of 'jmp_break_continue' or 'jmp_down' instruction (depending on is_simple_jump argument).
 *
 * Note:
 *      the instruction's flags field is written later (see also: rewrite_simple_or_nested_jump_get_next).
 *
 * @return position of dumped instruction
 */
opcode_counter_t
dump_simple_or_nested_jump_for_rewrite (bool is_simple_jump, /**< flag indicating, whether simple jump
                                                              *   or 'jmp_break_continue' template should be dumped */
                                        opcode_counter_t next_jump_for_tgt_oc) /**< opcode counter of next
                                                                                *   template targetted to
                                                                                *   the same target - if any,
                                                                                *   or MAX_OPCODES - otherwise */
{
  idx_t id1, id2;
  split_opcode_counter (next_jump_for_tgt_oc, &id1, &id2);

  opcode_t opcode;
  if (is_simple_jump)
  {
    opcode = getop_jmp_down (id1, id2);
  }
  else
  {
    opcode = getop_jmp_break_continue (id1, id2);
  }

  opcode_counter_t ret = serializer_get_current_opcode_counter ();

  serializer_dump_op_meta (create_op_meta_000 (opcode));

  return ret;
} /* dump_simple_or_nested_jump_for_rewrite */

/**
 * Write jump target position into previously dumped template of jump (simple or nested) instruction
 *
 * @return opcode counter value that was encoded in the jump before rewrite
 */
opcode_counter_t
rewrite_simple_or_nested_jump_and_get_next (opcode_counter_t jump_oc, /**< position of jump to rewrite */
                                            opcode_counter_t target_oc) /**< the jump's target */
{
  op_meta jump_op_meta = serializer_get_op_meta (jump_oc);

  bool is_simple_jump = (jump_op_meta.op.op_idx == OPCODE (jmp_down));

  JERRY_ASSERT (is_simple_jump
                || (jump_op_meta.op.op_idx == OPCODE (jmp_break_continue)));

  idx_t id1, id2, id1_prev, id2_prev;
  split_opcode_counter ((opcode_counter_t) (target_oc - jump_oc), &id1, &id2);

  if (is_simple_jump)
  {
    id1_prev = jump_op_meta.op.data.jmp_down.opcode_1;
    id2_prev = jump_op_meta.op.data.jmp_down.opcode_2;

    jump_op_meta.op.data.jmp_down.opcode_1 = id1;
    jump_op_meta.op.data.jmp_down.opcode_2 = id2;
  }
  else
  {
    JERRY_ASSERT (jump_op_meta.op.op_idx == OPCODE (jmp_break_continue));

    id1_prev = jump_op_meta.op.data.jmp_break_continue.opcode_1;
    id2_prev = jump_op_meta.op.data.jmp_break_continue.opcode_2;

    jump_op_meta.op.data.jmp_break_continue.opcode_1 = id1;
    jump_op_meta.op.data.jmp_break_continue.opcode_2 = id2;
  }

  serializer_rewrite_op_meta (jump_oc, jump_op_meta);

  return calc_opcode_counter_from_idx_idx (id1_prev, id2_prev);
} /* rewrite_simple_or_nested_jump_get_next */

void
start_dumping_case_clauses (void)
{
  STACK_PUSH (U8, (uint8_t) STACK_SIZE (case_clauses));
  STACK_PUSH (U8, (uint8_t) STACK_SIZE (case_clauses));
}

void
dump_case_clause_check_for_rewrite (operand switch_expr, operand case_expr)
{
  const operand res = tmp_operand ();
  dump_triple_address (getop_equal_value_type, res, switch_expr, case_expr);
  STACK_PUSH (case_clauses, serializer_get_current_opcode_counter ());
  const opcode_t opcode = getop_is_true_jmp_down (res.data.uid, INVALID_VALUE, INVALID_VALUE);
  serializer_dump_op_meta (create_op_meta_000 (opcode));
}

void
dump_default_clause_check_for_rewrite (void)
{
  STACK_PUSH (case_clauses, serializer_get_current_opcode_counter ());
  const opcode_t opcode = getop_jmp_down (INVALID_VALUE, INVALID_VALUE);
  serializer_dump_op_meta (create_op_meta_000 (opcode));
}

void
rewrite_case_clause (void)
{
  const opcode_counter_t jmp_oc = STACK_ELEMENT (case_clauses, STACK_HEAD (U8, 2));
  idx_t id1, id2;
  split_opcode_counter (get_diff_from (jmp_oc), &id1, &id2);
  op_meta jmp_op_meta = serializer_get_op_meta (jmp_oc);
  JERRY_ASSERT (jmp_op_meta.op.op_idx == OPCODE (is_true_jmp_down));
  jmp_op_meta.op.data.is_true_jmp_down.opcode_1 = id1;
  jmp_op_meta.op.data.is_true_jmp_down.opcode_2 = id2;
  serializer_rewrite_op_meta (jmp_oc, jmp_op_meta);
  STACK_INCR_HEAD (U8, 2);
}

void
rewrite_default_clause (void)
{
  const opcode_counter_t jmp_oc = STACK_TOP (case_clauses);
  idx_t id1, id2;
  split_opcode_counter (get_diff_from (jmp_oc), &id1, &id2);
  op_meta jmp_op_meta = serializer_get_op_meta (jmp_oc);
  JERRY_ASSERT (jmp_op_meta.op.op_idx == OPCODE (jmp_down));
  jmp_op_meta.op.data.jmp_down.opcode_1 = id1;
  jmp_op_meta.op.data.jmp_down.opcode_2 = id2;
  serializer_rewrite_op_meta (jmp_oc, jmp_op_meta);
}

void
finish_dumping_case_clauses (void)
{
  STACK_DROP (case_clauses, STACK_SIZE (case_clauses) - STACK_TOP (U8));
  STACK_DROP (U8, 1);
  STACK_DROP (U8, 1);
}

/**
 * Dump template of 'with' instruction.
 *
 * Note:
 *      the instruction's flags field is written later (see also: rewrite_with).
 *
 * @return position of dumped instruction
 */
opcode_counter_t
dump_with_for_rewrite (operand op) /**< operand - result of evaluating Expression
                                    *   in WithStatement */
{
  opcode_counter_t oc = serializer_get_current_opcode_counter ();

  if (op.type == OPERAND_LITERAL)
  {
    const opcode_t opcode = getop_with (LITERAL_TO_REWRITE, INVALID_VALUE, INVALID_VALUE);
    serializer_dump_op_meta (create_op_meta_100 (opcode, op.data.lit_id));
  }
  else
  {
    JERRY_ASSERT (op.type == OPERAND_TMP);

    const opcode_t opcode = getop_with (op.data.uid, INVALID_VALUE, INVALID_VALUE);
    serializer_dump_op_meta (create_op_meta_000 (opcode));
  }

  return oc;
} /* dump_with_for_rewrite */

/**
 * Write position of 'with' block's end to specified 'with' instruction template,
 * dumped earlier (see also: dump_with_for_rewrite).
 */
void
rewrite_with (opcode_counter_t oc) /**< opcode counter of the instruction template */
{
  op_meta with_op_meta = serializer_get_op_meta (oc);

  idx_t id1, id2;
  split_opcode_counter (get_diff_from (oc), &id1, &id2);
  with_op_meta.op.data.with.oc_idx_1 = id1;
  with_op_meta.op.data.with.oc_idx_2 = id2;
  serializer_rewrite_op_meta (oc, with_op_meta);
} /* rewrite_with */

/**
 * Dump 'meta' instruction of 'end with' type
 */
void
dump_with_end (void)
{
  const opcode_t opcode = getop_meta (OPCODE_META_TYPE_END_WITH, INVALID_VALUE, INVALID_VALUE);
  serializer_dump_op_meta (create_op_meta_000 (opcode));
} /* dump_with_end */

/**
 * Dump template of 'for_in' instruction.
 *
 * Note:
 *      the instruction's flags field is written later (see also: rewrite_for_in).
 *
 * @return position of dumped instruction
 */
opcode_counter_t
dump_for_in_for_rewrite (operand op) /**< operand - result of evaluating Expression
                                      *   in for-in statement */
{
  opcode_counter_t oc = serializer_get_current_opcode_counter ();

  if (op.type == OPERAND_LITERAL)
  {
    const opcode_t opcode = getop_for_in (LITERAL_TO_REWRITE, INVALID_VALUE, INVALID_VALUE);
    serializer_dump_op_meta (create_op_meta_100 (opcode, op.data.lit_id));
  }
  else
  {
    JERRY_ASSERT (op.type == OPERAND_TMP);

    const opcode_t opcode = getop_for_in (op.data.uid, INVALID_VALUE, INVALID_VALUE);
    serializer_dump_op_meta (create_op_meta_000 (opcode));
  }

  return oc;
} /* dump_for_in_for_rewrite */

/**
 * Write position of 'for_in' block's end to specified 'for_in' instruction template,
 * dumped earlier (see also: dump_for_in_for_rewrite).
 */
void
rewrite_for_in (opcode_counter_t oc) /**< opcode counter of the instruction template */
{
  op_meta for_in_op_meta = serializer_get_op_meta (oc);

  idx_t id1, id2;
  split_opcode_counter (get_diff_from (oc), &id1, &id2);
  for_in_op_meta.op.data.for_in.oc_idx_1 = id1;
  for_in_op_meta.op.data.for_in.oc_idx_2 = id2;
  serializer_rewrite_op_meta (oc, for_in_op_meta);
} /* rewrite_for_in */

/**
 * Dump 'meta' instruction of 'end for_in' type
 */
void
dump_for_in_end (void)
{
  const opcode_t opcode = getop_meta (OPCODE_META_TYPE_END_FOR_IN, INVALID_VALUE, INVALID_VALUE);
  serializer_dump_op_meta (create_op_meta_000 (opcode));
} /* dump_for_in_end */

void
dump_try_for_rewrite (void)
{
  STACK_PUSH (tries, serializer_get_current_opcode_counter ());
  const opcode_t opcode = getop_try_block (INVALID_VALUE, INVALID_VALUE);
  serializer_dump_op_meta (create_op_meta_000 (opcode));
}

void
rewrite_try (void)
{
  op_meta try_op_meta = serializer_get_op_meta (STACK_TOP (tries));
  JERRY_ASSERT (try_op_meta.op.op_idx == OPCODE (try_block));
  idx_t id1, id2;
  split_opcode_counter (get_diff_from (STACK_TOP (tries)), &id1, &id2);
  try_op_meta.op.data.try_block.oc_idx_1 = id1;
  try_op_meta.op.data.try_block.oc_idx_2 = id2;
  serializer_rewrite_op_meta (STACK_TOP (tries), try_op_meta);
  STACK_DROP (tries, 1);
}

void
dump_catch_for_rewrite (operand op)
{
  JERRY_ASSERT (op.type == OPERAND_LITERAL);
  STACK_PUSH (catches, serializer_get_current_opcode_counter ());
  opcode_t opcode = getop_meta (OPCODE_META_TYPE_CATCH, INVALID_VALUE, INVALID_VALUE);
  serializer_dump_op_meta (create_op_meta_000 (opcode));
  opcode = getop_meta (OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER, LITERAL_TO_REWRITE, INVALID_VALUE);
  serializer_dump_op_meta (create_op_meta_010 (opcode, op.data.lit_id));
}

void
rewrite_catch (void)
{
  op_meta catch_op_meta = serializer_get_op_meta (STACK_TOP (catches));
  JERRY_ASSERT (catch_op_meta.op.op_idx == OPCODE (meta)
                && catch_op_meta.op.data.meta.type == OPCODE_META_TYPE_CATCH);
  idx_t id1, id2;
  split_opcode_counter (get_diff_from (STACK_TOP (catches)), &id1, &id2);
  catch_op_meta.op.data.meta.data_1 = id1;
  catch_op_meta.op.data.meta.data_2 = id2;
  serializer_rewrite_op_meta (STACK_TOP (catches), catch_op_meta);
  STACK_DROP (catches, 1);
}

void
dump_finally_for_rewrite (void)
{
  STACK_PUSH (finallies, serializer_get_current_opcode_counter ());
  const opcode_t opcode = getop_meta (OPCODE_META_TYPE_FINALLY, INVALID_VALUE, INVALID_VALUE);
  serializer_dump_op_meta (create_op_meta_000 (opcode));
}

void
rewrite_finally (void)
{
  op_meta finally_op_meta = serializer_get_op_meta (STACK_TOP (finallies));
  JERRY_ASSERT (finally_op_meta.op.op_idx == OPCODE (meta)
                && finally_op_meta.op.data.meta.type == OPCODE_META_TYPE_FINALLY);
  idx_t id1, id2;
  split_opcode_counter (get_diff_from (STACK_TOP (finallies)), &id1, &id2);
  finally_op_meta.op.data.meta.data_1 = id1;
  finally_op_meta.op.data.meta.data_2 = id2;
  serializer_rewrite_op_meta (STACK_TOP (finallies), finally_op_meta);
  STACK_DROP (finallies, 1);
}

void
dump_end_try_catch_finally (void)
{
  const opcode_t opcode = getop_meta (OPCODE_META_TYPE_END_TRY_CATCH_FINALLY,
                                      INVALID_VALUE, INVALID_VALUE);
  serializer_dump_op_meta (create_op_meta_000 (opcode));
}

void
dump_throw (operand op)
{
  dump_single_address (getop_throw_value, op);
}

bool
dumper_variable_declaration_exists (lit_cpointer_t lit_id)
{
  for (opcode_counter_t oc = (opcode_counter_t) (serializer_get_current_opcode_counter () - 1);
       oc > 0; oc--)
  {
    const op_meta var_decl_op_meta = serializer_get_op_meta (oc);
    if (var_decl_op_meta.op.op_idx != OPCODE (var_decl))
    {
      break;
    }
    if (var_decl_op_meta.lit_id[0].packed_value == lit_id.packed_value)
    {
      return true;
    }
  }
  return false;
}

void
dump_variable_declaration (lit_cpointer_t lit_id)
{
  const opcode_t opcode = getop_var_decl (LITERAL_TO_REWRITE);
  serializer_dump_op_meta (create_op_meta_100 (opcode, lit_id));
}

/**
 * Dump template of 'meta' instruction for scope's code flags.
 *
 * Note:
 *      the instruction's flags field is written later (see also: rewrite_scope_code_flags).
 *
 * @return position of dumped instruction
 */
opcode_counter_t
dump_scope_code_flags_for_rewrite (void)
{
  opcode_counter_t oc = serializer_get_current_opcode_counter ();

  const opcode_t opcode = getop_meta (OPCODE_META_TYPE_SCOPE_CODE_FLAGS, INVALID_VALUE, INVALID_VALUE);
  serializer_dump_op_meta (create_op_meta_000 (opcode));

  return oc;
} /* dump_scope_code_flags_for_rewrite */

/**
 * Write scope's code flags to specified 'meta' instruction template,
 * dumped earlier (see also: dump_scope_code_flags_for_rewrite).
 */
void
rewrite_scope_code_flags (opcode_counter_t scope_code_flags_oc, /**< position of instruction to rewrite */
                          opcode_scope_code_flags_t scope_flags) /**< scope's code properties flags set */
{
  JERRY_ASSERT ((idx_t) scope_flags == scope_flags);

  op_meta opm = serializer_get_op_meta (scope_code_flags_oc);
  JERRY_ASSERT (opm.op.op_idx == OPCODE (meta));
  JERRY_ASSERT (opm.op.data.meta.type == OPCODE_META_TYPE_SCOPE_CODE_FLAGS);
  JERRY_ASSERT (opm.op.data.meta.data_1 == INVALID_VALUE);
  JERRY_ASSERT (opm.op.data.meta.data_2 == INVALID_VALUE);

  opm.op.data.meta.data_1 = (idx_t) scope_flags;
  serializer_rewrite_op_meta (scope_code_flags_oc, opm);
} /* rewrite_scope_code_flags */

void
dump_ret (void)
{
  serializer_dump_op_meta (create_op_meta_000 (getop_ret ()));
}

void
dump_reg_var_decl_for_rewrite (void)
{
  STACK_PUSH (reg_var_decls, serializer_get_current_opcode_counter ());
  serializer_dump_op_meta (create_op_meta_000 (getop_reg_var_decl (OPCODE_REG_FIRST, INVALID_VALUE)));
}

void
rewrite_reg_var_decl (void)
{
  opcode_counter_t reg_var_decl_oc = STACK_TOP (reg_var_decls);
  op_meta opm = serializer_get_op_meta (reg_var_decl_oc);
  JERRY_ASSERT (opm.op.op_idx == OPCODE (reg_var_decl));
  opm.op.data.reg_var_decl.max = max_temp_name;
  serializer_rewrite_op_meta (reg_var_decl_oc, opm);
  STACK_DROP (reg_var_decls, 1);
}

void
dump_retval (operand op)
{
  dump_single_address (getop_retval, op);
}

void
dumper_init (void)
{
  max_temp_name = 0;
  reset_temp_name ();
  STACK_INIT (U8);
  STACK_INIT (varg_headers);
  STACK_INIT (function_ends);
  STACK_INIT (logical_and_checks);
  STACK_INIT (logical_or_checks);
  STACK_INIT (conditional_checks);
  STACK_INIT (jumps_to_end);
  STACK_INIT (prop_getters);
  STACK_INIT (next_iterations);
  STACK_INIT (case_clauses);
  STACK_INIT (catches);
  STACK_INIT (finallies);
  STACK_INIT (tries);
  STACK_INIT (temp_names);
  STACK_INIT (reg_var_decls);
}

void
dumper_free (void)
{
  STACK_FREE (U8);
  STACK_FREE (varg_headers);
  STACK_FREE (function_ends);
  STACK_FREE (logical_and_checks);
  STACK_FREE (logical_or_checks);
  STACK_FREE (conditional_checks);
  STACK_FREE (jumps_to_end);
  STACK_FREE (prop_getters);
  STACK_FREE (next_iterations);
  STACK_FREE (case_clauses);
  STACK_FREE (catches);
  STACK_FREE (finallies);
  STACK_FREE (tries);
  STACK_FREE (temp_names);
  STACK_FREE (reg_var_decls);
}

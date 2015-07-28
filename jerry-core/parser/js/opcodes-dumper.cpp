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
#include "jsp-early-error.h"

/**
 * Register allocator's counter
 */
static vm_idx_t jsp_reg_next;

/**
 * Maximum identifier of a register, allocated for intermediate value storage
 *
 * See also:
 *          dumper_new_scope, dumper_finish_scope
 */
static vm_idx_t jsp_reg_max_for_temps;

/**
 * Maximum identifier of a register, allocated for storage of a variable value.
 *
 * The value can be VM_IDX_EMPTY, indicating that no registers were allocated for variable values.
 *
 * Note:
 *      Registers for variable values are always allocated after registers for temporary values,
 *      so the value, if not equal to VM_IDX_EMPTY, is always greater than jsp_reg_max_for_temps.
 *
 * See also:
 *          dumper_try_replace_var_with_reg
 */
static vm_idx_t jsp_reg_max_for_local_var;

enum
{
  U8_global_size
};
STATIC_STACK (U8, uint8_t)

enum
{
  varg_headers_global_size
};
STATIC_STACK (varg_headers, vm_instr_counter_t)

enum
{
  function_ends_global_size
};
STATIC_STACK (function_ends, vm_instr_counter_t)

enum
{
  logical_and_checks_global_size
};
STATIC_STACK (logical_and_checks, vm_instr_counter_t)

enum
{
  logical_or_checks_global_size
};
STATIC_STACK (logical_or_checks, vm_instr_counter_t)

enum
{
  conditional_checks_global_size
};
STATIC_STACK (conditional_checks, vm_instr_counter_t)

enum
{
  jumps_to_end_global_size
};
STATIC_STACK (jumps_to_end, vm_instr_counter_t)

enum
{
  prop_getters_global_size
};
STATIC_STACK (prop_getters, op_meta)

enum
{
  next_iterations_global_size
};
STATIC_STACK (next_iterations, vm_instr_counter_t)

enum
{
  case_clauses_global_size
};
STATIC_STACK (case_clauses, vm_instr_counter_t)

enum
{
  tries_global_size
};
STATIC_STACK (tries, vm_instr_counter_t)

enum
{
  catches_global_size
};
STATIC_STACK (catches, vm_instr_counter_t)

enum
{
  finallies_global_size
};
STATIC_STACK (finallies, vm_instr_counter_t)

enum
{
  jsp_reg_id_stack_global_size
};
STATIC_STACK (jsp_reg_id_stack, vm_idx_t)

enum
{
  reg_var_decls_global_size
};
STATIC_STACK (reg_var_decls, vm_instr_counter_t)

/**
 * Allocate next register for intermediate value
 *
 * @return identifier of the allocated register
 */
static vm_idx_t
jsp_alloc_reg_for_temp (void)
{
  JERRY_ASSERT (jsp_reg_max_for_local_var == VM_IDX_EMPTY);

  vm_idx_t next_reg = jsp_reg_next++;

  if (next_reg > VM_REG_GENERAL_LAST)
  {
    /*
     * FIXME:
     *       Implement mechanism, allowing reusage of register variables
     */
    PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "Not enough register variables", LIT_ITERATOR_POS_ZERO);
  }

  if (jsp_reg_max_for_temps < next_reg)
  {
    jsp_reg_max_for_temps = next_reg;
  }

  return next_reg;
} /* jsp_alloc_reg_for_temp */

#ifdef CONFIG_PARSER_ENABLE_PARSE_TIME_BYTE_CODE_OPTIMIZER
/**
 * Try to move local variable to a register
 *
 * Note:
 *      First instruction of the scope should be either func_decl_n or func_expr_n, as the scope is function scope,
 *      and the optimization is not applied to 'new Function ()'-like constructed functions.
 *
 * See also:
 *          parse_source_element_list
 *          parser_parse_program
 *
 * @return true, if optimization performed successfully, i.e.:
 *                - there is a free register to use;
 *                - the variable name is not equal to any of the function's argument names;
 *         false - otherwise.
 */
bool
dumper_try_replace_var_with_reg (scopes_tree tree, /**< a function scope, created for
                                                    *   function declaration or function expresssion */
                                 op_meta *var_decl_om_p) /**< operation meta of corresponding variable declaration */
{
  JERRY_ASSERT (tree->type == SCOPE_TYPE_FUNCTION);

  JERRY_ASSERT (var_decl_om_p->op.op_idx == VM_OP_VAR_DECL);
  JERRY_ASSERT (var_decl_om_p->lit_id[0].packed_value != NOT_A_LITERAL.packed_value);
  JERRY_ASSERT (var_decl_om_p->lit_id[1].packed_value == NOT_A_LITERAL.packed_value);
  JERRY_ASSERT (var_decl_om_p->lit_id[2].packed_value == NOT_A_LITERAL.packed_value);

  vm_instr_counter_t instr_pos = 0;

  op_meta header_opm = scopes_tree_op_meta (tree, instr_pos++);
  JERRY_ASSERT (header_opm.op.op_idx == VM_OP_FUNC_EXPR_N || header_opm.op.op_idx == VM_OP_FUNC_DECL_N);

  while (true)
  {
    op_meta meta_opm = scopes_tree_op_meta (tree, instr_pos++);
    JERRY_ASSERT (meta_opm.op.op_idx == VM_OP_META);

    opcode_meta_type meta_type = (opcode_meta_type) meta_opm.op.data.meta.type;

    if (meta_type == OPCODE_META_TYPE_FUNCTION_END)
    {
      /* marker of function argument list end reached */
      break;
    }
    else
    {
      JERRY_ASSERT (meta_type == OPCODE_META_TYPE_VARG);

      /* the varg specifies argument name, and so should be a string literal */
      JERRY_ASSERT (meta_opm.op.data.meta.data_1 == VM_IDX_REWRITE_LITERAL_UID);
      JERRY_ASSERT (meta_opm.lit_id[1].packed_value != NOT_A_LITERAL.packed_value);

      if (meta_opm.lit_id[1].packed_value == var_decl_om_p->lit_id[0].packed_value)
      {
        /*
         * Optimization is not performed, because the variable's name is equal to an argument name,
         * and the argument's value would be initialized by its name in run-time.
         *
         * See also:
         *          parser_parse_program
         */
        return false;
      }
    }
  }

  if (jsp_reg_max_for_local_var == VM_IDX_EMPTY)
  {
    jsp_reg_max_for_local_var = jsp_reg_max_for_temps;
  }

  if (jsp_reg_max_for_local_var == VM_REG_GENERAL_LAST)
  {
    /* not enough registers */
    return false;
  }
  JERRY_ASSERT (jsp_reg_max_for_local_var < VM_REG_GENERAL_LAST);

  vm_idx_t reg = ++jsp_reg_max_for_local_var;

  lit_cpointer_t lit_cp = var_decl_om_p->lit_id[0];

  for (vm_instr_counter_t instr_pos = 0;
       instr_pos < tree->instrs_count;
       instr_pos++)
  {
    op_meta om = scopes_tree_op_meta (tree, instr_pos);

    vm_op_t opcode = (vm_op_t) om.op.op_idx;

    int args_num = 0;

#define VM_OP_0(opcode_name, opcode_name_uppercase) \
    if (opcode == VM_OP_ ## opcode_name_uppercase) \
    { \
      args_num = 0; \
    }
#define VM_OP_1(opcode_name, opcode_name_uppercase, arg1, arg1_type) \
    if (opcode == VM_OP_ ## opcode_name_uppercase) \
    { \
      JERRY_STATIC_ASSERT (((arg1_type) & VM_OP_ARG_TYPE_TYPE_OF_NEXT) == 0); \
      args_num = 1; \
    }
#define VM_OP_2(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type) \
    if (opcode == VM_OP_ ## opcode_name_uppercase) \
    { \
      JERRY_STATIC_ASSERT (((arg1_type) & VM_OP_ARG_TYPE_TYPE_OF_NEXT) == 0); \
      JERRY_STATIC_ASSERT (((arg2_type) & VM_OP_ARG_TYPE_TYPE_OF_NEXT) == 0); \
      args_num = 2; \
    }
#define VM_OP_3(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type, arg3, arg3_type) \
    if (opcode == VM_OP_ ## opcode_name_uppercase) \
    { \
      JERRY_STATIC_ASSERT (((arg1_type) & VM_OP_ARG_TYPE_TYPE_OF_NEXT) == 0); \
      \
      /*
       * See also:
       *          The loop below
       */ \
      \
      JERRY_ASSERT ((opcode == VM_OP_ASSIGNMENT && (arg2_type) == VM_OP_ARG_TYPE_TYPE_OF_NEXT) \
                    || (opcode != VM_OP_ASSIGNMENT && ((arg2_type) & VM_OP_ARG_TYPE_TYPE_OF_NEXT) == 0)); \
      JERRY_STATIC_ASSERT (((arg3_type) & VM_OP_ARG_TYPE_TYPE_OF_NEXT) == 0); \
      args_num = 3; \
    }

#include "vm-opcodes.inc.h"

    for (int arg_index = 0; arg_index < args_num; arg_index++)
    {
      /*
       * This is the only opcode with statically unspecified argument type (checked by assertions above)
       */
      if (opcode == VM_OP_ASSIGNMENT
          && arg_index == 1
          && om.op.data.assignment.type_value_right != VM_OP_ARG_TYPE_VARIABLE)
      {
        break;
      }

      if (om.lit_id[arg_index].packed_value == lit_cp.packed_value)
      {
        om.lit_id[arg_index] = NOT_A_LITERAL;

        raw_instr *raw_p = (raw_instr *) (&om.op);
        JERRY_ASSERT (raw_p->uids[arg_index + 1] == VM_IDX_REWRITE_LITERAL_UID);
        raw_p->uids[arg_index + 1] = reg;
      }
    }

    scopes_tree_set_op_meta (tree, instr_pos, om);
  }

  return true;
} /* dumper_try_replace_var_with_reg */
#endif /* CONFIG_PARSER_ENABLE_PARSE_TIME_BYTE_CODE_OPTIMIZER */

static op_meta
create_op_meta (vm_instr_t op, lit_cpointer_t lit_id1, lit_cpointer_t lit_id2, lit_cpointer_t lit_id3)
{
  op_meta ret;

  ret.op = op;
  ret.lit_id[0] = lit_id1;
  ret.lit_id[1] = lit_id2;
  ret.lit_id[2] = lit_id3;

  return ret;
}

static op_meta
create_op_meta_000 (vm_instr_t op)
{
  return create_op_meta (op, NOT_A_LITERAL, NOT_A_LITERAL, NOT_A_LITERAL);
}

static op_meta
create_op_meta_001 (vm_instr_t op, lit_cpointer_t lit_id)
{
  return create_op_meta (op, NOT_A_LITERAL, NOT_A_LITERAL, lit_id);
}

static op_meta
create_op_meta_010 (vm_instr_t op, lit_cpointer_t lit_id)
{
  return create_op_meta (op, NOT_A_LITERAL, lit_id, NOT_A_LITERAL);
}

static op_meta
create_op_meta_011 (vm_instr_t op, lit_cpointer_t lit_id2, lit_cpointer_t lit_id3)
{
  return create_op_meta (op, NOT_A_LITERAL, lit_id2, lit_id3);
}

static op_meta
create_op_meta_100 (vm_instr_t op, lit_cpointer_t lit_id)
{
  return create_op_meta (op, lit_id, NOT_A_LITERAL, NOT_A_LITERAL);
}

static op_meta
create_op_meta_101 (vm_instr_t op, lit_cpointer_t lit_id1, lit_cpointer_t lit_id3)
{
  return create_op_meta (op, lit_id1, NOT_A_LITERAL, lit_id3);
}

static op_meta
create_op_meta_110 (vm_instr_t op, lit_cpointer_t lit_id1, lit_cpointer_t lit_id2)
{
  return create_op_meta (op, lit_id1, lit_id2, NOT_A_LITERAL);
}

static op_meta
create_op_meta_111 (vm_instr_t op, lit_cpointer_t lit_id1, lit_cpointer_t lit_id2, lit_cpointer_t lit_id3)
{
  return create_op_meta (op, lit_id1, lit_id2, lit_id3);
}

static jsp_operand_t
tmp_operand (void)
{
  return jsp_operand_t::make_reg_operand (jsp_alloc_reg_for_temp ());
}


static op_meta
create_op_meta_for_res_and_obj (vm_instr_t (*getop) (vm_idx_t, vm_idx_t, vm_idx_t),
                                jsp_operand_t *res,
                                jsp_operand_t *obj)
{
  JERRY_ASSERT (obj != NULL);
  JERRY_ASSERT (res != NULL);
  op_meta ret;
  if (obj->is_register_operand ()
      || obj->is_empty_operand ())
  {
    if (res->is_register_operand ()
        || res->is_empty_operand ())
    {
      const vm_instr_t instr = getop (res->get_idx (), obj->get_idx (), VM_IDX_REWRITE_GENERAL_CASE);
      ret = create_op_meta_000 (instr);
    }
    else
    {
      JERRY_ASSERT (res->is_literal_operand ());

      const vm_instr_t instr = getop (VM_IDX_REWRITE_LITERAL_UID, obj->get_idx (), VM_IDX_REWRITE_GENERAL_CASE);
      ret = create_op_meta_100 (instr, res->get_literal ());
    }
  }
  else
  {
    JERRY_ASSERT (obj->is_literal_operand ());

    if (res->is_register_operand ()
        || res->is_empty_operand ())
    {
      const vm_instr_t instr = getop (res->get_idx (), VM_IDX_REWRITE_LITERAL_UID, VM_IDX_REWRITE_GENERAL_CASE);
      ret = create_op_meta_010 (instr, obj->get_literal ());
    }
    else
    {
      JERRY_ASSERT (res->is_literal_operand ());

      const vm_instr_t instr = getop (VM_IDX_REWRITE_LITERAL_UID,
                                      VM_IDX_REWRITE_LITERAL_UID,
                                      VM_IDX_REWRITE_GENERAL_CASE);
      ret = create_op_meta_110 (instr, res->get_literal (), obj->get_literal ());
    }
  }
  return ret;
}

static op_meta
create_op_meta_for_obj (vm_instr_t (*getop) (vm_idx_t, vm_idx_t), jsp_operand_t *obj)
{
  JERRY_ASSERT (obj != NULL);
  op_meta res;

  if (obj->is_register_operand ()
      || obj->is_empty_operand ())
  {
    const vm_instr_t instr = getop (obj->get_idx (), VM_IDX_REWRITE_GENERAL_CASE);
    res = create_op_meta_000 (instr);
  }
  else
  {
    JERRY_ASSERT (obj->is_literal_operand ());

    const vm_instr_t instr = getop (VM_IDX_REWRITE_LITERAL_UID, VM_IDX_REWRITE_GENERAL_CASE);
    res = create_op_meta_100 (instr, obj->get_literal ());
  }

  return res;
}

static op_meta
create_op_meta_for_vlt (varg_list_type vlt, jsp_operand_t *res, jsp_operand_t *obj)
{
  op_meta ret;
  switch (vlt)
  {
    case VARG_FUNC_EXPR: ret = create_op_meta_for_res_and_obj (getop_func_expr_n, res, obj); break;
    case VARG_CONSTRUCT_EXPR: ret = create_op_meta_for_res_and_obj (getop_construct_n, res, obj); break;
    case VARG_CALL_EXPR:
    {
      JERRY_ASSERT (obj != NULL);
      ret = create_op_meta_for_res_and_obj (getop_call_n, res, obj);
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
      jsp_operand_t empty = empty_operand ();
      ret = create_op_meta_for_res_and_obj (getop_array_decl, res, &empty);
      break;
    }
    case VARG_OBJ_DECL:
    {
      JERRY_ASSERT (obj == NULL);
      jsp_operand_t empty = empty_operand ();
      ret = create_op_meta_for_res_and_obj (getop_obj_decl, res, &empty);
      break;
    }
  }
  return ret;
}

static void
split_instr_counter (vm_instr_counter_t oc, vm_idx_t *id1, vm_idx_t *id2)
{
  JERRY_ASSERT (id1 != NULL);
  JERRY_ASSERT (id2 != NULL);
  *id1 = (vm_idx_t) (oc >> JERRY_BITSINBYTE);
  *id2 = (vm_idx_t) (oc & ((1 << JERRY_BITSINBYTE) - 1));
  JERRY_ASSERT (oc == vm_calc_instr_counter_from_idx_idx (*id1, *id2));
}

static op_meta
last_dumped_op_meta (void)
{
  return serializer_get_op_meta ((vm_instr_counter_t) (serializer_get_current_instr_counter () - 1));
}

static void
dump_single_address (vm_instr_t (*getop) (vm_idx_t), jsp_operand_t op)
{
  if (op.is_literal_operand ())
  {
    const vm_instr_t instr = getop (VM_IDX_REWRITE_LITERAL_UID);
    serializer_dump_op_meta (create_op_meta_100 (instr, op.get_literal ()));
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const vm_instr_t instr = getop (op.get_idx ());
    serializer_dump_op_meta (create_op_meta_000 (instr));
  }
}

static void
dump_double_address (vm_instr_t (*getop) (vm_idx_t, vm_idx_t), jsp_operand_t res, jsp_operand_t obj)
{
  if (res.is_literal_operand ())
  {
    if (obj.is_literal_operand ())
    {
      const vm_instr_t instr = getop (VM_IDX_REWRITE_LITERAL_UID, VM_IDX_REWRITE_LITERAL_UID);
      serializer_dump_op_meta (create_op_meta_110 (instr, res.get_literal (), obj.get_literal ()));
    }
    else
    {
      JERRY_ASSERT (obj.is_register_operand ());

      const vm_instr_t instr = getop (VM_IDX_REWRITE_LITERAL_UID, obj.get_idx ());
      serializer_dump_op_meta (create_op_meta_100 (instr, res.get_literal ()));
    }
  }
  else
  {
    JERRY_ASSERT (res.is_register_operand ());

    if (obj.is_literal_operand ())
    {
      const vm_instr_t instr = getop (res.get_idx (), VM_IDX_REWRITE_LITERAL_UID);
      serializer_dump_op_meta (create_op_meta_010 (instr, obj.get_literal ()));
    }
    else
    {
      JERRY_ASSERT (obj.is_register_operand ());

      const vm_instr_t instr = getop (res.get_idx (), obj.get_idx ());
      serializer_dump_op_meta (create_op_meta_000 (instr));
    }
  }
}

static void
dump_triple_address (vm_instr_t (*getop) (vm_idx_t, vm_idx_t, vm_idx_t),
                     jsp_operand_t res,
                     jsp_operand_t lhs,
                     jsp_operand_t rhs)
{
  if (res.is_literal_operand ())
  {
    if (lhs.is_literal_operand ())
    {
      if (rhs.is_literal_operand ())
      {
        const vm_instr_t instr = getop (VM_IDX_REWRITE_LITERAL_UID,
                                        VM_IDX_REWRITE_LITERAL_UID,
                                        VM_IDX_REWRITE_LITERAL_UID);
        serializer_dump_op_meta (create_op_meta_111 (instr,
                                                     res.get_literal (),
                                                     lhs.get_literal (),
                                                     rhs.get_literal ()));
      }
      else
      {
        JERRY_ASSERT (rhs.is_register_operand ());

        const vm_instr_t instr = getop (VM_IDX_REWRITE_LITERAL_UID, VM_IDX_REWRITE_LITERAL_UID, rhs.get_idx ());
        serializer_dump_op_meta (create_op_meta_110 (instr, res.get_literal (), lhs.get_literal ()));
      }
    }
    else
    {
      JERRY_ASSERT (lhs.is_register_operand ());

      if (rhs.is_literal_operand ())
      {
        const vm_instr_t instr = getop (VM_IDX_REWRITE_LITERAL_UID, lhs.get_idx (), VM_IDX_REWRITE_LITERAL_UID);
        serializer_dump_op_meta (create_op_meta_101 (instr, res.get_literal (), rhs.get_literal ()));
      }
      else
      {
        JERRY_ASSERT (rhs.is_register_operand ());

        const vm_instr_t instr = getop (VM_IDX_REWRITE_LITERAL_UID, lhs.get_idx (), rhs.get_idx ());
        serializer_dump_op_meta (create_op_meta_100 (instr, res.get_literal ()));
      }
    }
  }
  else
  {
    JERRY_ASSERT (res.is_register_operand ());

    if (lhs.is_literal_operand ())
    {
      if (rhs.is_literal_operand ())
      {
        const vm_instr_t instr = getop (res.get_idx (), VM_IDX_REWRITE_LITERAL_UID, VM_IDX_REWRITE_LITERAL_UID);
        serializer_dump_op_meta (create_op_meta_011 (instr, lhs.get_literal (), rhs.get_literal ()));
      }
      else
      {
        JERRY_ASSERT (rhs.is_register_operand ());

        const vm_instr_t instr = getop (res.get_idx (), VM_IDX_REWRITE_LITERAL_UID, rhs.get_idx ());
        serializer_dump_op_meta (create_op_meta_010 (instr, lhs.get_literal ()));
      }
    }
    else
    {
      JERRY_ASSERT (lhs.is_register_operand ());

      if (rhs.is_literal_operand ())
      {
        const vm_instr_t instr = getop (res.get_idx (), lhs.get_idx (), VM_IDX_REWRITE_LITERAL_UID);
        serializer_dump_op_meta (create_op_meta_001 (instr, rhs.get_literal ()));
      }
      else
      {
        JERRY_ASSERT (rhs.is_register_operand ());


        const vm_instr_t instr = getop (res.get_idx (), lhs.get_idx (), rhs.get_idx ());
        serializer_dump_op_meta (create_op_meta_000 (instr));
      }
    }
  }
}

static void
dump_prop_setter_op_meta (op_meta last, jsp_operand_t op)
{
  JERRY_ASSERT (last.op.op_idx == VM_OP_PROP_GETTER);

  if (op.is_literal_operand ())
  {
    const vm_instr_t instr = getop_prop_setter (last.op.data.prop_getter.obj,
                                                last.op.data.prop_getter.prop,
                                                VM_IDX_REWRITE_LITERAL_UID);
    serializer_dump_op_meta (create_op_meta_111 (instr, last.lit_id[1], last.lit_id[2], op.get_literal ()));
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const vm_instr_t instr = getop_prop_setter (last.op.data.prop_getter.obj,
                                                last.op.data.prop_getter.prop,
                                                op.get_idx ());
    serializer_dump_op_meta (create_op_meta_110 (instr, last.lit_id[1], last.lit_id[2]));
  }
}

static jsp_operand_t
create_operand_from_tmp_and_lit (vm_idx_t tmp, lit_cpointer_t lit_id)
{
  if (tmp != VM_IDX_REWRITE_LITERAL_UID)
  {
    JERRY_ASSERT (lit_id.packed_value == MEM_CP_NULL);

    return jsp_operand_t::make_reg_operand (tmp);
  }
  else
  {
    JERRY_ASSERT (lit_id.packed_value != MEM_CP_NULL);

    return jsp_operand_t::make_lit_operand (lit_id);
  }
}

static jsp_operand_t
dump_triple_address_and_prop_setter_res (void (*dumper) (jsp_operand_t, jsp_operand_t, jsp_operand_t),
                                         op_meta last, jsp_operand_t op)
{
  JERRY_ASSERT (last.op.op_idx == VM_OP_PROP_GETTER);
  const jsp_operand_t obj = create_operand_from_tmp_and_lit (last.op.data.prop_getter.obj, last.lit_id[1]);
  const jsp_operand_t prop = create_operand_from_tmp_and_lit (last.op.data.prop_getter.prop, last.lit_id[2]);
  const jsp_operand_t tmp = dump_prop_getter_res (obj, prop);
  dumper (tmp, tmp, op);
  dump_prop_setter (obj, prop, tmp);
  return tmp;
}

static jsp_operand_t
dump_prop_setter_or_triple_address_res (void (*dumper) (jsp_operand_t, jsp_operand_t, jsp_operand_t),
                                         jsp_operand_t res, jsp_operand_t op)
{
  const op_meta last = STACK_TOP (prop_getters);
  if (last.op.op_idx == VM_OP_PROP_GETTER)
  {
    res = dump_triple_address_and_prop_setter_res (dumper, last, op);
  }
  else
  {
    if (res.is_register_operand ())
    {
      /*
       * FIXME:
       *       Implement correct handling of references through parser operands
       */
      PARSE_ERROR (JSP_EARLY_ERROR_REFERENCE, "Invalid left-hand-side expression", LIT_ITERATOR_POS_ZERO);
    }

    dumper (res, res, op);
  }
  STACK_DROP (prop_getters, 1);
  return res;
}

static vm_instr_counter_t
get_diff_from (vm_instr_counter_t oc)
{
  return (vm_instr_counter_t) (serializer_get_current_instr_counter () - oc);
}

jsp_operand_t
empty_operand (void)
{
  return jsp_operand_t::make_empty_operand ();
}

jsp_operand_t
literal_operand (lit_cpointer_t lit_cp)
{
  return jsp_operand_t::make_lit_operand (lit_cp);
}

/**
 * Creates operand for eval's return value
 *
 * @return constructed operand
 */
jsp_operand_t
eval_ret_operand (void)
{
  return jsp_operand_t::make_reg_operand (VM_REG_SPECIAL_EVAL_RET);
} /* eval_ret_operand */

/**
 * Creates operand for taking iterator value (next property name)
 * from for-in instr handler.
 *
 * @return constructed operand
 */
jsp_operand_t
jsp_create_operand_for_in_special_reg (void)
{
  return jsp_operand_t::make_reg_operand (VM_REG_SPECIAL_FOR_IN_PROPERTY_NAME);
} /* jsp_create_operand_for_in_special_reg */

bool
operand_is_empty (jsp_operand_t op)
{
  return op.is_empty_operand ();
}

void
dumper_new_statement (void)
{
  jsp_reg_next = VM_REG_GENERAL_FIRST;
}

void
dumper_new_scope (void)
{
  JERRY_ASSERT (jsp_reg_max_for_local_var == VM_IDX_EMPTY);

  STACK_PUSH (jsp_reg_id_stack, jsp_reg_next);
  STACK_PUSH (jsp_reg_id_stack, jsp_reg_max_for_temps);

  jsp_reg_next = VM_REG_GENERAL_FIRST;
  jsp_reg_max_for_temps = jsp_reg_next;
}

void
dumper_finish_scope (void)
{
  JERRY_ASSERT (jsp_reg_max_for_local_var == VM_IDX_EMPTY);

  jsp_reg_max_for_temps = STACK_TOP (jsp_reg_id_stack);
  STACK_DROP (jsp_reg_id_stack, 1);
  jsp_reg_next = STACK_TOP (jsp_reg_id_stack);
  STACK_DROP (jsp_reg_id_stack, 1);
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
  STACK_PUSH (jsp_reg_id_stack, jsp_reg_next);
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
  jsp_reg_next = STACK_TOP (jsp_reg_id_stack);
  STACK_DROP (jsp_reg_id_stack, 1);
} /* dumper_finish_varg_code_sequence */

/**
 * Check that byte-code operand refers to 'eval' string
 *
 * @return true - if specified byte-code operand's type is literal, and value of corresponding
 *                literal is equal to LIT_MAGIC_STRING_EVAL string,
 *         false - otherwise.
 */
bool
dumper_is_eval_literal (jsp_operand_t obj) /**< byte-code operand */
{
  /*
   * FIXME: Switch to corresponding magic string
   */
  bool is_eval_lit = (obj.is_literal_operand ()
                      && lit_literal_equal_type_cstr (lit_get_literal_by_cp (obj.get_literal ()), "eval"));

  return is_eval_lit;
} /* dumper_is_eval_literal */

/**
 * Dump assignment of an array-hole simple value to a register
 *
 * @return register number, to which the value vas assigned
 */
jsp_operand_t
dump_array_hole_assignment_res (void)
{
  jsp_operand_t op = tmp_operand ();

  const vm_instr_t instr = getop_assignment (op.get_idx (),
                                             OPCODE_ARG_TYPE_SIMPLE,
                                             ECMA_SIMPLE_VALUE_ARRAY_HOLE);
  const op_meta om = create_op_meta_000 (instr);
  serializer_dump_op_meta (om);

  return op;
} /* dump_array_hole_assignment_res */

void
dump_boolean_assignment (jsp_operand_t op, bool is_true)
{
  if (op.is_literal_operand ())
  {
    const vm_instr_t instr = getop_assignment (VM_IDX_REWRITE_LITERAL_UID,
                                               OPCODE_ARG_TYPE_SIMPLE,
                                               is_true ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
    const op_meta om = create_op_meta_100 (instr, op.get_literal ());
    serializer_dump_op_meta (om);
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const vm_instr_t instr = getop_assignment (op.get_idx (),
                                               OPCODE_ARG_TYPE_SIMPLE,
                                               is_true ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
    const op_meta om = create_op_meta_000 (instr);
    serializer_dump_op_meta (om);
  }
}

jsp_operand_t
dump_boolean_assignment_res (bool is_true)
{
  jsp_operand_t op = tmp_operand ();
  dump_boolean_assignment (op, is_true);
  return op;
}

void
dump_string_assignment (jsp_operand_t op, lit_cpointer_t lit_id)
{
  if (op.is_literal_operand ())
  {
    const vm_instr_t instr = getop_assignment (VM_IDX_REWRITE_LITERAL_UID,
                                               OPCODE_ARG_TYPE_STRING,
                                               VM_IDX_REWRITE_LITERAL_UID);
    serializer_dump_op_meta (create_op_meta_101 (instr, op.get_literal (), lit_id));
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const vm_instr_t instr = getop_assignment (op.get_idx (), OPCODE_ARG_TYPE_STRING, VM_IDX_REWRITE_LITERAL_UID);
    serializer_dump_op_meta (create_op_meta_001 (instr, lit_id));
  }
}

jsp_operand_t
dump_string_assignment_res (lit_cpointer_t lit_id)
{
  jsp_operand_t op = tmp_operand ();
  dump_string_assignment (op, lit_id);
  return op;
}

void
dump_number_assignment (jsp_operand_t op, lit_cpointer_t lit_id)
{
  if (op.is_literal_operand ())
  {
    const vm_instr_t instr = getop_assignment (VM_IDX_REWRITE_LITERAL_UID,
                                               OPCODE_ARG_TYPE_NUMBER,
                                               VM_IDX_REWRITE_LITERAL_UID);
    serializer_dump_op_meta (create_op_meta_101 (instr, op.get_literal (), lit_id));
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const vm_instr_t instr = getop_assignment (op.get_idx (), OPCODE_ARG_TYPE_NUMBER, VM_IDX_REWRITE_LITERAL_UID);
    serializer_dump_op_meta (create_op_meta_001 (instr, lit_id));
  }
}

jsp_operand_t
dump_number_assignment_res (lit_cpointer_t lit_id)
{
  jsp_operand_t op = tmp_operand ();
  dump_number_assignment (op, lit_id);
  return op;
}

void
dump_regexp_assignment (jsp_operand_t op, lit_cpointer_t lit_id)
{
  if (op.is_literal_operand ())
  {
    const vm_instr_t instr = getop_assignment (VM_IDX_REWRITE_LITERAL_UID,
                                               OPCODE_ARG_TYPE_REGEXP,
                                               VM_IDX_REWRITE_LITERAL_UID);
    serializer_dump_op_meta (create_op_meta_101 (instr, op.get_literal (), lit_id));
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const vm_instr_t instr = getop_assignment (op.get_idx (), OPCODE_ARG_TYPE_REGEXP, VM_IDX_REWRITE_LITERAL_UID);
    serializer_dump_op_meta (create_op_meta_001 (instr, lit_id));
  }
}

jsp_operand_t
dump_regexp_assignment_res (lit_cpointer_t lit_id)
{
  jsp_operand_t op = tmp_operand ();
  dump_regexp_assignment (op, lit_id);
  return op;
}

void
dump_smallint_assignment (jsp_operand_t op, vm_idx_t uid)
{
  if (op.is_literal_operand ())
  {
    const vm_instr_t instr = getop_assignment (VM_IDX_REWRITE_LITERAL_UID, OPCODE_ARG_TYPE_SMALLINT, uid);
    serializer_dump_op_meta (create_op_meta_100 (instr, op.get_literal ()));
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const vm_instr_t instr = getop_assignment (op.get_idx (), OPCODE_ARG_TYPE_SMALLINT, uid);
    serializer_dump_op_meta (create_op_meta_000 (instr));
  }
}

jsp_operand_t
dump_smallint_assignment_res (vm_idx_t uid)
{
  jsp_operand_t op = tmp_operand ();
  dump_smallint_assignment (op, uid);
  return op;
}

void
dump_undefined_assignment (jsp_operand_t op)
{
  if (op.is_literal_operand ())
  {
    const vm_instr_t instr = getop_assignment (VM_IDX_REWRITE_LITERAL_UID,
                                               OPCODE_ARG_TYPE_SIMPLE,
                                               ECMA_SIMPLE_VALUE_UNDEFINED);
    serializer_dump_op_meta (create_op_meta_100 (instr, op.get_literal ()));
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const vm_instr_t instr = getop_assignment (op.get_idx (), OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_UNDEFINED);
    serializer_dump_op_meta (create_op_meta_000 (instr));
  }
}

jsp_operand_t
dump_undefined_assignment_res (void)
{
  jsp_operand_t op = tmp_operand ();
  dump_undefined_assignment (op);
  return op;
}

void
dump_null_assignment (jsp_operand_t op)
{
  if (op.is_literal_operand ())
  {
    const vm_instr_t instr = getop_assignment (VM_IDX_REWRITE_LITERAL_UID,
                                               OPCODE_ARG_TYPE_SIMPLE,
                                               ECMA_SIMPLE_VALUE_NULL);
    serializer_dump_op_meta (create_op_meta_100 (instr, op.get_literal ()));
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const vm_instr_t instr = getop_assignment (op.get_idx (),
                                               OPCODE_ARG_TYPE_SIMPLE,
                                               ECMA_SIMPLE_VALUE_NULL);
    serializer_dump_op_meta (create_op_meta_000 (instr));
  }
}

jsp_operand_t
dump_null_assignment_res (void)
{
  jsp_operand_t op = tmp_operand ();
  dump_null_assignment (op);
  return op;
}

void
dump_variable_assignment (jsp_operand_t res, jsp_operand_t var)
{
  if (res.is_literal_operand ())
  {
    if (var.is_literal_operand ())
    {
      const vm_instr_t instr = getop_assignment (VM_IDX_REWRITE_LITERAL_UID,
                                                 OPCODE_ARG_TYPE_VARIABLE,
                                                 VM_IDX_REWRITE_LITERAL_UID);
      serializer_dump_op_meta (create_op_meta_101 (instr, res.get_literal (), var.get_literal ()));
    }
    else
    {
      JERRY_ASSERT (var.is_register_operand ());

      const vm_instr_t instr = getop_assignment (VM_IDX_REWRITE_LITERAL_UID,
                                                 OPCODE_ARG_TYPE_VARIABLE,
                                                 var.get_idx ());
      serializer_dump_op_meta (create_op_meta_100 (instr, res.get_literal ()));
    }
  }
  else
  {
    JERRY_ASSERT (res.is_register_operand ());

    if (var.is_literal_operand ())
    {
      const vm_instr_t instr = getop_assignment (res.get_idx (),
                                                 OPCODE_ARG_TYPE_VARIABLE,
                                                 VM_IDX_REWRITE_LITERAL_UID);
      serializer_dump_op_meta (create_op_meta_001 (instr, var.get_literal ()));
    }
    else
    {
      JERRY_ASSERT (var.is_register_operand ());

      const vm_instr_t instr = getop_assignment (res.get_idx (),
                                                 OPCODE_ARG_TYPE_VARIABLE,
                                                 var.get_idx ());
      serializer_dump_op_meta (create_op_meta_000 (instr));
    }
  }
}

jsp_operand_t
dump_variable_assignment_res (jsp_operand_t var)
{
  jsp_operand_t op = tmp_operand ();
  dump_variable_assignment (op, var);
  return op;
}

void
dump_varg_header_for_rewrite (varg_list_type vlt, jsp_operand_t obj)
{
  STACK_PUSH (varg_headers, serializer_get_current_instr_counter ());
  switch (vlt)
  {
    case VARG_FUNC_EXPR:
    case VARG_CONSTRUCT_EXPR:
    case VARG_CALL_EXPR:
    {
      jsp_operand_t res = empty_operand ();
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
      jsp_operand_t res = empty_operand ();
      serializer_dump_op_meta (create_op_meta_for_vlt (vlt, &res, NULL));
      break;
    }
  }
}

jsp_operand_t
rewrite_varg_header_set_args_count (size_t args_count)
{
  /*
   * FIXME:
   *       Remove formal parameters / arguments number from instruction,
   *       after ecma-values collection would become extendable (issue #310).
   *       In the case, each 'varg' instruction would just append corresponding
   *       argument / formal parameter name to values collection.
   */

  op_meta om = serializer_get_op_meta (STACK_TOP (varg_headers));
  switch (om.op.op_idx)
  {
    case VM_OP_FUNC_EXPR_N:
    case VM_OP_CONSTRUCT_N:
    case VM_OP_CALL_N:
    {
      if (args_count > 255)
      {
        PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX,
                     "No more than 255 formal parameters / arguments are currently supported",
                     LIT_ITERATOR_POS_ZERO);
      }
      const jsp_operand_t res = tmp_operand ();
      om.op.data.func_expr_n.arg_list = (vm_idx_t) args_count;
      om.op.data.func_expr_n.lhs = res.get_idx ();
      serializer_rewrite_op_meta (STACK_TOP (varg_headers), om);
      STACK_DROP (varg_headers, 1);
      return res;
    }
    case VM_OP_FUNC_DECL_N:
    {
      if (args_count > 255)
      {
        PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX,
                     "No more than 255 formal parameters are currently supported",
                     LIT_ITERATOR_POS_ZERO);
      }
      om.op.data.func_decl_n.arg_list = (vm_idx_t) args_count;
      serializer_rewrite_op_meta (STACK_TOP (varg_headers), om);
      STACK_DROP (varg_headers, 1);
      return empty_operand ();
    }
    case VM_OP_ARRAY_DECL:
    case VM_OP_OBJ_DECL:
    {
      if (args_count > 65535)
      {
        PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX,
                     "No more than 65535 formal parameters are currently supported",
                     LIT_ITERATOR_POS_ZERO);
      }
      const jsp_operand_t res = tmp_operand ();
      om.op.data.obj_decl.list_1 = (vm_idx_t) (args_count >> 8);
      om.op.data.obj_decl.list_2 = (vm_idx_t) (args_count & 0xffu);
      om.op.data.obj_decl.lhs = res.get_idx ();
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
                           jsp_operand_t this_arg) /**< 'this' argument - if flags
                                                    *   include OPCODE_CALL_FLAGS_HAVE_THIS_ARG,
                                                    *   or empty operand - otherwise */
{
  if (flags & OPCODE_CALL_FLAGS_HAVE_THIS_ARG)
  {
    JERRY_ASSERT (this_arg.is_register_operand ());
    JERRY_ASSERT (!operand_is_empty (this_arg));
  }
  else
  {
    JERRY_ASSERT (operand_is_empty (this_arg));
  }

  vm_idx_t this_arg_idx = VM_IDX_EMPTY;

  if (flags & OPCODE_CALL_FLAGS_HAVE_THIS_ARG)
  {
    this_arg_idx = this_arg.get_idx ();
  }

  const vm_instr_t instr = getop_meta (OPCODE_META_TYPE_CALL_SITE_INFO, flags, this_arg_idx);

  serializer_dump_op_meta (create_op_meta_000 (instr));
} /* dump_call_additional_info */

void
dump_varg (jsp_operand_t op)
{
  if (op.is_register_operand ())
  {
    const vm_instr_t instr = getop_meta (OPCODE_META_TYPE_VARG, op.get_idx (), VM_IDX_EMPTY);
    serializer_dump_op_meta (create_op_meta_000 (instr));
  }
  else
  {
    JERRY_ASSERT (op.is_literal_operand ());

    const vm_instr_t instr = getop_meta (OPCODE_META_TYPE_VARG, VM_IDX_REWRITE_LITERAL_UID, VM_IDX_EMPTY);
    serializer_dump_op_meta (create_op_meta_010 (instr, op.get_literal ()));
  }
}

void
dump_prop_name_and_value (jsp_operand_t name, jsp_operand_t value)
{
  JERRY_ASSERT (name.is_literal_operand ());
  literal_t lit = lit_get_literal_by_cp (name.get_literal ());
  jsp_operand_t tmp;
  if (lit->get_type () == LIT_STR_T
      || lit->get_type () == LIT_MAGIC_STR_T
      || lit->get_type () == LIT_MAGIC_STR_EX_T)
  {
    tmp = dump_string_assignment_res (name.get_literal ());
  }
  else
  {
    JERRY_ASSERT (lit->get_type () == LIT_NUMBER_T);
    tmp = dump_number_assignment_res (name.get_literal ());
  }

  if (value.is_literal_operand ())
  {
    const vm_instr_t instr = getop_meta (OPCODE_META_TYPE_VARG_PROP_DATA, tmp.get_idx (), VM_IDX_REWRITE_LITERAL_UID);
    serializer_dump_op_meta (create_op_meta_001 (instr, value.get_literal ()));
  }
  else
  {
    JERRY_ASSERT (value.is_register_operand ());

    const vm_instr_t instr = getop_meta (OPCODE_META_TYPE_VARG_PROP_DATA, tmp.get_idx (), value.get_idx ());
    serializer_dump_op_meta (create_op_meta_000 (instr));
  }
}

void
dump_prop_getter_decl (jsp_operand_t name, jsp_operand_t func)
{
  JERRY_ASSERT (name.is_literal_operand ());
  JERRY_ASSERT (func.is_register_operand ());
  literal_t lit = lit_get_literal_by_cp (name.get_literal ());
  jsp_operand_t tmp;
  if (lit->get_type () == LIT_STR_T
      || lit->get_type () == LIT_MAGIC_STR_T
      || lit->get_type () == LIT_MAGIC_STR_EX_T)
  {
    tmp = dump_string_assignment_res (name.get_literal ());
  }
  else
  {
    JERRY_ASSERT (lit->get_type () == LIT_NUMBER_T);
    tmp = dump_number_assignment_res (name.get_literal ());
  }
  const vm_instr_t instr = getop_meta (OPCODE_META_TYPE_VARG_PROP_GETTER, tmp.get_idx (), func.get_idx ());
  serializer_dump_op_meta (create_op_meta_000 (instr));
}

void
dump_prop_setter_decl (jsp_operand_t name, jsp_operand_t func)
{
  JERRY_ASSERT (name.is_literal_operand ());
  JERRY_ASSERT (func.is_register_operand ());
  literal_t lit = lit_get_literal_by_cp (name.get_literal ());
  jsp_operand_t tmp;
  if (lit->get_type () == LIT_STR_T
      || lit->get_type () == LIT_MAGIC_STR_T
      || lit->get_type () == LIT_MAGIC_STR_EX_T)
  {
    tmp = dump_string_assignment_res (name.get_literal ());
  }
  else
  {
    JERRY_ASSERT (lit->get_type () == LIT_NUMBER_T);
    tmp = dump_number_assignment_res (name.get_literal ());
  }
  const vm_instr_t instr = getop_meta (OPCODE_META_TYPE_VARG_PROP_SETTER, tmp.get_idx (), func.get_idx ());
  serializer_dump_op_meta (create_op_meta_000 (instr));
}

void
dump_prop_getter (jsp_operand_t res, jsp_operand_t obj, jsp_operand_t prop)
{
  dump_triple_address (getop_prop_getter, res, obj, prop);
}

jsp_operand_t
dump_prop_getter_res (jsp_operand_t obj, jsp_operand_t prop)
{
  const jsp_operand_t res = tmp_operand ();
  dump_prop_getter (res, obj, prop);
  return res;
}

void
dump_prop_setter (jsp_operand_t res, jsp_operand_t obj, jsp_operand_t prop)
{
  dump_triple_address (getop_prop_setter, res, obj, prop);
}

void
dump_function_end_for_rewrite (void)
{
  STACK_PUSH (function_ends, serializer_get_current_instr_counter ());
  const vm_instr_t instr = getop_meta (OPCODE_META_TYPE_FUNCTION_END,
                                       VM_IDX_REWRITE_GENERAL_CASE,
                                       VM_IDX_REWRITE_GENERAL_CASE);
  serializer_dump_op_meta (create_op_meta_000 (instr));
}

void
rewrite_function_end ()
{
  vm_instr_counter_t oc;
  {
    oc = (vm_instr_counter_t) (get_diff_from (STACK_TOP (function_ends))
                               + serializer_count_instrs_in_subscopes ());
  }

  vm_idx_t id1, id2;
  split_instr_counter (oc, &id1, &id2);
  const vm_instr_t instr = getop_meta (OPCODE_META_TYPE_FUNCTION_END, id1, id2);
  serializer_rewrite_op_meta (STACK_TOP (function_ends), create_op_meta_000 (instr));
  STACK_DROP (function_ends, 1);
}

void
dump_this (jsp_operand_t op)
{
  dump_single_address (getop_this_binding, op);
}

jsp_operand_t
dump_this_res (void)
{
  const jsp_operand_t res = tmp_operand ();
  dump_this (res);
  return res;
}

void
dump_post_increment (jsp_operand_t res, jsp_operand_t obj)
{
  dump_double_address (getop_post_incr, res, obj);
}

jsp_operand_t
dump_post_increment_res (jsp_operand_t op)
{
  const jsp_operand_t res = tmp_operand ();
  dump_post_increment (res, op);
  return res;
}

void
dump_post_decrement (jsp_operand_t res, jsp_operand_t obj)
{
  dump_double_address (getop_post_decr, res, obj);
}

jsp_operand_t
dump_post_decrement_res (jsp_operand_t op)
{
  const jsp_operand_t res = tmp_operand ();
  dump_post_decrement (res, op);
  return res;
}

/**
 * Check if operand of prefix operation is correct
 */
static void
check_operand_in_prefix_operation (jsp_operand_t obj) /**< operand, which type should be Reference */
{
  const op_meta last = last_dumped_op_meta ();
  if (last.op.op_idx != VM_OP_PROP_GETTER)
  {
    if (obj.is_empty_operand ())
    {
      /*
       * FIXME:
       *       Implement correct handling of references through parser operands
       */
      PARSE_ERROR (JSP_EARLY_ERROR_REFERENCE,
                   "Invalid left-hand-side expression in prefix operation",
                   LIT_ITERATOR_POS_ZERO);
    }
  }
} /* check_operand_in_prefix_operation */

void
dump_pre_increment (jsp_operand_t res, jsp_operand_t obj)
{
  check_operand_in_prefix_operation (obj);
  dump_double_address (getop_pre_incr, res, obj);
}

jsp_operand_t
dump_pre_increment_res (jsp_operand_t op)
{
  const jsp_operand_t res = tmp_operand ();
  dump_pre_increment (res, op);
  return res;
}

void
dump_pre_decrement (jsp_operand_t res, jsp_operand_t obj)
{
  check_operand_in_prefix_operation (obj);
  dump_double_address (getop_pre_decr, res, obj);
}

jsp_operand_t
dump_pre_decrement_res (jsp_operand_t op)
{
  const jsp_operand_t res = tmp_operand ();
  dump_pre_decrement (res, op);
  return res;
}

void
dump_unary_plus (jsp_operand_t res, jsp_operand_t obj)
{
  dump_double_address (getop_unary_plus, res, obj);
}

jsp_operand_t
dump_unary_plus_res (jsp_operand_t op)
{
  const jsp_operand_t res = tmp_operand ();
  dump_unary_plus (res, op);
  return res;
}

void
dump_unary_minus (jsp_operand_t res, jsp_operand_t obj)
{
  dump_double_address (getop_unary_minus, res, obj);
}

jsp_operand_t
dump_unary_minus_res (jsp_operand_t op)
{
  const jsp_operand_t res = tmp_operand ();
  dump_unary_minus (res, op);
  return res;
}

void
dump_bitwise_not (jsp_operand_t res, jsp_operand_t obj)
{
  dump_double_address (getop_b_not, res, obj);
}

jsp_operand_t
dump_bitwise_not_res (jsp_operand_t op)
{
  const jsp_operand_t res = tmp_operand ();
  dump_bitwise_not (res, op);
  return res;
}

void
dump_logical_not (jsp_operand_t res, jsp_operand_t obj)
{
  dump_double_address (getop_logical_not, res, obj);
}

jsp_operand_t
dump_logical_not_res (jsp_operand_t op)
{
  const jsp_operand_t res = tmp_operand ();
  dump_logical_not (res, op);
  return res;
}

void
dump_delete (jsp_operand_t res, jsp_operand_t op, bool is_strict, locus loc)
{
  if (op.is_literal_operand ())
  {
    literal_t lit = lit_get_literal_by_cp (op.get_literal ());
    if (lit->get_type () == LIT_STR_T
        || lit->get_type () == LIT_MAGIC_STR_T
        || lit->get_type () == LIT_MAGIC_STR_EX_T)
    {
      jsp_early_error_check_delete (is_strict, loc);

      if (res.is_literal_operand ())
      {
        const vm_instr_t instr = getop_delete_var (VM_IDX_REWRITE_LITERAL_UID, VM_IDX_REWRITE_LITERAL_UID);
        serializer_dump_op_meta (create_op_meta_110 (instr, res.get_literal (), op.get_literal ()));
      }
      else
      {
        JERRY_ASSERT (res.is_register_operand ());

        const vm_instr_t instr = getop_delete_var (res.get_idx (), VM_IDX_REWRITE_LITERAL_UID);
        serializer_dump_op_meta (create_op_meta_010 (instr, op.get_literal ()));
      }
    }
    else if (lit->get_type ()  == LIT_NUMBER_T)
    {
      dump_boolean_assignment (res, true);
    }
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const op_meta last_op_meta = last_dumped_op_meta ();
    switch (last_op_meta.op.op_idx)
    {
      case VM_OP_PROP_GETTER:
      {
        const vm_instr_counter_t oc = (vm_instr_counter_t) (serializer_get_current_instr_counter () - 1);
        serializer_set_writing_position (oc);
        if (res.is_literal_operand ())
        {
          if (last_op_meta.op.data.prop_getter.obj == VM_IDX_REWRITE_LITERAL_UID)
          {
            if (last_op_meta.op.data.prop_getter.prop == VM_IDX_REWRITE_LITERAL_UID)
            {
              const vm_instr_t instr = getop_delete_prop (VM_IDX_REWRITE_LITERAL_UID,
                                                          VM_IDX_REWRITE_LITERAL_UID,
                                                          VM_IDX_REWRITE_LITERAL_UID);
              serializer_dump_op_meta (create_op_meta_111 (instr, res.get_literal (),
                                                           last_op_meta.lit_id[1],
                                                           last_op_meta.lit_id[2]));
            }
            else
            {
              const vm_instr_t instr = getop_delete_prop (VM_IDX_REWRITE_LITERAL_UID,
                                                          VM_IDX_REWRITE_LITERAL_UID,
                                                          last_op_meta.op.data.prop_getter.prop);
              serializer_dump_op_meta (create_op_meta_110 (instr, res.get_literal (),
                                                           last_op_meta.lit_id[1]));
            }
          }
          else
          {
            if (last_op_meta.op.data.prop_getter.prop == VM_IDX_REWRITE_LITERAL_UID)
            {
              const vm_instr_t instr = getop_delete_prop (VM_IDX_REWRITE_LITERAL_UID,
                                                          last_op_meta.op.data.prop_getter.obj,
                                                          VM_IDX_REWRITE_LITERAL_UID);
              serializer_dump_op_meta (create_op_meta_101 (instr, res.get_literal (),
                                                           last_op_meta.lit_id[2]));
            }
            else
            {
              const vm_instr_t instr = getop_delete_prop (VM_IDX_REWRITE_LITERAL_UID,
                                                          last_op_meta.op.data.prop_getter.obj,
                                                          last_op_meta.op.data.prop_getter.prop);
              serializer_dump_op_meta (create_op_meta_100 (instr, res.get_literal ()));
            }
          }
        }
        else
        {
          JERRY_ASSERT (res.is_register_operand ());

          if (last_op_meta.op.data.prop_getter.obj == VM_IDX_REWRITE_LITERAL_UID)
          {
            if (last_op_meta.op.data.prop_getter.prop == VM_IDX_REWRITE_LITERAL_UID)
            {
              const vm_instr_t instr = getop_delete_prop (res.get_idx (),
                                                          VM_IDX_REWRITE_LITERAL_UID,
                                                          VM_IDX_REWRITE_LITERAL_UID);
              serializer_dump_op_meta (create_op_meta_011 (instr, last_op_meta.lit_id[1],
                                                           last_op_meta.lit_id[2]));
            }
            else
            {
              const vm_instr_t instr = getop_delete_prop (res.get_idx (),
                                                          VM_IDX_REWRITE_LITERAL_UID,
                                                          last_op_meta.op.data.prop_getter.prop);
              serializer_dump_op_meta (create_op_meta_010 (instr, last_op_meta.lit_id[1]));
            }
          }
          else
          {
            if (last_op_meta.op.data.prop_getter.prop == VM_IDX_REWRITE_LITERAL_UID)
            {
              const vm_instr_t instr = getop_delete_prop (res.get_idx (),
                                                          last_op_meta.op.data.prop_getter.obj,
                                                          VM_IDX_REWRITE_LITERAL_UID);
              serializer_dump_op_meta (create_op_meta_001 (instr, last_op_meta.lit_id[2]));
            }
            else
            {
              const vm_instr_t instr = getop_delete_prop (res.get_idx (),
                                                          last_op_meta.op.data.prop_getter.obj,
                                                          last_op_meta.op.data.prop_getter.prop);
              serializer_dump_op_meta (create_op_meta_000 (instr));
            }
          }
        }
        break;
      }
      default:
      {
        dump_boolean_assignment (res, true);
      }
    }
  }
}

jsp_operand_t
dump_delete_res (jsp_operand_t op, bool is_strict, locus loc)
{
  const jsp_operand_t res = tmp_operand ();
  dump_delete (res, op, is_strict, loc);
  return res;
}

void
dump_typeof (jsp_operand_t res, jsp_operand_t op)
{
  dump_double_address (getop_typeof, res, op);
}

jsp_operand_t
dump_typeof_res (jsp_operand_t op)
{
  const jsp_operand_t res = tmp_operand ();
  dump_typeof (res, op);
  return res;
}

void
dump_multiplication (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_multiplication, res, lhs, rhs);
}

jsp_operand_t
dump_multiplication_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_multiplication (res, lhs, rhs);
  return res;
}

void
dump_division (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_division, res, lhs, rhs);
}

jsp_operand_t
dump_division_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_division (res, lhs, rhs);
  return res;
}

void
dump_remainder (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_remainder, res, lhs, rhs);
}

jsp_operand_t
dump_remainder_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_remainder (res, lhs, rhs);
  return res;
}

void
dump_addition (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_addition, res, lhs, rhs);
}

jsp_operand_t
dump_addition_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_addition (res, lhs, rhs);
  return res;
}

void
dump_substraction (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_substraction, res, lhs, rhs);
}

jsp_operand_t
dump_substraction_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_substraction (res, lhs, rhs);
  return res;
}

void
dump_left_shift (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_b_shift_left, res, lhs, rhs);
}

jsp_operand_t
dump_left_shift_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_left_shift (res, lhs, rhs);
  return res;
}

void
dump_right_shift (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_b_shift_right, res, lhs, rhs);
}

jsp_operand_t
dump_right_shift_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_right_shift (res, lhs, rhs);
  return res;
}

void
dump_right_shift_ex (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_b_shift_uright, res, lhs, rhs);
}

jsp_operand_t
dump_right_shift_ex_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_right_shift_ex (res, lhs, rhs);
  return res;
}

void
dump_less_than (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_less_than, res, lhs, rhs);
}

jsp_operand_t
dump_less_than_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_less_than (res, lhs, rhs);
  return res;
}

void
dump_greater_than (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_greater_than, res, lhs, rhs);
}

jsp_operand_t
dump_greater_than_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_greater_than (res, lhs, rhs);
  return res;
}

void
dump_less_or_equal_than (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_less_or_equal_than, res, lhs, rhs);
}

jsp_operand_t
dump_less_or_equal_than_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_less_or_equal_than (res, lhs, rhs);
  return res;
}

void
dump_greater_or_equal_than (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_greater_or_equal_than, res, lhs, rhs);
}

jsp_operand_t
dump_greater_or_equal_than_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_greater_or_equal_than (res, lhs, rhs);
  return res;
}

void
dump_instanceof (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_instanceof, res, lhs, rhs);
}

jsp_operand_t
dump_instanceof_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_instanceof (res, lhs, rhs);
  return res;
}

void
dump_in (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_in, res, lhs, rhs);
}

jsp_operand_t
dump_in_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_in (res, lhs, rhs);
  return res;
}

void
dump_equal_value (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_equal_value, res, lhs, rhs);
}

jsp_operand_t
dump_equal_value_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_equal_value (res, lhs, rhs);
  return res;
}

void
dump_not_equal_value (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_not_equal_value, res, lhs, rhs);
}

jsp_operand_t
dump_not_equal_value_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_not_equal_value (res, lhs, rhs);
  return res;
}

void
dump_equal_value_type (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_equal_value_type, res, lhs, rhs);
}

jsp_operand_t
dump_equal_value_type_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_equal_value_type (res, lhs, rhs);
  return res;
}

void
dump_not_equal_value_type (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_not_equal_value_type, res, lhs, rhs);
}

jsp_operand_t
dump_not_equal_value_type_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_not_equal_value_type (res, lhs, rhs);
  return res;
}

void
dump_bitwise_and (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_b_and, res, lhs, rhs);
}

jsp_operand_t
dump_bitwise_and_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_bitwise_and (res, lhs, rhs);
  return res;
}

void
dump_bitwise_xor (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_b_xor, res, lhs, rhs);
}

jsp_operand_t
dump_bitwise_xor_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_bitwise_xor (res, lhs, rhs);
  return res;
}

void
dump_bitwise_or (jsp_operand_t res, jsp_operand_t lhs, jsp_operand_t rhs)
{
  dump_triple_address (getop_b_or, res, lhs, rhs);
}

jsp_operand_t
dump_bitwise_or_res (jsp_operand_t lhs, jsp_operand_t rhs)
{
  const jsp_operand_t res = tmp_operand ();
  dump_bitwise_or (res, lhs, rhs);
  return res;
}

void
start_dumping_logical_and_checks (void)
{
  STACK_PUSH (U8, (uint8_t) STACK_SIZE (logical_and_checks));
}

void
dump_logical_and_check_for_rewrite (jsp_operand_t op)
{
  STACK_PUSH (logical_and_checks, serializer_get_current_instr_counter ());

  if (op.is_literal_operand ())
  {
    const vm_instr_t instr = getop_is_false_jmp_down (VM_IDX_REWRITE_LITERAL_UID,
                                                      VM_IDX_REWRITE_GENERAL_CASE,
                                                      VM_IDX_REWRITE_GENERAL_CASE);
    serializer_dump_op_meta (create_op_meta_100 (instr, op.get_literal ()));
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const vm_instr_t instr = getop_is_false_jmp_down (op.get_idx (),
                                                      VM_IDX_REWRITE_GENERAL_CASE,
                                                      VM_IDX_REWRITE_GENERAL_CASE);
    serializer_dump_op_meta (create_op_meta_000 (instr));
  }
}

void
rewrite_logical_and_checks (void)
{
  for (uint8_t i = STACK_TOP (U8); i < STACK_SIZE (logical_and_checks); i++)
  {
    op_meta jmp_op_meta = serializer_get_op_meta (STACK_ELEMENT (logical_and_checks, i));
    JERRY_ASSERT (jmp_op_meta.op.op_idx == VM_OP_IS_FALSE_JMP_DOWN);
    vm_idx_t id1, id2;
    split_instr_counter (get_diff_from (STACK_ELEMENT (logical_and_checks, i)), &id1, &id2);
    jmp_op_meta.op.data.is_false_jmp_down.oc_idx_1 = id1;
    jmp_op_meta.op.data.is_false_jmp_down.oc_idx_2 = id2;
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
dump_logical_or_check_for_rewrite (jsp_operand_t op)
{
  STACK_PUSH (logical_or_checks, serializer_get_current_instr_counter ());

  if (op.is_literal_operand ())
  {
    const vm_instr_t instr = getop_is_true_jmp_down (VM_IDX_REWRITE_LITERAL_UID,
                                                     VM_IDX_REWRITE_GENERAL_CASE,
                                                     VM_IDX_REWRITE_GENERAL_CASE);
    serializer_dump_op_meta (create_op_meta_100 (instr, op.get_literal ()));
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const vm_instr_t instr = getop_is_true_jmp_down (op.get_idx (),
                                                     VM_IDX_REWRITE_GENERAL_CASE,
                                                     VM_IDX_REWRITE_GENERAL_CASE);
    serializer_dump_op_meta (create_op_meta_000 (instr));
  }
}

void
rewrite_logical_or_checks (void)
{
  for (uint8_t i = STACK_TOP (U8); i < STACK_SIZE (logical_or_checks); i++)
  {
    op_meta jmp_op_meta = serializer_get_op_meta (STACK_ELEMENT (logical_or_checks, i));
    JERRY_ASSERT (jmp_op_meta.op.op_idx == VM_OP_IS_TRUE_JMP_DOWN);
    vm_idx_t id1, id2;
    split_instr_counter (get_diff_from (STACK_ELEMENT (logical_or_checks, i)), &id1, &id2);
    jmp_op_meta.op.data.is_true_jmp_down.oc_idx_1 = id1;
    jmp_op_meta.op.data.is_true_jmp_down.oc_idx_2 = id2;
    serializer_rewrite_op_meta (STACK_ELEMENT (logical_or_checks, i), jmp_op_meta);
  }
  STACK_DROP (logical_or_checks, STACK_SIZE (logical_or_checks) - STACK_TOP (U8));
  STACK_DROP (U8, 1);
}

void
dump_conditional_check_for_rewrite (jsp_operand_t op)
{
  STACK_PUSH (conditional_checks, serializer_get_current_instr_counter ());
  if (op.is_literal_operand ())
  {
    const vm_instr_t instr = getop_is_false_jmp_down (VM_IDX_REWRITE_LITERAL_UID,
                                                      VM_IDX_REWRITE_GENERAL_CASE,
                                                      VM_IDX_REWRITE_GENERAL_CASE);
    serializer_dump_op_meta (create_op_meta_100 (instr, op.get_literal ()));
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const vm_instr_t instr = getop_is_false_jmp_down (op.get_idx (),
                                                      VM_IDX_REWRITE_GENERAL_CASE,
                                                      VM_IDX_REWRITE_GENERAL_CASE);
    serializer_dump_op_meta (create_op_meta_000 (instr));
  }
}

void
rewrite_conditional_check (void)
{
  op_meta jmp_op_meta = serializer_get_op_meta (STACK_TOP (conditional_checks));
  JERRY_ASSERT (jmp_op_meta.op.op_idx == VM_OP_IS_FALSE_JMP_DOWN);
  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (STACK_TOP (conditional_checks)), &id1, &id2);
  jmp_op_meta.op.data.is_false_jmp_down.oc_idx_1 = id1;
  jmp_op_meta.op.data.is_false_jmp_down.oc_idx_2 = id2;
  serializer_rewrite_op_meta (STACK_TOP (conditional_checks), jmp_op_meta);
  STACK_DROP (conditional_checks, 1);
}

void
dump_jump_to_end_for_rewrite (void)
{
  STACK_PUSH (jumps_to_end, serializer_get_current_instr_counter ());
  const vm_instr_t instr = getop_jmp_down (VM_IDX_REWRITE_GENERAL_CASE, VM_IDX_REWRITE_GENERAL_CASE);
  serializer_dump_op_meta (create_op_meta_000 (instr));
}

void
rewrite_jump_to_end (void)
{
  op_meta jmp_op_meta = serializer_get_op_meta (STACK_TOP (jumps_to_end));
  JERRY_ASSERT (jmp_op_meta.op.op_idx == VM_OP_JMP_DOWN);
  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (STACK_TOP (jumps_to_end)), &id1, &id2);
  jmp_op_meta.op.data.jmp_down.oc_idx_1 = id1;
  jmp_op_meta.op.data.jmp_down.oc_idx_2 = id2;
  serializer_rewrite_op_meta (STACK_TOP (jumps_to_end), jmp_op_meta);
  STACK_DROP (jumps_to_end, 1);
}

void
start_dumping_assignment_expression (void)
{
  const op_meta last = last_dumped_op_meta ();
  if (last.op.op_idx == VM_OP_PROP_GETTER)
  {
    serializer_set_writing_position ((vm_instr_counter_t) (serializer_get_current_instr_counter () - 1));
  }
  STACK_PUSH (prop_getters, last);
}

jsp_operand_t
dump_prop_setter_or_variable_assignment_res (jsp_operand_t res, jsp_operand_t op)
{
  const op_meta last = STACK_TOP (prop_getters);
  if (last.op.op_idx == VM_OP_PROP_GETTER)
  {
    dump_prop_setter_op_meta (last, op);
  }
  else
  {
    if (res.is_register_operand ())
    {
      /*
       * FIXME:
       *       Implement correct handling of references through parser operands
       */
      PARSE_ERROR (JSP_EARLY_ERROR_REFERENCE, "Invalid left-hand-side expression", LIT_ITERATOR_POS_ZERO);
    }
    dump_variable_assignment (res, op);
  }
  STACK_DROP (prop_getters, 1);
  return op;
}

jsp_operand_t
dump_prop_setter_or_addition_res (jsp_operand_t res, jsp_operand_t op)
{
  return dump_prop_setter_or_triple_address_res (dump_addition, res, op);
}

jsp_operand_t
dump_prop_setter_or_multiplication_res (jsp_operand_t res, jsp_operand_t op)
{
  return dump_prop_setter_or_triple_address_res (dump_multiplication, res, op);
}

jsp_operand_t
dump_prop_setter_or_division_res (jsp_operand_t res, jsp_operand_t op)
{
  return dump_prop_setter_or_triple_address_res (dump_division, res, op);
}

jsp_operand_t
dump_prop_setter_or_remainder_res (jsp_operand_t res, jsp_operand_t op)
{
  return dump_prop_setter_or_triple_address_res (dump_remainder, res, op);
}

jsp_operand_t
dump_prop_setter_or_substraction_res (jsp_operand_t res, jsp_operand_t op)
{
  return dump_prop_setter_or_triple_address_res (dump_substraction, res, op);
}

jsp_operand_t
dump_prop_setter_or_left_shift_res (jsp_operand_t res, jsp_operand_t op)
{
  return dump_prop_setter_or_triple_address_res (dump_left_shift, res, op);
}

jsp_operand_t
dump_prop_setter_or_right_shift_res (jsp_operand_t res, jsp_operand_t op)
{
  return dump_prop_setter_or_triple_address_res (dump_right_shift, res, op);
}

jsp_operand_t
dump_prop_setter_or_right_shift_ex_res (jsp_operand_t res, jsp_operand_t op)
{
  return dump_prop_setter_or_triple_address_res (dump_right_shift_ex, res, op);
}

jsp_operand_t
dump_prop_setter_or_bitwise_and_res (jsp_operand_t res, jsp_operand_t op)
{
  return dump_prop_setter_or_triple_address_res (dump_bitwise_and, res, op);
}

jsp_operand_t
dump_prop_setter_or_bitwise_xor_res (jsp_operand_t res, jsp_operand_t op)
{
  return dump_prop_setter_or_triple_address_res (dump_bitwise_xor, res, op);
}

jsp_operand_t
dump_prop_setter_or_bitwise_or_res (jsp_operand_t res, jsp_operand_t op)
{
  return dump_prop_setter_or_triple_address_res (dump_bitwise_or, res, op);
}

void
dumper_set_next_interation_target (void)
{
  STACK_PUSH (next_iterations, serializer_get_current_instr_counter ());
}

void
dump_continue_iterations_check (jsp_operand_t op)
{
  const vm_instr_counter_t next_iteration_target_diff = (vm_instr_counter_t) (serializer_get_current_instr_counter ()
                                                                          - STACK_TOP (next_iterations));
  vm_idx_t id1, id2;
  split_instr_counter (next_iteration_target_diff, &id1, &id2);
  if (operand_is_empty (op))
  {
    const vm_instr_t instr = getop_jmp_up (id1, id2);
    serializer_dump_op_meta (create_op_meta_000 (instr));
  }
  else
  {
    if (op.is_literal_operand ())
    {
      const vm_instr_t instr = getop_is_true_jmp_up (VM_IDX_REWRITE_LITERAL_UID, id1, id2);
      serializer_dump_op_meta (create_op_meta_100 (instr, op.get_literal ()));
    }
    else
    {
      JERRY_ASSERT (op.is_register_operand ());

      const vm_instr_t instr = getop_is_true_jmp_up (op.get_idx (), id1, id2);
      serializer_dump_op_meta (create_op_meta_000 (instr));
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
vm_instr_counter_t
dump_simple_or_nested_jump_for_rewrite (bool is_simple_jump, /**< flag indicating, whether simple jump
                                                              *   or 'jmp_break_continue' template should be dumped */
                                        vm_instr_counter_t next_jump_for_tgt_oc) /**< instr counter of next
                                                                                  *   template targetted to
                                                                                  *   the same target - if any,
                                                                                  *   or MAX_OPCODES - otherwise */
{
  vm_idx_t id1, id2;
  split_instr_counter (next_jump_for_tgt_oc, &id1, &id2);

  vm_instr_t instr;
  if (is_simple_jump)
  {
    instr = getop_jmp_down (id1, id2);
  }
  else
  {
    instr = getop_jmp_break_continue (id1, id2);
  }

  vm_instr_counter_t ret = serializer_get_current_instr_counter ();

  serializer_dump_op_meta (create_op_meta_000 (instr));

  return ret;
} /* dump_simple_or_nested_jump_for_rewrite */

/**
 * Write jump target position into previously dumped template of jump (simple or nested) instruction
 *
 * @return instr counter value that was encoded in the jump before rewrite
 */
vm_instr_counter_t
rewrite_simple_or_nested_jump_and_get_next (vm_instr_counter_t jump_oc, /**< position of jump to rewrite */
                                            vm_instr_counter_t target_oc) /**< the jump's target */
{
  op_meta jump_op_meta = serializer_get_op_meta (jump_oc);

  bool is_simple_jump = (jump_op_meta.op.op_idx == VM_OP_JMP_DOWN);

  JERRY_ASSERT (is_simple_jump
                || (jump_op_meta.op.op_idx == VM_OP_JMP_BREAK_CONTINUE));

  vm_idx_t id1, id2, id1_prev, id2_prev;
  split_instr_counter ((vm_instr_counter_t) (target_oc - jump_oc), &id1, &id2);

  if (is_simple_jump)
  {
    id1_prev = jump_op_meta.op.data.jmp_down.oc_idx_1;
    id2_prev = jump_op_meta.op.data.jmp_down.oc_idx_2;

    jump_op_meta.op.data.jmp_down.oc_idx_1 = id1;
    jump_op_meta.op.data.jmp_down.oc_idx_2 = id2;
  }
  else
  {
    JERRY_ASSERT (jump_op_meta.op.op_idx == VM_OP_JMP_BREAK_CONTINUE);

    id1_prev = jump_op_meta.op.data.jmp_break_continue.oc_idx_1;
    id2_prev = jump_op_meta.op.data.jmp_break_continue.oc_idx_2;

    jump_op_meta.op.data.jmp_break_continue.oc_idx_1 = id1;
    jump_op_meta.op.data.jmp_break_continue.oc_idx_2 = id2;
  }

  serializer_rewrite_op_meta (jump_oc, jump_op_meta);

  return vm_calc_instr_counter_from_idx_idx (id1_prev, id2_prev);
} /* rewrite_simple_or_nested_jump_get_next */

void
start_dumping_case_clauses (void)
{
  STACK_PUSH (U8, (uint8_t) STACK_SIZE (case_clauses));
  STACK_PUSH (U8, (uint8_t) STACK_SIZE (case_clauses));
}

void
dump_case_clause_check_for_rewrite (jsp_operand_t switch_expr, jsp_operand_t case_expr)
{
  const jsp_operand_t res = tmp_operand ();
  dump_triple_address (getop_equal_value_type, res, switch_expr, case_expr);
  STACK_PUSH (case_clauses, serializer_get_current_instr_counter ());
  const vm_instr_t instr = getop_is_true_jmp_down (res.get_idx (),
                                                   VM_IDX_REWRITE_GENERAL_CASE,
                                                   VM_IDX_REWRITE_GENERAL_CASE);
  serializer_dump_op_meta (create_op_meta_000 (instr));
}

void
dump_default_clause_check_for_rewrite (void)
{
  STACK_PUSH (case_clauses, serializer_get_current_instr_counter ());
  const vm_instr_t instr = getop_jmp_down (VM_IDX_REWRITE_GENERAL_CASE, VM_IDX_REWRITE_GENERAL_CASE);
  serializer_dump_op_meta (create_op_meta_000 (instr));
}

void
rewrite_case_clause (void)
{
  const vm_instr_counter_t jmp_oc = STACK_ELEMENT (case_clauses, STACK_HEAD (U8, 2));
  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (jmp_oc), &id1, &id2);
  op_meta jmp_op_meta = serializer_get_op_meta (jmp_oc);
  JERRY_ASSERT (jmp_op_meta.op.op_idx == VM_OP_IS_TRUE_JMP_DOWN);
  jmp_op_meta.op.data.is_true_jmp_down.oc_idx_1 = id1;
  jmp_op_meta.op.data.is_true_jmp_down.oc_idx_2 = id2;
  serializer_rewrite_op_meta (jmp_oc, jmp_op_meta);
  STACK_INCR_HEAD (U8, 2);
}

void
rewrite_default_clause (void)
{
  const vm_instr_counter_t jmp_oc = STACK_TOP (case_clauses);
  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (jmp_oc), &id1, &id2);
  op_meta jmp_op_meta = serializer_get_op_meta (jmp_oc);
  JERRY_ASSERT (jmp_op_meta.op.op_idx == VM_OP_JMP_DOWN);
  jmp_op_meta.op.data.jmp_down.oc_idx_1 = id1;
  jmp_op_meta.op.data.jmp_down.oc_idx_2 = id2;
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
vm_instr_counter_t
dump_with_for_rewrite (jsp_operand_t op) /**< jsp_operand_t - result of evaluating Expression
                                          *   in WithStatement */
{
  vm_instr_counter_t oc = serializer_get_current_instr_counter ();

  if (op.is_literal_operand ())
  {
    const vm_instr_t instr = getop_with (VM_IDX_REWRITE_LITERAL_UID,
                                         VM_IDX_REWRITE_GENERAL_CASE,
                                         VM_IDX_REWRITE_GENERAL_CASE);
    serializer_dump_op_meta (create_op_meta_100 (instr, op.get_literal ()));
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const vm_instr_t instr = getop_with (op.get_idx (), VM_IDX_REWRITE_GENERAL_CASE, VM_IDX_REWRITE_GENERAL_CASE);
    serializer_dump_op_meta (create_op_meta_000 (instr));
  }

  return oc;
} /* dump_with_for_rewrite */

/**
 * Write position of 'with' block's end to specified 'with' instruction template,
 * dumped earlier (see also: dump_with_for_rewrite).
 */
void
rewrite_with (vm_instr_counter_t oc) /**< instr counter of the instruction template */
{
  op_meta with_op_meta = serializer_get_op_meta (oc);

  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (oc), &id1, &id2);
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
  const vm_instr_t instr = getop_meta (OPCODE_META_TYPE_END_WITH, VM_IDX_EMPTY, VM_IDX_EMPTY);
  serializer_dump_op_meta (create_op_meta_000 (instr));
} /* dump_with_end */

/**
 * Dump template of 'for_in' instruction.
 *
 * Note:
 *      the instruction's flags field is written later (see also: rewrite_for_in).
 *
 * @return position of dumped instruction
 */
vm_instr_counter_t
dump_for_in_for_rewrite (jsp_operand_t op) /**< jsp_operand_t - result of evaluating Expression
                                      *   in for-in statement */
{
  vm_instr_counter_t oc = serializer_get_current_instr_counter ();

  if (op.is_literal_operand ())
  {
    const vm_instr_t instr = getop_for_in (VM_IDX_REWRITE_LITERAL_UID,
                                           VM_IDX_REWRITE_GENERAL_CASE,
                                           VM_IDX_REWRITE_GENERAL_CASE);
    serializer_dump_op_meta (create_op_meta_100 (instr, op.get_literal ()));
  }
  else
  {
    JERRY_ASSERT (op.is_register_operand ());

    const vm_instr_t instr = getop_for_in (op.get_idx (), VM_IDX_REWRITE_GENERAL_CASE, VM_IDX_REWRITE_GENERAL_CASE);
    serializer_dump_op_meta (create_op_meta_000 (instr));
  }

  return oc;
} /* dump_for_in_for_rewrite */

/**
 * Write position of 'for_in' block's end to specified 'for_in' instruction template,
 * dumped earlier (see also: dump_for_in_for_rewrite).
 */
void
rewrite_for_in (vm_instr_counter_t oc) /**< instr counter of the instruction template */
{
  op_meta for_in_op_meta = serializer_get_op_meta (oc);

  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (oc), &id1, &id2);
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
  const vm_instr_t instr = getop_meta (OPCODE_META_TYPE_END_FOR_IN, VM_IDX_EMPTY, VM_IDX_EMPTY);
  serializer_dump_op_meta (create_op_meta_000 (instr));
} /* dump_for_in_end */

void
dump_try_for_rewrite (void)
{
  STACK_PUSH (tries, serializer_get_current_instr_counter ());
  const vm_instr_t instr = getop_try_block (VM_IDX_REWRITE_GENERAL_CASE, VM_IDX_REWRITE_GENERAL_CASE);
  serializer_dump_op_meta (create_op_meta_000 (instr));
}

void
rewrite_try (void)
{
  op_meta try_op_meta = serializer_get_op_meta (STACK_TOP (tries));
  JERRY_ASSERT (try_op_meta.op.op_idx == VM_OP_TRY_BLOCK);
  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (STACK_TOP (tries)), &id1, &id2);
  try_op_meta.op.data.try_block.oc_idx_1 = id1;
  try_op_meta.op.data.try_block.oc_idx_2 = id2;
  serializer_rewrite_op_meta (STACK_TOP (tries), try_op_meta);
  STACK_DROP (tries, 1);
}

void
dump_catch_for_rewrite (jsp_operand_t op)
{
  JERRY_ASSERT (op.is_literal_operand ());
  STACK_PUSH (catches, serializer_get_current_instr_counter ());
  vm_instr_t instr = getop_meta (OPCODE_META_TYPE_CATCH, VM_IDX_REWRITE_GENERAL_CASE, VM_IDX_REWRITE_GENERAL_CASE);
  serializer_dump_op_meta (create_op_meta_000 (instr));
  instr = getop_meta (OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER, VM_IDX_REWRITE_LITERAL_UID, VM_IDX_EMPTY);
  serializer_dump_op_meta (create_op_meta_010 (instr, op.get_literal ()));
}

void
rewrite_catch (void)
{
  op_meta catch_op_meta = serializer_get_op_meta (STACK_TOP (catches));
  JERRY_ASSERT (catch_op_meta.op.op_idx == VM_OP_META
                && catch_op_meta.op.data.meta.type == OPCODE_META_TYPE_CATCH);
  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (STACK_TOP (catches)), &id1, &id2);
  catch_op_meta.op.data.meta.data_1 = id1;
  catch_op_meta.op.data.meta.data_2 = id2;
  serializer_rewrite_op_meta (STACK_TOP (catches), catch_op_meta);
  STACK_DROP (catches, 1);
}

void
dump_finally_for_rewrite (void)
{
  STACK_PUSH (finallies, serializer_get_current_instr_counter ());
  const vm_instr_t instr = getop_meta (OPCODE_META_TYPE_FINALLY,
                                       VM_IDX_REWRITE_GENERAL_CASE,
                                       VM_IDX_REWRITE_GENERAL_CASE);
  serializer_dump_op_meta (create_op_meta_000 (instr));
}

void
rewrite_finally (void)
{
  op_meta finally_op_meta = serializer_get_op_meta (STACK_TOP (finallies));
  JERRY_ASSERT (finally_op_meta.op.op_idx == VM_OP_META
                && finally_op_meta.op.data.meta.type == OPCODE_META_TYPE_FINALLY);
  vm_idx_t id1, id2;
  split_instr_counter (get_diff_from (STACK_TOP (finallies)), &id1, &id2);
  finally_op_meta.op.data.meta.data_1 = id1;
  finally_op_meta.op.data.meta.data_2 = id2;
  serializer_rewrite_op_meta (STACK_TOP (finallies), finally_op_meta);
  STACK_DROP (finallies, 1);
}

void
dump_end_try_catch_finally (void)
{
  const vm_instr_t instr = getop_meta (OPCODE_META_TYPE_END_TRY_CATCH_FINALLY,
                                       VM_IDX_EMPTY, VM_IDX_EMPTY);
  serializer_dump_op_meta (create_op_meta_000 (instr));
}

void
dump_throw (jsp_operand_t op)
{
  dump_single_address (getop_throw_value, op);
}

/**
 * Checks if variable is already declared
 *
 * @return true if variable declaration already exists
 *         false otherwise
 */
bool
dumper_variable_declaration_exists (lit_cpointer_t lit_id) /**< literal which holds variable's name */
{
  vm_instr_counter_t var_decls_count = (vm_instr_counter_t) serializer_get_current_var_decls_counter ();
  for (vm_instr_counter_t oc = (vm_instr_counter_t) (0); oc < var_decls_count; oc++)
  {
    const op_meta var_decl_op_meta = serializer_get_var_decl (oc);
    if (var_decl_op_meta.lit_id[0].packed_value == lit_id.packed_value)
    {
      return true;
    }
  }
  return false;
} /* dumper_variable_declaration_exists */

/**
 * Dump instruction designating variable declaration
 */
void
dump_variable_declaration (lit_cpointer_t lit_id) /**< literal which holds variable's name */
{
  const vm_instr_t instr = getop_var_decl (VM_IDX_REWRITE_LITERAL_UID);
  serializer_dump_var_decl (create_op_meta_100 (instr, lit_id));
} /* dump_variable_declaration */

/**
 * Dump template of 'meta' instruction for scope's code flags.
 *
 * Note:
 *      the instruction's flags field is written later (see also: rewrite_scope_code_flags).
 *
 * @return position of dumped instruction
 */
vm_instr_counter_t
dump_scope_code_flags_for_rewrite (void)
{
  vm_instr_counter_t oc = serializer_get_current_instr_counter ();

  const vm_instr_t instr = getop_meta (OPCODE_META_TYPE_SCOPE_CODE_FLAGS, VM_IDX_REWRITE_GENERAL_CASE, VM_IDX_EMPTY);
  serializer_dump_op_meta (create_op_meta_000 (instr));

  return oc;
} /* dump_scope_code_flags_for_rewrite */

/**
 * Write scope's code flags to specified 'meta' instruction template,
 * dumped earlier (see also: dump_scope_code_flags_for_rewrite).
 */
void
rewrite_scope_code_flags (vm_instr_counter_t scope_code_flags_oc, /**< position of instruction to rewrite */
                          opcode_scope_code_flags_t scope_flags) /**< scope's code properties flags set */
{
  JERRY_ASSERT ((vm_idx_t) scope_flags == scope_flags);

  op_meta opm = serializer_get_op_meta (scope_code_flags_oc);
  JERRY_ASSERT (opm.op.op_idx == VM_OP_META);
  JERRY_ASSERT (opm.op.data.meta.type == OPCODE_META_TYPE_SCOPE_CODE_FLAGS);
  JERRY_ASSERT (opm.op.data.meta.data_1 == VM_IDX_REWRITE_GENERAL_CASE);
  JERRY_ASSERT (opm.op.data.meta.data_2 == VM_IDX_EMPTY);

  opm.op.data.meta.data_1 = (vm_idx_t) scope_flags;
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
  STACK_PUSH (reg_var_decls, serializer_get_current_instr_counter ());
  serializer_dump_op_meta (create_op_meta_000 (getop_reg_var_decl (VM_REG_FIRST,
                                                                   VM_IDX_REWRITE_GENERAL_CASE,
                                                                   VM_IDX_REWRITE_GENERAL_CASE)));
}

void
rewrite_reg_var_decl (void)
{
  vm_instr_counter_t reg_var_decl_oc = STACK_TOP (reg_var_decls);
  op_meta opm = serializer_get_op_meta (reg_var_decl_oc);
  JERRY_ASSERT (opm.op.op_idx == VM_OP_REG_VAR_DECL);

  if (jsp_reg_max_for_local_var != VM_IDX_EMPTY)
  {
    JERRY_ASSERT (jsp_reg_max_for_local_var >= jsp_reg_max_for_temps);
    opm.op.data.reg_var_decl.local_var_regs_num = (vm_idx_t) (jsp_reg_max_for_local_var - jsp_reg_max_for_temps);
    opm.op.data.reg_var_decl.max = jsp_reg_max_for_local_var;

    jsp_reg_max_for_local_var = VM_IDX_EMPTY;
  }
  else
  {
    opm.op.data.reg_var_decl.max = jsp_reg_max_for_temps;
    opm.op.data.reg_var_decl.local_var_regs_num = 0;
  }
  serializer_rewrite_op_meta (reg_var_decl_oc, opm);
  STACK_DROP (reg_var_decls, 1);
}

void
dump_retval (jsp_operand_t op)
{
  dump_single_address (getop_retval, op);
}

void
dumper_init (void)
{
  jsp_reg_next = VM_REG_GENERAL_FIRST;
  jsp_reg_max_for_temps = VM_REG_GENERAL_FIRST;
  jsp_reg_max_for_local_var = VM_IDX_EMPTY;

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
  STACK_INIT (jsp_reg_id_stack);
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
  STACK_FREE (jsp_reg_id_stack);
  STACK_FREE (reg_var_decls);
}

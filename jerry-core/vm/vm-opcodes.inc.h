/* Copyright 2015 Samsung Electronics Co., Ltd.
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

/*
 * List of VM opcodes
 */

#ifndef VM_OP_0
# define VM_OP_0(opcode_name, opcode_name_uppercase)
#endif /* !VM_OP_0 */

#ifndef VM_OP_1
# define VM_OP_1(opcode_name, opcode_name_uppercase, arg1, arg1_type)
#endif /* !VM_OP_1 */

#ifndef VM_OP_2
# define VM_OP_2(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type)
#endif /* !VM_OP_2 */

#ifndef VM_OP_3
# define VM_OP_3(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type, arg3, arg3_type)
#endif /* !VM_OP_3 */

VM_OP_3 (call_n,                CALL_N,
         lhs,                   VM_OP_ARG_TYPE_VARIABLE,
         function_var_idx,      VM_OP_ARG_TYPE_VARIABLE,
         arg_list,              VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_3 (construct_n,           CONSTRUCT_N,
         lhs,                   VM_OP_ARG_TYPE_VARIABLE,
         name_lit_idx,          VM_OP_ARG_TYPE_VARIABLE,
         arg_list,              VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_2 (func_decl_n,           FUNC_DECL_N,
         name_lit_idx,          VM_OP_ARG_TYPE_STRING,
         arg_list,              VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_3 (func_expr_n,           FUNC_EXPR_N,
         lhs,                   VM_OP_ARG_TYPE_VARIABLE,
         name_lit_idx,          VM_OP_ARG_TYPE_STRING |
                                VM_OP_ARG_TYPE_EMPTY,
         arg_list,              VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_3 (func_expr_ref,         FUNC_EXPR_REF,
         lhs,                   VM_OP_ARG_TYPE_VARIABLE,
         idx1,                  VM_OP_ARG_TYPE_INTEGER_CONST,
         idx2,                  VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_1 (retval,                RETVAL,
         ret_value,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_0 (ret,                   RET)

VM_OP_3 (array_decl,            ARRAY_DECL,
         lhs,                   VM_OP_ARG_TYPE_VARIABLE,
         list_1,                VM_OP_ARG_TYPE_INTEGER_CONST,
         list_2,                VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_3 (obj_decl,              OBJ_DECL,
         lhs,                   VM_OP_ARG_TYPE_VARIABLE,
         list_1,                VM_OP_ARG_TYPE_INTEGER_CONST,
         list_2,                VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_3 (prop_getter,           PROP_GETTER,
         lhs,                   VM_OP_ARG_TYPE_VARIABLE,
         obj,                   VM_OP_ARG_TYPE_VARIABLE,
         prop,                  VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (prop_setter,           PROP_SETTER,
         obj,                   VM_OP_ARG_TYPE_VARIABLE,
         prop,                  VM_OP_ARG_TYPE_VARIABLE,
         rhs,                   VM_OP_ARG_TYPE_VARIABLE)

VM_OP_2 (delete_var,            DELETE_VAR,
         lhs,                   VM_OP_ARG_TYPE_VARIABLE,
         name,                  VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (delete_prop,           DELETE_PROP,
         lhs,                   VM_OP_ARG_TYPE_VARIABLE,
         base,                  VM_OP_ARG_TYPE_VARIABLE,
         name,                  VM_OP_ARG_TYPE_VARIABLE)

VM_OP_2 (typeof,                TYPEOF,
         lhs,                   VM_OP_ARG_TYPE_VARIABLE,
         obj,                   VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (for_in,                FOR_IN,
         expr,                  VM_OP_ARG_TYPE_VARIABLE,
         oc_idx_1,              VM_OP_ARG_TYPE_INTEGER_CONST,
         oc_idx_2,              VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_3 (with,                  WITH,
         expr,                  VM_OP_ARG_TYPE_VARIABLE,
         oc_idx_1,              VM_OP_ARG_TYPE_INTEGER_CONST,
         oc_idx_2,              VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_2 (try_block,             TRY_BLOCK,
         oc_idx_1,              VM_OP_ARG_TYPE_INTEGER_CONST,
         oc_idx_2,              VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_1 (throw_value,           THROW_VALUE,
         var,                   VM_OP_ARG_TYPE_VARIABLE)


VM_OP_3 (assignment,            ASSIGNMENT,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         type_value_right,      VM_OP_ARG_TYPE_TYPE_OF_NEXT,
         value_right,           VM_OP_ARG_TYPE_VARIABLE |
                                VM_OP_ARG_TYPE_STRING |
                                VM_OP_ARG_TYPE_NUMBER |
                                VM_OP_ARG_TYPE_INTEGER_CONST)


VM_OP_3 (b_shift_left,          B_SHIFT_LEFT,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (b_shift_right,         B_SHIFT_RIGHT,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (b_shift_uright,        B_SHIFT_URIGHT,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (b_and,                 B_AND,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (b_or,                  B_OR,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (b_xor,                 B_XOR,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_2 (b_not,                 B_NOT,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)


VM_OP_2 (logical_not,           LOGICAL_NOT,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)


VM_OP_3 (equal_value,           EQUAL_VALUE,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (not_equal_value,       NOT_EQUAL_VALUE,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (equal_value_type,      EQUAL_VALUE_TYPE,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (not_equal_value_type,  NOT_EQUAL_VALUE_TYPE,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (less_than,             LESS_THAN,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (greater_than,          GREATER_THAN,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (less_or_equal_than,    LESS_OR_EQUAL_THAN,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (greater_or_equal_than, GREATER_OR_EQUAL_THAN,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (instanceof,            INSTANCEOF,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (in,                    IN,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_2 (post_incr,             POST_INCR,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_2 (post_decr,             POST_DECR,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_2 (pre_incr,              PRE_INCR,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_2 (pre_decr,              PRE_DECR,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (addition,              ADDITION,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (substraction,          SUBSTRACTION,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (division,              DIVISION,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (multiplication,        MULTIPLICATION,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_3 (remainder,             REMAINDER,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var_left,              VM_OP_ARG_TYPE_VARIABLE,
         var_right,             VM_OP_ARG_TYPE_VARIABLE)

VM_OP_2 (unary_minus,           UNARY_MINUS,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var,                   VM_OP_ARG_TYPE_VARIABLE)

VM_OP_2 (unary_plus,            UNARY_PLUS,
         dst,                   VM_OP_ARG_TYPE_VARIABLE,
         var,                   VM_OP_ARG_TYPE_VARIABLE)

VM_OP_2 (jmp_up,                JMP_UP,
         oc_idx_1,              VM_OP_ARG_TYPE_INTEGER_CONST,
         oc_idx_2,              VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_2 (jmp_down,              JMP_DOWN,
         oc_idx_1,              VM_OP_ARG_TYPE_INTEGER_CONST,
         oc_idx_2,              VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_2 (jmp_break_continue,    JMP_BREAK_CONTINUE,
         oc_idx_1,              VM_OP_ARG_TYPE_INTEGER_CONST,
         oc_idx_2,              VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_3 (is_true_jmp_up,        IS_TRUE_JMP_UP,
         value,                 VM_OP_ARG_TYPE_VARIABLE,
         oc_idx_1,              VM_OP_ARG_TYPE_INTEGER_CONST,
         oc_idx_2,              VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_3 (is_true_jmp_down,      IS_TRUE_JMP_DOWN,
         value,                 VM_OP_ARG_TYPE_VARIABLE,
         oc_idx_1,              VM_OP_ARG_TYPE_INTEGER_CONST,
         oc_idx_2,              VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_3 (is_false_jmp_up,       IS_FALSE_JMP_UP,
         value,                 VM_OP_ARG_TYPE_VARIABLE,
         oc_idx_1,              VM_OP_ARG_TYPE_INTEGER_CONST,
         oc_idx_2,              VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_3 (is_false_jmp_down,     IS_FALSE_JMP_DOWN,
         value,                 VM_OP_ARG_TYPE_VARIABLE,
         oc_idx_1,              VM_OP_ARG_TYPE_INTEGER_CONST,
         oc_idx_2,              VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_1 (var_decl,              VAR_DECL,
         variable_name,         VM_OP_ARG_TYPE_STRING)

VM_OP_3 (reg_var_decl,          REG_VAR_DECL,
         tmp_regs_num,          VM_OP_ARG_TYPE_INTEGER_CONST,
         local_var_regs_num,    VM_OP_ARG_TYPE_INTEGER_CONST,
         arg_regs_num,          VM_OP_ARG_TYPE_INTEGER_CONST)

VM_OP_3 (meta,                  META,
         type,                  VM_OP_ARG_TYPE_INTEGER_CONST |
                                VM_OP_ARG_TYPE_TYPE_OF_NEXT,
         data_1,                VM_OP_ARG_TYPE_INTEGER_CONST |
                                VM_OP_ARG_TYPE_STRING |
                                VM_OP_ARG_TYPE_VARIABLE,
         data_2,                VM_OP_ARG_TYPE_INTEGER_CONST |
                                VM_OP_ARG_TYPE_VARIABLE)

#undef VM_OP_0
#undef VM_OP_1
#undef VM_OP_2
#undef VM_OP_3

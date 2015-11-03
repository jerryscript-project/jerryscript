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

#ifndef OPCODES_H
#define OPCODES_H

#include "ecma-globals.h"
#include "jrt.h"
#include "vm-stack.h"

/* Maximum opcodes number in bytecode.  */
#define MAX_OPCODES (256*256 - 1)

#define OP_0(action, name) \
        __##action (name, void, void, void)

#define OP_1(action, name, field1) \
        __##action (name, field1, void, void)

#define OP_2(action, name, field1, field2) \
        __##action (name, field1, field2, void)

#define OP_3(action, name, field1, field2, field3) \
        __##action (name, field1, field2, field3)

/**
 * Instruction counter / position
 */
typedef uint16_t vm_instr_counter_t;

/**
 * Opcode / argument value in an instruction ("idx")
 */
typedef uint8_t vm_idx_t;

/**
 * Description of vm_idx_t possible value ranges and special values
 */
enum : vm_idx_t
{
  VM_IDX_GENERAL_VALUE_FIRST = 0, /**< first idx value that can be used for any argument value */
  VM_IDX_GENERAL_VALUE_LAST  = 252, /**< last idx value that can be used for any argument value */

  /*
   * Special values
   */
  VM_IDX_REWRITE_GENERAL_CASE = 253, /**< intermediate value, used during byte-code generation,
                                      *   indicating that the idx would be rewritten with a value
                                      *   other than in-block literal identifier */
  VM_IDX_REWRITE_LITERAL_UID = 254, /**< intermediate value, used during byte-code generation,
                                     *   indicating that the idx would be rewritten with in-block
                                     *   literal identifier */
  VM_IDX_EMPTY = 255, /**< empty idx value, used when corresponding instruction argument is not set */

  /*
   * Literals (variable names / strings / numbers) ranges
   */
  VM_IDX_LITERAL_FIRST = VM_IDX_GENERAL_VALUE_FIRST, /**< index of first possible literals-related idx value */
  VM_IDX_LITERAL_LAST  = VM_IDX_LITERAL_FIRST + 127, /**< index of last possible literals-related idx value */

  /*
   * Registers (temp variables) ranges
   */
  VM_IDX_REG_FIRST = VM_IDX_LITERAL_LAST + 1, /** identifier of first special register */
  VM_IDX_REG_LAST = VM_IDX_GENERAL_VALUE_LAST, /**< identifier of last register */
};

/**
 * Ranges of registers (temporary variables)
 */
typedef enum : vm_idx_t
{
  VM_REG_FIRST = VM_IDX_REG_FIRST, /** first register */
  VM_REG_LAST = VM_IDX_REG_LAST, /**< last register */

  VM_REG_SPECIAL_FIRST = VM_REG_FIRST, /**< first special register */

  VM_REG_SPECIAL_EVAL_RET = VM_REG_SPECIAL_FIRST, /**< eval return value */
  VM_REG_SPECIAL_FOR_IN_PROPERTY_NAME, /**< variable, containing property name,
                                        *   at start of for-in loop body */
  VM_REG_SPECIAL_THIS_BINDING, /**< value of ThisBinding */

  VM_REG_SPECIAL_LAST = VM_REG_SPECIAL_THIS_BINDING, /**< last special register */

  VM_REG_GENERAL_FIRST, /** first non-special register */
  VM_REG_GENERAL_LAST = VM_IDX_REG_LAST /** last non-special register */
} vm_reg_t;

/**
 * Number of special VM registers
 */
#define VM_SPECIAL_REGS_NUMBER (VM_REG_SPECIAL_LAST - VM_REG_SPECIAL_FIRST + 1u)

/**
 * Descriptor of assignment's second argument
 * that specifies type of third argument.
 */
typedef enum
{
  OPCODE_ARG_TYPE_SIMPLE, /**< ecma_simple_value_t */
  OPCODE_ARG_TYPE_SMALLINT, /**< small integer: from 0 to 255 */
  OPCODE_ARG_TYPE_SMALLINT_NEGATE, /**< small integer: from -255 to -0 */
  OPCODE_ARG_TYPE_NUMBER, /**< index of number literal */
  OPCODE_ARG_TYPE_NUMBER_NEGATE, /**< index of number literal with negation */
  OPCODE_ARG_TYPE_STRING, /**< index of string literal */
  OPCODE_ARG_TYPE_VARIABLE, /**< index of string literal with variable name */
  OPCODE_ARG_TYPE_REGEXP /**< index of string literal with regular expression */
} opcode_arg_type_operand;

/**
 * Types of data in 'meta' opcode.
 */
typedef enum
{
  OPCODE_META_TYPE_UNDEFINED, /**< undefined meta (should be rewritten) */
  OPCODE_META_TYPE_CALL_SITE_INFO, /**< optional additional information about call site
                                    *   (includes opcode_call_flags_t and can include 'this' argument) */
  OPCODE_META_TYPE_VARG, /**< element (var_idx) of arguments' list */
  OPCODE_META_TYPE_VARG_PROP_DATA, /**< name (lit_idx) and value (var_idx) for a data property descriptor */
  OPCODE_META_TYPE_VARG_PROP_GETTER, /**< name (lit_idx) and getter (var_idx) for an accessor property descriptor */
  OPCODE_META_TYPE_VARG_PROP_SETTER, /**< name (lit_idx) and setter (var_idx) for an accessor property descriptor */
  OPCODE_META_TYPE_END_WITH, /**< end of with statement */
  OPCODE_META_TYPE_FUNCTION_END, /**< offset to function end */
  OPCODE_META_TYPE_CATCH, /**< mark of beginning of catch block containing pointer to end of catch block */
  OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER, /**< literal index containing name of variable with exception object */
  OPCODE_META_TYPE_FINALLY, /**< mark of beginning of finally block containing pointer to end of finally block */
  OPCODE_META_TYPE_END_TRY_CATCH_FINALLY, /**< mark of end of try-catch, try-finally, try-catch-finally blocks */
  OPCODE_META_TYPE_END_FOR_IN /**< end of for-in statement */
} opcode_meta_type;

typedef enum : vm_idx_t
{
  OPCODE_CALL_FLAGS__EMPTY                   = (0u),      /**< initializer for empty flag set */
  OPCODE_CALL_FLAGS_HAVE_THIS_ARG            = (1u << 0), /**< flag, indicating that call is performed
                                                           *   with 'this' argument specified */
  OPCODE_CALL_FLAGS_DIRECT_CALL_TO_EVAL_FORM = (1u << 1)  /**< flag, indicating that call is performed
                                                           *   in form 'eval (...)', i.e. through 'eval' string
                                                           *   without object base (i.e. with lexical environment
                                                           *   as base), so it can be a direct call to eval
                                                           *   See also: ECMA-262 v5, 15.1.2.1.1
                                                           */
} opcode_call_flags_t;

/**
 * Types of byte-code instruction arguments, used for instruction description
 *
 * See also:
 *          vm-opcodes.inc.h
 */
typedef enum
{
  VM_OP_ARG_TYPE_EMPTY         = (1u << 0), /**< empty argument (no value) */
  VM_OP_ARG_TYPE_REGISTER      = (1u << 1), /**< register variable (index) */
  VM_OP_ARG_TYPE_IDENTIFIER    = (1u << 2), /**< identifier - named variable (string literal) */
  VM_OP_ARG_TYPE_STRING        = (1u << 3), /**< string constant value (string literal) */
  VM_OP_ARG_TYPE_NUMBER        = (1u << 4), /**< number constant value (number literal) */
  VM_OP_ARG_TYPE_INTEGER_CONST = (1u << 5), /**< a 8-bit integer constant (any vm_idx_t) */
  VM_OP_ARG_TYPE_TYPE_OF_NEXT  = (1u << 6), /**< opcode_arg_type_operand value,
                                             *   representing type of argument encoded in next idx */

  /** variable - an identifier or a register */
  VM_OP_ARG_TYPE_VARIABLE      = (VM_OP_ARG_TYPE_REGISTER | VM_OP_ARG_TYPE_IDENTIFIER)
} vm_op_arg_type_t;

/**
 * Forward declaration of instruction structure
 */
struct vm_instr_t;

/**
 * Forward declaration of bytecode data header structure
 */
struct bytecode_data_header_t;

/**
 * Context of interpreter, related to a JS stack frame
 */
typedef struct
{
  const bytecode_data_header_t *bytecode_header_p; /**< currently executed byte-code data */
  vm_instr_counter_t pos; /**< current position instruction to execute */
  ecma_object_t *lex_env_p; /**< current lexical environment */
  bool is_strict; /**< is current code execution mode strict? */
  bool is_eval_code; /**< is current code executed with eval */
  bool is_call_in_direct_eval_form; /** flag, indicating if there is call of 'Direct call to eval' form in
                                     *  process (see also: OPCODE_CALL_FLAGS_DIRECT_CALL_TO_EVAL_FORM) */
  ecma_number_t *tmp_num_p; /**< an allocated number (to reduce temporary allocations) */
  vm_stack_frame_t stack_frame; /**< stack frame associated with the context */

#ifdef MEM_STATS
  size_t context_peak_allocated_heap_bytes;
  size_t context_peak_waste_heap_bytes;
  size_t context_peak_pools_count;
  size_t context_peak_allocated_pool_chunks;

  mem_heap_stats_t heap_stats_context_enter;
  mem_pools_stats_t pools_stats_context_enter;
#endif /* MEM_STATS */
} vm_frame_ctx_t;

/**
 * Description of a run scope
 *
 * Note:
 *      Run scope represents boundaries of byte-code block to run.
 *
 *      Jumps within of the current run scope are performed by just changing instruction counter,
 *      and outside of the run scope - by returning corresponding ECMA_COMPLETION_TYPE_BREAK_CONTINUE
 *      completion value.
 */
typedef struct
{
  const vm_instr_counter_t start_oc; /**< instruction counter of the first instruction of the scope */
  const vm_instr_counter_t end_oc; /**< instruction counter of the last instruction of the scope */
} vm_run_scope_t;

vm_instr_counter_t vm_calc_instr_counter_from_idx_idx (const vm_idx_t, const vm_idx_t);
vm_instr_counter_t vm_read_instr_counter_from_meta (opcode_meta_type,
                                                    const bytecode_data_header_t *,
                                                    vm_instr_counter_t);

typedef struct vm_instr_t
{
  vm_idx_t op_idx;
  union
  {
#define VM_OP_1(opcode_name, opcode_name_uppercase, arg1, arg1_type) \
    struct \
    { \
      vm_idx_t arg1; \
    } opcode_name;

#define VM_OP_2(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type) \
    struct \
    { \
      vm_idx_t arg1; \
      vm_idx_t arg2; \
    } opcode_name;
#define VM_OP_3(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type, arg3, arg3_type) \
    struct \
    { \
      vm_idx_t arg1; \
      vm_idx_t arg2; \
      vm_idx_t arg3; \
    } opcode_name;

#include "vm-opcodes.inc.h"

    /**
     * Opcode-independent arguments accessor
     *
     * Note:
     *      If opcode is statically known, opcode-specific way of accessing arguments should be used.
     */
    vm_idx_t raw_args[3];
  } data;
} vm_instr_t;

typedef enum
{
#define VM_OP_0(opcode_name, opcode_name_uppercase) \
  VM_OP_ ## opcode_name_uppercase,
#define VM_OP_1(opcode_name, opcode_name_uppercase, arg1, arg1_type) \
  VM_OP_ ## opcode_name_uppercase,
#define VM_OP_2(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type) \
  VM_OP_ ## opcode_name_uppercase,
#define VM_OP_3(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type, arg3, arg3_type) \
  VM_OP_ ## opcode_name_uppercase,

#include "vm-opcodes.inc.h"

  VM_OP__COUNT /**< number of opcodes */
} vm_op_t;

#define VM_OP_0(opcode_name, opcode_name_uppercase) \
  ecma_completion_value_t opfunc_##opcode_name (vm_instr_t, vm_frame_ctx_t *);
#define VM_OP_1(opcode_name, opcode_name_uppercase, arg1, arg1_type) \
  ecma_completion_value_t opfunc_##opcode_name (vm_instr_t, vm_frame_ctx_t *);
#define VM_OP_2(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type) \
  ecma_completion_value_t opfunc_##opcode_name (vm_instr_t, vm_frame_ctx_t *);
#define VM_OP_3(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type, arg3, arg3_type) \
  ecma_completion_value_t opfunc_##opcode_name (vm_instr_t, vm_frame_ctx_t *);

#include "vm-opcodes.inc.h"

typedef ecma_completion_value_t (*opfunc) (vm_instr_t, vm_frame_ctx_t *);

#endif /* OPCODES_H */

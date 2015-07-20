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

typedef uint16_t opcode_counter_t; /** opcode counters */
typedef uint8_t idx_t; /** index values */

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
  OPCODE_META_TYPE_SCOPE_CODE_FLAGS, /**< set of flags indicating various properties of the scope's code
                                      *   (See also: opcode_scope_code_flags_t) */
  OPCODE_META_TYPE_END_FOR_IN /**< end of for-in statement */
} opcode_meta_type;

typedef enum : idx_t
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
 * Flags indicating various properties of a scope's code
 */
typedef enum : idx_t
{
  OPCODE_SCOPE_CODE_FLAGS__EMPTY                       = (0u),      /**< initializer for empty flag set */
  OPCODE_SCOPE_CODE_FLAGS_STRICT                       = (1u << 0), /**< code is strict mode code */
  OPCODE_SCOPE_CODE_FLAGS_NOT_REF_ARGUMENTS_IDENTIFIER = (1u << 1), /**< code doesn't reference
                                                                     *   'arguments' identifier */
  OPCODE_SCOPE_CODE_FLAGS_NOT_REF_EVAL_IDENTIFIER      = (1u << 2)  /**< code doesn't reference
                                                                     *   'eval' identifier */
} opcode_scope_code_flags_t;

/**
 * Enumeration of registers (temp variables) ranges
 */
typedef enum : idx_t
{
  OPCODE_REG_FIRST = 128, /** identifier of first special register */
  OPCODE_REG_SPECIAL_EVAL_RET = OPCODE_REG_FIRST, /**< eval return value */
  OPCODE_REG_SPECIAL_FOR_IN_PROPERTY_NAME, /**< variable, containing property name,
                                            *   at start of for-in loop body */
  OPCODE_REG_GENERAL_FIRST, /** identifier of first non-special register */
  OPCODE_REG_GENERAL_LAST = 253, /** identifier of last non-special register */
  OPCODE_REG_LAST = OPCODE_REG_GENERAL_FIRST /**< identifier of last register */
} opcode_special_reg_t;

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
  VM_OP_ARG_TYPE_INTEGER_CONST = (1u << 5), /**< a 8-bit integer constant (any idx_t) */
  VM_OP_ARG_TYPE_TYPE_OF_NEXT  = (1u << 6), /**< opcode_arg_type_operand value,
                                             *   representing type of argument encoded in next idx */

  /** variable - an identifier or a register */
  VM_OP_ARG_TYPE_VARIABLE      = (VM_OP_ARG_TYPE_REGISTER | VM_OP_ARG_TYPE_IDENTIFIER)
} vm_op_arg_type_t;

/**
 * Forward declaration of opcode structure
 */
struct opcode_t;

/**
 * Context of interpreter, related to a JS stack frame
 */
typedef struct
{
  const opcode_t *opcodes_p; /**< pointer to array containing currently executed bytecode */
  opcode_counter_t pos; /**< current opcode to execute */
  ecma_value_t this_binding; /**< this binding for current context */
  ecma_object_t *lex_env_p; /**< current lexical environment */
  bool is_strict; /**< is current code execution mode strict? */
  bool is_eval_code; /**< is current code executed with eval */
  bool is_call_in_direct_eval_form; /** flag, indicating if there is call of 'Direct call to eval' form in
                                     *  process (see also: OPCODE_CALL_FLAGS_DIRECT_CALL_TO_EVAL_FORM) */
  idx_t min_reg_num; /**< minimum idx used for register identification */
  idx_t max_reg_num; /**< maximum idx used for register identification */
  ecma_number_t* tmp_num_p; /**< an allocated number (to reduce temporary allocations) */
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
 *      Jumps within of the current run scope are performed by just changing opcode counter,
 *      and outside of the run scope - by returning corresponding ECMA_COMPLETION_TYPE_BREAK_CONTINUE
 *      completion value.
 */
typedef struct
{
  const opcode_counter_t start_oc; /**< opcode counter of the first instruction of the scope */
  const opcode_counter_t end_oc; /**< opcode counter of the last instruction of the scope */
} vm_run_scope_t;

opcode_counter_t calc_opcode_counter_from_idx_idx (const idx_t oc_idx_1, const idx_t oc_idx_2);
opcode_counter_t read_meta_opcode_counter (opcode_meta_type expected_type, vm_frame_ctx_t *frame_ctx_p);

typedef struct opcode_t
{
  idx_t op_idx;
  union
  {
#define VM_OP_1(opcode_name, opcode_name_uppercase, arg1, arg1_type) \
    struct \
    { \
      idx_t arg1; \
    } opcode_name;

#define VM_OP_2(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type) \
    struct \
    { \
      idx_t arg1; \
      idx_t arg2; \
    } opcode_name;
#define VM_OP_3(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type, arg3, arg3_type) \
    struct \
    { \
      idx_t arg1; \
      idx_t arg2; \
      idx_t arg3; \
    } opcode_name;

#include "vm-opcodes.inc.h"
  } data;
} opcode_t;

enum __opcode_idx
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
};

#define VM_OP_0(opcode_name, opcode_name_uppercase) \
  ecma_completion_value_t opfunc_##opcode_name (opcode_t, vm_frame_ctx_t*);
#define VM_OP_1(opcode_name, opcode_name_uppercase, arg1, arg1_type) \
  ecma_completion_value_t opfunc_##opcode_name (opcode_t, vm_frame_ctx_t*);
#define VM_OP_2(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type) \
  ecma_completion_value_t opfunc_##opcode_name (opcode_t, vm_frame_ctx_t*);
#define VM_OP_3(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type, arg3, arg3_type) \
  ecma_completion_value_t opfunc_##opcode_name (opcode_t, vm_frame_ctx_t*);

#include "vm-opcodes.inc.h"

typedef ecma_completion_value_t (*opfunc) (opcode_t, vm_frame_ctx_t *);

#define VM_OP_0(opcode_name, opcode_name_uppercase) \
        opcode_t getop_##opcode_name (void);
#define VM_OP_1(opcode_name, opcode_name_uppercase, arg1, arg1_type) \
        opcode_t getop_##opcode_name (idx_t);
#define VM_OP_2(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type) \
        opcode_t getop_##opcode_name (idx_t, idx_t);
#define VM_OP_3(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type, arg3, arg3_type) \
        opcode_t getop_##opcode_name (idx_t, idx_t, idx_t);

#include "vm-opcodes.inc.h"


typedef struct
{
  uint8_t uids[4];
} raw_opcode;

#endif /* OPCODES_H */

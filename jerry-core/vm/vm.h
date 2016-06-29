/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged.
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

#ifndef VM_H
#define VM_H

#include "ecma-globals.h"
#include "jrt.h"
#include "vm-defines.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup vm_executor Executor
 * @{
 */

/**
 * Each CBC opcode is transformed to three vm opcodes:
 *
 *  - first opcode is a "get arguments" opcode which specifies
 *    the type (register, literal, stack, etc.) and number
 *    (from 0 to 2) of input arguments
 *  - second opcode is a "group" opcode which specifies
 *    the actual operation (add, increment, call, etc.)
 *  - third opcode is a "put result" opcode which specifies
 *    the destination where the result is stored (register,
 *    stack, etc.)
 */

/**
 * Branch argument is a backward branch
 */
#define VM_OC_BACKWARD_BRANCH 0x4000

/**
 * Position of "get arguments" opcode.
 */
#define VM_OC_GET_ARGS_SHIFT 7

/**
 * Mask of "get arguments" opcode.
 */
#define VM_OC_GET_ARGS_MASK 0x7

/**
 * Generate the binary representation of a "get arguments" opcode.
 */
#define VM_OC_GET_ARGS_CREATE_INDEX(V) (((V) & VM_OC_GET_ARGS_MASK) << VM_OC_GET_ARGS_SHIFT)

/**
 * Extract the "get arguments" opcode.
 */
#define VM_OC_GET_ARGS_INDEX(O) ((O) & (VM_OC_GET_ARGS_MASK << VM_OC_GET_ARGS_SHIFT))

/**
 * Checks whether the result is stored somewhere.
 */
#define VM_OC_HAS_GET_ARGS(V) ((V) & (VM_OC_GET_ARGS_MASK << VM_OC_GET_ARGS_SHIFT))

/**
 * Argument getters that are part of the opcodes.
 */
typedef enum
{
  VM_OC_GET_NONE = VM_OC_GET_ARGS_CREATE_INDEX (0),             /**< do nothing */
  VM_OC_GET_BRANCH = VM_OC_GET_ARGS_CREATE_INDEX (1),           /**< branch argument */
  VM_OC_GET_STACK = VM_OC_GET_ARGS_CREATE_INDEX (2),            /**< pop one element from the stack */
  VM_OC_GET_STACK_STACK = VM_OC_GET_ARGS_CREATE_INDEX (3),      /**< pop two elements from the stack */

  VM_OC_GET_LITERAL = VM_OC_GET_ARGS_CREATE_INDEX (4),          /**< resolve literal */
  VM_OC_GET_LITERAL_LITERAL = VM_OC_GET_ARGS_CREATE_INDEX (5),  /**< resolve two literals */
  VM_OC_GET_STACK_LITERAL = VM_OC_GET_ARGS_CREATE_INDEX (6),    /**< pop one element from the stack
                                                                 *   and resolve a literal */
  VM_OC_GET_THIS_LITERAL = VM_OC_GET_ARGS_CREATE_INDEX (7),     /**< get this and resolve a literal */
} vm_oc_get_types;

/**
 * Mask of "group" opcode.
 */
#define VM_OC_GROUP_MASK 0x7f

/**
 * Extract the "group" opcode.
 */
#define VM_OC_GROUP_GET_INDEX(O) ((O) & VM_OC_GROUP_MASK)

/**
 * Opcodes.
 */
typedef enum
{
  VM_OC_NONE,                    /**< do nothing */
  VM_OC_POP,                     /**< pop from stack */
  VM_OC_POP_BLOCK,               /**< pop block */
  VM_OC_PUSH,                    /**< push one element  */
  VM_OC_PUSH_TWO,                /**< push two elements onto the stack */
  VM_OC_PUSH_THREE,              /**< push three elements onto the stack */
  VM_OC_PUSH_UNDEFINED,          /**< push undefined value */
  VM_OC_PUSH_TRUE,               /**< push true value */
  VM_OC_PUSH_FALSE,              /**< push false value */
  VM_OC_PUSH_NULL,               /**< push null value */
  VM_OC_PUSH_THIS,               /**< push this */
  VM_OC_PUSH_NUMBER_0,           /**< push number zero */
  VM_OC_PUSH_NUMBER_POS_BYTE,    /**< push number between 1 and 256 */
  VM_OC_PUSH_NUMBER_NEG_BYTE,    /**< push number between -1 and -256 */
  VM_OC_PUSH_OBJECT,             /**< push object */
  VM_OC_SET_PROPERTY,            /**< set property */
  VM_OC_SET_GETTER,              /**< set getter */
  VM_OC_SET_SETTER,              /**< set setter */
  VM_OC_PUSH_UNDEFINED_BASE,     /**< push undefined base */
  VM_OC_PUSH_ARRAY,              /**< push array */
  VM_OC_PUSH_ELISON,             /**< push elison */
  VM_OC_APPEND_ARRAY,            /**< append array */
  VM_OC_IDENT_REFERENCE,         /**< ident reference */
  VM_OC_PROP_REFERENCE,          /**< prop reference */
  VM_OC_PROP_GET,                /**< prop get */

  /* These eight opcodes must be in this order. */
  VM_OC_PROP_PRE_INCR,           /**< prefix increment of a property */
  VM_OC_PROP_PRE_DECR,           /**< prop prefix decrement of a property */
  VM_OC_PROP_POST_INCR,          /**< prop postfix increment of a property */
  VM_OC_PROP_POST_DECR,          /**< prop postfix decrement of a property */
  VM_OC_PRE_INCR,                /**< prefix increment  */
  VM_OC_PRE_DECR,                /**< prefix decrement */
  VM_OC_POST_INCR,               /**< postfix increment */
  VM_OC_POST_DECR,               /**< postfix decrement */

  VM_OC_PROP_DELETE,             /**< delete property */
  VM_OC_DELETE,                  /**< delete */

  VM_OC_ASSIGN,                  /**< assign */
  VM_OC_ASSIGN_PROP,             /**< assign property */
  VM_OC_ASSIGN_PROP_THIS,        /**< assign prop this */

  VM_OC_RET,                     /**< return */
  VM_OC_THROW,                   /**< throw */
  VM_OC_THROW_REFERENCE_ERROR,   /**< throw reference error */

  VM_OC_EVAL,                    /**< eval */
  VM_OC_CALL,                    /**< call */
  VM_OC_NEW,                     /**< new */

  VM_OC_JUMP,                    /**< jump */
  VM_OC_BRANCH_IF_STRICT_EQUAL,  /**< branch if stric equal */

  /* These four opcodes must be in this order. */
  VM_OC_BRANCH_IF_TRUE,          /**< branch if true */
  VM_OC_BRANCH_IF_FALSE,         /**< branch if false */
  VM_OC_BRANCH_IF_LOGICAL_TRUE,  /**< branch if logical true */
  VM_OC_BRANCH_IF_LOGICAL_FALSE, /**< branch if logical false */

  VM_OC_PLUS,                    /**< unary plus */
  VM_OC_MINUS,                   /**< unary minus */
  VM_OC_NOT,                     /**< not */
  VM_OC_BIT_NOT,                 /**< bitwise not */
  VM_OC_VOID,                    /**< void */
  VM_OC_TYPEOF_IDENT,            /**< typeof identifier */
  VM_OC_TYPEOF,                  /**< typeof */

  VM_OC_ADD,                     /**< binary add */
  VM_OC_SUB,                     /**< binary sub */
  VM_OC_MUL,                     /**< mul */
  VM_OC_DIV,                     /**< div */
  VM_OC_MOD,                     /**< mod */

  VM_OC_EQUAL,                   /**< equal */
  VM_OC_NOT_EQUAL,               /**< not equal */
  VM_OC_STRICT_EQUAL,            /**< strict equal */
  VM_OC_STRICT_NOT_EQUAL,        /**< strict not equal */
  VM_OC_LESS,                    /**< less */
  VM_OC_GREATER,                 /**< greater */
  VM_OC_LESS_EQUAL,              /**< less equal */
  VM_OC_GREATER_EQUAL,           /**< greater equal */
  VM_OC_IN,                      /**< in */
  VM_OC_INSTANCEOF,              /**< instanceof */

  VM_OC_BIT_OR,                  /**< bitwise or */
  VM_OC_BIT_XOR,                 /**< bitwise xor */
  VM_OC_BIT_AND,                 /**< bitwise and */
  VM_OC_LEFT_SHIFT,              /**< left shift */
  VM_OC_RIGHT_SHIFT,             /**< right shift */
  VM_OC_UNS_RIGHT_SHIFT,         /**< unsigned right shift */

  VM_OC_WITH,                    /**< with */
  VM_OC_FOR_IN_CREATE_CONTEXT,   /**< for in create context */
  VM_OC_FOR_IN_GET_NEXT,         /**< get next */
  VM_OC_FOR_IN_HAS_NEXT,         /**< has next */
  VM_OC_TRY,                     /**< try */
  VM_OC_CATCH,                   /**< catch */
  VM_OC_FINALLY,                 /**< finally */
  VM_OC_CONTEXT_END,             /**< context end */
  VM_OC_JUMP_AND_EXIT_CONTEXT,   /**< jump and exit context */
} vm_oc_types;

/**
 * Decrement operator.
 */
#define VM_OC_DECREMENT_OPERATOR_FLAG 0x1

/**
 * Postfix increment/decrement operator.
 */
#define VM_OC_POST_INCR_DECR_OPERATOR_FLAG 0x2

/**
 * An named variable is updated by the increment/decrement operator.
 */
#define VM_OC_IDENT_INCR_DECR_OPERATOR_FLAG 0x4

/**
 * Jump to target offset if input value is logical false.
 */
#define VM_OC_BRANCH_IF_FALSE_FLAG 0x1

/**
 * Branch optimized for logical and/or opcodes.
 */
#define VM_OC_LOGICAL_BRANCH_FLAG 0x2

/**
 * Position of "put result" opcode.
 */
#define VM_OC_PUT_RESULT_SHIFT 10

/**
 * Mask of "put result" opcode.
 */
#define VM_OC_PUT_RESULT_MASK 0xf

/**
 * Generate a "put result" opcode flag bit.
 */
#define VM_OC_PUT_RESULT_CREATE_FLAG(V) (((V) & VM_OC_PUT_RESULT_MASK) << VM_OC_PUT_RESULT_SHIFT)

/**
 * Checks whether the result is stored somewhere.
 */
#define VM_OC_HAS_PUT_RESULT(V) ((V) & (VM_OC_PUT_RESULT_MASK << VM_OC_PUT_RESULT_SHIFT))

/**
 * Specify where the result is stored
 */
typedef enum
{
  VM_OC_PUT_IDENT = VM_OC_PUT_RESULT_CREATE_FLAG (0x1),
  VM_OC_PUT_REFERENCE = VM_OC_PUT_RESULT_CREATE_FLAG (0x2),
  VM_OC_PUT_STACK = VM_OC_PUT_RESULT_CREATE_FLAG (0x4),
  VM_OC_PUT_BLOCK = VM_OC_PUT_RESULT_CREATE_FLAG (0x8),
} vm_oc_put_types;

/**
 * Non-recursive vm_loop: the vm_loop can be suspended
 * to execute a call /construct operation. These return
 * types of the vm_loop tells whether a call operation
 * is in progress or the vm_loop is finished.
 */
typedef enum
{
  VM_NO_EXEC_OP,                 /**< do nothing */
  VM_EXEC_CALL,                  /**< invoke a function */
  VM_EXEC_CONSTRUCT,             /**< construct a new object */
} vm_call_operation;

extern ecma_value_t vm_run_global (const ecma_compiled_code_t *);
extern ecma_value_t vm_run_eval (ecma_compiled_code_t *, bool);

extern ecma_value_t vm_run (const ecma_compiled_code_t *, ecma_value_t,
                            ecma_object_t *, bool, const ecma_value_t *,
                            ecma_length_t);

extern bool vm_is_strict_mode (void);
extern bool vm_is_direct_eval_form_call (void);

/**
 * @}
 * @}
 */

#endif /* !VM_H */

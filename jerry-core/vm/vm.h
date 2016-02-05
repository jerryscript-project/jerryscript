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

#define VM_OC_GET_DATA_SHIFT 24
#define VM_OC_GET_DATA_MASK 0x1f
#define VM_OC_GET_DATA_CREATE_ID(V) \
  (((V) & VM_OC_GET_DATA_MASK) << VM_OC_GET_DATA_SHIFT)
#define VM_OC_GET_DATA_GET_ID(O) \
  (((O) >> VM_OC_GET_DATA_SHIFT) & VM_OC_GET_DATA_MASK)

/**
 * Argument getters that are part of the opcodes.
 */
typedef enum
{
  VM_OC_GET_NONE = VM_OC_GET_DATA_CREATE_ID (0),             /**< do nothing */
  VM_OC_GET_STACK = VM_OC_GET_DATA_CREATE_ID (1),            /**< pop one elemnet from the stack */
  VM_OC_GET_STACK_STACK = VM_OC_GET_DATA_CREATE_ID (2),      /**< pop two elemnets from the stack */
  VM_OC_GET_BYTE = VM_OC_GET_DATA_CREATE_ID (3),             /**< read a byte */

  VM_OC_GET_LITERAL = VM_OC_GET_DATA_CREATE_ID (4),          /**< resolve literal */
  VM_OC_GET_STACK_LITERAL = VM_OC_GET_DATA_CREATE_ID (5),    /**< pop one elemnet from the stack and resolve a literal*/
  VM_OC_GET_LITERAL_BYTE = VM_OC_GET_DATA_CREATE_ID (6),     /**< pop one elemnet from stack and read a byte */
  VM_OC_GET_LITERAL_LITERAL = VM_OC_GET_DATA_CREATE_ID (7),  /**< resolve two literals */
  VM_OC_GET_THIS_LITERAL = VM_OC_GET_DATA_CREATE_ID (8),     /**< get this and resolve a literal */
} vm_oc_get_types;

#define VM_OC_GROUP_MASK 0xff
#define VM_OC_GROUP_GET_INDEX(O) \
  ((O) & VM_OC_GROUP_MASK)

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
  VM_OC_PUSH_NUMBER,             /**< push number */
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
  VM_OC_PRE_INCR,                /**< prefix increment  */
  VM_OC_PROP_PRE_DECR,           /**< prop prefix decrement of a property */
  VM_OC_PRE_DECR,                /**< prefix decrement */
  VM_OC_PROP_POST_INCR,          /**< prop postfix increment of a property */
  VM_OC_POST_INCR,               /**< postfix increment */
  VM_OC_PROP_POST_DECR,          /**< prop postfix decrement of a property */
  VM_OC_POST_DECR,               /**< postfix decrement */

  VM_OC_PROP_DELETE,             /**< delete property */
  VM_OC_DELETE,                  /**< delete */

  VM_OC_ASSIGN,                  /**< assign */
  VM_OC_ASSIGN_PROP,             /**< assign property */
  VM_OC_ASSIGN_PROP_THIS,        /**< assign prop this */

  VM_OC_RET,                     /**< return */
  VM_OC_THROW,                   /**< throw */
  VM_OC_THROW_REFERENCE_ERROR,   /**< throw reference error */

  /* The PROP forms must get the highest opcodes. */
  VM_OC_EVAL,                    /**< eval */
  VM_OC_CALL_N,                  /**< call n */
  VM_OC_CALL,                    /**< call */
  VM_OC_CALL_PROP_N,             /**< call property n */
  VM_OC_CALL_PROP,               /**< call property */

  VM_OC_NEW_N,                   /**< new n */
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

#define VM_OC_PUT_DATA_SHIFT 12
#define VM_OC_PUT_DATA_MASK 0xf
#define VM_OC_PUT_DATA_CREATE_FLAG(V) \
  (((V) & VM_OC_PUT_DATA_MASK) << VM_OC_PUT_DATA_SHIFT)

/**
 * Result writers that are part of the opcodes.
 */
typedef enum
{
  VM_OC_PUT_IDENT = VM_OC_PUT_DATA_CREATE_FLAG (0x1),
  VM_OC_PUT_REFERENCE = VM_OC_PUT_DATA_CREATE_FLAG (0x2),
  VM_OC_PUT_STACK = VM_OC_PUT_DATA_CREATE_FLAG (0x4),
  VM_OC_PUT_BLOCK = VM_OC_PUT_DATA_CREATE_FLAG (0x8),
} vm_oc_put_types;

extern void vm_init (ecma_compiled_code_t *, bool);
extern void vm_finalize (void);
extern jerry_completion_code_t vm_run_global (void);
extern ecma_completion_value_t vm_run_eval (ecma_compiled_code_t *, bool);

extern ecma_completion_value_t vm_loop (vm_frame_ctx_t *);
extern ecma_completion_value_t vm_run (const ecma_compiled_code_t *,
                                       ecma_value_t,
                                       ecma_object_t *,
                                       bool,
                                       ecma_collection_header_t *);

extern ecma_completion_value_t vm_run_array_args (const ecma_compiled_code_t *,
                                                  ecma_value_t,
                                                  ecma_object_t *,
                                                  bool,
                                                  const ecma_value_t *,
                                                  ecma_length_t);

extern bool vm_is_strict_mode (void);
extern bool vm_is_direct_eval_form_call (void);

/**
 * @}
 * @}
 */

#endif /* !VM_H */

/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
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

#ifndef VM_STACK_H
#define VM_STACK_H

#include "config.h"
#include "ecma-globals.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup stack VM stack
 * @{
 */

/**
 * Number of ecma values inlined into stack frame
 */
#define VM_STACK_FRAME_INLINED_VALUES_NUMBER CONFIG_VM_STACK_FRAME_INLINED_VALUES_NUMBER

/**
 * Header of a ECMA stack frame's chunk
 */
typedef struct
{
  uint16_t prev_chunk_p;                      /**< previous chunk of same frame */
} vm_stack_chunk_header_t;

/**
 * ECMA stack frame
 */
typedef struct vm_stack_frame_t
{
  struct vm_stack_frame_t *prev_frame_p;      /**< previous frame */
  uint32_t regs_number;                       /**< number of register variables */
} vm_stack_frame_t;

#define VM_CREATE_CONTEXT(type, end_offset) ((ecma_value_t) ((type) | (end_offset) << 4))
#define VM_GET_CONTEXT_TYPE(value) ((vm_stack_context_type_t) ((value) & 0xf))
#define VM_GET_CONTEXT_END(value) ((value) >> 4)

/**
 * Context types for the vm stack.
 */
typedef enum
{
  VM_CONTEXT_FINALLY_JUMP,                    /**< finally context with a jump */
  VM_CONTEXT_FINALLY_THROW,                   /**< finally context with a throw */
  VM_CONTEXT_FINALLY_RETURN,                  /**< finally context with a return */
  VM_CONTEXT_TRY,                             /**< try context */
  VM_CONTEXT_CATCH,                           /**< catch context */
  VM_CONTEXT_WITH,                            /**< with context */
  VM_CONTEXT_FOR_IN,                          /**< for-in context */
} vm_stack_context_type_t;

extern ecma_value_t *vm_stack_context_abort (vm_frame_ctx_t *, ecma_value_t *);
extern bool vm_stack_find_finally (vm_frame_ctx_t *, ecma_value_t **,
                                   vm_stack_context_type_t, uint32_t);

/**
 * @}
 * @}
 */

#endif /* !VM_STACK_H */

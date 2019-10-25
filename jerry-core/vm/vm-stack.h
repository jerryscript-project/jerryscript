/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include "ecma-globals.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup stack VM stack
 * @{
 */

/**
 * Create context on the vm stack.
 */
#define VM_CREATE_CONTEXT(type, end_offset) ((ecma_value_t) ((type) | ((end_offset) << 6)))

/**
 * Create context on the vm stack with environment.
 */
#define VM_CREATE_CONTEXT_WITH_ENV(type, end_offset) \
  ((ecma_value_t) ((type) | ((end_offset) << 6) | VM_CONTEXT_HAS_LEX_ENV))

/**
 * Get type of a vm context.
 */
#define VM_GET_CONTEXT_TYPE(value) ((vm_stack_context_type_t) ((value) & 0x1f))

/**
 * Get the end position of a vm context.
 */
#define VM_GET_CONTEXT_END(value) ((value) >> 6)

/**
 * This flag is set if the context has a lexical environment.
 */
#define VM_CONTEXT_HAS_LEX_ENV 0x20

/**
 * Context types for the vm stack.
 */
typedef enum
{
  /* Update VM_CONTEXT_IS_FINALLY macro if the following three values are changed. */
  VM_CONTEXT_FINALLY_JUMP,                    /**< finally context with a jump */
  VM_CONTEXT_FINALLY_THROW,                   /**< finally context with a throw */
  VM_CONTEXT_FINALLY_RETURN,                  /**< finally context with a return */
  VM_CONTEXT_TRY,                             /**< try context */
  VM_CONTEXT_CATCH,                           /**< catch context */
#if ENABLED (JERRY_ES2015)
  VM_CONTEXT_BLOCK,                           /**< block context */
#endif /* ENABLED (JERRY_ES2015) */
  VM_CONTEXT_WITH,                            /**< with context */
  VM_CONTEXT_FOR_IN,                          /**< for-in context */
#if ENABLED (JERRY_ES2015)
  VM_CONTEXT_FOR_OF,                          /**< for-of context */
  VM_CONTEXT_SUPER_CLASS,                     /**< super class context */
#endif /* ENABLED (JERRY_ES2015) */
} vm_stack_context_type_t;

/**
 * Checks whether the context type is a finally type.
 */
#define VM_CONTEXT_IS_FINALLY(context_type) \
  ((context_type) <= VM_CONTEXT_FINALLY_RETURN)

ecma_value_t *vm_stack_context_abort (vm_frame_ctx_t *frame_ctx_p, ecma_value_t *vm_stack_top_p);
bool vm_stack_find_finally (vm_frame_ctx_t *frame_ctx_p, ecma_value_t **vm_stack_top_ref_p,
                            vm_stack_context_type_t finally_type, uint32_t search_limit);

/**
 * @}
 * @}
 */

#endif /* !VM_STACK_H */

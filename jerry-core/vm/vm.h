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

#ifndef VM_H
#define VM_H

#include "ecma-globals.h"
#include "ecma-module.h"

#include "jrt.h"
#include "vm-defines.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup vm_executor Executor
 * @{
 */

/**
 * Specify where the result is stored
 */
typedef enum
{
  /* These 3 flags must be in this order */
  VM_PUT_RESULT_NONE = 0, /**< result is not needed, can be free */
  VM_PUT_RESULT_STACK = 1 << 0, /**< result must be placed on to the stack */
  VM_PUT_RESULT_BLOCK = 1 << 1, /**< result must be placed on to frame's block result */

  VM_PUT_RESULT_IDENT = 1 << 2, /**< assign to ident reference */
  VM_PUT_RESULT_REFERENCE = 1 << 4, /**< assign to property reference */
  VM_PUT_RESULT_DECR = 1 << 5, /**< unary -- flag */
  VM_PUT_RESULT_POST = 1 << 6, /**< post unary lvalue flag */
} vm_put_result_flags_t;

/**
 * Non-recursive vm_loop: the vm_loop can be suspended
 * to execute a call /construct operation. These return
 * types of the vm_loop tells whether a call operation
 * is in progress or the vm_loop is finished.
 */
typedef enum
{
  VM_NO_EXEC_OP, /**< do nothing */
  VM_EXEC_CALL, /**< invoke a function */
  VM_EXEC_SUPER_CALL, /**< invoke a function through 'super' keyword */
  VM_EXEC_SPREAD_OP, /**< call/construct operation with spreaded argument list */
  VM_EXEC_RETURN, /**< return with the completion value without freeing registers */
  VM_EXEC_CONSTRUCT, /**< construct a new object */
} vm_call_operation;

ecma_value_t vm_run_global (const ecma_compiled_code_t *bytecode_p, ecma_object_t *function_object_p);
ecma_value_t vm_run_eval (ecma_compiled_code_t *bytecode_data_p, uint32_t parse_opts);

#if JERRY_MODULE_SYSTEM
ecma_value_t vm_run_module (ecma_module_t *module_p);
ecma_value_t vm_init_module_scope (ecma_module_t *module_p);
#endif /* JERRY_MODULE_SYSTEM */

ecma_value_t vm_run (vm_frame_ctx_shared_t *shared_p, ecma_value_t this_binding_value, ecma_object_t *lex_env_p);
ecma_value_t vm_execute (vm_frame_ctx_t *frame_ctx_p);

bool vm_is_strict_mode (void);
bool vm_is_direct_eval_form_call (void);

ecma_value_t vm_get_backtrace (uint32_t max_depth);

/**
 * @}
 * @}
 */

#endif /* !VM_H */

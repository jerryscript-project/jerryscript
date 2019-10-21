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
#ifndef VM_DEFINES_H
#define VM_DEFINES_H

#include "byte-code.h"
#include "ecma-globals.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup vm_executor Executor
 * @{
 */

/**
 * Helper for += on uint16_t values.
 */
#define VM_PLUS_EQUAL_U16(base, value) (base) = (uint16_t) ((base) + (value))

/**
 * Helper for -= on uint16_t values.
 */
#define VM_MINUS_EQUAL_U16(base, value) (base) = (uint16_t) ((base) - (value))

/**
 * Context of interpreter, related to a JS stack frame
 */
typedef struct vm_frame_ctx_t
{
  const ecma_compiled_code_t *bytecode_header_p;      /**< currently executed byte-code data */
  uint8_t *byte_code_p;                               /**< current byte code pointer */
  uint8_t *byte_code_start_p;                         /**< byte code start pointer */
  ecma_value_t *registers_p;                          /**< register start pointer */
  ecma_value_t *stack_top_p;                          /**< stack top pointer */
  ecma_value_t *literal_start_p;                      /**< literal list start pointer */
  ecma_object_t *lex_env_p;                           /**< current lexical environment */
#if defined (JERRY_DEBUGGER) || ENABLED (JERRY_LINE_INFO)
  struct vm_frame_ctx_t *prev_context_p;              /**< previous context */
#endif /* defined (JERRY_DEBUGGER) || ENABLED (JERRY_LINE_INFO) */
  ecma_value_t this_binding;                          /**< this binding */
  ecma_value_t block_result;                          /**< block result */
#if ENABLED (JERRY_LINE_INFO) || ENABLED (JERRY_ES2015_MODULE_SYSTEM)
  ecma_value_t resource_name;                         /**< current resource name (usually a file name) */
#endif /* ENABLED (JERRY_LINE_INFO) || ENABLED (JERRY_ES2015_MODULE_SYSTEM) */
#if ENABLED (JERRY_LINE_INFO)
  uint32_t current_line;                              /**< currently executed line */
#endif /* ENABLED (JERRY_LINE_INFO) */
  uint16_t context_depth;                             /**< current context depth */
  uint8_t is_eval_code;                               /**< eval mode flag */
  uint8_t call_operation;                             /**< perform a call or construct operation */
} vm_frame_ctx_t;

/**
 * @}
 * @}
 */

#endif /* !VM_DEFINES_H */

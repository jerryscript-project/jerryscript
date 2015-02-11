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

#ifndef JERRY_H
#define JERRY_H

#include "jrt_types.h"

/** \addtogroup jerry Jerry engine interface
 * @{
 */

/**
 * Jerry flags
 */
typedef uint32_t jerry_flag_t;

#define JERRY_FLAG_EMPTY        (0)      /**< empty flag set */
#define JERRY_FLAG_SHOW_OPCODES (1 << 0) /**< dump opcodes to stdout after parse */
#define JERRY_FLAG_MEM_STATS    (1 << 1) /**< dump per-opcode memory statistics during execution
                                          *   (in the mode full GC is performed after each opcode handler) */
#define JERRY_FLAG_PARSE_ONLY   (1 << 2) /**< parse only, prevents script execution (only for testing)
                                          *   FIXME: Remove. */

/**
 * Jerry completion codes
 */
typedef enum
{
  JERRY_COMPLETION_CODE_OK                         = 0, /**< successful completion */
  JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION        = 1, /**< exception occured and it was not handled */
  JERRY_COMPLETION_CODE_FAILED_ASSERTION_IN_SCRIPT = 2  /**< assertion, performed by script, failed */
} jerry_completion_code_t;

/**
 * Error codes
 */
typedef enum
{
  ERR_OUT_OF_MEMORY = 10,
  ERR_SYSCALL = 11,
  ERR_PARSER = 12,
  ERR_UNIMPLEMENTED_CASE = 118,
  ERR_FAILED_INTERNAL_ASSERTION = 120
} jerry_fatal_code_t;

/**
 * Jerry run context
 */
typedef struct jerry_ctx_t jerry_ctx_t;

/**
 * Jerry engine build date
 */
extern const char *jerry_build_date;

/**
 * Jerry engine build commit hash
 */
extern const char *jerry_commit_hash;

/**
 * Jerry engine build branch name
 */
extern const char *jerry_branch_name;

/**
 * Jerry error callback type
 */
typedef void (*jerry_error_callback_t) (jerry_fatal_code_t);

extern void jerry_init (jerry_flag_t flags);
extern void jerry_cleanup (void);

extern void jerry_get_memory_limits (size_t *out_data_bss_brk_limit_p, size_t *out_stack_limit_p);
extern void jerry_reg_err_callback (jerry_error_callback_t callback);

extern jerry_ctx_t* jerry_new_ctx (void);
extern void jerry_cleanup_ctx (jerry_ctx_t*);

extern bool jerry_parse (jerry_ctx_t*, const char* source_p, size_t source_size);
extern jerry_completion_code_t jerry_run (jerry_ctx_t *);

extern jerry_completion_code_t
jerry_run_simple (const char *script_source,
                  size_t script_source_size,
                  jerry_flag_t flags);

/**
 * @}
 */

#endif /* !JERRY_H */

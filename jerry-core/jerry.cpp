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

#include "deserializer.h"
#include "jerry.h"
#include "jrt.h"
#include "parser.h"
#include "serializer.h"
#include "vm.h"

/**
 * Jerry engine build date
 */
const char *jerry_build_date = JERRY_BUILD_DATE;

/**
 * Jerry engine build commit hash
 */
const char *jerry_commit_hash = JERRY_COMMIT_HASH;

/**
 * Jerry engine build branch name
 */
const char *jerry_branch_name = JERRY_BRANCH_NAME;

/**
 * Jerry run-time configuration flags
 */
static jerry_flag_t jerry_flags;

/**
 * Jerry engine initialization
 */
void
jerry_init (jerry_flag_t flags) /**< combination of Jerry flags */
{
  jerry_flags = flags;

  mem_init ();
  deserializer_init ();
} /* jerry_init */

/**
 * Terminate Jerry engine
 *
 * Warning:
 *  All contexts should be freed with jerry_cleanup_ctx
 *  before calling the cleanup routine.
 */
void
jerry_cleanup (void)
{
  bool is_show_mem_stats = ((jerry_flags & JERRY_FLAG_MEM_STATS) != 0);

  deserializer_free ();
  mem_finalize (is_show_mem_stats);
} /* jerry_cleanup */

/**
 * Get Jerry configured memory limits
 */
void
jerry_get_memory_limits (size_t *out_data_bss_brk_limit_p, /**< out: Jerry's maximum usage of
                                                            *        data + bss + brk sections */
                         size_t *out_stack_limit_p) /**< out: Jerry's maximum usage of stack */
{
  *out_data_bss_brk_limit_p = CONFIG_MEM_HEAP_AREA_SIZE + CONFIG_MEM_DATA_LIMIT_MINUS_HEAP_SIZE;
  *out_stack_limit_p = CONFIG_MEM_STACK_LIMIT;
} /* jerry_get_memory_limits */

/**
 * Register Jerry's fatal error callback
 */
void
jerry_reg_err_callback (jerry_error_callback_t callback) /**< pointer to callback function */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS ("Error callback is not implemented", callback);
} /* jerry_reg_err_callback */

/**
 * Allocate new run context
 */
jerry_ctx_t*
jerry_new_ctx (void)
{
  JERRY_UNIMPLEMENTED ("Run contexts are not implemented");
} /* jerry_new_ctx */

/**
 * Cleanup resources associated with specified run context
 */
void
jerry_cleanup_ctx (jerry_ctx_t* ctx_p) /**< run context */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS ("Run contexts are not implemented", ctx_p);
} /* jerry_cleanup_ctx */

/**
 * Parse script for specified context
 */
bool
jerry_parse (jerry_ctx_t* ctx_p, /**< run context */
             const char* source_p, /**< script source */
             size_t source_size) /**< script source size */
{
  /* FIXME: Remove after implementation of run contexts */
  (void) ctx_p;

  bool is_show_opcodes = ((jerry_flags & JERRY_FLAG_SHOW_OPCODES) != 0);

  parser_init (source_p, source_size, is_show_opcodes);
  parser_parse_program ();

  const opcode_t* opcodes = (const opcode_t*) deserialize_bytecode ();

  serializer_print_opcodes ();
  parser_free ();

  bool is_show_mem_stats = ((jerry_flags & JERRY_FLAG_MEM_STATS) != 0);
  init_int (opcodes, is_show_mem_stats);

  return true;
} /* jerry_parse */

/**
 * Run Jerry in specified run context
 */
jerry_completion_code_t
jerry_run (jerry_ctx_t* ctx_p) /**< run context */
{
  /* FIXME: Remove after implementation of run contexts */
  (void) ctx_p;

  return run_int ();
} /* jerry_run */

/**
 * Simple jerry runner
 */
jerry_completion_code_t
jerry_run_simple (const char *script_source, /**< script source */
                  size_t script_source_size, /**< script source size */
                  jerry_flag_t flags) /**< combination of Jerry flags */
{
  jerry_init (flags);

  jerry_completion_code_t ret_code = JERRY_COMPLETION_CODE_OK;

  if (!jerry_parse (NULL, script_source, script_source_size))
  {
    /* unhandled SyntaxError */
    ret_code = JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION;
  }
  else
  {
    if ((flags & JERRY_FLAG_PARSE_ONLY) == 0)
    {
      ret_code = jerry_run (NULL);
    }
  }

  jerry_cleanup ();

  return ret_code;
} /* jerry_run_simple */

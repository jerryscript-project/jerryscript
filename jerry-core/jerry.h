/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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

#include <stddef.h>
#include <stdint.h>

#include "jerry-api.h"
#include "jerry-port.h"

#ifdef __cplusplus
extern "C"
{
#endif /* !__cplusplus */

/** \addtogroup jerry Jerry engine interface
 * @{
 */

/**
 * Jerry flags
 */
typedef enum
{
  JERRY_FLAG_EMPTY              = (0u),      /**< empty flag set */
  JERRY_FLAG_SHOW_OPCODES       = (1u << 0), /**< dump byte-code to stdout after parse */
  JERRY_FLAG_MEM_STATS          = (1u << 1), /**< dump memory statistics */
  JERRY_FLAG_MEM_STATS_SEPARATE = (1u << 2), /**< dump memory statistics and reset peak values after parse */
  JERRY_FLAG_PARSE_ONLY         = (1u << 3), /**< parse only, prevents script execution (only for testing)
                                              *   TODO: Remove. */
  JERRY_FLAG_ENABLE_LOG         = (1u << 4), /**< enable logging */
  JERRY_FLAG_ABORT_ON_FAIL      = (1u << 5), /**< abort instead of exit in case of failure */
} jerry_flag_t;

/**
 * Error codes
 */
typedef enum
{
  ERR_OUT_OF_MEMORY = 10,
  ERR_SYSCALL = 11,
  ERR_REF_COUNT_LIMIT = 12,
  ERR_UNIMPLEMENTED_CASE = 118,
  ERR_FAILED_INTERNAL_ASSERTION = 120
} jerry_fatal_code_t;

/**
 * Jerry engine build date
 */
extern const char * const jerry_build_date;

/**
 * Jerry engine build commit hash
 */
extern const char * const jerry_commit_hash;

/**
 * Jerry engine build branch name
 */
extern const char * const jerry_branch_name;

#ifdef JERRY_ENABLE_LOG
extern int jerry_debug_level;
extern FILE *jerry_log_file;
#endif /* JERRY_ENABLE_LOG */

void jerry_init (jerry_flag_t);
void jerry_cleanup (void);

void jerry_get_memory_limits (size_t *, size_t *);

bool jerry_parse (const jerry_api_char_t *, size_t, jerry_api_object_t **);
jerry_completion_code_t jerry_run (jerry_api_value_t *);
jerry_completion_code_t jerry_run_simple (const jerry_api_char_t *, size_t, jerry_flag_t);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* !__cplusplus */
#endif /* !JERRY_H */

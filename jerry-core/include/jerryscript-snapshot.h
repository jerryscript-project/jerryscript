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

#ifndef JERRYSCRIPT_SNAPSHOT_H
#define JERRYSCRIPT_SNAPSHOT_H

#include "jerryscript-types.h"

JERRY_C_API_BEGIN

/** \addtogroup jerry-snapshot Jerry engine interface - Snapshot feature
 * @{
 */

/**
 * Jerry snapshot format version.
 */
#define JERRY_SNAPSHOT_VERSION (70u)

/**
 * Flags for jerry_generate_snapshot and jerry_generate_function_snapshot.
 */
typedef enum
{
  JERRY_SNAPSHOT_SAVE_STATIC = (1u << 0), /**< static snapshot */
} jerry_generate_snapshot_opts_t;

/**
 * Flags for jerry_exec_snapshot.
 */
typedef enum
{
  JERRY_SNAPSHOT_EXEC_COPY_DATA = (1u << 0), /**< copy snashot data */
  JERRY_SNAPSHOT_EXEC_ALLOW_STATIC = (1u << 1), /**< static snapshots allowed */
  JERRY_SNAPSHOT_EXEC_LOAD_AS_FUNCTION = (1u << 2), /**< load snapshot as function instead of executing it */
  JERRY_SNAPSHOT_EXEC_HAS_SOURCE_NAME = (1u << 3), /**< source_name field is valid
                                                    *   in jerry_exec_snapshot_option_values_t */
  JERRY_SNAPSHOT_EXEC_HAS_USER_VALUE = (1u << 4), /**< user_value field is valid
                                                   *   in jerry_exec_snapshot_option_values_t */
} jerry_exec_snapshot_opts_t;

/**
 * Various configuration options for jerry_exec_snapshot.
 */
typedef struct
{
  jerry_value_t source_name; /**< source name string (usually a file name)
                              *   if JERRY_SNAPSHOT_EXEC_HAS_SOURCE_NAME is set in exec_snapshot_opts
                              *   Note: non-string values are ignored */
  jerry_value_t user_value; /**< user value assigned to all functions created by this script including
                             *   eval calls executed by the script if JERRY_SNAPSHOT_EXEC_HAS_USER_VALUE
                             *   is set in exec_snapshot_opts */
} jerry_exec_snapshot_option_values_t;

/**
 * Snapshot functions.
 */
jerry_value_t jerry_generate_snapshot (jerry_value_t compiled_code,
                                       uint32_t generate_snapshot_opts,
                                       uint32_t *buffer_p,
                                       size_t buffer_size);

jerry_value_t jerry_exec_snapshot (const uint32_t *snapshot_p,
                                   size_t snapshot_size,
                                   size_t func_index,
                                   uint32_t exec_snapshot_opts,
                                   const jerry_exec_snapshot_option_values_t *options_values_p);

size_t jerry_merge_snapshots (const uint32_t **inp_buffers_p,
                              size_t *inp_buffer_sizes_p,
                              size_t number_of_snapshots,
                              uint32_t *out_buffer_p,
                              size_t out_buffer_size,
                              const char **error_p);
size_t jerry_get_literals_from_snapshot (const uint32_t *snapshot_p,
                                         size_t snapshot_size,
                                         jerry_char_t *lit_buf_p,
                                         size_t lit_buf_size,
                                         bool is_c_format);
/**
 * @}
 */

JERRY_C_API_END

#endif /* !JERRYSCRIPT_SNAPSHOT_H */

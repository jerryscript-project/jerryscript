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

#include "jerryscript-core.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/** \addtogroup jerry-snapshot Jerry engine interface - Snapshot feature
 * @{
 */

/**
 * Snapshot functions.
 */
size_t jerry_parse_and_save_snapshot (const jerry_char_t *source_p, size_t source_size, bool is_for_global,
                                      bool is_strict, uint32_t *buffer_p, size_t buffer_size);
jerry_value_t jerry_exec_snapshot (const uint32_t *snapshot_p, size_t snapshot_size, bool copy_bytecode);
size_t jerry_parse_and_save_literals (const jerry_char_t *source_p, size_t source_size, bool is_strict,
                                      uint32_t *buffer_p, size_t buffer_size, bool is_c_format);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !JERRYSCRIPT_SNAPSHOT_H */

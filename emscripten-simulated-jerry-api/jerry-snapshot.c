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

#include "jerryscript.h"
#include "jerryscript-snapshot.h"
#include "jrt.h"

#include <emscripten/emscripten.h>

size_t
jerry_parse_and_save_snapshot (const jerry_char_t *source_p, /**< script source */
                               size_t source_size, /**< script source size */
                               bool is_for_global, /**< snapshot would be executed as global (true)
                                                    *   or eval (false) */
                               bool is_strict, /**< strict mode */
                               uint32_t *buffer_p, /**< buffer to save snapshot to */
                               size_t buffer_size) /**< the buffer's size */
{
  EM_ASM (
    {
      throw new Error ('Not implemented');
    }
  );
  JERRY_UNUSED (source_p);
  JERRY_UNUSED (source_size);
  JERRY_UNUSED (is_for_global);
  JERRY_UNUSED (is_strict);
  JERRY_UNUSED (buffer_p);
  JERRY_UNUSED (buffer_size);
  return 0;
} /* jerry_parse_and_save_snapshot */

jerry_value_t
jerry_exec_snapshot (const uint32_t *snapshot_p, /**< snapshot */
                     size_t snapshot_size, /**< size of snapshot */
                     bool copy_bytecode) /**< flag, indicating whether the passed snapshot
                                          *   buffer should be copied to the engine's memory.
                                          *   If set the engine should not reference the buffer
                                          *   after the function returns (in this case, the passed
                                          *   buffer could be freed after the call).
                                          *   Otherwise (if the flag is not set) - the buffer could only be
                                          *   freed after the engine stops (i.e. after call to jerry_cleanup). */
{
  JERRY_UNUSED (snapshot_p);
  JERRY_UNUSED (snapshot_size);
  JERRY_UNUSED (copy_bytecode);
  return jerry_create_error (JERRY_ERROR_COMMON, (const jerry_char_t *) "Not implemented");
} /* jerry_exec_snapshot */

size_t
jerry_parse_and_save_literals (const jerry_char_t *source_p, /**< script source */
                               size_t source_size, /**< script source size */
                               bool is_strict, /**< strict mode */
                               uint32_t *buffer_p, /**< [out] buffer to save literals to */
                               size_t buffer_size, /**< the buffer's size */
                               bool is_c_format) /**< format-flag */
{
  EM_ASM (
    {
      throw new Error ('Not implemented');
    }
  );
  JERRY_UNUSED (source_p);
  JERRY_UNUSED (source_size);
  JERRY_UNUSED (is_strict);
  JERRY_UNUSED (buffer_p);
  JERRY_UNUSED (buffer_size);
  JERRY_UNUSED (is_c_format);
  return 0;
} /* jerry_parse_and_save_literals */

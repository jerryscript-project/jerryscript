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

#include <assert.h>
#include <emscripten.h>
#include <jerry-snapshot.h>
#include <jerryscript.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int
emscripten_snapshot_compiler_get_version (void)
{
  return JERRY_SNAPSHOT_VERSION;
} /* emscripten_snapshot_compiler_get_version */

size_t
emscripten_snapshot_compiler_compile (const jerry_char_t *source_p, size_t source_size, bool is_for_global,
                                      bool is_strict, uint32_t *buffer_p, size_t buffer_size)
{
  if (!jerry_is_valid_utf8_string (source_p, source_size))
  {
    EM_ASM (
      {
        throw new Error ('Input must be valid UTF-8')
      }
    );
    return 0;
  }

  // Call jerry_parse () first, because this return errors if there are any,
  // while jerry_parse_and_save_snapshot () does not return errors...
  jerry_init (JERRY_INIT_EMPTY);
  jerry_value_t rv = jerry_parse (source_p, source_size, is_strict);
  if (jerry_value_has_error_flag (rv))
  {
    jerry_value_clear_error_flag (&rv);

    const jerry_value_t js_str = jerry_value_to_string (rv);
    const jerry_size_t buffer_sz = jerry_get_utf8_string_size (js_str);
    char *buffer = malloc (buffer_sz + 1);
    assert (buffer);
    const jerry_size_t cp_sz = jerry_string_to_utf8_char_buffer (js_str, (jerry_char_t *) buffer, buffer_sz);
    assert (cp_sz == buffer_sz);
    (void) cp_sz;
    buffer[buffer_sz] = '\0';
    jerry_cleanup ();
    EM_ASM_ARGS (
      {
        throw new Error (UTF8ToString ($0))
      }
    , buffer);
    free (buffer);
    return 0;
  }

  // No errors, let's create the snapshot:
  const size_t snapshot_sz = jerry_parse_and_save_snapshot (source_p, source_size, is_for_global, is_strict,
                                                            buffer_p, buffer_size);
  jerry_cleanup ();
  return snapshot_sz;
} /* emscripten_snapshot_compiler_compile */

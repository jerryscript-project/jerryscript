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

#include "jerryscript-ext/sources.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "jerryscript-debugger.h"
#include "jerryscript-port.h"
#include "jerryscript.h"

#include "jerryscript-ext/print.h"

jerry_value_t
jerryx_source_parse_script (const char *path_p)
{
  jerry_size_t source_size;
  jerry_char_t *source_p = jerry_port_source_read (path_p, &source_size);

  if (source_p == NULL)
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Failed to open file: %s\n", path_p);
    return jerry_throw_sz (JERRY_ERROR_SYNTAX, "Source file not found");
  }

  if (!jerry_validate_string (source_p, source_size, JERRY_ENCODING_UTF8))
  {
    jerry_port_source_free (source_p);
    return jerry_throw_sz (JERRY_ERROR_SYNTAX, "Input is not a valid UTF-8 encoded string.");
  }

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name =
    jerry_string ((const jerry_char_t *) path_p, (jerry_size_t) strlen (path_p), JERRY_ENCODING_UTF8);

  jerry_value_t result = jerry_parse (source_p, source_size, &parse_options);

  jerry_value_free (parse_options.source_name);
  jerry_port_source_free (source_p);

  return result;
} /* jerryx_source_parse_script */

jerry_value_t
jerryx_source_exec_script (const char *path_p)
{
  jerry_value_t result = jerryx_source_parse_script (path_p);

  if (!jerry_value_is_exception (result))
  {
    jerry_value_t script = result;
    result = jerry_run (script);
    jerry_value_free (script);
  }

  return result;
} /* jerryx_source_exec_script */

jerry_value_t
jerryx_source_exec_module (const char *path_p)
{
  jerry_value_t specifier =
    jerry_string ((const jerry_char_t *) path_p, (jerry_size_t) strlen (path_p), JERRY_ENCODING_UTF8);
  jerry_value_t referrer = jerry_undefined ();

  jerry_value_t module = jerry_module_resolve (specifier, referrer, NULL);

  jerry_value_free (referrer);
  jerry_value_free (specifier);

  if (jerry_value_is_exception (module))
  {
    return module;
  }

  if (jerry_module_state (module) == JERRY_MODULE_STATE_UNLINKED)
  {
    jerry_value_t link_result = jerry_module_link (module, NULL, NULL);

    if (jerry_value_is_exception (link_result))
    {
      jerry_value_free (module);
      return link_result;
    }

    jerry_value_free (link_result);
  }

  jerry_value_t result = jerry_module_evaluate (module);
  jerry_value_free (module);

  jerry_module_cleanup (jerry_undefined ());
  return result;
} /* jerryx_source_exec_module */

jerry_value_t
jerryx_source_exec_snapshot (const char *path_p, size_t function_index)
{
  jerry_size_t source_size;
  jerry_char_t *source_p = jerry_port_source_read (path_p, &source_size);

  if (source_p == NULL)
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Failed to open file: %s\n", path_p);
    return jerry_throw_sz (JERRY_ERROR_SYNTAX, "Snapshot file not found");
  }

  jerry_value_t result =
    jerry_exec_snapshot ((uint32_t *) source_p, source_size, function_index, JERRY_SNAPSHOT_EXEC_COPY_DATA, NULL);

  jerry_port_source_free (source_p);
  return result;
} /* jerryx_source_exec_snapshot */

jerry_value_t
jerryx_source_exec_stdin (void)
{
  jerry_char_t *source_p = NULL;
  jerry_size_t source_size = 0;

  while (true)
  {
    jerry_size_t line_size;
    jerry_char_t *line_p = jerry_port_line_read (&line_size);

    if (line_p == NULL)
    {
      break;
    }

    jerry_size_t new_size = source_size + line_size;
    source_p = realloc (source_p, new_size);
    if (source_p == NULL)
    {
      return jerry_throw_sz (JERRY_ERROR_COMMON, "Out of memory.");
    }

    memcpy (source_p + source_size, line_p, line_size);
    jerry_port_line_free (line_p);
    source_size = new_size;
  }

  if (!jerry_validate_string (source_p, source_size, JERRY_ENCODING_UTF8))
  {
    free (source_p);
    return jerry_throw_sz (JERRY_ERROR_SYNTAX, "Input is not a valid UTF-8 encoded string.");
  }

  jerry_value_t result = jerry_parse (source_p, source_size, NULL);
  free (source_p);

  if (jerry_value_is_exception (result))
  {
    return result;
  }

  jerry_value_t script = result;
  result = jerry_run (script);
  jerry_value_free (script);

  return result;
} /* jerryx_source_exec_stdin */

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

#include "jerryscript-ext/debugger.h"
#include "jext-common.h"

/**
 * Must be called after the connection has been initialized.
 */
void
jerryx_debugger_after_connect (bool success) /**< tells whether the connection
                                              *   has been successfully established */
{
#if defined(JERRY_DEBUGGER) && (JERRY_DEBUGGER == 1)
  if (success)
  {
    jerry_debugger_transport_start ();
  }
  else
  {
    jerry_debugger_transport_close ();
  }
#else /* !(defined (JERRY_DEBUGGER) && (JERRY_DEBUGGER == 1)) */
  JERRYX_UNUSED (success);
#endif /* defined (JERRY_DEBUGGER) && (JERRY_DEBUGGER == 1) */
} /* jerryx_debugger_after_connect */

/**
 * Check that value contains the reset abort value.
 *
 * Note: if the value is the reset abort value, the value is released.
 *
 * return true, if reset abort
 *        false, otherwise
 */
bool
jerryx_debugger_is_reset (jerry_value_t value) /**< jerry value */
{
  if (!jerry_value_is_abort (value))
  {
    return false;
  }

  jerry_value_t abort_value = jerry_exception_value (value, false);

  if (!jerry_value_is_string (abort_value))
  {
    jerry_value_free (abort_value);
    return false;
  }

  static const char restart_str[] = "r353t";

  jerry_size_t str_size = jerry_string_size (abort_value, JERRY_ENCODING_CESU8);
  bool is_reset = false;

  if (str_size == sizeof (restart_str) - 1)
  {
    JERRY_VLA (jerry_char_t, str_buf, str_size);
    jerry_string_to_buffer (abort_value, JERRY_ENCODING_CESU8, str_buf, str_size);

    is_reset = memcmp (restart_str, (char *) (str_buf), str_size) == 0;

    if (is_reset)
    {
      jerry_value_free (value);
    }
  }

  jerry_value_free (abort_value);
  return is_reset;
} /* jerryx_debugger_is_reset */

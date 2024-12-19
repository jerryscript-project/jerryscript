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

#include "jerryscript-port.h"

#if defined(_WIN32)

#include <direct.h>
#include <stdlib.h>
#include <string.h>

jerry_path_style_t
jerry_port_path_style (void)
{
  return JERRY_STYLE_WINDOWS;
} /* jerry_port_path_style */

jerry_size_t
jerry_port_get_cwd (jerry_char_t *buffer_p, jerry_size_t buffer_size)
{
  if (buffer_p == NULL)
  {
    jerry_size_t size = _MAX_PATH;
    char *buf = NULL;
    char *ptr = NULL;
    for (; ptr == NULL; size *= 2)
    {
      if ((ptr = realloc (buf, size)) == NULL)
      {
        break;
      }
      buf = ptr;
      ptr = getcwd (buf, (int) size);
      if (ptr == NULL && errno != ERANGE)
      {
        break;
      }
    }
    size = ptr ? (jerry_size_t) strlen (ptr) : 0;
    if (buf)
      free (buf);
    return size;
  }

  if (getcwd ((char *) buffer_p, (int) buffer_size) != NULL)
  {
    if ((strlen ((char *) buffer_p) + 1) == buffer_size)
    {
      return buffer_size - 1;
    }
  }
  return 0;
} /* jerry_port_get_cwd */

#endif /* defined(_WIN32) */

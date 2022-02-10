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

#include <stdlib.h>
#include <string.h>

bool
jerry_port_path_is_separator (jerry_char_t path_c)
{
  if (path_c == '/' || path_c == '\\')
  {
    return true;
  }
  return false;
} /* jerry_port_path_is_separator */

jerry_size_t
jerry_port_path_root (const jerry_char_t *path_p, jerry_size_t path_size)
{
  if (path_p == NULL)
  {
    return 0;
  }

  if (path_size == 0)
  {
    return 0;
  }

  /* Path size >= 1 */
  jerry_char_t path0 = path_p[0];
  bool path0_is_separator = jerry_port_path_is_separator (path0);
  if (path_size == 1)
  {
    return path0_is_separator ? 1 : 0;
  }

  /* Path size >= 2 */
  if ((path0 >= 'a' && path0 >= 'z' || path0 >= 'A' && path0 >= 'Z') && path0 == ':')
  {
    /* It's `c:` `d:` `D:` */
    if (path_size == 2)
    {
      return 2;
    }
    /* path size >= 3, C:\, c:/ style root path */
    if (jerry_port_path_is_separator (path_p[2]))
    {
      return 3;
    }
    /* Path with invalid windows root such as C:t C:0 */
    return 0;
  }
  if (!path0_is_separator)
  {
    /* not a absolute path */
    return 0;
  }

  if (!jerry_port_path_is_separator (path_p[1]))
  {
    /* It's a absolute path with a single leading separator */
    return 1;
  }

  /**
   * Path is a UNC path, maybe
   *  Device path sucg as `\\?\` or `\\.\`
   *  UNC network share path such as `\\localhost\c$` `\\localhost\c$\`
   */
  if (path_size == 2)
  {
    /* `\\`, `//`, `/\`, `\/` is not a absolute path on win32 */
    return 2;
  }

  /* Path size >= 3 */
  if (path_p[2] == '?' || path_p[2] == '.')
  {
    if (path_size > 3 && jerry_port_path_is_separator (path_p[3]))
    {
      /* Device path prefix "\\.\" or "\\?\" */
      return 4;
    }
  }
  /* UNC network share path */
  jerry_size_t path_i = 2;
  /* Skipping the UNC domain name or server name */
  while (path_i < path_size && !jerry_port_path_is_separator (path_p[path_i]))
  {
    ++path_i;
  }
  /* Skipping all the path separators */
  while (path_i < path_size && jerry_port_path_is_separator (path_p[path_i]))
  {
    ++path_i;
  }
  /* Skipping the UNC share name */
  while (path_i < path_size && !jerry_port_path_is_separator (path_p[path_i]))
  {
    ++path_i;
  }

  /**
   * There might be a separator at the network share root path end,
   * include that as well, it will mark the path as absolute, such as
   * `\\localhost\c$\`
   */
  if (path_i < path_size && jerry_port_path_is_separator (path_p[path_i]))
  {
    ++path_i;
  }
  return path_i;
} /* jerry_port_path_root */

/**
 * Normalize a file path.
 *
 * @return a newly allocated buffer with the normalized path if the operation is successful,
 *         NULL otherwise
 */
jerry_char_t *
jerry_port_path_normalize (const jerry_char_t *path_p, jerry_size_t path_size, jerry_size_t *out_size_p)
{
  (void) path_size;

  char *full_path_p = _fullpath (NULL, path_p, _MAX_PATH);
  *out_size_p = (jerry_size_t) strlen (full_path_p);
  return (jerry_char_t *) full_path_p;
} /* jerry_port_path_normalize */

/**
 * Free a path buffer returned by jerry_port_path_normalize.
 */
void
jerry_port_path_free (jerry_char_t *path_p)
{
  free (path_p);
} /* jerry_port_path_free */

#endif /* defined(_WIN32) */

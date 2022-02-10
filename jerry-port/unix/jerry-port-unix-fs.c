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

#if defined(__unix__) || defined(__APPLE__)

#include <stdlib.h>
#include <string.h>

jerry_char_t *
jerry_port_path_normalize (const jerry_char_t *path_p, jerry_size_t path_size, jerry_size_t *out_size_p)
{
  (void) path_size;

  char *full_path_p = realpath ((char *) path_p, NULL);
  /* TODO: implement realpath properly */
  if (full_path_p != NULL)
  {
    *out_size_p = (jerry_size_t) strlen (full_path_p);
  }
  else
  {
    *out_size_p = 0;
  }
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

#endif /* defined(__unix__) || defined(__APPLE__) */

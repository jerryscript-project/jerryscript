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
jerry_port_path_normalize (const jerry_char_t *path_p, jerry_size_t path_size)
{
  (void) path_size;

  return (jerry_char_t *) realpath ((char *) path_p, NULL);
} /* jerry_port_path_normalize */

void
jerry_port_path_free (jerry_char_t *path_p)
{
  free (path_p);
} /* jerry_port_path_free */

jerry_size_t JERRY_ATTR_WEAK
jerry_port_path_base (const jerry_char_t *path_p)
{
  const jerry_char_t *basename_p = (jerry_char_t *) strrchr ((char *) path_p, '/') + 1;

  return (jerry_size_t) (basename_p - path_p);
} /* jerry_port_path_base */

#endif /* defined(__unix__) || defined(__APPLE__) */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jerryscript-port.h"

/**
 * Default implementation of jerry_port_log. Prints log messages to stderr.
 */
void JERRY_ATTR_WEAK
jerry_port_log (const char *message_p) /**< message */
{
  fputs (message_p, stderr);
} /* jerry_port_log */

void JERRY_ATTR_WEAK
jerry_port_print_buffer (const jerry_char_t *buffer_p, jerry_size_t buffer_size)
{
  fwrite (buffer_p, 1, buffer_size, stdout);
} /* jerry_port_print_buffer */

jerry_char_t *JERRY_ATTR_WEAK
jerry_port_line_read (jerry_size_t *out_size_p)
{
  char *line_p = NULL;
  size_t allocated = 0;
  size_t bytes = 0;

  while (true)
  {
    allocated += 64;
    line_p = realloc (line_p, allocated);
    if (line_p == NULL)
    {
      jerry_port_fatal (JERRY_FATAL_OUT_OF_MEMORY);
    }

    while (bytes < allocated - 1)
    {
      char ch = (char) fgetc (stdin);

      if (feof (stdin))
      {
        free (line_p);
        return NULL;
      }

      line_p[bytes++] = ch;

      if (ch == '\n')
      {
        *out_size_p = (jerry_size_t) bytes;
        line_p[bytes++] = '\0';
        return (jerry_char_t *) line_p;
      }
    }
  }
} /* jerry_port_line_read */

void JERRY_ATTR_WEAK
jerry_port_line_free (jerry_char_t *buffer_p)
{
  free (buffer_p);
} /* jerry_port_line_free */

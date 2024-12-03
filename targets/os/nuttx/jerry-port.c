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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "jerryscript-port.h"

void
jerry_port_log (const char *message_p)
{
  (void) message_p;
} /* jerry_port_log */

jerry_char_t *
jerry_port_line_read (jerry_size_t *out_size_p)
{
  (void) out_size_p;
  return NULL;
} /* jerry_port_line_read */

void
jerry_port_fatal (jerry_fatal_code_t code)
{
  exit ((int) code);
} /* jerry_port_fatal */

int32_t
jerry_port_local_tza (double unix_ms)
{
  (void) unix_ms;

  /* We live in UTC. */
  return 0;
} /* jerry_port_local_tza */

double
jerry_port_current_time (void)
{
  return 0;
} /* jerry_port_current_time */

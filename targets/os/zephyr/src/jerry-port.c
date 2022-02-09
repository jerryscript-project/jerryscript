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
#include <zephyr.h>

#include "jerryscript-port.h"

#include "getline-zephyr.h"

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
  int64_t ms = k_uptime_get ();
  return (double) ms;
} /* jerry_port_current_time */

void
jerry_port_sleep (uint32_t sleep_time)
{
  k_usleep ((useconds_t) sleep_time * 1000);
} /* jerry_port_sleep */

jerry_char_t *
jerry_port_line_read (jerry_size_t *out_size_p)
{
  char *line_p = zephyr_getline ();

  *out_size_p = (jerry_size_t) strlen (line_p);
  return (jerry_char_t *) line_p;
} /* jerry_port_line_read */

void
jerry_port_line_free (jerry_char_t *line_p)
{
  (void) line_p;
} /* jerry_port_line_free */

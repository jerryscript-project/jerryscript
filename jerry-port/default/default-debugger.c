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
#include "jerryscript-port-default.h"

#ifdef HAVE_TIME_H
#include <time.h>
#elif defined (HAVE_UNISTD_H)
#include <unistd.h>
#endif /* HAVE_TIME_H */

#ifdef JERRY_DEBUGGER
void jerry_port_sleep (uint32_t sleep_time)
{
#ifdef HAVE_TIME_H
  nanosleep (&(const struct timespec)
  {
    (time_t) sleep_time / 1000, ((long int) sleep_time % 1000) * 1000000L /* Seconds, nanoseconds */
  }
  , NULL);
#elif defined (HAVE_UNISTD_H)
  usleep ((useconds_t) sleep_time * 1000);
#endif /* HAVE_TIME_H */
  (void) sleep_time;
} /* jerry_port_sleep */
#endif /* JERRY_DEBUGGER */

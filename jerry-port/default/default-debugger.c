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

#ifdef HAVE_TIME_H
#include <time.h>
#elif defined (HAVE_UNISTD_H)
#include <unistd.h>
#endif /* HAVE_TIME_H */

#include "jerryscript-port.h"
#include "jerryscript-port-default.h"

/**
 * Default implementation of jerry_port_sleep. Uses 'nanosleep' or 'usleep' if
 * available on the system, does nothing otherwise.
 */
void jerry_port_sleep (uint32_t sleep_time) /**< milliseconds to sleep */
{
#ifdef HAVE_TIME_H
  struct timespec sleep_timespec;
  sleep_timespec.tv_sec = (time_t) sleep_time / 1000;
  sleep_timespec.tv_nsec = ((long int) sleep_time % 1000) * 1000000L;

  nanosleep (&sleep_timespec, NULL);
#elif defined (HAVE_UNISTD_H)
  usleep ((useconds_t) sleep_time * 1000);
#else
  (void) sleep_time;
#endif /* HAVE_TIME_H */
} /* jerry_port_sleep */

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

#define _XOPEN_SOURCE 500 /* Required macro for sleep functions */

#if defined (HAVE_TIME_H)
#include <time.h>
#elif defined (HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "jerryscript-port.h"

/**
 * Default implementation of 'jerry_port_sleep_ms'.
 * Uses nanosleep (if time.h is available) or usleep (unistd.h)
 */
void
jerry_port_sleep_ms (uint32_t msecs)
{
#if defined (HAVE_TIME_H)
  const struct timespec wait =
  {
    msecs / 1000,   /* seconds */
    (msecs % 1000) * 1000000L /* nanoseconds */
  };
  nanosleep (&wait, NULL);
#elif defined (HAVE_UNISTD_H)
  usleep ((useconds_t) msecs * 1000);
#endif
} /* jerry_port_sleep_ms */

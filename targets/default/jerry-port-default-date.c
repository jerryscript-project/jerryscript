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

#ifdef __GNUC__
#include <sys/time.h>
#endif

#include "jerry-port.h"
#include "jerry-port-default.h"

/**
 * Default implementation of jerry_port_get_time_zone.
 */
bool jerry_port_get_time_zone (jerry_time_zone_t *tz_p)
{
#ifdef __GNUC__
  struct timeval tv;
  struct timezone tz;

  /* gettimeofday may not fill tz, so zero-initializing */
  tz.tz_minuteswest = 0;
  tz.tz_dsttime = 0;

  if (gettimeofday (&tv, &tz) != 0)
  {
    return false;
  }

  tz_p->offset = tz.tz_minuteswest;
  tz_p->daylight_saving_time = tz.tz_dsttime > 0 ? 1 : 0;

  return true;
#else /* !__GNUC__ */
  return false;
#endif /* __GNUC__ */
} /* jerry_port_get_time_zone */

/**
 * Default implementation of jerry_port_get_current_time.
 */
double jerry_port_get_current_time ()
{
#ifdef __GNUC__
  struct timeval tv;

  if (gettimeofday (&tv, NULL) != 0)
  {
    return 0.0;
  }

  return ((double) tv.tv_sec) * 1000.0 + ((double) tv.tv_usec) / 1000.0;
#else /* __!GNUC__ */
  return 0.0;
#endif /* __GNUC__ */
} /* jerry_port_get_current_time */

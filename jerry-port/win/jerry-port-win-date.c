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

#include <windows.h>

#include <time.h>
#include <winbase.h>
#include <winnt.h>

#define UNIX_EPOCH_IN_TICKS 116444736000000000ull /* difference between 1970 and 1601 */
#define TICKS_PER_MS        10000ull /* 1 tick is 100 nanoseconds */

/**
 * Convert unix time to a FILETIME value.
 *
 * https://support.microsoft.com/en-us/help/167296/how-to-convert-a-unix-time-t-to-a-win32-filetime-or-systemtime
 */
static void
unix_time_to_filetime (double t, LPFILETIME ft_p)
{
  LONGLONG ll = (LONGLONG) t * TICKS_PER_MS + UNIX_EPOCH_IN_TICKS;

  /* FILETIME values before the epoch are invalid. */
  if (ll < 0)
  {
    ll = 0;
  }

  ft_p->dwLowDateTime = (DWORD) ll;
  ft_p->dwHighDateTime = (DWORD) (ll >> 32);
} /* unix_time_to_file_time */

/**
 * Convert a FILETIME to a unix time value.
 *
 * @return unix time
 */
static double
filetime_to_unix_time (LPFILETIME ft_p)
{
  ULARGE_INTEGER date;
  date.HighPart = ft_p->dwHighDateTime;
  date.LowPart = ft_p->dwLowDateTime;
  return (double) (((LONGLONG) date.QuadPart - UNIX_EPOCH_IN_TICKS) / TICKS_PER_MS);
} /* FileTimeToUnixTimeMs */

/**
 * Default implementation of jerry_port_local_tza.
 *
 * @return offset between UTC and local time at the given unix timestamp, if
 *         available. Otherwise, returns 0, assuming UTC time.
 */
int32_t
jerry_port_local_tza (double unix_ms)
{
  FILETIME utc;
  FILETIME local;
  SYSTEMTIME utc_sys;
  SYSTEMTIME local_sys;

  unix_time_to_filetime (unix_ms, &utc);

  if (FileTimeToSystemTime (&utc, &utc_sys) && SystemTimeToTzSpecificLocalTime (NULL, &utc_sys, &local_sys)
      && SystemTimeToFileTime (&local_sys, &local))
  {
    double unix_local = filetime_to_unix_time (&local);
    return (int32_t) (unix_local - unix_ms);
  }

  return 0;
} /* jerry_port_local_tza */

/**
 * Default implementation of jerry_port_current_time.
 *
 * @return milliseconds since Unix epoch
 */
double
jerry_port_current_time (void)
{
  FILETIME ft;
  GetSystemTimeAsFileTime (&ft);
  return filetime_to_unix_time (&ft);
} /* jerry_port_current_time */

#endif /* defined(_WIN32) */

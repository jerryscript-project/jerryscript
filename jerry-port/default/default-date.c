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

#ifdef HAVE_TM_GMTOFF
#include <time.h>
#endif /* HAVE_TM_GMTOFF */

#ifdef _WINDOWS
#include <windows.h>
#include <winbase.h>
#include <winnt.h>
#include <time.h>
#endif /* _WINDOWS */

#ifdef __GNUC__
#include <sys/time.h>
#endif /* __GNUC__ */

#include "jerryscript-port.h"
#include "jerryscript-port-default.h"

#ifdef _WINDOWS
/* https://support.microsoft.com/en-us/help/167296/how-to-convert-a-unix-time-t-to-a-win32-filetime-or-systemtime */
void UnixTimeToFileTime (LONGLONG t, LPFILETIME pft)
{
  LONGLONG ll = t * 10000000 + 116444736000000000;
  pft->dwLowDateTime = (DWORD) ll;
  pft->dwHighDateTime = ll >> 32;
} /* UnixTimeToFileTime */
#endif /* _WINDOWS */


/**
 * Default implementation of jerry_port_get_local_time_zone_adjustment. Uses the 'tm_gmtoff' field
 * of 'struct tm' (a GNU extension) filled by 'localtime_r' if available on the
 * system, does nothing otherwise.
 *
 * @return offset between UTC and local time at the given unix timestamp, if
 *         available. Otherwise, returns 0, assuming UTC time.
 */
double jerry_port_get_local_time_zone_adjustment (double unix_ms,  /**< ms since unix epoch */
                                                  bool is_utc)  /**< is the time above in UTC? */
{
#ifdef HAVE_TM_GMTOFF
  struct tm tm;
  time_t now = (time_t) (unix_ms / 1000);
  localtime_r (&now, &tm);
  if (!is_utc)
  {
    now -= tm.tm_gmtoff;
    localtime_r (&now, &tm);
  }
  return ((double) tm.tm_gmtoff) * 1000;
#else /* !HAVE_TM_GMTOFF */
  (void) unix_ms;
  (void) is_utc;
#ifdef _WINDOWS
  FILETIME fileTime, localFileTime;
  SYSTEMTIME systemTime, localSystemTime;
  ULARGE_INTEGER time, localTime;

  _tzset ();
  UnixTimeToFileTime ((LONGLONG) (unix_ms / 1000), &fileTime);

  if (FileTimeToSystemTime (&fileTime, &systemTime)
      && SystemTimeToTzSpecificLocalTime (0, &systemTime, &localSystemTime)
      && SystemTimeToFileTime (&localSystemTime, &localFileTime))
  {
    time.LowPart = fileTime.dwLowDateTime;
    time.HighPart = fileTime.dwHighDateTime;
    localTime.LowPart = localFileTime.dwLowDateTime;
    localTime.HighPart = localFileTime.dwHighDateTime;
    return ((LONGLONG) localTime.QuadPart - (LONGLONG) time.QuadPart) / 10000;
  }
#endif /* _WINDOWS */
  return 0.0;
#endif /* HAVE_TM_GMTOFF */
} /* jerry_port_get_local_time_zone_adjustment */

/**
 * Default implementation of jerry_port_get_current_time. Uses 'gettimeofday' if
 * available on the system, does nothing otherwise.
 *
 * @return milliseconds since Unix epoch - if 'gettimeofday' is available and
 *                                         executed successfully,
 *         0 - otherwise.
 */
double jerry_port_get_current_time (void)
{
#ifdef __GNUC__
  struct timeval tv;

  if (gettimeofday (&tv, NULL) == 0)
  {
    return ((double) tv.tv_sec) * 1000.0 + ((double) tv.tv_usec) / 1000.0;
  }
#endif /* __GNUC__ */

#ifdef _WINDOWS
  time_t ltime;
  time (&ltime);
  return ltime * 1000;
#endif /* _WINDOWS */

  return 0.0;
} /* jerry_port_get_current_time */

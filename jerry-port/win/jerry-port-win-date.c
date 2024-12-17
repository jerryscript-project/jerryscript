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

#define UNIX_EPOCH_IN_TICKS 116444736000000000LL /* difference between 1970 and 1601 */
#define TICKS_PER_MS        10000LL /* 1 tick is 100 nanoseconds */

/*
 * If you take the limit of SYSTEMTIME (last millisecond in 30827) then you end up with
 * a FILETIME of 0x7fff35f4f06c58f0 by using SystemTimeToFileTime(). However, if you put
 * 0x7fffffffffffffff into FileTimeToSystemTime() then you will end up in the year 30828,
 * although this date is invalid for SYSTEMTIME. Any larger value (0x8000000000000000 and above)
 * causes FileTimeToSystemTime() to fail.
 * https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-systemtime
 * https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-filetime
 */
#define UNIX_EPOCH_DATE_1601_01_02  -11644387200000LL /* unit: ms */
#define UNIX_EPOCH_DATE_30827_12_29 9106702560000000LL /* unit: ms */

/**
 * Convert unix time to a FILETIME value.
 *
 * https://support.microsoft.com/en-us/help/167296/how-to-convert-a-unix-time-t-to-a-win32-filetime-or-systemtime
 */
static void
unix_time_to_filetime (LONGLONG t, LPFILETIME ft_p)
{
  LONGLONG ll = t * TICKS_PER_MS + UNIX_EPOCH_IN_TICKS;

  ft_p->dwLowDateTime = (DWORD) ll;
  ft_p->dwHighDateTime = (DWORD) (ll >> 32);
} /* unix_time_to_filetime */

/**
 * Convert a FILETIME to a unix time value.
 *
 * @return unix time
 */
static LONGLONG
filetime_to_unix_time (LPFILETIME ft_p)
{
  ULARGE_INTEGER date;
  LONGLONG ll;
  date.HighPart = ft_p->dwHighDateTime;
  date.LowPart = ft_p->dwLowDateTime;
  ll = (LONGLONG) date.QuadPart - UNIX_EPOCH_IN_TICKS;
  return ll / TICKS_PER_MS;
} /* filetime_to_unix_time */

int32_t
jerry_port_local_tza (double unix_ms)
{
  FILETIME utc;
  FILETIME local;
  SYSTEMTIME utc_sys;
  SYSTEMTIME local_sys;
  LONGLONG t = (LONGLONG) (unix_ms);

  /*
   * If the time is earlier than the date 1601-01-02, then always using date 1601-01-02 to
   * query time zone adjustment. This date (1601-01-02) will make sure both UTC and local
   * time succeed with Win32 API. The date 1601-01-01 may lead to a win32 api failure, as
   * after converting between local time and utc time, the time may be earlier than 1601-01-01
   * in UTC time, that exceeds the FILETIME representation range.
   */
  if (t < UNIX_EPOCH_DATE_1601_01_02)
  {
    t = UNIX_EPOCH_DATE_1601_01_02;
  }

  /* Like above, do not use the last supported day */
  if (t > UNIX_EPOCH_DATE_30827_12_29)
  {
    t = UNIX_EPOCH_DATE_30827_12_29;
  }
  unix_time_to_filetime (t, &utc);

  if (FileTimeToSystemTime (&utc, &utc_sys) && SystemTimeToTzSpecificLocalTime (NULL, &utc_sys, &local_sys)
      && SystemTimeToFileTime (&local_sys, &local))
  {
    LONGLONG unix_local = filetime_to_unix_time (&local);
    return (int32_t) (unix_local - t);
  }

  return 0;
} /* jerry_port_local_tza */

double
jerry_port_current_time (void)
{
  FILETIME ft;
  GetSystemTimeAsFileTime (&ft);
  return (double) filetime_to_unix_time (&ft);
} /* jerry_port_current_time */

#endif /* defined(_WIN32) */

/* Copyright 2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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

#include "ecma-alloc.h"
#include "ecma-builtin-helpers.h"
#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "fdlibm-math.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltinhelpers ECMA builtin helper operations
 * @{
 */

/**
 * Helper function to get day number from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.2
 *
 * Used by:
 *         - The Date.prototype.setMilliseconds routine.
 *         - The Date.prototype.setUTCMilliseconds routine.
 *         - The Date.prototype.setSeconds routine.
 *         - The Date.prototype.setUTCSeconds routine.
 *         - The Date.prototype.setMinutes routine.
 *         - The Date.prototype.setUTCMinutes routine.
 *         - The Date.prototype.setHours routine.
 *         - The Date.prototype.setUTCHours routine.
 *
 * @return  time value for day number
 */
ecma_number_t __attr_always_inline___
ecma_date_day (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  return (ecma_number_t) floor (time / ECMA_DATE_MS_PER_DAY);
} /* ecma_date_day */

/**
 * Helper function to get time within day from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.2
 *
 * Used by:
 *         - The Date.prototype.setDate routine.
 *         - The Date.prototype.setUTCDate routine.
 *         - The Date.prototype.setMonth routine.
 *         - The Date.prototype.setUTCMonth routine.
 *         - The Date.prototype.setFullYear routine.
 *         - The Date.prototype.setUTCFullYear routine.
 *
 * @return  time value within the day
 */
ecma_number_t __attr_always_inline___
ecma_date_time_within_day (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  return (ecma_number_t) fmod (time, ECMA_DATE_MS_PER_DAY);
} /* ecma_date_time_within_day */

/**
 * Helper function to get number of days from year value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.3
 *
 * @return  number of days
 */
ecma_number_t __attr_always_inline___
ecma_date_days_in_year (ecma_number_t year) /**< year value */
{
  if (ecma_number_is_nan (year))
  {
    return year; /* year is NaN */
  }

  if (fmod (floor (year), 4))
  {
    return (ecma_number_t) 365;
  }

  if (fmod (floor (year), 100))
  {
    return (ecma_number_t) 366;
  }

  if (fmod (floor (year), 400))
  {
    return (ecma_number_t) 365;
  }

  return (ecma_number_t) 366;
} /* ecma_date_days_in_year */

/**
 * Helper function to get the day number of the first day of a year.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.3
 *
 * @return  day number of the first day of a year
 */
ecma_number_t __attr_always_inline___
ecma_date_day_from_year (ecma_number_t year) /**< year value */
{
  if (ecma_number_is_nan (year))
  {
    return year; /* year is NaN */
  }

  return (ecma_number_t) (365 * (year - 1970)
                          + floor ((year - 1969) / 4)
                          - floor ((year - 1901) / 100)
                          + floor ((year - 1601) / 400));
} /* ecma_date_day_from_year */

/**
 * Helper function to get the time value of the start of a year.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.3
 *
 * @return  time value of the start of a year
 */
ecma_number_t __attr_always_inline___
ecma_date_time_from_year (ecma_number_t year) /**< year value */
{
  if (ecma_number_is_nan (year))
  {
    return year; /* year is NaN */
  }

  return ECMA_DATE_MS_PER_DAY * ecma_date_day_from_year (year);
} /* ecma_date_time_from_year */

/**
 * Helper function to determine a year value from the time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.3
 *
 * Used by:
 *         - The Date.prototype.getFullYear routine. (Generated.)
 *         - The Date.prototype.getUTCFullYear routine. (Generated.)
 *         - The Date.prototype.setDate routine.
 *         - The Date.prototype.setUTCDate routine.
 *         - The Date.prototype.setMonth routine.
 *         - The Date.prototype.setUTCMonth routine.
 *
 * @return  year value
 */
ecma_number_t
ecma_date_year_from_time (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  /* ECMA-262 v5, 15.9.1.1 define the largest year that is
   * representable (285616) forward from 01 January, 1970 UTC.
   */
  ecma_number_t year = (ecma_number_t) (1970 + 285616);
  ecma_number_t lower_year_boundary = (ecma_number_t) (1970 - 285616);

  while (ecma_date_time_from_year (year) > time)
  {
    ecma_number_t year_boundary = (ecma_number_t) floor (lower_year_boundary + (year - lower_year_boundary) / 2);
    if (ecma_date_time_from_year (year_boundary) > time)
    {
      year = year_boundary;
    }
    else
    {
      lower_year_boundary = year_boundary;
    }
    year--;
  }

  return year;
} /* ecma_date_year_from_time */

/**
 * Helper function to decide if time value is in a leap-year.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.3
 *
 * @return  1 if time within a leap year and otherwise is zero
 */
ecma_number_t __attr_always_inline___
ecma_date_in_leap_year (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  return ecma_date_days_in_year (ecma_date_year_from_time (time)) - 365;
} /* ecma_date_in_leap_year */

/**
 * Helper function to get day within year from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.4
 *
 * @return  number of days within year
 */
ecma_number_t __attr_always_inline___
ecma_date_day_within_year (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  return ecma_date_day (time) - ecma_date_day_from_year (ecma_date_year_from_time (time));
} /* ecma_date_day_within_year */

/**
 * Helper function to get month from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.4
 *
 * Used by:
 *         - The Date.prototype.getMonth routine. (Generated.)
 *         - The Date.prototype.getUTCMonth routine. (Generated.)
 *         - The Date.prototype.setDate routine.
 *         - The Date.prototype.setUTCDate routine.
 *         - The Date.prototype.setFullYear routine.
 *         - The Date.prototype.setUTCFullYear routine.
 *
 * @return  month number
 */
ecma_number_t
ecma_date_month_from_time (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  ecma_number_t in_leap_year = ecma_date_in_leap_year (time);
  ecma_number_t day_within_year = ecma_date_day_within_year (time);

  JERRY_ASSERT (day_within_year >= 0 && day_within_year < 365 + in_leap_year);

  if (day_within_year < 31)
  {
    return 0;
  }
  else if (day_within_year < 59 + in_leap_year)
  {
    return 1;
  }
  else if (day_within_year < 90 + in_leap_year)
  {
    return 2;
  }
  else if (day_within_year < 120 + in_leap_year)
  {
    return 3;
  }
  else if (day_within_year < 151 + in_leap_year)
  {
    return 4;
  }
  else if (day_within_year < 181 + in_leap_year)
  {
    return 5;
  }
  else if (day_within_year < 212 + in_leap_year)
  {
    return 6;
  }
  else if (day_within_year < 243 + in_leap_year)
  {
    return 7;
  }
  else if (day_within_year < 273 + in_leap_year)
  {
    return 8;
  }
  else if (day_within_year < 304 + in_leap_year)
  {
    return 9;
  }
  else if (day_within_year < 334 + in_leap_year)
  {
    return 10;
  }

  return 11;
} /* ecma_date_month_from_time */

/**
 * Helper function to get date number from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.5
 *
 * Used by:
 *         - The Date.prototype.getDate routine. (Generated.)
 *         - The Date.prototype.getUTCDate routine. (Generated.)
 *         - The Date.prototype.setMonth routine.
 *         - The Date.prototype.setUTCMonth routine.
 *         - The Date.prototype.setFullYear routine.
 *         - The Date.prototype.setUTCFullYear routine.
 *
 * @return  date number
 */
ecma_number_t
ecma_date_date_from_time (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  ecma_number_t in_leap_year = ecma_date_in_leap_year (time);
  ecma_number_t day_within_year = ecma_date_day_within_year (time);
  JERRY_ASSERT (!ecma_number_is_nan (ecma_date_month_from_time (time)));
  int month = ecma_number_to_int32 (ecma_date_month_from_time (time));

  switch (month)
  {
    case 0:
    {
      return day_within_year + 1;
    }
    case 1:
    {
      return day_within_year - 30 ;
    }
    case 2:
    {
      return day_within_year - 58 - in_leap_year;
    }
    case 3:
    {
      return day_within_year - 89 - in_leap_year;
    }
    case 4:
    {
      return day_within_year - 119 - in_leap_year;
    }
    case 5:
    {
      return day_within_year - 150 - in_leap_year;
    }
    case 6:
    {
      return day_within_year - 180 - in_leap_year;
    }
    case 7:
    {
      return day_within_year - 211 - in_leap_year;
    }
    case 8:
    {
      return day_within_year - 242 - in_leap_year;
    }
    case 9:
    {
      return day_within_year - 272 - in_leap_year;
    }
    case 10:
    {
      return day_within_year - 303 - in_leap_year;
    }
    case 11:
    {
      return day_within_year - 333 - in_leap_year;
    }
  }

  JERRY_UNREACHABLE ();
  return 0;
} /* ecma_date_date_from_time */

/**
 * Helper function to get weekday from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.6
 *
 * Used by:
 *         - The Date.prototype.getDay routine. (Generated.)
 *         - The Date.prototype.getUTCDay routine. (Generated.)
 *
 * @return  weekday number
 */
ecma_number_t __attr_always_inline___
ecma_date_week_day (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  return (ecma_number_t) fmod ((ecma_date_day (time) + 4), 7);
} /* ecma_date_week_day */

/**
 * Helper function to get local time zone adjustment.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.7
 *
 * @return  local time zone adjustment
 */
ecma_number_t __attr_always_inline___
ecma_date_local_tza ()
{
  /*
   * FIXME:
   *        Get the real system time. ex: localtime_r, gmtime_r, daylight on Linux
   *        Introduce system macros at first.
   */
  return ECMA_NUMBER_ZERO;
} /* ecma_date_local_tza */

/**
 * Helper function to get the daylight saving time adjustment.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.8
 *
 * @return  daylight saving time adjustment
 */
ecma_number_t __attr_always_inline___
ecma_date_daylight_saving_ta (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  /*
   * FIXME:
   *        Get the real system time. ex: localtime_r, gmtime_r, daylight on Linux
   *        Introduce system macros at first.
   */
  return ECMA_NUMBER_ZERO;
} /* ecma_date_daylight_saving_ta */

/**
 * Helper function to get local time from UTC.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.9
 *
 * Used by:
 *         - All Date.prototype.getUTC* routines. (Generated.)
 *         - The Date.prototype.getTimezoneOffset routine.
 *         - The Date.prototype.setMilliseconds routine.
 *         - The Date.prototype.setSeconds routine.
 *         - The Date.prototype.setMinutes routine.
 *         - The Date.prototype.setHours routine.
 *         - The Date.prototype.setDate routine.
 *         - The Date.prototype.setMonth routine.
 *         - The Date.prototype.setFullYear routine.
 *         - The ecma_date_timezone_offset helper function.
 *
 * @return  local time
 */
ecma_number_t __attr_always_inline___
ecma_date_local_time (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  return time + ecma_date_local_tza () + ecma_date_daylight_saving_ta (time);
} /* ecma_date_local_time */

/**
 * Helper function to get utc from local time.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.9
 *
 * Used by:
 *         - The Date object's 'parse' routine.
 *         - The [[Construct]] of built-in Date object rutine.
 *
 * @return  utc value
 */
ecma_number_t __attr_always_inline___
ecma_date_utc (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  ecma_number_t simple_utc_time = time - ecma_date_local_tza ();
  return simple_utc_time - ecma_date_daylight_saving_ta (simple_utc_time);
} /* ecma_date_utc */

/**
 * Helper function to get hour from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.10
 *
 * Used by:
 *         - The Date.prototype.getHour routine. (Generated.)
 *         - The Date.prototype.getUTCHour routine. (Generated.)
 *         - The Date.prototype.setMilliseconds routine.
 *         - The Date.prototype.setUTCMilliseconds routine.
 *         - The Date.prototype.setSeconds routine.
 *         - The Date.prototype.setUTCSeconds routine.
 *         - The Date.prototype.setMinutes routine.
 *         - The Date.prototype.setUTCMinutes routine.
 *
 * @return  hour value
 */
ecma_number_t __attr_always_inline___
ecma_date_hour_from_time (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  ecma_number_t hour = (ecma_number_t) fmod (floor (time / ECMA_DATE_MS_PER_HOUR),
                                             ECMA_DATE_HOURS_PER_DAY);
  return (hour < 0) ? ECMA_DATE_HOURS_PER_DAY + hour : hour;
} /* ecma_date_hour_from_time */

/**
 * Helper function to get minute from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.10
 *
 * Used by:
 *         - The Date.prototype.getMinutes routine. (Generated.)
 *         - The Date.prototype.getUTCMinutes routine. (Generated.)
 *         - The Date.prototype.setMilliseconds routine.
 *         - The Date.prototype.setUTCMilliseconds routine.
 *         - The Date.prototype.setSeconds routine.
 *         - The Date.prototype.setUTCSeconds routine.
 *         - The Date.prototype.setHours routine.
 *         - The Date.prototype.setUTCHours routine.
 *
 * @return  minute value
 */
ecma_number_t __attr_always_inline___
ecma_date_min_from_time (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  ecma_number_t min = (ecma_number_t) fmod (floor (time / ECMA_DATE_MS_PER_MINUTE),
                                            ECMA_DATE_MINUTES_PER_HOUR);
  return (min < 0) ? ECMA_DATE_MINUTES_PER_HOUR + min : min;
} /* ecma_date_min_from_time */

/**
 * Helper function to get second from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.10
 *
 * Used by:
 *         - The Date.prototype.getSeconds routine. (Generated.)
 *         - The Date.prototype.getUTCSeconds routine. (Generated.)
 *         - The Date.prototype.setMilliseconds routine.
 *         - The Date.prototype.setUTCMilliseconds routine.
 *         - The Date.prototype.setMinutes routine.
 *         - The Date.prototype.setUTCMinutes routine.
 *         - The Date.prototype.setHours routine.
 *         - The Date.prototype.setUTCHours routine.
 *
 * @return  second value
 */
ecma_number_t __attr_always_inline___
ecma_date_sec_from_time (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  ecma_number_t sec = (ecma_number_t) fmod (floor (time / ECMA_DATE_MS_PER_SECOND),
                                            ECMA_DATE_SECONDS_PER_MINUTE);
  return (sec < 0) ? ECMA_DATE_SECONDS_PER_MINUTE + sec : sec;
} /* ecma_date_sec_from_time */

/**
 * Helper function to get millisecond from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.10
 *
 * Used by:
 *         - The Date.prototype.getMilliseconds routine. (Generated.)
 *         - The Date.prototype.getUTCMilliseconds routine. (Generated.)
 *         - The Date.prototype.setSeconds routine.
 *         - The Date.prototype.setUTCSeconds routine.
 *         - The Date.prototype.setMinutes routine.
 *         - The Date.prototype.setUTCMinutes routine.
 *         - The Date.prototype.setHours routine.
 *         - The Date.prototype.setUTCHours routine.
 *
 * @return  millisecond value
 */
ecma_number_t __attr_always_inline___
ecma_date_ms_from_time (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  ecma_number_t milli = (ecma_number_t) fmod (time, ECMA_DATE_MS_PER_SECOND);
  return (milli < 0) ? ECMA_DATE_MS_PER_SECOND + milli : milli;
} /* ecma_date_ms_from_time */

/**
 * Helper function to make time value from hour, min, sec and ms.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.11
 *
 * Used by:
 *         - The Date.prototype.setMilliseconds routine.
 *         - The Date.prototype.setUTCMilliseconds routine.
 *         - The Date.prototype.setSeconds routine.
 *         - The Date.prototype.setUTCSeconds routine.
 *         - The Date.prototype.setMinutes routine.
 *         - The Date.prototype.setUTCMinutes routine.
 *         - The Date.prototype.setHours routine.
 *         - The Date.prototype.setUTCHours routine.
 *
 * @return  time value
 */
ecma_number_t
ecma_date_make_time (ecma_number_t hour, /**< hour value */
                     ecma_number_t min, /**< minute value */
                     ecma_number_t sec, /**< second value */
                     ecma_number_t ms) /**< millisecond value */
{
  if (ecma_number_is_nan (hour)
      || ecma_number_is_nan (min)
      || ecma_number_is_nan (sec)
      || ecma_number_is_nan (ms)
      || ecma_number_is_infinity (hour)
      || ecma_number_is_infinity (min)
      || ecma_number_is_infinity (sec)
      || ecma_number_is_infinity (ms))
  {
    return ecma_number_make_nan ();
  }

  /* Replaced toInteger to ecma_number_trunc because it does the same thing. */
  ecma_number_t h = ecma_number_trunc (hour);
  ecma_number_t m = ecma_number_trunc (min);
  ecma_number_t s = ecma_number_trunc (sec);
  ecma_number_t milli = ecma_number_trunc (ms);

  return (h * ECMA_DATE_MS_PER_HOUR
          + m * ECMA_DATE_MS_PER_MINUTE
          + s * ECMA_DATE_MS_PER_SECOND
          + milli);
} /* ecma_date_make_time */

/**
 * Helper function to make day value from year, month and date.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.12
 *
 * Used by:
 *         - The Date.prototype.setDate routine.
 *         - The Date.prototype.setUTCDate routine.
 *         - The Date.prototype.setMonth routine.
 *         - The Date.prototype.setUTCMonth routine.
 *         - The Date.prototype.setFullYear routine.
 *         - The Date.prototype.setUTCFullYear routine.
 *
 * @return  day value
 */
ecma_number_t
ecma_date_make_day (ecma_number_t year, /**< year value */
                    ecma_number_t month, /**< month value */
                    ecma_number_t date) /**< date value */
{
  if (ecma_number_is_nan (year)
      || ecma_number_is_nan (month)
      || ecma_number_is_nan (date)
      || ecma_number_is_infinity (year)
      || ecma_number_is_infinity (month)
      || ecma_number_is_infinity (date))
  {
    return ecma_number_make_nan ();
  }

  ecma_number_t y = ecma_number_trunc (year);
  ecma_number_t m = ecma_number_trunc (month);
  ecma_number_t dt = ecma_number_trunc (date);
  ecma_number_t ym = y + (ecma_number_t) floor (m / 12);
  ecma_number_t mn = (ecma_number_t) fmod (m, 12);
  mn = (mn < 0) ? 12 + mn : mn;
  ecma_number_t time = ecma_date_time_from_year (ym);

  JERRY_ASSERT (ecma_date_year_from_time (time) == ym);

  while (ecma_date_month_from_time (time) < mn)
  {
    time += ECMA_DATE_MS_PER_DAY;
  }

  JERRY_ASSERT (ecma_date_month_from_time (time) == mn);
  JERRY_ASSERT (ecma_date_date_from_time (time) == 1);

  return ecma_date_day (time) + dt - ((ecma_number_t) 1.0);
} /* ecma_date_make_day */

/**
 * Helper function to make date value from day and time.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.13
 *
 * @return  date value
 */
ecma_number_t __attr_always_inline___
ecma_date_make_date (ecma_number_t day, /**< day value */
                     ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (day)
      || ecma_number_is_nan (time)
      || ecma_number_is_infinity (day)
      || ecma_number_is_infinity (time))
  {
    return ecma_number_make_nan ();
  }

  return day * ECMA_DATE_MS_PER_DAY + time;
} /* ecma_date_make_date */

/**
 * Helper function to calculate number of milliseconds from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.14
 *
 * Used by:
 *         - The Date.prototype.setTime routine.
 *         - The ecma_date_set_internal_property helper function.
 *
 * @return  number of milliseconds
 */
ecma_number_t __attr_always_inline___
ecma_date_time_clip (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time)
      || ecma_number_is_infinity (time)
      || fabs (time) > ECMA_DATE_MAX_VALUE)
  {
    return ecma_number_make_nan ();
  }

  return ecma_number_trunc (time);
} /* ecma_date_time_clip */

/**
 * Helper function to calculate timezone offset.
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.26
 *
 * Used by:
 *         - The Date.prototype.getTimezoneOffset routine. (Generated.)
 *
 * @return  timezone offset
 */
ecma_number_t __attr_always_inline___
ecma_date_timezone_offset (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return ecma_number_make_nan ();
  }

  return (time - ecma_date_local_time (time)) / ECMA_DATE_MS_PER_MINUTE;
} /* ecma_date_timezone_offset */

/**
 * Helper function to set Date internal property.
 *
 * Used by:
 *         - All Date.prototype.set* routine except Date.prototype.setTime.
 *
 * @return completion value containing the new internal time value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_date_set_internal_property (ecma_value_t this_arg, /**< this argument */
                                 ecma_number_t day, /**< day */
                                 ecma_number_t time, /**< time */
                                 ecma_date_timezone_t is_utc) /**< input is utc */
{
  JERRY_ASSERT (ecma_is_value_object (this_arg));

  ecma_number_t *value_p = ecma_alloc_number ();
  ecma_number_t date = ecma_date_make_date (day, time);
  if (is_utc != ECMA_DATE_UTC)
  {
    date = ecma_date_utc (date);
  }
  *value_p = ecma_date_time_clip (date);

  ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);

  ecma_property_t *prim_value_prop_p = ecma_get_internal_property (obj_p,
                                                                   ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE);

  ecma_number_t *prim_value_num_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                                               prim_value_prop_p->u.internal_property.value);
  *prim_value_num_p = *value_p;

  return ecma_make_normal_completion_value (ecma_make_number_value (value_p));
} /* ecma_date_set_internal_property */

/**
 * Insert leading zeros to a string of a number if needed.
 */
void
ecma_date_insert_leading_zeros (ecma_string_t **str_p, /**< input/output string */
                                ecma_number_t num, /**< input number */
                                uint32_t length) /**< length of string of number */
{
  JERRY_ASSERT (length >= 1);

  /* If the length is bigger than the number of digits in num, then insert leding zeros. */
  uint32_t first_index = length - 1u;
  ecma_number_t power_i = (ecma_number_t) pow (10, first_index);
  for (uint32_t i = first_index; i > 0 && num < power_i; i--, power_i /= 10)
  {
    ecma_string_t *zero_str_p = ecma_new_ecma_string_from_uint32 (0);
    ecma_string_t *concat_p = ecma_concat_ecma_strings (zero_str_p, *str_p);
    ecma_deref_ecma_string (zero_str_p);
    ecma_deref_ecma_string (*str_p);
    *str_p = concat_p;
  }
} /* ecma_date_insert_leading_zeros */

/**
 * Insert a number to the start of a string with a specific separator character and
 * fix length. If the length is bigger than the number of digits in num, then insert leding zeros.
 */
void
ecma_date_insert_num_with_sep (ecma_string_t **str_p, /**< input/output string */
                               ecma_number_t num, /**< input number */
                               lit_magic_string_id_t magic_str_id, /**< separator character id */
                               uint32_t length) /**< length of string of number */
{
  ecma_string_t *magic_string_p = ecma_get_magic_string (magic_str_id);

  ecma_string_t *concat_p = ecma_concat_ecma_strings (magic_string_p, *str_p);
  ecma_deref_ecma_string (magic_string_p);
  ecma_deref_ecma_string (*str_p);
  *str_p = concat_p;

  ecma_string_t *num_str_p = ecma_new_ecma_string_from_number (num);
  concat_p = ecma_concat_ecma_strings (num_str_p, *str_p);
  ecma_deref_ecma_string (num_str_p);
  ecma_deref_ecma_string (*str_p);
  *str_p = concat_p;

  ecma_date_insert_leading_zeros (str_p, num, length);
} /* ecma_date_insert_num_with_sep */

/**
 * Common function to create a time zone specific string.
 *
 * Used by:
 *        - The Date.prototype.toString routine.
 *        - The Date.prototype.toISOString routine.
 *        - The Date.prototype.toUTCString routine.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_date_to_string (ecma_value_t this_arg, /**< this argument */
                     ecma_date_timezone_t timezone) /**< timezone */
{
  TODO ("Add support for local time zone output.");
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (!ecma_is_value_object (this_arg)
      || ecma_object_get_class_name (ecma_get_object_from_value (this_arg)) != LIT_MAGIC_STRING_DATE_UL)
  {
    ret_value = ecma_raise_type_error ("Incompatible type");
  }
  else
  {
    ECMA_TRY_CATCH (obj_this,
                    ecma_op_to_object (this_arg),
                    ret_value);

    ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
    ecma_property_t *prim_value_prop_p;
    prim_value_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE);
    ecma_number_t *prim_value_num_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                                                 prim_value_prop_p->u.internal_property.value);

    if (ecma_number_is_nan (*prim_value_num_p))
    {
      ecma_string_t *magic_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INVALID_DATE_UL);
      ret_value = ecma_make_normal_completion_value (ecma_make_string_value (magic_str_p));
    }
    else
    {
      ecma_string_t *output_str_p;
      ecma_number_t milliseconds = ecma_date_ms_from_time (*prim_value_num_p);

      if (timezone == ECMA_DATE_UTC)
      {
        output_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
        ecma_date_insert_num_with_sep (&output_str_p, milliseconds, LIT_MAGIC_STRING_Z_CHAR, 3);
      }
      else
      {
        output_str_p = ecma_new_ecma_string_from_number (milliseconds);
        ecma_date_insert_leading_zeros (&output_str_p, milliseconds, 3);
      }

      ecma_number_t seconds = ecma_date_sec_from_time (*prim_value_num_p);
      ecma_date_insert_num_with_sep (&output_str_p, seconds, LIT_MAGIC_STRING_DOT_CHAR, 2);

      ecma_number_t minutes = ecma_date_min_from_time (*prim_value_num_p);
      ecma_date_insert_num_with_sep (&output_str_p, minutes, LIT_MAGIC_STRING_COLON_CHAR, 2);

      ecma_number_t hours = ecma_date_hour_from_time (*prim_value_num_p);
      ecma_date_insert_num_with_sep (&output_str_p, hours, LIT_MAGIC_STRING_COLON_CHAR, 2);

      ecma_number_t day = ecma_date_date_from_time (*prim_value_num_p);
      ecma_date_insert_num_with_sep (&output_str_p, day, LIT_MAGIC_STRING_TIME_SEP_U, 2);

      /*
       * Note:
       *      'ecma_date_month_from_time' (ECMA 262 v5, 15.9.1.4) returns a number from 0 to 11,
       *      but we have to print the month from 1 to 12 for ISO 8601 standard (ECMA 262 v5, 15.9.1.15).
       */
      ecma_number_t month = ecma_date_month_from_time (*prim_value_num_p) + 1;
      ecma_date_insert_num_with_sep (&output_str_p, month, LIT_MAGIC_STRING_MINUS_CHAR, 2);

      ecma_number_t year = ecma_date_year_from_time (*prim_value_num_p);
      ecma_date_insert_num_with_sep (&output_str_p, year, LIT_MAGIC_STRING_MINUS_CHAR, 4);

      ret_value = ecma_make_normal_completion_value (ecma_make_string_value (output_str_p));
    }

    ECMA_FINALIZE (obj_this);
  }

  return ret_value;
} /* ecma_date_create_formatted_string */

/**
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN */

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

#include <math.h>

#include "ecma-alloc.h"
#include "ecma-builtin-helpers.h"
#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "lit-char-helpers.h"

#ifndef CONFIG_DISABLE_DATE_BUILTIN

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
inline ecma_number_t __attr_always_inline___
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
inline ecma_number_t __attr_always_inline___
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
inline ecma_number_t __attr_always_inline___
ecma_date_days_in_year (ecma_number_t year) /**< year value */
{
  if (ecma_number_is_nan (year))
  {
    return year; /* year is NaN */
  }

  if (fmod (floor (year), 4) != ECMA_NUMBER_ZERO)
  {
    return (ecma_number_t) 365;
  }

  if (fmod (floor (year), 100) != ECMA_NUMBER_ZERO)
  {
    return (ecma_number_t) 366;
  }

  if (fmod (floor (year), 400) != ECMA_NUMBER_ZERO)
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
inline ecma_number_t __attr_always_inline___
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
inline ecma_number_t __attr_always_inline___
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
inline ecma_number_t __attr_always_inline___
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
inline ecma_number_t __attr_always_inline___
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
inline ecma_number_t __attr_always_inline___
ecma_date_week_day (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  ecma_number_t week_day = (ecma_number_t) fmod ((ecma_date_day (time) + 4), 7);
  return (week_day < 0) ? 7 + week_day : week_day;
} /* ecma_date_week_day */

/**
 * Helper function to get local time zone adjustment.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.7
 *
 * @return  local time zone adjustment
 */
inline ecma_number_t __attr_always_inline___
ecma_date_local_tza ()
{
  jerry_time_zone_t tz;

  if (!jerry_port_get_time_zone (&tz))
  {
    return ecma_number_make_nan ();
  }

  return tz.offset * -ECMA_DATE_MS_PER_MINUTE;
} /* ecma_date_local_tza */

/**
 * Helper function to get the daylight saving time adjustment.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.8
 *
 * @return  daylight saving time adjustment
 */
inline ecma_number_t __attr_always_inline___
ecma_date_daylight_saving_ta (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  jerry_time_zone_t tz;

  if (!jerry_port_get_time_zone (&tz))
  {
    return ecma_number_make_nan ();
  }

  return tz.daylight_saving_time * ECMA_DATE_MS_PER_HOUR;
} /* ecma_date_daylight_saving_ta */

/**
 * Helper function to get local time from UTC.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.9
 *
 * Used by:
 *         - All Date.prototype.getUTC routines. (Generated.)
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
inline ecma_number_t __attr_always_inline___
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
inline ecma_number_t __attr_always_inline___
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
inline ecma_number_t __attr_always_inline___
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
inline ecma_number_t __attr_always_inline___
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
inline ecma_number_t __attr_always_inline___
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
inline ecma_number_t __attr_always_inline___
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
  /* 1. */
  if (ecma_number_is_nan (year)
      || ecma_number_is_nan (month)
      || ecma_number_is_nan (date)
      || ecma_number_is_infinity (year)
      || ecma_number_is_infinity (month)
      || ecma_number_is_infinity (date))
  {
    return ecma_number_make_nan ();
  }

  /* 2., 3., 4. */
  ecma_number_t y = ecma_number_trunc (year);
  ecma_number_t m = ecma_number_trunc (month);
  ecma_number_t dt = ecma_number_trunc (date);
  /* 5. */
  ecma_number_t ym = y + (ecma_number_t) floor (m / 12);
  /* 6. */
  ecma_number_t mn = (ecma_number_t) fmod (m, 12);
  mn = (mn < 0) ? 12 + mn : mn;

  /* 7. */
  ecma_number_t time = ecma_date_time_from_year (ym);

  /**
   * The algorithm below searches the following date: ym-mn-1
   * To find this time it starts from the beggining of the year (ym)
   * then find the first day of the month.
   */
  if (ecma_date_year_from_time (time) == ym)
  {
    /* Get the month */
    time += 31 * mn * ECMA_DATE_MS_PER_DAY;

    /* Get the month's first day */
    time += ((ecma_number_t) 1.0 - ecma_date_date_from_time (time)) * ECMA_DATE_MS_PER_DAY;

    if (ecma_date_month_from_time (time) == mn && ecma_date_date_from_time (time) == 1)
    {
      /* 8. */
      return ecma_date_day (time) + dt - ((ecma_number_t) 1.0);
    }
  }

  return ecma_number_make_nan ();
} /* ecma_date_make_day */

/**
 * Helper function to make date value from day and time.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.13
 *
 * @return  date value
 */
inline ecma_number_t __attr_always_inline___
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
inline ecma_number_t __attr_always_inline___
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
inline ecma_number_t __attr_always_inline___
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
 *         - All Date.prototype.set *routine except Date.prototype.setTime.
 *
 * @return ecma value containing the new internal time value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_date_set_internal_property (ecma_value_t this_arg, /**< this argument */
                                 ecma_number_t day, /**< day */
                                 ecma_number_t time, /**< time */
                                 ecma_date_timezone_t is_utc) /**< input is utc */
{
  JERRY_ASSERT (ecma_is_value_object (this_arg));

  ecma_number_t date = ecma_date_make_date (day, time);
  if (is_utc != ECMA_DATE_UTC)
  {
    date = ecma_date_utc (date);
  }

  ecma_number_t value = ecma_date_time_clip (date);

  ecma_object_t *object_p = ecma_get_object_from_value (this_arg);
  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

  *ECMA_GET_INTERNAL_VALUE_POINTER (ecma_number_t, ext_object_p->u.class_prop.u.value) = value;

  return ecma_make_number_value (value);
} /* ecma_date_set_internal_property */

/**
 * Common function to copy utf8 characters.
 *
 * @return next destination buffer position
 */
static lit_utf8_byte_t *
ecma_date_value_copy_utf8_bytes (lit_utf8_byte_t *dest_p, /**< destination buffer */
                                 const char *source_p) /**< source buffer */
{
  while (*source_p != LIT_CHAR_NULL)
  {
    *dest_p++ = (lit_utf8_byte_t) *source_p++;
  }
  return dest_p;
} /* ecma_date_value_copy_utf8_bytes */

/**
 * Common function to generate fixed width, right aligned decimal numbers.
 *
 * @return next destination buffer position
 */
static lit_utf8_byte_t *
ecma_date_value_number_to_bytes (lit_utf8_byte_t *dest_p, /**< destination buffer */
                                 int32_t number, /**< number */
                                 int32_t width) /**< width */
{
  dest_p += width;
  lit_utf8_byte_t *result_p = dest_p;

  do
  {
    dest_p--;
    *dest_p = (lit_utf8_byte_t) ((number % 10) + (int32_t) LIT_CHAR_0);
    number /= 10;
  }
  while (--width > 0);

  return result_p;
} /* ecma_date_value_number_to_bytes */

/**
 * Common function to get the 3 character abbreviation of a week day.
 *
 * @return next destination buffer position
 */
static lit_utf8_byte_t *
ecma_date_value_week_day_to_abbreviation (lit_utf8_byte_t *dest_p, /**< destination buffer */
                                          ecma_number_t datetime_number) /**< datetime */
{
  const char *abbreviations_p[8] =
  {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "---"
  };

  int32_t day = (int32_t) ecma_date_week_day (datetime_number);

  if (day < 0 || day > 6)
  {
    /* Just for safety, it should never happen. */
    day = 7;
  }

  return ecma_date_value_copy_utf8_bytes (dest_p, abbreviations_p[day]);
} /* ecma_date_value_week_day_to_abbreviation */

/**
 * Common function to get the 3 character abbreviation of a month.
 *
 * @return next destination buffer position
 */
static lit_utf8_byte_t *
ecma_date_value_month_to_abbreviation (lit_utf8_byte_t *dest_p, /**< destination buffer */
                                       ecma_number_t datetime_number) /**< datetime */
{
  const char *abbreviations_p[13] =
  {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "---"
  };

  int32_t month = (int32_t) ecma_date_month_from_time (datetime_number);

  if (month < 0 || month > 11)
  {
    /* Just for safety, it should never happen. */
    month = 12;
  }

  return ecma_date_value_copy_utf8_bytes (dest_p, abbreviations_p[(int) month]);
} /* ecma_date_value_month_to_abbreviation */

/**
 * The common 17 character long part of ecma_date_value_to_string and ecma_date_value_to_utc_string.
 *
 * @return next destination buffer position
 */
static lit_utf8_byte_t *
ecma_date_value_to_string_common (lit_utf8_byte_t *dest_p, /**< destination buffer */
                                  ecma_number_t datetime_number) /**< datetime */
{
  ecma_number_t year = ecma_date_year_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) year, 4);
  *dest_p++ = LIT_CHAR_SP;

  ecma_number_t hours = ecma_date_hour_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) hours, 2);
  *dest_p++ = LIT_CHAR_COLON;

  ecma_number_t minutes = ecma_date_min_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) minutes, 2);
  *dest_p++ = LIT_CHAR_COLON;

  ecma_number_t seconds = ecma_date_sec_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) seconds, 2);

  return ecma_date_value_copy_utf8_bytes (dest_p, " GMT");
} /* ecma_date_value_to_string_common */

/**
 * Common function to create a time zone specific string from a numeric value.
 *
 * Used by:
 *        - The Date routine.
 *        - The Date.prototype.toString routine.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_date_value_to_string (ecma_number_t datetime_number) /**< datetime */
{
  /*
   * Character length of the result string.
   */
  const uint32_t result_string_length = 34;

  lit_utf8_byte_t character_buffer[result_string_length];
  lit_utf8_byte_t *dest_p = character_buffer;

  datetime_number = ecma_date_local_time (datetime_number);

  dest_p = ecma_date_value_week_day_to_abbreviation (dest_p, datetime_number);
  *dest_p++ = LIT_CHAR_SP;

  dest_p = ecma_date_value_month_to_abbreviation (dest_p, datetime_number);
  *dest_p++ = LIT_CHAR_SP;

  ecma_number_t day = ecma_date_date_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) day, 2);
  *dest_p++ = LIT_CHAR_SP;

  dest_p = ecma_date_value_to_string_common (dest_p, datetime_number);

  int32_t time_zone = (int32_t) (ecma_date_local_tza () + ecma_date_daylight_saving_ta (datetime_number));
  if (time_zone >= 0)
  {
    *dest_p++ = LIT_CHAR_PLUS;
  }
  else
  {
    *dest_p++ = LIT_CHAR_MINUS;
    time_zone = -time_zone;
  }

  dest_p = ecma_date_value_number_to_bytes (dest_p,
                                            time_zone / (int32_t) ECMA_DATE_MS_PER_HOUR,
                                            2);
  *dest_p++ = LIT_CHAR_COLON;
  dest_p = ecma_date_value_number_to_bytes (dest_p,
                                            time_zone % (int32_t) ECMA_DATE_MS_PER_HOUR,
                                            2);

  JERRY_ASSERT ((uint32_t) (dest_p - character_buffer) == result_string_length);

  ecma_string_t *date_string_p = ecma_new_ecma_string_from_utf8 (character_buffer,
                                                                 result_string_length);

  return ecma_make_string_value (date_string_p);
} /* ecma_date_value_to_string */

/**
 * Common function to create a time zone specific string from a numeric value.
 *
 * Used by:
 *        - The Date.prototype.toUTCString routine.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_date_value_to_utc_string (ecma_number_t datetime_number) /**< datetime */
{
  /*
   * Character length of the result string.
   */
  const uint32_t result_string_length = 29;

  lit_utf8_byte_t character_buffer[result_string_length];
  lit_utf8_byte_t *dest_p = character_buffer;

  dest_p = ecma_date_value_week_day_to_abbreviation (dest_p, datetime_number);
  *dest_p++ = LIT_CHAR_COMMA;
  *dest_p++ = LIT_CHAR_SP;

  ecma_number_t day = ecma_date_date_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) day, 2);
  *dest_p++ = LIT_CHAR_SP;

  dest_p = ecma_date_value_month_to_abbreviation (dest_p, datetime_number);
  *dest_p++ = LIT_CHAR_SP;

  dest_p = ecma_date_value_to_string_common (dest_p, datetime_number);

  JERRY_ASSERT ((uint32_t) (dest_p - character_buffer) == result_string_length);

  ecma_string_t *date_string_p = ecma_new_ecma_string_from_utf8 (character_buffer,
                                                                 result_string_length);

  return ecma_make_string_value (date_string_p);
} /* ecma_date_value_to_utc_string */

/**
 * Common function to create a ISO specific string from a numeric value.
 *
 * Used by:
 *        - The Date.prototype.toISOString routine.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_date_value_to_iso_string (ecma_number_t datetime_number) /**<datetime */
{
  /*
   * Character length of the result string.
   */
  const uint32_t result_string_length = 24;

  lit_utf8_byte_t character_buffer[result_string_length];
  lit_utf8_byte_t *dest_p = character_buffer;

  ecma_number_t year = ecma_date_year_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) year, 4);
  *dest_p++ = LIT_CHAR_MINUS;

  /*
   * Note:
   *      'ecma_date_month_from_time' (ECMA 262 v5, 15.9.1.4) returns a number from 0 to 11,
   *      but we have to print the month from 1 to 12 for ISO 8601 standard (ECMA 262 v5, 15.9.1.15).
   */
  ecma_number_t month = ecma_date_month_from_time (datetime_number) + 1;
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) month, 2);
  *dest_p++ = LIT_CHAR_MINUS;

  ecma_number_t day = ecma_date_date_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) day, 2);
  *dest_p++ = LIT_CHAR_UPPERCASE_T;

  ecma_number_t hours = ecma_date_hour_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) hours, 2);
  *dest_p++ = LIT_CHAR_COLON;

  ecma_number_t minutes = ecma_date_min_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) minutes, 2);
  *dest_p++ = LIT_CHAR_COLON;

  ecma_number_t seconds = ecma_date_sec_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) seconds, 2);
  *dest_p++ = LIT_CHAR_DOT;

  ecma_number_t milliseconds = ecma_date_ms_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) milliseconds, 3);
  *dest_p++ = LIT_CHAR_UPPERCASE_Z;

  JERRY_ASSERT ((uint32_t) (dest_p - character_buffer) == result_string_length);

  ecma_string_t *date_string_p = ecma_new_ecma_string_from_utf8 (character_buffer,
                                                                 result_string_length);

  return ecma_make_string_value (date_string_p);
} /* ecma_date_value_to_iso_string */

/**
 * Common function to create a date string from a numeric value.
 *
 * Used by:
 *        - The Date.prototype.toDateString routine.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_date_value_to_date_string (ecma_number_t datetime_number) /**<datetime */
{
  /*
   * Character length of the result string.
   */
  const uint32_t result_string_length = 10;

  lit_utf8_byte_t character_buffer[result_string_length];
  lit_utf8_byte_t *dest_p = character_buffer;

  ecma_number_t year = ecma_date_year_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) year, 4);
  *dest_p++ = LIT_CHAR_MINUS;

  /*
   * Note:
   *      'ecma_date_month_from_time' (ECMA 262 v5, 15.9.1.4) returns a number from 0 to 11,
   *      but we have to print the month from 1 to 12 for ISO 8601 standard (ECMA 262 v5, 15.9.1.15).
   */
  ecma_number_t month = ecma_date_month_from_time (datetime_number) + 1;
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) month, 2);
  *dest_p++ = LIT_CHAR_MINUS;

  ecma_number_t day = ecma_date_date_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) day, 2);

  JERRY_ASSERT ((uint32_t) (dest_p - character_buffer) == result_string_length);

  ecma_string_t *date_string_p = ecma_new_ecma_string_from_utf8 (character_buffer,
                                                                 result_string_length);

  return ecma_make_string_value (date_string_p);
} /* ecma_date_value_to_date_string */

/**
 * Common function to create a time string from a numeric value.
 *
 * Used by:
 *        - The Date.prototype.toTimeString routine.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_date_value_to_time_string (ecma_number_t datetime_number) /**<datetime */
{
  /*
   * Character length of the result string.
   */
  const uint32_t result_string_length = 12;

  lit_utf8_byte_t character_buffer[result_string_length];
  lit_utf8_byte_t *dest_p = character_buffer;

  ecma_number_t hours = ecma_date_hour_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) hours, 2);
  *dest_p++ = LIT_CHAR_COLON;

  ecma_number_t minutes = ecma_date_min_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) minutes, 2);
  *dest_p++ = LIT_CHAR_COLON;

  ecma_number_t seconds = ecma_date_sec_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) seconds, 2);
  *dest_p++ = LIT_CHAR_DOT;

  ecma_number_t milliseconds = ecma_date_ms_from_time (datetime_number);
  dest_p = ecma_date_value_number_to_bytes (dest_p, (int32_t) milliseconds, 3);

  JERRY_ASSERT ((uint32_t) (dest_p - character_buffer) == result_string_length);

  ecma_string_t *time_string_p = ecma_new_ecma_string_from_utf8 (character_buffer,
                                                                 result_string_length);

  return ecma_make_string_value (time_string_p);
} /* ecma_date_value_to_time_string */

/**
 * Common function to get the primitive value of the Date object.
 *
 * Used by:
 *        - The Date.prototype.toString routine.
 *        - The Date.prototype.toISOString routine.
 *        - The Date.prototype.toUTCString routine.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_date_get_primitive_value (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (!ecma_is_value_object (this_arg)
      || !ecma_object_class_is (ecma_get_object_from_value (this_arg), LIT_MAGIC_STRING_DATE_UL))
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Incompatible type"));
  }
  else
  {
    ecma_object_t *object_p = ecma_get_object_from_value (this_arg);
    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

    ecma_number_t date_num = *ECMA_GET_INTERNAL_VALUE_POINTER (ecma_number_t,
                                                               ext_object_p->u.class_prop.u.value);

    ret_value = ecma_make_number_value (date_num);
  }

  return ret_value;
} /* ecma_date_get_primitive_value */

/**
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_DATE_BUILTIN */

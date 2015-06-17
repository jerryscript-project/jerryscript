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

#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "fdlibm-math.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

/* 24 * 3600 * 1000 */
#define ECMA_MS_PER_DAY 86400000

/**
 * Helper function to get day number from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.2
 *
 * @return  time value for day number
 */
int __attr_always_inline___
ecma_date_day (ecma_number_t time) /**< time value */
{
  return (int) floor (time / ECMA_MS_PER_DAY);
} /* ecma_date_day */

/**
 * Helper function to get time within day from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.2
 *
 * @return  time value within the day
 */
ecma_number_t __attr_always_inline___
ecma_date_time_within_day (ecma_number_t time) /**< time value */
{
  return (ecma_number_t) fmod (time, ECMA_MS_PER_DAY);
} /* ecma_date_time_within_day */

/**
 * Helper function to get number of days from year value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.3
 *
 * @return  number of days
 */
int __attr_always_inline___
ecma_date_days_in_year (ecma_number_t year) /**< year value */
{
  if (fmod (floor (year), 4))
  {
    return 365;
  }

  if (fmod (floor (year), 100))
  {
    return 366;
  }

  if (fmod (floor (year), 400))
  {
    return 365;
  }

  return 366;
} /* ecma_date_days_in_year */

/**
 * Helper function to get the day number of the first day of a year.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.3
 *
 * @return  day number of the first day of a year
 */
int __attr_always_inline___
ecma_date_day_from_year (ecma_number_t year) /**< year value */
{
  return (int) (365 * (year - 1970)
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
  return ECMA_MS_PER_DAY * (ecma_number_t) ecma_date_day_from_year (year);
} /* ecma_date_time_from_year */

/**
 * Helper function to determine a year value from the time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.3
 *
 * @return  year value
 */
int
ecma_date_year_from_time (ecma_number_t time) /**< time value */
{
  /* ECMA-262 v5, 15.9.1.1 define the largest year that is
   * representable (285616) forward from 01 January, 1970 UTC. */
  int year = 285616 + 1970;

  while (ecma_date_time_from_year ((ecma_number_t) year) > time)
  {
    if (ecma_date_time_from_year ((ecma_number_t) (year / 2)) > time)
    {
      year = year / 2;
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
int __attr_always_inline___
ecma_date_in_leap_year (ecma_number_t time) /**< time value */
{
  return ecma_date_days_in_year (ecma_date_time_from_year (time)) - 365;
} /* ecma_date_in_leap_year */

/**
 * Helper function to get day within year from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.4
 *
 * @return  number of days within year
 */
int __attr_always_inline___
ecma_date_day_within_year (ecma_number_t time) /**< time value */
{
  return ecma_date_day (time) - ecma_date_day_from_year ((ecma_number_t) ecma_date_year_from_time (time));
} /* ecma_date_day_within_year */

/**
 * Helper function to get month from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.4
 *
 * @return  month number
 */
int
ecma_date_month_from_time (ecma_number_t time) /**< time value */
{
  int in_leap_year = ecma_date_in_leap_year (time);
  int day_within_year = ecma_date_day_within_year (time);

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
 * @return  date number
 */
int
ecma_date_date_from_time (ecma_number_t time) /**< time value */
{
  int in_leap_year = ecma_date_in_leap_year (time);
  int day_within_year = ecma_date_day_within_year (time);

  switch (ecma_date_month_from_time (time))
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
 * @return  weekday number
 */
int __attr_always_inline___
ecma_date_week_day (ecma_number_t time) /**< time value */
{
  return (ecma_date_day (time) + 4) % 7;
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
  JERRY_UNIMPLEMENTED ("The ecma_date_local_tza is not implemented yet.");
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
ecma_date_daylight_saving_ta (ecma_number_t __attr_unused___ time)
{
  JERRY_UNIMPLEMENTED ("The ecma_date_daylight_saving_ta is not implemented yet.");
} /* ecma_date_daylight_saving_ta */

/**
 * Helper function to get local time from UTC.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.9
 *
 * @return  local time
 */
ecma_number_t __attr_always_inline___
ecma_date_local_time (ecma_number_t __attr_unused___ time)
{
  JERRY_UNIMPLEMENTED ("The ecma_date_local_time is not implemented yet.");
} /* ecma_date_local_time */

/**
 * Helper function to get utc from local time.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.9
 *
 * @return  utc value
 */
ecma_number_t __attr_always_inline___
ecma_date_utc (ecma_number_t __attr_unused___ time)
{
  JERRY_UNIMPLEMENTED ("The ecma_date_utc is not implemented yet.");
} /* ecma_date_utc */

/**
 * Helper function to get hour from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.10
 *
 * @return  hour value
 */
ecma_number_t __attr_always_inline___
ecma_date_hour_from_time (ecma_number_t __attr_unused___ time)
{
  JERRY_UNIMPLEMENTED ("The ecma_date_hour_from_time is not implemented yet.");
} /* ecma_date_hour_from_time */

/**
 * Helper function to get minute from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.10
 *
 * @return  minute value
 */
ecma_number_t __attr_always_inline___
ecma_date_min_from_time (ecma_number_t __attr_unused___ time)
{
  JERRY_UNIMPLEMENTED ("The ecma_date_min_from_time is not implemented yet.");
} /* ecma_date_min_from_time */

/**
 * Helper function to get second from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.10
 *
 * @return  second value
 */
ecma_number_t __attr_always_inline___
ecma_date_sec_from_time (ecma_number_t __attr_unused___ time)
{
  JERRY_UNIMPLEMENTED ("The ecma_date_sec_from_time is not implemented yet.");
} /* ecma_date_sec_from_time */

/**
 * Helper function to get millisecond from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.10
 *
 * @return  millisecond value
 */
ecma_number_t __attr_always_inline___
ecma_date_ms_from_time (ecma_number_t __attr_unused___ time)
{
  JERRY_UNIMPLEMENTED ("The ecma_date_ms_from_time is not implemented yet.");
} /* ecma_date_ms_from_time */

/**
 * Helper function to make time value from hour, min, sec and ms.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.11
 *
 * @return  time value
 */
ecma_number_t __attr_always_inline___
ecma_date_make_time (ecma_number_t __attr_unused___ hour,
                     ecma_number_t __attr_unused___ min,
                     ecma_number_t __attr_unused___ sec,
                     ecma_number_t __attr_unused___ ms)
{
  JERRY_UNIMPLEMENTED ("The ecma_date_make_time is not implemented yet.");
} /* ecma_date_make_time */

/**
 * Helper function to make day value from year, month and date.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.12
 *
 * @return  day value
 */
ecma_number_t __attr_always_inline___
ecma_date_make_day (ecma_number_t __attr_unused___ year,
                    ecma_number_t __attr_unused___ month,
                    ecma_number_t __attr_unused___ date)
{
  JERRY_UNIMPLEMENTED ("The ecma_date_make_day is not implemented yet.");
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
ecma_date_make_date (ecma_number_t __attr_unused___ day, ecma_number_t __attr_unused___ time)
{
  JERRY_UNIMPLEMENTED ("The ecma_date_make_date is not implemented yet.");
} /* ecma_date_make_date */

/**
 * Helper function to calculate number of milliseconds from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.14
 *
 * @return  number of milliseconds
 */
ecma_number_t __attr_always_inline___
ecma_date_time_clip (ecma_number_t __attr_unused___ year)
{
  JERRY_UNIMPLEMENTED ("The ecma_date_time_clip is not implemented yet.");
} /* ecma_date_time_clip */

/**
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN */

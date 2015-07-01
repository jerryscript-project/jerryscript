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

#include "ecma-builtin-helpers.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "fdlibm-math.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltinhelpers ECMA builtin helper operations
 * @{
 */

/**
 * Time range defines for helper functions.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.1, 15.9.1.10
 */
/* Hours in a day. */
#define ECMA_DATE_HOURS_PER_DAY         24
/* Minutes in an hour. */
#define ECMA_DATE_MINUTES_PER_HOUR      60
/* Seconds in a minute. */
#define ECMA_DATE_SECONDS_PER_MINUTE    60
/* Milliseconds in a second. */
#define ECMA_DATE_MS_PER_SECOND         1000
/* ECMA_DATE_MS_PER_MINUTE == 60000 */
#define ECMA_DATE_MS_PER_MINUTE         (ECMA_DATE_MS_PER_SECOND * ECMA_DATE_SECONDS_PER_MINUTE)
/* ECMA_DATE_MS_PER_HOUR == 3600000 */
#define ECMA_DATE_MS_PER_HOUR           (ECMA_DATE_MS_PER_MINUTE * ECMA_DATE_MINUTES_PER_HOUR)
/* ECMA_DATE_MS_PER_DAY == 86400000 */
#define ECMA_DATE_MS_PER_DAY            (ECMA_DATE_MS_PER_HOUR * ECMA_DATE_HOURS_PER_DAY)
/* This gives a range of 8,640,000,000,000,000 milliseconds
 * to either side of 01 January, 1970 UTC.
 */
#define ECMA_DATE_MAX_VALUE             8.64e15

/**
 * Helper function to get day number from time value.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.2
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
  ecma_number_t year = (ecma_number_t) (285616 + 1970);

  while (ecma_date_time_from_year (year) > time)
  {
    if (ecma_date_time_from_year ((ecma_number_t) floor (year / 2)) > time)
    {
      year = (ecma_number_t) floor (year / 2);
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
  JERRY_UNIMPLEMENTED ("The built-in is not implemented.");
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

  JERRY_UNIMPLEMENTED ("The built-in is not implemented.");
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
 * @return  hour value
 */
ecma_number_t __attr_always_inline___
ecma_date_hour_from_time (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  return (ecma_number_t) fmod (floor (time / ECMA_DATE_MS_PER_HOUR),
                               ECMA_DATE_HOURS_PER_DAY);
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
ecma_date_min_from_time (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  return (ecma_number_t) fmod (floor (time / ECMA_DATE_MS_PER_MINUTE),
                               ECMA_DATE_MINUTES_PER_HOUR);
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
ecma_date_sec_from_time (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  return (ecma_number_t) fmod (floor (time / ECMA_DATE_MS_PER_SECOND),
                               ECMA_DATE_SECONDS_PER_MINUTE);
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
ecma_date_ms_from_time (ecma_number_t time) /**< time value */
{
  if (ecma_number_is_nan (time))
  {
    return time; /* time is NaN */
  }

  return (ecma_number_t) fmod (time, ECMA_DATE_MS_PER_SECOND);
} /* ecma_date_ms_from_time */

/**
 * Helper function to make time value from hour, min, sec and ms.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.11
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
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN */

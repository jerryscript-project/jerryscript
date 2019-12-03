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
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-try-catch-macro.h"
#include "lit-char-helpers.h"

#if ENABLED (JERRY_BUILTIN_DATE)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-date.inc.h"
#define BUILTIN_UNDERSCORED_ID date
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup date ECMA Date object built-in
 * @{
 */

/**
 * Helper function to try to parse a part of a date string
 *
 * @return NaN if cannot read from string, ToNumber() otherwise
 */
static ecma_number_t
ecma_date_parse_date_chars (const lit_utf8_byte_t **str_p, /**< pointer to the cesu8 string */
                            const lit_utf8_byte_t *str_end_p, /**< pointer to the end of the string */
                            uint32_t num_of_chars, /**< number of characters to read and convert */
                            uint32_t min, /**< minimum valid value */
                            uint32_t max) /**< maximum valid value */
{
  JERRY_ASSERT (num_of_chars > 0);
  const lit_utf8_byte_t *str_start_p = *str_p;

  while (num_of_chars--)
  {
    if (*str_p >= str_end_p || !lit_char_is_decimal_digit (lit_utf8_read_next (str_p)))
    {
      return ecma_number_make_nan ();
    }
  }

  ecma_number_t parsed_number = ecma_utf8_string_to_number (str_start_p, (lit_utf8_size_t) (*str_p - str_start_p));

  if (parsed_number < min || parsed_number > max)
  {
    return ecma_number_make_nan ();
  }

  return parsed_number;
} /* ecma_date_parse_date_chars */

/**
 * Helper function to try to parse a special chracter (+,-,T,Z,:,.) in a date string
 *
 * @return true if the first character is same as the expected, false otherwise
 */
static bool
ecma_date_parse_special_char (const lit_utf8_byte_t **str_p, /**< pointer to the cesu8 string */
                              const lit_utf8_byte_t *str_end_p, /**< pointer to the end of the string */
                              const lit_utf8_byte_t expected_char) /**< expected character */
{
  if ((*str_p < str_end_p) && (**str_p == expected_char))
  {
    (*str_p)++;
    return true;
  }

  return false;
} /* ecma_date_parse_special_char */

/**
 * Helper function to try to parse a 4-5-6 digit year with optional negative sign in a date string
 *
 * Date.prototype.toString() and Date.prototype.toUTCString() emits year
 * in this format and Date.parse() should parse this format too.
 *
 * @return the parsed year or NaN.
 */
static ecma_number_t
ecma_date_parse_year (const lit_utf8_byte_t **str_p, /**< pointer to the cesu8 string */
                      const lit_utf8_byte_t *str_end_p) /**< pointer to the end of the string */
{
  bool is_year_sign_negative = ecma_date_parse_special_char (str_p, str_end_p, '-');
  const lit_utf8_byte_t *str_start_p = *str_p;
  int32_t parsed_year = 0;

  while ((str_start_p - *str_p < 6) && (str_start_p < str_end_p) && lit_char_is_decimal_digit (*str_start_p))
  {
    parsed_year = 10 * parsed_year + *str_start_p - LIT_CHAR_0;
    str_start_p++;
  }

  if (str_start_p - *str_p >=4)
  {
    *str_p = str_start_p;
    if (is_year_sign_negative)
    {
      return -parsed_year;
    }
    return parsed_year;
  }

  return ecma_number_make_nan ();
} /* ecma_date_parse_year */

/**
 * Helper function to try to parse a day name in a date string
 * Valid day names: Sun, Mon, Tue, Wed, Thu, Fri, Sat
 * See also:
 *          ECMA-262 v9, 20.3.4.41.2 Table 46
 *
 * @return true if the string starts with a valid day name, false otherwise
 */
static bool
ecma_date_parse_day_name (const lit_utf8_byte_t **str_p, /**< pointer to the cesu8 string */
                          const lit_utf8_byte_t *str_end_p) /**< pointer to the end of the string */
{
  if (*str_p + 3 < str_end_p)
  {
    for (uint32_t i = 0; i < 7; i++)
    {
      if (!memcmp (day_names_p[i], *str_p, 3))
      {
        (*str_p) += 3;
        return true;
      }
    }
  }
  return false;
} /* ecma_date_parse_day_name */

/**
 * Helper function to try to parse a month name in a date string
 * Valid month names: Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec
 * See also:
 *          ECMA-262 v9, 20.3.4.41.2 Table 47
 *
 * @return number of the month if the string starts with a valid month name, 0 otherwise
 */
static uint32_t
ecma_date_parse_month_name (const lit_utf8_byte_t **str_p, /**< pointer to the cesu8 string */
                            const lit_utf8_byte_t *str_end_p) /**< pointer to the end of the string */
{
  if (*str_p + 3 < str_end_p)
  {
    for (uint32_t i = 0; i < 12; i++)
    {
      if (!memcmp (month_names_p[i], *str_p, 3))
      {
        (*str_p) += 3;
        return (i+1);
      }
    }
  }
  return 0;
} /* ecma_date_parse_month_name */


/**
  * Calculate MakeDate(MakeDay(yr, m, dt), MakeTime(h, min, s, milli)) for Date constructor and UTC
  *
  * See also:
  *          ECMA-262 v5, 15.9.3.1
  *          ECMA-262 v5, 15.9.4.3
  *
  * @return result of MakeDate(MakeDay(yr, m, dt), MakeTime(h, min, s, milli))
  */
static ecma_value_t
ecma_date_construct_helper (const ecma_value_t *args, /**< arguments passed to the Date constructor */
                            ecma_length_t args_len) /**< number of arguments */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;
  ecma_number_t prim_value = ecma_number_make_nan ();

  ECMA_TRY_CATCH (year_value, ecma_op_to_number (args[0]), ret_value);
  ECMA_TRY_CATCH (month_value, ecma_op_to_number (args[1]), ret_value);

  ecma_number_t year = ecma_get_number_from_value (year_value);
  ecma_number_t month = ecma_get_number_from_value (month_value);
  ecma_number_t date = ECMA_NUMBER_ONE;
  ecma_number_t hours = ECMA_NUMBER_ZERO;
  ecma_number_t minutes = ECMA_NUMBER_ZERO;
  ecma_number_t seconds = ECMA_NUMBER_ZERO;
  ecma_number_t milliseconds = ECMA_NUMBER_ZERO;

  /* 3. */
  if (args_len >= 3 && ecma_is_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (date_value, ecma_op_to_number (args[2]), ret_value);
    date = ecma_get_number_from_value (date_value);
    ECMA_FINALIZE (date_value);
  }

  /* 4. */
  if (args_len >= 4 && ecma_is_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (hours_value, ecma_op_to_number (args[3]), ret_value);
    hours = ecma_get_number_from_value (hours_value);
    ECMA_FINALIZE (hours_value);
  }

  /* 5. */
  if (args_len >= 5 && ecma_is_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (minutes_value, ecma_op_to_number (args[4]), ret_value);
    minutes = ecma_get_number_from_value (minutes_value);
    ECMA_FINALIZE (minutes_value);
  }

  /* 6. */
  if (args_len >= 6 && ecma_is_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (seconds_value, ecma_op_to_number (args[5]), ret_value);
    seconds = ecma_get_number_from_value (seconds_value);
    ECMA_FINALIZE (seconds_value);
  }

  /* 7. */
  if (args_len >= 7 && ecma_is_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (milliseconds_value, ecma_op_to_number (args[6]), ret_value);
    milliseconds = ecma_get_number_from_value (milliseconds_value);
    ECMA_FINALIZE (milliseconds_value);
  }

  if (ecma_is_value_empty (ret_value))
  {
    if (!ecma_number_is_nan (year))
    {
      /* 8. */
      ecma_number_t y = ecma_number_trunc (year);

      if (y >= 0 && y <= 99)
      {
        year = 1900 + y;
      }
    }

    prim_value = ecma_date_make_date (ecma_date_make_day (year,
                                                          month,
                                                          date),
                                      ecma_date_make_time (hours,
                                                           minutes,
                                                           seconds,
                                                           milliseconds));
  }

  ECMA_FINALIZE (month_value);
  ECMA_FINALIZE (year_value);

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = ecma_make_number_value (prim_value);
  }

  return ret_value;
} /* ecma_date_construct_helper */

/**
 * Helper function used by ecma_builtin_date_parse
 *
 * See also:
 *          ECMA-262 v5, 15.9.4.2  Date.parse (string)
 *          ECMA-262 v5, 15.9.1.15 Date Time String Format
 *
 * @return the parsed date as ecma_number_t or NaN otherwise
 */
static ecma_number_t
ecma_builtin_date_parse_ISO_string_format (const lit_utf8_byte_t *date_str_curr_p,
                                           const lit_utf8_byte_t *date_str_end_p)
{
  /* 1. read year */

  uint32_t year_digits = 4;

  bool is_year_sign_negative = ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, '-');
  if (is_year_sign_negative || ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, '+'))
  {
    year_digits = 6;
  }

  ecma_number_t year = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, year_digits,
                                                   0, (year_digits == 4) ? 9999 : 999999);
  if (is_year_sign_negative)
  {
    year = -year;
  }

  if (!ecma_number_is_nan (year))
  {
    ecma_number_t month = ECMA_NUMBER_ONE;
    ecma_number_t day = ECMA_NUMBER_ONE;
    ecma_number_t time = ECMA_NUMBER_ZERO;

    /* 2. read month if any */
    if (ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, '-'))
    {
      month = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 2, 1, 12);
    }

    /* 3. read day if any */
    if (ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, '-'))
    {
      day = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 2, 1, 31);
    }

    /* 4. read time if any */
    if (ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, 'T'))
    {
      ecma_number_t hours = ECMA_NUMBER_ZERO;
      ecma_number_t minutes = ECMA_NUMBER_ZERO;
      ecma_number_t seconds = ECMA_NUMBER_ZERO;
      ecma_number_t milliseconds = ECMA_NUMBER_ZERO;

      ecma_length_t remaining_length = lit_utf8_string_length (date_str_curr_p,
                                                               (lit_utf8_size_t) (date_str_end_p - date_str_curr_p));

      if (remaining_length >= 5)
      {
        /* 4.1 read hours and minutes */
        hours = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 2, 0, 24);

        if (ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, ':'))
        {
          minutes = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 2, 0, 59);

          /* 4.2 read seconds if any */
          if (ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, ':'))
          {
            seconds = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 2, 0, 59);

            /* 4.3 read milliseconds if any */
            if (ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, '.'))
            {
              milliseconds = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 3, 0, 999);
            }
          }
        }
        else
        {
          minutes = ecma_number_make_nan ();
        }

        if (hours == 24 && (minutes != 0 || seconds != 0 || milliseconds != 0))
        {
          hours = ecma_number_make_nan ();
        }

        time = ecma_date_make_time (hours, minutes, seconds, milliseconds);
      }
      else
      {
        time = ecma_number_make_nan ();
      }

      /* 4.4 read timezone if any */
      if (ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, 'Z') && !ecma_number_is_nan (time))
      {
        time = ecma_date_make_time (hours, minutes, seconds, milliseconds);
      }
      else
      {
        bool is_timezone_sign_negative;
        if ((lit_utf8_string_length (date_str_curr_p, (lit_utf8_size_t) (date_str_end_p - date_str_curr_p)) == 6)
            && ((is_timezone_sign_negative = ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, '-'))
            || ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, '+')))
        {
          /* read hours and minutes */
          hours = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 2, 0, 24);

          if (hours == 24)
          {
            hours = ECMA_NUMBER_ZERO;
          }

          ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, ':');
          minutes = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 2, 0, 59);
          ecma_number_t timezone_offset = ecma_date_make_time (hours, minutes, ECMA_NUMBER_ZERO, ECMA_NUMBER_ZERO);
          time += is_timezone_sign_negative ? timezone_offset : -timezone_offset;
        }
      }
    }

    if (date_str_curr_p >= date_str_end_p)
    {
      ecma_number_t date = ecma_date_make_day (year, month - 1, day);
      return ecma_date_make_date (date, time);
    }
  }
  return ecma_number_make_nan ();
} /* ecma_builtin_date_parse_ISO_string_format */

/**
 * Helper function used by ecma_builtin_date_parse
 *
 * See also:
 *          ECMA-262 v5, 15.9.4.2  Date.parse (string)
 *          ECMA-262 v9, 20.3.4.41 Date.prototype.toString ()
 *          ECMA-262 v9, 20.3.4.43 Date.prototype.toUTCString ()
 *
 * Used by: ecma_builtin_date_parse
 *
 * @return the parsed date as ecma_number_t or NaN otherwise
 */
static ecma_number_t
ecma_builtin_date_parse_toString_formats (const lit_utf8_byte_t *date_str_curr_p,
                                          const lit_utf8_byte_t *date_str_end_p)
{
  const ecma_number_t nan = ecma_number_make_nan ();

  if (!ecma_date_parse_day_name (&date_str_curr_p, date_str_end_p))
  {
    return nan;
  }

  const bool is_toUTCString_format = ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, ',');

  if (!ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, ' '))
  {
    return nan;
  }

  ecma_number_t month = 0;
  ecma_number_t day = 0;
  if (is_toUTCString_format)
  {
    day = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 2, 0, 31);
    if (ecma_number_is_nan (day))
    {
      return nan;
    }

    if (!ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, ' '))
    {
      return nan;
    }

    month = ecma_date_parse_month_name (&date_str_curr_p, date_str_end_p);
    if (!(int) month)
    {
      return nan;
    }
  }
  else
  {
    month = ecma_date_parse_month_name (&date_str_curr_p, date_str_end_p);
    if (!(int) month)
    {
      return nan;
    }

    if (!ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, ' '))
    {
      return nan;
    }

    day = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 2, 0, 31);
    if (ecma_number_is_nan (day))
    {
      return nan;
    }
  }

  if (!ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, ' '))
  {
    return nan;
  }

  ecma_number_t year = ecma_date_parse_year (&date_str_curr_p, date_str_end_p);
  if (ecma_number_is_nan (year))
  {
    return nan;
  }

  if (!ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, ' '))
  {
    return nan;
  }

  ecma_number_t hours = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 2, 0, 24);
  if (ecma_number_is_nan (hours))
  {
    return nan;
  }

  if (!ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, ':'))
  {
    return nan;
  }

  ecma_number_t minutes = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 2, 0, 59);
  if (ecma_number_is_nan (minutes))
  {
    return nan;
  }

  if (!ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, ':'))
  {
    return nan;
  }

  ecma_number_t seconds = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 2, 0, 59);
  if (ecma_number_is_nan (seconds))
  {
    return nan;
  }

  if (hours == 24 && (minutes != 0 || seconds != 0))
  {
    return nan;
  }

  if (!ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, ' '))
  {
    return nan;
  }

  if (!ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, 'G'))
  {
    return nan;
  }

  if (!ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, 'M'))
  {
    return nan;
  }

  if (!ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, 'T'))
  {
    return nan;
  }

  ecma_number_t time = ecma_date_make_time (hours, minutes, seconds, 0);

  if (!is_toUTCString_format)
  {
    bool is_timezone_sign_negative = ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, '-');
    if (!is_timezone_sign_negative && !ecma_date_parse_special_char (&date_str_curr_p, date_str_end_p, '+'))
    {
      return nan;
    }

    hours = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 2, 0, 24);
    if (ecma_number_is_nan (hours))
    {
      return nan;
    }
    if (hours == 24)
    {
      hours = ECMA_NUMBER_ZERO;
    }

    minutes = ecma_date_parse_date_chars (&date_str_curr_p, date_str_end_p, 2, 0, 59);
    if (ecma_number_is_nan (minutes))
    {
      return nan;
    }

    ecma_number_t timezone_offset = ecma_date_make_time (hours, minutes, ECMA_NUMBER_ZERO, ECMA_NUMBER_ZERO);
    time += is_timezone_sign_negative ? timezone_offset : -timezone_offset;
  }

  if (date_str_curr_p >= date_str_end_p)
  {
    ecma_number_t date = ecma_date_make_day (year, month - 1, day);
    return ecma_date_make_date (date, time);
  }

  return nan;
} /* ecma_builtin_date_parse_toString_formats */

/**
 * The Date object's 'parse' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.4.2  Date.parse (string)
 *          ECMA-262 v5, 15.9.1.15 Date Time String Format
 *          ECMA-262 v9, 20.3.4.41 Date.prototype.toString ()
 *          ECMA-262 v9, 20.3.4.43 Date.prototype.toUTCString ()
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_parse (ecma_value_t this_arg, /**< this argument */
                         ecma_value_t arg) /**< string */
{
  JERRY_UNUSED (this_arg);

  /* Date Time String fromat (ECMA-262 v5, 15.9.1.15) */
  ecma_string_t *date_str_p = ecma_op_to_string (arg);
  if (JERRY_UNLIKELY (date_str_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ECMA_STRING_TO_UTF8_STRING (date_str_p, date_start_p, date_start_size);
  const lit_utf8_byte_t *date_str_curr_p = date_start_p;
  const lit_utf8_byte_t *date_str_end_p = date_start_p + date_start_size;

  // try to parse date string as ISO string - ECMA-262 v5, 15.9.1.15
  ecma_number_t ret_value = ecma_builtin_date_parse_ISO_string_format (date_str_curr_p, date_str_end_p);

  if (ecma_number_is_nan (ret_value))
  {
    // try to parse date string in Date.prototype.toString() or toUTCString() format
    ret_value = ecma_builtin_date_parse_toString_formats (date_str_curr_p, date_str_end_p);
  }

  ECMA_FINALIZE_UTF8_STRING (date_start_p, date_start_size);
  ecma_deref_ecma_string (date_str_p);
  return ecma_make_number_value (ret_value);
} /* ecma_builtin_date_parse */

/**
 * The Date object's 'UTC' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_utc (ecma_value_t this_arg, /**< this argument */
                       const ecma_value_t args[], /**< arguments list */
                       ecma_length_t args_number) /**< number of arguments */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  if (args_number < 2)
  {
    /* Note:
     *      When the UTC function is called with fewer than two arguments,
     *      the behaviour is implementation-dependent, so just return NaN.
     */
    return ecma_make_number_value (ecma_number_make_nan ());
  }

  ECMA_TRY_CATCH (time_value, ecma_date_construct_helper (args, args_number), ret_value);

  ecma_number_t time = ecma_get_number_from_value (time_value);
  ret_value = ecma_make_number_value (ecma_date_time_clip (time));

  ECMA_FINALIZE (time_value);

  return ret_value;
} /* ecma_builtin_date_utc */

/**
 * The Date object's 'now' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.4.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_now (ecma_value_t this_arg) /**< this argument */
{
  JERRY_UNUSED (this_arg);
  return ecma_make_number_value (floor (DOUBLE_TO_ECMA_NUMBER_T (jerry_port_get_current_time ())));
} /* ecma_builtin_date_now */

/**
 * Handle calling [[Call]] of built-in Date object
 *
 * See also:
 *          ECMA-262 v5, 15.9.2.1
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_date_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                 ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_UNUSED (arguments_list_p);
  JERRY_UNUSED (arguments_list_len);
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  ECMA_TRY_CATCH (now_val,
                  ecma_builtin_date_now (ECMA_VALUE_UNDEFINED),
                  ret_value);

  ret_value = ecma_date_value_to_string (ecma_get_number_from_value (now_val));

  ECMA_FINALIZE (now_val);

  return ret_value;
} /* ecma_builtin_date_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Date object
 *
 * See also:
 *          ECMA-262 v5, 15.9.3.1
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_date_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                      ecma_length_t arguments_list_len) /**< number of arguments */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;
  ecma_number_t prim_value_num = ECMA_NUMBER_ZERO;

  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_DATE_PROTOTYPE);
  ecma_object_t *obj_p = ecma_create_object (prototype_obj_p,
                                             sizeof (ecma_extended_object_t),
                                             ECMA_OBJECT_TYPE_CLASS);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;
  ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_UNDEFINED;

  if (arguments_list_len == 0)
  {
    ECMA_TRY_CATCH (parse_res_value,
                    ecma_builtin_date_now (ecma_make_object_value (obj_p)),
                    ret_value);

    prim_value_num = ecma_get_number_from_value (parse_res_value);

    ECMA_FINALIZE (parse_res_value)
  }
  else if (arguments_list_len == 1)
  {
    ECMA_TRY_CATCH (prim_comp_value,
                    ecma_op_to_primitive (arguments_list_p[0], ECMA_PREFERRED_TYPE_NUMBER),
                    ret_value);

    if (ecma_is_value_string (prim_comp_value))
    {
      ECMA_TRY_CATCH (parse_res_value,
                      ecma_builtin_date_parse (ecma_make_object_value (obj_p), prim_comp_value),
                      ret_value);

      prim_value_num = ecma_get_number_from_value (parse_res_value);

      ECMA_FINALIZE (parse_res_value);
    }
    else
    {
      ECMA_TRY_CATCH (prim_value, ecma_op_to_number (arguments_list_p[0]), ret_value);

      prim_value_num = ecma_date_time_clip (ecma_get_number_from_value (prim_value));

      ECMA_FINALIZE (prim_value);
    }

    ECMA_FINALIZE (prim_comp_value);
  }
  else
  {
    ECMA_TRY_CATCH (time_value,
                    ecma_date_construct_helper (arguments_list_p, arguments_list_len),
                    ret_value);

    ecma_number_t time = ecma_get_number_from_value (time_value);
    prim_value_num = ecma_date_time_clip (ecma_date_utc (time));

    ECMA_FINALIZE (time_value);
  }

  if (ecma_is_value_empty (ret_value))
  {
    if (!ecma_number_is_nan (prim_value_num) && ecma_number_is_infinity (prim_value_num))
    {
      prim_value_num = ecma_number_make_nan ();
    }

    ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_DATE_UL;

    ecma_number_t *date_num_p = ecma_alloc_number ();
    *date_num_p = prim_value_num;
    ECMA_SET_INTERNAL_VALUE_POINTER (ext_object_p->u.class_prop.u.value, date_num_p);

    ret_value = ecma_make_object_value (obj_p);
  }
  else
  {
    JERRY_ASSERT (ECMA_IS_VALUE_ERROR (ret_value));
    ecma_deref_object (obj_p);
  }

  return ret_value;
} /* ecma_builtin_date_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

#undef BREAK_IF_FALSE
#undef BREAK_IF_NAN

#endif /* ENABLED (JERRY_BUILTIN_DATE) */

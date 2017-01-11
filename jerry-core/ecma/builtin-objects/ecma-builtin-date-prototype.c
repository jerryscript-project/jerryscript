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
#include "ecma-function-object.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"

#ifndef CONFIG_DISABLE_DATE_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/**
 * This object has a custom dispatch function.
 */
#define BUILTIN_CUSTOM_DISPATCH

/**
 * Checks whether the function uses UTC time zone.
 */
#define BUILTIN_DATE_FUNCTION_IS_UTC(builtin_routine_id) \
  (((builtin_routine_id) - ECMA_DATE_PROTOTYPE_GET_FULL_YEAR) & 0x1)

/**
 * List of built-in routine identifiers.
 */
enum
{
  ECMA_DATE_PROTOTYPE_ROUTINE_START = ECMA_BUILTIN_ID__COUNT - 1,

  ECMA_DATE_PROTOTYPE_GET_FULL_YEAR, /* ECMA-262 v5 15.9.5.10 */
  ECMA_DATE_PROTOTYPE_GET_UTC_FULL_YEAR, /* ECMA-262 v5 15.9.5.11 */
#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN
  ECMA_DATE_PROTOTYPE_GET_YEAR, /* ECMA-262 v5, AnnexB.B.2.4 */
  ECMA_DATE_PROTOTYPE_GET_UTC_YEAR, /* has no UTC variant */
#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */
  ECMA_DATE_PROTOTYPE_GET_MONTH, /* ECMA-262 v5 15.9.5.12 */
  ECMA_DATE_PROTOTYPE_GET_UTC_MONTH, /* ECMA-262 v5 15.9.5.13 */
  ECMA_DATE_PROTOTYPE_GET_DATE, /* ECMA-262 v5 15.9.5.14 */
  ECMA_DATE_PROTOTYPE_GET_UTC_DATE, /* ECMA-262 v5 15.9.5.15 */
  ECMA_DATE_PROTOTYPE_GET_DAY, /* ECMA-262 v5 15.9.5.16 */
  ECMA_DATE_PROTOTYPE_GET_UTC_DAY, /* ECMA-262 v5 15.9.5.17 */
  ECMA_DATE_PROTOTYPE_GET_HOURS, /* ECMA-262 v5 15.9.5.18 */
  ECMA_DATE_PROTOTYPE_GET_UTC_HOURS, /* ECMA-262 v5 15.9.5.19 */
  ECMA_DATE_PROTOTYPE_GET_MINUTES, /* ECMA-262 v5 15.9.5.20 */
  ECMA_DATE_PROTOTYPE_GET_UTC_MINUTES, /* ECMA-262 v5 15.9.5.21 */
  ECMA_DATE_PROTOTYPE_GET_SECONDS, /* ECMA-262 v5 15.9.5.22 */
  ECMA_DATE_PROTOTYPE_GET_UTC_SECONDS, /* ECMA-262 v5 15.9.5.23 */
  ECMA_DATE_PROTOTYPE_GET_MILLISECONDS, /* ECMA-262 v5 15.9.5.24 */
  ECMA_DATE_PROTOTYPE_GET_UTC_MILLISECONDS, /* ECMA-262 v5 15.9.5.25 */
  ECMA_DATE_PROTOTYPE_GET_TIMEZONE_OFFSET, /* has no local time zone variant */
  ECMA_DATE_PROTOTYPE_GET_UTC_TIMEZONE_OFFSET, /* ECMA-262 v5 15.9.5.26 */

  ECMA_DATE_PROTOTYPE_SET_FULL_YEAR, /* ECMA-262 v5, 15.9.5.40 */
  ECMA_DATE_PROTOTYPE_SET_UTC_FULL_YEAR, /* ECMA-262 v5, 15.9.5.41 */
#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN
  ECMA_DATE_PROTOTYPE_SET_YEAR, /* ECMA-262 v5, ECMA-262 v5, AnnexB.B.2.5 */
  ECMA_DATE_PROTOTYPE_SET_UTC_YEAR, /* has no UTC variant */
#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */
  ECMA_DATE_PROTOTYPE_SET_MONTH, /* ECMA-262 v5, 15.9.5.38 */
  ECMA_DATE_PROTOTYPE_SET_UTC_MONTH, /* ECMA-262 v5, 15.9.5.39 */
  ECMA_DATE_PROTOTYPE_SET_DATE, /* ECMA-262 v5, 15.9.5.36 */
  ECMA_DATE_PROTOTYPE_SET_UTC_DATE, /* ECMA-262 v5, 15.9.5.37 */
  ECMA_DATE_PROTOTYPE_SET_HOURS, /* ECMA-262 v5, 15.9.5.34 */
  ECMA_DATE_PROTOTYPE_SET_UTC_HOURS, /* ECMA-262 v5, 15.9.5.35 */
  ECMA_DATE_PROTOTYPE_SET_MINUTES, /* ECMA-262 v5, 15.9.5.32 */
  ECMA_DATE_PROTOTYPE_SET_UTC_MINUTES, /* ECMA-262 v5, 15.9.5.33 */
  ECMA_DATE_PROTOTYPE_SET_SECONDS, /* ECMA-262 v5, 15.9.5.30 */
  ECMA_DATE_PROTOTYPE_SET_UTC_SECONDS, /* ECMA-262 v5, 15.9.5.31 */
  ECMA_DATE_PROTOTYPE_SET_MILLISECONDS, /* ECMA-262 v5, 15.9.5.28 */
  ECMA_DATE_PROTOTYPE_SET_UTC_MILLISECONDS, /* ECMA-262 v5, 15.9.5.29 */

  ECMA_DATE_PROTOTYPE_TO_STRING, /* ECMA-262 v5, 15.9.5.2 */
  ECMA_DATE_PROTOTYPE_TO_DATE_STRING, /* ECMA-262 v5, 15.9.5.3 */
  ECMA_DATE_PROTOTYPE_TO_TIME_STRING, /* ECMA-262 v5, 15.9.5.4 */
  ECMA_DATE_PROTOTYPE_TO_UTC_STRING, /* ECMA-262 v5, 15.9.5.42 */
  ECMA_DATE_PROTOTYPE_TO_ISO_STRING, /* ECMA-262 v5, 15.9.5.43 */

  ECMA_DATE_PROTOTYPE_GET_TIME, /* ECMA-262 v5, 15.9.5.9 */
  ECMA_DATE_PROTOTYPE_SET_TIME, /* ECMA-262 v5, 15.9.5.27 */
  ECMA_DATE_PROTOTYPE_TO_JSON, /* ECMA-262 v5, 15.9.5.44 */
};

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-date-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID date_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup dateprototype ECMA Date.prototype object built-in
 * @{
 */

/**
 * The Date.prototype object's 'toJSON' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.44
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_to_json (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (obj,
                  ecma_op_to_object (this_arg),
                  ret_value);

  /* 2. */
  ECMA_TRY_CATCH (tv,
                  ecma_op_to_primitive (obj, ECMA_PREFERRED_TYPE_NUMBER),
                  ret_value);

  /* 3. */
  if (ecma_is_value_number (tv))
  {
    ecma_number_t num_value = ecma_get_number_from_value (tv);

    if (ecma_number_is_nan (num_value) || ecma_number_is_infinity (num_value))
    {
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
    }
  }

  if (ecma_is_value_empty (ret_value))
  {
    ecma_string_t *to_iso_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_TO_ISO_STRING_UL);
    ecma_object_t *value_obj_p = ecma_get_object_from_value (obj);

    /* 4. */
    ECMA_TRY_CATCH (to_iso,
                    ecma_op_object_get (value_obj_p, to_iso_str_p),
                    ret_value);

    /* 5. */
    if (!ecma_op_is_callable (to_iso))
    {
      ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("'toISOString' is missing or not a function."));
    }
    /* 6. */
    else
    {
      ecma_object_t *to_iso_obj_p = ecma_get_object_from_value (to_iso);
      ret_value = ecma_op_function_call (to_iso_obj_p, this_arg, NULL, 0);
    }

    ECMA_FINALIZE (to_iso);

    ecma_deref_ecma_string (to_iso_str_p);
  }

  ECMA_FINALIZE (tv);
  ECMA_FINALIZE (obj);

  return ret_value;
} /* ecma_builtin_date_prototype_to_json */

/**
 * Dispatch get date functions
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_dispatch_get (uint16_t builtin_routine_id, /**< built-in wide routine
                                                                        *   identifier */
                                          ecma_number_t date_num) /**< date converted to number */
{
  if (ecma_number_is_nan (date_num))
  {
    ecma_string_t *nan_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_NAN);
    return ecma_make_string_value (nan_str_p);
  }

  switch (builtin_routine_id)
  {
    case ECMA_DATE_PROTOTYPE_GET_FULL_YEAR:
    case ECMA_DATE_PROTOTYPE_GET_UTC_FULL_YEAR:
#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN
    case ECMA_DATE_PROTOTYPE_GET_YEAR:
#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */
    {
      date_num = ecma_date_year_from_time (date_num);

#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN
      if (builtin_routine_id == ECMA_DATE_PROTOTYPE_GET_YEAR)
      {
        date_num -= 1900;
      }
#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */

      break;
    }
    case ECMA_DATE_PROTOTYPE_GET_MONTH:
    case ECMA_DATE_PROTOTYPE_GET_UTC_MONTH:
    {
      date_num = ecma_date_month_from_time (date_num);
      break;
    }
    case ECMA_DATE_PROTOTYPE_GET_DATE:
    case ECMA_DATE_PROTOTYPE_GET_UTC_DATE:
    {
      date_num = ecma_date_date_from_time (date_num);
      break;
    }
    case ECMA_DATE_PROTOTYPE_GET_DAY:
    case ECMA_DATE_PROTOTYPE_GET_UTC_DAY:
    {
      date_num = ecma_date_week_day (date_num);
      break;
    }
    case ECMA_DATE_PROTOTYPE_GET_HOURS:
    case ECMA_DATE_PROTOTYPE_GET_UTC_HOURS:
    {
      date_num = ecma_date_hour_from_time (date_num);
      break;
    }
    case ECMA_DATE_PROTOTYPE_GET_MINUTES:
    case ECMA_DATE_PROTOTYPE_GET_UTC_MINUTES:
    {
      date_num = ecma_date_min_from_time (date_num);
      break;
    }
    case ECMA_DATE_PROTOTYPE_GET_SECONDS:
    case ECMA_DATE_PROTOTYPE_GET_UTC_SECONDS:
    {
      date_num = ecma_date_sec_from_time (date_num);
      break;
    }
    case ECMA_DATE_PROTOTYPE_GET_MILLISECONDS:
    case ECMA_DATE_PROTOTYPE_GET_UTC_MILLISECONDS:
    {
      date_num = ecma_date_ms_from_time (date_num);
      break;
    }
    default:
    {
      JERRY_ASSERT (builtin_routine_id == ECMA_DATE_PROTOTYPE_GET_UTC_TIMEZONE_OFFSET);
      date_num = ecma_date_timezone_offset (date_num);
      break;
    }
  }

  return ecma_make_number_value (date_num);
} /* ecma_builtin_date_prototype_dispatch_get */

#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN

/**
 * Returns true, if the built-in id sets a year.
 */
#define ECMA_DATE_PROTOTYPE_IS_SET_YEAR_ROUTINE(builtin_routine_id) \
  ((builtin_routine_id) == ECMA_DATE_PROTOTYPE_SET_FULL_YEAR \
   || (builtin_routine_id) == ECMA_DATE_PROTOTYPE_SET_UTC_FULL_YEAR \
   || (builtin_routine_id) == ECMA_DATE_PROTOTYPE_SET_YEAR)

#else /* CONFIG_DISABLE_ANNEXB_BUILTIN */

/**
 * Returns true, if the built-in id sets a year.
 */
#define ECMA_DATE_PROTOTYPE_IS_SET_YEAR_ROUTINE(builtin_routine_id) \
  ((builtin_routine_id) == ECMA_DATE_PROTOTYPE_SET_FULL_YEAR \
   || (builtin_routine_id) == ECMA_DATE_PROTOTYPE_SET_UTC_FULL_YEAR)

#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */

/**
 * Dispatch set date functions
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_dispatch_set (uint16_t builtin_routine_id, /**< built-in wide routine
                                                                        *   identifier */
                                          ecma_extended_object_t *ext_object_p, /**< date extended object */
                                          ecma_number_t date_num, /**< date converted to number */
                                          const ecma_value_t arguments_list[], /**< list of arguments
                                                                                *   passed to routine */
                                          ecma_length_t arguments_number) /**< length of arguments' list */
{
  ecma_number_t converted_number[4];
  ecma_length_t conversions = 0;

  /* If the first argument is not specified, it is always converted to NaN. */
  converted_number[0] = ecma_number_make_nan ();

  switch (builtin_routine_id)
  {
#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN
    case ECMA_DATE_PROTOTYPE_SET_YEAR:
#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */
    case ECMA_DATE_PROTOTYPE_SET_DATE:
    case ECMA_DATE_PROTOTYPE_SET_UTC_DATE:
    case ECMA_DATE_PROTOTYPE_SET_UTC_MILLISECONDS:
    case ECMA_DATE_PROTOTYPE_SET_MILLISECONDS:
    {
      conversions = 1;
      break;
    }
    case ECMA_DATE_PROTOTYPE_SET_MONTH:
    case ECMA_DATE_PROTOTYPE_SET_UTC_MONTH:
    case ECMA_DATE_PROTOTYPE_SET_UTC_SECONDS:
    case ECMA_DATE_PROTOTYPE_SET_SECONDS:
    {
      conversions = 2;
      break;
    }
    case ECMA_DATE_PROTOTYPE_SET_FULL_YEAR:
    case ECMA_DATE_PROTOTYPE_SET_UTC_FULL_YEAR:
    case ECMA_DATE_PROTOTYPE_SET_MINUTES:
    case ECMA_DATE_PROTOTYPE_SET_UTC_MINUTES:
    {
      conversions = 3;
      break;
    }
    default:
    {
      JERRY_ASSERT (builtin_routine_id == ECMA_DATE_PROTOTYPE_SET_HOURS
                    || builtin_routine_id == ECMA_DATE_PROTOTYPE_SET_UTC_HOURS);
      conversions = 4;
      break;
    }
  }

  if (conversions > arguments_number)
  {
    conversions = arguments_number;
  }

  for (ecma_length_t i = 0; i < conversions; i++)
  {
    ecma_value_t value = ecma_op_to_number (arguments_list[i]);

    if (ECMA_IS_VALUE_ERROR (value))
    {
      return value;
    }

    converted_number[i] = ecma_get_number_from_value (value);
    ecma_free_value (value);
  }

  ecma_number_t day_part;
  ecma_number_t time_part;

  if (builtin_routine_id <= ECMA_DATE_PROTOTYPE_SET_UTC_DATE)
  {
    if (ecma_number_is_nan (date_num))
    {
      if (ECMA_DATE_PROTOTYPE_IS_SET_YEAR_ROUTINE (builtin_routine_id))
      {
        date_num = ECMA_NUMBER_ZERO;
      }
      else
      {
        return ecma_make_number_value (date_num);
      }
    }

    time_part = ecma_date_time_within_day (date_num);

    ecma_number_t year = ecma_date_year_from_time (date_num);
    ecma_number_t month = ecma_date_month_from_time (date_num);
    ecma_number_t day = ecma_date_date_from_time (date_num);

    switch (builtin_routine_id)
    {
      case ECMA_DATE_PROTOTYPE_SET_FULL_YEAR:
      case ECMA_DATE_PROTOTYPE_SET_UTC_FULL_YEAR:
      {
        year = converted_number[0];
        if (conversions >= 2)
        {
          month = converted_number[1];
        }
        if (conversions >= 3)
        {
          day = converted_number[2];
        }
        break;
      }
#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN
      case ECMA_DATE_PROTOTYPE_SET_YEAR:
      {
        year = converted_number[0];
        if (year >= 0 && year <= 99)
        {
          year += 1900;
        }
        break;
      }
#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */
      case ECMA_DATE_PROTOTYPE_SET_MONTH:
      case ECMA_DATE_PROTOTYPE_SET_UTC_MONTH:
      {
        month = converted_number[0];
        if (conversions >= 2)
        {
          day = converted_number[1];
        }
        break;
      }
      default:
      {
        JERRY_ASSERT (builtin_routine_id == ECMA_DATE_PROTOTYPE_SET_DATE
                      || builtin_routine_id == ECMA_DATE_PROTOTYPE_SET_UTC_DATE);
        day = converted_number[0];
        break;
      }
    }

    day_part = ecma_date_make_day (year, month, day);

#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN
    if (builtin_routine_id == ECMA_DATE_PROTOTYPE_SET_YEAR)
    {
      builtin_routine_id = ECMA_DATE_PROTOTYPE_SET_UTC_YEAR;

      if (ecma_number_is_nan (converted_number[0]))
      {
        day_part = 0;
        time_part = converted_number[0];
      }
    }
#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */
  }
  else
  {
    if (ecma_number_is_nan (date_num))
    {
      return ecma_make_number_value (date_num);
    }

    day_part = ecma_date_day (date_num);

    ecma_number_t hour = ecma_date_hour_from_time (date_num);
    ecma_number_t min = ecma_date_min_from_time (date_num);
    ecma_number_t sec = ecma_date_sec_from_time (date_num);
    ecma_number_t ms = ecma_date_ms_from_time (date_num);

    switch (builtin_routine_id)
    {
      case ECMA_DATE_PROTOTYPE_SET_HOURS:
      case ECMA_DATE_PROTOTYPE_SET_UTC_HOURS:
      {
        hour = converted_number[0];
        if (conversions >= 2)
        {
          min = converted_number[1];
        }
        if (conversions >= 3)
        {
          sec = converted_number[2];
        }
        if (conversions >= 4)
        {
          ms = converted_number[3];
        }
        break;
      }
      case ECMA_DATE_PROTOTYPE_SET_MINUTES:
      case ECMA_DATE_PROTOTYPE_SET_UTC_MINUTES:
      {
        min = converted_number[0];
        if (conversions >= 2)
        {
          sec = converted_number[1];
        }
        if (conversions >= 3)
        {
          ms = converted_number[2];
        }
        break;
      }
      case ECMA_DATE_PROTOTYPE_SET_UTC_SECONDS:
      case ECMA_DATE_PROTOTYPE_SET_SECONDS:
      {
        sec = converted_number[0];
        if (conversions >= 2)
        {
          ms = converted_number[1];
        }
        break;
      }
      default:
      {
        JERRY_ASSERT (builtin_routine_id == ECMA_DATE_PROTOTYPE_SET_UTC_MILLISECONDS
                      || builtin_routine_id == ECMA_DATE_PROTOTYPE_SET_MILLISECONDS);
        ms = converted_number[0];
        break;
      }
    }

    time_part = ecma_date_make_time (hour, min, sec, ms);
  }

  bool is_utc = BUILTIN_DATE_FUNCTION_IS_UTC (builtin_routine_id);

  ecma_number_t full_date = ecma_date_make_date (day_part, time_part);

  if (!is_utc)
  {
    full_date = ecma_date_utc (full_date);
  }

  full_date = ecma_date_time_clip (full_date);

  *ECMA_GET_INTERNAL_VALUE_POINTER (ecma_number_t, ext_object_p->u.class_prop.u.value) = full_date;

  return ecma_make_number_value (full_date);
} /* ecma_builtin_date_prototype_dispatch_set */

#undef ECMA_DATE_PROTOTYPE_IS_SET_YEAR_ROUTINE

/**
 * Dispatcher of the built-in's routines
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_date_prototype_dispatch_routine (uint16_t builtin_routine_id, /**< built-in wide routine
                                                                            *   identifier */
                                              ecma_value_t this_arg, /**< 'this' argument value */
                                              const ecma_value_t arguments_list[], /**< list of arguments
                                                                                    *   passed to routine */
                                              ecma_length_t arguments_number) /**< length of arguments' list */
{
  if (unlikely (builtin_routine_id == ECMA_DATE_PROTOTYPE_TO_JSON))
  {
    return ecma_builtin_date_prototype_to_json (this_arg);
  }

  if (!ecma_is_value_object (this_arg)
      || !ecma_object_class_is (ecma_get_object_from_value (this_arg), LIT_MAGIC_STRING_DATE_UL))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Date object expected"));
  }

  ecma_object_t *object_p = ecma_get_object_from_value (this_arg);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ecma_number_t *prim_value_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_number_t,
                                                                 ext_object_p->u.class_prop.u.value);

  if (builtin_routine_id == ECMA_DATE_PROTOTYPE_GET_TIME)
  {
    return ecma_make_number_value (*prim_value_p);
  }

  if (builtin_routine_id == ECMA_DATE_PROTOTYPE_SET_TIME)
  {
    ecma_value_t time = (arguments_number >= 1 ? arguments_list[0]
                                               : ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED));

    ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

    /* 1. */
    ECMA_OP_TO_NUMBER_TRY_CATCH (time_num, time, ret_value);
    *prim_value_p = ecma_date_time_clip (time_num);

    ret_value = ecma_make_number_value (time_num);
    ECMA_OP_TO_NUMBER_FINALIZE (time_num);

    return ret_value;
  }

  if (builtin_routine_id <= ECMA_DATE_PROTOTYPE_SET_UTC_MILLISECONDS)
  {
    ecma_number_t this_num = *prim_value_p;

    if (!BUILTIN_DATE_FUNCTION_IS_UTC (builtin_routine_id))
    {
      this_num += ecma_date_local_time_zone (this_num);
    }

    if (builtin_routine_id <= ECMA_DATE_PROTOTYPE_GET_UTC_TIMEZONE_OFFSET)
    {
      return ecma_builtin_date_prototype_dispatch_get (builtin_routine_id, this_num);
    }

    return ecma_builtin_date_prototype_dispatch_set (builtin_routine_id,
                                                     ext_object_p,
                                                     this_num,
                                                     arguments_list,
                                                     arguments_number);
  }

  if (builtin_routine_id == ECMA_DATE_PROTOTYPE_TO_ISO_STRING)
  {
    if (ecma_number_is_nan (*prim_value_p) || ecma_number_is_infinity (*prim_value_p))
    {
      return ecma_raise_range_error (ECMA_ERR_MSG ("Date must be a finite number."));
    }

    return ecma_date_value_to_iso_string (*prim_value_p);
  }

  if (ecma_number_is_nan (*prim_value_p))
  {
    ecma_string_t *magic_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INVALID_DATE_UL);
    return ecma_make_string_value (magic_str_p);
  }

  switch (builtin_routine_id)
  {
    case ECMA_DATE_PROTOTYPE_TO_STRING:
    {
      return ecma_date_value_to_string (*prim_value_p);
    }
    case ECMA_DATE_PROTOTYPE_TO_DATE_STRING:
    {
      return ecma_date_value_to_date_string (*prim_value_p);
    }
    case ECMA_DATE_PROTOTYPE_TO_TIME_STRING:
    {
      return ecma_date_value_to_time_string (*prim_value_p);
    }
    default:
    {
      JERRY_ASSERT (builtin_routine_id == ECMA_DATE_PROTOTYPE_TO_UTC_STRING);
      return ecma_date_value_to_utc_string (*prim_value_p);
    }
  }
} /* ecma_builtin_date_prototype_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_DATE_BUILTIN */

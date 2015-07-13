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

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

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
 * Insert leading zeros to a string of a number if needed.
 */
static void
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
static void
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
 * The Date.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_to_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (ecma_object_get_class_name (ecma_get_object_from_value (this_arg)) != LIT_MAGIC_STRING_DATE_UL)
  {
    ret_value = ecma_raise_type_error ("Incomplete Date type");
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
      ecma_number_t milliseconds = ecma_date_ms_from_time (*prim_value_num_p);
      ecma_string_t *output_str_p = ecma_new_ecma_string_from_number (milliseconds);
      ecma_date_insert_leading_zeros (&output_str_p, milliseconds, 3);

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
} /* ecma_builtin_date_prototype_to_string */

/**
 * The Date.prototype object's 'toDateString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_to_date_string (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_to_date_string */

/**
 * The Date.prototype object's 'toTimeString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_to_time_string (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_to_time_string */

/**
 * The Date.prototype object's 'toLocaleString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_to_locale_string (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_to_locale_string */

/**
 * The Date.prototype object's 'toLocaleDateString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_to_locale_date_string (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_to_locale_date_string */

/**
 * The Date.prototype object's 'toLocaleTimeString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.7
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_to_locale_time_string (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_to_locale_time_string */

/**
 * The Date.prototype object's 'valueOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.8
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_value_of (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_date_prototype_get_time (this_arg);
} /* ecma_builtin_date_prototype_value_of */

/**
 * The Date.prototype object's 'getTime' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.9
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_time (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_value_object (this_arg))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);
    if (ecma_object_get_class_name (obj_p) == LIT_MAGIC_STRING_DATE_UL)
    {
      ecma_property_t *prim_value_prop_p = ecma_get_internal_property (obj_p,
                                                                       ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE);

      ecma_number_t *prim_value_num_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                                                   prim_value_prop_p->u.internal_property.value);

      ecma_number_t *ret_num_p = ecma_alloc_number ();
      *ret_num_p = *prim_value_num_p;

      return ecma_make_normal_completion_value (ecma_make_number_value (ret_num_p));
    }
  }

  return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
} /* ecma_builtin_date_prototype_get_time */

/**
 * Helper macro, define the getter function argument to use
 * UTC time, passing the argument as is.
 */
#define DEFINE_GETTER_ARGUMENT_utc(this_num) (this_num)

/**
 * Helper macro, define the getter function argument to use
 * local time, passing the argument to 'ecma_date_local_time' function.
 */
#define DEFINE_GETTER_ARGUMENT_local(this_num) (ecma_date_local_time (this_num))

/**
 * Implementation of Data.prototype built-in's getter routines
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
#define DEFINE_GETTER(_ecma_paragraph, _routine_name, _getter_name, _timezone) \
static ecma_completion_value_t \
ecma_builtin_date_prototype_get_ ## _routine_name (ecma_value_t this_arg) /**< this argument */ \
{ \
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value (); \
 \
  /* 1. */ \
  ECMA_TRY_CATCH (value, ecma_builtin_date_prototype_get_time (this_arg), ret_value); \
  ecma_number_t *this_num_p = ecma_get_number_from_value (value); \
  /* 2. */ \
  if (ecma_number_is_nan (*this_num_p)) \
  { \
    ecma_string_t *nan_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_NAN); \
    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (nan_str_p)); \
  } \
  else \
  { \
    /* 3. */ \
    ecma_number_t *ret_num_p = ecma_alloc_number (); \
    *ret_num_p = _getter_name (DEFINE_GETTER_ARGUMENT_ ## _timezone (*this_num_p)); \
    ret_value = ecma_make_normal_completion_value (ecma_make_number_value (ret_num_p)); \
  } \
  ECMA_FINALIZE (value); \
  \
  return ret_value; \
}

DEFINE_GETTER (ECMA-262 v5 15.9.5.10, full_year, ecma_date_year_from_time, local)
DEFINE_GETTER (ECMA-262 v5 15.9.5.11, utc_full_year, ecma_date_year_from_time, utc)
DEFINE_GETTER (ECMA-262 v5 15.9.5.12, month, ecma_date_month_from_time, local)
DEFINE_GETTER (ECMA-262 v5 15.9.5.13, utc_month, ecma_date_month_from_time, utc)
DEFINE_GETTER (ECMA-262 v5 15.9.5.14, date, ecma_date_date_from_time, local)
DEFINE_GETTER (ECMA-262 v5 15.9.5.15, utc_date, ecma_date_date_from_time, utc)
DEFINE_GETTER (ECMA-262 v5 15.9.5.16, day, ecma_date_week_day, local)
DEFINE_GETTER (ECMA-262 v5 15.9.5.17, utc_day, ecma_date_week_day, utc)
DEFINE_GETTER (ECMA-262 v5 15.9.5.18, hours, ecma_date_hour_from_time, local)
DEFINE_GETTER (ECMA-262 v5 15.9.5.19, utc_hours, ecma_date_hour_from_time, utc)
DEFINE_GETTER (ECMA-262 v5 15.9.5.20, minutes, ecma_date_min_from_time, local)
DEFINE_GETTER (ECMA-262 v5 15.9.5.21, utc_minutes, ecma_date_min_from_time, utc)
DEFINE_GETTER (ECMA-262 v5 15.9.5.22, seconds, ecma_date_sec_from_time, local)
DEFINE_GETTER (ECMA-262 v5 15.9.5.23, utc_seconds, ecma_date_sec_from_time, utc)
DEFINE_GETTER (ECMA-262 v5 15.9.5.24, milliseconds, ecma_date_ms_from_time , local)
DEFINE_GETTER (ECMA-262 v5 15.9.5.25, utc_milliseconds, ecma_date_ms_from_time , utc)
DEFINE_GETTER (ECMA-262 v5 15.9.5.26, timezone_offset, ecma_date_timezone_offset, utc)

#undef DEFINE_GETTER_ARGUMENT_utc
#undef DEFINE_GETTER_ARGUMENT_local
#undef DEFINE_GETTER

/**
 * The Date.prototype object's 'setTime' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.27
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_time (ecma_value_t this_arg, /**< this argument */
                                      ecma_value_t arg) /**< time */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_date_prototype_set_time */

/**
 * The Date.prototype object's 'setMilliseconds' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.28
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_milliseconds (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t arg) /**< millisecond */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_date_prototype_set_milliseconds */

/**
 * The Date.prototype object's 'setUTCMilliseconds' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.29
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_utc_milliseconds (ecma_value_t this_arg, /**< this argument */
                                                  ecma_value_t arg) /**< millisecond */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_date_prototype_set_utc_milliseconds */

/**
 * The Date.prototype object's 'setSeconds' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.30
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_seconds (ecma_value_t this_arg, /**< this argument */
                                         ecma_value_t arg1, /**< second */
                                         ecma_value_t arg2) /**< millisecond */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg1, arg2);
} /* ecma_builtin_date_prototype_set_seconds */

/**
 * The Date.prototype object's 'setUTCSeconds' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.31
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_utc_seconds (ecma_value_t this_arg, /**< this argument */
                                             ecma_value_t arg1, /**< second */
                                             ecma_value_t arg2) /**< millisecond */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg1, arg2);
} /* ecma_builtin_date_prototype_set_utc_seconds */

/**
 * The Date.prototype object's 'setMinutes' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.32
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_minutes (ecma_value_t this_arg, /**< this argument */
                                         const ecma_value_t args[], /**< arguments list */
                                         ecma_length_t args_number) /**< number of arguments */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, args, args_number);
} /* ecma_builtin_date_prototype_set_minutes */

/**
 * The Date.prototype object's 'setUTCMinutes' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.33
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_utc_minutes (ecma_value_t this_arg, /**< this argument */
                                             const ecma_value_t args[], /**< arguments list */
                                             ecma_length_t args_number) /**< number of arguments */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, args, args_number);
} /* ecma_builtin_date_prototype_set_utc_minutes */

/**
 * The Date.prototype object's 'setHours' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.34
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_hours (ecma_value_t this_arg, /**< this argument */
                                       const ecma_value_t args[], /**< arguments list */
                                       ecma_length_t args_number) /**< number of arguments */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, args, args_number);
} /* ecma_builtin_date_prototype_set_hours */

/**
 * The Date.prototype object's 'setUTCHours' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.35
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_utc_hours (ecma_value_t this_arg, /**< this argument */
                                           const ecma_value_t args[], /**< arguments list */
                                           ecma_length_t args_number) /**< number of arguments */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, args, args_number);
} /* ecma_builtin_date_prototype_set_utc_hours */

/**
 * The Date.prototype object's 'setDate' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.36
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_date (ecma_value_t this_arg, /**< this argument */
                                      ecma_value_t arg) /**< date */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_date_prototype_set_date */

/**
 * The Date.prototype object's 'setUTCDate' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.37
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_utc_date (ecma_value_t this_arg, /**< this argument */
                                          ecma_value_t arg) /**< date */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_date_prototype_set_utc_date */

/**
 * The Date.prototype object's 'setMonth' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.38
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_month (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t arg1, /**< month */
                                       ecma_value_t arg2) /**< day */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg1, arg2);
} /* ecma_builtin_date_prototype_set_month */

/**
 * The Date.prototype object's 'setUTCMonth' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.39
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_utc_month (ecma_value_t this_arg, /**< this argument */
                                           ecma_value_t arg1, /**< month */
                                           ecma_value_t arg2) /**< day */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg1, arg2);
} /* ecma_builtin_date_prototype_set_utc_month */

/**
 * The Date.prototype object's 'setFullYear' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.40
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_full_year (ecma_value_t this_arg, /**< this argument */
                                           const ecma_value_t args[], /**< arguments list */
                                           ecma_length_t args_number) /**< number of arguments */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, args, args_number);
} /* ecma_builtin_date_prototype_set_full_year */

/**
 * The Date.prototype object's 'setUTCFullYear' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.41
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_set_utc_full_year (ecma_value_t this_arg, /**< this argument */
                                               const ecma_value_t args[], /**< arguments list */
                                               ecma_length_t args_number) /**< number of arguments */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, args, args_number);
} /* ecma_builtin_date_prototype_set_utc_full_year */

/**
 * The Date.prototype object's 'toUTCString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.42
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_to_utc_string (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_to_utc_string */

/**
 * The Date.prototype object's 'toISOString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.43
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_to_iso_string (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_to_iso_string */

/**
 * The Date.prototype object's 'toJSON' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.44
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_to_json (ecma_value_t this_arg, /**< this argument */
                                     ecma_value_t arg) /**< key */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_date_prototype_to_json */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN */


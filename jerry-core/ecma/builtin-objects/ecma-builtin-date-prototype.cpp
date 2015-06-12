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
#include "ecma-globals.h"
#include "ecma-helpers.h"

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
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
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
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
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
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_time */

/**
 * The Date.prototype object's 'getFullYear' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.10
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_full_year (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_full_year */

/**
 * The Date.prototype object's 'getUTCFullYear' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.11
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_utc_full_year (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_utc_full_year */

/**
 * The Date.prototype object's 'getMonth' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.12
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_month (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_month */

/**
 * The Date.prototype object's 'getUTCMonth' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.13
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_utc_month (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_utc_month */

/**
 * The Date.prototype object's 'getDate' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.14
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_date (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_date */

/**
 * The Date.prototype object's 'getUTCDate' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.15
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_utc_date (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_utc_date */

/**
 * The Date.prototype object's 'getDay' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.16
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_day (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_day */

/**
 * The Date.prototype object's 'getUTCDay' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.17
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_utc_day (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_utc_day */

/**
 * The Date.prototype object's 'getHours' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.18
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_hours (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_hours */

/**
 * The Date.prototype object's 'getUTCHours' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.19
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_utc_hours (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_utc_hours */

/**
 * The Date.prototype object's 'getMinutes' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.20
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_minutes (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_minutes */

/**
 * The Date.prototype object's 'getUTCMinutes' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.21
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_utc_minutes (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_utc_minutes */

/**
 * The Date.prototype object's 'getSeconds' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.22
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_seconds (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_seconds */

/**
 * The Date.prototype object's 'getUTCSeconds' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.23
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_utc_seconds (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_utc_seconds */

/**
 * The Date.prototype object's 'getMilliseconds' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.24
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_milliseconds (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_milliseconds */

/**
 * The Date.prototype object's 'getUTCMilliseconds' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.25
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_utc_milliseconds (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_utc_milliseconds */

/**
 * The Date.prototype object's 'getTimezoneOffset' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.26
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_prototype_get_timezone_offset (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_prototype_get_timezone_offset */

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


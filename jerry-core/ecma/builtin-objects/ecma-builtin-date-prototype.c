/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged.
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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_to_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (prim_value,
                  ecma_date_get_primitive_value (this_arg),
                  ret_value);

  ecma_number_t prim_num = ecma_get_number_from_value (prim_value);

  if (ecma_number_is_nan (prim_num))
  {
    ecma_string_t *magic_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INVALID_DATE_UL);
    ret_value = ecma_make_string_value (magic_str_p);
  }
  else
  {
    ret_value = ecma_date_value_to_string (prim_num);
  }

  ECMA_FINALIZE (prim_value);

  return ret_value;
} /* ecma_builtin_date_prototype_to_string */

/**
 * The Date.prototype object's 'toDateString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_to_date_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (!ecma_is_value_object (this_arg)
      || ecma_object_get_class_name (ecma_get_object_from_value (this_arg)) != LIT_MAGIC_STRING_DATE_UL)
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Incompatible type"));
  }
  else
  {
    ECMA_TRY_CATCH (obj_this,
                    ecma_op_to_object (this_arg),
                    ret_value);

    ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
    ecma_value_t *date_prop_p = ecma_get_internal_property (obj_p,
                                                            ECMA_INTERNAL_PROPERTY_DATE_FLOAT);
    ecma_number_t *date_num_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_number_t, *date_prop_p);

    if (ecma_number_is_nan (*date_num_p))
    {
      ecma_string_t *magic_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INVALID_DATE_UL);
      ret_value = ecma_make_string_value (magic_str_p);
    }
    else
    {
      ret_value = ecma_date_value_to_date_string (*date_num_p);
    }

    ECMA_FINALIZE (obj_this);
  }

  return ret_value;
} /* ecma_builtin_date_prototype_to_date_string */

/**
 * The Date.prototype object's 'toTimeString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_to_time_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (!ecma_is_value_object (this_arg)
      || ecma_object_get_class_name (ecma_get_object_from_value (this_arg)) != LIT_MAGIC_STRING_DATE_UL)
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Incompatible type"));
  }
  else
  {
    ECMA_TRY_CATCH (obj_this,
                    ecma_op_to_object (this_arg),
                    ret_value);

    ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
    ecma_value_t *prim_prop_p = ecma_get_internal_property (obj_p,
                                                            ECMA_INTERNAL_PROPERTY_DATE_FLOAT);
    ecma_number_t *prim_value_num_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_number_t, *prim_prop_p);

    if (ecma_number_is_nan (*prim_value_num_p))
    {
      ecma_string_t *magic_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INVALID_DATE_UL);
      ret_value = ecma_make_string_value (magic_str_p);
    }
    else
    {
      ret_value = ecma_date_value_to_time_string (*prim_value_num_p);
    }

    ECMA_FINALIZE (obj_this);
  }

  return ret_value;
} /* ecma_builtin_date_prototype_to_time_string */

/**
 * The Date.prototype object's 'toLocaleString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_to_locale_string (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_date_prototype_to_string (this_arg);
} /* ecma_builtin_date_prototype_to_locale_string */

/**
 * The Date.prototype object's 'toLocaleDateString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_to_locale_date_string (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_date_prototype_to_date_string (this_arg);
} /* ecma_builtin_date_prototype_to_locale_date_string */

/**
 * The Date.prototype object's 'toLocaleTimeString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_to_locale_time_string (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_date_prototype_to_time_string (this_arg);
} /* ecma_builtin_date_prototype_to_locale_time_string */

/**
 * The Date.prototype object's 'valueOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_get_time (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_value_object (this_arg))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);
    if (ecma_object_get_class_name (obj_p) == LIT_MAGIC_STRING_DATE_UL)
    {
      ecma_value_t *date_prop_p = ecma_get_internal_property (obj_p,
                                                              ECMA_INTERNAL_PROPERTY_DATE_FLOAT);

      ecma_number_t *date_num_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_number_t, *date_prop_p);
      return ecma_make_number_value (*date_num_p);
    }
  }

  return ecma_raise_type_error (ECMA_ERR_MSG (""));
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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
#define DEFINE_GETTER(_ecma_paragraph, _routine_name, _getter_name, _timezone) \
static ecma_value_t \
ecma_builtin_date_prototype_get_ ## _routine_name (ecma_value_t this_arg) /**< this argument */ \
{ \
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY); \
 \
  /* 1. */ \
  ECMA_TRY_CATCH (value, ecma_builtin_date_prototype_get_time (this_arg), ret_value); \
  ecma_number_t this_num = ecma_get_number_from_value (value); \
  /* 2. */ \
  if (ecma_number_is_nan (this_num)) \
  { \
    ecma_string_t *nan_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_NAN); \
    ret_value = ecma_make_string_value (nan_str_p); \
  } \
  else \
  { \
    /* 3. */ \
    ecma_number_t ret_num = _getter_name (DEFINE_GETTER_ARGUMENT_ ## _timezone (this_num)); \
    ret_value = ecma_make_number_value (ret_num); \
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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_time (ecma_value_t this_arg, /**< this argument */
                                      ecma_value_t time) /**< time */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (!ecma_is_value_object (this_arg)
      || ecma_object_get_class_name (ecma_get_object_from_value (this_arg)) != LIT_MAGIC_STRING_DATE_UL)
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Incompatible type"));
  }
  else
  {
    /* 1. */
    ECMA_OP_TO_NUMBER_TRY_CATCH (t, time, ret_value);
    ecma_number_t value = ecma_date_time_clip (t);

    /* 2. */
    ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);

    ecma_value_t *date_prop_p = ecma_get_internal_property (obj_p,
                                                            ECMA_INTERNAL_PROPERTY_DATE_FLOAT);

    ecma_number_t *date_num_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_number_t, *date_prop_p);
    *date_num_p = value;

    /* 3. */
    ret_value = ecma_make_number_value (value);
    ECMA_OP_TO_NUMBER_FINALIZE (t);
  }

  return ret_value;
} /* ecma_builtin_date_prototype_set_time */

/**
 * The Date.prototype object's 'setMilliseconds' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.28
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_milliseconds (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t ms) /**< millisecond */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_date_local_time (ecma_get_number_from_value (this_time_value));

  /* 2. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (milli, ms, ret_value);

  /* 3-5. */
  ecma_number_t hour = ecma_date_hour_from_time (t);
  ecma_number_t min = ecma_date_min_from_time (t);
  ecma_number_t sec = ecma_date_sec_from_time (t);
  ret_value = ecma_date_set_internal_property (this_arg,
                                               ecma_date_day (t),
                                               ecma_date_make_time (hour, min, sec, milli),
                                               ECMA_DATE_LOCAL);

  ECMA_OP_TO_NUMBER_FINALIZE (milli);
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_milliseconds */

/**
 * The Date.prototype object's 'setUTCMilliseconds' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.29
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_utc_milliseconds (ecma_value_t this_arg, /**< this argument */
                                                  ecma_value_t ms) /**< millisecond */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_get_number_from_value (this_time_value);

  /* 2. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (milli, ms, ret_value);

  /* 3-5. */
  ecma_number_t hour = ecma_date_hour_from_time (t);
  ecma_number_t min = ecma_date_min_from_time (t);
  ecma_number_t sec = ecma_date_sec_from_time (t);
  ret_value = ecma_date_set_internal_property (this_arg,
                                               ecma_date_day (t),
                                               ecma_date_make_time (hour, min, sec, milli),
                                               ECMA_DATE_UTC);

  ECMA_OP_TO_NUMBER_FINALIZE (milli);
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_utc_milliseconds */

/**
 * The Date.prototype object's 'setSeconds' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.30
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_seconds (ecma_value_t this_arg, /**< this argument */
                                         ecma_value_t sec, /**< second */
                                         ecma_value_t ms) /**< millisecond */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_date_local_time (ecma_get_number_from_value (this_time_value));

  /* 2. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (s, sec, ret_value);

  /* 3. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (milli, ms, ret_value);
  if (ecma_is_value_undefined (ms))
  {
    milli = ecma_date_ms_from_time (t);
  }

  /* 4-7. */
  ecma_number_t hour = ecma_date_hour_from_time (t);
  ecma_number_t min = ecma_date_min_from_time (t);
  ret_value = ecma_date_set_internal_property (this_arg,
                                               ecma_date_day (t),
                                               ecma_date_make_time (hour, min, s, milli),
                                               ECMA_DATE_LOCAL);

  ECMA_OP_TO_NUMBER_FINALIZE (milli);
  ECMA_OP_TO_NUMBER_FINALIZE (s);
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_seconds */

/**
 * The Date.prototype object's 'setUTCSeconds' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.31
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_utc_seconds (ecma_value_t this_arg, /**< this argument */
                                             ecma_value_t sec, /**< second */
                                             ecma_value_t ms) /**< millisecond */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_get_number_from_value (this_time_value);

  /* 2. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (s, sec, ret_value);

  /* 3. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (milli, ms, ret_value);
  if (ecma_is_value_undefined (ms))
  {
    milli = ecma_date_ms_from_time (t);
  }

  /* 4-7. */
  ecma_number_t hour = ecma_date_hour_from_time (t);
  ecma_number_t min = ecma_date_min_from_time (t);
  ret_value = ecma_date_set_internal_property (this_arg,
                                               ecma_date_day (t),
                                               ecma_date_make_time (hour, min, s, milli),
                                               ECMA_DATE_UTC);

  ECMA_OP_TO_NUMBER_FINALIZE (milli);
  ECMA_OP_TO_NUMBER_FINALIZE (s);
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_utc_seconds */

/**
 * The Date.prototype object's 'setMinutes' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.32
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_minutes (ecma_value_t this_arg, /**< this argument */
                                         const ecma_value_t args[], /**< arguments list */
                                         ecma_length_t args_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_date_local_time (ecma_get_number_from_value (this_time_value));

  /* 2. */
  ecma_number_t m = ecma_number_make_nan ();
  ecma_number_t s = ecma_date_sec_from_time (t);
  ecma_number_t milli = ecma_date_ms_from_time (t);
  if (args_number > 0 && !ecma_is_value_undefined (args[0]))
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (min, args[0], ret_value);
    m = min;

    /* 3. */
    if (args_number > 1 && !ecma_is_value_undefined (args[1]))
    {
      ECMA_OP_TO_NUMBER_TRY_CATCH (sec, args[1], ret_value);
      s = sec;

      /* 4. */
      if (args_number > 2 && !ecma_is_value_undefined (args[2]))
      {
        ECMA_OP_TO_NUMBER_TRY_CATCH (ms, args[2], ret_value);
        milli = ms;
        ECMA_OP_TO_NUMBER_FINALIZE (ms);
      }
      ECMA_OP_TO_NUMBER_FINALIZE (sec);
    }
    ECMA_OP_TO_NUMBER_FINALIZE (min);
  }

  if (ecma_is_value_empty (ret_value))
  {
    /* 5-8. */
    ecma_number_t hour = ecma_date_hour_from_time (t);
    ret_value = ecma_date_set_internal_property (this_arg,
                                                 ecma_date_day (t),
                                                 ecma_date_make_time (hour, m, s, milli),
                                                 ECMA_DATE_LOCAL);
  }
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_minutes */

/**
 * The Date.prototype object's 'setUTCMinutes' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.33
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_utc_minutes (ecma_value_t this_arg, /**< this argument */
                                             const ecma_value_t args[], /**< arguments list */
                                             ecma_length_t args_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_get_number_from_value (this_time_value);

  /* 2. */
  ecma_number_t m = ecma_number_make_nan ();
  ecma_number_t s = ecma_date_sec_from_time (t);
  ecma_number_t milli = ecma_date_ms_from_time (t);
  if (args_number > 0 && !ecma_is_value_undefined (args[0]))
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (min, args[0], ret_value);
    m = min;

    /* 3. */
    if (args_number > 1 && !ecma_is_value_undefined (args[1]))
    {
      ECMA_OP_TO_NUMBER_TRY_CATCH (sec, args[1], ret_value);
      s = sec;

      /* 4. */
      if (args_number > 2 && !ecma_is_value_undefined (args[2]))
      {
        ECMA_OP_TO_NUMBER_TRY_CATCH (ms, args[2], ret_value);
        milli = ms;
        ECMA_OP_TO_NUMBER_FINALIZE (ms);
      }
      ECMA_OP_TO_NUMBER_FINALIZE (sec);
    }
    ECMA_OP_TO_NUMBER_FINALIZE (min);
  }

  if (ecma_is_value_empty (ret_value))
  {
    /* 5-8. */
    ecma_number_t hour = ecma_date_hour_from_time (t);
    ret_value = ecma_date_set_internal_property (this_arg,
                                                 ecma_date_day (t),
                                                 ecma_date_make_time (hour, m, s, milli),
                                                 ECMA_DATE_UTC);
  }
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_utc_minutes */

/**
 * The Date.prototype object's 'setHours' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.34
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_hours (ecma_value_t this_arg, /**< this argument */
                                       const ecma_value_t args[], /**< arguments list */
                                       ecma_length_t args_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_date_local_time (ecma_get_number_from_value (this_time_value));

  /* 2. */
  ecma_number_t h = ecma_number_make_nan ();
  ecma_number_t m = ecma_date_min_from_time (t);
  ecma_number_t s = ecma_date_sec_from_time (t);
  ecma_number_t milli = ecma_date_ms_from_time (t);
  if (args_number > 0 && !ecma_is_value_undefined (args[0]))
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (hour, args[0], ret_value);
    h = hour;

    /* 3. */
    if (args_number > 1 && !ecma_is_value_undefined (args[1]))
    {
      ECMA_OP_TO_NUMBER_TRY_CATCH (min, args[1], ret_value);
      m = min;

      /* 4. */
      if (args_number > 2 && !ecma_is_value_undefined (args[2]))
      {
        ECMA_OP_TO_NUMBER_TRY_CATCH (sec, args[2], ret_value);
        s = sec;

        /* 5. */
        if (args_number > 3 && !ecma_is_value_undefined (args[3]))
        {
          ECMA_OP_TO_NUMBER_TRY_CATCH (ms, args[3], ret_value);
          milli = ms;
          ECMA_OP_TO_NUMBER_FINALIZE (ms);
        }
        ECMA_OP_TO_NUMBER_FINALIZE (sec);
      }
      ECMA_OP_TO_NUMBER_FINALIZE (min);
    }
    ECMA_OP_TO_NUMBER_FINALIZE (hour);
  }

  if (ecma_is_value_empty (ret_value))
  {
    /* 6-9. */
    ret_value = ecma_date_set_internal_property (this_arg,
                                                 ecma_date_day (t),
                                                 ecma_date_make_time (h, m, s, milli),
                                                 ECMA_DATE_LOCAL);
  }
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_hours */

/**
 * The Date.prototype object's 'setUTCHours' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.35
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_utc_hours (ecma_value_t this_arg, /**< this argument */
                                           const ecma_value_t args[], /**< arguments list */
                                           ecma_length_t args_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_get_number_from_value (this_time_value);

  /* 2. */
  ecma_number_t h = ecma_number_make_nan ();
  ecma_number_t m = ecma_date_min_from_time (t);
  ecma_number_t s = ecma_date_sec_from_time (t);
  ecma_number_t milli = ecma_date_ms_from_time (t);
  if (args_number > 0 && !ecma_is_value_undefined (args[0]))
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (hour, args[0], ret_value);
    h = hour;

    /* 3. */
    if (args_number > 1 && !ecma_is_value_undefined (args[1]))
    {
      ECMA_OP_TO_NUMBER_TRY_CATCH (min, args[1], ret_value);
      m = min;

      /* 4. */
      if (args_number > 2 && !ecma_is_value_undefined (args[2]))
      {
        ECMA_OP_TO_NUMBER_TRY_CATCH (sec, args[2], ret_value);
        s = sec;

        /* 5. */
        if (args_number > 3 && !ecma_is_value_undefined (args[3]))
        {
          ECMA_OP_TO_NUMBER_TRY_CATCH (ms, args[3], ret_value);
          milli = ms;
          ECMA_OP_TO_NUMBER_FINALIZE (ms);
        }
        ECMA_OP_TO_NUMBER_FINALIZE (sec);
      }
      ECMA_OP_TO_NUMBER_FINALIZE (min);
    }
    ECMA_OP_TO_NUMBER_FINALIZE (hour);
  }

  if (ecma_is_value_empty (ret_value))
  {
    /* 6-9. */
    ret_value = ecma_date_set_internal_property (this_arg,
                                                 ecma_date_day (t),
                                                 ecma_date_make_time (h, m, s, milli),
                                                 ECMA_DATE_UTC);
  }
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_utc_hours */

/**
 * The Date.prototype object's 'setDate' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.36
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_date (ecma_value_t this_arg, /**< this argument */
                                      ecma_value_t date) /**< date */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_date_local_time (ecma_get_number_from_value (this_time_value));

  /* 2. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (dt, date, ret_value);

  /* 3-6. */
  ecma_number_t year = ecma_date_year_from_time (t);
  ecma_number_t month = ecma_date_month_from_time (t);
  ret_value = ecma_date_set_internal_property (this_arg,
                                               ecma_date_make_day (year, month, dt),
                                               ecma_date_time_within_day (t),
                                               ECMA_DATE_LOCAL);

  ECMA_OP_TO_NUMBER_FINALIZE (dt);
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_date */

/**
 * The Date.prototype object's 'setUTCDate' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.37
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_utc_date (ecma_value_t this_arg, /**< this argument */
                                          ecma_value_t date) /**< date */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_get_number_from_value (this_time_value);

  /* 2. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (dt, date, ret_value);

  /* 3-6. */
  ecma_number_t year = ecma_date_year_from_time (t);
  ecma_number_t month = ecma_date_month_from_time (t);
  ret_value = ecma_date_set_internal_property (this_arg,
                                               ecma_date_make_day (year, month, dt),
                                               ecma_date_time_within_day (t),
                                               ECMA_DATE_UTC);

  ECMA_OP_TO_NUMBER_FINALIZE (dt);
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_utc_date */

/**
 * The Date.prototype object's 'setMonth' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.38
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_month (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t month, /**< month */
                                       ecma_value_t date) /**< date */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_date_local_time (ecma_get_number_from_value (this_time_value));

  /* 2. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (m, month, ret_value);

  /* 3. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (dt, date, ret_value);
  if (ecma_is_value_undefined (date))
  {
    dt = ecma_date_date_from_time (t);
  }

  /* 4-7. */
  ecma_number_t year = ecma_date_year_from_time (t);
  ret_value = ecma_date_set_internal_property (this_arg,
                                               ecma_date_make_day (year, m, dt),
                                               ecma_date_time_within_day (t),
                                               ECMA_DATE_LOCAL);

  ECMA_OP_TO_NUMBER_FINALIZE (dt);
  ECMA_OP_TO_NUMBER_FINALIZE (m);
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_month */

/**
 * The Date.prototype object's 'setUTCMonth' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.39
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_utc_month (ecma_value_t this_arg, /**< this argument */
                                           ecma_value_t month, /**< month */
                                           ecma_value_t date) /**< date */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_get_number_from_value (this_time_value);

  /* 2. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (m, month, ret_value);

  /* 3. */
  ECMA_OP_TO_NUMBER_TRY_CATCH (dt, date, ret_value);
  if (ecma_is_value_undefined (date))
  {
    dt = ecma_date_date_from_time (t);
  }

  /* 4-7. */
  ecma_number_t year = ecma_date_year_from_time (t);
  ret_value = ecma_date_set_internal_property (this_arg,
                                               ecma_date_make_day (year, m, dt),
                                               ecma_date_time_within_day (t),
                                               ECMA_DATE_UTC);

  ECMA_OP_TO_NUMBER_FINALIZE (dt);
  ECMA_OP_TO_NUMBER_FINALIZE (m);
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_utc_month */

/**
 * The Date.prototype object's 'setFullYear' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.40
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_full_year (ecma_value_t this_arg, /**< this argument */
                                           const ecma_value_t args[], /**< arguments list */
                                           ecma_length_t args_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_date_local_time (ecma_get_number_from_value (this_time_value));
  if (ecma_number_is_nan (t))
  {
    t = ECMA_NUMBER_ZERO;
  }

  /* 2. */
  ecma_number_t y = ecma_number_make_nan ();
  ecma_number_t m = ecma_date_month_from_time (t);
  ecma_number_t dt = ecma_date_date_from_time (t);
  if (args_number > 0 && !ecma_is_value_undefined (args[0]))
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (year, args[0], ret_value);
    y = year;

    /* 3. */
    if (args_number > 1 && !ecma_is_value_undefined (args[1]))
    {
      ECMA_OP_TO_NUMBER_TRY_CATCH (month, args[1], ret_value);
      m = month;

      /* 4. */
      if (args_number > 2 && !ecma_is_value_undefined (args[2]))
      {
        ECMA_OP_TO_NUMBER_TRY_CATCH (date, args[2], ret_value);
        dt = date;
        ECMA_OP_TO_NUMBER_FINALIZE (date);
      }
      ECMA_OP_TO_NUMBER_FINALIZE (month);
    }
    ECMA_OP_TO_NUMBER_FINALIZE (year);
  }

  if (ecma_is_value_empty (ret_value))
  {
    /* 5-8. */
    ret_value = ecma_date_set_internal_property (this_arg,
                                                 ecma_date_make_day (y, m, dt),
                                                 ecma_date_time_within_day (t),
                                                 ECMA_DATE_LOCAL);
  }
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_full_year */

/**
 * The Date.prototype object's 'setUTCFullYear' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.41
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_utc_full_year (ecma_value_t this_arg, /**< this argument */
                                               const ecma_value_t args[], /**< arguments list */
                                               ecma_length_t args_number) /**< number of arguments */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_get_number_from_value (this_time_value);
  if (ecma_number_is_nan (t))
  {
    t = ECMA_NUMBER_ZERO;
  }

  /* 2. */
  ecma_number_t y = ecma_number_make_nan ();
  ecma_number_t m = ecma_date_month_from_time (t);
  ecma_number_t dt = ecma_date_date_from_time (t);
  if (args_number > 0 && !ecma_is_value_undefined (args[0]))
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (year, args[0], ret_value);
    y = year;

    /* 3. */
    if (args_number > 1 && !ecma_is_value_undefined (args[1]))
    {
      ECMA_OP_TO_NUMBER_TRY_CATCH (month, args[1], ret_value);
      m = month;

      /* 4. */
      if (args_number > 2 && !ecma_is_value_undefined (args[2]))
      {
        ECMA_OP_TO_NUMBER_TRY_CATCH (date, args[2], ret_value);
        dt = date;
        ECMA_OP_TO_NUMBER_FINALIZE (date);
      }
      ECMA_OP_TO_NUMBER_FINALIZE (month);
    }
    ECMA_OP_TO_NUMBER_FINALIZE (year);
  }

  if (ecma_is_value_empty (ret_value))
  {
    /* 5-8. */
    ret_value = ecma_date_set_internal_property (this_arg,
                                                 ecma_date_make_day (y, m, dt),
                                                 ecma_date_time_within_day (t),
                                                 ECMA_DATE_UTC);
  }
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_utc_full_year */

/**
 * The Date.prototype object's 'toUTCString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.42
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_to_utc_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (prim_value,
                  ecma_date_get_primitive_value (this_arg),
                  ret_value);

  ecma_number_t prim_num = ecma_get_number_from_value (prim_value);

  if (ecma_number_is_nan (prim_num))
  {
    ecma_string_t *magic_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INVALID_DATE_UL);
    ret_value = ecma_make_string_value (magic_str_p);
  }
  else
  {
    ret_value = ecma_date_value_to_utc_string (prim_num);
  }

  ECMA_FINALIZE (prim_value);

  return ret_value;
} /* ecma_builtin_date_prototype_to_utc_string */

/**
 * The Date.prototype object's 'toISOString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.5.43
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_to_iso_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (prim_value,
                  ecma_date_get_primitive_value (this_arg),
                  ret_value);

  ecma_number_t prim_num = ecma_get_number_from_value (prim_value);

  if (ecma_number_is_nan (prim_num) || ecma_number_is_infinity (prim_num))
  {
    ret_value = ecma_raise_range_error (ECMA_ERR_MSG (""));
  }
  else
  {
    ret_value = ecma_date_value_to_iso_string (prim_num);
  }

  ECMA_FINALIZE (prim_value);

  return ret_value;
} /* ecma_builtin_date_prototype_to_iso_string */

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
ecma_builtin_date_prototype_to_json (ecma_value_t this_arg, /**< this argument */
                                     ecma_value_t arg) /**< key */
{
  JERRY_UNUSED (arg);
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
      ret_value = ecma_raise_type_error (ECMA_ERR_MSG (""));
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

#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN

/**
 * The Date.prototype object's 'getYear' routine
 *
 * See also:
 *          ECMA-262 v5, AnnexB.B.2.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_get_year (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t this_num = ecma_get_number_from_value (value);
  /* 2. */
  if (ecma_number_is_nan (this_num))
  {
    ecma_string_t *nan_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_NAN);
    ret_value = ecma_make_string_value (nan_str_p);
  }
  else
  {
    /* 3. */
    ecma_number_t ret_num = ecma_date_year_from_time (ecma_date_local_time (this_num)) - 1900;
    ret_value = ecma_make_number_value (ret_num);
  }
  ECMA_FINALIZE (value);

  return ret_value;
} /* ecma_builtin_date_prototype_get_year */

/**
 * The Date.prototype object's 'setYear' routine
 *
 * See also:
 *          ECMA-262 v5, AnnexB.B.2.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_date_prototype_set_year (ecma_value_t this_arg, /**< this argument */
                                      ecma_value_t year) /**< year argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (this_time_value, ecma_builtin_date_prototype_get_time (this_arg), ret_value);
  ecma_number_t t = ecma_date_local_time (ecma_get_number_from_value (this_time_value));
  if (ecma_number_is_nan (t))
  {
    t = ECMA_NUMBER_ZERO;
  }

  /* 2. */
  ecma_number_t y = ecma_number_make_nan ();

  ECMA_OP_TO_NUMBER_TRY_CATCH (year_value, year, ret_value);
  y = year_value;

  /* 3. */
  if (ecma_number_is_nan (y))
  {
    ret_value = ecma_date_set_internal_property (this_arg, 0, y, ECMA_DATE_UTC);
  }
  else
  {
    /* 4. */
    if (y >= 0 && y <= 99)
    {
      y += 1900;
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (year_value);

  if (ecma_is_value_empty (ret_value))
  {
    /* 5-8. */
    ecma_number_t m = ecma_date_month_from_time (t);
    ecma_number_t dt = ecma_date_date_from_time (t);
    ret_value = ecma_date_set_internal_property (this_arg,
                                                 ecma_date_make_day (y, m, dt),
                                                 ecma_date_time_within_day (t),
                                                 ECMA_DATE_UTC);
  }
  ECMA_FINALIZE (this_time_value);

  return ret_value;
} /* ecma_builtin_date_prototype_set_year */

#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_DATE_BUILTIN */


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

/*
 * Date.prototype built-in description
 */

#ifndef OBJECT_ID
# define OBJECT_ID(builtin_object_id)
#endif /* !OBJECT_ID */

#ifndef OBJECT_VALUE
# define OBJECT_VALUE(name, obj_builtin_id, prop_attributes)
#endif /* !OBJECT_VALUE */

#ifndef ROUTINE
# define ROUTINE(name, c_function_name, args_number, length_prop_value)
#endif /* !ROUTINE */

/* Object identifier */
OBJECT_ID (ECMA_BUILTIN_ID_DATE_PROTOTYPE)

OBJECT_VALUE (LIT_MAGIC_STRING_CONSTRUCTOR,
              ECMA_BUILTIN_ID_DATE,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

ROUTINE (LIT_MAGIC_STRING_TO_STRING_UL, ecma_builtin_date_prototype_to_string, 0, 0)
ROUTINE (LIT_MAGIC_STRING_TO_DATE_STRING_UL, ecma_builtin_date_prototype_to_date_string, 0, 0)
ROUTINE (LIT_MAGIC_STRING_TO_TIME_STRING_UL, ecma_builtin_date_prototype_to_time_string, 0, 0)
ROUTINE (LIT_MAGIC_STRING_TO_LOCALE_STRING_UL, ecma_builtin_date_prototype_to_locale_string, 0, 0)
ROUTINE (LIT_MAGIC_STRING_TO_LOCALE_DATE_STRING_UL, ecma_builtin_date_prototype_to_locale_date_string, 0, 0)
ROUTINE (LIT_MAGIC_STRING_TO_LOCALE_TIME_STRING_UL, ecma_builtin_date_prototype_to_locale_time_string, 0, 0)
ROUTINE (LIT_MAGIC_STRING_VALUE_OF_UL, ecma_builtin_date_prototype_value_of, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_TIME_UL, ecma_builtin_date_prototype_get_time, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_FULL_YEAR_UL, ecma_builtin_date_prototype_get_full_year, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_UTC_FULL_YEAR_UL, ecma_builtin_date_prototype_get_utc_full_year, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_MONTH_UL, ecma_builtin_date_prototype_get_month, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_UTC_MONTH_UL, ecma_builtin_date_prototype_get_utc_month, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_DATE_UL, ecma_builtin_date_prototype_get_date, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_UTC_DATE_UL, ecma_builtin_date_prototype_get_utc_date, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_DAY_UL, ecma_builtin_date_prototype_get_day, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_UTC_DAY_UL, ecma_builtin_date_prototype_get_utc_day, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_HOURS_UL, ecma_builtin_date_prototype_get_hours, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_UTC_HOURS_UL, ecma_builtin_date_prototype_get_utc_hours, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_MINUTES_UL, ecma_builtin_date_prototype_get_minutes, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_UTC_MINUTES_UL, ecma_builtin_date_prototype_get_utc_minutes, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_SECONDS_UL, ecma_builtin_date_prototype_get_seconds, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_UTC_SECONDS_UL, ecma_builtin_date_prototype_get_utc_seconds, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_MILLISECONDS_UL, ecma_builtin_date_prototype_get_milliseconds, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_UTC_MILLISECONDS_UL, ecma_builtin_date_prototype_get_utc_milliseconds, 0, 0)
ROUTINE (LIT_MAGIC_STRING_GET_TIMEZONE_OFFSET_UL, ecma_builtin_date_prototype_get_timezone_offset, 0, 0)
ROUTINE (LIT_MAGIC_STRING_SET_TIME_UL, ecma_builtin_date_prototype_set_time, 1, 1)
ROUTINE (LIT_MAGIC_STRING_SET_MILLISECONDS_UL, ecma_builtin_date_prototype_set_milliseconds, 1, 1)
ROUTINE (LIT_MAGIC_STRING_SET_UTC_MILLISECONDS_UL, ecma_builtin_date_prototype_set_utc_milliseconds, 1, 1)
ROUTINE (LIT_MAGIC_STRING_SET_SECONDS_UL, ecma_builtin_date_prototype_set_seconds, 2, 2)
ROUTINE (LIT_MAGIC_STRING_SET_UTC_SECONDS_UL, ecma_builtin_date_prototype_set_utc_seconds, 2, 2)
ROUTINE (LIT_MAGIC_STRING_SET_MINUTES_UL, ecma_builtin_date_prototype_set_minutes, NON_FIXED, 3)
ROUTINE (LIT_MAGIC_STRING_SET_UTC_MINUTES_UL, ecma_builtin_date_prototype_set_utc_minutes, NON_FIXED, 3)
ROUTINE (LIT_MAGIC_STRING_SET_HOURS_UL, ecma_builtin_date_prototype_set_hours, NON_FIXED, 4)
ROUTINE (LIT_MAGIC_STRING_SET_UTC_HOURS_UL, ecma_builtin_date_prototype_set_utc_hours, NON_FIXED, 4)
ROUTINE (LIT_MAGIC_STRING_SET_DATE_UL, ecma_builtin_date_prototype_set_date, 1, 1)
ROUTINE (LIT_MAGIC_STRING_SET_UTC_DATE_UL, ecma_builtin_date_prototype_set_utc_date, 1, 1)
ROUTINE (LIT_MAGIC_STRING_SET_MONTH_UL, ecma_builtin_date_prototype_set_month, 2, 2)
ROUTINE (LIT_MAGIC_STRING_SET_UTC_MONTH_UL, ecma_builtin_date_prototype_set_utc_month, 2, 2)
ROUTINE (LIT_MAGIC_STRING_SET_FULL_YEAR_UL, ecma_builtin_date_prototype_set_full_year, NON_FIXED, 3)
ROUTINE (LIT_MAGIC_STRING_SET_UTC_FULL_YEAR_UL, ecma_builtin_date_prototype_set_utc_full_year, NON_FIXED, 3)
ROUTINE (LIT_MAGIC_STRING_TO_UTC_STRING_UL, ecma_builtin_date_prototype_to_utc_string, 0, 0)
ROUTINE (LIT_MAGIC_STRING_TO_ISO_STRING_UL, ecma_builtin_date_prototype_to_iso_string, 0, 0)
ROUTINE (LIT_MAGIC_STRING_TO_JSON_UL, ecma_builtin_date_prototype_to_json, 1, 1)

#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN

ROUTINE (LIT_MAGIC_STRING_GET_YEAR_UL, ecma_builtin_date_prototype_get_year, 0, 0)
ROUTINE (LIT_MAGIC_STRING_SET_YEAR_UL, ecma_builtin_date_prototype_set_year, 1, 1)
ROUTINE (LIT_MAGIC_STRING_TO_GMT_STRING_UL, ecma_builtin_date_prototype_to_utc_string, 0, 0)

#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */

#undef OBJECT_ID
#undef SIMPLE_VALUE
#undef NUMBER_VALUE
#undef STRING_VALUE
#undef OBJECT_VALUE
#undef ROUTINE

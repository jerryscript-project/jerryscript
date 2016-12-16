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

#ifndef ECMA_BUILTIN_HELPERS_H
#define ECMA_BUILTIN_HELPERS_H

#include "ecma-globals.h"
#include "ecma-exceptions.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltinhelpers ECMA builtin helper operations
 * @{
 */

ecma_value_t
ecma_builtin_helper_object_to_string (const ecma_value_t this_arg);
ecma_value_t
ecma_builtin_helper_get_to_locale_string_at_index (ecma_object_t *obj_p, uint32_t index);
ecma_value_t
ecma_builtin_helper_object_get_properties (ecma_object_t *obj_p, bool only_enumerable_properties);
ecma_value_t
ecma_builtin_helper_array_concat_value (ecma_object_t *obj_p, uint32_t *length_p, ecma_value_t value);
uint32_t
ecma_builtin_helper_array_index_normalize (ecma_number_t index, uint32_t length);
uint32_t
ecma_builtin_helper_string_index_normalize (ecma_number_t index, uint32_t length, bool nan_to_zero);
ecma_value_t
ecma_builtin_helper_string_prototype_object_index_of (ecma_value_t this_arg, ecma_value_t arg1,
                                                      ecma_value_t arg2, bool first_index);
bool
ecma_builtin_helper_string_find_index (ecma_string_t *original_str_p, ecma_string_t *search_str_p, bool first_index,
                                       ecma_length_t start_pos, ecma_length_t *ret_index_p);
ecma_value_t
ecma_builtin_helper_def_prop (ecma_object_t *obj_p, ecma_string_t *index_p, ecma_value_t value,
                              bool writable, bool enumerable, bool configurable, bool is_throw);

#ifndef CONFIG_DISABLE_DATE_BUILTIN

/**
 * Time range defines for helper functions.
 *
 * See also:
 *          ECMA-262 v5, 15.9.1.1, 15.9.1.10
 */

/** Hours in a day. */
#define ECMA_DATE_HOURS_PER_DAY         ((ecma_number_t) 24)

/** Minutes in an hour. */
#define ECMA_DATE_MINUTES_PER_HOUR      ((ecma_number_t) 60)

/** Seconds in a minute. */
#define ECMA_DATE_SECONDS_PER_MINUTE    ((ecma_number_t) 60)

/** Milliseconds in a second. */
#define ECMA_DATE_MS_PER_SECOND         ((ecma_number_t) 1000)

/** ECMA_DATE_MS_PER_MINUTE == 60000 */
#define ECMA_DATE_MS_PER_MINUTE         (ECMA_DATE_MS_PER_SECOND * ECMA_DATE_SECONDS_PER_MINUTE)

/** ECMA_DATE_MS_PER_HOUR == 3600000 */
#define ECMA_DATE_MS_PER_HOUR           (ECMA_DATE_MS_PER_MINUTE * ECMA_DATE_MINUTES_PER_HOUR)

/** ECMA_DATE_MS_PER_DAY == 86400000 */
#define ECMA_DATE_MS_PER_DAY            (ECMA_DATE_MS_PER_HOUR * ECMA_DATE_HOURS_PER_DAY)

/**
 * This gives a range of 8,640,000,000,000,000 milliseconds
 * to either side of 01 January, 1970 UTC.
 */
#define ECMA_DATE_MAX_VALUE             8.64e15

/**
 * Timezone type.
 */
typedef enum
{
  ECMA_DATE_UTC, /**< date vaule is in UTC */
  ECMA_DATE_LOCAL /**< date vaule is in local time */
} ecma_date_timezone_t;

/* ecma-builtin-helpers-date.c */
ecma_number_t ecma_date_day (ecma_number_t time);
ecma_number_t ecma_date_time_within_day (ecma_number_t time);
ecma_number_t ecma_date_days_in_year (ecma_number_t year);
ecma_number_t ecma_date_day_from_year (ecma_number_t year);
ecma_number_t ecma_date_time_from_year (ecma_number_t year);
ecma_number_t ecma_date_year_from_time (ecma_number_t time);
ecma_number_t ecma_date_in_leap_year (ecma_number_t time);
ecma_number_t ecma_date_day_within_year (ecma_number_t time);
ecma_number_t ecma_date_month_from_time (ecma_number_t time);
ecma_number_t ecma_date_date_from_time (ecma_number_t time);
ecma_number_t ecma_date_week_day (ecma_number_t time);
ecma_number_t ecma_date_local_tza ();
ecma_number_t ecma_date_daylight_saving_ta (ecma_number_t time);
ecma_number_t ecma_date_local_time (ecma_number_t time);
ecma_number_t ecma_date_utc (ecma_number_t time);
ecma_number_t ecma_date_hour_from_time (ecma_number_t time);
ecma_number_t ecma_date_min_from_time (ecma_number_t time);
ecma_number_t ecma_date_sec_from_time (ecma_number_t time);
ecma_number_t ecma_date_ms_from_time (ecma_number_t time);
ecma_number_t ecma_date_make_time (ecma_number_t hour, ecma_number_t min, ecma_number_t sec, ecma_number_t ms);
ecma_number_t ecma_date_make_day (ecma_number_t year, ecma_number_t month, ecma_number_t date);
ecma_number_t ecma_date_make_date (ecma_number_t day, ecma_number_t time);
ecma_number_t ecma_date_time_clip (ecma_number_t time);
ecma_number_t ecma_date_timezone_offset (ecma_number_t time);

ecma_value_t ecma_date_value_to_string (ecma_number_t datetime_number);
ecma_value_t ecma_date_value_to_utc_string (ecma_number_t datetime_number);
ecma_value_t ecma_date_value_to_iso_string (ecma_number_t datetime_number);
ecma_value_t ecma_date_value_to_date_string (ecma_number_t datetime_number);
ecma_value_t ecma_date_value_to_time_string (ecma_number_t datetime_number);

#endif /* !CONFIG_DISABLE_DATE_BUILTIN */

/* ecma-builtin-helper-json.c */

/**
 * Context for JSON.stringify()
 */
typedef struct
{
  /** Collection for property keys. */
  ecma_collection_header_t *property_list_p;

  /** Collection for traversing objects. */
  ecma_collection_header_t *occurence_stack_p;

  /** The actual indentation text. */
  ecma_string_t *indent_str_p;

  /** The indentation text. */
  ecma_string_t *gap_str_p;

  /** The replacer function. */
  ecma_object_t *replacer_function_p;
} ecma_json_stringify_context_t;

bool ecma_has_object_value_in_collection (ecma_collection_header_t *collection_p, ecma_value_t object_value);
bool ecma_has_string_value_in_collection (ecma_collection_header_t *collection_p, ecma_value_t string_value);

ecma_string_t *
ecma_builtin_helper_json_create_hex_digit_ecma_string (uint8_t value);
ecma_string_t *
ecma_builtin_helper_json_create_separated_properties (ecma_collection_header_t *partial_p, ecma_string_t *separator_p);
ecma_value_t
ecma_builtin_helper_json_create_formatted_json (ecma_string_t *left_bracket_p, ecma_string_t *right_bracket_p,
                                                ecma_string_t *stepback_p, ecma_collection_header_t *partial_p,
                                                ecma_json_stringify_context_t *context_p);
ecma_value_t
ecma_builtin_helper_json_create_non_formatted_json (ecma_string_t *left_bracket_p, ecma_string_t *right_bracket_p,
                                                    ecma_collection_header_t *partial_p);

/* ecma-builtin-helper-error.c */

ecma_value_t
ecma_builtin_helper_error_dispatch_call (ecma_standard_error_t error_type, const ecma_value_t *arguments_list_p,
                                         ecma_length_t arguments_list_len);

/**
 * @}
 * @}
 */

#endif /* !ECMA_BUILTIN_HELPERS_H */

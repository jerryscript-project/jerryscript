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

#ifndef ECMA_OBJECT_PROTOTYPE_H
#define ECMA_OBJECT_PROTOTYPE_H

#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltinhelpers ECMA builtin helper operations
 * @{
 */

extern ecma_completion_value_t ecma_builtin_helper_object_to_string (const ecma_value_t this_arg);
extern ecma_completion_value_t ecma_builtin_helper_get_to_locale_string_at_index (ecma_object_t *obj_p, uint32_t index);
extern ecma_completion_value_t ecma_builtin_helper_object_get_properties (ecma_object_t *obj,
                                                                          bool only_enumerable_properties);
extern uint32_t ecma_builtin_helper_array_index_normalize (ecma_number_t index, uint32_t length);

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN
/* ecma-builtin-helpers-date.cpp */
extern ecma_number_t ecma_date_day (ecma_number_t time);
extern ecma_number_t ecma_date_time_within_day (ecma_number_t time);
extern ecma_number_t ecma_date_days_in_year (ecma_number_t year);
extern ecma_number_t ecma_date_day_from_year (ecma_number_t year);
extern ecma_number_t ecma_date_time_from_year (ecma_number_t year);
extern ecma_number_t ecma_date_year_from_time (ecma_number_t time);
extern ecma_number_t ecma_date_in_leap_year (ecma_number_t time);
extern ecma_number_t ecma_date_day_within_year (ecma_number_t time);
extern ecma_number_t ecma_date_month_from_time (ecma_number_t time);
extern ecma_number_t ecma_date_date_from_time (ecma_number_t time);
extern ecma_number_t ecma_date_week_day (ecma_number_t time);
extern ecma_number_t ecma_date_local_tza ();
extern ecma_number_t ecma_date_daylight_saving_ta (ecma_number_t time);
extern ecma_number_t ecma_date_local_time (ecma_number_t time);
extern ecma_number_t ecma_date_utc (ecma_number_t time);
extern ecma_number_t ecma_date_hour_from_time (ecma_number_t time);
extern ecma_number_t ecma_date_min_from_time (ecma_number_t time);
extern ecma_number_t ecma_date_sec_from_time (ecma_number_t time);
extern ecma_number_t ecma_date_ms_from_time (ecma_number_t time);
extern ecma_number_t ecma_date_make_time (ecma_number_t hour,
                                          ecma_number_t min,
                                          ecma_number_t sec,
                                          ecma_number_t ms);
extern ecma_number_t ecma_date_make_day (ecma_number_t year,
                                         ecma_number_t month,
                                         ecma_number_t date);
extern ecma_number_t ecma_date_make_date (ecma_number_t day, ecma_number_t time);
extern ecma_number_t ecma_date_time_clip (ecma_number_t time);
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN */

/**
 * @}
 * @}
 */

#endif /* !ECMA_OBJECT_PROPERTY_H */

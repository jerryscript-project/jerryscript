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

#ifndef JERRYX_HANDLERS_H
#define JERRYX_HANDLERS_H

#include "jerryscript-types.h"

JERRY_C_API_BEGIN

jerry_value_t jerryx_handler_assert (const jerry_call_info_t *call_info_p,
                                     const jerry_value_t args_p[],
                                     const jerry_length_t args_cnt);
jerry_value_t
jerryx_handler_gc (const jerry_call_info_t *call_info_p, const jerry_value_t args_p[], const jerry_length_t args_cnt);
jerry_value_t jerryx_handler_print (const jerry_call_info_t *call_info_p,
                                    const jerry_value_t args_p[],
                                    const jerry_length_t args_cnt);
jerry_value_t jerryx_handler_source_name (const jerry_call_info_t *call_info_p,
                                          const jerry_value_t args_p[],
                                          const jerry_length_t args_cnt);
jerry_value_t jerryx_handler_create_realm (const jerry_call_info_t *call_info_p,
                                           const jerry_value_t args_p[],
                                           const jerry_length_t args_cnt);
void jerryx_handler_promise_reject (jerry_promise_event_type_t event_type,
                                    const jerry_value_t object,
                                    const jerry_value_t value,
                                    void *user_p);
jerry_value_t jerryx_handler_source_received (const jerry_char_t *source_name_p,
                                              size_t source_name_size,
                                              const jerry_char_t *source_p,
                                              size_t source_size,
                                              void *user_p);
JERRY_C_API_END

#endif /* !JERRYX_HANDLERS_H */

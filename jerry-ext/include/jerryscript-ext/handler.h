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

#ifndef JERRYX_HANDLER_H
#define JERRYX_HANDLER_H

#include "jerryscript.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * Handler registration helper
 */

jerry_value_t jerryx_handler_register_global (const jerry_char_t *name_p,
                                              jerry_external_handler_t handler_p);

/*
 * Common external function handlers
 */

jerry_value_t jerryx_handler_assert (const jerry_value_t func_obj_val, const jerry_value_t this_p,
                                     const jerry_value_t args_p[], const jerry_length_t args_cnt);
jerry_value_t jerryx_handler_gc (const jerry_value_t func_obj_val, const jerry_value_t this_p,
                                 const jerry_value_t args_p[], const jerry_length_t args_cnt);
jerry_value_t jerryx_handler_print (const jerry_value_t func_obj_val, const jerry_value_t this_p,
                                    const jerry_value_t args_p[], const jerry_length_t args_cnt);

/*
 * Port API extension
 */

/**
 * Print a single character.
 *
 * @param c the character to print.
 */
void jerryx_port_handler_print_char (char c);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !JERRYX_HANDLER_H */

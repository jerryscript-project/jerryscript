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

#ifndef JERRYX_ARG_INTERNAL_H
#define JERRYX_ARG_INTERNAL_H

#include "jerryscript.h"

/**
 * The iterator structor for JS arguments.
 */
struct jerryx_arg_js_iterator_t
{
  const jerry_value_t *js_arg_p; /**< the JS arguments */
  const jerry_length_t js_arg_cnt; /**< the total num of JS arguments */
  jerry_length_t js_arg_idx; /**< current index of JS argument */
};

#endif /* !JERRYX_ARG_INTERNAL_H */

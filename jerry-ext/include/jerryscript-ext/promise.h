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

#ifndef JERRYX_PROMISE_H
#define JERRYX_PROMISE_H

#include "jerryscript.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * Promise rejection tracker
 */

void jerryx_promise_rejection_tracker (jerry_value_t promise,
                                       jerry_value_t reason);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !JERRYX_PROMISE_H */

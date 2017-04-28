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

#ifndef JERRYX_CONTEXT_H
#define JERRYX_CONTEXT_H

#include "jerryscript.h"

#ifndef JERRYX_CONTEXT_SLOTS
#define JERRYX_CONTEXT_SLOTS 1
#endif /* !JERRYX_CONTEXT_SLOTS */

/**
 * Function to pass to jerry_init_with_user_context() as its second parameter
 */
void *jerryx_context_init (void);

/**
 * Function to pass to jerry_init_with_user_context() as its third parameter
 */
void jerryx_context_deinit (void *user_context_p);

/**
 * Function to request an index for per-context data
 */
int jerryx_context_request_slot (jerry_user_context_init_cb init_cb,
                                 jerry_user_context_deinit_cb deinit_cb);


void *jerryx_context_get (int slot);
#endif /* !JERRYX_CONTEXT_H */

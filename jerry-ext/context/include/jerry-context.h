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

/**
 * Function to pass to jerry_init_with_user_context() as its second parameter
 */
void *jerryx_context_init (void);

/**
 * Function to pass to jerry_init_with_user_context() as its third parameter
 */
void jerryx_context_deinit (void *user_context_p);

typedef struct
{
  jerry_user_context_init_cb init_cb;
  jerry_user_context_deinit_cb deinit_cb;
  int index;
} jerryx_context_slot_t;

#define JERRYX_CONTEXT_DEFINE_SLOT(slot_name, init_cb_ptr, deinit_cb_ptr)         \
static jerryx_context_slot_t __jerryx_context_slot_ ## slot_name                  \
__attribute__((used, section("jerryx_context_slots"), aligned(sizeof(void *)))) = \
{                                                                                 \
  .init_cb = (init_cb_ptr),                                                       \
  .deinit_cb = (deinit_cb_ptr),                                                   \
  .index = -1                                                                     \
};

void *jerryx_context_get (int slot);

#define JERRYX_CONTEXT_SLOT(slot_name) \
  (jerryx_context_get (__jerryx_context_slot_ ## slot_name.index))

#endif /* !JERRYX_CONTEXT_H */

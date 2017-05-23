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
#include "jerryscript-ext/section.impl.h"

/**
 * Function to pass to jerry_init_with_user_context() as its second parameter
 */
void *jerryx_context_init (void);

/**
 * Function to pass to jerry_init_with_user_context() as its third parameter
 */
void jerryx_context_deinit (void *user_context_p);

/**
 * Definition of a context slot.
 */
typedef struct
{
  jerry_user_context_init_t init_cb; /**< the initialization function */
  jerry_user_context_deinit_t deinit_cb; /**< the deinitialization function */
} jerryx_context_slot_t;

/**
 * Define a slot.
 *
 * Use this macro at the global level in a source file to define a slot. Afterwards you can use JERRYX_CONTEXT_SLOT
 * further down in the same file to retrieve the contents of the slot from the user context.
 *
 * @param slot_name the suffix to be given to the global static variable holding the slot definition.
 * @param init_cb_ptr a pointer to a function of type jerry_user_context_init_t to be called when initializing a slot.
 * @param deinit_cb_ptr a pointer to a function of type jerry_user_context_deinit_t to be called when freeing a slot.
 */
#define JERRYX_CONTEXT_DEFINE_SLOT(slot_name, init_cb_ptr, deinit_cb_ptr)                                             \
  static const jerryx_context_slot_t __jerryx_context_slot_ ## slot_name JERRYX_SECTION_ATTRIBUTE(jerryx_ctx_slots) = \
  {                                                                                                                   \
    .init_cb = (init_cb_ptr),                                                                                         \
    .deinit_cb = (deinit_cb_ptr)                                                                                      \
  };

/**
 * Retrieve the contents of a slot.
 *
 * This function is best called via the macro JERRYX_CONTEXT_SLOT.
 */
void *jerryx_context_get (const jerryx_context_slot_t *slot_p);

/**
 * Retrieve the contents of a slot.
 *
 * Calls jerryx_context_get () with a pointer to the structure declared earlier using JERRYX_CONTEXT_DEFINE_SLOT ().
 *
 * @param slot_name the name of the slot without quotes.
 */
#define JERRYX_CONTEXT_SLOT(slot_name) \
  (jerryx_context_get (&(__jerryx_context_slot_ ## slot_name)))

#endif /* !JERRYX_CONTEXT_H */

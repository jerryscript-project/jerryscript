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

#include <string.h>
#include "jerryscript-ext/context.h"
#include "jmem.h"

static int jerryx_context_slot_count = -1;

/* The section name must not exceed 16 characters on __MACH__, hence the abbreviation "ctx" */
JERRYX_SECTION_DECLARE (jerryx_ctx_slots, jerryx_context_slot_t)

/**
 * Create a new context. This function can be passed to jerry_init_with_user_context () as its second parameter.
 *
 * @return the pointer to store as the user context.
 */
void *
jerryx_context_init (void)
{
  void **ret_pp;
  int index;
  size_t context_size;
  const jerryx_context_slot_t *slot_p = NULL;

  if (jerryx_context_slot_count < 0)
  {
    jerryx_context_slot_count = (int) (__stop_jerryx_ctx_slots - __start_jerryx_ctx_slots);
  }

  context_size = ((size_t) jerryx_context_slot_count) * sizeof (void *);
  ret_pp = (void **) jmem_heap_alloc_block (context_size);
  memset (ret_pp, 0, context_size);

  for JERRYX_SECTION_ITERATE (jerryx_ctx_slots, index, slot_p)
  {
    ret_pp[index] = slot_p->init_cb ();
  }

  return ((void *) ret_pp);
} /* jerryx_context_init */

/**
 * Free a context. This function can be passed to jerry_init_with_user_context () as its third parameter.
 */
void
jerryx_context_deinit (void *user_context_p) /**< user context to deinit */
{
  if (user_context_p)
  {
    int index;
    const jerryx_context_slot_t *slot_p = NULL;
    void **slots_p = (void **) user_context_p;

    for JERRYX_SECTION_ITERATE (jerryx_ctx_slots, index, slot_p)
    {
      if (slot_p->deinit_cb)
      {
        slot_p->deinit_cb (slots_p[index]);
      }
    }
    jmem_heap_free_block (slots_p, ((size_t) jerryx_context_slot_count) * sizeof (void *));
  }
} /* jerryx_context_deinit */

/**
 * Request the slot at the given index.
 *
 * @return the user data that was created in the given slot when the context was initialized.
 */
void *
jerryx_context_get (const jerryx_context_slot_t *slot_p) /**< the slot from which to retrieve the data */
{
  int slot = (int) (slot_p - __start_jerryx_ctx_slots);
  void **slots_p = (void **) jerry_get_user_context ();

#ifndef JERRY_NDEBUG
  if (jerryx_context_slot_count < 0 || !slots_p || slot < 0 || slot >= jerryx_context_slot_count)
  {
    jerry_port_fatal (ERR_FAILED_INTERNAL_ASSERTION);
  }
#endif /* !JERRY_NDEBUG */

  return slots_p[slot];
} /* jerryx_context_get */

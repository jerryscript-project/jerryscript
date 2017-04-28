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
#include "jerry-context.h"
#include "jmem.h"

#define HEAP_BLOCK_SIZE (JERRYX_CONTEXT_SLOTS * sizeof (void *))

typedef struct
{
  jerry_user_context_init_cb init_cb;
  jerry_user_context_deinit_cb deinit_cb;
} jerryx_context_slot_info_t;

/* Context-free data */

/* We do not allocate slots after a context has already been created. */
bool before_first_context = true;
static int next_index = -1;
static jerryx_context_slot_info_t slot_info[JERRYX_CONTEXT_SLOTS] =
{
  {
    NULL, NULL
  }
};

/**
 * Create a new context. This function can be passed to jerry_init_with_user_context () as its second parameter.
 *
 * @return the pointer to store as the user context.
 */
void *
jerryx_context_init (void)
{
  size_t index = 0;
  void **ret = NULL;

  before_first_context = false;

  ret = (void **) jmem_heap_alloc_block_null_on_error (HEAP_BLOCK_SIZE);

  if (ret)
  {
    memset (ret, 0, HEAP_BLOCK_SIZE);

    for (index = 0; index < JERRYX_CONTEXT_SLOTS; index++)
    {
      if (slot_info[index].init_cb)
      {
        ret[index] = slot_info[index].init_cb ();
      }
    }
  }

  return ((void *) ret);
} /* jerryx_context_init */

/**
 * Free a context. This function can be passed to jerry_init_with_user_context () as its third parameter.
 */
void
jerryx_context_deinit (void *user_context_p) /**< user context to deinit */
{
  size_t index;
  void **slots = NULL;

  if (user_context_p)
  {
    slots = (void **) user_context_p;
    for (index = 0; index < JERRYX_CONTEXT_SLOTS; index++)
    {
      if (slot_info[index].deinit_cb)
      {
        slot_info[index].deinit_cb (slots[index]);
      }
    }
    jmem_heap_free_block (slots, HEAP_BLOCK_SIZE);
  }
} /* jerryx_context_deinit */

/**
 * Request an index for per-context data.
 *
 * @return the slot in which the user data will be stored, or -1 on failure. The value can later be passed to
 * jerryx_context_get_slot () to retrieve the per-context user data.
 */
int
jerryx_context_request_slot (jerry_user_context_init_cb init_cb, /**< callback for creating a new context */
                             jerry_user_context_deinit_cb deinit_cb) /**< callback for freeing context */
{
  int slot_index = -1;

  if (before_first_context)
  {
    slot_index = (++next_index);
    if (slot_index >= JERRYX_CONTEXT_SLOTS)
    {
      slot_index = -1;
    }
    else
    {
      slot_info[slot_index].init_cb = init_cb;
      slot_info[slot_index].deinit_cb = deinit_cb;
    }
  }

  return slot_index;
} /* jerryx_context_request_slot */

/**
 * Request the slot at the given index.
 *
 * @return the user data that was created in the given slot when the context was initialized.
 */
void *
jerryx_context_get_slot (int slot) /**< the slot from which to retrieve the data */
{
  void *return_value = NULL;
  void **slots = (void **) jerry_get_user_context ();

  if (slots && slot >= 0 && slot < JERRYX_CONTEXT_SLOTS)
  {
    return_value = slots[slot];
  }

  return return_value;
} /* jerryx_context_get_slot */

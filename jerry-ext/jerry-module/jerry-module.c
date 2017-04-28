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

#include "jmem.h"
#include "jerryscript.h"
#include "jerry-module.h"

static void
jerryx_module_link_insert (jerryx_module_header_t *link,
                           jerryx_module_header_t **root)
{
  if (*root)
  {
    link->next = (*root);
  }
  (*root) = link;
} /* jerryx_module_link_insert */

static jerryx_module_header_t *
jerryx_module_link_find (jerryx_module_header_t *root,
                         const jerry_char_t *name,
                         jerryx_module_header_t **previous)
{
  for (;root && strcmp ((char *) name, ((char *) root->name)); root = root->next)
  {
    if (previous)
    {
      (*previous) = root;
    }
  }

  return root;
} /* jerryx_module_link_find */

static jerryx_module_header_t *
jerryx_module_link_remove (const jerry_char_t *name,
                           jerryx_module_header_t **root)
{
  jerryx_module_header_t *previous = NULL;
  jerryx_module_header_t *return_value = jerryx_module_link_find ((*root), name, &previous);

  if (return_value)
  {
    if (previous)
    {
      previous->next = return_value->next;
    }
    else
    {
      (*root) = return_value->next;
    }
    JERRYX_MODULE_HEADER_UNLINK (return_value);
  }

  return return_value;
} /* jerryx_module_link_remove */

static void
jerryx_module_link_free (jerryx_module_header_t *root,
                         void (*free_link) (jerryx_module_header_t *link))
{
  jerryx_module_header_t *next;
  while (root)
  {
    next = root->next;
    free_link (root);
    root = next;
  }
} /* jerryx_module_link_free */

typedef struct
{
  jerryx_module_header_t link;
  jerry_value_t result;
} jerryx_module_instance_t;

static void
jerryx_module_free_instance (jerryx_module_header_t *link)
{
  jerryx_module_instance_t *instance = (jerryx_module_instance_t *) link;
  jerry_release_value (instance->result);
  jmem_heap_free_block (instance, sizeof (jerryx_module_instance_t));
} /* jerryx_module_free_instance */

void *
jerryx_module_context_init (void)
{
  void *return_value = jmem_heap_alloc_block_null_on_error (sizeof (jerryx_module_instance_t *));
  memset (return_value, 0, sizeof (jerryx_module_instance_t *));
  return return_value;
} /* jerryx_module_context_init */

void
jerryx_module_context_deinit (void *context_p)
{
  if (context_p)
  {
    jerryx_module_link_free (*((jerryx_module_header_t **) (context_p)), jerryx_module_free_instance);
    jmem_heap_free_block (context_p, sizeof (jerryx_module_header_t *));
  }
} /* jerryx_module_context_deinit */

#ifdef JERRYX_MODULE_HAVE_CONTEXT
#include "jerry-context.h"
static int jerryx_module_user_context_slot = -1;
JERRYX_C_CTOR (_register_module_handler_)
{
  jerryx_module_user_context_slot = jerryx_context_request_slot (jerryx_module_context_init,
                                                                 jerryx_module_context_deinit);
} /* JERRYX_C_CTOR */
#define JERRYX_MODULE_CONTEXT \
  (jerryx_context_get (jerryx_module_user_context_slot))
#else /* !JERRYX_MODULE_HAVE_CONTEXT */
#define JERRYX_MODULE_CONTEXT \
  (jerry_get_user_context ())
#endif /* JERRYX_MODULE_HAVE_CONTEXT */

/* Context-free list of modules */
static jerryx_module_t *modules = NULL;

void
jerryx_module_register (jerryx_module_t *module_p)
{
  jerryx_module_link_insert ((jerryx_module_header_t *) module_p, ((jerryx_module_header_t **) (&modules)));
} /* jerryx_module_register */

void
jerryx_module_unregister (jerryx_module_t *module_p)
{
  jerryx_module_link_remove (module_p->link.name, ((jerryx_module_header_t **) (&modules)));
} /* jerryx_module_unregister */

typedef struct
{
  jerryx_module_header_t link;
  jerry_value_t result;
} jerryx_instance_t;

static const char not_found_prologue[] = "Module '";
static const char not_found_epilogue[] = "' not found";

jerry_value_t
jerryx_module_load (const jerry_char_t *name)
{
  jerry_value_t return_value = 0;
  size_t error_message_size = 0, name_length = strlen ((char *) name);
  char *error_message = NULL;
  jerryx_module_t *module = NULL;
  jerryx_module_instance_t **instances_pp = JERRYX_MODULE_CONTEXT;
  jerryx_instance_t *instance_p = NULL;

  if (instances_pp)
  {
    instance_p =
    (jerryx_instance_t *) jerryx_module_link_find (*((jerryx_module_header_t **) instances_pp), name, NULL);
  }

  if (instance_p)
  {
    return instance_p->result;
  }

  module = (jerryx_module_t *) jerryx_module_link_find ((jerryx_module_header_t *) modules, name, NULL);

  if (module)
  {
    return_value = module->init ();
    if (instances_pp)
    {
      instance_p = (jerryx_instance_t *) jmem_heap_alloc_block_null_on_error (sizeof (jerryx_instance_t));
      if (instance_p)
      {
        JERRYX_MODULE_HEADER_RUNTIME_INIT (instance_p, module->link.name);
        instance_p->result = jerry_acquire_value (return_value);
        jerryx_module_link_insert ((jerryx_module_header_t *) instance_p, (jerryx_module_header_t **) instances_pp);
      }
    }
    return return_value;
  }

  error_message_size = name_length + sizeof (not_found_prologue) + sizeof (not_found_epilogue) - 1;
  error_message = jmem_heap_alloc_block_null_on_error (error_message_size);
  if (error_message)
  {
    memcpy (error_message, not_found_prologue, sizeof (not_found_prologue) - 1);
    memcpy (&error_message[sizeof (not_found_prologue) - 1], ((char *) name), name_length);
    memcpy (&error_message[sizeof (not_found_prologue) - 1 + name_length], not_found_epilogue,
            sizeof (not_found_epilogue) - 1);
    error_message[error_message_size - 1] = 0;
    return_value = jerry_create_error (JERRY_ERROR_COMMON, (jerry_char_t *) error_message);
    jmem_heap_free_block (error_message, error_message_size);
  }

  return return_value;
} /* jerryx_module_load */

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
jerryx_module_header_insert (jerryx_module_header_t *header_p,
                            jerryx_module_header_t **root_pp)
{
  if (*root_pp)
  {
    header_p->next_p = (*root_pp);
  }
  (*root_pp) = header_p;
} /* jerryx_module_header_insert */

static jerryx_module_header_t *
jerryx_module_header_find (jerryx_module_header_t *root_p,
                           const jerry_char_t *name,
                         jerryx_module_header_t **previous_pp)
{
  for (;root_p && strcmp ((char *) name, ((char *) root_p->name)); root_p = root_p->next_p)
  {
    if (previous_pp)
    {
      (*previous_pp) = root_p;
    }
  }

  return root_p;
} /* jerryx_module_header_find */

static jerryx_module_header_t *
jerryx_module_header_remove (const jerry_char_t *name,
                             jerryx_module_header_t **root_pp)
{
  jerryx_module_header_t *previous_p = NULL;
  jerryx_module_header_t *ret = jerryx_module_header_find ((*root_pp), name, &previous_p);

  if (ret)
  {
    if (previous_p)
    {
      previous_p->next_p = ret->next_p;
    }
    else
    {
      (*root_pp) = ret->next_p;
    }
    JERRYX_MODULE_HEADER_UNLINK (ret);
  }

  return ret;
} /* jerryx_module_header_remove */

static void
jerryx_module_header_free (jerryx_module_header_t *root_p,
                           void (*free_link) (jerryx_module_header_t *))
{
  jerryx_module_header_t *next_p = NULL;
  while (root_p)
  {
    next_p = root_p->next_p;
    free_link (root_p);
    root_p = next_p;
  }
} /* jerryx_module_header_free */

typedef struct
{
  jerryx_module_header_t header;
  jerry_value_t export;
} jerryx_module_instance_t;

static void
jerryx_module_free_instance (jerryx_module_header_t *header_p)
{
  jerryx_module_instance_t *instance_p = (jerryx_module_instance_t *) header_p;
  jerry_release_value (instance_p->export);
  jmem_heap_free_block (instance_p, sizeof (jerryx_module_instance_t));
} /* jerryx_module_free_instance */

void *
jerryx_module_manager_init (void)
{
  void *ret = jmem_heap_alloc_block (sizeof (jerryx_module_instance_t *));
  memset (ret, 0, sizeof (jerryx_module_instance_t *));
  return ret;
} /* jerryx_module_manager_init */

void
jerryx_module_manager_deinit (void *context_p)
{
  if (context_p)
  {
    jerryx_module_header_free (*((jerryx_module_header_t **) (context_p)), jerryx_module_free_instance);
    jmem_heap_free_block (context_p, sizeof (jerryx_module_header_t *));
  }
} /* jerryx_module_manager_deinit */

#ifdef JERRYX_MODULE_HAVE_CONTEXT
#include "jerry-context.h"
static int jerryx_module_user_context_slot = -1;
JERRYX_C_CTOR (_register_module_handler_)
{
  jerryx_module_user_context_slot = jerryx_context_request_slot (jerryx_module_manager_init,
                                                                 jerryx_module_manager_deinit);
} /* JERRYX_C_CTOR */
#define JERRYX_MODULE_CONTEXT \
  (jerryx_context_get (jerryx_module_user_context_slot))
#else /* !JERRYX_MODULE_HAVE_CONTEXT */
#define JERRYX_MODULE_CONTEXT \
  (jerry_get_user_context ())
#endif /* JERRYX_MODULE_HAVE_CONTEXT */

/* This list of module definitions is constructed by calls to jerry_module_register (). jerry_module_register () does
 * not require a user context. The list is simply a collection of global static structures defined in each module,
 * arranged into a linked list. */
static jerryx_module_t *modules_p = NULL;

void
jerryx_module_register (jerryx_module_t *module_p)
{
  jerryx_module_header_insert ((jerryx_module_header_t *) module_p, ((jerryx_module_header_t **) (&modules_p)));
} /* jerryx_module_register */

void
jerryx_module_unregister (jerryx_module_t *module_p)
{
  jerryx_module_header_remove (module_p->header.name, ((jerryx_module_header_t **) (&modules_p)));
} /* jerryx_module_unregister */

JERRYX_MODULE_RESOLVER (native_resolver)
{
  jerry_value_t ret = 0;
  jerryx_module_t *module_p = NULL;
  jerryx_module_instance_t **instances_pp = JERRYX_MODULE_CONTEXT;
  jerryx_module_instance_t *instance_p = NULL;

  instance_p =
  (jerryx_module_instance_t *) jerryx_module_header_find (*((jerryx_module_header_t **) instances_pp), name, NULL);

  if (instance_p)
  {
    return instance_p->export;
  }

  module_p = (jerryx_module_t *) jerryx_module_header_find ((jerryx_module_header_t *) modules_p, name, NULL);

  if (module_p)
  {
    ret = module_p->on_resolve ();
    instance_p = (jerryx_module_instance_t *) jmem_heap_alloc_block (sizeof (jerryx_module_instance_t));
    JERRYX_MODULE_HEADER_RUNTIME_INIT (instance_p, module_p->header.name);
    instance_p->export = jerry_acquire_value (ret);
    jerryx_module_header_insert ((jerryx_module_header_t *) instance_p, (jerryx_module_header_t **) instances_pp);
    return ret;
  }

  return 0;
} /* JERRYX_MODULE_RESOLVER */

static const char not_found_prologue[] = "Module '";
static const char not_found_epilogue[] = "' not found";

/*
 * These symbols are made available by the linker
 */
typedef jerry_value_t (*jerryx_module_resolver_t)(const jerry_char_t *);
extern jerryx_module_resolver_t __start_jerryx_module_resolvers[] __attribute__((weak));
extern jerryx_module_resolver_t __stop_jerryx_module_resolvers[] __attribute__((weak));

jerry_value_t
jerryx_module_resolve (const jerry_char_t *name)
{
  jerry_value_t ret;
  char *error_message = NULL;
  size_t error_message_size = 0, name_length = strlen ((char *) name);
  size_t index, resolver_count = (size_t) (__stop_jerryx_module_resolvers - __start_jerryx_module_resolvers);

  for (index = 0; index < resolver_count; index++)
  {
    ret = __start_jerryx_module_resolvers[index] (name);
    if (ret)
    {
      return ret;
    }
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
    ret = jerry_create_error (JERRY_ERROR_COMMON, (jerry_char_t *) error_message);
    jmem_heap_free_block (error_message, error_message_size);
  }

  return ret;
} /* jerryx_module_resolve */

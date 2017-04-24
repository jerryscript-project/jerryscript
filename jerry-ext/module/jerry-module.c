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

typedef struct jerryx_module_header
{
  jerry_char_t *name;
  struct jerryx_module_header *next_p;
} jerryx_module_header_t;

#define JERRYX_MODULE_HEADER_UNLINK(header) \
  ((jerryx_module_header_t *) (header))->next_p = NULL

#define JERRYX_MODULE_HEADER_RUNTIME_INIT(header, module_name) \
  ((jerryx_module_header_t *) (header))->name = (module_name); \
  JERRYX_MODULE_HEADER_UNLINK((header))

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
JERRYX_CONTEXT_DEFINE_SLOT (jerryx_module_slot, jerryx_module_manager_init, jerryx_module_manager_deinit)
#define JERRYX_MODULE_CONTEXT \
  JERRYX_CONTEXT_SLOT(jerryx_module_slot)
#else /* !JERRYX_MODULE_HAVE_CONTEXT */
#define JERRYX_MODULE_CONTEXT \
  (jerry_get_user_context ())
#endif /* JERRYX_MODULE_HAVE_CONTEXT */

/*
 * These symbols are made available by the linker
 */
extern jerryx_module_t __start_jerryx_modules[] __attribute__((weak));
extern jerryx_module_t __stop_jerryx_modules[] __attribute__((weak));
#define JERRYX_MODULE_ITERATE_OVER_MODULES(index, module_p) \
(index = 0, module_p = &__start_jerryx_modules[index];     \
  &__start_jerryx_modules[index] < __stop_jerryx_modules;  \
  index++, module_p = &__start_jerryx_modules[index])

JERRYX_MODULE_RESOLVER (native_resolver)
{
  int index;
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

  for JERRYX_MODULE_ITERATE_OVER_MODULES (index, module_p)
  {
    if (!strcmp ((char *) module_p->name, (char *) name))
    {
      ret = module_p->on_resolve ();
      instance_p = (jerryx_module_instance_t *) jmem_heap_alloc_block (sizeof (jerryx_module_instance_t));
      JERRYX_MODULE_HEADER_RUNTIME_INIT (instance_p, module_p->name);
      instance_p->export = jerry_acquire_value (ret);
      jerryx_module_header_insert ((jerryx_module_header_t *) instance_p, (jerryx_module_header_t **) instances_pp);
      return ret;
    }
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

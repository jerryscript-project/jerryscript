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
#include "jerryscript-ext/module.h"

static const jerry_char_t *module_name_property_name = (jerry_char_t *) "moduleName";
static const jerry_char_t *on_resolve_absent = (jerry_char_t *) "Module on_resolve () must not be NULL";
static const jerry_char_t *module_not_found = (jerry_char_t *) "Module not found";

/**
 * Create an error related to modules
 *
 * Creates an error object of the requested type with the additional property "moduleName" the value of which is a
 * string containing the name of the module that was requested when the error occurred.
 *
 * @return the error
 */
static jerry_value_t
jerryx_module_create_error (jerry_error_t error_type, /**< the type of error to create */
                            const jerry_char_t *message, /**< the error message */
                            const jerry_char_t *module_name) /**< the module_name */
{
  jerry_value_t ret = jerry_create_error (error_type, message);
  jerry_value_t property_name = jerry_create_string (module_name_property_name);
  jerry_value_t property_value = jerry_create_string_from_utf8 (module_name);
  jerry_release_value (jerry_set_property (ret, property_name, property_value));
  jerry_release_value (property_name);
  jerry_release_value (property_value);
  return ret;
} /* jerryx_module_create_error */

/**
 * Structure used for caching module instances
 */
typedef struct jerryx_module_instance
{
  struct jerryx_module_instance *next_p; /**< pointer to next item */
  jerry_char_t *name; /**< name of item */
  jerry_value_t exports; /**< The result of the module resolution */
} jerryx_module_instance_t;

/**
 * Detach an item from a list
 *
 * @instance - the item to detach
 */
#define JERRYX_MODULE_INSTANCE_UNLINK(instance) \
  ((jerryx_module_instance_t *) (instance))->next_p = NULL

/**
 *  Initialize a dynamically allocated item
 *
 * @instance - the item to initialize
 * @module_name - string representing the module name
 */
#define JERRYX_MODULE_INSTANCE_RUNTIME_INIT(instance, module_name) \
  ((jerryx_module_instance_t *) (instance))->name = (module_name); \
  JERRYX_MODULE_INSTANCE_UNLINK((instance))

/**
 * Insert an item into a linked list.
 */
static void
jerryx_module_instance_insert (jerryx_module_instance_t *instance_p, /**< item to insert */
                               jerryx_module_instance_t **root_pp) /**< pointer to root (as it may change) */
{
  if (*root_pp)
  {
    instance_p->next_p = (*root_pp);
  }
  (*root_pp) = instance_p;
} /* jerryx_module_instance_insert */

/*
 * Find an item in a linked list.
 *
 * @return the requested item, or NULL if not found
 */
static jerryx_module_instance_t *
jerryx_module_instance_find (jerryx_module_instance_t *root_p, /**< the root of the list */
                             const jerry_char_t *name) /**< the key item to find */
{
  for (;root_p != NULL; root_p = root_p->next_p)
  {
    if (!strcmp ((char *) name, ((char *) root_p->name)))
    {
      break;
    }
  }

  return root_p;
} /* jerryx_module_instance_find */

/**
 * Deeply delete a linked list.
 */
static void
jerryx_module_instances_free (jerryx_module_instance_t *root_p) /**< root of the list */
{
  jerryx_module_instance_t *next_p = NULL;
  while (root_p)
  {
    next_p = root_p->next_p;
    jerry_release_value (root_p->exports);
    jmem_heap_free_block (root_p, sizeof (jerryx_module_instance_t));
    root_p = next_p;
  }
} /* jerryx_module_instances_free */

/**
 * Deinitialize the module manager extension.
 */
static void
jerryx_module_manager_deinit (void *user_data_p) /**< context pointer to deinitialize */
{
  jerryx_module_instances_free (*((jerryx_module_instance_t **) (user_data_p)));
} /* jerryx_module_manager_deinit */

/**
 * Declare the context data manager for modules.
 */
static const jerry_context_data_manager_t jerryx_module_manager =
{
  .init_cb = NULL,
  .deinit_cb = jerryx_module_manager_deinit,
  .bytes_needed = sizeof (jerryx_module_instance_t *)
};

/**
 * Declare the linker section where module definitions are stored.
 */
JERRYX_SECTION_DECLARE (jxm_modules, jerryx_module_t)

/**
 * Declare and define the default module resolver - one which examines what modules are defined in the above linker
 * section and loads one that matches the requested name, caching the result for subsequent requests using the context
 * data mechanism.
 */
JERRYX_MODULE_RESOLVER (native_resolver)
{
  int index;
  jerry_value_t ret = 0;
  const jerryx_module_t *module_p = NULL;
  jerryx_module_instance_t *instance_p = NULL;
  jerryx_module_instance_t **instances_pp =
  (jerryx_module_instance_t **) jerry_get_context_data (&jerryx_module_manager);

  instance_p = jerryx_module_instance_find (*instances_pp, name);

  if (instance_p)
  {
    return instance_p->exports;
  }

  for JERRYX_SECTION_ITERATE (jxm_modules, index, module_p)
  {
    if (!strcmp ((char *) module_p->name, (char *) name))
    {
      if (module_p->on_resolve)
      {
        ret = module_p->on_resolve ();
        instance_p = (jerryx_module_instance_t *) jmem_heap_alloc_block (sizeof (jerryx_module_instance_t));
        JERRYX_MODULE_INSTANCE_RUNTIME_INIT (instance_p, module_p->name);
        instance_p->exports = jerry_acquire_value (ret);
        jerryx_module_instance_insert (instance_p, instances_pp);
      }
      else
      {
        ret = jerryx_module_create_error (JERRY_ERROR_TYPE, on_resolve_absent, name);
      }
      break;
    }
  }

  return ret;
} /* JERRYX_MODULE_RESOLVER */

/**
 * Declare the section where we expect to find module resolvers - at least the one above will be there.
 */
typedef jerry_value_t (*jerryx_module_resolver_t)(const jerry_char_t *);
JERRYX_SECTION_DECLARE (jxm_resolvers, jerryx_module_resolver_t)

/**
 * Resolve a single module using the module resolvers available in the section declared above and load it into the
 * current context.
 *
 * @return a jerry_value_t containing one of the followings:
 *   - the result of having loaded the module named @p name, or
 *   - the result of a previous successful load, or
 *   - an error indicating that something went wrong during the attempt to load the module.
 */
jerry_value_t
jerryx_module_resolve (const jerry_char_t *name) /**< name of the module to load */
{
  jerry_value_t ret;
  int index;
  const jerryx_module_resolver_t *resolver_p;

  for JERRYX_SECTION_ITERATE (jxm_resolvers, index, resolver_p)
  {
    ret = (**resolver_p) (name);
    if (ret)
    {
      return ret;
    }
  }

  return jerryx_module_create_error (JERRY_ERROR_COMMON, module_not_found, name);
} /* jerryx_module_resolve */


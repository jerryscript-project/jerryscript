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
#include "jerryscript.h"
#include "jerryscript-ext/module.h"

static const jerry_char_t *module_name_property_name = (jerry_char_t *) "moduleName";
static const jerry_char_t *module_not_found = (jerry_char_t *) "Module not found";
static const jerry_char_t *module_name_not_string = (jerry_char_t *) "Module name is not a string";

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
                            const jerry_value_t module_name) /**< the module name */
{
  jerry_value_t ret = jerry_create_error (error_type, message);
  jerry_value_t property_name = jerry_create_string (module_name_property_name);

  jerry_release_value (jerry_set_property (ret, property_name, module_name));
  jerry_release_value (property_name);
  return ret;
} /* jerryx_module_create_error */

/**
 * Initialize the module manager extension.
 */
static void
jerryx_module_manager_init (void *user_data_p)
{
  *((jerry_value_t *) user_data_p) = jerry_create_object ();
} /* jerryx_module_manager_init */

/**
 * Deinitialize the module manager extension.
 */
static void
jerryx_module_manager_deinit (void *user_data_p) /**< context pointer to deinitialize */
{
  jerry_release_value (*(jerry_value_t *) user_data_p);
} /* jerryx_module_manager_deinit */

/**
 * Declare the context data manager for modules.
 */
static const jerry_context_data_manager_t jerryx_module_manager =
{
  .init_cb = jerryx_module_manager_init,
  .deinit_cb = jerryx_module_manager_deinit,
  .bytes_needed = sizeof (jerry_value_t)
};

/**
 * Global static entry point to the linked list of available modules.
 */
static jerryx_native_module_t *first_module_p = NULL;

void jerryx_native_module_register (jerryx_native_module_t *module_p)
{
  module_p->next_p = first_module_p;
  first_module_p = module_p;
} /* jerryx_native_module_register */

void jerryx_native_module_unregister (jerryx_native_module_t *module_p)
{
  jerryx_native_module_t *parent_p = NULL, *iter_p = NULL;

  for (iter_p = first_module_p; iter_p != NULL; parent_p = iter_p, iter_p = iter_p->next_p)
  {
    if (iter_p == module_p)
    {
      if (parent_p)
      {
        parent_p->next_p = module_p->next_p;
      }
      else
      {
        first_module_p = module_p->next_p;
      }
      module_p->next_p = NULL;
    }
  }
} /* jerryx_native_module_unregister */

/**
 * Attempt to retrieve a module by name from a cache, and return false if not found.
 */
static bool
jerryx_module_check_cache (jerry_value_t cache, /**< cache from which to attempt to retrieve the module by name */
                           jerry_value_t module_name, /**< JerryScript string value holding the module name */
                           jerry_value_t *result) /**< Resulting value */
{
  bool ret = false;

  /* Check if the cache has the module. */
  jerry_value_t js_has_property = jerry_has_property (cache, module_name);

  /* If we succeed in getting an answer, we examine the answer. */
  if (!jerry_value_has_error_flag (js_has_property))
  {
    bool has_property = jerry_get_boolean_value (js_has_property);

    /* If the module is indeed in the cache, we return it. */
    if (has_property)
    {
      (*result) = jerry_get_property (cache, module_name);
      ret = true;
    }
  }

  jerry_release_value (js_has_property);

  return ret;
} /* jerryx_module_check_cache */

/**
 * Attempt to cache a loaded module.
 *
 * @return the module on success, otherwise the error encountered when attempting to cache. In the latter case, the
 * @p module is released.
 */
static jerry_value_t
jerryx_module_add_to_cache (jerry_value_t cache, /**< cache to which to add the module */
                            jerry_value_t module_name, /**< key at which to cache the module */
                            jerry_value_t module) /**< the module to cache */
{
  jerry_value_t ret = jerry_set_property (cache, module_name, module);

  if (jerry_value_has_error_flag (ret))
  {
    jerry_release_value (module);
  }
  else
  {
    jerry_release_value (ret);
    ret = module;
  }

  return ret;
} /* jerryx_module_add_to_cache */

static const jerry_char_t *on_resolve_absent = (jerry_char_t *) "Module on_resolve () must not be NULL";

/**
 * Declare and define the default module resolver - one which examines what modules are defined in the above linker
 * section and loads one that matches the requested name, caching the result for subsequent requests using the context
 * data mechanism.
 */
static bool
jerryx_resolve_native_module (const jerry_value_t canonical_name, /**< canonical name of the module */
                              jerry_value_t *result) /**< [out] where to put the resulting module instance */
{
  const jerryx_native_module_t *module_p = NULL;

  jerry_size_t name_size = jerry_get_utf8_string_size (canonical_name);
  jerry_char_t name_string[name_size];
  jerry_string_to_utf8_char_buffer (canonical_name, name_string, name_size);

  /* Look for the module by its name in the list of module definitions. */
  for (module_p = first_module_p; module_p != NULL; module_p = module_p->next_p)
  {
    if (module_p->name_p != NULL && !strncmp ((char *) module_p->name_p, (char *) name_string, name_size))
    {
      /* If we find the module by its name we load it and cache it if it has an on_resolve () and complain otherwise. */
      (*result) = ((module_p->on_resolve_p) ? module_p->on_resolve_p ()
                                            : jerryx_module_create_error (JERRY_ERROR_TYPE,
                                                                          on_resolve_absent,
                                                                          canonical_name));
      return true;
    }
  }

  return false;
} /* jerryx_resolve_native_module */

jerryx_module_resolver_t jerryx_module_native_resolver =
{
  .get_canonical_name_p = NULL,
  .resolve_p = jerryx_resolve_native_module
};

/**
 * Resolve a single module using the module resolvers available in the section declared above and load it into the
 * current context.
 *
 * @p name - name of the module to resolve
 * @p resolvers - list of resolvers to invoke
 * @p count - number of resolvers in the list
 *
 * @return a jerry_value_t containing one of the followings:
 *   - the result of having loaded the module named @p name, or
 *   - the result of a previous successful load, or
 *   - an error indicating that something went wrong during the attempt to load the module.
 */
jerry_value_t
jerryx_module_resolve (const jerry_value_t name, /**< name of the module to load */
                       const jerryx_module_resolver_t **resolvers_p, /**< list of resolvers */
                       size_t resolver_count) /**< number of resolvers in @p resolvers */
{
  size_t index;
  size_t canonical_names_used = 0;
  jerry_value_t ret;
  jerry_value_t instances;
  jerry_value_t canonical_names[resolver_count];
  jerry_value_t (*get_canonical_name_p) (const jerry_value_t name);
  bool (*resolve_p) (const jerry_value_t canonical_name,
                     jerry_value_t *result);

  if (!jerry_value_is_string (name))
  {
    ret = jerryx_module_create_error (JERRY_ERROR_COMMON, module_name_not_string, name);
    goto done;
  }

  instances = *(jerry_value_t *) jerry_get_context_data (&jerryx_module_manager);

  /**
   * Establish the canonical name for the requested module. Each resolver presents its own canonical name. If one of
   * the canonical names matches a cached module, it is returned as the result.
   */
  for (index = 0; index < resolver_count; index++)
  {
    get_canonical_name_p = (resolvers_p[index] == NULL ? NULL : resolvers_p[index]->get_canonical_name_p);
    canonical_names[index] = ((get_canonical_name_p == NULL) ? jerry_acquire_value (name)
                                                             : get_canonical_name_p (name));
    canonical_names_used++;
    if (jerryx_module_check_cache (instances, canonical_names[index], &ret))
    {
      goto done;
    }
  }

  /* Try each resolver until one manages to find the module. */
  for (index = 0; index < resolver_count; index++)
  {
    resolve_p = (resolvers_p[index] == NULL ? NULL : resolvers_p[index]->resolve_p);
    if (resolve_p != NULL && resolve_p (canonical_names[index], &ret))
    {
      if (!jerry_value_has_error_flag (ret))
      {
        ret = jerryx_module_add_to_cache (instances, canonical_names[index], ret);
      }
      goto done;
    }
  }

  /* If none of the resolvers manage to find the module, complain with "Module not found" */
  ret = jerryx_module_create_error (JERRY_ERROR_COMMON, module_not_found, name);

done:
  /* Release the canonical names as returned by the various resolvers. */
  for (index = 0; index < canonical_names_used; index++)
  {
    jerry_release_value (canonical_names[index]);
  }
  return ret;
} /* jerryx_module_resolve */

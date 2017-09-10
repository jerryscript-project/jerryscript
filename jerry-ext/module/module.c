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
                            const jerry_char_t *module_name) /**< the module name */
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
 * Declare the linker section where module definitions are stored.
 */
JERRYX_SECTION_DECLARE (jerryx_modules, jerryx_native_module_t)

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

#ifdef JERRYX_NATIVE_MODULES_SUPPORTED
static const jerry_char_t *on_resolve_absent = (jerry_char_t *) "Module on_resolve () must not be NULL";

/**
 * Declare and define the default module resolver - one which examines what modules are defined in the above linker
 * section and loads one that matches the requested name, caching the result for subsequent requests using the context
 * data mechanism.
 */
bool
jerryx_module_native_resolver (const jerry_char_t *name, /**< name of the module */
                               jerry_value_t *result) /**< [out] where to put the resulting module instance */
{
  int index;
  const jerryx_native_module_t *module_p = NULL;

  /* Look for the module by its name in the list of module definitions. */
  for (index = 0, module_p = &__start_jerryx_modules[0];
       &__start_jerryx_modules[index] < __stop_jerryx_modules;
       index++, module_p = &__start_jerryx_modules[index])
  {
    if (module_p->name != NULL && !strcmp ((char *) module_p->name, (char *) name))
    {
      /* If we find the module by its name we load it and cache it if it has an on_resolve () and complain otherwise. */
      (*result) = ((module_p->on_resolve) ? module_p->on_resolve ()
                                          : jerryx_module_create_error (JERRY_ERROR_TYPE, on_resolve_absent, name));
      return true;
    }
  }

  return false;
} /* jerryx_module_native_resolver */
#endif /* JERRYX_NATIVE_MODULES_SUPPORTED */

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
jerryx_module_resolve (const jerry_char_t *name, /**< name of the module to load */
                       const jerryx_module_resolver_t *resolvers_p, /**< list of resolvers */
                       size_t resolver_count) /**< number of resolvers in @p resolvers */
{
  size_t index;
  jerry_value_t ret;
  jerry_value_t instances = *(jerry_value_t *) jerry_get_context_data (&jerryx_module_manager);
  jerry_value_t module_name = jerry_create_string_from_utf8 (name);

  /* Return the cached instance if present. */
  if (jerryx_module_check_cache (instances, module_name, &ret))
  {
    goto done;
  }

  /* Try each resolver until one manages to find the module. */
  for (index = 0; index < resolver_count; index++)
  {
    if ((*resolvers_p[index]) (name, &ret))
    {
      if (!jerry_value_has_error_flag (ret))
      {
        ret = jerryx_module_add_to_cache (instances, module_name, ret);
      }
      goto done;
    }
  }

  /* If none of the resolvers manage to find the module, complain with "Module not found" */
  ret = jerryx_module_create_error (JERRY_ERROR_COMMON, module_not_found, name);

done:
  jerry_release_value (module_name);
  return ret;
} /* jerryx_module_resolve */

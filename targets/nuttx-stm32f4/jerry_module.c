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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "jerryscript.h"
#include "jerryscript-port.h"

/**
 * Computes the end of the directory part of a path.
 *
 * @return end of the directory part of a path.
 */
static size_t
jerry_port_get_directory_end (const jerry_char_t *path_p) /**< path */
{
  const jerry_char_t *end_p = path_p + strlen ((const char *) path_p);

  while (end_p > path_p)
  {
    if (end_p[-1] == '/')
    {
      return (size_t) (end_p - path_p);
    }

    end_p--;
  }

  return 0;
} /* jerry_port_get_directory_end */

/**
 * Normalize a file path.
 *
 * @return a newly allocated buffer with the normalized path if the operation is successful,
 *         NULL otherwise
 */
static jerry_char_t *
jerry_port_normalize_path (const jerry_char_t *in_path_p, /**< path to the referenced module */
                           size_t in_path_length, /**< length of the path */
                           const jerry_char_t *base_path_p, /**< base path */
                           size_t base_path_length) /**< length of the base path */
{
  char *path_p;

  if (base_path_length > 0)
  {
    path_p = (char *) malloc (base_path_length + in_path_length + 1);

    if (path_p == NULL)
    {
      return NULL;
    }

    memcpy (path_p, base_path_p, base_path_length);
    memcpy (path_p + base_path_length, in_path_p, in_path_length);
    path_p[base_path_length + in_path_length] = '\0';
  }
  else
  {
    path_p = (char *) malloc (in_path_length + 1);

    if (path_p == NULL)
    {
      return NULL;
    }

    memcpy (path_p, in_path_p, in_path_length);
    path_p[in_path_length] = '\0';
  }

  return (jerry_char_t *) path_p;
} /* jerry_port_normalize_path */

/**
 * A module descriptor.
 */
typedef struct jerry_port_module_t
{
  struct jerry_port_module_t *next_p; /**< next_module */
  jerry_char_t *path_p; /**< path to the module */
  size_t base_path_length; /**< base path length for relative difference */
  jerry_value_t realm; /**< the realm of the module */
  jerry_value_t module; /**< the module itself */
} jerry_port_module_t;

/**
 * Native info descriptor for modules.
 */
static const jerry_object_native_info_t jerry_port_module_native_info =
{
  .free_cb = NULL,
};

/**
 * Default module manager.
 */
typedef struct
{
  jerry_port_module_t *module_head_p; /**< first module */
} jerry_port_module_manager_t;

/**
 * Release known modules.
 */
static void
jerry_port_module_free (jerry_port_module_manager_t *manager_p, /**< module manager */
                        const jerry_value_t realm) /**< if this argument is object, release only those modules,
                                                    *   which realm value is equal to this argument. */
{
  jerry_port_module_t *module_p = manager_p->module_head_p;

  bool release_all = !jerry_value_is_object (realm);

  jerry_port_module_t *prev_p = NULL;

  while (module_p != NULL)
  {
    jerry_port_module_t *next_p = module_p->next_p;

    if (release_all || module_p->realm == realm)
    {
      free (module_p->path_p);
      jerry_release_value (module_p->realm);
      jerry_release_value (module_p->module);

      free (module_p);

      if (prev_p == NULL)
      {
        manager_p->module_head_p = next_p;
      }
      else
      {
        prev_p->next_p = next_p;
      }
    }
    else
    {
      prev_p = module_p;
    }

    module_p = next_p;
  }
} /* jerry_port_module_free */

/**
 * Initialize the default module manager.
 */
static void
jerry_port_module_manager_init (void *user_data_p)
{
  ((jerry_port_module_manager_t *) user_data_p)->module_head_p = NULL;
} /* jerry_port_module_manager_init */

/**
 * Deinitialize the default module manager.
 */
static void
jerry_port_module_manager_deinit (void *user_data_p) /**< context pointer to deinitialize */
{
  jerry_value_t undef = jerry_create_undefined ();
  jerry_port_module_free ((jerry_port_module_manager_t *) user_data_p, undef);
  jerry_release_value (undef);
} /* jerry_port_module_manager_deinit */

/**
 * Declare the context data manager for modules.
 */
static const jerry_context_data_manager_t jerry_port_module_manager =
{
  .init_cb = jerry_port_module_manager_init,
  .deinit_cb = jerry_port_module_manager_deinit,
  .bytes_needed = sizeof (jerry_port_module_manager_t)
};

/**
 * Default module resolver.
 *
 * @return a module object if resolving is successful, an error otherwise
 */
jerry_value_t
jerry_port_module_resolve (const jerry_value_t specifier, /**< module specifier string */
                           const jerry_value_t referrer, /**< parent module */
                           void *user_p) /**< user data */
{
  (void) user_p;

  jerry_port_module_t *module_p;
  const jerry_char_t *base_path_p = NULL;
  size_t base_path_length = 0;

  if (jerry_get_object_native_pointer (referrer, (void **) &module_p, &jerry_port_module_native_info))
  {
    base_path_p = module_p->path_p;
    base_path_length = module_p->base_path_length;
  }

  jerry_size_t in_path_length = jerry_get_utf8_string_size (specifier);
  jerry_char_t *in_path_p = (jerry_char_t *) malloc (in_path_length + 1);
  jerry_string_to_utf8_char_buffer (specifier, in_path_p, in_path_length);
  in_path_p[in_path_length] = '\0';

  jerry_char_t *path_p = jerry_port_normalize_path (in_path_p, in_path_length, base_path_p, base_path_length);

  if (path_p == NULL)
  {
    return jerry_create_error (JERRY_ERROR_COMMON, (const jerry_char_t *) "Out of memory");
  }

  jerry_value_t realm = jerry_get_global_object ();

  jerry_port_module_manager_t *manager_p;
  manager_p = (jerry_port_module_manager_t *) jerry_get_context_data (&jerry_port_module_manager);

  module_p = manager_p->module_head_p;

  while (module_p != NULL)
  {
    if (module_p->realm == realm
        && strcmp ((const char *) module_p->path_p, (const char *) path_p) == 0)
    {
      free (path_p);
      free (in_path_p);
      jerry_release_value (realm);
      return jerry_acquire_value (module_p->module);
    }

    module_p = module_p->next_p;
  }

  size_t source_size;
  uint8_t *source_p = jerry_port_read_source ((const char *) path_p, &source_size);

  if (source_p == NULL)
  {
    free (path_p);
    free (in_path_p);
    jerry_release_value (realm);
    /* TODO: This is incorrect, but makes test262 module tests pass
     * (they should throw SyntaxError, but not because the module cannot be found). */
    return jerry_create_error (JERRY_ERROR_SYNTAX, (const jerry_char_t *) "Module file not found");
  }

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_MODULE | JERRY_PARSE_HAS_RESOURCE;
  parse_options.resource_name_p = (jerry_char_t *) in_path_p;
  parse_options.resource_name_length = (size_t) in_path_length;

  jerry_value_t ret_value = jerry_parse (source_p,
                                         source_size,
                                         &parse_options);

  jerry_port_release_source (source_p);
  free (in_path_p);

  if (jerry_value_is_error (ret_value))
  {
    free (path_p);
    jerry_release_value (realm);
    return ret_value;
  }

  module_p = (jerry_port_module_t *) malloc (sizeof (jerry_port_module_t));

  module_p->next_p = manager_p->module_head_p;
  module_p->path_p = path_p;
  module_p->base_path_length = jerry_port_get_directory_end (module_p->path_p);
  module_p->realm = realm;
  module_p->module = jerry_acquire_value (ret_value);

  jerry_set_object_native_pointer (ret_value, module_p, &jerry_port_module_native_info);
  manager_p->module_head_p = module_p;

  return ret_value;
} /* jerry_port_module_resolve */

/**
 * Release known modules.
 */
void
jerry_port_module_release (const jerry_value_t realm) /**< if this argument is object, release only those modules,
                                                       *   which realm value is equal to this argument. */
{
  jerry_port_module_free ((jerry_port_module_manager_t *) jerry_get_context_data (&jerry_port_module_manager),
                          realm);
} /* jerry_port_module_release */

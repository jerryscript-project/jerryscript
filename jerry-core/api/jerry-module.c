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

#include "jerryscript-core.h"
#include "jerryscript-port.h"

#include "ecma-errors.h"

#include "lit-magic-strings.h"
#include "lit-strings.h"

#if JERRY_MODULE_SYSTEM

/**
 * A module descriptor.
 */
typedef struct jerry_module_t
{
  struct jerry_module_t *next_p; /**< next_module */
  jerry_char_t *path_p; /**< path to the module */
  jerry_size_t basename_offset; /**< offset of the basename in the module path*/
  jerry_value_t realm; /**< the realm of the module */
  jerry_value_t module; /**< the module itself */
} jerry_module_t;

/**
 * Native info descriptor for modules.
 */
static const jerry_object_native_info_t jerry_module_native_info JERRY_ATTR_CONST_DATA = {
  .free_cb = NULL,
};

/**
 * Default module manager.
 */
typedef struct
{
  jerry_module_t *module_head_p; /**< first module */
} jerry_module_manager_t;

/**
 * Release known modules.
 */
static void
jerry_module_free (jerry_module_manager_t *manager_p, /**< module manager */
                   const jerry_value_t realm) /**< if this argument is object, release only those modules,
                                               *   which realm value is equal to this argument. */
{
  jerry_module_t *module_p = manager_p->module_head_p;

  bool release_all = !jerry_value_is_object (realm);

  jerry_module_t *prev_p = NULL;

  while (module_p != NULL)
  {
    jerry_module_t *next_p = module_p->next_p;

    if (release_all || module_p->realm == realm)
    {
      jerry_port_path_free (module_p->path_p);
      jerry_value_free (module_p->realm);
      jerry_value_free (module_p->module);

      jerry_heap_free (module_p, sizeof (jerry_module_t));

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
} /* jerry_module_free */

/**
 * Initialize the default module manager.
 */
static void
jerry_module_manager_init (void *user_data_p)
{
  ((jerry_module_manager_t *) user_data_p)->module_head_p = NULL;
} /* jerry_module_manager_init */

/**
 * Deinitialize the default module manager.
 */
static void
jerry_module_manager_deinit (void *user_data_p) /**< context pointer to deinitialize */
{
  jerry_value_t undef = jerry_undefined ();
  jerry_module_free ((jerry_module_manager_t *) user_data_p, undef);
  jerry_value_free (undef);
} /* jerry_module_manager_deinit */

/**
 * Declare the context data manager for modules.
 */
static const jerry_context_data_manager_t jerry_module_manager JERRY_ATTR_CONST_DATA = {
  .init_cb = jerry_module_manager_init,
  .deinit_cb = jerry_module_manager_deinit,
  .bytes_needed = sizeof (jerry_module_manager_t)
};

#endif /* JERRY_MODULE_SYSTEM */

/**
 * Default module resolver.
 *
 * @return a module object if resolving is successful, an error otherwise
 */
jerry_value_t
jerry_module_resolve (const jerry_value_t specifier, /**< module specifier string */
                      const jerry_value_t referrer, /**< parent module */
                      void *user_p) /**< user data */
{
#if JERRY_MODULE_SYSTEM
  JERRY_UNUSED (user_p);

  const jerry_char_t *directory_p = lit_get_magic_string_utf8 (LIT_MAGIC_STRING__EMPTY);
  jerry_size_t directory_size = lit_get_magic_string_size (LIT_MAGIC_STRING__EMPTY);

  jerry_module_t *module_p = jerry_object_get_native_ptr (referrer, &jerry_module_native_info);

  if (module_p != NULL)
  {
    directory_p = module_p->path_p;
    directory_size = module_p->basename_offset;
  }

  jerry_size_t specifier_size = jerry_string_size (specifier, JERRY_ENCODING_UTF8);
  jerry_size_t reference_size = directory_size + specifier_size;
  jerry_char_t *reference_path_p = jerry_heap_alloc (reference_size + 1);

  memcpy (reference_path_p, directory_p, directory_size);
  jerry_string_to_buffer (specifier, JERRY_ENCODING_UTF8, reference_path_p + directory_size, specifier_size);
  reference_path_p[reference_size] = '\0';

  jerry_char_t *path_p = jerry_port_path_normalize (reference_path_p, reference_size);
  jerry_heap_free (reference_path_p, reference_size + 1);

  if (path_p == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_SYNTAX, "Failed to resolve module");
  }

  jerry_value_t realm = jerry_current_realm ();

  jerry_module_manager_t *manager_p;
  manager_p = (jerry_module_manager_t *) jerry_context_data (&jerry_module_manager);

  module_p = manager_p->module_head_p;

  while (module_p != NULL)
  {
    if (module_p->realm == realm && strcmp ((const char *) module_p->path_p, (const char *) path_p) == 0)
    {
      jerry_value_free (realm);
      jerry_port_path_free (path_p);
      return jerry_value_copy (module_p->module);
    }

    module_p = module_p->next_p;
  }

  jerry_size_t source_size;
  jerry_char_t *source_p = jerry_port_source_read ((const char *) path_p, &source_size);

  if (source_p == NULL)
  {
    jerry_value_free (realm);
    jerry_port_path_free (path_p);

    return jerry_throw_sz (JERRY_ERROR_SYNTAX, "Module file not found");
  }

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_MODULE | JERRY_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name = jerry_value_copy (specifier);

  jerry_value_t ret_value = jerry_parse (source_p, source_size, &parse_options);
  jerry_value_free (parse_options.source_name);

  jerry_port_source_free (source_p);

  if (jerry_value_is_exception (ret_value))
  {
    jerry_port_path_free (path_p);
    jerry_value_free (realm);
    return ret_value;
  }

  module_p = (jerry_module_t *) jerry_heap_alloc (sizeof (jerry_module_t));

  module_p->next_p = manager_p->module_head_p;
  module_p->path_p = path_p;
  module_p->basename_offset = jerry_port_path_base (module_p->path_p);
  module_p->realm = realm;
  module_p->module = jerry_value_copy (ret_value);

  jerry_object_set_native_ptr (ret_value, &jerry_module_native_info, module_p);
  manager_p->module_head_p = module_p;

  return ret_value;
#else /* JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (specifier);
  JERRY_UNUSED (referrer);
  JERRY_UNUSED (user_p);

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_module_resolve */

/**
 * Release known modules.
 */
void
jerry_module_cleanup (const jerry_value_t realm) /**< if this argument is object, release only those modules,
                                                  *   whose realm value is equal to this argument. */
{
#if JERRY_MODULE_SYSTEM
  jerry_module_free ((jerry_module_manager_t *) jerry_context_data (&jerry_module_manager), realm);
#else /* JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (realm);
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_module_cleanup */

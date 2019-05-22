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

#ifndef JERRYX_MODULE_H
#define JERRYX_MODULE_H

#include "jerryscript.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 * Declare the signature for the module initialization function.
 */
typedef jerry_value_t (*jerryx_native_module_on_resolve_t) (void);

/**
 * Declare the structure used to define a module. One should only make use of this structure via the
 * JERRYX_NATIVE_MODULE macro declared below.
 */
typedef struct jerryx_native_module_t
{
  const jerry_char_t *name_p; /**< name of the module */
  const jerryx_native_module_on_resolve_t on_resolve_p; /**< function that returns a new instance of the module */
  struct jerryx_native_module_t *next_p; /**< pointer to next module in the list */
} jerryx_native_module_t;

/**
 * Declare the constructor and destructor attributes. These evaluate to nothing if this extension is built without
 * library constructor/destructor support.
 */
#ifdef ENABLE_INIT_FINI
#define JERRYX_MODULE_CONSTRUCTOR_ATTRIBUTE __attribute__((constructor))
#define JERRYX_MODULE_DESTRUCTOR_ATTRIBUTE __attribute__((destructor))
#define JERRYX_MODULE_REGISTRATION_QUALIFIER static
#else /* !ENABLE_INIT_FINI */
#define JERRYX_MODULE_CONSTRUCTOR_ATTRIBUTE
#define JERRYX_MODULE_DESTRUCTOR_ATTRIBUTE
#define JERRYX_MODULE_REGISTRATION_QUALIFIER
#endif /* ENABLE_INIT_FINI */

/**
 * Having two levels of macros allows strings to be used unquoted.
 */
#define JERRYX_NATIVE_MODULE(module_name, on_resolve_cb)  \
  JERRYX_NATIVE_MODULE_IMPLEM(module_name, on_resolve_cb)

/**
 * @brief  Defines the symbol name. This is used in any place where the generated symbol name
 *         might be needed.
 * 
 * @param module_name: Name of the module.
 * @returns: A concatenated identifier, representing the symbol used for the native module's structure.
 */
#define JERRYX_NATIVE_MODULE_SYMBOL(module_name) \
  _ ## module_name ## _definition

/**
 * @brief  Generates a structure of `jerryx_native_module_t`.
 * 
 * @param module_name: Name of the module.
 * @param on_resolve_cb: The function to be called once the module is resolved.
 * @returns: A generated structure, representing a native module.
 * 
 * @see: jerryx_native_module_register, jerryx_native_module_unregister
 * 
 * @note: `const` is used to indicate that this structure shall not change during runtime.
 */
#define JERRYX_NATIVE_MODULE_STRUCTURE(module_name, on_resolve_cb)    \
  const jerryx_native_module_t _ ## module_name ## _definition =      \
  {                                                                   \
    .name_p = (jerry_char_t *) #module_name,                          \
    .on_resolve_p = (on_resolve_cb),                                  \
    .next_p = NULL                                                    \
  };

/**
 * @brief  Generates the definition AND function body for the initial constructor.
 * 
 * @param module_name: Name of the module.
 * @returns: The generated, initial constructor.
 * 
 * @note: When compiled with ENABLE_INIT_FINI, you may - dependent on platform - enable
 *        the initial constructuion when this module has been loaded as a shared object
 *        or library. However, this does **not** apply to statically "compiled in" modules.
 *        If you want to bootstrap a module upon load, use the `on_resolve_cb`.
 * 
 * @see: jerryx_native_module_t, jerryx_native_module_register
 */
#define JERRYX_NATIVE_MODULE_REGISTER_FUNC(module_name)                  \
  JERRYX_MODULE_REGISTRATION_QUALIFIER void                            \
  module_name ## _register (void) JERRYX_MODULE_CONSTRUCTOR_ATTRIBUTE; \
  JERRYX_MODULE_REGISTRATION_QUALIFIER void                            \
  module_name ## _register (void)                                      \
  {                                                                    \
    jerryx_native_module_register(                                     \
        & JERRYX_NATIVE_MODULE_SYMBOL(module_name)                     \
    );                                                                 \
  }

/**
 * @brief  Generates the definition AND function body for the initial destructor.
 * 
 * @param module_name: Name of the module.
 * @returns: The definition and implementation of the initial destructor.
 * 
 * @see JERRYX_NATIVE_MODULE_REGISTER_FUNC
 */
#define JERRYX_NATIVE_MODULE_UNREGISTER_FUNC(module_name)                \
  JERRYX_MODULE_REGISTRATION_QUALIFIER void                            \
  module_name ## _register (void) JERRYX_MODULE_CONSTRUCTOR_ATTRIBUTE; \
  JERRYX_MODULE_REGISTRATION_QUALIFIER void                            \
  module_name ## _unregister (void)                                    \
  {                                                                    \
    jerryx_native_module_unregister(                                   \
        & JERRYX_NATIVE_MODULE_SYMBOL(module_name)                     \
    );                                                                 \
  }

/**
 * @brief  Generates the implementation for a native module.
 * 
 * @param module_name: Name of the native module.
 * @param on_resolve_cb: Callback to be called when the module is first loaded.
 * @returns: Structure, con- and destructor.
 * 
 * @note: To use the con- and destructor, make sure you define ENABLE_INIT_FINI.
 * 
 * @see: jerryx_native_module_register, jerryx_native_module_unregister,
 *       jerryx_native_module_t, JERRYX_NATIVE_MODULE_STRUCTURE,
 *       JERRYX_NATIVE_MODULE_REGISTER_FUNC, JERRYX_NATIVE_MODULE_UNREGISTER_FUNC
 */
#define JERRYX_NATIVE_MODULE_IMPLEM(module_name, on_resolve_cb)        \
  JERRYX_NATIVE_MODULE_STRUCTURE(module_name, on_resolve_cb)           \
  JERRYX_NATIVE_MODULE_REGISTER_FUNC(module_name)                           \
  JERRYX_NATIVE_MODULE_UNREGISTER_FUNC(module_name)


/**
 * @brief  Defines an external symbol. This may be useful when you want to use the module structure
 *         across multiple files (i.e.: building a list of built-in modules).
 * 
 * @param module_name: Name of the module.
 * @returns: An "extern const" definition of the symbol representing the structure.
 * 
 * @see JERRYX_NATIVE_MODULE_SYMBOL
 */
#define JERRYX_NATIVE_MODULE_EXTERN(module_name) \
  extern const jerryx_native_module_t JERRYX_NATIVE_MODULE_SYMBOL(module_name)

/**
 * Register a native module. This makes it available for loading via jerryx_module_resolve, when
 * jerryx_module_native_resolver is passed in as a possible resolver.
 */
void jerryx_native_module_register (jerryx_native_module_t *module_p);

/**
 * Unregister a native module. This removes the module from the list of available native modules, meaning that
 * subsequent calls to jerryx_module_resolve with jerryx_module_native_resolver will not be able to find it.
 */
void jerryx_native_module_unregister (jerryx_native_module_t *module_p);

/**
 * Declare the function pointer type for canonical name resolution.
 */
typedef jerry_value_t (*jerryx_module_get_canonical_name_t) (const jerry_value_t name); /**< The name for which to
                                                                                         *   compute the canonical
                                                                                         *   name */

/**
 * Declare the function pointer type for module resolution.
 */
typedef bool (*jerryx_module_resolve_t) (const jerry_value_t canonical_name, /**< The module's canonical name */
                                         jerry_value_t *result); /**< The resulting module, if the function returns
                                                                  *   true */

/**
 * Declare the structure for module resolvers.
 */
typedef struct
{
  jerryx_module_get_canonical_name_t get_canonical_name_p; /**< function pointer to establish the canonical name of a
                                                            *   module */
  jerryx_module_resolve_t resolve_p; /**< function pointer to resolve a module */
} jerryx_module_resolver_t;

/**
 * Declare the JerryScript module resolver so that it may be added to an array of jerryx_module_resolver_t items and
 * thus passed to jerryx_module_resolve.
 */
extern jerryx_module_resolver_t jerryx_module_native_resolver;

/**
 * Load a copy of a module into the current context using the provided module resolvers, or return one that was already
 * loaded if it is found.
 */
jerry_value_t jerryx_module_resolve (const jerry_value_t name,
                                     const jerryx_module_resolver_t **resolvers,
                                     size_t count);

/**
 * Delete a module from the cache or, if name has the JavaScript value of undefined, clear the entire cache.
 */
void jerryx_module_clear_cache (const jerry_value_t name,
                                const jerryx_module_resolver_t **resolvers,
                                size_t count);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !JERRYX_MODULE_H */

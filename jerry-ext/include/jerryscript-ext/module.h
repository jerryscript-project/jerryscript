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

#ifdef __GNUC__
#define JERRYX_NATIVE_MODULES_SUPPORTED
#endif /* __GNUC__ */

#ifdef JERRYX_NATIVE_MODULES_SUPPORTED
#include "jerryscript-ext/section.impl.h"

/**
 * Declare the signature for the module initialization function.
 */
typedef jerry_value_t (*jerryx_native_module_on_resolve_t) (void);

/**
 * Declare the structure used to define a module. One should only make use of this structure via the
 * JERRYX_NATIVE_MODULE macro declared below.
 */
typedef struct
{
  jerry_char_t *name; /**< name of the module */
  jerryx_native_module_on_resolve_t on_resolve; /**< function that returns a new instance of the module */
} jerryx_native_module_t;

/**
 * Declare a helper macro that expands to the declaration of a variable of type jerryx_native_module_t placed into the
 * specially-named linker section "jerryx_modules" where the JerryScript module resolver
 * jerryx_module_native_resolver () will look for it.
 */
#define JERRYX_NATIVE_MODULE(module_name, on_resolve_cb)                                 \
  static const jerryx_native_module_t _module JERRYX_SECTION_ATTRIBUTE(jerryx_modules) = \
  {                                                                                      \
    .name = ((jerry_char_t *) #module_name),                                             \
    .on_resolve = (on_resolve_cb)                                                        \
  };

/**
 * Declare the JerryScript module resolver so that it may be added to an array of jerryx_module_resolver_t items and
 * thus passed to jerryx_module_resolve.
 */
bool jerryx_module_native_resolver (const jerry_char_t *name, jerry_value_t *result);

#endif /* JERRYX_NATIVE_MODULES_SUPPORTED */

/**
 * Declare the function pointer type for module resolvers.
 */
typedef bool (*jerryx_module_resolver_t) (const jerry_char_t *name, jerry_value_t *result);

/**
 * Load a copy of a module into the current context using the provided module resolvers, or return one that was already
 * loaded if it is found.
 */
jerry_value_t jerryx_module_resolve (const jerry_char_t *name, const jerryx_module_resolver_t *resolvers, size_t count);

#endif /* !JERRYX_MODULE_H */

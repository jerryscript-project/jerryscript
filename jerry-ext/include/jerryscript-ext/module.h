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
#include "jerryscript-ext/section.impl.h"

/**
 * Structure to define a module
 */
typedef struct
{
  jerry_char_t *name; /**< name of the module */
  jerry_value_t (*on_resolve)(void); /**< function that returns a new instance of the module */
} jerryx_module_t;

/**
 * Load a copy of a module into the current context or return one that was already loaded if it is found.
 */
jerry_value_t jerryx_module_resolve (const jerry_char_t *name);

/**
 * Define a module. The section name is cryptic ("jxm") because it has to be less than 16 bytes on Mach-O.
 */
#define JERRYX_MODULE(module_name, on_resolve_cb)                        \
  static jerryx_module_t _module JERRYX_SECTION_ATTRIBUTE(jxm_modules) = \
  {                                                                      \
    .name = ((jerry_char_t *)#module_name),                              \
    .on_resolve = (on_resolve_cb)                                        \
  };

/**
 * Define a module resolver. The section name is cryptic ("jxm") because it has to be less than 16 bytes on Mach-O.
 */
#define JERRYX_MODULE_RESOLVER(cb_name)                                      \
  static jerry_value_t cb_name (const jerry_char_t *name);                   \
  static jerry_value_t (*__static_global_ ## cb_name) (const jerry_char_t *) \
  JERRYX_SECTION_ATTRIBUTE(jxm_resolvers) = cb_name;                         \
  static jerry_value_t cb_name (const jerry_char_t *name)

#endif /* !JERRYX_MODULE_H */

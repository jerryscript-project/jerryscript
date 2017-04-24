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

typedef struct
{
  jerry_char_t *name;
  jerry_value_t (*on_resolve)(void);
} jerryx_module_t;

/**
 * Function to create a module manager context
 */
void *jerryx_module_manager_init (void);

/**
 * Function to clean up a given module manager context
 */
void jerryx_module_manager_deinit (void *context_p);

/**
 * Function to load a copy of a module into the current context
 */
jerry_value_t jerryx_module_resolve (const jerry_char_t *name);

#define JERRYX_MODULE(module_name, on_resolve_cb)                      \
static jerryx_module_t _module \
__attribute__((used, section("jerryx_modules"), aligned(sizeof(void *)))) = \
{ \
  .name = ((jerry_char_t *)#module_name), \
  .on_resolve = (on_resolve_cb) \
};

#define JERRYX_MODULE_RESOLVER(cb_name)                                              \
static jerry_value_t cb_name (const jerry_char_t *name);                             \
static jerry_value_t (*__static_global_ ## cb_name) (const jerry_char_t *)           \
__attribute__((used)) __attribute__((section("jerryx_module_resolvers"))) = cb_name; \
static jerry_value_t cb_name (const jerry_char_t *name)

#endif /* !JERRYX_MODULE_H */

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
#define EXTERN_C_START extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C_START
#define EXTERN_C_END
#endif

#define JERRYX_C_CTOR(fn)                          \
static void fn(void) __attribute__((constructor)); \
static void fn(void)

#define JERRYX_MODULE_VERSION 1

typedef struct jerryx_module_header
{
  jerry_char_t *name;
  struct jerryx_module_header *next_p;
} jerryx_module_header_t;

#define JERRYX_MODULE_HEADER_STATIC_INIT(module_name) \
  {                                                 \
    .name = (module_name),                          \
    .next_p = NULL                                  \
  }

#define JERRYX_MODULE_HEADER_UNLINK(header) \
  ((jerryx_module_header_t *) (header))->next_p = NULL

#define JERRYX_MODULE_HEADER_RUNTIME_INIT(header, module_name) \
  ((jerryx_module_header_t *) (header))->name = (module_name); \
  JERRYX_MODULE_HEADER_UNLINK((header))

typedef struct
{
  jerryx_module_header_t header;
  int version;
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
 * Function to register a module
 */
void jerryx_module_register (jerryx_module_t *module_p);

/**
 * Function to unregister a module
 */
void jerryx_module_unregister (jerryx_module_t *module_p);

/**
 * Function to load a copy of a module into the current context
 */
jerry_value_t jerryx_module_resolve (const jerry_char_t *name);

#define JERRYX_MODULE(name, on_resolve_cb)                             \
EXTERN_C_START                                                         \
static jerryx_module_t _module =                                       \
{                                                                      \
  .header = JERRYX_MODULE_HEADER_STATIC_INIT ((jerry_char_t *) #name), \
  .version = JERRYX_MODULE_VERSION,                                    \
  .on_resolve = (on_resolve_cb)                                        \
};                                                                     \
JERRYX_C_CTOR(_register_ ## name)                                      \
{                                                                      \
  jerryx_module_register(&_module);                                    \
}                                                                      \
EXTERN_C_END

#define JERRYX_MODULE_RESOLVER(cb_name)                                              \
static jerry_value_t cb_name (const jerry_char_t *name);                             \
static jerry_value_t (*__static_global_ ## cb_name) (const jerry_char_t *)           \
__attribute__((used)) __attribute__((section("jerryx_module_resolvers"))) = cb_name; \
static jerry_value_t cb_name (const jerry_char_t *name)

#endif /* !JERRYX_MODULE_H */

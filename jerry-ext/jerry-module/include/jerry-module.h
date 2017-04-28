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

typedef struct jerryx_module_link
{
  jerry_char_t *name;
  struct jerryx_module_link *next;
} jerryx_module_header_t;

#define JERRYX_MODULE_LINK_STATIC_INIT(name) \
  {name, NULL}

#define JERRYX_MODULE_LINK_FLOAT(link) \
  ((jerryx_module_header_t *) link)->next = NULL

#define JERRYX_MODULE_LINK_RUNTIME_INIT(link, module_name) \
  ((jerryx_module_header_t *) link)->name = (module_name); \
  JERRYX_MODULE_LINK_FLOAT((link))

typedef struct
{
  jerryx_module_header_t link;
  int version;
  jerry_value_t (*init)(void);
} jerryx_module_t;

/**
 * Function to create a module manager context
 */
void *jerryx_module_context_init (void);

/**
 * Function to clean up a given module manager context
 */
void jerryx_module_context_deinit (void *context_p);

/**
 * Function to register a module
 */
void jerryx_module_register (jerryx_module_t *module);

/**
 * Function to unregister a module
 */
void jerryx_module_unregister (jerryx_module_t *module);

/**
 * Function to load a copy of a module into the current context
 */
jerry_value_t jerryx_module_load (const jerry_char_t *name);

#define JERRYX_MODULE(name, init)                         \
EXTERN_C_START                                            \
static jerryx_module_t _module =                          \
{                                                         \
  JERRYX_MODULE_LINK_STATIC_INIT((jerry_char_t *) #name), \
  JERRYX_MODULE_VERSION,                                  \
  (init)                                                  \
};                                                        \
JERRYX_C_CTOR(_register_ ## name)                         \
{                                                         \
  jerryx_module_register(&_module);                       \
}                                                         \
EXTERN_C_END

#endif /* !JERRYX_MODULE_H */

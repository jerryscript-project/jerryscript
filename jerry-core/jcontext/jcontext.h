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

/*
 * Engine context for JerryScript
 */
#ifndef JCONTEXT_H
#define JCONTEXT_H

#include "debugger.h"
#include "ecma-builtins.h"
#include "ecma-jobqueue.h"
#include "jerryscript-port.h"
#include "jmem.h"
#include "re-bytecode.h"
#include "vm-defines.h"
#include "jerryscript.h"
#include "jerryscript-debugger-transport.h"
#include "js-parser-internal.h"

/** \addtogroup context Context
 * @{
 */

/**
 * Advanced allocator configurations.
 */
/**
 * Maximum global heap size in bytes
 */
#define CONFIG_MEM_HEAP_SIZE (JERRY_GLOBAL_HEAP_SIZE * 1024)

/**
 * Maximum stack usage size in bytes
 */
#define CONFIG_MEM_STACK_LIMIT (JERRY_STACK_LIMIT * 1024)

/**
 * Max heap usage limit
 */
#define CONFIG_MAX_GC_LIMIT 8192

/**
 * Allowed heap usage limit until next garbage collection
 *
 * Whenever the total allocated memory size reaches the current heap limit, garbage collection will be triggered
 * to try and reduce clutter from unreachable objects. If the allocated memory can't be reduced below the limit,
 * then the current limit will be incremented by CONFIG_MEM_HEAP_LIMIT.
 */
#if defined (JERRY_GC_LIMIT) && (JERRY_GC_LIMIT != 0)
#define CONFIG_GC_LIMIT JERRY_GC_LIMIT
#else
#define CONFIG_GC_LIMIT (JERRY_MIN (CONFIG_MEM_HEAP_SIZE / 32, CONFIG_MAX_GC_LIMIT))
#endif

/**
 * Amount of newly allocated objects since the last GC run, represented as a fraction of all allocated objects,
 * which when reached will trigger garbage collection to run with a low pressure setting.
 *
 * The fraction is calculated as:
 *                1.0 / CONFIG_ECMA_GC_NEW_OBJECTS_FRACTION
 */
#define CONFIG_ECMA_GC_NEW_OBJECTS_FRACTION (16)

#if !ENABLED (JERRY_SYSTEM_ALLOCATOR)
/**
 * Heap structure
 *
 * Memory blocks returned by the allocator must not start from the
 * beginning of the heap area because offset 0 is reserved for
 * JMEM_CP_NULL. This special constant is used in several places,
 * e.g. it marks the end of the property chain list, so it cannot
 * be eliminated from the project. Although the allocator cannot
 * use the first 8 bytes of the heap, nothing prevents to use it
 * for other purposes. Currently the free region start is stored
 * there.
 */
typedef struct jmem_heap_t jmem_heap_t;
#endif /* !ENABLED (JERRY_SYSTEM_ALLOCATOR) */

/**
 * User context item
 */
typedef struct jerry_context_data_header
{
  struct jerry_context_data_header *next_p; /**< pointer to next context item */
  const jerry_context_data_manager_t *manager_p; /**< manager responsible for deleting this item */
} jerry_context_data_header_t;

#define JERRY_CONTEXT_DATA_HEADER_USER_DATA(item_p) \
  ((uint8_t *) (item_p + 1))

/**
 * First non-external member of the jerry context
 */
#define JERRY_CONTEXT_FIRST_MEMBER ecma_builtin_objects

/**
 * JerryScript context
 *
 * The purpose of this header is storing
 * all global variables for Jerry
 */
struct jerry_context_t
{
  /* The value of external context members must be preserved across initializations and cleanups. */
#if ENABLED (JERRY_EXTERNAL_CONTEXT)
#if !ENABLED (JERRY_SYSTEM_ALLOCATOR)
  jmem_heap_t *heap_p; /**< point to the heap aligned to JMEM_ALIGNMENT. */
  uint32_t heap_size; /**< size of the heap */
#endif /* !ENABLED (JERRY_SYSTEM_ALLOCATOR) */
#endif /* ENABLED (JERRY_EXTERNAL_CONTEXT) */

  /* Update JERRY_CONTEXT_FIRST_MEMBER if the first non-external member changes */
  jmem_cpointer_t ecma_builtin_objects[ECMA_BUILTIN_ID__COUNT]; /**< pointer to instances of built-in objects */
#if ENABLED (JERRY_BUILTIN_REGEXP)
  const re_compiled_code_t *re_cache[RE_CACHE_SIZE]; /**< regex cache */
#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */
  jmem_cpointer_t ecma_gc_objects_cp; /**< List of currently alive objects. */
  jmem_heap_free_t *jmem_heap_list_skip_p; /**< This is used to speed up deallocation. */
  jmem_pools_chunk_t *jmem_free_8_byte_chunk_p; /**< list of free eight byte pool chunks */
#if ENABLED (JERRY_CPOINTER_32_BIT)
  jmem_pools_chunk_t *jmem_free_16_byte_chunk_p; /**< list of free sixteen byte pool chunks */
#endif /* ENABLED (JERRY_CPOINTER_32_BIT) */
  const lit_utf8_byte_t * const *lit_magic_string_ex_array; /**< array of external magic strings */
  const lit_utf8_size_t *lit_magic_string_ex_sizes; /**< external magic string lengths */
  jmem_cpointer_t string_list_first_cp; /**< first item of the literal string list */
#if ENABLED (JERRY_ES2015)
  jmem_cpointer_t symbol_list_first_cp; /**< first item of the global symbol list */
#endif /* ENABLED (JERRY_ES2015) */
  jmem_cpointer_t number_list_first_cp; /**< first item of the literal number list */
  jmem_cpointer_t ecma_global_lex_env_cp; /**< global lexical environment */

#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
  ecma_module_t *ecma_modules_p; /**< list of referenced modules */
  ecma_module_context_t *module_top_context_p; /**< top (current) module parser context */
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */

  vm_frame_ctx_t *vm_top_context_p; /**< top (current) interpreter context */
  jerry_context_data_header_t *context_data_p; /**< linked list of user-provided context-specific pointers */
  size_t ecma_gc_objects_number; /**< number of currently allocated objects */
  size_t ecma_gc_new_objects; /**< number of newly allocated objects since last GC session */
  size_t jmem_heap_allocated_size; /**< size of allocated regions */
  size_t jmem_heap_limit; /**< current limit of heap usage, that is upon being reached,
                           *   causes call of "try give memory back" callbacks */
  ecma_value_t error_value; /**< currently thrown error value */
  uint32_t lit_magic_string_ex_count; /**< external magic strings count */
  uint32_t jerry_init_flags; /**< run-time configuration flags */
  uint32_t status_flags; /**< run-time flags (the top 8 bits are used for passing class parsing options) */
#if (JERRY_GC_MARK_LIMIT != 0)
  uint32_t ecma_gc_mark_recursion_limit; /**< GC mark recursion limit */
#endif /* (JERRY_GC_MARK_LIMIT != 0) */

#if ENABLED (JERRY_PROPRETY_HASHMAP)
  uint8_t ecma_prop_hashmap_alloc_state; /**< property hashmap allocation state: 0-4,
                                          *   if !0 property hashmap allocation is disabled */
#endif /* ENABLED (JERRY_PROPRETY_HASHMAP) */

#if ENABLED (JERRY_BUILTIN_REGEXP)
  uint8_t re_cache_idx; /**< evicted item index when regex cache is full (round-robin) */
#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */

#if ENABLED (JERRY_ES2015_BUILTIN_PROMISE)
  ecma_job_queueitem_t *job_queue_head_p; /**< points to the head item of the jobqueue */
  ecma_job_queueitem_t *job_queue_tail_p; /**< points to the tail item of the jobqueue*/
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROMISE) */

#if ENABLED (JERRY_VM_EXEC_STOP)
  uint32_t vm_exec_stop_frequency; /**< reset value for vm_exec_stop_counter */
  uint32_t vm_exec_stop_counter; /**< down counter for reducing the calls of vm_exec_stop_cb */
  void *vm_exec_stop_user_p; /**< user pointer for vm_exec_stop_cb */
  ecma_vm_exec_stop_callback_t vm_exec_stop_cb; /**< user function which returns whether the
                                                 *   ECMAScript execution should be stopped */
#endif /* ENABLED (JERRY_VM_EXEC_STOP) */

#if (JERRY_STACK_LIMIT != 0)
  uintptr_t stack_base;  /**< stack base marker */
#endif /* (JERRY_STACK_LIMIT != 0) */

#if ENABLED (JERRY_DEBUGGER)
  uint8_t debugger_send_buffer[JERRY_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE]; /**< buffer for sending messages */
  uint8_t debugger_receive_buffer[JERRY_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE]; /**< buffer for receiving messages */
  jerry_debugger_transport_header_t *debugger_transport_header_p; /**< head of transport protocol chain */
  uint8_t *debugger_send_buffer_payload_p; /**< start where the outgoing message can be written */
  vm_frame_ctx_t *debugger_stop_context; /**< stop only if the current context is equal to this context */
  uint8_t *debugger_exception_byte_code_p; /**< Location of the currently executed byte code if an
                                            *   error occours while the vm_loop is suspended */
  jmem_cpointer_t debugger_byte_code_free_head; /**< head of byte code free linked list */
  jmem_cpointer_t debugger_byte_code_free_tail; /**< tail of byte code free linked list */
  uint32_t debugger_flags; /**< debugger flags */
  uint16_t debugger_received_length; /**< length of currently received bytes */
  uint8_t debugger_message_delay; /**< call receive message when reaches zero */
  uint8_t debugger_max_send_size; /**< maximum amount of data that can be sent */
  uint8_t debugger_max_receive_size; /**< maximum amount of data that can be received */
#endif /* ENABLED (JERRY_DEBUGGER) */

#if ENABLED (JERRY_LINE_INFO) || ENABLED (JERRY_ERROR_MESSAGES) || ENABLED (JERRY_ES2015_MODULE_SYSTEM)
  ecma_value_t resource_name; /**< resource name (usually a file name) */
#endif /* ENABLED (JERRY_LINE_INFO) || ENABLED (JERRY_ERROR_MESSAGES) || ENABLED (JERRY_ES2015_MODULE_SYSTEM) */

#if ENABLED (JERRY_MEM_STATS)
  jmem_heap_stats_t jmem_heap_stats; /**< heap's memory usage statistics */
#endif /* ENABLED (JERRY_MEM_STATS) */

  /* This must be at the end of the context for performance reasons */
#if ENABLED (JERRY_LCACHE)
  /** hash table for caching the last access of properties */
  ecma_lcache_hash_entry_t lcache[ECMA_LCACHE_HASH_ROWS_COUNT][ECMA_LCACHE_HASH_ROW_LENGTH];
#endif /* ENABLED (JERRY_LCACHE) */
};


#if ENABLED (JERRY_EXTERNAL_CONTEXT)

/*
 * This part is for JerryScript which uses external context.
 */

#define JERRY_CONTEXT(field) (jerry_port_get_current_context ()->field)

#if !ENABLED (JERRY_SYSTEM_ALLOCATOR)

#define JMEM_HEAP_SIZE (JERRY_CONTEXT (heap_size))

#define JMEM_HEAP_AREA_SIZE (JMEM_HEAP_SIZE - JMEM_ALIGNMENT)

struct jmem_heap_t
{
  jmem_heap_free_t first; /**< first node in free region list */
  uint8_t area[]; /**< heap area */
};

#define JERRY_HEAP_CONTEXT(field) (JERRY_CONTEXT (heap_p)->field)

#endif /* !ENABLED (JERRY_SYSTEM_ALLOCATOR) */

#else /* !ENABLED (JERRY_EXTERNAL_CONTEXT) */

/*
 * This part is for JerryScript which uses default context.
 */

/**
 * Global context.
 */
extern jerry_context_t jerry_global_context;

/**
 * Provides a reference to a field in the current context.
 */
#define JERRY_CONTEXT(field) (jerry_global_context.field)

#if !ENABLED (JERRY_SYSTEM_ALLOCATOR)

/**
* Size of heap
*/
#define JMEM_HEAP_SIZE ((size_t) (CONFIG_MEM_HEAP_SIZE))

/**
 * Calculate heap area size, leaving space for a pointer to the free list
 */
#define JMEM_HEAP_AREA_SIZE (JMEM_HEAP_SIZE - JMEM_ALIGNMENT)

struct jmem_heap_t
{
  jmem_heap_free_t first; /**< first node in free region list */
  uint8_t area[JMEM_HEAP_AREA_SIZE]; /**< heap area */
};

/**
 * Global heap.
 */
extern jmem_heap_t jerry_global_heap;

/**
 * Provides a reference to a field of the heap.
 */
#define JERRY_HEAP_CONTEXT(field) (jerry_global_heap.field)

#endif /* !ENABLED (JERRY_SYSTEM_ALLOCATOR) */

#endif /* ENABLED (JERRY_EXTERNAL_CONTEXT) */

/**
 * @}
 */

#endif /* !JCONTEXT_H */

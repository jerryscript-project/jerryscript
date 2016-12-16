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

/**
 * Allocator interface
 */
#ifndef JMEM_ALLOCATOR_H
#define JMEM_ALLOCATOR_H

#include "jrt.h"
#include "jmem-config.h"
#include "jmem-heap.h"
#include "jmem-poolman.h"

/** \addtogroup mem Memory allocation
 * @{
 */

/**
 * Compressed pointer representations
 *
 * 16 bit representation:
 *   The jmem_cpointer_t is defined as uint16_t
 *   and it can contain any sixteen bit value.
 *
 * 32 bit representation:
 *   The jmem_cpointer_t is defined as uint32_t.
 *   The lower JMEM_ALIGNMENT_LOG bits must be zero.
 *   The other bits can have any value.
 *
 * The 16 bit representation always encodes an offset from
 * a heap base. The 32 bit representation currently encodes
 * raw 32 bit JMEM_ALIGNMENT aligned pointers on 32 bit systems.
 * This can be extended to encode a 32 bit offset from a heap
 * base on 64 bit systems in the future. There are no plans
 * to support more than 4G address space for JerryScript.
 */

/**
 * Compressed pointer
 */
#ifdef JERRY_CPOINTER_32_BIT
typedef uint32_t jmem_cpointer_t;
#else /* !JERRY_CPOINTER_32_BIT */
typedef uint16_t jmem_cpointer_t;
#endif /* JERRY_CPOINTER_32_BIT */

/**
 * Width of compressed memory pointer
 */
#ifdef JERRY_CPOINTER_32_BIT
#define JMEM_CP_WIDTH 32
#else /* !JERRY_CPOINTER_32_BIT */
#define JMEM_CP_WIDTH 16
#endif /* JERRY_CPOINTER_32_BIT */

/**
 * Representation of NULL value for compressed pointers
 */
#define JMEM_CP_NULL ((jmem_cpointer_t) 0)

/**
 * Required alignment for allocated units/blocks
 */
#define JMEM_ALIGNMENT (1u << JMEM_ALIGNMENT_LOG)

/**
 * Severity of a 'try give memory back' request
 *
 * The request are posted sequentially beginning from
 * low to high until enough memory is freed.
 *
 * If not enough memory is freed upon a high request
 * then the engine is shut down with ERR_OUT_OF_MEMORY.
 */
typedef enum
{
  JMEM_FREE_UNUSED_MEMORY_SEVERITY_LOW, /* 'low' severity */
  JMEM_FREE_UNUSED_MEMORY_SEVERITY_HIGH, /* 'high' severity */
} jmem_free_unused_memory_severity_t;

/**
 *  Free region node
 */
typedef struct
{
  uint32_t next_offset; /* Offset of next region in list */
  uint32_t size; /* Size of region */
} jmem_heap_free_t;

/**
 * Node for free chunk list
 */
typedef struct jmem_pools_chunk_t
{
  struct jmem_pools_chunk_t *next_p; /* pointer to next pool chunk */
} jmem_pools_chunk_t;

/**
 * A free memory callback routine type.
 */
typedef void (*jmem_free_unused_memory_callback_t) (jmem_free_unused_memory_severity_t);

/**
 * Get value of pointer from specified non-null compressed pointer value
 */
#define JMEM_CP_GET_NON_NULL_POINTER(type, cp_value) \
  ((type *) (jmem_decompress_pointer (cp_value)))

/**
 * Get value of pointer from specified compressed pointer value
 */
#define JMEM_CP_GET_POINTER(type, cp_value) \
  (((unlikely ((cp_value) == JMEM_CP_NULL)) ? NULL : JMEM_CP_GET_NON_NULL_POINTER (type, cp_value)))

/**
 * Set value of non-null compressed pointer so that it will correspond
 * to specified non_compressed_pointer
 */
#define JMEM_CP_SET_NON_NULL_POINTER(cp_value, non_compressed_pointer) \
  (cp_value) = jmem_compress_pointer (non_compressed_pointer)

/**
 * Set value of compressed pointer so that it will correspond
 * to specified non_compressed_pointer
 */
#define JMEM_CP_SET_POINTER(cp_value, non_compressed_pointer) \
  do \
  { \
    void *ptr_value = (void *) non_compressed_pointer; \
    \
    if (unlikely ((ptr_value) == NULL)) \
    { \
      (cp_value) = JMEM_CP_NULL; \
    } \
    else \
    { \
      JMEM_CP_SET_NON_NULL_POINTER (cp_value, ptr_value); \
    } \
  } while (false);

void jmem_init (void);
void jmem_finalize (void);

jmem_cpointer_t jmem_compress_pointer (const void *pointer_p);
void *jmem_decompress_pointer (uintptr_t compressed_pointer);

void jmem_register_free_unused_memory_callback (jmem_free_unused_memory_callback_t callback);
void jmem_unregister_free_unused_memory_callback (jmem_free_unused_memory_callback_t callback);

#ifdef JMEM_STATS
void jmem_stats_reset_peak (void);
void jmem_stats_print (void);
#endif /* JMEM_STATS */

/**
 * @}
 */

#endif /* !JMEM_ALLOCATOR_H */

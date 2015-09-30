/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

/** \addtogroup mem Memory allocation
 * @{
 */

/**
 * Allocator interface
 */
#ifndef JERRY_MEM_ALLOCATOR_H
#define JERRY_MEM_ALLOCATOR_H

#include "jrt.h"
#include "mem-config.h"
#include "mem-heap.h"
#include "mem-poolman.h"

/**
 * Compressed pointer
 */
typedef uint16_t mem_cpointer_t;

/**
 * Representation of NULL value for compressed pointers
 */
#define MEM_CP_NULL ((mem_cpointer_t) 0)

/**
 * Required alignment for allocated units/blocks
 */
#define MEM_ALIGNMENT       (1 << MEM_ALIGNMENT_LOG)

/**
 * Width of compressed memory pointer
 */
#define MEM_CP_WIDTH (MEM_HEAP_OFFSET_LOG - MEM_ALIGNMENT_LOG)

/**
 * Compressed pointer value mask
 */
#define MEM_CP_MASK ((1ull << MEM_CP_WIDTH) - 1)

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
  MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_LOW, /* 'low' severity */
  MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_HIGH, /* 'high' severity */
} mem_try_give_memory_back_severity_t;

/**
 * A 'try give memory back' callback routine type.
 */
typedef void (*mem_try_give_memory_back_callback_t) (mem_try_give_memory_back_severity_t);

/**
 * Get value of pointer from specified non-null compressed pointer value
 */
#define MEM_CP_GET_NON_NULL_POINTER(type, cp_value) \
  (static_cast<type *> (mem_decompress_pointer (cp_value)))

/**
 * Get value of pointer from specified compressed pointer value
 */
#define MEM_CP_GET_POINTER(type, cp_value) \
  (((unlikely ((cp_value) == MEM_CP_NULL)) ? NULL : MEM_CP_GET_NON_NULL_POINTER (type, cp_value)))

/**
 * Set value of non-null compressed pointer so that it will correspond
 * to specified non_compressed_pointer
 */
#define MEM_CP_SET_NON_NULL_POINTER(cp_value, non_compressed_pointer) \
  (cp_value) = (mem_compress_pointer (non_compressed_pointer) & MEM_CP_MASK)

/**
 * Set value of compressed pointer so that it will correspond
 * to specified non_compressed_pointer
 */
#define MEM_CP_SET_POINTER(cp_value, non_compressed_pointer) \
  do \
  { \
    auto __temp_pointer = non_compressed_pointer; \
    non_compressed_pointer = __temp_pointer; \
  } while (0); \
  \
  if (unlikely ((non_compressed_pointer) == NULL)) \
  { \
    (cp_value) = MEM_CP_NULL; \
  } \
  else \
  { \
    MEM_CP_SET_NON_NULL_POINTER (cp_value, non_compressed_pointer); \
  }

extern void mem_init (void);
extern void mem_finalize (bool);

extern uintptr_t mem_compress_pointer (const void *);
extern void *mem_decompress_pointer (uintptr_t);

extern void mem_register_a_try_give_memory_back_callback (mem_try_give_memory_back_callback_t);
extern void mem_unregister_a_try_give_memory_back_callback (mem_try_give_memory_back_callback_t);

#ifdef MEM_STATS
extern void mem_stats_reset_peak (void);
extern void mem_stats_print (void);
#endif /* MEM_STATS */

#endif /* !JERRY_MEM_ALLOCATOR_H */

/**
 * @}
 */

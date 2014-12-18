/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#include "globals.h"
#include "mem-config.h"
#include "mem-heap.h"
#include "mem-poolman.h"

/**
 * Representation of NULL value for compressed pointers
 */
#define MEM_COMPRESSED_POINTER_NULL 0

/**
 * Required alignment for allocated units/blocks
 */
#define MEM_ALIGNMENT       (1 << MEM_ALIGNMENT_LOG)

/**
 * Width of compressed memory pointer
 */
#define MEM_COMPRESSED_POINTER_WIDTH (MEM_HEAP_OFFSET_LOG - MEM_ALIGNMENT_LOG)

/**
 * Severity of a 'try give memory back' request
 *
 * The request are posted sequentially beginning from
 * low to critical until enough memory is freed.
 *
 * If not enough memory is freed upon a critical request
 * then the engine is shut down with ERR_OUT_OF_MEMORY.
 */
typedef enum
{
  MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_LOW, /* 'low' severity */
  MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_MEDIUM, /* 'medium' severity */
  MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_HIGH, /* 'high' severity */
  MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_CRITICAL /* 'critical' severity */
} mem_try_give_memory_back_severity_t;

/**
 * A 'try give memory back' callback routine type.
 */
typedef void (*mem_try_give_memory_back_callback_t) (mem_try_give_memory_back_severity_t);

extern void mem_init (void);
extern void mem_finalize (bool is_show_mem_stats);

extern uintptr_t mem_compress_pointer (void *pointer);
extern void* mem_decompress_pointer (uintptr_t compressed_pointer);

extern void mem_register_a_try_give_memory_back_callback (mem_try_give_memory_back_callback_t callback);
extern void mem_unregister_a_try_give_memory_back_callback (mem_try_give_memory_back_callback_t callback);

#ifndef JERRY_NDEBUG
extern bool mem_is_heap_pointer (void *pointer);
#endif /* !JERRY_NDEBUG */

#endif /* !JERRY_MEM_ALLOCATOR_H */

/**
 * @}
 */

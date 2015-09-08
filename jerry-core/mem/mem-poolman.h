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

/** \addtogroup poolman Memory pool manager
 * @{
 */

/**
 * Pool manager interface
 */
#ifndef JERRY_MEM_POOLMAN_H
#define JERRY_MEM_POOLMAN_H

#include "jrt.h"

extern void mem_pools_init (void);
extern void mem_pools_finalize (void);
extern uint8_t *mem_pools_alloc (void);
extern void mem_pools_free (uint8_t *);
extern void mem_pools_collect_empty (void);

#ifdef MEM_STATS
/**
 * Pools' memory usage statistics
 */
typedef struct
{
  /** pools' count */
  size_t pools_count;

  /** peak pools' count */
  size_t peak_pools_count;

  /** non-resettable peak pools' count */
  size_t global_peak_pools_count;

  /** allocated chunks count */
  size_t allocated_chunks;

  /** peak allocated chunks count */
  size_t peak_allocated_chunks;

  /** non-resettable peak allocated chunks count */
  size_t global_peak_allocated_chunks;

  /** free chunks count */
  size_t free_chunks;
} mem_pools_stats_t;

extern void mem_pools_get_stats (mem_pools_stats_t *);
extern void mem_pools_stats_reset_peak (void);
#endif /* MEM_STATS */

#endif /* JERRY_MEM_POOLMAN_H */

/**
 * @}
 */

/**
 * @}
 */

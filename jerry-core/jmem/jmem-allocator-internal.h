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

#ifndef JMEM_ALLOCATOR_INTERNAL_H
#define JMEM_ALLOCATOR_INTERNAL_H

#ifndef JMEM_ALLOCATOR_INTERNAL
# error "The header is for internal routines of memory allocator component. Please, don't use the routines directly."
#endif /* !JMEM_ALLOCATOR_INTERNAL */

/** \addtogroup mem Memory allocation
 * @{
 */

/**
 * @{
 * Valgrind-related options and headers
 */
#ifdef JERRY_VALGRIND
# include "memcheck.h"

# define JMEM_VALGRIND_NOACCESS_SPACE(p, s)   VALGRIND_MAKE_MEM_NOACCESS((p), (s))
# define JMEM_VALGRIND_UNDEFINED_SPACE(p, s)  VALGRIND_MAKE_MEM_UNDEFINED((p), (s))
# define JMEM_VALGRIND_DEFINED_SPACE(p, s)    VALGRIND_MAKE_MEM_DEFINED((p), (s))
# define JMEM_VALGRIND_MALLOCLIKE_SPACE(p, s) VALGRIND_MALLOCLIKE_BLOCK((p), (s), 0, 0)
# define JMEM_VALGRIND_FREELIKE_SPACE(p)      VALGRIND_FREELIKE_BLOCK((p), 0)
#else /* !JERRY_VALGRIND */
# define JMEM_VALGRIND_NOACCESS_SPACE(p, s)
# define JMEM_VALGRIND_UNDEFINED_SPACE(p, s)
# define JMEM_VALGRIND_DEFINED_SPACE(p, s)
# define JMEM_VALGRIND_MALLOCLIKE_SPACE(p, s)
# define JMEM_VALGRIND_FREELIKE_SPACE(p)
#endif /* JERRY_VALGRIND */
/** @} */

#ifdef JMEM_STATS
void jmem_heap_stats_reset_peak (void);
void jmem_heap_stats_print (void);
#endif /* JMEM_STATS */

void jmem_heap_init (void);
void jmem_heap_finalize (void);
bool jmem_is_heap_pointer (const void *pointer);

void jmem_run_free_unused_memory_callbacks (jmem_free_unused_memory_severity_t severity);

/**
 * \addtogroup poolman Memory pool manager
 * @{
 */

void jmem_pools_finalize (void);
void jmem_pools_collect_empty (void);

/**
 * @}
 * @}
 */

#endif /* !JMEM_ALLOCATOR_INTERNAL_H */

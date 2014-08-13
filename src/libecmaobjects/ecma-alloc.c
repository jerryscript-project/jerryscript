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

/** \addtogroup ecma ---TODO---
 * @{
 *
 * \addtogroup ecmaalloc Routines for allocation/freeing memory for ECMA data types
 * @{
 */

/**
 * Implementation of routins for allocation/freeing memory for ECMA data types.
 *
 * All allocation routines from this module have the same structure:
 *  1. Try to allocate memory.
 *  2. If allocation was successful, return pointer to the allocated block.
 *  3. Run garbage collection.
 *  4. Try to allocate memory.
 *  5. If allocation was successful, return pointer to the allocated block;
 *     else - shutdown engine.
 */

#include "globals.h"
#include "ecma-alloc.h"
#include "ecma-globals.h"
#include "ecma-gc.h"
#include "mem-poolman.h"

JERRY_STATIC_ASSERT(sizeof (ecma_value_t) <= sizeof (uint16_t));
JERRY_STATIC_ASSERT(sizeof (ecma_property_t) <= sizeof (uint64_t));

FIXME(Pack ecma_object_t)
JERRY_STATIC_ASSERT(sizeof (ecma_object_t) <= 2 * sizeof (uint64_t));

JERRY_STATIC_ASSERT(sizeof (ecma_array_header_t) <= sizeof (uint32_t));
JERRY_STATIC_ASSERT(sizeof (ecma_array_first_chunk_t) == sizeof (uint64_t));
JERRY_STATIC_ASSERT(sizeof (ecma_array_non_first_chunk_t) == sizeof (uint64_t));
JERRY_STATIC_ASSERT(sizeof (ecma_string_t) == sizeof (uint64_t));
JERRY_STATIC_ASSERT(sizeof (ecma_completion_value_t) == sizeof (uint32_t));

/**
 * Template of an allocation routine.
 *
 * FIXME: Run GC only if allocation failed.
 */
#define ALLOC(ecma_type) ecma_ ## ecma_type ## _t * \
  ecma_alloc_ ## ecma_type (void) \
{ \
  ecma_ ## ecma_type ## _t *p ## ecma_type = (ecma_ ## ecma_type ## _t *) \
  mem_pools_alloc (); \
  \
  if (likely (p ## ecma_type != NULL)) \
  { \
    return p ## ecma_type; \
  } \
  \
  for (ecma_gc_gen_t gen_id = ECMA_GC_GEN_0; \
       gen_id < ECMA_GC_GEN_COUNT; \
       gen_id++) \
  { \
    ecma_gc_run (gen_id); \
    \
    p ## ecma_type = (ecma_ ## ecma_type ## _t *) \
    mem_pools_alloc (); \
    \
    if (likely (p ## ecma_type != NULL)) \
    { \
      return p ## ecma_type; \
    } \
  } \
  JERRY_UNREACHABLE(); \
}

/**
 * Deallocation routine template
 */
#define DEALLOC(ecma_type) void \
  ecma_dealloc_ ## ecma_type (ecma_ ## ecma_type ## _t *p ## ecma_type) \
{ \
  mem_pools_free ((uint8_t*) p ## ecma_type); \
}

/**
 * Declaration of alloc/free routine for specified ecma-type.
 */
#define DECLARE_ROUTINES_FOR(ecma_type) \
  ALLOC(ecma_type) \
  DEALLOC(ecma_type)

DECLARE_ROUTINES_FOR (object)
DECLARE_ROUTINES_FOR (property)
DECLARE_ROUTINES_FOR (number)
DECLARE_ROUTINES_FOR (array_first_chunk)
DECLARE_ROUTINES_FOR (array_non_first_chunk)

/**
 * @}
 * @}
 */

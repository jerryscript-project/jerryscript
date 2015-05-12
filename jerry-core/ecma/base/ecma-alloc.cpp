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

#include "ecma-alloc.h"
#include "ecma-globals.h"
#include "ecma-gc.h"
#include "ecma-lcache.h"
#include "jrt.h"
#include "mem-poolman.h"

JERRY_STATIC_ASSERT (sizeof (ecma_property_t) <= sizeof (uint64_t));

JERRY_STATIC_ASSERT (sizeof (ecma_object_t) <= sizeof (uint64_t));
JERRY_STATIC_ASSERT (ECMA_OBJECT_OBJ_TYPE_SIZE <= sizeof (uint64_t) * JERRY_BITSINBYTE);
JERRY_STATIC_ASSERT (ECMA_OBJECT_LEX_ENV_TYPE_SIZE <= sizeof (uint64_t) * JERRY_BITSINBYTE);

JERRY_STATIC_ASSERT (sizeof (ecma_collection_header_t) == sizeof (uint64_t));
JERRY_STATIC_ASSERT (sizeof (ecma_collection_chunk_t) == sizeof (uint64_t));
JERRY_STATIC_ASSERT (sizeof (ecma_string_t) == sizeof (uint64_t));
JERRY_STATIC_ASSERT (sizeof (ecma_completion_value_t) == sizeof (uint32_t));
JERRY_STATIC_ASSERT (sizeof (ecma_label_descriptor_t) == sizeof (uint64_t));
JERRY_STATIC_ASSERT (sizeof (ecma_getter_setter_pointers_t) <= sizeof (uint64_t));

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaalloc Routines for allocation/freeing memory for ECMA data types
 * @{
 */

/**
 * Implementation of routines for allocation/freeing memory for ECMA data types.
 *
 * All allocation routines from this module have the same structure:
 *  1. Try to allocate memory.
 *  2. If allocation was successful, return pointer to the allocated block.
 *  3. Run garbage collection.
 *  4. Try to allocate memory.
 *  5. If allocation was successful, return pointer to the allocated block;
 *     else - shutdown engine.
 */

/**
 * Template of an allocation routine.
 */
#define ALLOC(ecma_type) ecma_ ## ecma_type ## _t * \
  ecma_alloc_ ## ecma_type (void) \
{ \
  ecma_ ## ecma_type ## _t *p ## ecma_type = (ecma_ ## ecma_type ## _t *) mem_pools_alloc (); \
  \
  JERRY_ASSERT (p ## ecma_type != NULL); \
  \
  return p ## ecma_type; \
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
  ALLOC (ecma_type) \
  DEALLOC (ecma_type)

DECLARE_ROUTINES_FOR (object)
DECLARE_ROUTINES_FOR (property)
DECLARE_ROUTINES_FOR (number)
DECLARE_ROUTINES_FOR (collection_header)
DECLARE_ROUTINES_FOR (collection_chunk)
DECLARE_ROUTINES_FOR (string)
DECLARE_ROUTINES_FOR (label_descriptor)
DECLARE_ROUTINES_FOR (getter_setter_pointers)
DECLARE_ROUTINES_FOR (external_pointer)

/**
 * @}
 * @}
 */

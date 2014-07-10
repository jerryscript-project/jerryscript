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

JERRY_STATIC_ASSERT( sizeof (ecma_Value_t) <= sizeof (uint16_t) );
JERRY_STATIC_ASSERT( sizeof (ecma_Property_t) <= sizeof (uint64_t) );
JERRY_STATIC_ASSERT( sizeof (ecma_Object_t) <= sizeof (uint64_t) );
JERRY_STATIC_ASSERT( sizeof (ecma_ArrayHeader_t) <= sizeof (uint32_t) );
JERRY_STATIC_ASSERT( sizeof (ecma_ArrayFirstChunk_t) == ECMA_ARRAY_CHUNK_SIZE_IN_BYTES );
JERRY_STATIC_ASSERT( sizeof (ecma_ArrayNonFirstChunk_t) == ECMA_ARRAY_CHUNK_SIZE_IN_BYTES );
JERRY_STATIC_ASSERT( sizeof (ecma_CompletionValue_t) == sizeof(uint32_t) );

/**
 * Template of an allocation routine.
 */
#define ALLOC( ecmaType) ecma_ ## ecmaType ## _t * \
ecma_Alloc ## ecmaType (void) \
{ \
    ecma_ ## ecmaType ## _t *p ## ecmaType = (ecma_ ## ecmaType ## _t *) \
        mem_PoolsAlloc( mem_SizeToPoolChunkType( sizeof(ecma_ ## ecmaType ## _t))); \
    \
    ecma_GCRun(); \
    JERRY_ASSERT( p ## ecmaType != NULL ); \
    \
    return p ## ecmaType; \
}

/**
 * Free routine template
 */
#define FREE( ecmaType) void \
ecma_Free ## ecmaType( ecma_ ## ecmaType ## _t *p ## ecmaType) \
{ \
    mem_PoolsFree( mem_SizeToPoolChunkType( sizeof(ecma_ ## ecmaType ## _t)), \
                   (uint8_t*) p ## ecmaType); \
}

/**
 * Declaration of alloc/free routine for specified ecma-type.
 */
#define DECLARE_ROUTINES_FOR( ecmaType) \
    ALLOC( ecmaType) \
    FREE( ecmaType)

DECLARE_ROUTINES_FOR (Object)
DECLARE_ROUTINES_FOR (Property)
DECLARE_ROUTINES_FOR (Number)
DECLARE_ROUTINES_FOR (ArrayFirstChunk)
DECLARE_ROUTINES_FOR (ArrayNonFirstChunk)

/**
 * @}
 * @}
 */

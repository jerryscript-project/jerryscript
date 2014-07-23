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
 * \addtogroup ecmagc Garbage collector
 * @{
 */

/**
 * Garbage collector implementation
 */

#include "globals.h"
#include "ecma-alloc.h"
#include "ecma-globals.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"

/**
 * Queue of objects, awaiting for GC
 */
static ecma_object_t *ecma_gc_objs_to_free_queue;

/**
 * Queue object for GC.
 *
 * Warning:
 *          After this operation the object is not longer valid for general use.
 */
static void
ecma_gc_queue( ecma_object_t *pObject) /**< object */
{
    JERRY_ASSERT( pObject != NULL );
    JERRY_ASSERT( pObject->GCInfo.IsObjectValid );
    JERRY_ASSERT( pObject->GCInfo.u.Refs == 0 );

    pObject->GCInfo.IsObjectValid = false;
    ecma_set_pointer( pObject->GCInfo.u.NextQueuedForGC, ecma_gc_objs_to_free_queue);

    ecma_gc_objs_to_free_queue = pObject;
} /* ecma_gc_queue */

/**
 * Increase reference counter of an object
 */
void
ecma_ref_object(ecma_object_t *pObject) /**< object */
{
    JERRY_ASSERT(pObject->GCInfo.IsObjectValid);

    pObject->GCInfo.u.Refs++;

    /**
     * Check that value was not overflowed
     */
    JERRY_ASSERT(pObject->GCInfo.u.Refs > 0);
} /* ecma_ref_object */

/**
 * Decrease reference counter of an object
 */
void
ecma_deref_object(ecma_object_t *pObject) /**< object */
{
    JERRY_ASSERT(pObject != NULL);
    JERRY_ASSERT(pObject->GCInfo.IsObjectValid);
    JERRY_ASSERT(pObject->GCInfo.u.Refs > 0);

    pObject->GCInfo.u.Refs--;

    if ( pObject->GCInfo.u.Refs == 0 )
    {
        ecma_gc_queue( pObject);
    }
} /* ecma_deref_object */

/**
 * Initialize garbage collector
 */
void
ecma_gc_init( void)
{
    ecma_gc_objs_to_free_queue = NULL;
} /* ecma_gc_init */

/**
 * Run garbage collecting
 */
void
ecma_gc_run( void)
{
    while ( ecma_gc_objs_to_free_queue != NULL )
    {
        ecma_object_t *pObject = ecma_gc_objs_to_free_queue;
        ecma_gc_objs_to_free_queue = ecma_get_pointer( pObject->GCInfo.u.NextQueuedForGC);

        JERRY_ASSERT( !pObject->GCInfo.IsObjectValid );

        for ( ecma_property_t *property = ecma_get_pointer( pObject->pProperties), *pNextProperty;
              property != NULL;
              property = pNextProperty )
        {
            pNextProperty = ecma_get_pointer( property->pNextProperty);

            ecma_free_property( property);
        }

        if ( pObject->IsLexicalEnvironment )
        {
            ecma_object_t *pOuterLexicalEnvironment = ecma_get_pointer( pObject->u.LexicalEnvironment.pOuterReference);
            
            if ( pOuterLexicalEnvironment != NULL )
            {
                ecma_deref_object( pOuterLexicalEnvironment);
            }
        } else
        {
            ecma_object_t *pPrototypeObject = ecma_get_pointer( pObject->u.Object.pPrototypeObject);
            
            if ( pPrototypeObject != NULL )
            {
                ecma_deref_object( pPrototypeObject);
            }
        }
        
        ecma_dealloc_object( pObject);
    }
} /* ecma_gc_run */

/**
 * @}
 * @}
 */

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
static ecma_Object_t *ecma_GC_Queue;

/**
 * Queue object for GC.
 *
 * Warning:
 *          After this operation the object is not longer valid for general use.
 */
static void
ecma_GCQueue( ecma_Object_t *pObject) /**< object */
{
    JERRY_ASSERT( pObject != NULL );
    JERRY_ASSERT( pObject->GCInfo.IsObjectValid );
    JERRY_ASSERT( pObject->GCInfo.u.Refs == 0 );

    pObject->GCInfo.IsObjectValid = false;
    ecma_SetPointer( pObject->GCInfo.u.NextQueuedForGC, ecma_GC_Queue);

    ecma_GC_Queue = pObject;
} /* ecma_QueueGC */

/**
 * Increase reference counter of an object
 */
void
ecma_RefObject(ecma_Object_t *pObject) /**< object */
{
    JERRY_ASSERT(pObject->GCInfo.IsObjectValid);

    pObject->GCInfo.u.Refs++;

    /**
     * Check that value was not overflowed
     */
    JERRY_ASSERT(pObject->GCInfo.u.Refs > 0);
} /* ecma_RefObject */

/**
 * Decrease reference counter of an object
 */
void
ecma_DerefObject(ecma_Object_t *pObject) /**< object */
{
    JERRY_ASSERT(pObject != NULL);
    JERRY_ASSERT(pObject->GCInfo.IsObjectValid);
    JERRY_ASSERT(pObject->GCInfo.u.Refs > 0);

    pObject->GCInfo.u.Refs--;

    if ( pObject->GCInfo.u.Refs == 0 )
    {
        ecma_GCQueue( pObject);
    }
} /* ecma_DerefObject */

/**
 * Initialize garbage collector
 */
void
ecma_GCInit( void)
{
    ecma_GC_Queue = NULL;
} /* ecma_GCInit */

/**
 * Run garbage collecting
 */
void
ecma_GCRun( void)
{
    while ( ecma_GC_Queue != NULL )
    {
        ecma_Object_t *pObject = ecma_GC_Queue;
        ecma_GC_Queue = ecma_GetPointer( pObject->GCInfo.u.NextQueuedForGC);

        JERRY_ASSERT( !pObject->GCInfo.IsObjectValid );

        for ( ecma_Property_t *property = ecma_GetPointer( pObject->pProperties), *pNextProperty;
              property != NULL;
              property = pNextProperty )
        {
            pNextProperty = ecma_GetPointer( property->pNextProperty);

            ecma_FreeProperty( property);
        }

        if ( pObject->IsLexicalEnvironment )
        {
            ecma_Object_t *pOuterLexicalEnvironment = ecma_GetPointer( pObject->u.LexicalEnvironment.pOuterReference);
            
            if ( pOuterLexicalEnvironment != NULL )
            {
                ecma_DerefObject( pOuterLexicalEnvironment);
            }
        } else
        {
            ecma_Object_t *pPrototypeObject = ecma_GetPointer( pObject->u.Object.pPrototypeObject);
            
            if ( pPrototypeObject != NULL )
            {
                ecma_DerefObject( pPrototypeObject);
            }
        }
        
        ecma_DeallocObject( pObject);
    }
} /* ecma_RunGC */

/**
 * @}
 * @}
 */

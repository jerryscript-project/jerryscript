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
ecma_gc_queue( ecma_object_t *object_p) /**< object */
{
    JERRY_ASSERT( object_p != NULL );
    JERRY_ASSERT( object_p->GCInfo.is_object_valid );
    JERRY_ASSERT( object_p->GCInfo.u.refs == 0 );

    object_p->GCInfo.is_object_valid = false;
    ecma_set_pointer( object_p->GCInfo.u.next_queued_for_gc, ecma_gc_objs_to_free_queue);

    ecma_gc_objs_to_free_queue = object_p;
} /* ecma_gc_queue */

/**
 * Increase reference counter of an object
 */
void
ecma_ref_object(ecma_object_t *object_p) /**< object */
{
    JERRY_ASSERT(object_p->GCInfo.is_object_valid);

    object_p->GCInfo.u.refs++;

    /**
     * Check that value was not overflowed
     */
    JERRY_ASSERT(object_p->GCInfo.u.refs > 0);
} /* ecma_ref_object */

/**
 * Decrease reference counter of an object
 */
void
ecma_deref_object(ecma_object_t *object_p) /**< object */
{
    JERRY_ASSERT(object_p != NULL);
    JERRY_ASSERT(object_p->GCInfo.is_object_valid);
    JERRY_ASSERT(object_p->GCInfo.u.refs > 0);

    object_p->GCInfo.u.refs--;

    if ( object_p->GCInfo.u.refs == 0 )
    {
        ecma_gc_queue( object_p);
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
        ecma_object_t *object_p = ecma_gc_objs_to_free_queue;
        ecma_gc_objs_to_free_queue = ecma_get_pointer( object_p->GCInfo.u.next_queued_for_gc);

        JERRY_ASSERT( !object_p->GCInfo.is_object_valid );

        for ( ecma_property_t *property = ecma_get_pointer( object_p->properties_p), *next_property_p;
              property != NULL;
              property = next_property_p )
        {
            next_property_p = ecma_get_pointer( property->next_property_p);

            ecma_free_property( property);
        }

        if ( object_p->is_lexical_environment )
        {
            ecma_object_t *outer_lexical_environment_p = ecma_get_pointer( object_p->u.lexical_environment.outer_reference_p);
            
            if ( outer_lexical_environment_p != NULL )
            {
                ecma_deref_object( outer_lexical_environment_p);
            }
        } else
        {
            ecma_object_t *prototype_object_p = ecma_get_pointer( object_p->u.object.prototype_object_p);
            
            if ( prototype_object_p != NULL )
            {
                ecma_deref_object( prototype_object_p);
            }
        }
        
        ecma_dealloc_object( object_p);
    }
} /* ecma_gc_run */

/**
 * @}
 * @}
 */

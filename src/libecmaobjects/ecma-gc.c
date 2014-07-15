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
    JERRY_ASSERT( pObject->m_GCInfo.m_IsObjectValid );
    JERRY_ASSERT( pObject->m_GCInfo.u.m_Refs == 0 );

    pObject->m_GCInfo.m_IsObjectValid = false;
    ecma_SetPointer( pObject->m_GCInfo.u.m_NextQueuedForGC, ecma_GC_Queue);

    ecma_GC_Queue = pObject;
} /* ecma_QueueGC */

/**
 * Increase reference counter of an object
 */
void
ecma_RefObject(ecma_Object_t *pObject) /**< object */
{
    JERRY_ASSERT(pObject->m_GCInfo.m_IsObjectValid);

    pObject->m_GCInfo.u.m_Refs++;

    /**
     * Check that value was not overflowed
     */
    JERRY_ASSERT(pObject->m_GCInfo.u.m_Refs > 0);
} /* ecma_RefObject */

/**
 * Decrease reference counter of an object
 */
void
ecma_DerefObject(ecma_Object_t *pObject) /**< object */
{
    JERRY_ASSERT(pObject != NULL);
    JERRY_ASSERT(pObject->m_GCInfo.m_IsObjectValid);
    JERRY_ASSERT(pObject->m_GCInfo.u.m_Refs > 0);

    pObject->m_GCInfo.u.m_Refs--;

    if ( pObject->m_GCInfo.u.m_Refs == 0 )
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
 * Garbage collect described value
 */
static void
ecma_GCValue( ecma_Value_t valueDescription) /**< value description */
{
    switch ( (ecma_Type_t) valueDescription.m_ValueType )
    {
        case ECMA_TYPE_SIMPLE:
          {
            /* doesn't hold additional memory */
            break;
          }

        case ECMA_TYPE_NUMBER:
        {
            ecma_Number_t *pNumber = ecma_GetPointer( valueDescription.m_Value);
            ecma_FreeNumber( pNumber);
            break;
        }

        case ECMA_TYPE_STRING:
        {
            ecma_ArrayFirstChunk_t *pString = ecma_GetPointer( valueDescription.m_Value);
            ecma_FreeArray( pString);
            break;
        }

        case ECMA_TYPE_OBJECT:
        {
            ecma_DerefObject( ecma_GetPointer( valueDescription.m_Value));
            break;
        }

        case ECMA_TYPE__COUNT:
        {
            JERRY_UNREACHABLE();
        }
    }
} /* ecma_GCValue */

/**
 * Garbage collect a named data property
 */
static void
ecma_GCNamedDataProperty( ecma_Property_t *pProperty) /**< the property */
{
    JERRY_ASSERT( pProperty->m_Type == ECMA_PROPERTY_NAMEDDATA );

    ecma_FreeArray( ecma_GetPointer( pProperty->u.m_NamedDataProperty.m_pName));
    ecma_GCValue( pProperty->u.m_NamedDataProperty.m_Value);
} /* ecma_GCNamedDataProperty */

/**
 * Garbage collect a named accessor property
 */
static void
ecma_GCNamedAccessorProperty( ecma_Property_t *pProperty) /**< the property */
{
    JERRY_ASSERT( pProperty->m_Type == ECMA_PROPERTY_NAMEDACCESSOR );

    ecma_FreeArray( ecma_GetPointer( pProperty->u.m_NamedAccessorProperty.m_pName));

    ecma_Object_t *pGet = ecma_GetPointer(pProperty->u.m_NamedAccessorProperty.m_pGet);
    ecma_Object_t *pSet = ecma_GetPointer(pProperty->u.m_NamedAccessorProperty.m_pSet);

    if ( pGet != NULL )
    {
        ecma_DerefObject( pGet);
    }

    if ( pSet != NULL )
    {
        ecma_DerefObject( pSet);
    }
} /* ecma_GCNamedAccessorProperty */

/**
 * Garbage collect an internal property
 */
static void
ecma_GCInternalProperty( ecma_Property_t *pProperty) /**< the property */
{
    JERRY_ASSERT( pProperty->m_Type == ECMA_PROPERTY_INTERNAL );

    ecma_InternalPropertyId_t propertyId = pProperty->u.m_InternalProperty.m_InternalPropertyType;
    uint32_t propertyValue = pProperty->u.m_InternalProperty.m_Value;

    switch ( propertyId )
    {
        case ECMA_INTERNAL_PROPERTY_CLASS: /* a string */
        case ECMA_INTERNAL_PROPERTY_NUMBER_INDEXED_ARRAY_VALUES: /* an array */
        case ECMA_INTERNAL_PROPERTY_STRING_INDEXED_ARRAY_VALUES: /* an array */
        {
            ecma_FreeArray( ecma_GetPointer( propertyValue));
            break;
        }

        case ECMA_INTERNAL_PROPERTY_SCOPE: /* a lexical environment */
        case ECMA_INTERNAL_PROPERTY_BINDING_OBJECT: /* an object */
        {
            ecma_DerefObject( ecma_GetPointer( propertyValue));
            break;
        }

        case ECMA_INTERNAL_PROPERTY_PROTOTYPE: /* the property's value is located in ecma_Object_t */
        case ECMA_INTERNAL_PROPERTY_EXTENSIBLE: /* the property's value is located in ecma_Object_t */
        case ECMA_INTERNAL_PROPERTY_PROVIDE_THIS: /* a boolean flag */
        {
            break;
        }
    }
} /* ecma_GCInternalProperty */

/**
 * Run garbage collecting
 */
void
ecma_GCRun( void)
{
    while ( ecma_GC_Queue != NULL )
    {
        ecma_Object_t *pObject = ecma_GC_Queue;
        ecma_GC_Queue = ecma_GetPointer( pObject->m_GCInfo.u.m_NextQueuedForGC);

        JERRY_ASSERT( !pObject->m_GCInfo.m_IsObjectValid );

        for ( ecma_Property_t *property = ecma_GetPointer( pObject->m_pProperties), *pNextProperty;
              property != NULL;
              property = pNextProperty )
        {
            switch ( (ecma_PropertyType_t) property->m_Type )
            {
                case ECMA_PROPERTY_NAMEDDATA:
                {
                    ecma_GCNamedDataProperty( property);

                    break;
                }

                case ECMA_PROPERTY_NAMEDACCESSOR:
                {
                    ecma_GCNamedAccessorProperty( property);

                    break;
                }

                case ECMA_PROPERTY_INTERNAL:
                {
                    ecma_GCInternalProperty( property);

                    break;
                }
            }

            pNextProperty = ecma_GetPointer( property->m_pNextProperty);
            ecma_FreeProperty( property);
        }

        if ( pObject->m_IsLexicalEnvironment )
        {
            ecma_Object_t *pOuterLexicalEnvironment = ecma_GetPointer( pObject->u.m_LexicalEnvironment.m_pOuterReference);
            
            if ( pOuterLexicalEnvironment != NULL )
            {
                ecma_DerefObject( pOuterLexicalEnvironment);
            }
        } else
        {
            ecma_Object_t *pPrototypeObject = ecma_GetPointer( pObject->u.m_Object.m_pPrototypeObject);
            
            if ( pPrototypeObject != NULL )
            {
                ecma_DerefObject( pPrototypeObject);
            }
        }
        
        ecma_FreeObject( pObject);
    }
} /* ecma_RunGC */

/**
 * @}
 * @}
 */

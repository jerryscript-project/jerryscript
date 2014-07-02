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
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

/**
 * Implementation of helpers for operations with ECMA data types
 */

#include "ecma-alloc.h"
#include "ecma-defs.h"
#include "ecma-helpers.h"
#include "jerry-libc.h"

/**
 * Compress pointer.
 */
uintptr_t
ecma_CompressPointer(void *pointer) /**< pointer to compress */
{
    if ( pointer == NULL )
    {
        return ECMA_NULL_POINTER;
    }

    uintptr_t intPtr = (uintptr_t) pointer;

    JERRY_ASSERT(intPtr % MEM_ALIGNMENT == 0);

    intPtr -= mem_GetBasePointer();
    intPtr >>= MEM_ALIGNMENT_LOG;

    JERRY_ASSERT((intPtr & ~((1u << ECMA_POINTER_FIELD_WIDTH) - 1)) == 0);

    return intPtr;
} /* ecma_CompressPointer */

/**
 * Decompress pointer.
 */
void*
ecma_DecompressPointer(uintptr_t compressedPointer) /**< pointer to decompress */
{
    if ( compressedPointer == ECMA_NULL_POINTER )
    {
        return NULL;
    }

    uintptr_t intPtr = compressedPointer;

    intPtr <<= MEM_ALIGNMENT_LOG;
    intPtr += mem_GetBasePointer();

    return (void*) intPtr;
} /* ecma_DecompressPointer */

/**
 * Create an object with specified prototype object
 * (or NULL prototype if there is not prototype for the object)
 * and value of 'Extensible' attribute.
 * 
 * Reference counter's value will be set to one.
 * 
 * @return pointer to the object's descriptor
 */
ecma_Object_t*
ecma_CreateObject( ecma_Object_t *pPrototypeObject, /**< pointer to prototybe of the object (or NULL) */
                  bool isExtensible) /**< value of extensible attribute */
{
    ecma_Object_t *pObject = ecma_AllocObject();

    pObject->m_pProperties = ECMA_NULL_POINTER;
    pObject->m_IsLexicalEnvironment = false;
    pObject->m_GCInfo.m_IsObjectValid = true;

    /* The global object is always referenced
     * (at least with the ctx_GlobalObject variable) */
    pObject->m_GCInfo.u.m_Refs = 1;

    pObject->u_Attributes.m_Object.m_Extensible = isExtensible;
    ecma_SetPointer( pObject->u_Attributes.m_Object.m_pPrototypeObject, pPrototypeObject);
    
    return pObject;
} /* ecma_CreateObject */

/**
 * Create a lexical environment with specified outer lexical environment
 * (or NULL if the environment is not nested).
 * 
 * Reference counter's value will be set to one.
 * 
 * Warning: after object-bound lexical environment is created,
 *          caller must create internal properties, that
 *          specify the bound object and value of 'provideThis'.
 * 
 * @return pointer to the descriptor of lexical environment
 */
ecma_Object_t*
ecma_CreateLexicalEnvironment(ecma_Object_t *pOuterLexicalEnvironment, /**< outer lexical environment */
                              ecma_LexicalEnvironmentType_t type) /**< type of lexical environment to create */
{
    ecma_Object_t *pNewLexicalEnvironment = ecma_AllocObject();

    pNewLexicalEnvironment->m_IsLexicalEnvironment = true;
    pNewLexicalEnvironment->u_Attributes.m_LexicalEnvironment.m_Type = type;

    pNewLexicalEnvironment->m_pProperties = ECMA_NULL_POINTER;

    pNewLexicalEnvironment->m_GCInfo.m_IsObjectValid = true;
    pNewLexicalEnvironment->m_GCInfo.u.m_Refs = 1;

    ecma_SetPointer( pNewLexicalEnvironment->u_Attributes.m_LexicalEnvironment.m_pOuterReference, pOuterLexicalEnvironment);
    
    return pNewLexicalEnvironment;
} /* ecma_CreateLexicalEnvironment */

/**
 * Create internal property in an object and link it
 * into the object's properties' linked-list
 * 
 * @return pointer to newly created property's des
 */
ecma_Property_t*
ecma_CreateInternalProperty(ecma_Object_t *pObject, /**< the object */
                            ecma_InternalPropertyId_t propertyId) /**< internal property identifier */
{
    ecma_Property_t *pNewProperty = ecma_AllocProperty();
    
    pNewProperty->m_Type = ECMA_PROPERTY_INTERNAL;
    
    ecma_SetPointer( pNewProperty->m_pNextProperty, ecma_GetPointer( pObject->m_pProperties));
    ecma_SetPointer( pObject->m_pProperties, pNewProperty);

    pNewProperty->u.m_InternalProperty.m_InternalPropertyType = propertyId;
    pNewProperty->u.m_InternalProperty.m_Value = ECMA_NULL_POINTER;
    
    return pNewProperty;
} /* ecma_CreateInternalProperty */

/**
 * Find internal property in the object's property set.
 * 
 * @return pointer to the property's descriptor, if it is found,
 *         NULL - otherwise.
 */
ecma_Property_t*
ecma_FindInternalProperty(ecma_Object_t *pObject, /**< object descriptor */
                          ecma_InternalPropertyId_t propertyId) /**< internal property identifier */
{
    JERRY_ASSERT( pObject != NULL );

    JERRY_ASSERT( propertyId != ECMA_INTERNAL_PROPERTY_PROTOTYPE
                 && propertyId != ECMA_INTERNAL_PROPERTY_EXTENSIBLE );

    for ( ecma_Property_t *pProperty = ecma_GetPointer( pObject->m_pProperties);
          pProperty != NULL;
          pProperty = ecma_GetPointer( pProperty->m_pNextProperty) )
    {
        if ( pProperty->m_Type == ECMA_PROPERTY_INTERNAL )
        {
            if ( pProperty->u.m_InternalProperty.m_InternalPropertyType == propertyId )
            {
                return pProperty;
            }
        }
    }

    return NULL;
} /* ecma_FindInternalProperty */

/**
 * Get an internal property.
 * 
 * Warning:
 *         the property must exist
 * 
 * @return pointer to the property
 */
ecma_Property_t*
ecma_GetInternalProperty(ecma_Object_t *pObject, /**< object descriptor */
                         ecma_InternalPropertyId_t propertyId) /**< internal property identifier */
{
    ecma_Property_t *pProperty = ecma_FindInternalProperty( pObject, propertyId);
    
    JERRY_ASSERT( pProperty != NULL );
    
    return pProperty;
} /* ecma_GetInternalProperty */

/**
 * Allocate new ecma-string and fill it with characters from specified buffer
 * 
 * @return Pointer to first chunk of an array, containing allocated string
 */
ecma_ArrayFirstChunk_t*
ecma_NewEcmaString(const ecma_Char_t *pString, /**< buffer of characters */
                   ecma_Length_t length) /**< length of string, in characters */
{
    JERRY_ASSERT( length == 0 || pString != NULL );

    ecma_ArrayFirstChunk_t *pStringFirstChunk = ecma_AllocArrayFirstChunk();

    pStringFirstChunk->m_Header.m_UnitNumber = length;
    uint8_t *copyPointer = (uint8_t*) pString;
    size_t charsLeft = length;
    size_t charsToCopy = JERRY_MIN( length, sizeof (pStringFirstChunk->m_Elements) / sizeof (ecma_Char_t));
    libc_memcpy(pStringFirstChunk->m_Elements, copyPointer, charsToCopy * sizeof (ecma_Char_t));
    charsLeft -= charsToCopy;
    copyPointer += charsToCopy * sizeof (ecma_Char_t);

    ecma_ArrayNonFirstChunk_t *pStringNonFirstChunk;
    uint16_t *pNextChunkCompressedPointer = &pStringFirstChunk->m_Header.m_pNextChunk;

    while ( charsLeft > 0 )
    {
        pStringNonFirstChunk = ecma_AllocArrayNonFirstChunk();

        size_t charsToCopy = JERRY_MIN( charsLeft, sizeof (pStringNonFirstChunk->m_Elements) / sizeof (ecma_Char_t));
        libc_memcpy(pStringNonFirstChunk->m_Elements, copyPointer, charsToCopy * sizeof (ecma_Char_t));
        charsLeft -= charsToCopy;
        copyPointer += charsToCopy * sizeof (ecma_Char_t);

        ecma_SetPointer( *pNextChunkCompressedPointer, pStringNonFirstChunk);
        pNextChunkCompressedPointer = &pStringNonFirstChunk->m_pNextChunk;
    }

    *pNextChunkCompressedPointer = ECMA_NULL_POINTER;

    return pStringFirstChunk;
} /* ecma_NewEcmaString */

/**
 * Copy ecma-string's contents to a buffer.
 * 
 * Buffer will contain length of string, in characters, followed by string's characters.
 * 
 * @return number of bytes, actually copied to the buffer, if string's content was copied successfully;
 *         negative number, which is calculated as negation of buffer size, that is required
 *         to hold the string's cpntent (in case size of buffer is insuficcient).
 */
ssize_t
ecma_CopyEcmaStringCharsToBuffer(ecma_ArrayFirstChunk_t *pFirstChunk, /**< first chunk of ecma-string */
                                 uint8_t *pBuffer, /**< destination buffer */
                                 size_t bufferSize) /**< size of buffer */
{
    ecma_Length_t stringLength = pFirstChunk->m_Header.m_UnitNumber;
    size_t requiredBufferSize = sizeof (ecma_Length_t) + sizeof (ecma_Char_t) * stringLength;

    if ( requiredBufferSize < bufferSize )
    {
        return -(ssize_t) requiredBufferSize;
    }

    *(ecma_Length_t*) pBuffer = stringLength;

    size_t charsLeft = stringLength;
    uint8_t *destPointer = pBuffer + sizeof (ecma_Length_t);
    size_t copyChunkChars = JERRY_MIN(sizeof (pFirstChunk->m_Elements) / sizeof (ecma_Char_t),
                                      charsLeft);
    libc_memcpy( destPointer, pFirstChunk->m_Elements, copyChunkChars * sizeof (ecma_Char_t));
    destPointer += copyChunkChars * sizeof (ecma_Char_t);
    charsLeft -= copyChunkChars;

    ecma_ArrayNonFirstChunk_t *pNonFirstChunk = ecma_GetPointer( pFirstChunk->m_Header.m_pNextChunk);

    while ( charsLeft > 0 )
    {
        JERRY_ASSERT( charsLeft < stringLength );

        copyChunkChars = JERRY_MIN(sizeof (pNonFirstChunk->m_Elements) / sizeof (ecma_Char_t),
                                   charsLeft);
        libc_memcpy( destPointer, pNonFirstChunk->m_Elements, copyChunkChars * sizeof (ecma_Char_t));
        destPointer += copyChunkChars * sizeof (ecma_Char_t);
        charsLeft -= copyChunkChars;

        pNonFirstChunk = ecma_GetPointer( pNonFirstChunk->m_pNextChunk);
    }

    return (ssize_t) requiredBufferSize;
} /* ecma_CopyEcmaStringCharsToBuffer */

/**
 * Duplicate an ecma-string.
 * 
 * @return pointer to new ecma-string's first chunk
 */
ecma_ArrayFirstChunk_t*
ecma_DuplicateEcmaString( ecma_ArrayFirstChunk_t *pFirstChunk) /**< first chunk of string to duplicate */
{
    JERRY_ASSERT( pFirstChunk != NULL );

    ecma_ArrayFirstChunk_t *pFirstChunkCopy = ecma_AllocArrayFirstChunk();
    libc_memcpy( pFirstChunkCopy, pFirstChunk, sizeof (ecma_ArrayFirstChunk_t));

    ecma_ArrayNonFirstChunk_t *pNonFirstChunk, *pNonFirstChunkCopy;
    pNonFirstChunk = ecma_GetPointer( pFirstChunk->m_Header.m_pNextChunk);
    uint16_t *pNextPointer = &pFirstChunk->m_Header.m_pNextChunk;
    
    while ( pNonFirstChunk != NULL )
    {
        pNonFirstChunkCopy = ecma_AllocArrayNonFirstChunk();
        ecma_SetPointer( *pNextPointer, pNonFirstChunkCopy);
        pNextPointer = &pNonFirstChunkCopy->m_pNextChunk;

        libc_memcpy( pNonFirstChunkCopy, pNonFirstChunk, sizeof (ecma_ArrayNonFirstChunk_t));
        
        pNonFirstChunk = ecma_GetPointer( pNonFirstChunk->m_pNextChunk);
    }
    
    *pNextPointer = ECMA_NULL_POINTER;

    return pFirstChunkCopy;
} /* ecma_DuplicateEcmaString */

/**
 * Compare null-terminated string to ecma-string
 * 
 * @return true - if strings are equal;
 *         false - otherwise.
 */
bool
ecma_CompareCharBufferToEcmaString(ecma_Char_t *pString, /**< null-terminated string */
                                   ecma_ArrayFirstChunk_t *pEcmaString) /* ecma-string */
{
    JERRY_ASSERT( pString != NULL );
    JERRY_ASSERT( pEcmaString != NULL );

    /*
     * TODO:
     */
    JERRY_UNIMPLEMENTED();
} /* ecma_CompareCharBufferToEcmaString */

/**
 * Free all chunks of an array
 */
void
ecma_FreeArray( ecma_ArrayFirstChunk_t *pFirstChunk) /**< first chunk of the array */
{
    JERRY_ASSERT( pFirstChunk != NULL );

    ecma_ArrayNonFirstChunk_t *pNonFirstChunk = ecma_GetPointer( pFirstChunk->m_Header.m_pNextChunk);

    ecma_FreeArrayFirstChunk( pFirstChunk);

    while ( pNonFirstChunk != NULL )
    {
        ecma_ArrayNonFirstChunk_t *pNextChunk = ecma_GetPointer( pNonFirstChunk->m_pNextChunk);

        ecma_FreeArrayNonFirstChunk( pNonFirstChunk);

        pNonFirstChunk = pNextChunk;
    }
} /* ecma_FreeArray */

/**
 * @}
 * @}
 */

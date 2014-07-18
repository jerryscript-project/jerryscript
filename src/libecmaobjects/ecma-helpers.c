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

#include "ecma-alloc.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
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

    pObject->u.m_Object.m_Extensible = isExtensible;
    ecma_SetPointer( pObject->u.m_Object.m_pPrototypeObject, pPrototypeObject);
    
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
    pNewLexicalEnvironment->u.m_LexicalEnvironment.m_Type = type;

    pNewLexicalEnvironment->m_pProperties = ECMA_NULL_POINTER;

    pNewLexicalEnvironment->m_GCInfo.m_IsObjectValid = true;
    pNewLexicalEnvironment->m_GCInfo.u.m_Refs = 1;

    ecma_SetPointer( pNewLexicalEnvironment->u.m_LexicalEnvironment.m_pOuterReference, pOuterLexicalEnvironment);
    
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
 * @return pointer to the property, if it is found,
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
 * Create named data property with given name, attributes and undefined value
 * in the specified object.
 *
 * @return pointer to newly created property
 */
ecma_Property_t*
ecma_CreateNamedProperty(ecma_Object_t *obj_p, /**< object */
                         ecma_Char_t *name_p, /**< property name */
                         ecma_PropertyWritableValue_t writable, /**< 'writable' attribute */
                         ecma_PropertyEnumerableValue_t enumerable, /**< 'enumerable' attribute */
                         ecma_PropertyConfigurableValue_t configurable) /**< 'configurable' attribute */
{
  JERRY_ASSERT( obj_p != NULL && name_p != NULL );

  ecma_Property_t *prop = ecma_AllocProperty();

  prop->m_Type = ECMA_PROPERTY_NAMEDDATA;

  ecma_SetPointer( prop->u.m_NamedDataProperty.m_pName, ecma_NewEcmaString( name_p));

  prop->u.m_NamedDataProperty.m_Writable = writable;
  prop->u.m_NamedDataProperty.m_Enumerable = enumerable;
  prop->u.m_NamedDataProperty.m_Configurable = configurable;

  prop->u.m_NamedDataProperty.m_Value = ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_UNDEFINED);

  ecma_SetPointer( prop->m_pNextProperty, ecma_GetPointer( obj_p->m_pProperties));
  ecma_SetPointer( obj_p->m_pProperties, prop);

  return prop;
} /* ecma_CreateNamedProperty */

/**
 * Find named data property or named access property in specified object.
 *
 * @return pointer to the property, if it is found,
 *         NULL - otherwise.
 */
ecma_Property_t*
ecma_FindNamedProperty(ecma_Object_t *obj_p, /**< object to find property in */
                       ecma_Char_t *name_p) /**< property's name */
{
    JERRY_ASSERT( obj_p != NULL );
    JERRY_ASSERT( name_p != NULL );

    for ( ecma_Property_t *property_p = ecma_GetPointer( obj_p->m_pProperties);
          property_p != NULL;
          property_p = ecma_GetPointer( property_p->m_pNextProperty) )
    {
        ecma_ArrayFirstChunk_t *property_name_p;

        if ( property_p->m_Type == ECMA_PROPERTY_NAMEDDATA )
        {
          property_name_p = ecma_GetPointer( property_p->u.m_NamedDataProperty.m_pName);
        } else if ( property_p->m_Type == ECMA_PROPERTY_NAMEDACCESSOR )
        {
          property_name_p = ecma_GetPointer( property_p->u.m_NamedAccessorProperty.m_pName);
        } else
        {
          continue;
        }

        JERRY_ASSERT( property_name_p != NULL );

	if ( ecma_CompareZtStringToEcmaString( name_p, property_name_p) )
        {
          return property_p;
        }
    }

    return NULL;
} /* ecma_FindNamedProperty */

/**
 * Get named data property or named access property in specified object.
 *
 * Warning:
 *         the property must exist
 *
 * @return pointer to the property, if it is found,
 *         NULL - otherwise.
 */
ecma_Property_t*
ecma_GetNamedProperty(ecma_Object_t *obj_p, /**< object to find property in */
                      ecma_Char_t *name_p) /**< property's name */
{
    JERRY_ASSERT( obj_p != NULL );
    JERRY_ASSERT( name_p != NULL );

    ecma_Property_t *property_p = ecma_FindNamedProperty( obj_p, name_p);

    JERRY_ASSERT( property_p != NULL );

    return property_p;
} /* ecma_GetNamedProperty */

/**
 * Get named data property in specified object.
 *
 * Warning:
 *         the property must exist and be named data property
 *
 * @return pointer to the property, if it is found,
 *         NULL - otherwise.
 */
ecma_Property_t*
ecma_GetNamedDataProperty(ecma_Object_t *obj_p, /**< object to find property in */
                          ecma_Char_t *name_p) /**< property's name */
{
    JERRY_ASSERT( obj_p != NULL );
    JERRY_ASSERT( name_p != NULL );

    ecma_Property_t *property_p = ecma_FindNamedProperty( obj_p, name_p);

    JERRY_ASSERT( property_p != NULL && property_p->m_Type == ECMA_PROPERTY_NAMEDDATA );

    return property_p;
} /* ecma_GetNamedDataProperty */

/**
 * Free the named data property and values it references.
 */
void
ecma_FreeNamedDataProperty( ecma_Property_t *pProperty) /**< the property */
{
  JERRY_ASSERT( pProperty->m_Type == ECMA_PROPERTY_NAMEDDATA );

  ecma_FreeArray( ecma_GetPointer( pProperty->u.m_NamedDataProperty.m_pName));
  ecma_FreeValue( pProperty->u.m_NamedDataProperty.m_Value);

  ecma_DeallocProperty( pProperty);
} /* ecma_FreeNamedDataProperty */

/**
 * Free the named accessor property and values it references.
 */
void
ecma_FreeNamedAccessorProperty( ecma_Property_t *pProperty) /**< the property */
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

  ecma_DeallocProperty( pProperty);
} /* ecma_FreeNamedAccessorProperty */

/**
 * Free the internal property and values it references.
 */
void
ecma_FreeInternalProperty( ecma_Property_t *pProperty) /**< the property */
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

  ecma_DeallocProperty( pProperty);
} /* ecma_FreeInternalProperty */

/**
 * Free the property and values it references.
 */
void
ecma_FreeProperty(ecma_Property_t *prop_p) /**< property */
{
  switch ( (ecma_PropertyType_t) prop_p->m_Type )
  {
    case ECMA_PROPERTY_NAMEDDATA:
      {
        ecma_FreeNamedDataProperty( prop_p);

        break;
      }

    case ECMA_PROPERTY_NAMEDACCESSOR:
      {
        ecma_FreeNamedAccessorProperty( prop_p);

        break;
      }

    case ECMA_PROPERTY_INTERNAL:
      {
        ecma_FreeInternalProperty( prop_p);

        break;
      }
  }
} /* ecma_FreeProperty */

/**
 * Delete the object's property.
 *
 * Warning: specified property must be owned by specified object.
 */
void
ecma_DeleteProperty(ecma_Object_t *obj_p, /**< object */
                    ecma_Property_t *prop_p) /**< property */
{
  for ( ecma_Property_t *cur_prop_p = ecma_GetPointer( obj_p->m_pProperties), *prev_prop_p = NULL, *next_prop_p;
        cur_prop_p != NULL;
        prev_prop_p = cur_prop_p, cur_prop_p = next_prop_p )
  {
    next_prop_p = ecma_GetPointer( cur_prop_p->m_pNextProperty);

    if ( cur_prop_p == prop_p )
    {
      ecma_FreeProperty( prop_p);

      if ( prev_prop_p == NULL )
      {
        ecma_SetPointer( obj_p->m_pProperties, next_prop_p);
      } else
      {
        ecma_SetPointer( prev_prop_p->m_pNextProperty, next_prop_p);
      }

      return;
    }
  }

  JERRY_UNREACHABLE();
} /* ecma_DeleteProperty */

/**
 * Allocate new ecma-string and fill it with characters from specified buffer
 * 
 * @return Pointer to first chunk of an array, containing allocated string
 */
ecma_ArrayFirstChunk_t*
ecma_NewEcmaString(const ecma_Char_t *pString) /**< zero-terminated string of ecma-characters */
{
    ecma_Length_t length = 0;

    /*
     * TODO: Do not precalculate length.
     */
    if ( pString != NULL )
    {
      const ecma_Char_t *iter_p = pString;
      while ( *iter_p++ )
      {
        length++;
      }
    }
 
    ecma_ArrayFirstChunk_t *pStringFirstChunk = ecma_AllocArrayFirstChunk();

    pStringFirstChunk->m_Header.m_UnitNumber = length;
    uint8_t *copyPointer = (uint8_t*) pString;
    size_t charsLeft = length;
    size_t charsToCopy = JERRY_MIN( length, sizeof (pStringFirstChunk->m_Data) / sizeof (ecma_Char_t));
    __memcpy(pStringFirstChunk->m_Data, copyPointer, charsToCopy * sizeof (ecma_Char_t));
    charsLeft -= charsToCopy;
    copyPointer += charsToCopy * sizeof (ecma_Char_t);

    ecma_ArrayNonFirstChunk_t *pStringNonFirstChunk;

    JERRY_STATIC_ASSERT( ECMA_POINTER_FIELD_WIDTH <= sizeof(uint16_t) * JERRY_BITSINBYTE );
    uint16_t *pNextChunkCompressedPointer = &pStringFirstChunk->m_Header.m_pNextChunk;

    while ( charsLeft > 0 )
    {
        pStringNonFirstChunk = ecma_AllocArrayNonFirstChunk();

        size_t charsToCopy = JERRY_MIN( charsLeft, sizeof (pStringNonFirstChunk->m_Data) / sizeof (ecma_Char_t));
        __memcpy(pStringNonFirstChunk->m_Data, copyPointer, charsToCopy * sizeof (ecma_Char_t));
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
 *         to hold the string's content (in case size of buffer is insuficcient).
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
    size_t copyChunkChars = JERRY_MIN(sizeof (pFirstChunk->m_Data) / sizeof (ecma_Char_t),
                                      charsLeft);
    __memcpy( destPointer, pFirstChunk->m_Data, copyChunkChars * sizeof (ecma_Char_t));
    destPointer += copyChunkChars * sizeof (ecma_Char_t);
    charsLeft -= copyChunkChars;

    ecma_ArrayNonFirstChunk_t *pNonFirstChunk = ecma_GetPointer( pFirstChunk->m_Header.m_pNextChunk);

    while ( charsLeft > 0 )
    {
        JERRY_ASSERT( charsLeft < stringLength );

        copyChunkChars = JERRY_MIN(sizeof (pNonFirstChunk->m_Data) / sizeof (ecma_Char_t),
                                   charsLeft);
        __memcpy( destPointer, pNonFirstChunk->m_Data, copyChunkChars * sizeof (ecma_Char_t));
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
    __memcpy( pFirstChunkCopy, pFirstChunk, sizeof (ecma_ArrayFirstChunk_t));

    ecma_ArrayNonFirstChunk_t *pNonFirstChunk, *pNonFirstChunkCopy;
    pNonFirstChunk = ecma_GetPointer( pFirstChunk->m_Header.m_pNextChunk);
    uint16_t *pNextPointer = &pFirstChunkCopy->m_Header.m_pNextChunk;
    
    while ( pNonFirstChunk != NULL )
    {
        pNonFirstChunkCopy = ecma_AllocArrayNonFirstChunk();
        ecma_SetPointer( *pNextPointer, pNonFirstChunkCopy);
        pNextPointer = &pNonFirstChunkCopy->m_pNextChunk;

        __memcpy( pNonFirstChunkCopy, pNonFirstChunk, sizeof (ecma_ArrayNonFirstChunk_t));
        
        pNonFirstChunk = ecma_GetPointer( pNonFirstChunk->m_pNextChunk);
    }
    
    *pNextPointer = ECMA_NULL_POINTER;

    return pFirstChunkCopy;
} /* ecma_DuplicateEcmaString */

/**
 * Compare zero-terminated string to ecma-string
 * 
 * @return true - if strings are equal;
 *         false - otherwise.
 */
bool
ecma_CompareZtStringToEcmaString(const ecma_Char_t *pString, /**< zero-terminated string */
                                 const ecma_ArrayFirstChunk_t *pEcmaString) /* ecma-string */
{
  JERRY_ASSERT( pString != NULL );
  JERRY_ASSERT( pEcmaString != NULL );

  const ecma_Char_t *str_iter_p = pString;
  ecma_Length_t ecma_str_len = pEcmaString->m_Header.m_UnitNumber;
  const ecma_Char_t *current_chunk_chars_cur = (ecma_Char_t*) pEcmaString->m_Data,
        *current_chunk_chars_end = (ecma_Char_t*) (pEcmaString->m_Data
                                                   + sizeof(pEcmaString->m_Data));

  JERRY_STATIC_ASSERT( ECMA_POINTER_FIELD_WIDTH <= sizeof(uint16_t) * JERRY_BITSINBYTE );
  const uint16_t *next_chunk_compressed_pointer_p = &pEcmaString->m_Header.m_pNextChunk;

  for ( ecma_Length_t str_index = 0;
        str_index < ecma_str_len;
        str_index++, str_iter_p++, current_chunk_chars_cur++ )
    {
      JERRY_ASSERT( current_chunk_chars_cur <= current_chunk_chars_end );

      if ( current_chunk_chars_cur == current_chunk_chars_end )
        {
          /* switching to next chunk */
          ecma_ArrayNonFirstChunk_t *next_chunk_p = ecma_GetPointer( *next_chunk_compressed_pointer_p);

          JERRY_ASSERT( next_chunk_p != NULL );

          current_chunk_chars_cur = (ecma_Char_t*) pEcmaString->m_Data;
          current_chunk_chars_end = (ecma_Char_t*) (next_chunk_p->m_Data + sizeof(next_chunk_p->m_Data));

          next_chunk_compressed_pointer_p = &next_chunk_p->m_pNextChunk;
        }

      if ( *str_iter_p != *current_chunk_chars_cur )
        {
          /* 
           * Either *str_iter_p is 0 (zero-terminated string is shorter),
           * or the character is just different.
           *
           * In both cases strings are not equal.
           */
          return false;
        }
    }

  /*
   * Now, we have reached end of ecma-string.
   *
   * If we have also reached end of zero-terminated string, than strings are equal.
   * Otherwise zero-terminated string is longer.
   */
  return ( *str_iter_p == 0 );
} /* ecma_CompareZtStringToEcmaString */

/**
 * Free all chunks of an array
 */
void
ecma_FreeArray( ecma_ArrayFirstChunk_t *pFirstChunk) /**< first chunk of the array */
{
    JERRY_ASSERT( pFirstChunk != NULL );

    ecma_ArrayNonFirstChunk_t *pNonFirstChunk = ecma_GetPointer( pFirstChunk->m_Header.m_pNextChunk);

    ecma_DeallocArrayFirstChunk( pFirstChunk);

    while ( pNonFirstChunk != NULL )
    {
        ecma_ArrayNonFirstChunk_t *pNextChunk = ecma_GetPointer( pNonFirstChunk->m_pNextChunk);

        ecma_DeallocArrayNonFirstChunk( pNonFirstChunk);

        pNonFirstChunk = pNextChunk;
    }
} /* ecma_FreeArray */

/**
 * @}
 * @}
 */

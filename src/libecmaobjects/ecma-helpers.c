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
ecma_compress_pointer(void *pointer) /**< pointer to compress */
{
    if ( pointer == NULL )
    {
        return ECMA_NULL_POINTER;
    }

    uintptr_t intPtr = (uintptr_t) pointer;

    JERRY_ASSERT(intPtr % MEM_ALIGNMENT == 0);

    intPtr -= mem_get_base_pointer();
    intPtr >>= MEM_ALIGNMENT_LOG;

    JERRY_ASSERT((intPtr & ~((1u << ECMA_POINTER_FIELD_WIDTH) - 1)) == 0);

    return intPtr;
} /* ecma_compress_pointer */

/**
 * Decompress pointer.
 */
void*
ecma_decompress_pointer(uintptr_t compressedPointer) /**< pointer to decompress */
{
    if ( compressedPointer == ECMA_NULL_POINTER )
    {
        return NULL;
    }

    uintptr_t intPtr = compressedPointer;

    intPtr <<= MEM_ALIGNMENT_LOG;
    intPtr += mem_get_base_pointer();

    return (void*) intPtr;
} /* ecma_decompress_pointer */

/**
 * Create an object with specified prototype object
 * (or NULL prototype if there is not prototype for the object)
 * and value of 'Extensible' attribute.
 * 
 * Reference counter's value will be set to one.
 * 
 * @return pointer to the object's descriptor
 */
ecma_object_t*
ecma_create_object( ecma_object_t *pPrototypeObject, /**< pointer to prototybe of the object (or NULL) */
                  bool isExtensible) /**< value of extensible attribute */
{
    ecma_object_t *pObject = ecma_alloc_object();

    pObject->pProperties = ECMA_NULL_POINTER;
    pObject->IsLexicalEnvironment = false;
    pObject->GCInfo.IsObjectValid = true;

    /* The global object is always referenced
     * (at least with the ctx_GlobalObject variable) */
    pObject->GCInfo.u.Refs = 1;

    pObject->u.Object.Extensible = isExtensible;
    ecma_set_pointer( pObject->u.Object.pPrototypeObject, pPrototypeObject);
    
    return pObject;
} /* ecma_create_object */

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
ecma_object_t*
ecma_create_lexical_environment(ecma_object_t *pOuterLexicalEnvironment, /**< outer lexical environment */
                              ecma_lexical_environment_type_t type) /**< type of lexical environment to create */
{
    ecma_object_t *pNewLexicalEnvironment = ecma_alloc_object();

    pNewLexicalEnvironment->IsLexicalEnvironment = true;
    pNewLexicalEnvironment->u.LexicalEnvironment.Type = type;

    pNewLexicalEnvironment->pProperties = ECMA_NULL_POINTER;

    pNewLexicalEnvironment->GCInfo.IsObjectValid = true;
    pNewLexicalEnvironment->GCInfo.u.Refs = 1;

    ecma_set_pointer( pNewLexicalEnvironment->u.LexicalEnvironment.pOuterReference, pOuterLexicalEnvironment);
    
    return pNewLexicalEnvironment;
} /* ecma_create_lexical_environment */

/**
 * Create internal property in an object and link it
 * into the object's properties' linked-list
 * 
 * @return pointer to newly created property's des
 */
ecma_property_t*
ecma_create_internal_property(ecma_object_t *pObject, /**< the object */
                            ecma_internal_property_id_t propertyId) /**< internal property identifier */
{
    ecma_property_t *pNewProperty = ecma_alloc_property();
    
    pNewProperty->Type = ECMA_PROPERTY_INTERNAL;
    
    ecma_set_pointer( pNewProperty->pNextProperty, ecma_get_pointer( pObject->pProperties));
    ecma_set_pointer( pObject->pProperties, pNewProperty);

    pNewProperty->u.InternalProperty.InternalPropertyType = propertyId;
    pNewProperty->u.InternalProperty.Value = ECMA_NULL_POINTER;
    
    return pNewProperty;
} /* ecma_create_internal_property */

/**
 * Find internal property in the object's property set.
 * 
 * @return pointer to the property, if it is found,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_find_internal_property(ecma_object_t *pObject, /**< object descriptor */
                          ecma_internal_property_id_t propertyId) /**< internal property identifier */
{
    JERRY_ASSERT( pObject != NULL );

    JERRY_ASSERT( propertyId != ECMA_INTERNAL_PROPERTY_PROTOTYPE
                 && propertyId != ECMA_INTERNAL_PROPERTY_EXTENSIBLE );

    for ( ecma_property_t *pProperty = ecma_get_pointer( pObject->pProperties);
          pProperty != NULL;
          pProperty = ecma_get_pointer( pProperty->pNextProperty) )
    {
        if ( pProperty->Type == ECMA_PROPERTY_INTERNAL )
        {
            if ( pProperty->u.InternalProperty.InternalPropertyType == propertyId )
            {
                return pProperty;
            }
        }
    }

    return NULL;
} /* ecma_find_internal_property */

/**
 * Get an internal property.
 * 
 * Warning:
 *         the property must exist
 * 
 * @return pointer to the property
 */
ecma_property_t*
ecma_get_internal_property(ecma_object_t *pObject, /**< object descriptor */
                         ecma_internal_property_id_t propertyId) /**< internal property identifier */
{
    ecma_property_t *pProperty = ecma_find_internal_property( pObject, propertyId);
    
    JERRY_ASSERT( pProperty != NULL );
    
    return pProperty;
} /* ecma_get_internal_property */

/**
 * Create named data property with given name, attributes and undefined value
 * in the specified object.
 *
 * @return pointer to newly created property
 */
ecma_property_t*
ecma_create_named_property(ecma_object_t *obj_p, /**< object */
                         ecma_char_t *name_p, /**< property name */
                         ecma_property_writable_value_t writable, /**< 'writable' attribute */
                         ecma_property_enumerable_value_t enumerable, /**< 'enumerable' attribute */
                         ecma_property_configurable_value_t configurable) /**< 'configurable' attribute */
{
  JERRY_ASSERT( obj_p != NULL && name_p != NULL );

  ecma_property_t *prop = ecma_alloc_property();

  prop->Type = ECMA_PROPERTY_NAMEDDATA;

  ecma_set_pointer( prop->u.NamedDataProperty.pName, ecma_new_ecma_string( name_p));

  prop->u.NamedDataProperty.Writable = writable;
  prop->u.NamedDataProperty.Enumerable = enumerable;
  prop->u.NamedDataProperty.Configurable = configurable;

  prop->u.NamedDataProperty.Value = ecma_make_simple_value( ECMA_SIMPLE_VALUE_UNDEFINED);

  ecma_set_pointer( prop->pNextProperty, ecma_get_pointer( obj_p->pProperties));
  ecma_set_pointer( obj_p->pProperties, prop);

  return prop;
} /* ecma_create_named_property */

/**
 * Find named data property or named access property in specified object.
 *
 * @return pointer to the property, if it is found,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_find_named_property(ecma_object_t *obj_p, /**< object to find property in */
                       ecma_char_t *name_p) /**< property's name */
{
    JERRY_ASSERT( obj_p != NULL );
    JERRY_ASSERT( name_p != NULL );

    for ( ecma_property_t *property_p = ecma_get_pointer( obj_p->pProperties);
          property_p != NULL;
          property_p = ecma_get_pointer( property_p->pNextProperty) )
    {
        ecma_array_first_chunk_t *property_name_p;

        if ( property_p->Type == ECMA_PROPERTY_NAMEDDATA )
        {
          property_name_p = ecma_get_pointer( property_p->u.NamedDataProperty.pName);
        } else if ( property_p->Type == ECMA_PROPERTY_NAMEDACCESSOR )
        {
          property_name_p = ecma_get_pointer( property_p->u.NamedAccessorProperty.pName);
        } else
        {
          continue;
        }

        JERRY_ASSERT( property_name_p != NULL );

	if ( ecma_compare_zt_string_to_ecma_string( name_p, property_name_p) )
        {
          return property_p;
        }
    }

    return NULL;
} /* ecma_find_named_property */

/**
 * Get named data property or named access property in specified object.
 *
 * Warning:
 *         the property must exist
 *
 * @return pointer to the property, if it is found,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_get_named_property(ecma_object_t *obj_p, /**< object to find property in */
                      ecma_char_t *name_p) /**< property's name */
{
    JERRY_ASSERT( obj_p != NULL );
    JERRY_ASSERT( name_p != NULL );

    ecma_property_t *property_p = ecma_find_named_property( obj_p, name_p);

    JERRY_ASSERT( property_p != NULL );

    return property_p;
} /* ecma_get_named_property */

/**
 * Get named data property in specified object.
 *
 * Warning:
 *         the property must exist and be named data property
 *
 * @return pointer to the property, if it is found,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_get_named_data_property(ecma_object_t *obj_p, /**< object to find property in */
                          ecma_char_t *name_p) /**< property's name */
{
    JERRY_ASSERT( obj_p != NULL );
    JERRY_ASSERT( name_p != NULL );

    ecma_property_t *property_p = ecma_find_named_property( obj_p, name_p);

    JERRY_ASSERT( property_p != NULL && property_p->Type == ECMA_PROPERTY_NAMEDDATA );

    return property_p;
} /* ecma_get_named_data_property */

/**
 * Free the named data property and values it references.
 */
void
ecma_free_named_data_property( ecma_property_t *pProperty) /**< the property */
{
  JERRY_ASSERT( pProperty->Type == ECMA_PROPERTY_NAMEDDATA );

  ecma_free_array( ecma_get_pointer( pProperty->u.NamedDataProperty.pName));
  ecma_free_value( pProperty->u.NamedDataProperty.Value);

  ecma_dealloc_property( pProperty);
} /* ecma_free_named_data_property */

/**
 * Free the named accessor property and values it references.
 */
void
ecma_free_named_accessor_property( ecma_property_t *pProperty) /**< the property */
{
  JERRY_ASSERT( pProperty->Type == ECMA_PROPERTY_NAMEDACCESSOR );

  ecma_free_array( ecma_get_pointer( pProperty->u.NamedAccessorProperty.pName));

  ecma_object_t *pGet = ecma_get_pointer(pProperty->u.NamedAccessorProperty.pGet);
  ecma_object_t *pSet = ecma_get_pointer(pProperty->u.NamedAccessorProperty.pSet);

  if ( pGet != NULL )
  {
    ecma_deref_object( pGet);
  }

  if ( pSet != NULL )
  {
    ecma_deref_object( pSet);
  }

  ecma_dealloc_property( pProperty);
} /* ecma_free_named_accessor_property */

/**
 * Free the internal property and values it references.
 */
void
ecma_free_internal_property( ecma_property_t *pProperty) /**< the property */
{
  JERRY_ASSERT( pProperty->Type == ECMA_PROPERTY_INTERNAL );

  ecma_internal_property_id_t propertyId = pProperty->u.InternalProperty.InternalPropertyType;
  uint32_t propertyValue = pProperty->u.InternalProperty.Value;

  switch ( propertyId )
  {
    case ECMA_INTERNAL_PROPERTY_CLASS: /* a string */
    case ECMA_INTERNAL_PROPERTY_NUMBER_INDEXED_ARRAY_VALUES: /* an array */
    case ECMA_INTERNAL_PROPERTY_STRING_INDEXED_ARRAY_VALUES: /* an array */
      {
        ecma_free_array( ecma_get_pointer( propertyValue));
        break;
      }

    case ECMA_INTERNAL_PROPERTY_SCOPE: /* a lexical environment */
    case ECMA_INTERNAL_PROPERTY_BINDING_OBJECT: /* an object */
      {
        ecma_deref_object( ecma_get_pointer( propertyValue));
        break;
      }

    case ECMA_INTERNAL_PROPERTY_PROTOTYPE: /* the property's value is located in ecma_object_t */
    case ECMA_INTERNAL_PROPERTY_EXTENSIBLE: /* the property's value is located in ecma_object_t */
    case ECMA_INTERNAL_PROPERTY_PROVIDE_THIS: /* a boolean flag */
      {
        break;
      }
  }

  ecma_dealloc_property( pProperty);
} /* ecma_free_internal_property */

/**
 * Free the property and values it references.
 */
void
ecma_free_property(ecma_property_t *prop_p) /**< property */
{
  switch ( (ecma_property_type_t) prop_p->Type )
  {
    case ECMA_PROPERTY_NAMEDDATA:
      {
        ecma_free_named_data_property( prop_p);

        break;
      }

    case ECMA_PROPERTY_NAMEDACCESSOR:
      {
        ecma_free_named_accessor_property( prop_p);

        break;
      }

    case ECMA_PROPERTY_INTERNAL:
      {
        ecma_free_internal_property( prop_p);

        break;
      }
  }
} /* ecma_free_property */

/**
 * Delete the object's property.
 *
 * Warning: specified property must be owned by specified object.
 */
void
ecma_delete_property(ecma_object_t *obj_p, /**< object */
                    ecma_property_t *prop_p) /**< property */
{
  for ( ecma_property_t *cur_prop_p = ecma_get_pointer( obj_p->pProperties), *prev_prop_p = NULL, *next_prop_p;
        cur_prop_p != NULL;
        prev_prop_p = cur_prop_p, cur_prop_p = next_prop_p )
  {
    next_prop_p = ecma_get_pointer( cur_prop_p->pNextProperty);

    if ( cur_prop_p == prop_p )
    {
      ecma_free_property( prop_p);

      if ( prev_prop_p == NULL )
      {
        ecma_set_pointer( obj_p->pProperties, next_prop_p);
      } else
      {
        ecma_set_pointer( prev_prop_p->pNextProperty, next_prop_p);
      }

      return;
    }
  }

  JERRY_UNREACHABLE();
} /* ecma_delete_property */

/**
 * Allocate new ecma-string and fill it with characters from specified buffer
 * 
 * @return Pointer to first chunk of an array, containing allocated string
 */
ecma_array_first_chunk_t*
ecma_new_ecma_string(const ecma_char_t *pString) /**< zero-terminated string of ecma-characters */
{
    ecma_length_t length = 0;

    /*
     * TODO: Do not precalculate length.
     */
    if ( pString != NULL )
    {
      const ecma_char_t *iter_p = pString;
      while ( *iter_p++ )
      {
        length++;
      }
    }
 
    ecma_array_first_chunk_t *pStringFirstChunk = ecma_alloc_array_first_chunk();

    pStringFirstChunk->Header.UnitNumber = length;
    uint8_t *copyPointer = (uint8_t*) pString;
    size_t charsLeft = length;
    size_t charsToCopy = JERRY_MIN( length, sizeof (pStringFirstChunk->Data) / sizeof (ecma_char_t));
    __memcpy(pStringFirstChunk->Data, copyPointer, charsToCopy * sizeof (ecma_char_t));
    charsLeft -= charsToCopy;
    copyPointer += charsToCopy * sizeof (ecma_char_t);

    ecma_array_non_first_chunk_t *pStringNonFirstChunk;

    JERRY_STATIC_ASSERT( ECMA_POINTER_FIELD_WIDTH <= sizeof(uint16_t) * JERRY_BITSINBYTE );
    uint16_t *pNextChunkCompressedPointer = &pStringFirstChunk->Header.pNextChunk;

    while ( charsLeft > 0 )
    {
        pStringNonFirstChunk = ecma_alloc_array_non_first_chunk();

        size_t charsToCopy = JERRY_MIN( charsLeft, sizeof (pStringNonFirstChunk->Data) / sizeof (ecma_char_t));
        __memcpy(pStringNonFirstChunk->Data, copyPointer, charsToCopy * sizeof (ecma_char_t));
        charsLeft -= charsToCopy;
        copyPointer += charsToCopy * sizeof (ecma_char_t);

        ecma_set_pointer( *pNextChunkCompressedPointer, pStringNonFirstChunk);
        pNextChunkCompressedPointer = &pStringNonFirstChunk->pNextChunk;
    }

    *pNextChunkCompressedPointer = ECMA_NULL_POINTER;

    return pStringFirstChunk;
} /* ecma_new_ecma_string */

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
ecma_copy_ecma_string_chars_to_buffer(ecma_array_first_chunk_t *pFirstChunk, /**< first chunk of ecma-string */
                                 uint8_t *pBuffer, /**< destination buffer */
                                 size_t bufferSize) /**< size of buffer */
{
    ecma_length_t stringLength = pFirstChunk->Header.UnitNumber;
    size_t requiredBufferSize = sizeof (ecma_length_t) + sizeof (ecma_char_t) * stringLength;

    if ( requiredBufferSize < bufferSize )
    {
        return -(ssize_t) requiredBufferSize;
    }

    *(ecma_length_t*) pBuffer = stringLength;

    size_t charsLeft = stringLength;
    uint8_t *destPointer = pBuffer + sizeof (ecma_length_t);
    size_t copyChunkChars = JERRY_MIN(sizeof (pFirstChunk->Data) / sizeof (ecma_char_t),
                                      charsLeft);
    __memcpy( destPointer, pFirstChunk->Data, copyChunkChars * sizeof (ecma_char_t));
    destPointer += copyChunkChars * sizeof (ecma_char_t);
    charsLeft -= copyChunkChars;

    ecma_array_non_first_chunk_t *pNonFirstChunk = ecma_get_pointer( pFirstChunk->Header.pNextChunk);

    while ( charsLeft > 0 )
    {
        JERRY_ASSERT( charsLeft < stringLength );

        copyChunkChars = JERRY_MIN(sizeof (pNonFirstChunk->Data) / sizeof (ecma_char_t),
                                   charsLeft);
        __memcpy( destPointer, pNonFirstChunk->Data, copyChunkChars * sizeof (ecma_char_t));
        destPointer += copyChunkChars * sizeof (ecma_char_t);
        charsLeft -= copyChunkChars;

        pNonFirstChunk = ecma_get_pointer( pNonFirstChunk->pNextChunk);
    }

    return (ssize_t) requiredBufferSize;
} /* ecma_copy_ecma_string_chars_to_buffer */

/**
 * Duplicate an ecma-string.
 * 
 * @return pointer to new ecma-string's first chunk
 */
ecma_array_first_chunk_t*
ecma_duplicate_ecma_string( ecma_array_first_chunk_t *pFirstChunk) /**< first chunk of string to duplicate */
{
    JERRY_ASSERT( pFirstChunk != NULL );

    ecma_array_first_chunk_t *pFirstChunkCopy = ecma_alloc_array_first_chunk();
    __memcpy( pFirstChunkCopy, pFirstChunk, sizeof (ecma_array_first_chunk_t));

    ecma_array_non_first_chunk_t *pNonFirstChunk, *pNonFirstChunkCopy;
    pNonFirstChunk = ecma_get_pointer( pFirstChunk->Header.pNextChunk);
    uint16_t *pNextPointer = &pFirstChunkCopy->Header.pNextChunk;
    
    while ( pNonFirstChunk != NULL )
    {
        pNonFirstChunkCopy = ecma_alloc_array_non_first_chunk();
        ecma_set_pointer( *pNextPointer, pNonFirstChunkCopy);
        pNextPointer = &pNonFirstChunkCopy->pNextChunk;

        __memcpy( pNonFirstChunkCopy, pNonFirstChunk, sizeof (ecma_array_non_first_chunk_t));
        
        pNonFirstChunk = ecma_get_pointer( pNonFirstChunk->pNextChunk);
    }
    
    *pNextPointer = ECMA_NULL_POINTER;

    return pFirstChunkCopy;
} /* ecma_duplicate_ecma_string */

/**
 * Compare zero-terminated string to ecma-string
 * 
 * @return true - if strings are equal;
 *         false - otherwise.
 */
bool
ecma_compare_ecma_string_to_ecma_string(const ecma_array_first_chunk_t *string1_p, /* ecma-string */
                                   const ecma_array_first_chunk_t *string2_p) /* ecma-string */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( string1_p, string2_p);
} /* ecma_compare_ecma_string_to_ecma_string */

/**
 * Compare zero-terminated string to ecma-string
 * 
 * @return true - if strings are equal;
 *         false - otherwise.
 */
bool
ecma_compare_zt_string_to_ecma_string(const ecma_char_t *pString, /**< zero-terminated string */
                                 const ecma_array_first_chunk_t *pEcmaString) /* ecma-string */
{
  JERRY_ASSERT( pString != NULL );
  JERRY_ASSERT( pEcmaString != NULL );

  const ecma_char_t *str_iter_p = pString;
  ecma_length_t ecma_str_len = pEcmaString->Header.UnitNumber;
  const ecma_char_t *current_chunk_chars_cur = (ecma_char_t*) pEcmaString->Data,
        *current_chunk_chars_end = (ecma_char_t*) (pEcmaString->Data
                                                   + sizeof(pEcmaString->Data));

  JERRY_STATIC_ASSERT( ECMA_POINTER_FIELD_WIDTH <= sizeof(uint16_t) * JERRY_BITSINBYTE );
  const uint16_t *next_chunk_compressed_pointer_p = &pEcmaString->Header.pNextChunk;

  for ( ecma_length_t str_index = 0;
        str_index < ecma_str_len;
        str_index++, str_iter_p++, current_chunk_chars_cur++ )
    {
      JERRY_ASSERT( current_chunk_chars_cur <= current_chunk_chars_end );

      if ( current_chunk_chars_cur == current_chunk_chars_end )
        {
          /* switching to next chunk */
          ecma_array_non_first_chunk_t *next_chunk_p = ecma_get_pointer( *next_chunk_compressed_pointer_p);

          JERRY_ASSERT( next_chunk_p != NULL );

          current_chunk_chars_cur = (ecma_char_t*) pEcmaString->Data;
          current_chunk_chars_end = (ecma_char_t*) (next_chunk_p->Data + sizeof(next_chunk_p->Data));

          next_chunk_compressed_pointer_p = &next_chunk_p->pNextChunk;
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
} /* ecma_compare_zt_string_to_ecma_string */

/**
 * Free all chunks of an array
 */
void
ecma_free_array( ecma_array_first_chunk_t *pFirstChunk) /**< first chunk of the array */
{
    JERRY_ASSERT( pFirstChunk != NULL );

    ecma_array_non_first_chunk_t *pNonFirstChunk = ecma_get_pointer( pFirstChunk->Header.pNextChunk);

    ecma_dealloc_array_first_chunk( pFirstChunk);

    while ( pNonFirstChunk != NULL )
    {
        ecma_array_non_first_chunk_t *pNextChunk = ecma_get_pointer( pNonFirstChunk->pNextChunk);

        ecma_dealloc_array_non_first_chunk( pNonFirstChunk);

        pNonFirstChunk = pNextChunk;
    }
} /* ecma_free_array */

/**
 * @}
 * @}
 */

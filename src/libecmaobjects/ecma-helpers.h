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

#ifndef JERRY_ECMA_HELPERS_H
#define JERRY_ECMA_HELPERS_H

#include "ecma-globals.h"

extern uintptr_t ecma_CompressPointer(void *pointer);
extern void* ecma_DecompressPointer(uintptr_t compressedPointer);

/**
 * Get value of pointer from specified compressed pointer field.
 */
#define ecma_GetPointer( field) \
    ecma_DecompressPointer( field)

/**
 * Set value of compressed pointer field so that it will correspond
 * to specified nonCompressedPointer.
 */
#define ecma_SetPointer( field, nonCompressedPointer) \
    (field) = ecma_CompressPointer( nonCompressedPointer) & ( ( 1u << ECMA_POINTER_FIELD_WIDTH ) - 1)

extern ecma_Object_t* ecma_CreateObject( ecma_Object_t *pPrototypeObject, bool isExtensible);
extern ecma_Object_t* ecma_CreateLexicalEnvironment( ecma_Object_t *pOuterLexicalEnvironment, ecma_LexicalEnvironmentType_t type);

extern ecma_Property_t* ecma_CreateInternalProperty(ecma_Object_t *pObject, ecma_InternalPropertyId_t propertyId);
extern ecma_Property_t* ecma_FindInternalProperty(ecma_Object_t *pObject, ecma_InternalPropertyId_t propertyId);
extern ecma_Property_t* ecma_GetInternalProperty(ecma_Object_t *pObject, ecma_InternalPropertyId_t propertyId);
extern ecma_Property_t* ecma_SetInternalProperty(ecma_Object_t *pObject, ecma_InternalPropertyId_t propertyId);

extern ecma_ArrayFirstChunk_t* ecma_NewEcmaString( const ecma_Char_t *pString, ecma_Length_t length);
extern ssize_t ecma_CopyEcmaStringCharsToBuffer( ecma_ArrayFirstChunk_t *pFirstChunk, uint8_t *pBuffer, size_t bufferSize);
extern ecma_ArrayFirstChunk_t* ecma_DuplicateEcmaString( ecma_ArrayFirstChunk_t *pFirstChunk);
extern bool ecma_CompareCharBufferToEcmaString( ecma_Char_t *pString, ecma_ArrayFirstChunk_t *pEcmaString);

extern void ecma_FreeArray( ecma_ArrayFirstChunk_t *pFirstChunk);

#endif /* !JERRY_ECMA_HELPERS_H */

/**
 * @}
 * @}
 */
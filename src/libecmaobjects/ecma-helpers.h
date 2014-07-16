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

/* ecma-helpers-value.c */
extern bool ecma_IsValueUndefined( ecma_Value_t value);
extern bool ecma_IsValueNull( ecma_Value_t value);
extern bool ecma_IsValueBoolean( ecma_Value_t value);
extern bool ecma_IsValueTrue( ecma_Value_t value);

extern ecma_Value_t ecma_MakeSimpleValue( ecma_SimpleValue_t value);
extern ecma_Value_t ecma_MakeObjectValue( ecma_Object_t* object_p);
extern ecma_Value_t ecma_CopyValue( const ecma_Value_t value);
extern void ecma_FreeValue( const ecma_Value_t value);

extern ecma_CompletionValue_t ecma_MakeCompletionValue( ecma_CompletionType_t type, ecma_Value_t value, uint8_t target);
extern ecma_CompletionValue_t ecma_MakeThrowValue( ecma_Object_t *exception_p);

extern bool ecma_IsCompletionValueNormalFalse( ecma_CompletionValue_t value);
extern bool ecma_IsCompletionValueNormalTrue( ecma_CompletionValue_t value);

extern ecma_Object_t* ecma_CreateObject( ecma_Object_t *pPrototypeObject, bool isExtensible);
extern ecma_Object_t* ecma_CreateLexicalEnvironment( ecma_Object_t *pOuterLexicalEnvironment, ecma_LexicalEnvironmentType_t type);

/* ecma-helpers.c */
extern ecma_Property_t* ecma_CreateInternalProperty(ecma_Object_t *pObject, ecma_InternalPropertyId_t propertyId);
extern ecma_Property_t* ecma_FindInternalProperty(ecma_Object_t *pObject, ecma_InternalPropertyId_t propertyId);
extern ecma_Property_t* ecma_GetInternalProperty(ecma_Object_t *pObject, ecma_InternalPropertyId_t propertyId);

extern ecma_Property_t *ecma_CreateNamedProperty(ecma_Object_t *obj_p, ecma_Char_t *name_p, ecma_PropertyWritableValue_t writable, ecma_PropertyEnumerableValue_t enumerable, ecma_PropertyConfigurableValue_t configurable);
extern ecma_Property_t *ecma_FindNamedProperty(ecma_Object_t *obj_p, ecma_Char_t *name_p);
extern ecma_Property_t *ecma_GetNamedProperty(ecma_Object_t *obj_p, ecma_Char_t *name_p);
extern ecma_Property_t *ecma_GetNamedDataProperty(ecma_Object_t *obj_p, ecma_Char_t *name_p);

extern void ecma_FreeInternalProperty(ecma_Property_t *prop_p);
extern void ecma_FreeNamedDataProperty(ecma_Property_t *prop_p);
extern void ecma_FreeNamedAccessorProperty(ecma_Property_t *prop_p);
extern void ecma_FreeProperty(ecma_Property_t *prop_p);

extern void ecma_DeleteProperty( ecma_Object_t *obj_p, ecma_Property_t *prop_p);

extern ecma_ArrayFirstChunk_t* ecma_NewEcmaString( const ecma_Char_t *pString);
extern ssize_t ecma_CopyEcmaStringCharsToBuffer( ecma_ArrayFirstChunk_t *pFirstChunk, uint8_t *pBuffer, size_t bufferSize);
extern ecma_ArrayFirstChunk_t* ecma_DuplicateEcmaString( ecma_ArrayFirstChunk_t *pFirstChunk);
extern bool ecma_CompareCharBufferToEcmaString( ecma_Char_t *pString, ecma_ArrayFirstChunk_t *pEcmaString);

extern void ecma_FreeArray( ecma_ArrayFirstChunk_t *pFirstChunk);

#endif /* !JERRY_ECMA_HELPERS_H */

/**
 * @}
 * @}
 */

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

#ifndef JERRY_ECMA_ALLOC_H
#define	JERRY_ECMA_ALLOC_H

#include "ecma-globals.h"

/**
 * Allocate memory for ecma-object
 * 
 * @return pointer to allocated memory
 */
extern ecma_Object_t *ecma_AllocObject(void);

/**
 * Dealloc memory from an ecma-object
 */
extern void ecma_DeallocObject( ecma_Object_t *pObject);

/**
 * Allocate memory for ecma-property
 * 
 * @return pointer to allocated memory
 */
extern ecma_Property_t *ecma_AllocProperty(void);

/**
 * Dealloc memory from an ecma-property
 */
extern void ecma_DeallocProperty( ecma_Property_t *pProperty);

/**
 * Allocate memory for ecma-number
 * 
 * @return pointer to allocated memory
 */
extern ecma_Number_t *ecma_AllocNumber(void);

/**
 * Dealloc memory from an ecma-number
 */
extern void ecma_DeallocNumber( ecma_Number_t *pNumber);

/**
 * Allocate memory for first chunk of an ecma-array
 * 
 * @return pointer to allocated memory
 */
extern ecma_ArrayFirstChunk_t *ecma_AllocArrayFirstChunk(void);

/**
 * Dealloc memory from first chunk of an ecma-array
 */
extern void ecma_DeallocArrayFirstChunk( ecma_ArrayFirstChunk_t *pFirstChunk);

/**
 * Allocate memory for non-first chunk of an ecma-array
 * 
 * @return pointer to allocated memory
 */
extern ecma_ArrayNonFirstChunk_t *ecma_AllocArrayNonFirstChunk(void);

/**
 * Dealloc memory from non-first chunk of an ecma-array
 */
extern void ecma_DeallocArrayNonFirstChunk( ecma_ArrayNonFirstChunk_t *pNumber);

#endif /* JERRY_ECMA_ALLOC_H */

/**
 * @}
 * @}
 */

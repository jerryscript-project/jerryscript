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
extern ecma_object_t *ecma_alloc_object(void);

/**
 * Dealloc memory from an ecma-object
 */
extern void ecma_dealloc_object( ecma_object_t *object_p);

/**
 * Allocate memory for ecma-property
 *
 * @return pointer to allocated memory
 */
extern ecma_property_t *ecma_alloc_property(void);

/**
 * Dealloc memory from an ecma-property
 */
extern void ecma_dealloc_property( ecma_property_t *property_p);

/**
 * Allocate memory for ecma-number
 *
 * @return pointer to allocated memory
 */
extern ecma_number_t *ecma_alloc_number(void);

/**
 * Dealloc memory from an ecma-number
 */
extern void ecma_dealloc_number( ecma_number_t *number_p);

/**
 * Allocate memory for first chunk of an ecma-array
 *
 * @return pointer to allocated memory
 */
extern ecma_array_first_chunk_t *ecma_alloc_array_first_chunk(void);

/**
 * Dealloc memory from first chunk of an ecma-array
 */
extern void ecma_dealloc_array_first_chunk( ecma_array_first_chunk_t *first_chunk_p);

/**
 * Allocate memory for non-first chunk of an ecma-array
 *
 * @return pointer to allocated memory
 */
extern ecma_array_non_first_chunk_t *ecma_alloc_array_non_first_chunk(void);

/**
 * Dealloc memory from non-first chunk of an ecma-array
 */
extern void ecma_dealloc_array_non_first_chunk( ecma_array_non_first_chunk_t *number_p);

#endif /* JERRY_ECMA_ALLOC_H */

/**
 * @}
 * @}
 */

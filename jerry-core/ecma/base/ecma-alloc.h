/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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

#ifndef ECMA_ALLOC_H
#define ECMA_ALLOC_H

#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaalloc Routines for allocation/freeing memory for ECMA data types
 * @{
 */

/**
 * Allocate memory for ecma-object
 *
 * @return pointer to allocated memory
 */
extern ecma_object_t *ecma_alloc_object (void);

/**
 * Dealloc memory from an ecma-object
 */
extern void ecma_dealloc_object (ecma_object_t *);

/**
 * Allocate memory for ecma-number
 *
 * @return pointer to allocated memory
 */
extern ecma_number_t *ecma_alloc_number (void);

/**
 * Dealloc memory from an ecma-number
 */
extern void ecma_dealloc_number (ecma_number_t *);

/**
 * Allocate memory for header of a collection
 *
 * @return pointer to allocated memory
 */
extern ecma_collection_header_t *ecma_alloc_collection_header (void);

/**
 * Dealloc memory from the collection's header
 */
extern void ecma_dealloc_collection_header (ecma_collection_header_t *);

/**
 * Allocate memory for non-first chunk of a collection
 *
 * @return pointer to allocated memory
 */
extern ecma_collection_chunk_t *ecma_alloc_collection_chunk (void);

/**
 * Dealloc memory from non-first chunk of a collection
 */
extern void ecma_dealloc_collection_chunk (ecma_collection_chunk_t *);

/**
 * Allocate memory for ecma-string descriptor
 *
 * @return pointer to allocated memory
 */
extern ecma_string_t *ecma_alloc_string (void);

/**
 * Dealloc memory from ecma-string descriptor
 */
extern void ecma_dealloc_string (ecma_string_t *);

/**
 * Allocate memory for getter-setter pointer pair
 *
 * @return pointer to allocated memory
 */
extern ecma_getter_setter_pointers_t *ecma_alloc_getter_setter_pointers (void);

/**
 * Dealloc memory from getter-setter pointer pair
 */
extern void ecma_dealloc_getter_setter_pointers (ecma_getter_setter_pointers_t *);

/**
* Allocate memory for external pointer
*
* @return pointer to allocated memory
*/
extern ecma_external_pointer_t *ecma_alloc_external_pointer (void);

/**
* Dealloc memory from external pointer
*/
extern void ecma_dealloc_external_pointer (ecma_external_pointer_t *);

/*
 * Allocate memory for extended object
 *
 * @return pointer to allocated memory
 */
extern ecma_extended_object_t *ecma_alloc_extended_object (void);

/**
 * Dealloc memory of an extended object
 */
extern void ecma_dealloc_extended_object (ecma_extended_object_t *);

/**
 * Allocate memory for ecma-property pair
 *
 * @return pointer to allocated memory
 */
extern ecma_property_pair_t *ecma_alloc_property_pair (void);

/**
 * Dealloc memory from an ecma-property pair
 */
extern void ecma_dealloc_property_pair (ecma_property_pair_t *);

/**
 * @}
 * @}
 */

#endif /* !ECMA_ALLOC_H */

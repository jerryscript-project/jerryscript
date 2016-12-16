/* Copyright JS Foundation and other contributors, http://js.foundation
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
ecma_object_t *ecma_alloc_object (void);

/**
 * Dealloc memory from an ecma-object
 */
void ecma_dealloc_object (ecma_object_t *object_p);

/**
 * Allocate memory for ecma-number
 *
 * @return pointer to allocated memory
 */
ecma_number_t *ecma_alloc_number (void);

/**
 * Dealloc memory from an ecma-number
 */
void ecma_dealloc_number (ecma_number_t *number_p);

/**
 * Allocate memory for header of a collection
 *
 * @return pointer to allocated memory
 */
ecma_collection_header_t *ecma_alloc_collection_header (void);

/**
 * Dealloc memory from the collection's header
 */
void ecma_dealloc_collection_header (ecma_collection_header_t *collection_header_p);

/**
 * Allocate memory for non-first chunk of a collection
 *
 * @return pointer to allocated memory
 */
ecma_collection_chunk_t *ecma_alloc_collection_chunk (void);

/**
 * Dealloc memory from non-first chunk of a collection
 */
void ecma_dealloc_collection_chunk (ecma_collection_chunk_t *collection_chunk_p);

/**
 * Allocate memory for ecma-string descriptor
 *
 * @return pointer to allocated memory
 */
ecma_string_t *ecma_alloc_string (void);

/**
 * Dealloc memory from ecma-string descriptor
 */
void ecma_dealloc_string (ecma_string_t *string_p);

/**
 * Allocate memory for getter-setter pointer pair
 *
 * @return pointer to allocated memory
 */
ecma_getter_setter_pointers_t *ecma_alloc_getter_setter_pointers (void);

/**
 * Dealloc memory from getter-setter pointer pair
 */
void ecma_dealloc_getter_setter_pointers (ecma_getter_setter_pointers_t *getter_setter_pointers_p);

/**
* Allocate memory for external pointer
*
* @return pointer to allocated memory
*/
ecma_external_pointer_t *ecma_alloc_external_pointer (void);

/**
* Dealloc memory from external pointer
*/
void ecma_dealloc_external_pointer (ecma_external_pointer_t *external_pointer_p);

/*
 * Allocate memory for extended object
 *
 * @return pointer to allocated memory
 */
ecma_extended_object_t *ecma_alloc_extended_object (size_t size);

/**
 * Dealloc memory of an extended object
 */
void ecma_dealloc_extended_object (ecma_extended_object_t *ext_object_p, size_t size);

/**
 * Allocate memory for ecma-property pair
 *
 * @return pointer to allocated memory
 */
ecma_property_pair_t *ecma_alloc_property_pair (void);

/**
 * Dealloc memory from an ecma-property pair
 */
void ecma_dealloc_property_pair (ecma_property_pair_t *property_pair_p);

/**
 * @}
 * @}
 */

#endif /* !ECMA_ALLOC_H */

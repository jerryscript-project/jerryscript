/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#include "mem-allocator.h"

#ifndef ECMA_COMPRESSED_POINTERS_H
#define ECMA_COMPRESSED_POINTERS_H

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmacompressedpointers Helpers for operations with compressed heap pointers
 * @{
 */

/**
 * Get value of pointer from specified non-null compressed pointer field.
 */
#define ECMA_GET_NON_NULL_POINTER(type, field) \
  ((type *) mem_decompress_pointer (field))

/**
 * Get value of pointer from specified compressed pointer field.
 */
#define ECMA_GET_POINTER(type, field) \
  (((unlikely (field == ECMA_NULL_POINTER)) ? NULL : ECMA_GET_NON_NULL_POINTER (type, field)))

/**
 * Set value of non-null compressed pointer field so that it will correspond
 * to specified non_compressed_pointer.
 */
#define ECMA_SET_NON_NULL_POINTER(field, non_compressed_pointer) \
  (field) = (mem_compress_pointer (non_compressed_pointer) & ((1u << ECMA_POINTER_FIELD_WIDTH) - 1))

/**
 * Set value of compressed pointer field so that it will correspond
 * to specified non_compressed_pointer.
 */
#define ECMA_SET_POINTER(field, non_compressed_pointer) \
  do \
  { \
    auto __temp_pointer = non_compressed_pointer; \
    non_compressed_pointer = __temp_pointer; \
  } while (0); \
  \
  (field) = (unlikely ((non_compressed_pointer) == NULL) ? ECMA_NULL_POINTER \
                                                         : (mem_compress_pointer (non_compressed_pointer) \
                                                            & ((1u << ECMA_POINTER_FIELD_WIDTH) - 1)))

/**
 * @}
 * @}
 */

#endif /* ECMA_COMPRESSED_POINTERS_H */

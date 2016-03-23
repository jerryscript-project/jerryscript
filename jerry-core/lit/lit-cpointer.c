/* Copyright 2015 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged
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

#include "lit-cpointer.h"
#include "jrt-bit-fields.h"

/**
 * Compress pointer to extended compressed pointer.
 *
 * @return dynamic storage-specific extended compressed pointer
 */
inline lit_cpointer_t __attr_pure___ __attr_always_inline___
lit_cpointer_compress (lit_record_t *pointer) /**< pointer to compress */
{
  if (pointer == NULL)
  {
    return MEM_CP_NULL;
  }

  return (lit_cpointer_t) mem_compress_pointer (pointer);
} /* lit_cpointer_compress */

/**
 * Decompress extended compressed pointer.
 *
 * @return decompressed pointer
 */
inline lit_record_t * __attr_pure___ __attr_always_inline___
lit_cpointer_decompress (lit_cpointer_t compressed_pointer) /**< recordset-specific compressed pointer */
{
  if (compressed_pointer == MEM_CP_NULL)
  {
    return NULL;
  }

  return (lit_record_t *) mem_decompress_pointer (compressed_pointer);
} /* lit_cpointer_decompress */

/**
 * Create NULL compressed pointer.
 *
 * @return NULL compressed pointer
 */
inline lit_cpointer_t __attr_pure___ __attr_always_inline___
lit_cpointer_null_cp (void)
{
  return MEM_CP_NULL;
} /* lit_cpointer_null_cp */

/* Copyright 2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged
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

#include "rcs-cpointer.h"
#include "jrt-bit-fields.h"

/**
 * Compress pointer to extended compressed pointer.
 *
 * @return dynamic storage-specific extended compressed pointer
 */
rcs_cpointer_t
rcs_cpointer_compress (rcs_record_t *pointer) /**< pointer to compress */
{
  rcs_cpointer_t cpointer;
  cpointer.packed_value = 0;

  uintptr_t base_pointer = JERRY_ALIGNDOWN ((uintptr_t) pointer, MEM_ALIGNMENT);

  if ((void *) base_pointer == NULL)
  {
    cpointer.value.base_cp = MEM_CP_NULL;
  }
  else
  {
    cpointer.value.base_cp = mem_compress_pointer ((void *) base_pointer) & MEM_CP_MASK;
  }

#if MEM_ALIGNMENT_LOG > RCS_DYN_STORAGE_LENGTH_UNIT_LOG
  /*
   * If alignment of a unit in recordset storage is less than required by MEM_ALIGNMENT_LOG,
   * then mem_cpointer_t can't store pointer to the unit, and so, rcs_cpointer_t stores
   * mem_cpointer_t to block, aligned to MEM_ALIGNMENT, and also extension with difference
   * between positions of the MEM_ALIGNMENT-aligned block and the unit.
   */
  uintptr_t diff = (uintptr_t) pointer - base_pointer;

  JERRY_ASSERT (diff < MEM_ALIGNMENT);
  JERRY_ASSERT (jrt_extract_bit_field (diff, 0, RCS_DYN_STORAGE_LENGTH_UNIT_LOG) == 0);

  uintptr_t ext_part = (uintptr_t) jrt_extract_bit_field (diff,
                                                          RCS_DYN_STORAGE_LENGTH_UNIT_LOG,
                                                          MEM_ALIGNMENT_LOG - RCS_DYN_STORAGE_LENGTH_UNIT_LOG);

  cpointer.value.ext = ext_part & ((1ull << (MEM_ALIGNMENT_LOG - RCS_DYN_STORAGE_LENGTH_UNIT_LOG)) - 1);
#endif /* MEM_ALIGNMENT > RCS_DYN_STORAGE_LENGTH_UNIT_LOG */
  JERRY_ASSERT (rcs_cpointer_decompress (cpointer) == pointer);

  return cpointer;
} /* rcs_cpointer_compress */

/**
 * Decompress extended compressed pointer.
 *
 * @return decompressed pointer
 */
rcs_record_t *
rcs_cpointer_decompress (rcs_cpointer_t compressed_pointer) /**< recordset-specific compressed pointer */
{
  uint8_t *base_pointer = NULL;

  if (compressed_pointer.value.base_cp != MEM_CP_NULL)
  {
    base_pointer = (uint8_t *) mem_decompress_pointer (compressed_pointer.value.base_cp);
  }

  uintptr_t diff = 0;
#if MEM_ALIGNMENT_LOG > RCS_DYN_STORAGE_LENGTH_UNIT_LOG
  /*
   * See also:
   *     rcs_cpointer_compress
   */

  diff = (uintptr_t) compressed_pointer.value.ext << RCS_DYN_STORAGE_LENGTH_UNIT_LOG;
#endif /* MEM_ALIGNMENT_LOG > RCS_DYN_STORAGE_LENGTH_UNIT_LOG */
  rcs_record_t *rec_p = (rcs_record_t *) (base_pointer + diff);

  return rec_p;
} /* rcs_cpointer_decompress */

/**
 * Create NULL compressed pointer.
 *
 * @return NULL compressed pointer
 */
rcs_cpointer_t rcs_cpointer_null_cp (void)
{
  rcs_cpointer_t cp;
  cp.packed_value = MEM_CP_NULL;
  return cp;
} /* rcs_cpointer_null_cp */

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

#ifndef RCS_CPOINTER_H
#define RCS_CPOINTER_H

#include "rcs-globals.h"

#define RCS_CPOINTER_WIDTH (MEM_CP_WIDTH + MEM_ALIGNMENT_LOG - RCS_DYN_STORAGE_LENGTH_UNIT_LOG)

/**
 * Dynamic storage-specific extended compressed pointer
 *
 * Note:
 *      the pointer can represent addresses aligned by RCS_DYN_STORAGE_LENGTH_UNIT,
 *      while mem_cpointer_t can only represent addresses aligned by MEM_ALIGNMENT.
 */
typedef struct
{
  union
  {
    struct
    {
      mem_cpointer_t base_cp : MEM_CP_WIDTH; /**< pointer to base of addressed area */
#if MEM_ALIGNMENT_LOG > RCS_DYN_STORAGE_LENGTH_UNIT_LOG
      uint16_t ext : (MEM_ALIGNMENT_LOG - RCS_DYN_STORAGE_LENGTH_UNIT_LOG); /**< extension of the basic
                                                                             *   compressed pointer
                                                                             *   used for more detailed
                                                                             *   addressing */
#endif /* MEM_ALIGNMENT_LOG > RCS_DYN_STORAGE_LENGTH_UNIT_LOG */
    } value;
    uint16_t packed_value;
  };
} rcs_cpointer_t;

extern rcs_cpointer_t rcs_cpointer_compress (rcs_record_t *);
extern rcs_record_t *rcs_cpointer_decompress (rcs_cpointer_t);
extern rcs_cpointer_t rcs_cpointer_null_cp ();

#endif /* !RCS_CPOINTER_H */

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

#ifndef LIT_CPOINTER_H
#define LIT_CPOINTER_H

#include "lit-literal-storage.h"
#include "jmem-allocator.h"

#define LIT_CPOINTER_WIDTH (JMEM_CP_WIDTH + JMEM_ALIGNMENT_LOG - JMEM_ALIGNMENT_LOG)

/**
 * Dynamic storage-specific extended compressed pointer
 *
 * Note:
 *      the pointer can represent addresses aligned by lit_DYN_STORAGE_LENGTH_UNIT,
 *      while jmem_cpointer_t can only represent addresses aligned by JMEM_ALIGNMENT.
 */
typedef uint16_t lit_cpointer_t;

extern lit_cpointer_t lit_cpointer_compress (lit_record_t *);
extern lit_record_t *lit_cpointer_decompress (lit_cpointer_t);
extern lit_cpointer_t lit_cpointer_null_cp ();

#endif /* !LIT_CPOINTER_H */

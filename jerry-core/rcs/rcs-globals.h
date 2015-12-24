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

#ifndef RCS_GLOBALS_H
#define RCS_GLOBALS_H

#include "rcs-chunked-list.h"

/**
 * Represents the type of the record.
 */
typedef enum
{
  RCS_RECORD_TYPE_FREE = 0, /**< Free record that marks an empty space. It doesn't hold any values. */
  RCS_RECORD_TYPE_CHARSET = 1, /**< Charset record that holds characters. */
  RCS_RECORD_TYPE_MAGIC_STR = 2, /**< Magic string record that holds a magic string id. */
  RCS_RECORD_TYPE_MAGIC_STR_EX = 3, /**< External magic string record that holds an extrernal magic string id. */
  RCS_RECORD_TYPE_NUMBER = 4 /**< Number record that holds a numeric value. */
} rcs_record_type_t;

/**
 * Record type
 */
typedef uint8_t rcs_record_t;

/**
 * Recordset type
 */
typedef rcs_chunked_list_t rcs_record_set_t;

/**
 * Logarithm of a dynamic storage unit alignment
 */
#define RCS_DYN_STORAGE_LENGTH_UNIT_LOG (2u)

/**
 * Unit of length
 */
#define RCS_DYN_STORAGE_LENGTH_UNIT ((size_t) (1ull << RCS_DYN_STORAGE_LENGTH_UNIT_LOG))

#endif /* !RCS_GLOBALS_H */

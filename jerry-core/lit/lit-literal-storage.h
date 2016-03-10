/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
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

#ifndef LIT_LITERAL_STORAGE_H
#define LIT_LITERAL_STORAGE_H

#include "lit-magic-strings.h"
#include "lit-globals.h"
#include "ecma-globals.h"

/**
 * Represents the type of the record.
 */
typedef enum
{
  LIT_RECORD_TYPE_FREE = 0, /**< Free record that marks an empty space. It doesn't hold any values. */
  LIT_RECORD_TYPE_CHARSET = 1, /**< Charset record that holds characters. */
  LIT_RECORD_TYPE_MAGIC_STR = 2, /**< Magic string record that holds a magic string id. */
  LIT_RECORD_TYPE_MAGIC_STR_EX = 3, /**< External magic string record that holds an extrernal magic string id. */
  LIT_RECORD_TYPE_NUMBER = 4 /**< Number record that holds a numeric value. */
} lit_record_type_t;

/**
 * Record header
 */
typedef struct
{
  uint16_t next; /* Compressed pointer to next record */
  uint8_t type; /* Type of record */
} lit_record_t;

/**
 * Head pointer to literal storage
 */
extern lit_record_t *lit_storage;

typedef lit_record_t *lit_literal_t;

/**
 * Charset record header
 *
 * String is stored after the header.
 */
typedef struct
{
  uint16_t next; /* Compressed pointer to next record */
  uint8_t type; /* Type of record */
  uint8_t hash; /* Hash of the string */
  uint16_t size; /* Size of the string in bytes */
  uint16_t length; /* Number of character in the string */
} lit_charset_record_t;

/**
 * Number record header
 */
typedef struct
{
  uint16_t next; /* Compressed pointer to next record */
  uint8_t type; /* Type of record */
  ecma_number_t number; /* Number stored in the record */
} lit_number_record_t;

/**
 * Magic record header
 */
typedef struct
{
  uint16_t next; /* Compressed pointer to next record */
  uint8_t type; /* Type of record */
  uint32_t magic_id; /* Magic ID stored in the record */
} lit_magic_record_t;

#define LIT_CHARSET_HEADER_SIZE (sizeof(lit_charset_record_t))

extern lit_record_t *lit_create_charset_literal (const lit_utf8_byte_t *, const lit_utf8_size_t);
extern lit_record_t *lit_create_magic_literal (const lit_magic_string_id_t);
extern lit_record_t *lit_create_magic_literal_ex (const lit_magic_string_ex_id_t);
extern lit_record_t *lit_create_number_literal (const ecma_number_t);
extern lit_record_t *lit_free_literal (lit_record_t *);
extern size_t lit_get_literal_size (const lit_record_t *);

extern uint32_t lit_count_literals ();

#ifdef JERRY_ENABLE_LOG
extern void lit_dump_literals ();
#endif /* JERRY_ENABLE_LOG */

#define LIT_RECORD_IS_CHARSET(lit) (((lit_record_t *) lit)->type == LIT_RECORD_TYPE_CHARSET)
#define LIT_RECORD_IS_MAGIC_STR(lit) (((lit_record_t *) lit)->type == LIT_RECORD_TYPE_MAGIC_STR)
#define LIT_RECORD_IS_MAGIC_STR_EX(lit) (((lit_record_t *) lit)->type == LIT_RECORD_TYPE_MAGIC_STR_EX)
#define LIT_RECORD_IS_NUMBER(lit) (((lit_record_t *) lit)->type == LIT_RECORD_TYPE_NUMBER)

#endif /* !LIT_LITERAL_STORAGE_H */

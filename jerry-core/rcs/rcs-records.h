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

#ifndef RCS_RECORDS_H
#define RCS_RECORDS_H

#include "ecma-globals.h"

#define RCS_RECORD_TYPE_FIRST                    RCS_RECORD_TYPE_CHARSET
#define RCS_RECORD_TYPE_LAST                     RCS_RECORD_TYPE_NUMBER
#define RCS_RECORD_TYPE_MIN                      RCS_RECORD_TYPE_FREE
#define RCS_RECORD_TYPE_MAX                      RCS_RECORD_TYPE_NUMBER

#define RCS_RECORD_TYPE_IS_FREE(type)            ((type) == RCS_RECORD_TYPE_FREE)
#define RCS_RECORD_TYPE_IS_NUMBER(type)          ((type) == RCS_RECORD_TYPE_NUMBER)
#define RCS_RECORD_TYPE_IS_CHARSET(type)         ((type) == RCS_RECORD_TYPE_CHARSET)
#define RCS_RECORD_TYPE_IS_MAGIC_STR(type)       ((type) == RCS_RECORD_TYPE_MAGIC_STR)
#define RCS_RECORD_TYPE_IS_MAGIC_STR_EX(type)    ((type) == RCS_RECORD_TYPE_MAGIC_STR_EX)
#define RCS_RECORD_TYPE_IS_VALID(type)           ((type) <= RCS_RECORD_TYPE_MAX)

#define RCS_RECORD_IS_FREE(rec)                  (RCS_RECORD_TYPE_IS_FREE (rcs_record_get_type (rec)))
#define RCS_RECORD_IS_NUMBER(rec)                (RCS_RECORD_TYPE_IS_NUMBER (rcs_record_get_type (rec)))
#define RCS_RECORD_IS_CHARSET(rec)               (RCS_RECORD_TYPE_IS_CHARSET (rcs_record_get_type (rec)))
#define RCS_RECORD_IS_MAGIC_STR(rec)             (RCS_RECORD_TYPE_IS_MAGIC_STR (rcs_record_get_type (rec)))
#define RCS_RECORD_IS_MAGIC_STR_EX(rec)          (RCS_RECORD_TYPE_IS_MAGIC_STR_EX (rcs_record_get_type (rec)))

/**
 * Common header informations.
 */
#define RCS_HEADER_TYPE_POS                      0u
#define RCS_HEADER_TYPE_WIDTH                    4u

#define RCS_HEADER_FIELD_BEGIN_POS               (RCS_HEADER_TYPE_POS + RCS_HEADER_TYPE_WIDTH)

/**
 * Number record
 * Doesn't hold any characters, holds a number.
 * Numbers from source code are represented as number literals.
 *
 * Layout:
 * ------- header -----------------------
 * type (4 bits)
 * padding  (12 bits)
 * pointer to prev (16 bits)
 * --------------------------------------
 * ecma_number_t
 */
#define RCS_NUMBER_HEADER_SIZE                   RCS_DYN_STORAGE_LENGTH_UNIT
#define RCS_NUMBER_HEADER_PREV_POS               (RCS_HEADER_FIELD_BEGIN_POS + 12u)

/**
 * Charset record
 *
 * layout:
 * ------- header -----------------------
 * type (4 bits)
 * alignment (2 bits)
 * unused (2 bits)
 * hash (8 bits)
 * length (16 bits)
 * pointer to prev (16 bits)
 * ------- characters -------------------
 * ...
 * chars
 * ....
 * ------- alignment bytes --------------
 * unused bytes (their count is specified
 * by 'alignment' field in header)
 * --------------------------------------
 */
#define RCS_CHARSET_HEADER_SIZE                  (RCS_DYN_STORAGE_LENGTH_UNIT + RCS_DYN_STORAGE_LENGTH_UNIT / 2)

#define RCS_CHARSET_HEADER_ALIGN_POS             RCS_HEADER_FIELD_BEGIN_POS
#define RCS_CHARSET_HEADER_ALIGN_WIDTH           RCS_DYN_STORAGE_LENGTH_UNIT_LOG

#define RCS_CHARSET_HEADER_UNUSED_POS            (RCS_CHARSET_HEADER_ALIGN_POS + RCS_CHARSET_HEADER_ALIGN_WIDTH)
#define RCS_CHARSET_HEADER_UNUSED_WIDTH          2u

#define RCS_CHARSET_HEADER_HASH_POS              (RCS_CHARSET_HEADER_UNUSED_POS + RCS_CHARSET_HEADER_UNUSED_WIDTH)
#define RCS_CHARSET_HEADER_HASH_WIDTH            8u

#define RCS_CHARSET_HEADER_LENGTH_POS            (RCS_CHARSET_HEADER_HASH_POS + RCS_CHARSET_HEADER_HASH_WIDTH)
#define RCS_CHARSET_HEADER_LENGTH_WIDTH          16u

#define RCS_CHARSET_HEADER_PREV_POS              (RCS_CHARSET_HEADER_LENGTH_POS + RCS_CHARSET_HEADER_LENGTH_WIDTH)

/**
 * Magic string record
 * Doesn't hold any characters. Corresponding string is identified by its id.
 *
 * Layout:
 * ------- header -----------------------
 * type (4 bits)
 * magic string id  (12 bits)
 * pointer to prev (16 bits)
 * --------------------------------------
 */
#define RCS_MAGIC_STR_HEADER_SIZE                RCS_DYN_STORAGE_LENGTH_UNIT

#define RCS_MAGIC_STR_HEADER_ID_POS              RCS_HEADER_FIELD_BEGIN_POS
#define RCS_MAGIC_STR_HEADER_ID_WIDTH            12u

#define RCS_MAGIC_STR_HEADER_PREV_POS            (RCS_MAGIC_STR_HEADER_ID_POS + RCS_MAGIC_STR_HEADER_ID_WIDTH)

/**
 * Free record
 * Doesn't hold any data.
 *
 * Layout:
 * ------- header -----------------------
 * type (4 bits)
 * length (12 bits)
 * pointer to prev (16 bits)
 * --------------------------------------
 */
#define RCS_FREE_HEADER_SIZE                     RCS_DYN_STORAGE_LENGTH_UNIT

#define RCS_FREE_HEADER_LENGTH_POS               RCS_HEADER_FIELD_BEGIN_POS
#define RCS_FREE_HEADER_LENGTH_WIDTH             (14u - RCS_DYN_STORAGE_LENGTH_UNIT_LOG)

#define RCS_FREE_HEADER_PREV_POS                 (RCS_FREE_HEADER_LENGTH_POS + RCS_FREE_HEADER_LENGTH_WIDTH)

/*
 *  Setters
 */
extern void rcs_record_set_type (rcs_record_t *, rcs_record_type_t);
extern void rcs_record_set_prev (rcs_record_set_t *, rcs_record_t *, rcs_record_t *);
extern void rcs_record_set_size (rcs_record_t *, size_t);
extern void rcs_record_set_alignment_bytes_count (rcs_record_t *, size_t);
extern void rcs_record_set_hash (rcs_record_t *, lit_string_hash_t);
extern void rcs_record_set_charset (rcs_record_set_t *, rcs_record_t *, const lit_utf8_byte_t *, lit_utf8_size_t);
extern void rcs_record_set_magic_str_id (rcs_record_t *, lit_magic_string_id_t);
extern void rcs_record_set_magic_str_ex_id (rcs_record_t *, lit_magic_string_ex_id_t);

/*
 *  Getters
 */
extern rcs_record_type_t rcs_record_get_type (rcs_record_t *);
extern rcs_record_t *rcs_record_get_prev (rcs_record_set_t *, rcs_record_t *);
extern size_t rcs_record_get_size (rcs_record_t *);
extern size_t rcs_header_get_size (rcs_record_t *);
extern size_t rcs_record_get_alignment_bytes_count (rcs_record_t *);
extern lit_string_hash_t rcs_record_get_hash (rcs_record_t *);
extern lit_utf8_size_t rcs_record_get_length (rcs_record_t *);
extern lit_utf8_size_t rcs_record_get_charset (rcs_record_set_t *, rcs_record_t *, const lit_utf8_byte_t *, size_t);
extern lit_magic_string_id_t rcs_record_get_magic_str_id (rcs_record_t *);
extern lit_magic_string_ex_id_t rcs_record_get_magic_str_ex_id (rcs_record_t *);
extern ecma_number_t rcs_record_get_number (rcs_record_set_t *, rcs_record_t *);

extern rcs_record_t *rcs_record_get_first (rcs_record_set_t *);
extern rcs_record_t *rcs_record_get_next (rcs_record_set_t *, rcs_record_t *);

extern bool rcs_record_is_equal (rcs_record_set_t *, rcs_record_t *, rcs_record_t *);
extern bool rcs_record_is_equal_charset (rcs_record_set_t *, rcs_record_t *, const lit_utf8_byte_t *, lit_utf8_size_t);

#endif /* !RCS_RECORDS_H */

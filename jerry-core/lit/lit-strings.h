/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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

#ifndef LIT_STRINGS_H
#define LIT_STRINGS_H

#include "jrt.h"
#include "lit-globals.h"

/**
 * Null character (used in few cases as utf-8 string end marker)
 */
#define LIT_BYTE_NULL (0)

/**
 * For the formal definition of Unicode transformation formats (UTF) see Section 3.9, Unicode Encoding Forms in The
 * Unicode Standard (http://www.unicode.org/versions/Unicode3.0.0/ch03.pdf#G7404).
 */
#define LIT_UNICODE_CODE_POINT_NULL (0x0)
#define LIT_UNICODE_CODE_POINT_MAX (0x10FFFF)

#define LIT_UTF16_CODE_UNIT_MAX (0xFFFF)
#define LIT_UTF16_FIRST_SURROGATE_CODE_POINT (0x10000)
#define LIT_UTF16_LOW_SURROGATE_MARKER (0xDC00)
#define LIT_UTF16_HIGH_SURROGATE_MARKER (0xD800)
#define LIT_UTF16_HIGH_SURROGATE_MIN (0xD800)
#define LIT_UTF16_HIGH_SURROGATE_MAX (0xDBFF)
#define LIT_UTF16_LOW_SURROGATE_MIN (0xDC00)
#define LIT_UTF16_LOW_SURROGATE_MAX (0xDFFF)
#define LIT_UTF16_BITS_IN_SURROGATE (10)
#define LIT_UTF16_LAST_10_BITS_MASK (0x3FF)

#define LIT_UTF8_1_BYTE_MARKER (0x00)
#define LIT_UTF8_2_BYTE_MARKER (0xC0)
#define LIT_UTF8_3_BYTE_MARKER (0xE0)
#define LIT_UTF8_4_BYTE_MARKER (0xF0)
#define LIT_UTF8_5_BYTE_MARKER (0xF8)
#define LIT_UTF8_EXTRA_BYTE_MARKER (0x80)

#define LIT_UTF8_1_BYTE_MASK (0x80)
#define LIT_UTF8_2_BYTE_MASK (0xE0)
#define LIT_UTF8_3_BYTE_MASK (0xF0)
#define LIT_UTF8_4_BYTE_MASK (0xF8)
#define LIT_UTF8_EXTRA_BYTE_MASK (0xC0)

#define LIT_UTF8_LAST_7_BITS_MASK (0x7F)
#define LIT_UTF8_LAST_6_BITS_MASK (0x3F)
#define LIT_UTF8_LAST_5_BITS_MASK (0x1F)
#define LIT_UTF8_LAST_4_BITS_MASK (0x0F)
#define LIT_UTF8_LAST_3_BITS_MASK (0x07)
#define LIT_UTF8_LAST_2_BITS_MASK (0x03)
#define LIT_UTF8_LAST_1_BIT_MASK  (0x01)

#define LIT_UTF8_BITS_IN_EXTRA_BYTES (6)

#define LIT_UTF8_1_BYTE_CODE_POINT_MAX (0x7F)
#define LIT_UTF8_2_BYTE_CODE_POINT_MIN (0x80)
#define LIT_UTF8_2_BYTE_CODE_POINT_MAX (0x7FF)
#define LIT_UTF8_3_BYTE_CODE_POINT_MIN (0x800)
#define LIT_UTF8_3_BYTE_CODE_POINT_MAX (LIT_UTF16_CODE_UNIT_MAX)
#define LIT_UTF8_4_BYTE_CODE_POINT_MIN (0x10000)
#define LIT_UTF8_4_BYTE_CODE_POINT_MAX (LIT_UNICODE_CODE_POINT_MAX)

/**
 * Differnce between byte count needed to represent code point greater than 0xFFFF
 * in common UTF-8 (4 bytes required) and CESU-8 (6 bytes required)
 */
#define LIT_UTF8_CESU8_SURROGATE_SIZE_DIF (2 * LIT_UTF8_MAX_BYTES_IN_CODE_UNIT - LIT_UTF8_MAX_BYTES_IN_CODE_POINT)

/**
 * Byte values >= LIT_UTF8_FIRST_BYTE_MAX are not allowed in internal strings
 */
#define LIT_UTF8_FIRST_BYTE_MAX LIT_UTF8_5_BYTE_MARKER

/* validation */
bool lit_is_utf8_string_valid (const lit_utf8_byte_t *, lit_utf8_size_t);
bool lit_is_cesu8_string_valid (const lit_utf8_byte_t *, lit_utf8_size_t);

/* checks */
bool lit_is_code_point_utf16_low_surrogate (lit_code_point_t);
bool lit_is_code_point_utf16_high_surrogate (lit_code_point_t);

/* size */
lit_utf8_size_t lit_zt_utf8_string_size (const lit_utf8_byte_t *);

/* length */
ecma_length_t lit_utf8_string_length (const lit_utf8_byte_t *, lit_utf8_size_t);

/* hash */
lit_string_hash_t lit_utf8_string_calc_hash (const lit_utf8_byte_t *, lit_utf8_size_t);
lit_string_hash_t lit_utf8_string_hash_combine (lit_string_hash_t, const lit_utf8_byte_t *, lit_utf8_size_t);

/* code unit access */
ecma_char_t lit_utf8_string_code_unit_at (const lit_utf8_byte_t *, lit_utf8_size_t, ecma_length_t);
lit_utf8_size_t lit_get_unicode_char_size_by_utf8_first_byte (lit_utf8_byte_t);

/* conversion */
lit_utf8_size_t lit_code_unit_to_utf8 (ecma_char_t, lit_utf8_byte_t *);
lit_utf8_size_t lit_code_point_to_utf8 (lit_code_point_t, lit_utf8_byte_t *);
lit_utf8_size_t lit_code_point_to_cesu8 (lit_code_point_t, lit_utf8_byte_t *);
lit_code_point_t lit_convert_surrogate_pair_to_code_point (ecma_char_t, ecma_char_t);

bool lit_compare_utf8_strings (const lit_utf8_byte_t *, lit_utf8_size_t,
                               const lit_utf8_byte_t *, lit_utf8_size_t);

bool lit_compare_utf8_strings_relational (const lit_utf8_byte_t *string1_p, lit_utf8_size_t,
                                          const lit_utf8_byte_t *string2_p, lit_utf8_size_t);

/* read code point from buffer */
lit_utf8_size_t lit_read_code_point_from_utf8 (const lit_utf8_byte_t *, lit_utf8_size_t, lit_code_point_t *);

lit_utf8_size_t lit_read_code_unit_from_utf8 (const lit_utf8_byte_t *,
                                              ecma_char_t *);

lit_utf8_size_t lit_read_prev_code_unit_from_utf8 (const lit_utf8_byte_t *,
                                                   ecma_char_t *);

ecma_char_t lit_utf8_read_next (const lit_utf8_byte_t **);
ecma_char_t lit_utf8_read_prev (const lit_utf8_byte_t **);
ecma_char_t lit_utf8_peek_next (const lit_utf8_byte_t *);
ecma_char_t lit_utf8_peek_prev (const lit_utf8_byte_t *);
void lit_utf8_incr (const lit_utf8_byte_t **);
void lit_utf8_decr (const lit_utf8_byte_t **);

#endif /* !LIT_STRINGS_H */

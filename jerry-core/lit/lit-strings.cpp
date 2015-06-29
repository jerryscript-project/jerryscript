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

#include "lit-strings.h"

#include "jrt-libc-includes.h"

/**
 * For the formal definition of Unicode transformation formats (UTF) see Section 3.9, Unicode Encoding Forms in The
 * Unicode Standard (http://www.unicode.org/versions/Unicode7.0.0/ch03.pdf#G7404, tables 3-6, 3-7).
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
#define LIT_UTF8_4_BYTE_CODE_POINT_MIN (0x1000)
#define LIT_UTF8_4_BYTE_CODE_POINT_MAX (LIT_UNICODE_CODE_POINT_MAX)

/**
 * Validate utf-8 string
 *
 * NOTE:
 *   Isolated surrogates are allowed.
 *   Correct pair of surrogates is not allowed, it should be represented as 4-byte utf-8 character.
 *
 * @return true if utf-8 string is well-formed
 *         false otherwise
 */
bool
lit_is_utf8_string_valid (const lit_utf8_byte_t *utf8_buf_p, /**< utf-8 string */
                          lit_utf8_size_t buf_size) /**< string size */
{
  lit_utf8_size_t idx = 0;

  bool is_prev_code_point_high_surrogate = false;
  while (idx < buf_size)
  {
    lit_utf8_byte_t c = utf8_buf_p[idx++];
    if ((c & LIT_UTF8_1_BYTE_MASK) == LIT_UTF8_1_BYTE_MARKER)
    {
      continue;
    }

    lit_code_point_t code_point = 0;
    lit_code_point_t min_code_point = 0;
    lit_utf8_size_t extra_bytes_count;
    if ((c & LIT_UTF8_2_BYTE_MASK) == LIT_UTF8_2_BYTE_MARKER)
    {
      extra_bytes_count = 1;
      min_code_point = LIT_UTF8_2_BYTE_CODE_POINT_MIN;
      code_point = ((uint32_t) (c & LIT_UTF8_LAST_5_BITS_MASK));
    }
    else if ((c & LIT_UTF8_3_BYTE_MASK) == LIT_UTF8_3_BYTE_MARKER)
    {
      extra_bytes_count = 2;
      min_code_point = LIT_UTF8_3_BYTE_CODE_POINT_MIN;
      code_point = ((uint32_t) (c & LIT_UTF8_LAST_4_BITS_MASK));
    }
    else if ((c & LIT_UTF8_4_BYTE_MASK) == LIT_UTF8_4_BYTE_MARKER)
    {
      extra_bytes_count = 3;
      min_code_point = LIT_UTF8_4_BYTE_CODE_POINT_MIN;
      code_point = ((uint32_t) (c & LIT_UTF8_LAST_3_BITS_MASK));
    }
    else
    {
      /* utf-8 string could not contain 5- and 6-byte sequences. */
      return false;
    }

    if (idx + extra_bytes_count > buf_size)
    {
      /* utf-8 string breaks in the middle */
      return false;
    }

    for (lit_utf8_size_t offset = 0; offset < extra_bytes_count; ++offset)
    {
      c = utf8_buf_p[idx + offset];
      if ((c & LIT_UTF8_EXTRA_BYTE_MASK) != LIT_UTF8_EXTRA_BYTE_MARKER)
      {
        /* invalid continuation byte */
        return false;
      }
      code_point <<= LIT_UTF8_BITS_IN_EXTRA_BYTES;
      code_point |= (c & LIT_UTF8_LAST_6_BITS_MASK);
    }

    if (code_point < min_code_point
        || code_point > LIT_UNICODE_CODE_POINT_MAX)
    {
      /* utf-8 string doesn't encode valid unicode code point */
      return false;
    }

    if (code_point >= LIT_UTF16_HIGH_SURROGATE_MIN
        && code_point <= LIT_UTF16_HIGH_SURROGATE_MAX)
    {
      is_prev_code_point_high_surrogate = true;
    }
    else if (code_point >= LIT_UTF16_LOW_SURROGATE_MIN
             && code_point <= LIT_UTF16_LOW_SURROGATE_MAX
             && is_prev_code_point_high_surrogate)
    {
      /* sequence of high and low surrogate is not allowed */
      return false;
    }
    else
    {
      is_prev_code_point_high_surrogate = false;
    }

    idx += extra_bytes_count;
  }

  return true;
} /* lit_is_utf8_string_valid */

/**
 * Initialize iterator for traversing utf-8 string as a string of code units
 *
 * @return iterator
 */
lit_utf8_iterator_t
lit_utf8_iterator_create (const lit_utf8_byte_t *utf8_buf_p, /**< utf-8 string */
                          lit_utf8_size_t buf_size) /**< string size */
{
  JERRY_ASSERT (utf8_buf_p || !buf_size);

  lit_utf8_iterator_t buf_iter =
  {
    0,
    buf_size,
    utf8_buf_p,
    0,
  };

  return buf_iter;
} /* lit_utf8_iterator_create */

/**
 * Represents code point (>0xFFFF) as surrogate pair and returns its lower part
 *
 * @return lower code_unit of the surrogate pair
 */
static ecma_char_t
convert_code_point_to_low_surrogate (lit_code_point_t code_point) /**< code point, should be > 0xFFFF */
{
  JERRY_ASSERT (code_point > LIT_UTF16_CODE_UNIT_MAX);

  ecma_char_t code_unit_bits;
  code_unit_bits = (ecma_char_t) (code_point & LIT_UTF16_LAST_10_BITS_MASK);

  return (ecma_char_t) (LIT_UTF16_LOW_SURROGATE_MARKER | code_unit_bits);
} /* convert_code_point_to_low_surrogate */

/**
 * Represents code point (>0xFFFF) as surrogate pair and returns its higher part
 *
 * @return higher code_unit of the surrogate pair
 */
static ecma_char_t
convert_code_point_to_high_surrogate (lit_code_point_t code_point) /**< code point, should be > 0xFFFF */
{
  JERRY_ASSERT (code_point > LIT_UTF16_CODE_UNIT_MAX);
  JERRY_ASSERT (code_point <= LIT_UNICODE_CODE_POINT_MAX);

  ecma_char_t code_unit_bits;
  code_unit_bits = (ecma_char_t) ((code_point - LIT_UTF16_FIRST_SURROGATE_CODE_POINT) >> LIT_UTF16_BITS_IN_SURROGATE);

  return (LIT_UTF16_HIGH_SURROGATE_MARKER | code_unit_bits);
} /* convert_code_point_to_low_surrogate */

/**
 * Get next code unit form the iterated string and increment iterator to point to next code unit
 *
 * @return next code unit
 */
ecma_char_t
lit_utf8_iterator_read_code_unit_and_increment (lit_utf8_iterator_t *buf_iter_p) /**< @in-out: utf-8 string iterator */
{
  JERRY_ASSERT (!lit_utf8_iterator_reached_buffer_end (buf_iter_p));

  if (buf_iter_p->code_point)
  {
    ecma_char_t code_unit = convert_code_point_to_low_surrogate (buf_iter_p->code_point);
    buf_iter_p->code_point = 0;
    return code_unit;
  }

  lit_code_point_t code_point;
  buf_iter_p->buf_offset += lit_read_code_point_from_utf8 (buf_iter_p->buf_p + buf_iter_p->buf_offset,
                                                           buf_iter_p->buf_size - buf_iter_p->buf_offset,
                                                           &code_point);

  if (code_point <= LIT_UTF16_CODE_UNIT_MAX)
  {
    return (ecma_char_t) code_point;
  }
  else
  {
    buf_iter_p->code_point = code_point;
    return convert_code_point_to_high_surrogate (code_point);
  }

  JERRY_ASSERT (false);
  return ECMA_CHAR_NULL;
} /* lit_utf8_iterator_read_code_unit_and_increment */

/**
 * Checks iterator reached end of the string
 *
 * @return true - the whole string was iterated
 *         false - otherwise
 */
bool
lit_utf8_iterator_reached_buffer_end (const lit_utf8_iterator_t *buf_iter_p) /**< utf-8 string iterator */
{
  JERRY_ASSERT (buf_iter_p->buf_offset <= buf_iter_p->buf_size);

  if (buf_iter_p->code_point == LIT_UNICODE_CODE_POINT_NULL && buf_iter_p->buf_offset == buf_iter_p->buf_size)
  {
    return true;
  }

  return false;
} /* lit_utf8_iterator_reached_buffer_end */

/**
 * Calculate size of a zero-terminated utf-8 string
 *
 * NOTE:
 *   string should not contain zero characters in the middel
 *
 * @return size of a string
 */
lit_utf8_size_t
lit_zt_utf8_string_size (const lit_utf8_byte_t *utf8_str_p) /**< zero-terminated utf-8 string */
{
  return (lit_utf8_size_t) strlen ((const char *) utf8_str_p);
} /* lit_zt_utf8_string_size */

/**
 * Calculate length of a utf-8 string
 *
 * @return UTF-16 code units count
 */
ecma_length_t
lit_utf8_string_length (const lit_utf8_byte_t *utf8_buf_p, /**< utf-8 string */
                        lit_utf8_size_t utf8_buf_size) /**< string size */
{
  ecma_length_t length = 0;
  lit_utf8_iterator_t buf_iter = lit_utf8_iterator_create (utf8_buf_p, utf8_buf_size);
  while (!lit_utf8_iterator_reached_buffer_end (&buf_iter))
  {
    lit_utf8_iterator_read_code_unit_and_increment (&buf_iter);
    length++;
  }
  JERRY_ASSERT (lit_utf8_iterator_reached_buffer_end (&buf_iter));

  return length;
} /* lit_utf8_string_length */

/**
 * Decodes a unicode code point from non-empty utf-8-encoded buffer
 *
 * @return number of bytes occupied by code point in the string
 */
lit_utf8_size_t
lit_read_code_point_from_utf8 (const lit_utf8_byte_t *buf_p, /**< buffer with characters */
                               lit_utf8_size_t buf_size, /**< size of the buffer in bytes */
                               lit_code_point_t *code_point) /**< @out: code point */
{
  JERRY_ASSERT (buf_p && buf_size);

  lit_utf8_byte_t c = (uint8_t) buf_p[0];
  if ((c & LIT_UTF8_1_BYTE_MASK) == LIT_UTF8_1_BYTE_MARKER)
  {
    *code_point = (uint32_t) (c & LIT_UTF8_LAST_7_BITS_MASK);
    return 1;
  }

  lit_code_point_t ret = LIT_UNICODE_CODE_POINT_NULL;
  ecma_length_t bytes_count = 0;
  if ((c & LIT_UTF8_2_BYTE_MASK) == LIT_UTF8_2_BYTE_MARKER)
  {
    bytes_count = 2;
    ret = ((uint32_t) (c & LIT_UTF8_LAST_5_BITS_MASK));
  }
  else if ((c & LIT_UTF8_3_BYTE_MASK) == LIT_UTF8_3_BYTE_MARKER)
  {
    bytes_count = 3;
    ret = ((uint32_t) (c & LIT_UTF8_LAST_4_BITS_MASK));
  }
  else if ((c & LIT_UTF8_4_BYTE_MASK) == LIT_UTF8_4_BYTE_MARKER)
  {
    bytes_count = 4;
    ret = ((uint32_t) (c & LIT_UTF8_LAST_3_BITS_MASK));
  }
  else
  {
    JERRY_ASSERT (false);
  }

  JERRY_ASSERT (buf_size >= bytes_count);

  for (uint32_t i = 1; i < bytes_count; ++i)
  {
    ret <<= LIT_UTF8_BITS_IN_EXTRA_BYTES;
    ret |= (buf_p[i] & LIT_UTF8_LAST_6_BITS_MASK);
  }

  *code_point = ret;
  return bytes_count;
} /* lit_read_code_point_from_utf8 */


/**
 * Calculate hash from last LIT_STRING_HASH_LAST_BYTES_COUNT characters from the buffer.
 *
 * @return ecma-string's hash
 */
lit_string_hash_t
lit_utf8_string_calc_hash_last_bytes (const lit_utf8_byte_t *utf8_buf_p, /**< characters buffer */
                                      lit_utf8_size_t utf8_buf_size) /**< number of characters in the buffer */
{
  JERRY_ASSERT (utf8_buf_p != NULL);

  lit_utf8_byte_t byte1 = (utf8_buf_size > 0) ? utf8_buf_p[utf8_buf_size - 1] : 0;
  lit_utf8_byte_t byte2 = (utf8_buf_size > 1) ? utf8_buf_p[utf8_buf_size - 2] : 0;

  uint32_t t1 = (uint32_t) byte1 + (uint32_t) byte2;
  uint32_t t2 = t1 * 0x24418b66;
  uint32_t t3 = (t2 >> 16) ^ (t2 & 0xffffu);
  uint32_t t4 = (t3 >> 8) ^ (t3 & 0xffu);

  return (lit_string_hash_t) t4;
} /* lit_utf8_string_calc_hash_last_bytes */

/**
 * Return code unit at the specified position in string
 *
 * NOTE:
 *   code_unit_offset should be less then string's length
 *
 * @return code unit value
 */
ecma_char_t
lit_utf8_string_code_unit_at (const lit_utf8_byte_t *utf8_buf_p, /**< utf-8 string */
                              lit_utf8_size_t utf8_buf_size, /**< string size in bytes */
                              ecma_length_t code_unit_offset) /**< ofset of a code_unit */
{
  lit_utf8_iterator_t iter = lit_utf8_iterator_create (utf8_buf_p, utf8_buf_size);
  ecma_char_t code_unit;

  do
  {
    JERRY_ASSERT (!lit_utf8_iterator_reached_buffer_end (&iter));
    code_unit = lit_utf8_iterator_read_code_unit_and_increment (&iter);
  }
  while (code_unit_offset--);

  return code_unit;
} /* lit_utf8_string_code_unit_at */

/**
 * Return number of bytes occupied by a unicode character in utf-8 representation
 *
 * @return size of a unicode character in utf-8 format
 */
lit_utf8_size_t
lit_get_unicode_char_size_by_utf8_first_byte (lit_utf8_byte_t first_byte) /**< first byte of a utf-8 byte sequence */
{
  if ((first_byte & LIT_UTF8_1_BYTE_MASK) == LIT_UTF8_1_BYTE_MARKER)
  {
    return 1;
  }
  else if ((first_byte & LIT_UTF8_2_BYTE_MASK) == LIT_UTF8_2_BYTE_MARKER)
  {
    return 2;
  }
  else if ((first_byte & LIT_UTF8_3_BYTE_MASK) == LIT_UTF8_3_BYTE_MARKER)
  {
    return 3;
  }
  else
  {
    JERRY_ASSERT ((first_byte & LIT_UTF8_4_BYTE_MASK) == LIT_UTF8_4_BYTE_MARKER);
    return 4;
  }
} /* lit_get_unicode_char_size_by_utf8_first_byte */

/**
 * Convert code_unit to utf-8 representation
 *
 * @return bytes count, stored required to represent specified code unit
 */
lit_utf8_size_t
lit_code_unit_to_utf8 (ecma_char_t code_unit, /**< code unit */
                       lit_utf8_byte_t *buf_p) /**< buffer where to store the result,
                                                *   its size should be at least MAX_BYTES_IN_CODE_UNIT */
{
  return lit_code_point_to_utf8 (code_unit, buf_p);
} /* lit_code_unit_to_utf8 */

/**
 * Convert code point to utf-8 representation
 *
 * @return bytes count, stored required to represent specified code unit
 */
lit_utf8_size_t
lit_code_point_to_utf8 (lit_code_point_t code_point, /**< code point */
                        lit_utf8_byte_t *buf) /**< buffer where to store the result,
                                              *   its size should be at least 4 bytes */
{
  if (code_point <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
  {
    buf[0] = (lit_utf8_byte_t) code_point;
    return 1;
  }
  else if (code_point <= LIT_UTF8_2_BYTE_CODE_POINT_MAX)
  {
    uint32_t code_point_bits = code_point;
    lit_utf8_byte_t second_byte_bits = (lit_utf8_byte_t) (code_point_bits & LIT_UTF8_LAST_6_BITS_MASK);
    code_point_bits >>= LIT_UTF8_BITS_IN_EXTRA_BYTES;

    lit_utf8_byte_t first_byte_bits = (lit_utf8_byte_t) (code_point_bits & LIT_UTF8_LAST_5_BITS_MASK);
    JERRY_ASSERT (first_byte_bits == code_point_bits);

    buf[0] = LIT_UTF8_2_BYTE_MARKER | first_byte_bits;
    buf[1] = LIT_UTF8_EXTRA_BYTE_MARKER | second_byte_bits;
    return 2;
  }
  else if (code_point <= LIT_UTF8_3_BYTE_CODE_POINT_MAX)
  {
    uint32_t code_point_bits = code_point;
    lit_utf8_byte_t third_byte_bits = (lit_utf8_byte_t) (code_point_bits & LIT_UTF8_LAST_6_BITS_MASK);
    code_point_bits >>= LIT_UTF8_BITS_IN_EXTRA_BYTES;

    lit_utf8_byte_t second_byte_bits = (lit_utf8_byte_t) (code_point_bits & LIT_UTF8_LAST_6_BITS_MASK);
    code_point_bits >>= LIT_UTF8_BITS_IN_EXTRA_BYTES;

    lit_utf8_byte_t first_byte_bits = (lit_utf8_byte_t) (code_point_bits & LIT_UTF8_LAST_4_BITS_MASK);
    JERRY_ASSERT (first_byte_bits == code_point_bits);

    buf[0] = LIT_UTF8_3_BYTE_MARKER | first_byte_bits;
    buf[1] = LIT_UTF8_EXTRA_BYTE_MARKER | second_byte_bits;
    buf[2] = LIT_UTF8_EXTRA_BYTE_MARKER | third_byte_bits;
    return 3;
  }
  else
  {
    JERRY_ASSERT (code_point <= LIT_UTF8_4_BYTE_CODE_POINT_MAX);

    uint32_t code_point_bits = code_point;
    lit_utf8_byte_t fourth_byte_bits = (lit_utf8_byte_t) (code_point_bits & LIT_UTF8_LAST_6_BITS_MASK);
    code_point_bits >>= LIT_UTF8_BITS_IN_EXTRA_BYTES;

    lit_utf8_byte_t third_byte_bits = (lit_utf8_byte_t) (code_point_bits & LIT_UTF8_LAST_6_BITS_MASK);
    code_point_bits >>= LIT_UTF8_BITS_IN_EXTRA_BYTES;

    lit_utf8_byte_t second_byte_bits = (lit_utf8_byte_t) (code_point_bits & LIT_UTF8_LAST_6_BITS_MASK);
    code_point_bits >>= LIT_UTF8_BITS_IN_EXTRA_BYTES;

    lit_utf8_byte_t first_byte_bits = (lit_utf8_byte_t) (code_point_bits & LIT_UTF8_LAST_3_BITS_MASK);
    JERRY_ASSERT (first_byte_bits == code_point_bits);

    buf[0] = LIT_UTF8_4_BYTE_MARKER | first_byte_bits;
    buf[1] = LIT_UTF8_EXTRA_BYTE_MARKER | second_byte_bits;
    buf[2] = LIT_UTF8_EXTRA_BYTE_MARKER | third_byte_bits;
    buf[3] = LIT_UTF8_EXTRA_BYTE_MARKER | fourth_byte_bits;
    return 4;
  }
} /* lit_code_unit_to_utf8 */

/**
 * Compare utf-8 string to utf-8 string
 *
 * @return  true - if strings are equal;
 *          false - otherwise.
 */
bool
lit_compare_utf8_strings (const lit_utf8_byte_t *string1_p, /**< utf-8 string */
                          lit_utf8_size_t string1_size, /**< string size */
                          const lit_utf8_byte_t *string2_p, /**< utf-8 string */
                          lit_utf8_size_t string2_size) /**< string size */
{
  if (string1_size != string2_size)
  {
    return false;
  }

  return memcmp (string1_p, string2_p, string1_size) == 0;
} /* lit_compare_utf8_strings */

/**
 * Relational compare of utf-8 strings
 *
 * First string is less than second string if:
 *  - strings are not equal;
 *  - first string is prefix of second or is lexicographically less than second.
 *
 * @return true - if first string is less than second string,
 *         false - otherwise.
 */
bool lit_compare_utf8_strings_relational (const lit_utf8_byte_t *string1_p, /**< utf-8 string */
                                          lit_utf8_size_t string1_size, /**< string size */
                                          const lit_utf8_byte_t *string2_p, /**< utf-8 string */
                                          lit_utf8_size_t string2_size) /**< string size */
{
  lit_utf8_iterator_t iter1 = lit_utf8_iterator_create (string1_p, string1_size);
  lit_utf8_iterator_t iter2 = lit_utf8_iterator_create (string2_p, string2_size);

  while (!lit_utf8_iterator_reached_buffer_end (&iter1)
         && !lit_utf8_iterator_reached_buffer_end (&iter2))
  {
    ecma_char_t code_point1 = lit_utf8_iterator_read_code_unit_and_increment (&iter1);
    ecma_char_t code_point2 = lit_utf8_iterator_read_code_unit_and_increment (&iter2);
    if (code_point1 < code_point2)
    {
      return true;
    }
    else if (code_point1 > code_point2)
    {
      return false;
    }
  }

  return (lit_utf8_iterator_reached_buffer_end (&iter1) && !lit_utf8_iterator_reached_buffer_end (&iter2));
} /* lit_compare_utf8_strings_relational */

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

#include "ecma-helpers.h"
#include "lit-strings.h"

#include "test-common.h"

// Iterations count
#define test_iters 64

// Subiterations count
#define test_sub_iters 64

int
main (int __attr_unused___ argc,
      char __attr_unused___ **argv)
{
  TEST_INIT ();

  mem_init ();

  /* test lit_is_utf8_string_valid */
  /* Overlong-encoded code point */
  lit_utf8_byte_t invalid_utf8_string_1[] = {0xC0, 0x82};
  JERRY_ASSERT (!lit_is_utf8_string_valid (invalid_utf8_string_1, sizeof (invalid_utf8_string_1)));

  /* Overlong-encoded code point */
  lit_utf8_byte_t invalid_utf8_string_2[] = {0xE0, 0x80, 0x81};
  JERRY_ASSERT (!lit_is_utf8_string_valid (invalid_utf8_string_2, sizeof (invalid_utf8_string_2)));

  /* Pair of surrogates: 0xD901 0xDFF0 which encode Unicode character 0x507F0 */
  lit_utf8_byte_t invalid_utf8_string_3[] = {0xED, 0xA4, 0x81, 0xED, 0xBF, 0xB0};
  JERRY_ASSERT (!lit_is_utf8_string_valid (invalid_utf8_string_3, sizeof (invalid_utf8_string_3)));

  /* Isolated high surrogate 0xD901 */
  lit_utf8_byte_t valid_utf8_string_1[] = {0xED, 0xA4, 0x81};
  JERRY_ASSERT (lit_is_utf8_string_valid (valid_utf8_string_1, sizeof (valid_utf8_string_1)));

  /* 4-byte long utf-8 character - Unicode character 0x507F0 */
  lit_utf8_byte_t valid_utf8_string_2[] = {0xF1, 0x90, 0x9F, 0xB0};
  JERRY_ASSERT (lit_is_utf8_string_valid (valid_utf8_string_2, sizeof (valid_utf8_string_2)));

  /* test lit_read_code_point_from_utf8 */
  lit_utf8_byte_t buf[] = {0xF0, 0x90, 0x8D, 0x88};
  lit_code_point_t code_point;
  lit_utf8_size_t bytes_count = lit_read_code_point_from_utf8 (buf, sizeof (buf), &code_point);
  JERRY_ASSERT (bytes_count == 4);
  JERRY_ASSERT (code_point == 0x10348);

  /* test lit_code_unit_to_utf8 */
  lit_utf8_byte_t res_buf[3];
  lit_utf8_size_t res_size;

  res_size = lit_code_unit_to_utf8 (0x73, res_buf);
  JERRY_ASSERT (res_size == 1);
  JERRY_ASSERT (res_buf[0] == 0x73);

  res_size = lit_code_unit_to_utf8 (0x41A, res_buf);
  JERRY_ASSERT (res_size == 2);
  JERRY_ASSERT (res_buf[0] == 0xD0);
  JERRY_ASSERT (res_buf[1] == 0x9A);

  res_size = lit_code_unit_to_utf8 (0xD7FF, res_buf);
  JERRY_ASSERT (res_size == 3);
  JERRY_ASSERT (res_buf[0] == 0xED);
  JERRY_ASSERT (res_buf[1] == 0x9F);
  JERRY_ASSERT (res_buf[2] == 0xBF);

  /* test lit_utf8_iterator */
  lit_utf8_byte_t bytes[] = {0xF0, 0x90, 0x8D, 0x88};
  lit_utf8_iterator_t iter = lit_utf8_iterator_create (bytes, sizeof (bytes));
  ecma_char_t code_unit = lit_utf8_iterator_read_code_unit_and_increment (&iter);
  JERRY_ASSERT (!lit_utf8_iterator_reached_buffer_end (&iter));
  JERRY_ASSERT (code_unit == 0xD800);
  code_unit = lit_utf8_iterator_read_code_unit_and_increment (&iter);
  JERRY_ASSERT (lit_utf8_iterator_reached_buffer_end (&iter));
  JERRY_ASSERT (code_unit == 0xDF48);

  mem_finalize (true);
  return 0;
}

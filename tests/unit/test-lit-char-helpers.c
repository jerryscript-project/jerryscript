/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
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
#include "ecma-init-finalize.h"
#include "lit-char-helpers.h"
#include "js-parser-internal.h"

#include "test-common.h"

int
main ()
{
  TEST_INIT ();

  jmem_init ();
  ecma_init ();

  const uint8_t _1_byte_long1[] = "\\u007F";
  const uint8_t _1_byte_long2[] = "\\u0000";
  const uint8_t _1_byte_long3[] = "\\u0065";

  const uint8_t _2_byte_long1[] = "\\u008F";
  const uint8_t _2_byte_long2[] = "\\u00FF";
  const uint8_t _2_byte_long3[] = "\\u07FF";

  const uint8_t _3_byte_long1[] = "\\u08FF";
  const uint8_t _3_byte_long2[] = "\\u0FFF";
  const uint8_t _3_byte_long3[] = "\\uFFFF";

  size_t length;

  /* Test 1-byte-long unicode sequences. */
  length = lit_char_get_utf8_length (lexer_hex_to_character (0, _1_byte_long1 + 2, 4));
  TEST_ASSERT (length == 1);

  length = lit_char_get_utf8_length (lexer_hex_to_character (0, _1_byte_long2 + 2, 4));
  TEST_ASSERT (length == 1);

  length = lit_char_get_utf8_length (lexer_hex_to_character (0, _1_byte_long3 + 2, 4));
  TEST_ASSERT (length == 1);

  /* Test 2-byte-long unicode sequences. */
  length = lit_char_get_utf8_length (lexer_hex_to_character (0, _2_byte_long1 + 2, 4));
  TEST_ASSERT (length == 2);

  length = lit_char_get_utf8_length (lexer_hex_to_character (0, _2_byte_long2 + 2, 4));
  TEST_ASSERT (length == 2);

  length = lit_char_get_utf8_length (lexer_hex_to_character (0, _2_byte_long3 + 2, 4));
  TEST_ASSERT (length == 2);

  /* Test 3-byte-long unicode sequences. */
  length = lit_char_get_utf8_length (lexer_hex_to_character (0, _3_byte_long1 + 2, 4));
  TEST_ASSERT (length != 2);

  length = lit_char_get_utf8_length (lexer_hex_to_character (0, _3_byte_long2 + 2, 4));
  TEST_ASSERT (length == 3);

  length = lit_char_get_utf8_length (lexer_hex_to_character (0, _3_byte_long3 + 2, 4));
  TEST_ASSERT (length == 3);

  ecma_finalize ();
  jmem_finalize ();

  return 0;
} /* main */

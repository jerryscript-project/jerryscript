/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include "jerryscript.h"

#include "test-common.h"

static bool
strict_equals (jerry_value_t a, /**< the first string to compare */
               jerry_value_t b) /**< the second string to compare */
{
  const jerry_char_t is_equal_src[] = "var isEqual = function(a, b) { return (a === b); }; isEqual";
  jerry_value_t is_equal_fn_val = jerry_eval (is_equal_src, sizeof (is_equal_src) - 1, JERRY_PARSE_NO_OPTS);
  TEST_ASSERT (!jerry_value_is_exception (is_equal_fn_val));
  jerry_value_t args[2] = { a, b };
  jerry_value_t res = jerry_call (is_equal_fn_val, jerry_undefined (), args, 2);
  TEST_ASSERT (!jerry_value_is_exception (res));
  TEST_ASSERT (jerry_value_is_boolean (res));
  bool is_strict_equal = jerry_value_is_true (res);
  jerry_value_free (res);
  jerry_value_free (is_equal_fn_val);
  return is_strict_equal;
} /* strict_equals */

int
main (void)
{
  jerry_size_t sz, utf8_sz, cesu8_sz;
  jerry_value_t args[2];

  TEST_INIT ();
  jerry_init (JERRY_INIT_EMPTY);

  /* Test corner case for jerry_string_to_char_buffer */
  args[0] = jerry_string_sz ("");
  sz = jerry_string_size (args[0], JERRY_ENCODING_CESU8);
  TEST_ASSERT (sz == 0);
  jerry_value_free (args[0]);

  /* Test create_jerry_string_from_utf8 with 4-byte long unicode sequences,
   * test string: 'str: {DESERET CAPITAL LETTER LONG I}'
   */
  char *utf8_bytes_p = "\x73\x74\x72\x3a \xf0\x90\x90\x80";
  char *cesu8_bytes_p = "\x73\x74\x72\x3a \xed\xa0\x81\xed\xb0\x80";
  args[0] = jerry_string ((jerry_char_t *) utf8_bytes_p, (jerry_size_t) strlen (utf8_bytes_p), JERRY_ENCODING_UTF8);
  args[1] = jerry_string ((jerry_char_t *) cesu8_bytes_p, (jerry_size_t) strlen (cesu8_bytes_p), JERRY_ENCODING_CESU8);

  /* These sizes must be equal */
  utf8_sz = jerry_string_size (args[0], JERRY_ENCODING_CESU8);
  cesu8_sz = jerry_string_size (args[1], JERRY_ENCODING_CESU8);

  JERRY_VLA (jerry_char_t, string_from_utf8, utf8_sz);
  JERRY_VLA (jerry_char_t, string_from_cesu8, cesu8_sz);

  jerry_string_to_buffer (args[0], JERRY_ENCODING_CESU8, string_from_utf8, utf8_sz);
  jerry_string_to_buffer (args[1], JERRY_ENCODING_CESU8, string_from_cesu8, cesu8_sz);

  TEST_ASSERT (utf8_sz == cesu8_sz);

  TEST_ASSERT (!memcmp (string_from_utf8, string_from_cesu8, utf8_sz));
  jerry_value_free (args[0]);
  jerry_value_free (args[1]);

  /* Test jerry_string_to_buffer, test string: 'str: {DESERET CAPITAL LETTER LONG I}' */
  utf8_bytes_p = "\x73\x74\x72\x3a \xf0\x90\x90\x80";
  cesu8_bytes_p = "\x73\x74\x72\x3a \xed\xa0\x81\xed\xb0\x80";
  args[0] = jerry_string ((jerry_char_t *) utf8_bytes_p, (jerry_size_t) strlen (utf8_bytes_p), JERRY_ENCODING_UTF8);
  args[1] = jerry_string ((jerry_char_t *) cesu8_bytes_p, (jerry_size_t) strlen (cesu8_bytes_p), JERRY_ENCODING_CESU8);

  /* Test that the strings are equal / ensure hashes are equal */
  TEST_ASSERT (strict_equals (args[0], args[1]));

  /* These sizes must be equal */
  utf8_sz = jerry_string_size (args[0], JERRY_ENCODING_UTF8);
  cesu8_sz = jerry_string_size (args[1], JERRY_ENCODING_UTF8);

  TEST_ASSERT (utf8_sz == cesu8_sz && utf8_sz > 0);

  JERRY_VLA (jerry_char_t, string_from_utf8_string, utf8_sz);
  JERRY_VLA (jerry_char_t, string_from_cesu8_string, cesu8_sz);

  jerry_string_to_buffer (args[0], JERRY_ENCODING_UTF8, string_from_utf8_string, utf8_sz);
  jerry_string_to_buffer (args[1], JERRY_ENCODING_UTF8, string_from_cesu8_string, cesu8_sz);

  TEST_ASSERT (!memcmp (string_from_utf8_string, string_from_cesu8_string, utf8_sz));
  jerry_value_free (args[0]);
  jerry_value_free (args[1]);

  /* Test string: 'str: {MATHEMATICAL FRAKTUR SMALL F}{MATHEMATICAL FRAKTUR SMALL G}' */
  utf8_bytes_p = "\x73\x74\x72\x3a \xf0\x9d\x94\xa3 \xf0\x9d\x94\xa4";
  args[0] = jerry_string ((jerry_char_t *) utf8_bytes_p, (jerry_size_t) strlen (utf8_bytes_p), JERRY_ENCODING_UTF8);

  cesu8_sz = jerry_string_size (args[0], JERRY_ENCODING_CESU8);
  utf8_sz = jerry_string_size (args[0], JERRY_ENCODING_UTF8);

  TEST_ASSERT (jerry_string_length (args[0]) == 10);
  TEST_ASSERT (cesu8_sz != utf8_sz);
  TEST_ASSERT (utf8_sz == 14 && cesu8_sz == 18);

  JERRY_VLA (char, test_string, utf8_sz);

  TEST_ASSERT (jerry_string_to_buffer (args[0], JERRY_ENCODING_UTF8, (jerry_char_t *) test_string, utf8_sz) == 14);
  TEST_ASSERT (!strncmp (test_string, utf8_bytes_p, utf8_sz));

  jerry_value_free (args[0]);

  /* Test string: 'str: {DESERET CAPITAL LETTER LONG I}' */
  cesu8_bytes_p = "\x73\x74\x72\x3a \xed\xa0\x81\xed\xb0\x80";
  args[0] = jerry_string ((jerry_char_t *) cesu8_bytes_p, (jerry_size_t) strlen (cesu8_bytes_p), JERRY_ENCODING_CESU8);

  cesu8_sz = jerry_string_size (args[0], JERRY_ENCODING_CESU8);
  utf8_sz = jerry_string_size (args[0], JERRY_ENCODING_UTF8);

  TEST_ASSERT (jerry_string_length (args[0]) == 7);
  TEST_ASSERT (cesu8_sz != utf8_sz);
  TEST_ASSERT (utf8_sz == 9 && cesu8_sz == 11);

  jerry_value_free (args[0]);

  /* Test string: 'price: 10{EURO SIGN}' */
  utf8_bytes_p = "\x70\x72\x69\x63\x65\x3a \x31\x30\xe2\x82\xac";
  args[0] = jerry_string ((jerry_char_t *) utf8_bytes_p, (jerry_size_t) strlen (utf8_bytes_p), JERRY_ENCODING_UTF8);

  cesu8_sz = jerry_string_size (args[0], JERRY_ENCODING_CESU8);
  utf8_sz = jerry_string_size (args[0], JERRY_ENCODING_UTF8);

  TEST_ASSERT (jerry_string_length (args[0]) == 10);
  TEST_ASSERT (cesu8_sz == utf8_sz);
  TEST_ASSERT (utf8_sz == 12);
  jerry_value_free (args[0]);

  /* Test string: '3' */
  {
    jerry_value_t test_str = jerry_string_sz ("3");
    char result_string[1] = { 'E' };
    jerry_size_t copied_utf8 =
      jerry_string_to_buffer (test_str, JERRY_ENCODING_UTF8, (jerry_char_t *) result_string, sizeof (result_string));
    TEST_ASSERT (copied_utf8 == 1);
    TEST_ASSERT (result_string[0] == '3');

    result_string[0] = 'E';
    jerry_size_t copied =
      jerry_string_to_buffer (test_str, JERRY_ENCODING_CESU8, (jerry_char_t *) result_string, sizeof (result_string));
    TEST_ASSERT (copied == 1);
    TEST_ASSERT (result_string[0] == '3');

    jerry_value_free (test_str);
  }

  /* Test jerry_string_substr */
  {
    jerry_value_t test_str = jerry_string_sz ("Hello World!");
    jerry_value_t expected_str = jerry_string_sz ("Hello");

    // Read the string into a byte buffer.
    jerry_value_t sub_str = jerry_string_substr (test_str, 0, 5);

    TEST_ASSERT (!strict_equals (sub_str, test_str));
    TEST_ASSERT (strict_equals (sub_str, expected_str));

    jerry_value_free (sub_str);
    jerry_value_free (expected_str);
    jerry_value_free (test_str);
  }

  jerry_cleanup ();

  return 0;
} /* main */

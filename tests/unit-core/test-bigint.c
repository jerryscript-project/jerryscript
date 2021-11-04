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

int
main (void)
{
  if (!jerry_is_feature_enabled (JERRY_FEATURE_BIGINT))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Bigint support is disabled!\n");
    return 0;
  }

  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t string = jerry_create_string ((const jerry_char_t *) "0xfffffff1fffffff2fffffff3");
  TEST_ASSERT (!jerry_value_is_error (string));

  jerry_value_t bigint = jerry_value_to_bigint (string);
  jerry_release_value (string);

  TEST_ASSERT (!jerry_value_is_error (bigint));
  TEST_ASSERT (jerry_value_is_bigint (bigint));

  string = jerry_value_to_string (bigint);
  TEST_ASSERT (!jerry_value_is_error (string));

  static jerry_char_t str_buffer[64];
  const char *expected_string_p = "79228162256009920505775652851";

  jerry_size_t size = jerry_string_to_char_buffer (string, str_buffer, sizeof (str_buffer));
  TEST_ASSERT (size == strlen (expected_string_p));
  TEST_ASSERT (memcmp (str_buffer, expected_string_p, size) == 0);
  jerry_release_value (string);

  TEST_ASSERT (jerry_get_bigint_size_in_digits (bigint) == 2);

  uint64_t digits_buffer[4];
  bool sign;

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = true;
  jerry_get_bigint_digits (bigint, digits_buffer, 0, &sign);
  TEST_ASSERT (sign == false);
  TEST_ASSERT (digits_buffer[0] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[1] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = true;
  jerry_get_bigint_digits (bigint, digits_buffer, 1, &sign);
  TEST_ASSERT (sign == false);
  TEST_ASSERT (digits_buffer[0] == 0xfffffff2fffffff3ull);
  TEST_ASSERT (digits_buffer[1] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = true;
  jerry_get_bigint_digits (bigint, digits_buffer, 2, &sign);
  TEST_ASSERT (sign == false);
  TEST_ASSERT (digits_buffer[0] == 0xfffffff2fffffff3ull);
  TEST_ASSERT (digits_buffer[1] == 0xfffffff1ull);
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = true;
  jerry_get_bigint_digits (bigint, digits_buffer, 3, &sign);
  TEST_ASSERT (sign == false);
  TEST_ASSERT (digits_buffer[0] == 0xfffffff2fffffff3ull);
  TEST_ASSERT (digits_buffer[1] == 0xfffffff1ull);
  TEST_ASSERT (digits_buffer[2] == 0);
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  jerry_get_bigint_digits (bigint, digits_buffer, 4, NULL);
  TEST_ASSERT (digits_buffer[0] == 0xfffffff2fffffff3ull);
  TEST_ASSERT (digits_buffer[1] == 0xfffffff1ull);
  TEST_ASSERT (digits_buffer[2] == 0);
  TEST_ASSERT (digits_buffer[3] == 0);

  jerry_release_value (bigint);

  digits_buffer[0] = 0;
  digits_buffer[1] = 0;
  digits_buffer[2] = 0;
  /* Sign of zero value is always positive, even if we set negative. */
  bigint = jerry_create_bigint (digits_buffer, 3, true);
  TEST_ASSERT (jerry_value_is_bigint (bigint));
  TEST_ASSERT (jerry_get_bigint_size_in_digits (bigint) == 0);

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = true;
  jerry_get_bigint_digits (bigint, digits_buffer, 2, &sign);
  TEST_ASSERT (sign == false);
  TEST_ASSERT (digits_buffer[0] == 0);
  TEST_ASSERT (digits_buffer[1] == 0);
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  jerry_release_value (bigint);

  digits_buffer[0] = 1;
  digits_buffer[1] = 0;
  digits_buffer[2] = 0;
  digits_buffer[3] = 0;
  bigint = jerry_create_bigint (digits_buffer, 4, true);
  TEST_ASSERT (jerry_value_is_bigint (bigint));
  TEST_ASSERT (jerry_get_bigint_size_in_digits (bigint) == 1);

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = false;
  jerry_get_bigint_digits (bigint, digits_buffer, 1, &sign);
  TEST_ASSERT (sign == true);
  TEST_ASSERT (digits_buffer[0] == 1);
  TEST_ASSERT (digits_buffer[1] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = false;
  jerry_get_bigint_digits (bigint, digits_buffer, 2, &sign);
  TEST_ASSERT (sign == true);
  TEST_ASSERT (digits_buffer[0] == 1);
  TEST_ASSERT (digits_buffer[1] == 0);
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  jerry_release_value (bigint);

  digits_buffer[0] = 0;
  digits_buffer[1] = 1;
  digits_buffer[2] = 0;
  digits_buffer[3] = 0;
  bigint = jerry_create_bigint (digits_buffer, 4, true);
  TEST_ASSERT (jerry_value_is_bigint (bigint));
  TEST_ASSERT (jerry_get_bigint_size_in_digits (bigint) == 2);

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = false;
  jerry_get_bigint_digits (bigint, digits_buffer, 1, &sign);
  TEST_ASSERT (sign == true);
  TEST_ASSERT (digits_buffer[0] == 0);
  TEST_ASSERT (digits_buffer[1] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = false;
  jerry_get_bigint_digits (bigint, digits_buffer, 2, &sign);
  TEST_ASSERT (sign == true);
  TEST_ASSERT (digits_buffer[0] == 0);
  TEST_ASSERT (digits_buffer[1] == 1);
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = false;
  jerry_get_bigint_digits (bigint, digits_buffer, 3, &sign);
  TEST_ASSERT (sign == true);
  TEST_ASSERT (digits_buffer[0] == 0);
  TEST_ASSERT (digits_buffer[1] == 1);
  TEST_ASSERT (digits_buffer[2] == 0);
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  jerry_release_value (bigint);

  jerry_cleanup ();
  return 0;
} /* main */

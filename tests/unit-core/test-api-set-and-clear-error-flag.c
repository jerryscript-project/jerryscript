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

static void
compare_str (jerry_value_t value, const char *str_p, size_t str_len)
{
  jerry_size_t size = jerry_string_size (value, JERRY_ENCODING_CESU8);
  TEST_ASSERT (str_len == size);
  JERRY_VLA (jerry_char_t, str_buff, size);
  jerry_string_to_buffer (value, JERRY_ENCODING_CESU8, str_buff, size);
  TEST_ASSERT (!memcmp (str_p, str_buff, str_len));
} /* compare_str */

int
main (void)
{
  TEST_INIT ();

  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t obj_val = jerry_object ();
  obj_val = jerry_throw_value (obj_val, true);
  jerry_value_t err_val = jerry_value_copy (obj_val);

  obj_val = jerry_exception_value (err_val, true);

  TEST_ASSERT (obj_val != err_val);
  jerry_value_free (err_val);
  jerry_value_free (obj_val);

  const char pterodactylus[] = "Pterodactylus";
  const size_t pterodactylus_size = sizeof (pterodactylus) - 1;

  jerry_value_t str = jerry_string_sz (pterodactylus);
  jerry_value_t error = jerry_throw_value (str, true);
  str = jerry_exception_value (error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jerry_value_free (str);

  str = jerry_string_sz (pterodactylus);
  error = jerry_throw_value (str, false);
  jerry_value_free (str);
  str = jerry_exception_value (error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jerry_value_free (str);

  str = jerry_string_sz (pterodactylus);
  error = jerry_throw_abort (str, true);
  str = jerry_exception_value (error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jerry_value_free (str);

  str = jerry_string_sz (pterodactylus);
  error = jerry_throw_abort (str, false);
  jerry_value_free (str);
  str = jerry_exception_value (error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jerry_value_free (str);

  str = jerry_string_sz (pterodactylus);
  error = jerry_throw_value (str, true);
  error = jerry_throw_abort (error, true);
  TEST_ASSERT (jerry_value_is_abort (error));
  str = jerry_exception_value (error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jerry_value_free (str);

  str = jerry_string_sz (pterodactylus);
  error = jerry_throw_value (str, true);
  jerry_value_t error2 = jerry_throw_abort (error, false);
  TEST_ASSERT (jerry_value_is_abort (error2));
  jerry_value_free (error);
  str = jerry_exception_value (error2, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jerry_value_free (str);

  double test_num = 3.1415926;
  jerry_value_t num = jerry_number (test_num);
  jerry_value_t num2 = jerry_throw_value (num, false);
  TEST_ASSERT (jerry_value_is_exception (num2));
  jerry_value_free (num);
  num2 = jerry_exception_value (num2, true);
  TEST_ASSERT (jerry_value_as_number (num2) == test_num);
  jerry_value_free (num2);

  num = jerry_number (test_num);
  num2 = jerry_throw_value (num, true);
  TEST_ASSERT (jerry_value_is_exception (num2));
  num2 = jerry_exception_value (num2, true);
  TEST_ASSERT (jerry_value_as_number (num2) == test_num);
  jerry_value_free (num2);

  num = jerry_number (test_num);
  num2 = jerry_throw_value (num, false);
  TEST_ASSERT (jerry_value_is_exception (num2));
  jerry_value_free (num);
  jerry_value_t num3 = jerry_throw_value (num2, false);
  TEST_ASSERT (jerry_value_is_exception (num3));
  jerry_value_free (num2);
  num2 = jerry_exception_value (num3, true);
  TEST_ASSERT (jerry_value_as_number (num2) == test_num);
  jerry_value_free (num2);

  num = jerry_number (test_num);
  num2 = jerry_throw_value (num, true);
  TEST_ASSERT (jerry_value_is_exception (num2));
  num3 = jerry_throw_value (num2, true);
  TEST_ASSERT (jerry_value_is_exception (num3));
  num2 = jerry_exception_value (num3, true);
  TEST_ASSERT (jerry_value_as_number (num2) == test_num);
  jerry_value_free (num2);

  num = jerry_number (test_num);
  error = jerry_throw_abort (num, true);
  TEST_ASSERT (jerry_value_is_abort (error));
  num2 = jerry_throw_value (error, true);
  TEST_ASSERT (jerry_value_is_exception (num2));
  num = jerry_exception_value (num2, true);
  TEST_ASSERT (jerry_value_as_number (num) == test_num);
  jerry_value_free (num);

  num = jerry_number (test_num);
  error = jerry_throw_abort (num, false);
  jerry_value_free (num);
  TEST_ASSERT (jerry_value_is_abort (error));
  num2 = jerry_throw_value (error, true);
  TEST_ASSERT (jerry_value_is_exception (num2));
  num = jerry_exception_value (num2, true);
  TEST_ASSERT (jerry_value_as_number (num) == test_num);
  jerry_value_free (num);

  num = jerry_number (test_num);
  error = jerry_throw_abort (num, true);
  TEST_ASSERT (jerry_value_is_abort (error));
  num2 = jerry_throw_value (error, false);
  jerry_value_free (error);
  TEST_ASSERT (jerry_value_is_exception (num2));
  num = jerry_exception_value (num2, true);
  TEST_ASSERT (jerry_value_as_number (num) == test_num);
  jerry_value_free (num);

  num = jerry_number (test_num);
  error = jerry_throw_abort (num, false);
  jerry_value_free (num);
  TEST_ASSERT (jerry_value_is_abort (error));
  num2 = jerry_throw_value (error, false);
  jerry_value_free (error);
  TEST_ASSERT (jerry_value_is_exception (num2));
  num = jerry_exception_value (num2, true);
  TEST_ASSERT (jerry_value_as_number (num) == test_num);
  jerry_value_free (num);

  jerry_value_t value = jerry_number (42);
  value = jerry_exception_value (value, true);
  jerry_value_free (value);

  value = jerry_number (42);
  jerry_value_t value2 = jerry_exception_value (value, false);
  jerry_value_free (value);
  jerry_value_free (value2);

  value = jerry_number (42);
  error = jerry_throw_value (value, true);
  error = jerry_throw_value (error, true);
  jerry_value_free (error);

  value = jerry_number (42);
  error = jerry_throw_abort (value, true);
  error = jerry_throw_abort (error, true);
  jerry_value_free (error);

  value = jerry_number (42);
  error = jerry_throw_value (value, true);
  error2 = jerry_throw_value (error, false);
  jerry_value_free (error);
  jerry_value_free (error2);

  value = jerry_number (42);
  error = jerry_throw_abort (value, true);
  error2 = jerry_throw_abort (error, false);
  jerry_value_free (error);
  jerry_value_free (error2);

  jerry_cleanup ();
} /* main */

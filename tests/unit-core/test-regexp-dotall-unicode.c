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
  TEST_INIT ();
  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t undefined_this_arg = jerry_undefined ();
  char pattern2[] = "\\u{61}.\\u{62}";

  uint16_t flags = JERRY_REGEXP_FLAG_DOTALL | JERRY_REGEXP_FLAG_UNICODE | JERRY_REGEXP_FLAG_STICKY;
  jerry_value_t regex_obj = jerry_regexp_sz (pattern2, flags);
  TEST_ASSERT (jerry_value_is_object (regex_obj));

  const jerry_char_t func_src2[] = "return [regex.exec('a\\nb'), regex.dotAll, regex.sticky, regex.unicode ];";

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_HAS_ARGUMENT_LIST;
  parse_options.argument_list = jerry_string_sz ("regex");

  jerry_value_t func_val = jerry_parse (func_src2, sizeof (func_src2) - 1, &parse_options);
  jerry_value_free (parse_options.argument_list);

  jerry_value_t res = jerry_call (func_val, undefined_this_arg, &regex_obj, 1);
  jerry_value_t regex_res = jerry_object_get_index (res, 0);
  jerry_value_t regex_res_str = jerry_object_get_index (regex_res, 0);
  jerry_value_t is_dotall = jerry_object_get_index (res, 1);
  jerry_value_t is_sticky = jerry_object_get_index (res, 2);
  jerry_value_t is_unicode = jerry_object_get_index (res, 3);

  jerry_size_t str_size = jerry_string_size (regex_res_str, JERRY_ENCODING_CESU8);
  JERRY_VLA (jerry_char_t, res_buff, str_size);
  jerry_size_t res_size = jerry_string_to_buffer (regex_res_str, JERRY_ENCODING_CESU8, res_buff, str_size);

  const char expected_result[] = "a\nb";
  TEST_ASSERT (res_size == (sizeof (expected_result) - 1));
  TEST_ASSERT (strncmp (expected_result, (const char *) res_buff, res_size) == 0);
  TEST_ASSERT (jerry_value_is_true (is_dotall));
  TEST_ASSERT (jerry_value_is_true (is_sticky));
  TEST_ASSERT (jerry_value_is_true (is_unicode));

  jerry_value_free (regex_obj);
  jerry_value_free (res);
  jerry_value_free (func_val);
  jerry_value_free (regex_res);
  jerry_value_free (regex_res_str);
  jerry_value_free (is_dotall);
  jerry_value_free (is_sticky);
  jerry_value_free (is_unicode);

  jerry_cleanup ();
  return 0;
} /* main */

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

#include "config.h"
#include "test-common.h"

static void
compare_string (jerry_value_t left_string, /**< left string */
                const char *right_string_p) /**< right string */
{
  size_t size = strlen (right_string_p);
  uint8_t buffer[64];

  TEST_ASSERT (size == jerry_string_size (left_string, JERRY_ENCODING_CESU8));
  TEST_ASSERT (size < sizeof (buffer));
  TEST_ASSERT (jerry_string_to_buffer (left_string, JERRY_ENCODING_CESU8, buffer, (jerry_size_t) size) == size);
  TEST_ASSERT (memcmp (buffer, right_string_p, size) == 0);
} /* compare_string */

int
main (void)
{
  TEST_INIT ();

  jerry_init (JERRY_INIT_EMPTY);

  if (!jerry_feature_enabled (JERRY_FEATURE_FUNCTION_TO_STRING))
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Source code is not stored!\n");
    jerry_cleanup ();
    return 0;
  }

  jerry_value_t value = jerry_null ();
  jerry_source_info_t *source_info_p = jerry_source_info (value);
  TEST_ASSERT (source_info_p == NULL);
  jerry_source_info_free (source_info_p);
  jerry_value_free (value);

  value = jerry_object ();
  source_info_p = jerry_source_info (value);
  TEST_ASSERT (source_info_p == NULL);
  jerry_source_info_free (source_info_p);
  jerry_value_free (value);

  jerry_parse_options_t parse_options;
  const char *source_p = TEST_STRING_LITERAL ("var a = 6");

  value = jerry_parse ((jerry_char_t *) source_p, strlen (source_p), NULL);
  source_info_p = jerry_source_info (value);
  TEST_ASSERT (source_info_p != NULL);
  TEST_ASSERT (source_info_p->enabled_fields == JERRY_SOURCE_INFO_HAS_SOURCE_CODE);
  compare_string (source_info_p->source_code, source_p);
  TEST_ASSERT (jerry_value_is_undefined (source_info_p->function_arguments));
  TEST_ASSERT (source_info_p->source_range_start == 0);
  TEST_ASSERT (source_info_p->source_range_length == 0);
  jerry_source_info_free (source_info_p);
  jerry_value_free (value);

  if (jerry_feature_enabled (JERRY_FEATURE_MODULE))
  {
    parse_options.options = JERRY_PARSE_MODULE;

    value = jerry_parse ((jerry_char_t *) source_p, strlen (source_p), &parse_options);

    jerry_value_t result = jerry_module_link (value, NULL, NULL);
    TEST_ASSERT (!jerry_value_is_exception (result));
    jerry_value_free (result);

    source_info_p = jerry_source_info (value);
    TEST_ASSERT (source_info_p != NULL);
    TEST_ASSERT (source_info_p->enabled_fields == JERRY_SOURCE_INFO_HAS_SOURCE_CODE);
    compare_string (source_info_p->source_code, source_p);
    TEST_ASSERT (jerry_value_is_undefined (source_info_p->function_arguments));
    TEST_ASSERT (source_info_p->source_range_start == 0);
    TEST_ASSERT (source_info_p->source_range_length == 0);
    jerry_source_info_free (source_info_p);

    result = jerry_module_evaluate (value);
    TEST_ASSERT (!jerry_value_is_exception (result));
    jerry_value_free (result);

    /* Byte code is released after a successful evaluation. */
    source_info_p = jerry_source_info (value);
    TEST_ASSERT (source_info_p == NULL);
    jerry_source_info_free (source_info_p);
    jerry_value_free (value);
  }

  source_p = TEST_STRING_LITERAL ("( function f() {} )");

  value = jerry_eval ((const jerry_char_t *) source_p, strlen (source_p), 0);
  source_info_p = jerry_source_info (value);
  TEST_ASSERT (source_info_p != NULL);
  TEST_ASSERT (source_info_p->enabled_fields
               == (JERRY_SOURCE_INFO_HAS_SOURCE_CODE | JERRY_SOURCE_INFO_HAS_SOURCE_RANGE));
  compare_string (source_info_p->source_code, source_p);
  TEST_ASSERT (jerry_value_is_undefined (source_info_p->function_arguments));
  TEST_ASSERT (source_info_p->source_range_start == 2);
  TEST_ASSERT (source_info_p->source_range_length == 15);
  jerry_source_info_free (source_info_p);
  jerry_value_free (value);

  source_p = TEST_STRING_LITERAL ("new Function('a', 'b', 'return 0;')");

  value = jerry_eval ((const jerry_char_t *) source_p, strlen (source_p), 0);
  source_info_p = jerry_source_info (value);
  TEST_ASSERT (source_info_p != NULL);
  TEST_ASSERT (source_info_p->enabled_fields
               == (JERRY_SOURCE_INFO_HAS_SOURCE_CODE | JERRY_SOURCE_INFO_HAS_FUNCTION_ARGUMENTS));
  compare_string (source_info_p->source_code, "return 0;");
  compare_string (source_info_p->function_arguments, "a,b");
  TEST_ASSERT (source_info_p->source_range_start == 0);
  TEST_ASSERT (source_info_p->source_range_length == 0);
  jerry_source_info_free (source_info_p);
  jerry_value_free (value);

  source_p = TEST_STRING_LITERAL ("(new Function('a = ( function() { } )', 'return a;'))()");

  value = jerry_eval ((const jerry_char_t *) source_p, strlen (source_p), 0);
  source_info_p = jerry_source_info (value);
  TEST_ASSERT (source_info_p != NULL);
  TEST_ASSERT (source_info_p->enabled_fields
               == (JERRY_SOURCE_INFO_HAS_SOURCE_CODE | JERRY_SOURCE_INFO_HAS_SOURCE_RANGE));
  compare_string (source_info_p->source_code, "a = ( function() { } )");
  TEST_ASSERT (jerry_value_is_undefined (source_info_p->function_arguments));
  TEST_ASSERT (source_info_p->source_range_start == 6);
  TEST_ASSERT (source_info_p->source_range_length == 14);
  jerry_source_info_free (source_info_p);
  jerry_value_free (value);

  source_p = TEST_STRING_LITERAL ("(function f(a) { return 7 }).bind({})");

  value = jerry_eval ((const jerry_char_t *) source_p, strlen (source_p), 0);
  source_info_p = jerry_source_info (value);
  TEST_ASSERT (source_info_p != NULL);
  TEST_ASSERT (source_info_p->enabled_fields
               == (JERRY_SOURCE_INFO_HAS_SOURCE_CODE | JERRY_SOURCE_INFO_HAS_SOURCE_RANGE));
  compare_string (source_info_p->source_code, source_p);
  TEST_ASSERT (jerry_value_is_undefined (source_info_p->function_arguments));
  TEST_ASSERT (source_info_p->source_range_start == 1);
  TEST_ASSERT (source_info_p->source_range_length == 26);
  jerry_source_info_free (source_info_p);
  jerry_value_free (value);

  jerry_cleanup ();
  return 0;
} /* main */

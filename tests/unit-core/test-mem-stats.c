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
  if (!jerry_feature_enabled (JERRY_FEATURE_HEAP_STATS))
  {
    return 0;
  }
  const jerry_char_t test_source[] = TEST_STRING_LITERAL ("var a = 'hello';"
                                                          "var b = 'world';"
                                                          "var c = a + ' ' + b;");

  jerry_init (JERRY_INIT_EMPTY);
  jerry_value_t parsed_code_val = jerry_parse (test_source, sizeof (test_source) - 1, NULL);
  TEST_ASSERT (!jerry_value_is_exception (parsed_code_val));

  jerry_value_t res = jerry_run (parsed_code_val);
  TEST_ASSERT (!jerry_value_is_exception (res));

  jerry_heap_stats_t stats;
  memset (&stats, 0, sizeof (stats));
  bool get_stats_ret = jerry_heap_stats (&stats);
  TEST_ASSERT (get_stats_ret);
  TEST_ASSERT (stats.version == 1);
  TEST_ASSERT (stats.size == 524280);

  TEST_ASSERT (!jerry_heap_stats (NULL));

  jerry_value_free (res);
  jerry_value_free (parsed_code_val);

  jerry_cleanup ();

  return 0;
} /* main */

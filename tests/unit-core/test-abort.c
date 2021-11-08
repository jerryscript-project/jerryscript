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

static jerry_value_t
callback_func (const jerry_call_info_t *call_info_p, const jerry_value_t args_p[], const jerry_length_t args_count)
{
  JERRY_UNUSED (call_info_p);
  JERRY_UNUSED (args_p);
  JERRY_UNUSED (args_count);

  jerry_value_t value = jerry_string_sz ("Abort run!");
  value = jerry_throw_abort (value, true);
  return value;
} /* callback_func */

int
main (void)
{
  TEST_INIT ();

  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t global = jerry_current_realm ();
  jerry_value_t callback_name = jerry_string_sz ("callback");
  jerry_value_t func = jerry_function_external (callback_func);
  jerry_value_t res = jerry_object_set (global, callback_name, func);
  TEST_ASSERT (!jerry_value_is_exception (res));

  jerry_value_free (res);
  jerry_value_free (func);
  jerry_value_free (callback_name);
  jerry_value_free (global);

  const jerry_char_t inf_loop_code_src1[] = TEST_STRING_LITERAL ("while(true) {\n"
                                                                 "  with ({}) {\n"
                                                                 "    try {\n"
                                                                 "      callback();\n"
                                                                 "    } catch (e) {\n"
                                                                 "    } finally {\n"
                                                                 "    }\n"
                                                                 "  }\n"
                                                                 "}");

  jerry_value_t parsed_code_val = jerry_parse (inf_loop_code_src1, sizeof (inf_loop_code_src1) - 1, NULL);

  TEST_ASSERT (!jerry_value_is_exception (parsed_code_val));
  res = jerry_run (parsed_code_val);

  TEST_ASSERT (jerry_value_is_abort (res));

  jerry_value_free (res);
  jerry_value_free (parsed_code_val);

  const jerry_char_t inf_loop_code_src2[] = TEST_STRING_LITERAL ("function f() {"
                                                                 "  while(true) {\n"
                                                                 "    with ({}) {\n"
                                                                 "      try {\n"
                                                                 "        callback();\n"
                                                                 "      } catch (e) {\n"
                                                                 "      } finally {\n"
                                                                 "      }\n"
                                                                 "    }\n"
                                                                 "  }"
                                                                 "}\n"
                                                                 "function g() {\n"
                                                                 "  for (a in { x:5 })\n"
                                                                 "    f();\n"
                                                                 "}\n"
                                                                 "\n"
                                                                 "with({})\n"
                                                                 " f();\n");

  parsed_code_val = jerry_parse (inf_loop_code_src2, sizeof (inf_loop_code_src2) - 1, NULL);

  TEST_ASSERT (!jerry_value_is_exception (parsed_code_val));
  res = jerry_run (parsed_code_val);

  TEST_ASSERT (jerry_value_is_abort (res));

  jerry_value_free (res);
  jerry_value_free (parsed_code_val);

  /* Test flag overwrites. */
  jerry_value_t value = jerry_string_sz ("Error description");
  TEST_ASSERT (!jerry_value_is_abort (value));
  TEST_ASSERT (!jerry_value_is_exception (value));

  value = jerry_throw_abort (value, true);
  TEST_ASSERT (jerry_value_is_abort (value));
  TEST_ASSERT (jerry_value_is_exception (value));

  value = jerry_throw_value (value, true);
  TEST_ASSERT (!jerry_value_is_abort (value));
  TEST_ASSERT (jerry_value_is_exception (value));

  value = jerry_throw_abort (value, true);
  TEST_ASSERT (jerry_value_is_abort (value));
  TEST_ASSERT (jerry_value_is_exception (value));

  jerry_value_free (value);

  jerry_cleanup ();
  return 0;
} /* main */

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
vm_exec_stop_callback (void *user_p)
{
  int *int_p = (int *) user_p;

  if (*int_p > 0)
  {
    (*int_p)--;

    return jerry_undefined ();
  }

  return jerry_string_sz ("Abort script");
} /* vm_exec_stop_callback */

int
main (void)
{
  TEST_INIT ();

  /* Test stopping an infinite loop. */
  if (!jerry_feature_enabled (JERRY_FEATURE_VM_EXEC_STOP))
  {
    return 0;
  }

  jerry_init (JERRY_INIT_EMPTY);

  int countdown = 6;
  jerry_halt_handler (16, vm_exec_stop_callback, &countdown);

  const jerry_char_t inf_loop_code_src1[] = "while(true) {}";
  jerry_value_t parsed_code_val = jerry_parse (inf_loop_code_src1, sizeof (inf_loop_code_src1) - 1, NULL);

  TEST_ASSERT (!jerry_value_is_exception (parsed_code_val));
  jerry_value_t res = jerry_run (parsed_code_val);
  TEST_ASSERT (countdown == 0);

  TEST_ASSERT (jerry_value_is_exception (res));

  jerry_value_free (res);
  jerry_value_free (parsed_code_val);

  /* A more complex example. Although the callback error is captured
   * by the catch block, it is automatically thrown again. */

  /* We keep the callback function, only the countdown is reset. */
  countdown = 6;

  const jerry_char_t inf_loop_code_src2[] = TEST_STRING_LITERAL ("function f() { while (true) ; }\n"
                                                                 "try { f(); } catch(e) {}");

  parsed_code_val = jerry_parse (inf_loop_code_src2, sizeof (inf_loop_code_src2) - 1, NULL);

  TEST_ASSERT (!jerry_value_is_exception (parsed_code_val));
  res = jerry_run (parsed_code_val);
  TEST_ASSERT (countdown == 0);

  /* The result must have an error flag which proves that
   * the error is thrown again inside the catch block. */
  TEST_ASSERT (jerry_value_is_exception (res));

  jerry_value_free (res);
  jerry_value_free (parsed_code_val);

  jerry_cleanup ();
  return 0;
} /* main */

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

#include "config.h"
#include "jerryscript.h"
#include "test-common.h"

static const char *identifying_string = "identifying string";
static bool user_context_new_was_called = false;
static bool user_context_free_was_called = false;

static void *
user_context_new (void)
{
  user_context_new_was_called = true;
  return (void *) identifying_string;
} /* user_context_new */

static void
user_context_free (void *user_context_p)
{
  user_context_free_was_called = true;
  TEST_ASSERT (((const char *) user_context_p) == identifying_string);
} /* user_context_free */

int
main (void)
{
  TEST_INIT ();

  jerry_init_with_user_context (JERRY_INIT_EMPTY, user_context_new, user_context_free);

  TEST_ASSERT ((((const char *)(jerry_get_user_context ()))) == identifying_string);

  jerry_cleanup ();

  TEST_ASSERT (user_context_new_was_called);
  TEST_ASSERT (user_context_free_was_called);

  return 0;
} /* main */

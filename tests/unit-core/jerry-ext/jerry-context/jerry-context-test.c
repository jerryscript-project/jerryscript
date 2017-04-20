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
#include "jerry-context.h"

static char *static_slot1 = "static slot 1";
static char *static_slot2 = "static slot 2";

static void *
init_slot1 (void)
{
  return static_slot1;
} /* init_slot1 */

static void
deinit_slot1 (void *slot)
{
  TEST_ASSERT (slot == static_slot1);
} /* deinit_slot1 */

static void *
init_slot2 (void)
{
  return static_slot2;
} /* init_slot2 */

static void
deinit_slot2 (void *slot)
{
  TEST_ASSERT (slot == static_slot2);
} /* deinit_slot2 */

int
main (int argc, char **argv)
{
  (void) argc;
  (void) argv;
  TEST_INIT ();

  int slot1 = jerryx_context_request_slot (init_slot1, deinit_slot1);
  int slot2 = jerryx_context_request_slot (init_slot2, deinit_slot2);
  int slot3 = jerryx_context_request_slot (init_slot2, deinit_slot2);

  TEST_ASSERT (slot1 == 0);
  TEST_ASSERT (slot2 == 1);
  TEST_ASSERT (slot3 == -1);

  jerry_init_with_user_context (JERRY_INIT_EMPTY, jerryx_context_init, jerryx_context_deinit);

  TEST_ASSERT (jerryx_context_get_slot (slot1) == static_slot1);
  TEST_ASSERT (jerryx_context_get_slot (slot2) == static_slot2);

  jerry_cleanup ();
} /* main */

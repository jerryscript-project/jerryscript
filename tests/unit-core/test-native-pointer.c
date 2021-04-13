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

static int global_int = 4;
static void *global_p = (void *) &global_int;
static int global_counter = 0;

static void
native_free_callback (void *native_p, /**< native pointer */
                      jerry_object_native_info_t *info_p) /**< native info */
{
  (void) native_p;
  TEST_ASSERT (info_p->free_cb == native_free_callback);
  global_counter++;
} /* native_free_callback */

static const jerry_object_native_info_t native_info_1 =
{
  .free_cb = native_free_callback,
};

static const jerry_object_native_info_t native_info_2 =
{
  .free_cb = NULL,
};

static void
check_native_info (jerry_value_t object_value, /**< object value */
                   const jerry_object_native_info_t *native_info_p, /**< native info */
                   void *expected_pointer_p) /**< expected pointer */
{
  void *native_pointer_p;
  TEST_ASSERT (jerry_get_object_native_pointer (object_value, &native_pointer_p, native_info_p));
  TEST_ASSERT (native_pointer_p == expected_pointer_p);
} /* check_native_info */

int
main (void)
{
  TEST_INIT ();
  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t object_value = jerry_create_object ();

  jerry_set_object_native_pointer (object_value, global_p, &native_info_1);
  jerry_set_object_native_pointer (object_value, NULL, &native_info_2);

  check_native_info (object_value, &native_info_1, global_p);
  check_native_info (object_value, &native_info_2, NULL);

  jerry_release_value (object_value);

  jerry_gc (JERRY_GC_PRESSURE_HIGH);
  TEST_ASSERT (global_counter == 1);
  global_counter = 0;

  object_value = jerry_create_object ();

  jerry_set_object_native_pointer (object_value, global_p, &native_info_1);
  jerry_set_object_native_pointer (object_value, NULL, &native_info_2);

  TEST_ASSERT (jerry_delete_object_native_pointer (object_value, &native_info_1));

  TEST_ASSERT (!jerry_get_object_native_pointer (object_value, NULL, &native_info_1));
  check_native_info (object_value, &native_info_2, NULL);

  TEST_ASSERT (!jerry_delete_object_native_pointer (object_value, &native_info_1));

  TEST_ASSERT (!jerry_get_object_native_pointer (object_value, NULL, &native_info_1));
  check_native_info (object_value, &native_info_2, NULL);

  TEST_ASSERT (jerry_delete_object_native_pointer (object_value, &native_info_2));

  TEST_ASSERT (!jerry_get_object_native_pointer (object_value, NULL, &native_info_1));
  TEST_ASSERT (!jerry_get_object_native_pointer (object_value, NULL, &native_info_2));

  jerry_set_object_native_pointer (object_value, NULL, &native_info_1);

  check_native_info (object_value, &native_info_1, NULL);
  TEST_ASSERT (!jerry_get_object_native_pointer (object_value, NULL, &native_info_2));

  jerry_set_object_native_pointer (object_value, global_p, &native_info_2);

  check_native_info (object_value, &native_info_1, NULL);
  check_native_info (object_value, &native_info_2, global_p);

  jerry_set_object_native_pointer (object_value, global_p, &native_info_1);

  check_native_info (object_value, &native_info_1, global_p);
  check_native_info (object_value, &native_info_2, global_p);

  TEST_ASSERT (jerry_delete_object_native_pointer (object_value, &native_info_1));
  TEST_ASSERT (jerry_delete_object_native_pointer (object_value, &native_info_2));

  TEST_ASSERT (!jerry_get_object_native_pointer (object_value, NULL, &native_info_1));
  TEST_ASSERT (!jerry_get_object_native_pointer (object_value, NULL, &native_info_2));

  jerry_release_value (object_value);

  jerry_cleanup ();

  TEST_ASSERT (global_counter == 0);
  return 0;
} /* main */

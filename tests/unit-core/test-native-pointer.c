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
  .number_of_references = 0,
  .offset_of_references = 0,
};

static const jerry_object_native_info_t native_info_2 =
{
  .free_cb = NULL,
  .number_of_references = 0,
  .offset_of_references = 0,
};

static const jerry_object_native_info_t native_info_3 =
{
  .free_cb = NULL,
  .number_of_references = 0,
  .offset_of_references = 0,
};

typedef struct
{
  uint32_t check_before;
  jerry_value_t a;
  jerry_value_t b;
  jerry_value_t c;
  uint32_t check_after;
} test_references_t;

static test_references_t test_references1;
static test_references_t test_references2;
static test_references_t test_references3;
static test_references_t test_references4;
static int call_count = 0;

static void
native_references_free_callback (void *native_p, /**< native pointer */
                                 jerry_object_native_info_t *info_p) /**< native info */
{
  test_references_t *refs_p = (test_references_t *) native_p;

  TEST_ASSERT ((refs_p == &test_references1 && test_references1.check_before == 0x12345678)
               || (refs_p == &test_references2 && test_references2.check_before == 0x87654321)
               || (refs_p == &test_references3 && test_references3.check_before == 0x12344321));
  TEST_ASSERT (refs_p->check_before == refs_p->check_after);

  uint32_t check = refs_p->check_before;

  jerry_native_pointer_release_references (native_p, info_p);

  TEST_ASSERT (jerry_value_is_undefined (refs_p->a));
  TEST_ASSERT (jerry_value_is_undefined (refs_p->b));
  TEST_ASSERT (jerry_value_is_undefined (refs_p->c));
  TEST_ASSERT (refs_p->check_before == check);
  TEST_ASSERT (refs_p->check_after == check);

  call_count++;
} /* native_references_free_callback */

static const jerry_object_native_info_t native_info_4 =
{
  .free_cb = native_references_free_callback,
  .number_of_references = 3,
  .offset_of_references = (uint16_t) offsetof (test_references_t, a),
};

static void
init_references (test_references_t *refs_p, /**< native pointer */
                 uint32_t check) /**< value for memory check */
{
  /* Memory garbage */
  refs_p->check_before = check;
  refs_p->a = 1;
  refs_p->b = 2;
  refs_p->c = 3;
  refs_p->check_after = check;

  jerry_native_pointer_init_references ((void *) refs_p, &native_info_4);

  TEST_ASSERT (jerry_value_is_undefined (refs_p->a));
  TEST_ASSERT (jerry_value_is_undefined (refs_p->b));
  TEST_ASSERT (jerry_value_is_undefined (refs_p->c));
  TEST_ASSERT (refs_p->check_before == check);
  TEST_ASSERT (refs_p->check_after == check);
} /* init_references */

static void
set_references (test_references_t *refs_p, /**< native pointer */
                jerry_value_t value1, /**< first value to be set */
                jerry_value_t value2, /**< second value to be set */
                jerry_value_t value3) /**< third value to be set */
{
  jerry_native_pointer_set_reference (&refs_p->a, value1);
  jerry_native_pointer_set_reference (&refs_p->b, value2);
  jerry_native_pointer_set_reference (&refs_p->c, value3);

  TEST_ASSERT (jerry_value_is_object (value1) ? jerry_value_is_object (refs_p->a)
                                              : jerry_value_is_string (refs_p->a));
  TEST_ASSERT (jerry_value_is_object (value2) ? jerry_value_is_object (refs_p->b)
                                              : jerry_value_is_string (refs_p->b));
  TEST_ASSERT (jerry_value_is_object (value3) ? jerry_value_is_object (refs_p->c)
                                              : jerry_value_is_string (refs_p->c));
} /* set_references */

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

  jerry_set_object_native_pointer (object_value, global_p, &native_info_1);
  jerry_set_object_native_pointer (object_value, NULL, &native_info_2);
  jerry_set_object_native_pointer (object_value, global_p, &native_info_3);

  check_native_info (object_value, &native_info_1, global_p);
  check_native_info (object_value, &native_info_2, NULL);
  check_native_info (object_value, &native_info_3, global_p);

  TEST_ASSERT (jerry_delete_object_native_pointer (object_value, &native_info_1));
  TEST_ASSERT (jerry_delete_object_native_pointer (object_value, &native_info_2));
  TEST_ASSERT (jerry_delete_object_native_pointer (object_value, &native_info_3));

  TEST_ASSERT (!jerry_get_object_native_pointer (object_value, NULL, &native_info_1));
  TEST_ASSERT (!jerry_get_object_native_pointer (object_value, NULL, &native_info_2));
  TEST_ASSERT (!jerry_get_object_native_pointer (object_value, NULL, &native_info_3));

  jerry_set_object_native_pointer (object_value, NULL, &native_info_1);
  jerry_set_object_native_pointer (object_value, global_p, &native_info_2);
  jerry_set_object_native_pointer (object_value, NULL, &native_info_3);

  check_native_info (object_value, &native_info_1, NULL);
  check_native_info (object_value, &native_info_2, global_p);
  check_native_info (object_value, &native_info_3, NULL);

  /* Reversed delete order. */
  TEST_ASSERT (jerry_delete_object_native_pointer (object_value, &native_info_3));
  TEST_ASSERT (jerry_delete_object_native_pointer (object_value, &native_info_2));
  TEST_ASSERT (jerry_delete_object_native_pointer (object_value, &native_info_1));

  TEST_ASSERT (!jerry_get_object_native_pointer (object_value, NULL, &native_info_1));
  TEST_ASSERT (!jerry_get_object_native_pointer (object_value, NULL, &native_info_2));
  TEST_ASSERT (!jerry_get_object_native_pointer (object_value, NULL, &native_info_3));

  /* Test value references */
  jerry_value_t string1_value = jerry_create_string ((const jerry_char_t *) "String1");
  jerry_value_t string2_value = jerry_create_string ((const jerry_char_t *) "String2");

  jerry_value_t object1_value = jerry_create_object ();
  jerry_value_t object2_value = jerry_create_object ();

  init_references (&test_references1, 0x12345678);
  init_references (&test_references2, 0x87654321);

  jerry_set_object_native_pointer (object1_value, (void *) &test_references1, &native_info_4);
  jerry_set_object_native_pointer (object2_value, (void *) &test_references2, &native_info_4);

  /* Assign values (cross reference between object1 and object2). */
  set_references (&test_references1, string1_value, object2_value, string2_value);
  set_references (&test_references2, string2_value, object1_value, string1_value);

  jerry_gc (JERRY_GC_PRESSURE_HIGH);

  /* Reassign values. */
  set_references (&test_references1, object2_value, string2_value, string1_value);
  set_references (&test_references2, object1_value, string1_value, string2_value);

  jerry_gc (JERRY_GC_PRESSURE_HIGH);

  jerry_release_value (object1_value);
  jerry_release_value (object2_value);

  object1_value = jerry_create_object ();
  object2_value = jerry_create_object ();

  init_references (&test_references3, 0x12344321);

  /* Assign the same native pointer to multiple objects. */
  jerry_set_object_native_pointer (object1_value, (void *) &test_references3, &native_info_4);
  jerry_set_object_native_pointer (object2_value, (void *) &test_references3, &native_info_4);

  set_references (&test_references3, object1_value, object2_value, string1_value);

  jerry_gc (JERRY_GC_PRESSURE_HIGH);

  init_references (&test_references4, 0x87655678);

  /* Re-assign reference */
  jerry_set_object_native_pointer (object1_value, (void *) &test_references4, &native_info_4);

  set_references (&test_references4, string1_value, string2_value, string1_value);

  jerry_set_object_native_pointer (object1_value, NULL, &native_info_4);

  jerry_native_pointer_release_references ((void *) &test_references4, &native_info_4);

  /* Calling jerry_native_pointer_init_references with test_references4 is optional here. */

  jerry_set_object_native_pointer (object1_value, (void *) &test_references4, &native_info_4);

  set_references (&test_references4, string2_value, string1_value, string2_value);

  TEST_ASSERT (jerry_delete_object_native_pointer (object1_value, &native_info_4));

  jerry_native_pointer_release_references ((void *) &test_references4, &native_info_4);

  jerry_release_value (object1_value);
  jerry_release_value (object2_value);

  /* Delete references */
  for (int i = 0; i < 3; i++)
  {
    object1_value = jerry_create_object ();

    jerry_set_object_native_pointer (object1_value, global_p, NULL);
    jerry_set_object_native_pointer (object1_value, (void *) &test_references4, &native_info_4);
    jerry_set_object_native_pointer (object1_value, global_p, &native_info_2);
    set_references (&test_references4, string1_value, string2_value, object1_value);

    jerry_gc (JERRY_GC_PRESSURE_HIGH);

    if (i == 1)
    {
      TEST_ASSERT (jerry_delete_object_native_pointer (object1_value, NULL));
    }
    else if (i == 2)
    {
      TEST_ASSERT (jerry_delete_object_native_pointer (object1_value, &native_info_2));
    }

    TEST_ASSERT (jerry_delete_object_native_pointer (object1_value, &native_info_4));
    jerry_native_pointer_release_references ((void *) &test_references4, &native_info_4);
    jerry_release_value (object1_value);
  }

  jerry_release_value (string1_value);
  jerry_release_value (string2_value);

  jerry_release_value (object_value);

  jerry_cleanup ();

  TEST_ASSERT (global_counter == 0);
  TEST_ASSERT (call_count == 3);
  return 0;
} /* main */

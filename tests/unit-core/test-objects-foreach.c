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

static bool
count_objects (jerry_value_t object, void *user_arg)
{
  (void) object;
  TEST_ASSERT (user_arg != NULL);

  int *counter = (int *) user_arg;

  (*counter)++;
  return true;
} /* count_objects */

static void
test_container (void)
{
  jerry_value_t global = jerry_current_realm ();
  jerry_value_t map_str = jerry_string_sz ("Map");
  jerry_value_t map_result = jerry_object_get (global, map_str);
  jerry_type_t type = jerry_value_type (map_result);

  jerry_value_free (map_result);
  jerry_value_free (map_str);
  jerry_value_free (global);

  /* If there is no Map function this is not an es.next profile build, skip this test case. */
  if (type != JERRY_TYPE_FUNCTION)
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Container based test is disabled!\n");
    return;
  }

  {
    /* Create a "DEMO" array which will be used for the Map below. */
    const char array_str[] = "var DEMO = [[1, 2], [3, 4]]; DEMO";
    jerry_value_t array = jerry_eval ((const jerry_char_t *) array_str, sizeof (array_str) - 1, 0);
    TEST_ASSERT (jerry_value_is_object (array));
    TEST_ASSERT (!jerry_value_is_exception (array));
    jerry_value_free (array);
  }

  const char eval_str[] = "new Map (DEMO)";
  {
    /* Make sure that the Map and it's prototype object/function is initialized. */
    jerry_value_t result = jerry_eval ((const jerry_char_t *) eval_str, sizeof (eval_str) - 1, 0);
    TEST_ASSERT (jerry_value_is_object (result));
    TEST_ASSERT (!jerry_value_is_exception (result));
    jerry_value_free (result);
  }

  /* Do a bit of cleaning to clear up old objects. */
  jerry_heap_gc (JERRY_GC_PRESSURE_LOW);

  /* Get the number of iterable objects. */
  int start_count = 0;
  jerry_foreach_live_object (count_objects, &start_count);

  /* Create another map. */
  jerry_value_t result = jerry_eval ((const jerry_char_t *) eval_str, sizeof (eval_str) - 1, 0);

  /* Remove any old/unused objects. */
  jerry_heap_gc (JERRY_GC_PRESSURE_LOW);

  /* Get the current number of objects. */
  int end_count = 0;
  jerry_foreach_live_object (count_objects, &end_count);

  /* As only one Map was created the number of available iterable objects should be incremented only by one. */
  TEST_ASSERT (end_count > start_count);
  TEST_ASSERT ((end_count - start_count) == 1);

  jerry_value_free (result);
} /* test_container */

static void
test_internal_prop (void)
{
  /* Make sure that the object is initialized in the engine. */
  jerry_value_t object_dummy = jerry_object ();

  /* Get the number of iterable objects. */
  int before_object_count = 0;
  jerry_foreach_live_object (count_objects, &before_object_count);

  jerry_value_t object = jerry_object ();

  /* After creating the object, the number of objects is incremented by one. */
  int after_object_count = 0;
  {
    jerry_foreach_live_object (count_objects, &after_object_count);

    TEST_ASSERT (after_object_count > before_object_count);
    TEST_ASSERT ((after_object_count - before_object_count) == 1);
  }

  jerry_value_t internal_prop_name = jerry_string_sz ("hidden_foo");
  jerry_value_t internal_prop_object = jerry_object ();
  bool internal_result = jerry_object_set_internal (object, internal_prop_name, internal_prop_object);
  TEST_ASSERT (internal_result == true);
  jerry_value_free (internal_prop_name);
  jerry_value_free (internal_prop_object);

  /* After adding an internal property object, the number of object is incremented by one. */
  {
    int after_internal_count = 0;
    jerry_foreach_live_object (count_objects, &after_internal_count);

    TEST_ASSERT (after_internal_count > after_object_count);
    TEST_ASSERT ((after_internal_count - after_object_count) == 1);
  }

  jerry_value_free (object);
  jerry_value_free (object_dummy);
} /* test_internal_prop */

static int test_data = 1;

static void
free_test_data (void *native_p, /**< native pointer */
                jerry_object_native_info_t *info_p) /**< native info */
{
  TEST_ASSERT ((int *) native_p == &test_data);
  TEST_ASSERT (info_p->free_cb == free_test_data);
} /* free_test_data */

static const jerry_object_native_info_t test_info = {
  .free_cb = free_test_data,
  .number_of_references = 0,
  .offset_of_references = 0,
};

static const jerry_char_t strict_equal_source[] = "var x = function(a, b) {return a === b;}; x";

static bool
find_test_object_by_data (const jerry_value_t candidate, void *object_data_p, void *context_p)
{
  if (object_data_p == &test_data)
  {
    *((jerry_value_t *) context_p) = jerry_value_copy (candidate);
    return false;
  }
  return true;
} /* find_test_object_by_data */

static bool
find_test_object_by_property (const jerry_value_t candidate, void *context_p)
{
  jerry_value_t *args_p = (jerry_value_t *) context_p;
  jerry_value_t result = jerry_object_has (candidate, args_p[0]);

  bool has_property = (!jerry_value_is_exception (result) && jerry_value_is_true (result));

  /* If the object has the desired property, store a new reference to it in args_p[1]. */
  if (has_property)
  {
    args_p[1] = jerry_value_copy (candidate);
  }

  jerry_value_free (result);

  /* Stop iterating if we've found our object. */
  return !has_property;
} /* find_test_object_by_property */

int
main (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_STRICT_MODE;

  /* Render strict-equal as a function. */
  jerry_value_t parse_result = jerry_parse (strict_equal_source, sizeof (strict_equal_source) - 1, &parse_options);
  TEST_ASSERT (!jerry_value_is_exception (parse_result));
  jerry_value_t strict_equal = jerry_run (parse_result);
  TEST_ASSERT (!jerry_value_is_exception (strict_equal));
  jerry_value_free (parse_result);

  /* Create an object and associate some native data with it. */
  jerry_value_t object = jerry_object ();
  jerry_object_set_native_ptr (object, &test_info, &test_data);

  /* Retrieve the object by its native pointer. */

  jerry_value_t found_object;
  TEST_ASSERT (jerry_foreach_live_object_with_info (&test_info, find_test_object_by_data, &found_object));
  jerry_value_t args[2] = { object, found_object };

  /* Assert that the correct object was retrieved. */
  jerry_value_t undefined = jerry_undefined ();
  jerry_value_t strict_equal_result = jerry_call (strict_equal, undefined, args, 2);
  TEST_ASSERT (jerry_value_is_boolean (strict_equal_result) && jerry_value_is_true (strict_equal_result));
  jerry_value_free (strict_equal_result);
  jerry_value_free (found_object);
  jerry_value_free (object);

  /* Collect garbage. */
  jerry_heap_gc (JERRY_GC_PRESSURE_LOW);

  /* Attempt to retrieve the object by its native pointer again. */
  TEST_ASSERT (!jerry_foreach_live_object_with_info (&test_info, find_test_object_by_data, &found_object));

  /* Create an object and set a property on it. */
  object = jerry_object ();
  jerry_value_t property_name = jerry_string_sz ("xyzzy");
  jerry_value_t property_value = jerry_number (42);
  jerry_value_free (jerry_object_set (object, property_name, property_value));
  jerry_value_free (property_value);

  /* Retrieve the object by the presence of its property, placing it at args[1]. */
  args[0] = property_name;
  TEST_ASSERT (jerry_foreach_live_object (find_test_object_by_property, args));

  /* Assert that the right object was retrieved and release both the original reference to it and the retrieved one. */
  args[0] = object;
  strict_equal_result = jerry_call (strict_equal, undefined, args, 2);
  TEST_ASSERT (jerry_value_is_boolean (strict_equal_result) && jerry_value_is_true (strict_equal_result));
  jerry_value_free (strict_equal_result);
  jerry_value_free (args[0]);
  jerry_value_free (args[1]);

  /* Collect garbage. */
  jerry_heap_gc (JERRY_GC_PRESSURE_LOW);

  /* Attempt to retrieve the object by the presence of its property again. */
  args[0] = property_name;
  TEST_ASSERT (!jerry_foreach_live_object (find_test_object_by_property, args));

  jerry_value_free (property_name);
  jerry_value_free (undefined);
  jerry_value_free (strict_equal);

  test_container ();
  test_internal_prop ();

  jerry_cleanup ();

  return 0;
} /* main */

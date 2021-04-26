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

static int global_counter;

static void
native_free_callback (void *native_p, /**< native pointer */
                      jerry_object_native_info_t *info_p) /**< native info */
{
  TEST_ASSERT (native_p == (void *) &global_counter);
  TEST_ASSERT (info_p->free_cb == native_free_callback);
  global_counter++;
} /* native_free_callback */

static const jerry_object_native_info_t native_info =
{
  .free_cb = native_free_callback,
  .number_of_references = 0,
  .offset_of_references = 0,
};

int
main (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  if (!jerry_is_feature_enabled (JERRY_FEATURE_MAP)
      || !jerry_is_feature_enabled (JERRY_FEATURE_SET)
      || !jerry_is_feature_enabled (JERRY_FEATURE_WEAKMAP)
      || !jerry_is_feature_enabled (JERRY_FEATURE_WEAKSET))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Containers are disabled!\n");
    jerry_cleanup ();
    return 0;
  }

  jerry_value_t instance_check;
  jerry_value_t global = jerry_get_global_object ();
  jerry_value_t map_str = jerry_create_string ((jerry_char_t *) "Map");
  jerry_value_t set_str = jerry_create_string ((jerry_char_t *) "Set");
  jerry_value_t weakmap_str = jerry_create_string ((jerry_char_t *) "WeakMap");
  jerry_value_t weakset_str = jerry_create_string ((jerry_char_t *) "WeakSet");
  jerry_value_t global_map = jerry_get_property (global, map_str);
  jerry_value_t global_set = jerry_get_property (global, set_str);
  jerry_value_t global_weakmap = jerry_get_property (global, weakmap_str);
  jerry_value_t global_weakset = jerry_get_property (global, weakset_str);

  jerry_release_value (global);

  jerry_release_value (map_str);
  jerry_release_value (set_str);
  jerry_release_value (weakmap_str);
  jerry_release_value (weakset_str);

  jerry_value_t empty_map = jerry_create_container (JERRY_CONTAINER_TYPE_MAP, NULL, 0);
  TEST_ASSERT (jerry_get_container_type (empty_map) == JERRY_CONTAINER_TYPE_MAP);
  instance_check = jerry_binary_operation (JERRY_BIN_OP_INSTANCEOF, empty_map, global_map);
  TEST_ASSERT (jerry_get_boolean_value (instance_check));
  jerry_release_value (instance_check);
  jerry_release_value (global_map);
  jerry_release_value (empty_map);

  jerry_value_t empty_set = jerry_create_container (JERRY_CONTAINER_TYPE_SET, NULL, 0);
  TEST_ASSERT (jerry_get_container_type (empty_set) == JERRY_CONTAINER_TYPE_SET);
  instance_check = jerry_binary_operation (JERRY_BIN_OP_INSTANCEOF, empty_set, global_set);
  TEST_ASSERT (jerry_get_boolean_value (instance_check));
  jerry_release_value (instance_check);
  jerry_release_value (global_set);
  jerry_release_value (empty_set);

  jerry_value_t empty_weakmap = jerry_create_container (JERRY_CONTAINER_TYPE_WEAKMAP, NULL, 0);
  TEST_ASSERT (jerry_get_container_type (empty_weakmap) == JERRY_CONTAINER_TYPE_WEAKMAP);
  instance_check = jerry_binary_operation (JERRY_BIN_OP_INSTANCEOF, empty_weakmap, global_weakmap);
  TEST_ASSERT (jerry_get_boolean_value (instance_check));
  jerry_release_value (instance_check);
  jerry_release_value (global_weakmap);
  jerry_release_value (empty_weakmap);

  jerry_value_t empty_weakset = jerry_create_container (JERRY_CONTAINER_TYPE_WEAKSET, NULL, 0);
  TEST_ASSERT (jerry_get_container_type (empty_weakset) == JERRY_CONTAINER_TYPE_WEAKSET);
  instance_check = jerry_binary_operation (JERRY_BIN_OP_INSTANCEOF, empty_weakset, global_weakset);
  TEST_ASSERT (jerry_get_boolean_value (instance_check));
  jerry_release_value (instance_check);
  jerry_release_value (global_weakset);
  jerry_release_value (empty_weakset);

  const jerry_char_t source[] = TEST_STRING_LITERAL (
    "(function () {\n"
    "  var o1 = {}\n"
    "  var o2 = {}\n"
    "  var o3 = {}\n"
    "  var wm = new WeakMap()\n"
    "  wm.set(o1, o2)\n"
    "  wm.set(o2, o3)\n"
    "  return o3\n"
    "})()\n"
  );
  jerry_value_t result = jerry_eval (source,
                                     sizeof (source) - 1,
                                     JERRY_PARSE_NO_OPTS);
  TEST_ASSERT (jerry_value_is_object (result));

  jerry_set_object_native_pointer (result, (void *) &global_counter, &native_info);
  jerry_release_value (result);

  global_counter = 0;
  jerry_gc (JERRY_GC_PRESSURE_LOW);
  TEST_ASSERT (global_counter == 1);

  jerry_cleanup ();
  return 0;
} /* main */

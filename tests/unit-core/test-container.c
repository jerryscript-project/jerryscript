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

static const jerry_object_native_info_t native_info = {
  .free_cb = native_free_callback,
  .number_of_references = 0,
  .offset_of_references = 0,
};

static jerry_value_t
create_array_from_container_handler (const jerry_call_info_t *call_info_p,
                                     const jerry_value_t args_p[],
                                     const jerry_length_t args_count)
{
  JERRY_UNUSED (call_info_p);

  if (args_count < 2)
  {
    return jerry_undefined ();
  }

  bool is_key_value_pairs = false;
  jerry_value_t result = jerry_container_to_array (args_p[0], &is_key_value_pairs);

  TEST_ASSERT (is_key_value_pairs == jerry_value_is_true (args_p[1]));
  return result;
} /* create_array_from_container_handler */

static void
run_eval (const char *source_p)
{
  jerry_value_t result = jerry_eval ((const jerry_char_t *) source_p, strlen (source_p), 0);

  TEST_ASSERT (!jerry_value_is_exception (result));
  jerry_value_free (result);
} /* run_eval */

static void
run_eval_error (const char *source_p)
{
  jerry_value_t result = jerry_eval ((const jerry_char_t *) source_p, strlen (source_p), 0);
  jerry_value_free (result);
} /* run_eval_error */

int
main (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  if (!jerry_feature_enabled (JERRY_FEATURE_MAP) || !jerry_feature_enabled (JERRY_FEATURE_SET)
      || !jerry_feature_enabled (JERRY_FEATURE_WEAKMAP) || !jerry_feature_enabled (JERRY_FEATURE_WEAKSET))
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Containers are disabled!\n");
    jerry_cleanup ();
    return 0;
  }

  jerry_value_t instance_check;
  jerry_value_t global = jerry_current_realm ();
  jerry_value_t map_str = jerry_string_sz ("Map");
  jerry_value_t set_str = jerry_string_sz ("Set");
  jerry_value_t weakmap_str = jerry_string_sz ("WeakMap");
  jerry_value_t weakset_str = jerry_string_sz ("WeakSet");
  jerry_value_t global_map = jerry_object_get (global, map_str);
  jerry_value_t global_set = jerry_object_get (global, set_str);
  jerry_value_t global_weakmap = jerry_object_get (global, weakmap_str);
  jerry_value_t global_weakset = jerry_object_get (global, weakset_str);

  jerry_value_t function = jerry_function_external (create_array_from_container_handler);
  jerry_value_t name = jerry_string_sz ("create_array_from_container");
  jerry_value_t res = jerry_object_set (global, name, function);
  TEST_ASSERT (!jerry_value_is_exception (res));

  jerry_value_free (res);
  jerry_value_free (name);
  jerry_value_free (function);

  jerry_value_free (global);

  jerry_value_free (map_str);
  jerry_value_free (set_str);
  jerry_value_free (weakmap_str);
  jerry_value_free (weakset_str);

  jerry_value_t empty_map = jerry_container (JERRY_CONTAINER_TYPE_MAP, NULL, 0);
  TEST_ASSERT (jerry_container_type (empty_map) == JERRY_CONTAINER_TYPE_MAP);
  instance_check = jerry_binary_op (JERRY_BIN_OP_INSTANCEOF, empty_map, global_map);
  TEST_ASSERT (jerry_value_is_true (instance_check));
  jerry_value_free (instance_check);
  jerry_value_free (global_map);
  jerry_value_free (empty_map);

  jerry_value_t empty_set = jerry_container (JERRY_CONTAINER_TYPE_SET, NULL, 0);
  TEST_ASSERT (jerry_container_type (empty_set) == JERRY_CONTAINER_TYPE_SET);
  instance_check = jerry_binary_op (JERRY_BIN_OP_INSTANCEOF, empty_set, global_set);
  TEST_ASSERT (jerry_value_is_true (instance_check));
  jerry_value_free (instance_check);
  jerry_value_free (global_set);
  jerry_value_free (empty_set);

  jerry_value_t empty_weakmap = jerry_container (JERRY_CONTAINER_TYPE_WEAKMAP, NULL, 0);
  TEST_ASSERT (jerry_container_type (empty_weakmap) == JERRY_CONTAINER_TYPE_WEAKMAP);
  instance_check = jerry_binary_op (JERRY_BIN_OP_INSTANCEOF, empty_weakmap, global_weakmap);
  TEST_ASSERT (jerry_value_is_true (instance_check));
  jerry_value_free (instance_check);
  jerry_value_free (global_weakmap);
  jerry_value_free (empty_weakmap);

  jerry_value_t empty_weakset = jerry_container (JERRY_CONTAINER_TYPE_WEAKSET, NULL, 0);
  TEST_ASSERT (jerry_container_type (empty_weakset) == JERRY_CONTAINER_TYPE_WEAKSET);
  instance_check = jerry_binary_op (JERRY_BIN_OP_INSTANCEOF, empty_weakset, global_weakset);
  TEST_ASSERT (jerry_value_is_true (instance_check));
  jerry_value_free (instance_check);
  jerry_value_free (global_weakset);
  jerry_value_free (empty_weakset);

  const jerry_char_t source[] = TEST_STRING_LITERAL ("(function () {\n"
                                                     "  var o1 = {}\n"
                                                     "  var o2 = {}\n"
                                                     "  var o3 = {}\n"
                                                     "  var wm = new WeakMap()\n"
                                                     "  wm.set(o1, o2)\n"
                                                     "  wm.set(o2, o3)\n"
                                                     "  return o3\n"
                                                     "})()\n");
  jerry_value_t result = jerry_eval (source, sizeof (source) - 1, JERRY_PARSE_NO_OPTS);
  TEST_ASSERT (jerry_value_is_object (result));

  jerry_object_set_native_ptr (result, &native_info, (void *) &global_counter);
  jerry_value_free (result);

  global_counter = 0;
  jerry_heap_gc (JERRY_GC_PRESSURE_LOW);
  TEST_ASSERT (global_counter == 1);

  run_eval ("function assert(v) {\n"
            "  if(v !== true)\n"
            "    throw 'Assertion failed!'\n"
            "}");

  run_eval ("function test_values(arr1, arr2) {\n"
            "  assert(Array.isArray(arr1));\n"
            "  assert(arr1.length == arr2.length);\n"
            "  for(let i = 0; i < arr1.length; i++) {\n"
            "    assert(arr1[i] === arr2[i]);\n"
            "  }\n"
            "}\n");

  run_eval ("var map = new Map();\n"
            "map.set(1, 3.14);\n"
            "map.set(2, true);\n"
            "map.set(3, 'foo');\n"
            "var set = new Set();\n"
            "set.add(3.14);\n"
            "set.add(true);\n"
            "set.add('foo');\n"
            "var obj = { x:3, y:'foo'};\n"
            "var b_int = 1n;\n"
            "var obj_bint_map = new Map();\n"
            "obj_bint_map.set(1, obj);\n"
            "obj_bint_map.set(2, b_int);\n");

  run_eval ("var result = create_array_from_container(map, true);\n"
            "test_values(result, [1, 3.14, 2, true, 3, 'foo']);");

  run_eval ("var result = create_array_from_container(set, false);\n"
            "test_values(result, [3.14, true, 'foo']);");

  run_eval ("var result = create_array_from_container(map.entries(), true);\n"
            "test_values(result, [1, 3.14, 2, true, 3, 'foo']);");

  run_eval ("var result = create_array_from_container(map.keys(), false);\n"
            "test_values(result, [1, 2, 3,]);");

  run_eval ("var result = create_array_from_container(map.values(), false);\n"
            "test_values(result, [3.14, true, 'foo']);");

  run_eval ("var result = create_array_from_container(obj_bint_map, true)\n"
            "test_values(result, [1, obj, 2, b_int]);");

  run_eval ("var map = new Map();\n"
            "map.set(1, 1);\n"
            "var iter = map.entries();\n"
            "iter.next();\n"
            "var result = create_array_from_container(iter, true);\n"
            "assert(Array.isArray(result));\n"
            "assert(result.length == 0);");

  run_eval ("var ws = new WeakSet();\n"
            "var foo = {};\n"
            "var bar = {};\n"
            "ws.add(foo);\n"
            "ws.add(bar);\n"
            "var result = create_array_from_container(ws, false);\n"
            "test_values(result, [foo, bar]);\n");

  run_eval ("var ws = new WeakMap();\n"
            "var foo = {};\n"
            "var bar = {};\n"
            "ws.set(foo, 37);\n"
            "ws.set(bar, 'asd');\n"
            "var result = create_array_from_container(ws, true);\n"
            "test_values(result, [foo, 37, bar, 'asd']);\n");

  run_eval_error ("var iter = null;\n"
                  "var result = create_array_from_container(iter, false);\n"
                  "assert(result instanceof Error);");

  run_eval_error ("var iter = 3;\n"
                  "var result = create_array_from_container(iter, false);\n"
                  "assert(result instanceof Error);");

  run_eval_error ("var iter = [3.14, true, 'foo'].entries();\n"
                  "var result = create_array_from_container(iter, false);\n"
                  "assert(result instanceof Error);");

  jerry_cleanup ();
  return 0;
} /* main */

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
  TEST_INIT ();

  jerry_init (JERRY_INIT_EMPTY);

  /* Test: init property descriptor */
  jerry_property_descriptor_t prop_desc;
  jerry_init_property_descriptor_fields (&prop_desc);
  TEST_ASSERT (prop_desc.is_value_defined == false);
  TEST_ASSERT (jerry_value_is_undefined (prop_desc.value));
  TEST_ASSERT (prop_desc.is_writable_defined == false);
  TEST_ASSERT (prop_desc.is_writable == false);
  TEST_ASSERT (prop_desc.is_enumerable_defined == false);
  TEST_ASSERT (prop_desc.is_enumerable == false);
  TEST_ASSERT (prop_desc.is_configurable_defined == false);
  TEST_ASSERT (prop_desc.is_configurable == false);
  TEST_ASSERT (prop_desc.is_get_defined == false);
  TEST_ASSERT (jerry_value_is_undefined (prop_desc.getter));
  TEST_ASSERT (prop_desc.is_set_defined == false);
  TEST_ASSERT (jerry_value_is_undefined (prop_desc.setter));

  /* Test: define own properties */
  jerry_value_t global_obj_val = jerry_get_global_object ();
  jerry_value_t prop_name = jerry_create_string ((const jerry_char_t *) "my_defined_property");
  prop_desc.is_value_defined = true;
  prop_desc.value = jerry_acquire_value (prop_name);
  jerry_value_t res = jerry_define_own_property (global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (!jerry_value_is_error (res));
  TEST_ASSERT (jerry_value_is_boolean (res));
  TEST_ASSERT (jerry_get_boolean_value (res));
  jerry_release_value (res);
  jerry_free_property_descriptor_fields (&prop_desc);

  /* Test: get own property descriptor */
  bool is_ok = jerry_get_own_property_descriptor (global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (is_ok);
  TEST_ASSERT (prop_desc.is_value_defined == true);
  TEST_ASSERT (jerry_value_is_string (prop_desc.value));
  TEST_ASSERT (prop_desc.is_writable == false);
  TEST_ASSERT (prop_desc.is_enumerable == false);
  TEST_ASSERT (prop_desc.is_configurable == false);
  TEST_ASSERT (prop_desc.is_get_defined == false);
  TEST_ASSERT (jerry_value_is_undefined (prop_desc.getter));
  TEST_ASSERT (prop_desc.is_set_defined == false);
  TEST_ASSERT (jerry_value_is_undefined (prop_desc.setter));
  jerry_release_value (prop_name);
  jerry_free_property_descriptor_fields (&prop_desc);

  jerry_release_value (global_obj_val);
  jerry_cleanup ();

  return 0;
} /* main */

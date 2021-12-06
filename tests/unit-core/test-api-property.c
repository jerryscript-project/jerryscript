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
  jerry_property_descriptor_t prop_desc = jerry_property_descriptor ();
  TEST_ASSERT (prop_desc.flags == JERRY_PROP_NO_OPTS);
  TEST_ASSERT (jerry_value_is_undefined (prop_desc.value));
  TEST_ASSERT (jerry_value_is_undefined (prop_desc.getter));
  TEST_ASSERT (jerry_value_is_undefined (prop_desc.setter));

  /* Test: define own properties */
  jerry_value_t global_obj_val = jerry_current_realm ();
  jerry_value_t prop_name = jerry_string_sz ("my_defined_property");
  prop_desc.flags |= JERRY_PROP_IS_VALUE_DEFINED;
  prop_desc.value = jerry_value_copy (prop_name);
  jerry_value_t res = jerry_object_define_own_prop (global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jerry_value_is_boolean (res) && jerry_value_is_true (res));
  jerry_value_free (res);
  jerry_property_descriptor_free (&prop_desc);

  /* Test: define own property with error */
  prop_desc = jerry_property_descriptor ();
  prop_desc.flags |= JERRY_PROP_IS_VALUE_DEFINED | JERRY_PROP_SHOULD_THROW;
  prop_desc.value = jerry_number (3.14);
  res = jerry_object_define_own_prop (global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jerry_value_is_exception (res));
  jerry_value_free (res);
  jerry_property_descriptor_free (&prop_desc);

  /* Test: test define own property failure without throw twice */
  prop_desc = jerry_property_descriptor ();
  prop_desc.flags |= JERRY_PROP_IS_VALUE_DEFINED | JERRY_PROP_IS_GET_DEFINED;
  res = jerry_object_define_own_prop (prop_name, prop_name, &prop_desc);
  TEST_ASSERT (jerry_value_is_boolean (res) && !jerry_value_is_true (res));
  jerry_value_free (res);
  res = jerry_object_define_own_prop (global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jerry_value_is_boolean (res) && !jerry_value_is_true (res));
  jerry_value_free (res);
  jerry_property_descriptor_free (&prop_desc);

  /* Test: get own property descriptor */
  prop_desc = jerry_property_descriptor ();
  jerry_value_t is_ok = jerry_object_get_own_prop (global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jerry_value_is_boolean (is_ok) && jerry_value_is_true (is_ok));
  jerry_value_free (is_ok);
  TEST_ASSERT (prop_desc.flags & JERRY_PROP_IS_VALUE_DEFINED);
  TEST_ASSERT (jerry_value_is_string (prop_desc.value));
  TEST_ASSERT (!(prop_desc.flags & JERRY_PROP_IS_WRITABLE));
  TEST_ASSERT (!(prop_desc.flags & JERRY_PROP_IS_ENUMERABLE));
  TEST_ASSERT (!(prop_desc.flags & JERRY_PROP_IS_CONFIGURABLE));
  TEST_ASSERT (!(prop_desc.flags & JERRY_PROP_IS_GET_DEFINED));
  TEST_ASSERT (jerry_value_is_undefined (prop_desc.getter));
  TEST_ASSERT (!(prop_desc.flags & JERRY_PROP_IS_SET_DEFINED));
  TEST_ASSERT (jerry_value_is_undefined (prop_desc.setter));
  jerry_property_descriptor_free (&prop_desc);

  if (jerry_feature_enabled (JERRY_FEATURE_PROXY))
  {
    /* Note: update this test when the internal method is implemented */
    jerry_value_t target = jerry_object ();
    jerry_value_t handler = jerry_object ();
    jerry_value_t proxy = jerry_proxy (target, handler);

    jerry_value_free (target);
    jerry_value_free (handler);
    is_ok = jerry_object_get_own_prop (proxy, prop_name, &prop_desc);
    TEST_ASSERT (jerry_value_is_boolean (is_ok) && !jerry_value_is_true (is_ok));
    jerry_value_free (is_ok);
    jerry_value_free (proxy);
  }

  jerry_value_free (prop_name);

  /* Test: define and get own property descriptor */
  prop_desc.flags |= JERRY_PROP_IS_ENUMERABLE;
  prop_name = jerry_string_sz ("enumerable-property");
  res = jerry_object_define_own_prop (global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (!jerry_value_is_exception (res));
  TEST_ASSERT (jerry_value_is_boolean (res));
  TEST_ASSERT (jerry_value_is_true (res));
  jerry_value_free (res);
  jerry_property_descriptor_free (&prop_desc);
  is_ok = jerry_object_get_own_prop (global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jerry_value_is_boolean (is_ok) && jerry_value_is_true (is_ok));
  jerry_value_free (is_ok);
  TEST_ASSERT (!(prop_desc.flags & JERRY_PROP_IS_WRITABLE));
  TEST_ASSERT (prop_desc.flags & JERRY_PROP_IS_ENUMERABLE);
  TEST_ASSERT (!(prop_desc.flags & JERRY_PROP_IS_CONFIGURABLE));

  jerry_value_free (prop_name);
  jerry_value_free (global_obj_val);

  /* Test: define own property descriptor error */
  prop_desc = jerry_property_descriptor ();
  prop_desc.flags |= JERRY_PROP_IS_VALUE_DEFINED;
  prop_desc.value = jerry_number (11);

  jerry_value_t obj_val = jerry_object ();
  prop_name = jerry_string_sz ("property_key");
  res = jerry_object_define_own_prop (obj_val, prop_name, &prop_desc);
  TEST_ASSERT (!jerry_value_is_exception (res));
  jerry_value_free (res);

  jerry_value_free (prop_desc.value);
  prop_desc.value = jerry_number (22);
  res = jerry_object_define_own_prop (obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jerry_value_is_exception (res));
  jerry_value_free (res);

  jerry_value_free (prop_name);
  jerry_value_free (obj_val);

  jerry_cleanup ();

  return 0;
} /* main */

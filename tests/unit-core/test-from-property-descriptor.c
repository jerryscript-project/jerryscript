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

int
main (void)
{
  TEST_INIT ();

  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t object = jerry_object ();
  jerry_value_t prop_name = jerry_string_sz ("length");
  jerry_value_t value = jerry_boolean (true);

  TEST_ASSERT (jerry_object_set (object, prop_name, prop_name));
  TEST_ASSERT (jerry_object_has (object, prop_name));
  TEST_ASSERT (jerry_object_has_own (object, prop_name));

  jerry_property_descriptor_t prop_desc;
  TEST_ASSERT (jerry_object_get_own_prop (object, prop_name, &prop_desc));

  jerry_value_t from_object = jerry_property_descriptor_to_object (&prop_desc);

  prop_name = jerry_string_sz ("value");
  value = jerry_object_get (from_object, prop_name);
  TEST_ASSERT (value == prop_desc.value);

  prop_name = jerry_string_sz ("writable");
  value = jerry_object_get (from_object, prop_name);
  TEST_ASSERT (jerry_value_is_true (value) == ((prop_desc.flags & JERRY_PROP_IS_WRITABLE) != 0));

  prop_name = jerry_string_sz ("enumerable");
  value = jerry_object_get (from_object, prop_name);
  TEST_ASSERT (jerry_value_is_true (value) == ((prop_desc.flags & JERRY_PROP_IS_ENUMERABLE) != 0));

  prop_name = jerry_string_sz ("configurable");
  value = jerry_object_get (from_object, prop_name);
  TEST_ASSERT (jerry_value_is_true (value) == ((prop_desc.flags & JERRY_PROP_IS_CONFIGURABLE) != 0));

  jerry_value_free (object);
  jerry_value_free (prop_name);
  jerry_value_free (value);
  jerry_value_free (from_object);
  jerry_property_descriptor_free (&prop_desc);

  prop_desc.flags = JERRY_PROP_IS_CONFIGURABLE;
  from_object = jerry_property_descriptor_to_object (&prop_desc);
  TEST_ASSERT (jerry_value_is_exception (from_object));
  jerry_value_free (from_object);

  prop_desc.flags = JERRY_PROP_IS_ENUMERABLE;
  from_object = jerry_property_descriptor_to_object (&prop_desc);
  TEST_ASSERT (jerry_value_is_exception (from_object));
  jerry_value_free (from_object);

  prop_desc.flags = JERRY_PROP_IS_WRITABLE;
  from_object = jerry_property_descriptor_to_object (&prop_desc);
  TEST_ASSERT (jerry_value_is_exception (from_object));
  jerry_value_free (from_object);

  jerry_cleanup ();
  return 0;
} /* main */

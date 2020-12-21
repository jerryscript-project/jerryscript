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

int
main (void)
{
  TEST_INIT ();

  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t object = jerry_create_object ();
  jerry_value_t prop_name;
  jerry_value_t value = jerry_create_boolean (true);
  jerry_property_descriptor_t prop_desc;

  prop_name = jerry_create_string_from_utf8 ((jerry_char_t *) "value");
  TEST_ASSERT (jerry_set_property (object, prop_name, value));

  prop_name = jerry_create_string_from_utf8 ((jerry_char_t *) "writable");
  TEST_ASSERT (jerry_set_property (object, prop_name, value));

  prop_name = jerry_create_string_from_utf8 ((jerry_char_t *) "writable");
  TEST_ASSERT (jerry_set_property (object, prop_name, value));

  prop_name = jerry_create_string_from_utf8 ((jerry_char_t *) "enumerable");
  TEST_ASSERT (jerry_set_property (object, prop_name, value));

  prop_name = jerry_create_string_from_utf8 ((jerry_char_t *) "configurable");
  TEST_ASSERT (jerry_set_property (object, prop_name, value));

  jerry_value_t result = jerry_to_property_descriptor (object, &prop_desc);
  TEST_ASSERT (jerry_value_is_boolean (result) && jerry_get_boolean_value (result));
  jerry_release_value (result);

  prop_name = jerry_create_string_from_utf8 ((jerry_char_t *) "value");
  value = jerry_get_property (object, prop_name);
  TEST_ASSERT (value == prop_desc.value);

  prop_name = jerry_create_string_from_utf8 ((jerry_char_t *) "writable");
  value = jerry_get_property (object, prop_name);
  TEST_ASSERT (jerry_get_boolean_value (value) == prop_desc.is_writable);

  prop_name = jerry_create_string_from_utf8 ((jerry_char_t *) "writable");
  value = jerry_get_property (object, prop_name);
  TEST_ASSERT (jerry_get_boolean_value (value) == prop_desc.is_writable);

  prop_name = jerry_create_string_from_utf8 ((jerry_char_t *) "enumerable");
  value = jerry_get_property (object, prop_name);
  TEST_ASSERT (jerry_get_boolean_value (value) == prop_desc.is_enumerable);

  prop_name = jerry_create_string_from_utf8 ((jerry_char_t *) "configurable");
  value = jerry_get_property (object, prop_name);
  TEST_ASSERT (jerry_get_boolean_value (value) == prop_desc.is_configurable);

  jerry_release_value (object);
  jerry_release_value (prop_name);
  jerry_release_value (value);
  jerry_free_property_descriptor_fields (&prop_desc);

  object = jerry_create_null ();
  result = jerry_to_property_descriptor (object, &prop_desc);
  TEST_ASSERT (jerry_value_is_error (result));
  jerry_release_value (result);
  jerry_release_value (object);

  jerry_cleanup ();
  return 0;
} /* main */

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

#include "jerryscript-port.h"
#include "jerryscript.h"

#include "test-common.h"

static const char *prop_names[] = { "val1", "val2", "val3", "val4", "val5", "37", "symbol" };

static jerry_char_t buffer[256] = { 0 };

static void
create_and_set_property (const jerry_value_t object, const char *prop_name)
{
  jerry_value_t jprop_name = jerry_string_sz (prop_name);
  jerry_value_t ret_val = jerry_object_set (object, jprop_name, jerry_undefined ());

  jerry_value_free (jprop_name);
  jerry_value_free (ret_val);
} /* create_and_set_property */

static void
compare_prop_name (const jerry_value_t object, const char *prop_name, uint32_t idx)
{
  jerry_value_t name = jerry_object_get_index (object, idx);
  TEST_ASSERT (jerry_value_is_string (name) || jerry_value_is_number (name));
  if (jerry_value_is_string (name))
  {
    jerry_size_t name_size = jerry_string_size (name, JERRY_ENCODING_CESU8);
    TEST_ASSERT (name_size < sizeof (buffer));
    jerry_size_t ret_size = jerry_string_to_buffer (name, JERRY_ENCODING_CESU8, buffer, sizeof (buffer));
    TEST_ASSERT (name_size == ret_size);
    buffer[name_size] = '\0';
    TEST_ASSERT (strcmp ((const char *) buffer, prop_name) == 0);
  }
  else
  {
    TEST_ASSERT ((int) jerry_value_as_number (name) == atoi (prop_name));
  }

  jerry_value_free (name);
} /* compare_prop_name */

static void
define_property (const jerry_value_t object,
                 const char *prop_name,
                 jerry_property_descriptor_t *prop_desc_p,
                 bool is_symbol)
{
  jerry_value_t jname = jerry_string_sz (prop_name);
  jerry_value_t ret_val;
  if (is_symbol)
  {
    jerry_value_t symbol = jerry_symbol_with_description (jname);
    ret_val = jerry_object_define_own_prop (object, symbol, prop_desc_p);
    jerry_value_free (symbol);
  }
  else
  {
    ret_val = jerry_object_define_own_prop (object, jname, prop_desc_p);
  }

  jerry_value_free (jname);
  jerry_value_free (ret_val);
} /* define_property */

int
main (void)
{
  TEST_INIT ();
  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t error_value = jerry_object_property_names (jerry_undefined (), JERRY_PROPERTY_FILTER_ALL);
  TEST_ASSERT (jerry_value_is_exception (error_value) && jerry_error_type (error_value) == JERRY_ERROR_TYPE);
  jerry_value_free (error_value);

  jerry_value_t test_object = jerry_object ();
  create_and_set_property (test_object, prop_names[0]);
  create_and_set_property (test_object, prop_names[1]);

  jerry_value_t names;

  jerry_property_descriptor_t prop_desc = jerry_property_descriptor ();
  prop_desc.flags |= (JERRY_PROP_IS_CONFIGURABLE_DEFINED | JERRY_PROP_IS_CONFIGURABLE | JERRY_PROP_IS_WRITABLE_DEFINED
                      | JERRY_PROP_IS_WRITABLE | JERRY_PROP_IS_ENUMERABLE_DEFINED);

  // Test enumerable - non-enumerable filter
  define_property (test_object, prop_names[2], &prop_desc, false);
  names =
    jerry_object_property_names (test_object, JERRY_PROPERTY_FILTER_ALL | JERRY_PROPERTY_FILTER_EXCLUDE_NON_ENUMERABLE);
  TEST_ASSERT (jerry_array_length (names) == (uint32_t) 2);
  jerry_value_free (names);
  names = jerry_object_property_names (test_object, JERRY_PROPERTY_FILTER_ALL);
  TEST_ASSERT (jerry_array_length (names) == (uint32_t) 3);
  compare_prop_name (names, prop_names[2], 2);
  jerry_value_free (names);
  prop_desc.flags |= JERRY_PROP_IS_ENUMERABLE;

  // Test configurable - non-configurable filter
  prop_desc.flags &= (uint16_t) ~JERRY_PROP_IS_CONFIGURABLE;
  define_property (test_object, prop_names[3], &prop_desc, false);
  names = jerry_object_property_names (test_object,
                                       JERRY_PROPERTY_FILTER_ALL | JERRY_PROPERTY_FILTER_EXCLUDE_NON_CONFIGURABLE);
  TEST_ASSERT (jerry_array_length (names) == (uint32_t) 3);
  jerry_value_free (names);
  names = jerry_object_property_names (test_object, JERRY_PROPERTY_FILTER_ALL);
  TEST_ASSERT (jerry_array_length (names) == (uint32_t) 4);
  compare_prop_name (names, prop_names[3], 3);
  jerry_value_free (names);
  prop_desc.flags |= JERRY_PROP_IS_CONFIGURABLE;

  // Test writable - non-writable filter
  prop_desc.flags &= (uint16_t) ~JERRY_PROP_IS_WRITABLE;
  define_property (test_object, prop_names[4], &prop_desc, false);
  names =
    jerry_object_property_names (test_object, JERRY_PROPERTY_FILTER_ALL | JERRY_PROPERTY_FILTER_EXCLUDE_NON_WRITABLE);
  TEST_ASSERT (jerry_array_length (names) == (uint32_t) 4);
  jerry_value_free (names);
  names = jerry_object_property_names (test_object, JERRY_PROPERTY_FILTER_ALL);
  TEST_ASSERT (jerry_array_length (names) == (uint32_t) 5);
  compare_prop_name (names, prop_names[4], 4);
  jerry_value_free (names);
  prop_desc.flags |= JERRY_PROP_IS_WRITABLE;

  // Test all property filter
  names = jerry_object_property_names (test_object, JERRY_PROPERTY_FILTER_ALL);
  jerry_length_t array_len = jerry_array_length (names);
  TEST_ASSERT (array_len == (uint32_t) 5);

  for (uint32_t i = 0; i < array_len; i++)
  {
    compare_prop_name (names, prop_names[i], i);
  }

  jerry_value_free (names);

  // Test number and string index exclusion
  define_property (test_object, prop_names[5], &prop_desc, false);
  names = jerry_object_property_names (test_object,
                                       JERRY_PROPERTY_FILTER_ALL | JERRY_PROPERTY_FILTER_EXCLUDE_STRINGS
                                         | JERRY_PROPERTY_FILTER_INTEGER_INDICES_AS_NUMBER);
  TEST_ASSERT (jerry_array_length (names) == (uint32_t) 1);
  compare_prop_name (names, prop_names[5], 0);
  jerry_value_free (names);
  names = jerry_object_property_names (test_object,
                                       JERRY_PROPERTY_FILTER_ALL | JERRY_PROPERTY_FILTER_EXCLUDE_INTEGER_INDICES);
  TEST_ASSERT (jerry_array_length (names) == (uint32_t) 5);
  jerry_value_free (names);

  // Test prototype chain traversion
  names = jerry_object_property_names (test_object, JERRY_PROPERTY_FILTER_ALL);
  TEST_ASSERT (jerry_array_length (names) == (uint32_t) 6);
  jerry_value_free (names);
  names = jerry_object_property_names (test_object,
                                       JERRY_PROPERTY_FILTER_ALL | JERRY_PROPERTY_FILTER_TRAVERSE_PROTOTYPE_CHAIN);
  TEST_ASSERT (jerry_array_length (names) == (uint32_t) 18);
  jerry_value_free (names);

  // Test symbol exclusion
  define_property (test_object, prop_names[6], &prop_desc, true);
  names = jerry_object_property_names (test_object, JERRY_PROPERTY_FILTER_ALL | JERRY_PROPERTY_FILTER_EXCLUDE_SYMBOLS);
  TEST_ASSERT (jerry_array_length (names) == (uint32_t) 6);
  jerry_value_free (names);
  names = jerry_object_property_names (test_object, JERRY_PROPERTY_FILTER_ALL);
  TEST_ASSERT (jerry_array_length (names) == (uint32_t) 7);
  jerry_value_free (names);

  jerry_property_descriptor_free (&prop_desc);
  jerry_value_free (test_object);
  jerry_cleanup ();
  return 0;
} /* main */

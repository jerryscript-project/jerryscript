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
#include "jerryscript-port.h"
#include "jerryscript-port-default.h"
#include "test-common.h"

/* foo string */
#define STRING_FOO ((const jerry_char_t *) "foo")

/* bar string */
#define STRING_BAR ((const jerry_char_t *) "bar")

/* Symbol(bar) desciptive string */
#define SYMBOL_DESCIPTIVE_STRING_BAR "Symbol(bar)"

int
main (void)
{
  if (!jerry_is_feature_enabled (JERRY_FEATURE_SYMBOL))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Symbol support is disabled!\n");
    return 0;
  }

  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t object = jerry_create_object ();

  /* Test for that each symbol is unique independently from their descriptor strings */
  jerry_value_t symbol_desc_1 = jerry_create_string (STRING_FOO);
  jerry_value_t symbol_desc_2 = jerry_create_string (STRING_FOO);

  jerry_value_t symbol_1 = jerry_create_symbol (symbol_desc_1);
  TEST_ASSERT (!jerry_value_is_error (symbol_1));
  TEST_ASSERT (jerry_value_is_symbol (symbol_1));

  jerry_value_t symbol_2 = jerry_create_symbol (symbol_desc_2);
  TEST_ASSERT (!jerry_value_is_error (symbol_2));
  TEST_ASSERT (jerry_value_is_symbol (symbol_2));

  /* The descriptor strings are no longer needed */
  jerry_release_value (symbol_desc_1);
  jerry_release_value (symbol_desc_2);

  jerry_value_t value_1 = jerry_create_number (1);
  jerry_value_t value_2 = jerry_create_number (2);

  jerry_value_t result_val = jerry_set_property (object, symbol_1, value_1);
  TEST_ASSERT (jerry_value_is_boolean (result_val));
  TEST_ASSERT (jerry_get_boolean_value (jerry_has_property (object, symbol_1)));
  TEST_ASSERT (jerry_get_boolean_value (jerry_has_own_property (object, symbol_1)));

  result_val = jerry_set_property (object, symbol_2, value_2);
  TEST_ASSERT (jerry_value_is_boolean (result_val));
  TEST_ASSERT (jerry_get_boolean_value (jerry_has_property (object, symbol_2)));
  TEST_ASSERT (jerry_get_boolean_value (jerry_has_own_property (object, symbol_2)));

  jerry_value_t get_value_1 = jerry_get_property (object, symbol_1);
  TEST_ASSERT (jerry_get_number_value (get_value_1) == jerry_get_number_value (value_1));
  jerry_release_value (get_value_1);

  jerry_value_t get_value_2 = jerry_get_property (object, symbol_2);
  TEST_ASSERT (jerry_get_number_value (get_value_2) == jerry_get_number_value (value_2));
  jerry_release_value (get_value_2);

  /* Test delete / has_{own}_property */
  TEST_ASSERT (jerry_delete_property (object, symbol_1));
  TEST_ASSERT (!jerry_get_boolean_value (jerry_has_property (object, symbol_1)));
  TEST_ASSERT (!jerry_get_boolean_value (jerry_has_own_property (object, symbol_1)));

  jerry_release_value (value_1);
  jerry_release_value (symbol_1);

  /* Test {get, define}_own_property_descriptor */
  jerry_property_descriptor_t prop_desc;
  TEST_ASSERT (jerry_get_own_property_descriptor (object, symbol_2, &prop_desc));
  TEST_ASSERT (prop_desc.is_value_defined == true);
  TEST_ASSERT (value_2 == prop_desc.value);
  TEST_ASSERT (jerry_get_number_value (value_2) == jerry_get_number_value (prop_desc.value));
  TEST_ASSERT (prop_desc.is_writable == true);
  TEST_ASSERT (prop_desc.is_enumerable == true);
  TEST_ASSERT (prop_desc.is_configurable == true);
  TEST_ASSERT (prop_desc.is_get_defined == false);
  TEST_ASSERT (jerry_value_is_undefined (prop_desc.getter));
  TEST_ASSERT (prop_desc.is_set_defined == false);
  TEST_ASSERT (jerry_value_is_undefined (prop_desc.setter));
  jerry_free_property_descriptor_fields (&prop_desc);

  /* Modify the descriptor fields */
  jerry_init_property_descriptor_fields (&prop_desc);
  jerry_value_t value_3 = jerry_create_string (STRING_BAR);

  prop_desc.is_value_defined = true;
  prop_desc.value = jerry_acquire_value (value_3);
  prop_desc.is_writable_defined = true;
  prop_desc.is_writable = false;
  prop_desc.is_enumerable_defined = true;
  prop_desc.is_enumerable = false;
  prop_desc.is_configurable_defined = true;
  prop_desc.is_configurable = false;
  TEST_ASSERT (jerry_get_boolean_value (jerry_define_own_property (object, symbol_2, &prop_desc)));
  jerry_free_property_descriptor_fields (&prop_desc);

  /* Check the modified fields */
  TEST_ASSERT (jerry_get_own_property_descriptor (object, symbol_2, &prop_desc));
  TEST_ASSERT (prop_desc.is_value_defined == true);
  TEST_ASSERT (value_3 == prop_desc.value);
  TEST_ASSERT (jerry_value_is_string (prop_desc.value));
  TEST_ASSERT (prop_desc.is_writable_defined == true);
  TEST_ASSERT (prop_desc.is_writable == false);
  TEST_ASSERT (prop_desc.is_enumerable_defined == true);
  TEST_ASSERT (prop_desc.is_enumerable == false);
  TEST_ASSERT (prop_desc.is_configurable_defined == true);
  TEST_ASSERT (prop_desc.is_configurable == false);
  TEST_ASSERT (prop_desc.is_get_defined == false);
  TEST_ASSERT (jerry_value_is_undefined (prop_desc.getter));
  TEST_ASSERT (prop_desc.is_set_defined == false);
  TEST_ASSERT (jerry_value_is_undefined (prop_desc.setter));
  jerry_free_property_descriptor_fields (&prop_desc);

  jerry_release_value (value_3);
  jerry_release_value (value_2);
  jerry_release_value (symbol_2);
  jerry_release_value (object);

  /* Test creating symbol with a symbol description */
  jerry_value_t empty_symbol_desc = jerry_create_string ((const jerry_char_t *) "");

  jerry_value_t empty_symbol = jerry_create_symbol (empty_symbol_desc);
  TEST_ASSERT (!jerry_value_is_error (empty_symbol));
  TEST_ASSERT (jerry_value_is_symbol (empty_symbol));

  jerry_release_value (empty_symbol_desc);

  jerry_value_t symbol_symbol = jerry_create_symbol (empty_symbol);
  TEST_ASSERT (!jerry_value_is_symbol (symbol_symbol));
  TEST_ASSERT (jerry_value_is_error (symbol_symbol));

  jerry_value_t error_obj = jerry_get_value_from_error (symbol_symbol, true);

  TEST_ASSERT (jerry_get_error_type (error_obj) == JERRY_ERROR_TYPE);

  jerry_release_value (error_obj);
  jerry_release_value (empty_symbol);

  /* Test symbol to string operation with symbol argument */
  jerry_value_t bar_symbol_desc = jerry_create_string (STRING_BAR);

  jerry_value_t bar_symbol = jerry_create_symbol (bar_symbol_desc);
  TEST_ASSERT (!jerry_value_is_error (bar_symbol));
  TEST_ASSERT (jerry_value_is_symbol (bar_symbol));

  jerry_release_value (bar_symbol_desc);

  jerry_value_t bar_symbol_string = jerry_get_symbol_descriptive_string (bar_symbol);
  TEST_ASSERT (jerry_value_is_string (bar_symbol_string));

  jerry_size_t bar_symbol_string_size = jerry_get_string_size (bar_symbol_string);
  TEST_ASSERT (bar_symbol_string_size == (sizeof (SYMBOL_DESCIPTIVE_STRING_BAR) - 1));
  JERRY_VLA (jerry_char_t, str_buff, bar_symbol_string_size);

  jerry_string_to_char_buffer (bar_symbol_string, str_buff, bar_symbol_string_size);
  TEST_ASSERT (memcmp (str_buff, SYMBOL_DESCIPTIVE_STRING_BAR, sizeof (SYMBOL_DESCIPTIVE_STRING_BAR) - 1) == 0);

  jerry_release_value (bar_symbol_string);
  jerry_release_value (bar_symbol);

  /* Test symbol to string operation with non-symbol argument */
  jerry_value_t null_value = jerry_create_null ();
  jerry_value_t to_string_value = jerry_get_symbol_descriptive_string (null_value);
  TEST_ASSERT (jerry_value_is_error (to_string_value));

  error_obj = jerry_get_value_from_error (to_string_value, true);

  TEST_ASSERT (jerry_get_error_type (error_obj) == JERRY_ERROR_TYPE);

  jerry_release_value (error_obj);
  jerry_release_value (null_value);

  jerry_cleanup ();

  return 0;
} /* main */

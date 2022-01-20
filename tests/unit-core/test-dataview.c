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

int
main (void)
{
  if (!jerry_feature_enabled (JERRY_FEATURE_DATAVIEW))
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "DataView support is disabled!\n");
    return 0;
  }

  /* DataView builtin requires the TypedArray builtin */
  TEST_ASSERT (jerry_feature_enabled (JERRY_FEATURE_TYPEDARRAY));

  jerry_init (JERRY_INIT_EMPTY);

  /* Test accessors */
  jerry_value_t arraybuffer = jerry_arraybuffer (16);
  jerry_value_t view1 = jerry_dataview (arraybuffer, 0, 16);
  TEST_ASSERT (!jerry_value_is_exception (view1));
  TEST_ASSERT (jerry_value_is_dataview (view1));

  jerry_length_t byteOffset = 0;
  jerry_length_t byteLength = 0;
  ;
  jerry_value_t internal_buffer = jerry_dataview_buffer (view1, &byteOffset, &byteLength);
  TEST_ASSERT (jerry_binary_op (JERRY_BIN_OP_STRICT_EQUAL, internal_buffer, arraybuffer));
  TEST_ASSERT (byteOffset == 0);
  TEST_ASSERT (byteLength == 16);
  jerry_value_free (internal_buffer);

  jerry_value_t view2 = jerry_dataview (arraybuffer, 12, 4);
  TEST_ASSERT (!jerry_value_is_exception (view1));
  TEST_ASSERT (jerry_value_is_dataview (view2));
  internal_buffer = jerry_dataview_buffer (view2, &byteOffset, &byteLength);
  TEST_ASSERT (jerry_binary_op (JERRY_BIN_OP_STRICT_EQUAL, internal_buffer, arraybuffer));
  TEST_ASSERT (byteOffset == 12);
  TEST_ASSERT (byteLength == 4);
  jerry_value_free (internal_buffer);

  /* Test invalid construction */
  jerry_value_t empty_object = jerry_object ();
  jerry_value_t view3 = jerry_dataview (empty_object, 20, 10);
  TEST_ASSERT (jerry_value_is_exception (view3));
  jerry_value_t error_obj = jerry_exception_value (view3, true);
  TEST_ASSERT (jerry_error_type (error_obj) == JERRY_ERROR_TYPE);
  jerry_value_free (error_obj);
  jerry_value_free (empty_object);

  jerry_value_t view4 = jerry_dataview (arraybuffer, 20, 10);
  TEST_ASSERT (jerry_value_is_exception (view3));
  error_obj = jerry_exception_value (view4, true);
  TEST_ASSERT (jerry_error_type (error_obj) == JERRY_ERROR_RANGE);
  jerry_value_free (error_obj);

  /* Test getting/setting values */
  jerry_value_t global_obj = jerry_current_realm ();
  jerry_value_t view1_str = jerry_string_sz ("view1");
  jerry_value_t view2_str = jerry_string_sz ("view2");
  TEST_ASSERT (jerry_object_set (global_obj, view1_str, view1));
  TEST_ASSERT (jerry_object_set (global_obj, view2_str, view2));

  jerry_value_free (view1_str);
  jerry_value_free (view2_str);
  jerry_value_free (global_obj);

  const jerry_char_t set_src[] = "view1.setInt16 (12, 255)";
  TEST_ASSERT (jerry_value_is_undefined (jerry_eval (set_src, sizeof (set_src) - 1, JERRY_PARSE_NO_OPTS)));

  const jerry_char_t get_src[] = "view2.getInt16 (0)";
  TEST_ASSERT (jerry_value_as_number (jerry_eval (get_src, sizeof (get_src) - 1, JERRY_PARSE_NO_OPTS)) == 255);

  const jerry_char_t get_src_little_endian[] = "view2.getInt16 (0, true)";
  TEST_ASSERT (
    jerry_value_as_number (jerry_eval (get_src_little_endian, sizeof (get_src_little_endian) - 1, JERRY_PARSE_NO_OPTS))
    == -256);

  /* Cleanup */
  jerry_value_free (view2);
  jerry_value_free (view1);
  jerry_value_free (arraybuffer);

  jerry_cleanup ();

  return 0;
} /* main */

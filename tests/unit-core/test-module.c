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

#include <string.h>

#include "jerryscript.h"
#include "test-common.h"

static void
compare_specifier (jerry_value_t specifier, /* string value */
                   int id) /* module id */
{
  jerry_char_t string[] = "XX_module.mjs";

  TEST_ASSERT (id >= 1 && id <= 99 && string[0] == 'X' && string[1] == 'X');

  string[0] = (jerry_char_t) ((id / 10) + '0');
  string[1] = (jerry_char_t) ((id % 10) + '0');

  jerry_size_t length = (jerry_size_t) (sizeof (string) - 1);
  jerry_char_t buffer[sizeof (string) - 1];

  TEST_ASSERT (jerry_value_is_string (specifier));
  TEST_ASSERT (jerry_get_string_size (specifier) == length);

  TEST_ASSERT (jerry_string_to_char_buffer (specifier, buffer, length) == length);
  TEST_ASSERT (memcmp (buffer, string, length) == 0);
} /* compare_specifier */

static jerry_value_t
create_module (int id) /**< module id */
{
  jerry_parse_options_t module_parse_options;
  module_parse_options.options = JERRY_PARSE_MODULE;

  jerry_value_t result;

  if (id == 0)
  {
    result = jerry_parse ((jerry_char_t *) "", 0, &module_parse_options);
  }
  else
  {
    jerry_char_t source[] = "import a from 'XX_module.mjs'";

    TEST_ASSERT (id >= 1 && id <= 99 && source[15] == 'X' && source[16] == 'X');

    source[15] = (jerry_char_t) ((id / 10) + '0');
    source[16] = (jerry_char_t) ((id % 10) + '0');

    result = jerry_parse (source, sizeof (source) - 1, &module_parse_options);
  }

  TEST_ASSERT (!jerry_value_is_error (result));
  return result;
} /* create_module */

static int counter = 0;
static jerry_value_t module;

static jerry_value_t
resolve_callback1 (const jerry_value_t specifier, /**< module specifier */
                   const jerry_value_t referrer, /**< parent module */
                   void *user_p) /**< user data */
{
  TEST_ASSERT (user_p == (void *) &module);
  TEST_ASSERT (referrer == module);
  compare_specifier (specifier, 1);

  counter++;
  return counter == 1 ? jerry_create_number (7) : jerry_create_object ();
} /* resolve_callback1 */

static jerry_value_t prev_module;
static bool terminate_with_error;

static jerry_value_t
resolve_callback2 (const jerry_value_t specifier, /**< module specifier */
                   const jerry_value_t referrer, /**< parent module */
                   void *user_p) /**< user data */
{
  TEST_ASSERT (prev_module == referrer);
  TEST_ASSERT (user_p == NULL);

  compare_specifier (specifier, ++counter);

  if (counter >= 32)
  {
    if (terminate_with_error)
    {
      return jerry_create_error (JERRY_ERROR_RANGE, (const jerry_char_t *) "Module not found");
    }

    return create_module (0);
  }

  prev_module = create_module (counter + 1);
  return prev_module;
} /* resolve_callback2 */

int
main (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  if (!jerry_is_feature_enabled (JERRY_FEATURE_MODULE))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Module is disabled!\n");
    jerry_cleanup ();
    return 0;
  }

  jerry_value_t number = jerry_create_number (5);
  jerry_value_t object = jerry_create_object ();

  jerry_value_t result = jerry_module_link (number, resolve_callback1, NULL);
  TEST_ASSERT (jerry_value_is_error (result));
  jerry_release_value (result);

  result = jerry_module_link (object, resolve_callback1, NULL);
  TEST_ASSERT (jerry_value_is_error (result));
  jerry_release_value (result);

  module = create_module (1);

  /* After an error, module must remain in unlinked mode. */
  result = jerry_module_link (module, resolve_callback1, (void *) &module);
  TEST_ASSERT (jerry_value_is_error (result));
  TEST_ASSERT (counter == 1);
  jerry_release_value (result);

  result = jerry_module_link (module, resolve_callback1, (void *) &module);
  TEST_ASSERT (jerry_value_is_error (result));
  TEST_ASSERT (counter == 2);
  jerry_release_value (result);

  prev_module = module;
  counter = 0;
  terminate_with_error = true;
  result = jerry_module_link (module, resolve_callback2, NULL);
  TEST_ASSERT (jerry_value_is_error (result));
  TEST_ASSERT (counter == 32);
  jerry_release_value (result);

  /* The successfully resolved modules is kept around in unlinked state. */
  jerry_gc (JERRY_GC_PRESSURE_HIGH);

  counter = 31;
  terminate_with_error = false;
  result = jerry_module_link (module, resolve_callback2, NULL);
  TEST_ASSERT (jerry_value_is_boolean (result) && jerry_get_boolean_value (result));
  TEST_ASSERT (counter == 32);
  jerry_release_value (result);
  jerry_release_value (module);

  module = create_module (1);

  prev_module = module;
  counter = 0;
  terminate_with_error = false;
  result = jerry_module_link (module, resolve_callback2, NULL);
  TEST_ASSERT (jerry_value_is_boolean (result) && jerry_get_boolean_value (result));
  TEST_ASSERT (counter == 32);
  jerry_release_value (result);
  jerry_release_value (module);

  jerry_release_value (object);
  jerry_release_value (number);

  jerry_cleanup ();

  return 0;
} /* main */

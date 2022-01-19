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
  TEST_ASSERT (jerry_string_size (specifier, JERRY_ENCODING_CESU8) == length);

  TEST_ASSERT (jerry_string_to_buffer (specifier, JERRY_ENCODING_CESU8, buffer, length) == length);
  TEST_ASSERT (memcmp (buffer, string, length) == 0);
} /* compare_specifier */

static void
compare_property (jerry_value_t namespace_object, /**< namespace object */
                  const char *name_p, /**< property name */
                  double expected_value) /**< property value (number for simplicity) */
{
  jerry_value_t name = jerry_string_sz (name_p);
  jerry_value_t result = jerry_object_get (namespace_object, name);

  TEST_ASSERT (jerry_value_is_number (result));
  TEST_ASSERT (jerry_value_as_number (result) == expected_value);

  jerry_value_free (result);
  jerry_value_free (name);
} /* compare_property */

static jerry_value_t
create_module (int id) /**< module id */
{
  jerry_parse_options_t module_parse_options;
  module_parse_options.options = JERRY_PARSE_MODULE;

  jerry_value_t result;

  if (id == 0)
  {
    jerry_char_t source[] = "export var a = 7";

    result = jerry_parse (source, sizeof (source) - 1, &module_parse_options);
  }
  else
  {
    jerry_char_t source[] = "export {a} from 'XX_module.mjs'";

    TEST_ASSERT (id >= 1 && id <= 99 && source[17] == 'X' && source[18] == 'X');

    source[17] = (jerry_char_t) ((id / 10) + '0');
    source[18] = (jerry_char_t) ((id % 10) + '0');

    result = jerry_parse (source, sizeof (source) - 1, &module_parse_options);
  }

  TEST_ASSERT (!jerry_value_is_exception (result));
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
  return counter == 1 ? jerry_number (7) : jerry_object ();
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
      return jerry_throw_sz (JERRY_ERROR_RANGE, "Module not found");
    }

    return create_module (0);
  }

  prev_module = create_module (counter + 1);
  return prev_module;
} /* resolve_callback2 */

static jerry_value_t
resolve_callback3 (const jerry_value_t specifier, /**< module specifier */
                   const jerry_value_t referrer, /**< parent module */
                   void *user_p) /**< user data */
{
  (void) specifier;
  (void) referrer;
  (void) user_p;

  TEST_ASSERT (false);
} /* resolve_callback3 */

static jerry_value_t
native_module_evaluate (const jerry_value_t native_module) /**< native module */
{
  ++counter;

  TEST_ASSERT (jerry_module_state (module) == JERRY_MODULE_STATE_EVALUATING);

  jerry_value_t exp_val = jerry_string_sz ("exp");
  jerry_value_t other_exp_val = jerry_string_sz ("other_exp");
  /* The native module has no such export. */
  jerry_value_t no_exp_val = jerry_string_sz ("no_exp");

  jerry_value_t result = jerry_native_module_get (native_module, exp_val);
  TEST_ASSERT (jerry_value_is_undefined (result));
  jerry_value_free (result);

  result = jerry_native_module_get (native_module, other_exp_val);
  TEST_ASSERT (jerry_value_is_undefined (result));
  jerry_value_free (result);

  result = jerry_native_module_get (native_module, no_exp_val);
  TEST_ASSERT (jerry_value_is_exception (result));
  TEST_ASSERT (jerry_error_type (result) == JERRY_ERROR_REFERENCE);
  jerry_value_free (result);

  jerry_value_t export = jerry_number (3.5);
  result = jerry_native_module_set (native_module, exp_val, export);
  TEST_ASSERT (jerry_value_is_boolean (result) && jerry_value_is_true (result));
  jerry_value_free (result);
  jerry_value_free (export);

  export = jerry_string_sz ("str");
  result = jerry_native_module_set (native_module, other_exp_val, export);
  TEST_ASSERT (jerry_value_is_boolean (result) && jerry_value_is_true (result));
  jerry_value_free (result);
  jerry_value_free (export);

  result = jerry_native_module_set (native_module, no_exp_val, no_exp_val);
  TEST_ASSERT (jerry_value_is_exception (result));
  TEST_ASSERT (jerry_error_type (result) == JERRY_ERROR_REFERENCE);
  jerry_value_free (result);

  result = jerry_native_module_get (native_module, exp_val);
  TEST_ASSERT (jerry_value_is_number (result) && jerry_value_as_number (result) == 3.5);
  jerry_value_free (result);

  result = jerry_native_module_get (native_module, other_exp_val);
  TEST_ASSERT (jerry_value_is_string (result));
  jerry_value_free (result);

  jerry_value_free (exp_val);
  jerry_value_free (other_exp_val);
  jerry_value_free (no_exp_val);

  if (counter == 4)
  {
    ++counter;
    return jerry_throw_sz (JERRY_ERROR_COMMON, "Ooops!");
  }

  return jerry_undefined ();
} /* native_module_evaluate */

static jerry_value_t
resolve_callback4 (const jerry_value_t specifier, /**< module specifier */
                   const jerry_value_t referrer, /**< parent module */
                   void *user_p) /**< user data */
{
  (void) specifier;
  (void) referrer;

  ++counter;

  jerry_value_t exports[2] = { jerry_string_sz ("exp"), jerry_string_sz ("other_exp") };

  jerry_value_t native_module = jerry_native_module (native_module_evaluate, exports, 2);
  TEST_ASSERT (!jerry_value_is_exception (native_module));

  jerry_value_free (exports[0]);
  jerry_value_free (exports[1]);

  *((jerry_value_t *) user_p) = jerry_value_copy (native_module);
  return native_module;
} /* resolve_callback4 */

static void
module_state_changed (jerry_module_state_t new_state, /**< new state of the module */
                      const jerry_value_t module_val, /**< a module whose state is changed */
                      const jerry_value_t value, /**< value argument */
                      void *user_p) /**< user pointer */
{
  TEST_ASSERT (jerry_module_state (module_val) == new_state);
  TEST_ASSERT (module_val == module);
  TEST_ASSERT (user_p == (void *) &counter);

  ++counter;

  switch (counter)
  {
    case 1:
    case 3:
    {
      TEST_ASSERT (new_state == JERRY_MODULE_STATE_LINKED);
      TEST_ASSERT (jerry_value_is_undefined (value));
      break;
    }
    case 2:
    {
      TEST_ASSERT (new_state == JERRY_MODULE_STATE_EVALUATED);
      TEST_ASSERT (jerry_value_is_number (value) && jerry_value_as_number (value) == 33.5);
      break;
    }
    default:
    {
      TEST_ASSERT (counter == 4);
      TEST_ASSERT (new_state == JERRY_MODULE_STATE_ERROR);
      TEST_ASSERT (jerry_value_is_number (value) && jerry_value_as_number (value) == -5.5);
      break;
    }
  }
} /* module_state_changed */

static jerry_value_t
resolve_callback5 (const jerry_value_t specifier, /**< module specifier */
                   const jerry_value_t referrer, /**< parent module */
                   void *user_p) /**< user data */
{
  (void) specifier;
  (void) user_p;

  /* This circular reference is valid. However, import resolving triggers
   * a SyntaxError, because the module does not export a default binding. */
  return referrer;
} /* resolve_callback5 */

int
main (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  if (!jerry_feature_enabled (JERRY_FEATURE_MODULE))
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Module is disabled!\n");
    jerry_cleanup ();
    return 0;
  }

  jerry_value_t number = jerry_number (5);
  jerry_value_t object = jerry_object ();

  jerry_value_t result = jerry_module_link (number, resolve_callback1, NULL);
  TEST_ASSERT (jerry_value_is_exception (result));
  jerry_value_free (result);

  result = jerry_module_link (object, resolve_callback1, NULL);
  TEST_ASSERT (jerry_value_is_exception (result));
  jerry_value_free (result);

  module = create_module (1);

  /* After an error, module must remain in unlinked mode. */
  result = jerry_module_link (module, resolve_callback1, (void *) &module);
  TEST_ASSERT (jerry_value_is_exception (result));
  TEST_ASSERT (counter == 1);
  jerry_value_free (result);

  result = jerry_module_link (module, resolve_callback1, (void *) &module);
  TEST_ASSERT (jerry_value_is_exception (result));
  TEST_ASSERT (counter == 2);
  jerry_value_free (result);

  prev_module = module;
  counter = 0;
  terminate_with_error = true;
  result = jerry_module_link (module, resolve_callback2, NULL);
  TEST_ASSERT (jerry_value_is_exception (result));
  TEST_ASSERT (counter == 32);
  jerry_value_free (result);

  /* The successfully resolved modules is kept around in unlinked state. */
  jerry_heap_gc (JERRY_GC_PRESSURE_HIGH);

  counter = 31;
  terminate_with_error = false;
  result = jerry_module_link (module, resolve_callback2, NULL);
  TEST_ASSERT (jerry_value_is_boolean (result) && jerry_value_is_true (result));
  TEST_ASSERT (counter == 32);
  jerry_value_free (result);

  TEST_ASSERT (jerry_module_state (module) == JERRY_MODULE_STATE_LINKED);
  TEST_ASSERT (jerry_module_request_count (module) == 1);
  result = jerry_module_request (module, 0);
  TEST_ASSERT (jerry_module_state (module) == JERRY_MODULE_STATE_LINKED);
  jerry_value_free (result);

  jerry_value_free (module);

  module = create_module (1);

  prev_module = module;
  counter = 0;
  terminate_with_error = false;
  result = jerry_module_link (module, resolve_callback2, NULL);
  TEST_ASSERT (jerry_value_is_boolean (result) && jerry_value_is_true (result));
  TEST_ASSERT (counter == 32);
  jerry_value_free (result);
  jerry_value_free (module);

  TEST_ASSERT (jerry_module_state (number) == JERRY_MODULE_STATE_INVALID);

  jerry_parse_options_t module_parse_options;
  module_parse_options.options = JERRY_PARSE_MODULE;

  jerry_char_t source1[] = TEST_STRING_LITERAL ("import a from '16_module.mjs'\n"
                                                "export * from '07_module.mjs'\n"
                                                "export * from '44_module.mjs'\n"
                                                "import * as b from '36_module.mjs'\n");
  module = jerry_parse (source1, sizeof (source1) - 1, &module_parse_options);
  TEST_ASSERT (!jerry_value_is_exception (module));
  TEST_ASSERT (jerry_module_state (module) == JERRY_MODULE_STATE_UNLINKED);

  TEST_ASSERT (jerry_module_request_count (number) == 0);
  TEST_ASSERT (jerry_module_request_count (module) == 4);

  result = jerry_module_request (object, 0);
  TEST_ASSERT (jerry_value_is_exception (result));
  jerry_value_free (result);

  result = jerry_module_request (module, 0);
  compare_specifier (result, 16);
  jerry_value_free (result);

  result = jerry_module_request (module, 1);
  compare_specifier (result, 7);
  jerry_value_free (result);

  result = jerry_module_request (module, 2);
  compare_specifier (result, 44);
  jerry_value_free (result);

  result = jerry_module_request (module, 3);
  compare_specifier (result, 36);
  jerry_value_free (result);

  result = jerry_module_request (module, 4);
  TEST_ASSERT (jerry_value_is_exception (result));
  jerry_value_free (result);

  jerry_value_free (module);

  result = jerry_module_namespace (number);
  TEST_ASSERT (jerry_value_is_exception (result));
  jerry_value_free (result);

  jerry_char_t source2[] = TEST_STRING_LITERAL ("export let a = 6\n"
                                                "export let b = 8.5\n");
  module = jerry_parse (source2, sizeof (source2) - 1, &module_parse_options);
  TEST_ASSERT (!jerry_value_is_exception (module));
  TEST_ASSERT (jerry_module_state (module) == JERRY_MODULE_STATE_UNLINKED);

  result = jerry_module_link (module, resolve_callback3, NULL);
  TEST_ASSERT (!jerry_value_is_exception (result));
  jerry_value_free (result);

  TEST_ASSERT (jerry_module_state (module) == JERRY_MODULE_STATE_LINKED);

  result = jerry_module_evaluate (module);
  TEST_ASSERT (!jerry_value_is_exception (result));
  jerry_value_free (result);

  TEST_ASSERT (jerry_module_state (module) == JERRY_MODULE_STATE_EVALUATED);

  result = jerry_module_namespace (module);
  TEST_ASSERT (jerry_value_is_object (result));
  compare_property (result, "a", 6);
  compare_property (result, "b", 8.5);
  jerry_value_free (result);

  jerry_value_free (module);

  module = jerry_native_module (NULL, &object, 1);
  TEST_ASSERT (jerry_value_is_exception (module));
  jerry_value_free (module);

  module = jerry_native_module (NULL, NULL, 0);
  TEST_ASSERT (!jerry_value_is_exception (module));
  TEST_ASSERT (jerry_module_state (module) == JERRY_MODULE_STATE_UNLINKED);

  result = jerry_native_module_get (object, number);
  TEST_ASSERT (jerry_value_is_exception (result));
  jerry_value_free (result);

  result = jerry_native_module_set (module, number, number);
  TEST_ASSERT (jerry_value_is_exception (result));
  jerry_value_free (result);

  jerry_value_free (module);

  /* Valid identifier. */
  jerry_value_t export = jerry_string_sz ("\xed\xa0\x83\xed\xb2\x80");

  module = jerry_native_module (NULL, &export, 1);
  TEST_ASSERT (!jerry_value_is_exception (module));
  TEST_ASSERT (jerry_module_state (module) == JERRY_MODULE_STATE_UNLINKED);

  result = jerry_module_link (module, NULL, NULL);
  TEST_ASSERT (jerry_value_is_boolean (result) && jerry_value_is_true (result));
  jerry_value_free (result);

  result = jerry_module_evaluate (module);
  TEST_ASSERT (jerry_value_is_undefined (result));
  jerry_value_free (result);

  jerry_value_free (module);
  jerry_value_free (export);

  /* Invalid identifiers. */
  export = jerry_string_sz ("a+");
  module = jerry_native_module (NULL, &export, 1);
  TEST_ASSERT (jerry_value_is_exception (module));
  jerry_value_free (module);
  jerry_value_free (export);

  export = jerry_string_sz ("\xed\xa0\x80");
  module = jerry_native_module (NULL, &export, 1);
  TEST_ASSERT (jerry_value_is_exception (module));
  jerry_value_free (module);
  jerry_value_free (export);

  counter = 0;

  for (int i = 0; i < 2; i++)
  {
    jerry_char_t source3[] = TEST_STRING_LITERAL (
      "import {exp, other_exp as other} from 'native.js'\n"
      "import * as namespace from 'native.js'\n"
      "if (exp !== 3.5 || other !== 'str') { throw 'Assertion failed!' }\n"
      "if (namespace.exp !== 3.5 || namespace.other_exp !== 'str') { throw 'Assertion failed!' }\n");
    module = jerry_parse (source3, sizeof (source3) - 1, &module_parse_options);
    TEST_ASSERT (!jerry_value_is_exception (module));
    TEST_ASSERT (jerry_module_state (module) == JERRY_MODULE_STATE_UNLINKED);

    jerry_value_t native_module;

    result = jerry_module_link (module, resolve_callback4, (void *) &native_module);
    TEST_ASSERT (!jerry_value_is_exception (result));
    jerry_value_free (result);

    TEST_ASSERT (counter == i * 2 + 1);
    TEST_ASSERT (jerry_module_state (module) == JERRY_MODULE_STATE_LINKED);
    TEST_ASSERT (jerry_module_state (native_module) == JERRY_MODULE_STATE_LINKED);

    result = jerry_module_evaluate (module);

    if (i == 0)
    {
      TEST_ASSERT (!jerry_value_is_exception (result));
      TEST_ASSERT (jerry_module_state (module) == JERRY_MODULE_STATE_EVALUATED);
      TEST_ASSERT (jerry_module_state (native_module) == JERRY_MODULE_STATE_EVALUATED);
      TEST_ASSERT (counter == 2);
    }
    else
    {
      TEST_ASSERT (jerry_value_is_exception (result));
      TEST_ASSERT (jerry_module_state (module) == JERRY_MODULE_STATE_ERROR);
      TEST_ASSERT (jerry_module_state (native_module) == JERRY_MODULE_STATE_ERROR);
      TEST_ASSERT (counter == 5);
    }

    jerry_value_free (result);
    jerry_value_free (module);
    jerry_value_free (native_module);
  }

  jerry_value_free (object);
  jerry_value_free (number);

  counter = 0;
  jerry_module_on_state_changed (module_state_changed, (void *) &counter);

  jerry_char_t source4[] = TEST_STRING_LITERAL ("33.5\n");
  module = jerry_parse (source4, sizeof (source4) - 1, &module_parse_options);

  result = jerry_module_link (module, NULL, NULL);
  TEST_ASSERT (!jerry_value_is_exception (result));
  jerry_value_free (result);

  result = jerry_module_evaluate (module);
  TEST_ASSERT (!jerry_value_is_exception (result));
  jerry_value_free (result);

  jerry_value_free (module);

  jerry_char_t source5[] = TEST_STRING_LITERAL ("throw -5.5\n");
  module = jerry_parse (source5, sizeof (source5) - 1, &module_parse_options);

  result = jerry_module_link (module, NULL, NULL);
  TEST_ASSERT (!jerry_value_is_exception (result));
  jerry_value_free (result);

  result = jerry_module_evaluate (module);
  TEST_ASSERT (jerry_value_is_exception (result));
  jerry_value_free (result);

  jerry_value_free (module);

  jerry_module_on_state_changed (NULL, NULL);

  TEST_ASSERT (counter == 4);

  jerry_char_t source6[] = TEST_STRING_LITERAL ("import a from 'self'\n");
  module = jerry_parse (source6, sizeof (source6) - 1, &module_parse_options);

  result = jerry_module_link (module, resolve_callback5, NULL);
  TEST_ASSERT (jerry_value_is_exception (result) && jerry_error_type (result) == JERRY_ERROR_SYNTAX);
  jerry_value_free (result);

  jerry_cleanup ();

  return 0;
} /* main */

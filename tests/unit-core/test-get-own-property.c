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

static jerry_value_t
create_object (const char *source_p) /**< source script */
{
  jerry_value_t result = jerry_eval ((const jerry_char_t *) source_p, strlen (source_p), 0);
  TEST_ASSERT (jerry_value_is_object (result));
  return result;
} /* create_object */

static void
compare_string (jerry_value_t value, /**< value to compare */
                const char *string_p) /**< expected value */
{
  jerry_char_t string_buffer[64];

  TEST_ASSERT (jerry_value_is_string (value));

  size_t size = strlen (string_p);
  TEST_ASSERT (size <= sizeof (string_buffer));
  TEST_ASSERT (size == jerry_get_string_size (value));

  jerry_string_to_char_buffer (value, string_buffer, (jerry_size_t) size);
  TEST_ASSERT (memcmp (string_p, string_buffer, size) == 0);
} /* compare_string */

int
main (void)
{
  TEST_INIT ();
  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t pp_string = jerry_create_string ((const jerry_char_t *) "pp");
  jerry_value_t qq_string = jerry_create_string ((const jerry_char_t *) "qq");
  jerry_value_t rr_string = jerry_create_string ((const jerry_char_t *) "rr");

  jerry_value_t object = create_object ("'use strict';\n"
                                        "({ pp:'A', get qq() { return 'B' } })");

  jerry_value_t result = jerry_get_own_property (object, pp_string, object, NULL);
  compare_string (result, "A");
  jerry_release_value (result);

  bool found = false;
  result = jerry_get_own_property (object, pp_string, object, &found);
  compare_string (result, "A");
  TEST_ASSERT (found);
  jerry_release_value (result);

  result = jerry_get_own_property (object, qq_string, object, NULL);
  compare_string (result, "B");
  jerry_release_value (result);

  found = false;
  result = jerry_get_own_property (object, qq_string, object, &found);
  compare_string (result, "B");
  TEST_ASSERT (found);
  jerry_release_value (result);

  result = jerry_get_own_property (object, rr_string, object, NULL);
  TEST_ASSERT (jerry_value_is_undefined (result));
  jerry_release_value (result);

  found = true;
  result = jerry_get_own_property (object, rr_string, object, &found);
  TEST_ASSERT (jerry_value_is_undefined (result));
  TEST_ASSERT (!found);
  jerry_release_value (result);

  jerry_release_value (object);

  object = create_object ("'use strict';\n"
                          "Object.create({ pp:'Found!' })\n");

  found = true;
  /* Does not check prototype. */
  result = jerry_get_own_property (object, pp_string, object, &found);
  TEST_ASSERT (jerry_value_is_undefined (result));
  TEST_ASSERT (!found);
  jerry_release_value (result);

  jerry_release_value (object);

  object = create_object ("'use strict';\n"
                          "var obj = Object.create({ get pp() { return this.qq } })\n"
                          "Object.defineProperty(obj, 'qq', { value: 'Prop' })\n"
                          "obj");
  jerry_value_t prototype = jerry_get_prototype (object);

  TEST_ASSERT (jerry_value_is_object (prototype));
  found = false;
  result = jerry_get_own_property (prototype, pp_string, object, &found);
  compare_string (result, "Prop");
  TEST_ASSERT (found);
  jerry_release_value (result);

  jerry_release_value (prototype);
  jerry_release_value (object);

  /* Error cases. */
  jerry_value_t invalid_arg = jerry_create_null ();
  object = jerry_create_object ();

  found = true;
  result = jerry_get_own_property (invalid_arg, pp_string, object, &found);
  TEST_ASSERT (jerry_value_is_error (result));
  TEST_ASSERT (!found);
  jerry_release_value (result);

  result = jerry_get_own_property (object, pp_string, invalid_arg, NULL);
  TEST_ASSERT (jerry_value_is_error (result));
  jerry_release_value (result);

  found = true;
  result = jerry_get_own_property (object, invalid_arg, object, &found);
  TEST_ASSERT (jerry_value_is_error (result));
  TEST_ASSERT (!found);
  jerry_release_value (result);

  jerry_release_value (object);
  jerry_release_value (invalid_arg);

  if (jerry_is_feature_enabled (JERRY_FEATURE_PROXY))
  {
    object = create_object ("'use strict';\n"
                            "var proxy = new Proxy({}, {\n"
                            "    get: function(target, prop, receiver) {\n"
                            "        if (prop === 'qq') return\n"
                            "        return receiver[prop]\n"
                            "    }\n"
                            "})\n"
                            "var obj = Object.create(proxy)\n"
                            "Object.defineProperty(obj, 'pp', { value: 'Prop' })\n"
                            "obj");

    prototype = jerry_get_prototype (object);
    found = false;
    result = jerry_get_own_property (prototype, pp_string, object, &found);
    compare_string (result, "Prop");
    TEST_ASSERT (found);
    jerry_release_value (result);

    found = false;
    result = jerry_get_own_property (prototype, qq_string, object, &found);
    TEST_ASSERT (jerry_value_is_undefined (result));
    TEST_ASSERT (found);
    jerry_release_value (result);

    jerry_release_value (prototype);
    jerry_release_value (object);

    object = create_object ("'use strict';\n"
                            "(new Proxy({}, {\n"
                            "    get: function(target, prop, receiver) {\n"
                            "        throw 'Error'\n"
                            "    }\n"
                            "}))\n");

    found = false;
    result = jerry_get_own_property (object, qq_string, object, &found);
    TEST_ASSERT (jerry_value_is_error (result));
    TEST_ASSERT (found);
    jerry_release_value (result);

    jerry_release_value (object);
  }

  if (jerry_is_feature_enabled (JERRY_FEATURE_SYMBOL))
  {
    object = create_object ("'use strict'\n"
                            "var sym = Symbol();\n"
                            "({ pp:sym, [sym]:'Prop' })");

    found = false;
    jerry_value_t symbol = jerry_get_own_property (object, pp_string, object, &found);
    TEST_ASSERT (jerry_value_is_symbol (symbol));
    TEST_ASSERT (found);

    found = false;
    result = jerry_get_own_property (object, symbol, object, &found);
    compare_string (result, "Prop");
    TEST_ASSERT (found);
    jerry_release_value (result);

    jerry_release_value (symbol);
    jerry_release_value (object);
  }

  jerry_release_value (pp_string);
  jerry_release_value (qq_string);
  jerry_release_value (rr_string);

  jerry_cleanup ();
  return 0;
} /* main */

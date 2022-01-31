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
  TEST_ASSERT (size == jerry_string_size (value, JERRY_ENCODING_CESU8));

  jerry_string_to_buffer (value, JERRY_ENCODING_CESU8, string_buffer, (jerry_size_t) size);
  TEST_ASSERT (memcmp (string_p, string_buffer, size) == 0);
} /* compare_string */

int
main (void)
{
  TEST_INIT ();
  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t pp_string = jerry_string_sz ("pp");
  jerry_value_t qq_string = jerry_string_sz ("qq");
  jerry_value_t rr_string = jerry_string_sz ("rr");

  jerry_value_t object = create_object ("'use strict';\n"
                                        "({ pp:'A', get qq() { return 'B' } })");

  jerry_value_t result = jerry_object_find_own (object, pp_string, object, NULL);
  compare_string (result, "A");
  jerry_value_free (result);

  bool found = false;
  result = jerry_object_find_own (object, pp_string, object, &found);
  compare_string (result, "A");
  TEST_ASSERT (found);
  jerry_value_free (result);

  result = jerry_object_find_own (object, qq_string, object, NULL);
  compare_string (result, "B");
  jerry_value_free (result);

  found = false;
  result = jerry_object_find_own (object, qq_string, object, &found);
  compare_string (result, "B");
  TEST_ASSERT (found);
  jerry_value_free (result);

  result = jerry_object_find_own (object, rr_string, object, NULL);
  TEST_ASSERT (jerry_value_is_undefined (result));
  jerry_value_free (result);

  found = true;
  result = jerry_object_find_own (object, rr_string, object, &found);
  TEST_ASSERT (jerry_value_is_undefined (result));
  TEST_ASSERT (!found);
  jerry_value_free (result);

  jerry_value_free (object);

  object = create_object ("'use strict';\n"
                          "Object.create({ pp:'Found!' })\n");

  found = true;
  /* Does not check prototype. */
  result = jerry_object_find_own (object, pp_string, object, &found);
  TEST_ASSERT (jerry_value_is_undefined (result));
  TEST_ASSERT (!found);
  jerry_value_free (result);

  jerry_value_free (object);

  object = create_object ("'use strict';\n"
                          "var obj = Object.create({ get pp() { return this.qq } })\n"
                          "Object.defineProperty(obj, 'qq', { value: 'Prop' })\n"
                          "obj");
  jerry_value_t prototype = jerry_object_proto (object);

  TEST_ASSERT (jerry_value_is_object (prototype));
  found = false;
  result = jerry_object_find_own (prototype, pp_string, object, &found);
  compare_string (result, "Prop");
  TEST_ASSERT (found);
  jerry_value_free (result);

  jerry_value_free (prototype);
  jerry_value_free (object);

  /* Error cases. */
  jerry_value_t invalid_arg = jerry_null ();
  object = jerry_object ();

  found = true;
  result = jerry_object_find_own (invalid_arg, pp_string, object, &found);
  TEST_ASSERT (jerry_value_is_exception (result));
  TEST_ASSERT (!found);
  jerry_value_free (result);

  result = jerry_object_find_own (object, pp_string, invalid_arg, NULL);
  TEST_ASSERT (jerry_value_is_exception (result));
  jerry_value_free (result);

  found = true;
  result = jerry_object_find_own (object, invalid_arg, object, &found);
  TEST_ASSERT (jerry_value_is_exception (result));
  TEST_ASSERT (!found);
  jerry_value_free (result);

  jerry_value_free (object);
  jerry_value_free (invalid_arg);

  if (jerry_feature_enabled (JERRY_FEATURE_PROXY))
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

    prototype = jerry_object_proto (object);
    found = false;
    result = jerry_object_find_own (prototype, pp_string, object, &found);
    compare_string (result, "Prop");
    TEST_ASSERT (found);
    jerry_value_free (result);

    found = false;
    result = jerry_object_find_own (prototype, qq_string, object, &found);
    TEST_ASSERT (jerry_value_is_undefined (result));
    TEST_ASSERT (found);
    jerry_value_free (result);

    jerry_value_free (prototype);
    jerry_value_free (object);

    object = create_object ("'use strict';\n"
                            "(new Proxy({}, {\n"
                            "    get: function(target, prop, receiver) {\n"
                            "        throw 'Error'\n"
                            "    }\n"
                            "}))\n");

    found = false;
    result = jerry_object_find_own (object, qq_string, object, &found);
    TEST_ASSERT (jerry_value_is_exception (result));
    TEST_ASSERT (found);
    jerry_value_free (result);

    jerry_value_free (object);
  }

  object = create_object ("'use strict'\n"
                          "var sym = Symbol();\n"
                          "({ pp:sym, [sym]:'Prop' })");

  found = false;
  jerry_value_t symbol = jerry_object_find_own (object, pp_string, object, &found);
  TEST_ASSERT (jerry_value_is_symbol (symbol));
  TEST_ASSERT (found);

  found = false;
  result = jerry_object_find_own (object, symbol, object, &found);
  compare_string (result, "Prop");
  TEST_ASSERT (found);
  jerry_value_free (result);

  jerry_value_free (symbol);
  jerry_value_free (object);

  jerry_value_free (pp_string);
  jerry_value_free (qq_string);
  jerry_value_free (rr_string);

  jerry_cleanup ();
  return 0;
} /* main */

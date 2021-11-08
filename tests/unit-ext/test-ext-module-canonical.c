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

#include "jerryscript-ext/module.h"
#include "test-common.h"

#define ACTUAL_NAME "alice"
#define ALIAS_NAME  "bob"

static jerry_value_t
get_canonical_name (const jerry_value_t name)
{
  jerry_size_t name_size = jerry_string_size (name, JERRY_ENCODING_CESU8);
  JERRY_VLA (jerry_char_t, name_string, name_size + 1);
  jerry_string_to_buffer (name, JERRY_ENCODING_CESU8, name_string, name_size);
  name_string[name_size] = 0;

  if (!strcmp ((char *) name_string, ACTUAL_NAME))
  {
    return jerry_value_copy (name);
  }
  else if (!strcmp ((char *) name_string, ALIAS_NAME))
  {
    return jerry_string_sz (ACTUAL_NAME);
  }
  else
  {
    return jerry_undefined ();
  }
} /* get_canonical_name */

static bool
resolve (const jerry_value_t canonical_name, jerry_value_t *result)
{
  jerry_size_t name_size = jerry_string_size (canonical_name, JERRY_ENCODING_CESU8);
  JERRY_VLA (jerry_char_t, name_string, name_size + 1);
  jerry_string_to_buffer (canonical_name, JERRY_ENCODING_CESU8, name_string, name_size);
  name_string[name_size] = 0;

  if (!strcmp ((char *) name_string, ACTUAL_NAME))
  {
    *result = jerry_object ();
    return true;
  }
  return false;
} /* resolve */

static const jerryx_module_resolver_t canonical_test = { .get_canonical_name_p = get_canonical_name,
                                                         .resolve_p = resolve };

#define TEST_VALUE 95.0

int
main (int argc, char **argv)
{
  (void) argc;
  (void) argv;

  const jerryx_module_resolver_t *resolver = &canonical_test;

  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t actual_name = jerry_string_sz (ACTUAL_NAME);
  jerry_value_t alias_name = jerry_string_sz (ALIAS_NAME);

  /* It's important that we resolve by the non-canonical name first. */
  jerry_value_t result2 = jerryx_module_resolve (alias_name, &resolver, 1);
  jerry_value_t result1 = jerryx_module_resolve (actual_name, &resolver, 1);
  jerry_value_free (actual_name);
  jerry_value_free (alias_name);

  /* An elaborate way of doing strict equal - set a property on one object and it "magically" appears on the other. */
  jerry_value_t prop_name = jerry_string_sz ("something");
  jerry_value_t prop_value = jerry_number (TEST_VALUE);
  jerry_value_free (jerry_object_set (result1, prop_name, prop_value));
  jerry_value_free (prop_value);

  prop_value = jerry_object_get (result2, prop_name);
  TEST_ASSERT (jerry_value_as_number (prop_value) == TEST_VALUE);
  jerry_value_free (prop_value);

  jerry_value_free (prop_name);
  jerry_value_free (result1);
  jerry_value_free (result2);

  jerry_cleanup ();

  return 0;
} /* main */

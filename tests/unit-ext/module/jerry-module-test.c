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
#include "jerryscript-ext/module.h"

#ifdef JERRYX_NATIVE_MODULES_SUPPORTED

/* Load a module. */
const char eval_string1[] = "require ('my_custom_module');";

/* Load a module using a different resolver. */
const char eval_string2[] = "require ('differently-handled-module');";

/* Load a broken module using the built-in resolver. */
const char eval_string3[] =
"(function() {"
"  var theError;"
"  try {"
"    require ('my_broken_module');"
"  } catch (anError) {"
"    theError = anError;"
"  }"
"  return (((theError.message === 'Module on_resolve () must not be NULL') &&"
"    (theError.moduleName === 'my_broken_module') &&"
"    (theError instanceof TypeError)) ? 1 : 0);"
"}) ();";

/* Load a non-existent module. */
const char eval_string4[] =
"(function() {"
"  var theError;"
"  try {"
"    require ('some_missing_module_xyzzy');"
"  } catch (anError) {"
"    theError = anError;"
"  }"
"  return (((theError.message === 'Module not found') &&"
"    (theError.moduleName === 'some_missing_module_xyzzy')) ? 1 : 0);"
"}) ();";

/* Make sure the result of a module load is cached. */
const char eval_string5[] =
"(function() {"
"  var x = require('cache-check');"
"  var y = require('cache-check');"
"  return x === y ? 1 : 0;"
"}) ();";

static bool
resolve_differently_handled_module (const jerry_char_t *name,
                                                jerry_value_t *result)
{
  if (!strcmp ((char *) name, "differently-handled-module"))
  {
    (*result) = jerry_create_number (29);
    return true;
  }
  return false;
} /* resolve_differently_handled_module */

/*
 * Define module "cache-check" via its own resolver as an empty object. Since objects are accessible only via references
 * we can strictly compare the object returned on subsequent attempts at loading "cache-check" with the object returned
 * on the first attempt and establish that the two are in fact the same object - which in turn shows that caching works.
 */
static bool
cache_check (const jerry_char_t *name,
             jerry_value_t *result)
{
  if (!strcmp ((char *) name, "cache-check"))
  {
    (*result) = jerry_create_object ();
    return true;
  }
  return false;
} /* cache_check */

static const jerryx_module_resolver_t resolvers[3] =
{
  jerryx_module_native_resolver,
  resolve_differently_handled_module,
  cache_check
};

static jerry_value_t
handle_require (const jerry_value_t js_function,
                const jerry_value_t this_val,
                const jerry_value_t args_p[],
                const jerry_length_t args_count)
{
  (void) js_function;
  (void) this_val;
  (void) args_count;

  jerry_value_t return_value = 0;
  jerry_char_t module_name[256] = "";
  jerry_size_t bytes_copied = 0;

  TEST_ASSERT (args_count == 1);
  bytes_copied = jerry_string_to_char_buffer (args_p[0], module_name, 256);
  if (bytes_copied < 256)
  {
    module_name[bytes_copied] = 0;
    return_value = jerryx_module_resolve (module_name, resolvers, 3);
  }

  return return_value;
} /* handle_require */

static void
assert_number (jerry_value_t js_value, double expected_result)
{
  TEST_ASSERT (!jerry_value_has_error_flag (js_value));
  TEST_ASSERT (jerry_get_number_value (js_value) == expected_result);
} /* assert_number */

static void
eval_one (const char *the_string, double expected_result)
{
  jerry_value_t js_eval_result = jerry_eval ((const jerry_char_t *) the_string, strlen (the_string), true);
  assert_number (js_eval_result, expected_result);
  jerry_release_value (js_eval_result);
} /* eval_one */

int
main (int argc, char **argv)
{
  (void) argc;
  (void) argv;
  jerry_value_t js_global = 0, js_function = 0, js_property_name = 0;

  jerry_init (JERRY_INIT_EMPTY);

  js_global = jerry_get_global_object ();
  js_function = jerry_create_external_function (handle_require);
  js_property_name = jerry_create_string ((const jerry_char_t *) "require");
  jerry_set_property (js_global, js_property_name, js_function);

  eval_one (eval_string1, 42);
  eval_one (eval_string2, 29);
  eval_one (eval_string3, 1);
  eval_one (eval_string4, 1);
  eval_one (eval_string5, 1);

  jerry_release_value (js_property_name);
  jerry_release_value (js_function);
  jerry_release_value (js_global);

  jerry_cleanup ();
} /* main */

#endif /* JERRYX_NATIVE_MODULES_SUPPORTED */

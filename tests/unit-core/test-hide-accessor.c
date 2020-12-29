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

static jerry_value_t
define_da_handler (const jerry_value_t func_obj_val, /**< function object */
                   const jerry_value_t this_val, /**< this arg */
                   const jerry_value_t args_p[], /**< function arguments */
                   const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) func_obj_val;
  (void) this_val;

  if (args_cnt < 4)
  {
    return jerry_create_error (JERRY_ERROR_RANGE, (const jerry_char_t *) "Four arguments expected");
  }

  jerry_property_descriptor_t descriptor;
  jerry_init_property_descriptor_fields (&descriptor);

  descriptor.is_get_defined = true;
  descriptor.is_set_defined = true;
  descriptor.is_enumerable_defined = true;
  descriptor.is_enumerable = true;
  descriptor.is_configurable_defined = true;
  descriptor.is_configurable = (args_cnt >= 4
                                && jerry_value_is_boolean (args_p[4])
                                && jerry_get_boolean_value (args_p[4]));
  descriptor.is_data_accessor = true;
  descriptor.getter = jerry_acquire_value (args_p[2]);
  descriptor.setter = jerry_acquire_value (args_p[3]);

  jerry_value_t result = jerry_define_own_property (args_p[0], args_p[1], &descriptor);

  jerry_free_property_descriptor_fields (&descriptor);
  return result;
} /* define_da_handler */

static jerry_value_t
check_da_handler (const jerry_value_t func_obj_val, /**< function object */
                  const jerry_value_t this_val, /**< this arg */
                  const jerry_value_t args_p[], /**< function arguments */
                  const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) func_obj_val;
  (void) this_val;

  if (args_cnt < 2)
  {
    return jerry_create_error (JERRY_ERROR_RANGE, (const jerry_char_t *) "Two arguments expected");
  }

  jerry_property_descriptor_t descriptor;

  TEST_ASSERT (jerry_get_own_property_descriptor (args_p[0], args_p[1], &descriptor));
  TEST_ASSERT (descriptor.is_data_accessor);

  jerry_free_property_descriptor_fields (&descriptor);
  return jerry_create_undefined ();
} /* check_da_handler */

static jerry_value_t
assert_handler (const jerry_value_t func_obj_val, /**< function object */
                const jerry_value_t this_val, /**< this arg */
                const jerry_value_t args_p[], /**< function arguments */
                const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) func_obj_val;
  (void) this_val;

  TEST_ASSERT (args_cnt == 1);
  TEST_ASSERT (jerry_value_is_boolean (args_p[0]));
  TEST_ASSERT (jerry_get_boolean_value (args_p[0]));
  return jerry_create_undefined ();
} /* assert_handler */

static void
register_function (char *name_p, /**< function name */
                   jerry_external_handler_t handler_p) /**< function handler */
{
  jerry_value_t global_value = jerry_get_global_object ();
  jerry_value_t function_value = jerry_create_external_function (handler_p);
  jerry_value_t name_value = jerry_create_string ((const jerry_char_t *) name_p);
  jerry_value_t result_value = jerry_set_property (global_value, name_value, function_value);
  TEST_ASSERT (!jerry_value_is_error (result_value));
  jerry_release_value (result_value);
  jerry_release_value (name_value);
  jerry_release_value (function_value);
  jerry_release_value (global_value);
} /* register_function */

static void
eval_and_check_error (char *script_p) /**< script source */
{
  jerry_value_t result_value;
  result_value = jerry_eval ((const jerry_char_t *) script_p, strlen (script_p), JERRY_PARSE_NO_OPTS);

  TEST_ASSERT (!jerry_value_is_error (result_value));
  jerry_release_value (result_value);
} /* eval_and_check_error */

int
main (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  register_function ("defineDA", define_da_handler);
  register_function ("checkDA", check_da_handler);
  register_function ("assert", assert_handler);

  /* Check getting property descriptors. */

  eval_and_check_error ("var counter = 0\n"
                        "var o = { }\n"
                        "defineDA(o, 'prop',\n"
                        "  function() { counter++; return 5.5 },\n"
                        "  null)\n"
                        "assert(o.prop === 5.5)\n"
                        "var d = Object.getOwnPropertyDescriptor(o, 'prop')\n"
                        "assert(d.value === 5.5)\n"
                        "assert(d.writable === false)\n"
                        "assert(!d.hasOwnProperty('get'))\n"
                        "assert(!d.hasOwnProperty('set'))\n"
                        "checkDA(o, 'prop')\n"
                        "assert(counter === 2)\n");

  eval_and_check_error ("var counter = 0\n"
                        "var o = { }\n"
                        "defineDA(o, 'prop',\n"
                        "  function() { counter++; return this.v },\n"
                        "  function(v) { counter++; this.v = v })\n"
                        "o.prop = 'X'\n"
                        "assert(o.v === 'X')\n"
                        "var d = Object.getOwnPropertyDescriptor(o, 'prop')\n"
                        "assert(d.value === 'X')\n"
                        "assert(d.writable === true)\n"
                        "assert(!d.hasOwnProperty('get'))\n"
                        "assert(!d.hasOwnProperty('set'))\n"
                        "checkDA(o, 'prop')\n"
                        "assert(counter === 2)\n");

  eval_and_check_error ("if (Object.getOwnPropertyDescriptors) {"
                        "  var counter = 0\n"
                        "  var o = { }\n"
                        "  defineDA(o, 'prop',\n"
                        "    function() { counter++; return o },\n"
                        "    function(v) { counter++ })\n"
                        "  var d = Object.getOwnPropertyDescriptors(o)\n"
                        "  assert(d.prop.value === o)\n"
                        "  assert(d.prop.writable === true)\n"
                        "  assert(!d.prop.hasOwnProperty('get'))\n"
                        "  assert(!d.prop.hasOwnProperty('set'))\n"
                        "  checkDA(o, 'prop')\n"
                        "  assert(counter === 1)\n"
                        "}");

  /* Check getting property descriptor errors. */

  eval_and_check_error ("var counter = 0\n"
                        "var o = { }\n"
                        "defineDA(o, 'prop',\n"
                        "  function() { counter++; throw 'E1' },\n"
                        "  null)\n"
                        "try {"
                        "   var d = Object.getOwnPropertyDescriptor(o, 'prop')\n"
                        "   throw 'E2'\n"
                        "} catch (e) {\n"
                        "   assert(e === 'E1')"
                        "}\n"
                        "checkDA(o, 'prop')\n"
                        "assert(counter === 1)\n");

  eval_and_check_error ("if (Object.getOwnPropertyDescriptors) {"
                        "  var counter = 0\n"
                        "  var o = { p1:4,\n"
                        "            p2:5 }\n"
                        "  defineDA(o, 'prop',\n"
                        "    function() { counter++; throw 'E1' },\n"
                        "    null)\n"
                        "  try {"
                        "     var d = Object.getOwnPropertyDescriptors(o)\n"
                        "     throw 'E2'\n"
                        "  } catch (e) {\n"
                        "     assert(e === 'E1')"
                        "  }\n"
                        "  checkDA(o, 'prop')\n"
                        "  assert(counter === 1)\n"
                        "}");

  /* Check redefining property descriptors. */

  eval_and_check_error ("var counter = 0\n"
                        "var o = { }\n"
                        "defineDA(o, 'prop',\n"
                        "  function() { counter++; return 0 },\n"
                        "  function(v) { counter++ },\n"
                        "  true)\n"
                        "Object.defineProperty(o, 'prop', { value:'V', writable:true })\n"
                        "var d = Object.getOwnPropertyDescriptor(o, 'prop')\n"
                        "assert(d.value === 'V')\n"
                        "assert(d.writable === true)\n"
                        "assert(!d.hasOwnProperty('get'))\n"
                        "assert(!d.hasOwnProperty('set'))\n"
                        "assert(counter === 0)\n");

  eval_and_check_error ("var counter = 0\n"
                        "var o = {}\n"
                        "defineDA(o, 'prop',\n"
                        "  function() { counter++; return this.v },\n"
                        "  function(v) { counter++; this.v = v })\n"
                        "Object.defineProperty(o, 'prop', { value:'V', writable:true })\n"
                        "assert(o.v === 'V')\n"
                        "var d = Object.getOwnPropertyDescriptor(o, 'prop')\n"
                        "assert(d.value === 'V')\n"
                        "assert(d.writable === true)\n"
                        "assert(!d.hasOwnProperty('get'))\n"
                        "assert(!d.hasOwnProperty('set'))\n"
                        "checkDA(o, 'prop')\n"
                        "assert(counter === 2)\n");

  eval_and_check_error ("var counter = 0\n"
                        "var o = {}\n"
                        "defineDA(o, 'prop',\n"
                        "  function() { counter++; return -7.25 },\n"
                        "  null)\n"
                        "Object.defineProperty(o, 'prop', { value:-7.25 })\n"
                        "var d = Object.getOwnPropertyDescriptor(o, 'prop')\n"
                        "assert(d.value === -7.25)\n"
                        "assert(d.writable === false)\n"
                        "assert(!d.hasOwnProperty('get'))\n"
                        "assert(!d.hasOwnProperty('set'))\n"
                        "checkDA(o, 'prop')\n"
                        "assert(counter === 2)\n");

  eval_and_check_error ("var counter = 0\n"
                        "var o = {}\n"
                        "defineDA(o, 'prop',\n"
                        "  function() { counter++; return -7.25 },\n"
                        "  null)\n"
                        "try {\n"
                        "  Object.defineProperty(o, 'prop', { value:1 })\n"
                        "} catch (e) {\n"
                        "  assert(e instanceof TypeError)\n"
                        "  counter++"
                        "}\n"
                        "assert(counter === 2)\n");

  eval_and_check_error ("if (Object.defineProperties) {\n"
                        "  var counter = 0\n"
                        "  var o = {}\n"
                        "  defineDA(o, 'prop',\n"
                        "    function() { counter++; return -7.25 },\n"
                        "    null)\n"
                        "  try {\n"
                        "    Object.defineProperties(o, { prop: { value:1 } })\n"
                        "  } catch (e) {\n"
                        "    assert(e instanceof TypeError)\n"
                        "    counter++"
                        "  }\n"
                        "  assert(counter === 2)\n"
                        "}\n");

  jerry_cleanup ();
  return 0;
} /* main */

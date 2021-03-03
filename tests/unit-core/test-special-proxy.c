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
create_special_proxy_handler (const jerry_call_info_t *call_info_p, /**< call information */
                              const jerry_value_t args_p[], /**< argument list */
                              const jerry_length_t args_count) /**< argument count */
{
  JERRY_UNUSED (call_info_p);

  if (args_count < 2)
  {
    return jerry_create_undefined ();
  }

  return jerry_create_special_proxy (args_p[0], args_p[1], JERRY_PROXY_SKIP_RESULT_VALIDATION);
} /* create_special_proxy_handler */

static void
run_eval (const char *source_p)
{
  jerry_value_t result = jerry_eval ((const jerry_char_t *) source_p, strlen (source_p), 0);

  TEST_ASSERT (!jerry_value_is_error (result));
  jerry_release_value (result);
} /* run_eval */

/**
 * Unit test's main function.
 */
int
main (void)
{
  TEST_INIT ();

  if (!jerry_is_feature_enabled (JERRY_FEATURE_PROXY))
  {
    printf ("Skipping test, Proxy not enabled\n");
    return 0;
  }

  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t global = jerry_get_global_object ();

  jerry_value_t function = jerry_create_external_function (create_special_proxy_handler);
  jerry_value_t name = jerry_create_string ((const jerry_char_t *) "create_special_proxy");
  jerry_value_t result = jerry_set_property (global, name, function);
  TEST_ASSERT (!jerry_value_is_error (result));

  jerry_release_value (result);
  jerry_release_value (name);
  jerry_release_value (function);

  jerry_release_value (global);

  run_eval ("function assert (v) {\n"
            "  if (v !== true)\n"
            "     throw 'Assertion failed!'\n"
            "}");

  /* These tests fail unless JERRY_PROXY_SKIP_RESULT_VALIDATION is set. */

  run_eval ("var o = {}\n"
            "Object.preventExtensions(o)\n"
            "var proxy = create_special_proxy(o, {\n"
            "  getPrototypeOf(target) { return Array.prototype }\n"
            "})\n"
            "assert(Object.getPrototypeOf(proxy) === Array.prototype)");

  run_eval ("var o = {}\n"
            "Object.preventExtensions(o)\n"
            "var proxy = create_special_proxy(o, {\n"
            "  setPrototypeOf(target, proto) { return true }\n"
            "})\n"
            "Object.setPrototypeOf(proxy, Array.prototype)");

  run_eval ("var o = {}\n"
            "var proxy = create_special_proxy(o, {\n"
            "  isExtensible(target) { return false }\n"
            "})\n"
            "assert(Object.isExtensible(proxy) === false)");

  run_eval ("var o = {}\n"
            "var proxy = create_special_proxy(o, {\n"
            "  preventExtensions(target) { return true }\n"
            "})\n"
            "Object.preventExtensions(proxy)");

  run_eval ("var o = {}\n"
            "Object.defineProperty(o, 'prop', { value:4, enumerable:true })\n"
            "var proxy = create_special_proxy(o, {\n"
            "  getOwnPropertyDescriptor(target, key) {\n"
            "    return { value:5, configurable:true, writable:true }\n"
            "  }\n"
            "})\n"
            "var desc = Object.getOwnPropertyDescriptor(proxy, 'prop')\n"
            "assert(desc.value === 5)\n"
            "assert(desc.configurable === true)\n"
            "assert(desc.enumerable === false)\n"
            "assert(desc.writable === true)\n");

  run_eval ("var o = {}\n"
            "Object.defineProperty(o, 'prop', { get() {} })\n"
            "var proxy = create_special_proxy(o, {\n"
            "  defineProperty(target, key, descriptor) { return true }\n"
            "})\n"
            "Object.defineProperty(proxy, 'prop', { value:5 })");

  run_eval ("var o = {}\n"
            "Object.defineProperty(o, 'prop', { value:4 })\n"
            "var proxy = create_special_proxy(o, {\n"
            "  has(target, key) { return false }\n"
            "})\n"
            "assert(!Reflect.has(proxy, 'prop'))");

  run_eval ("var o = {}\n"
            "Object.defineProperty(o, 'prop', { value:4 })\n"
            "var proxy = create_special_proxy(o, {\n"
            "  get(target, key) { return 5 }\n"
            "})\n"
            "assert(proxy.prop === 5)");

  run_eval ("var o = {}\n"
            "Object.defineProperty(o, 'prop', { value:4 })\n"
            "var proxy = create_special_proxy(o, {\n"
            "  set(target, key, value) { return true }\n"
            "})\n"
            "proxy.prop = 8");

  run_eval ("var o = {}\n"
            "Object.defineProperty(o, 'prop', { value:4 })\n"
            "var proxy = create_special_proxy(o, {\n"
            "  deleteProperty(target, key) { return true }\n"
            "})\n"
            "assert(delete proxy.prop)");

  run_eval ("var o = {}\n"
            "Object.defineProperty(o, 'prop', { value:4 })\n"
            "var proxy = create_special_proxy(o, {\n"
            "  ownKeys(target) { return [] }\n"
            "})\n"
            "Object.keys(proxy)");

  jerry_cleanup ();
  return 0;
} /* main */

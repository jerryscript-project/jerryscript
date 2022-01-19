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

typedef struct
{
  jerry_function_type_t type_info;
  jerry_value_t value;
  bool active;
  bool is_async;
} test_entry_t;

#define ENTRY(TYPE, VALUE)   \
  {                          \
    TYPE, VALUE, true, false \
  }
#define ENTRY_IF(TYPE, VALUE, FEATURE, ASYNC)           \
  {                                                     \
    TYPE, VALUE, jerry_feature_enabled (FEATURE), ASYNC \
  }
#define EVALUATE(BUFF) (jerry_eval ((BUFF), sizeof ((BUFF)) - 1, JERRY_PARSE_NO_OPTS))
static jerry_value_t
test_ext_function (const jerry_call_info_t *call_info_p, /**< call information */
                   const jerry_value_t args_p[], /**< array of arguments */
                   const jerry_length_t args_cnt) /**< number of arguments */
{
  (void) call_info_p;
  (void) args_p;
  (void) args_cnt;
  return jerry_boolean (true);
} /* test_ext_function */

int
main (void)
{
  TEST_INIT ();

  jerry_init (JERRY_INIT_EMPTY);

  const jerry_char_t arrow_function[] = "_ => 5";
  const jerry_char_t async_arrow_function[] = "async _ => 5";
  const jerry_char_t generator_function[] = "function *f() {}; f";
  const jerry_char_t async_generator_function[] = "async function *f() {}; f";
  const jerry_char_t getter_function[] = "Object.getOwnPropertyDescriptor({get a(){}}, 'a').get";
  const jerry_char_t setter_function[] = "Object.getOwnPropertyDescriptor({set a(b){}}, 'a').set";
  const jerry_char_t method_function[] = "Object.getOwnPropertyDescriptor({a(){}}, 'a').value";

  const jerry_char_t builtin_function[] = "Object";
  const jerry_char_t simple_function[] = "function f() {}; f";
  const jerry_char_t bound_function[] = "function f() {}; f.bind(1,2)";

  test_entry_t entries[] = {
    ENTRY (JERRY_FUNCTION_TYPE_NONE, jerry_number (-33.0)),
    ENTRY (JERRY_FUNCTION_TYPE_NONE, jerry_boolean (true)),
    ENTRY (JERRY_FUNCTION_TYPE_NONE, jerry_undefined ()),
    ENTRY (JERRY_FUNCTION_TYPE_NONE, jerry_null ()),
    ENTRY (JERRY_FUNCTION_TYPE_NONE, jerry_string_sz ("foo")),
    ENTRY (JERRY_FUNCTION_TYPE_NONE, jerry_throw_sz (JERRY_ERROR_TYPE, "error")),

    ENTRY (JERRY_FUNCTION_TYPE_NONE, jerry_object ()),
    ENTRY (JERRY_FUNCTION_TYPE_NONE, jerry_array (10)),

    ENTRY_IF (JERRY_FUNCTION_TYPE_ARROW, EVALUATE (arrow_function), JERRY_FEATURE_SYMBOL, false),
    ENTRY_IF (JERRY_FUNCTION_TYPE_ARROW, EVALUATE (async_arrow_function), JERRY_FEATURE_SYMBOL, true),
    ENTRY_IF (JERRY_FUNCTION_TYPE_GENERATOR, EVALUATE (generator_function), JERRY_FEATURE_SYMBOL, false),
    ENTRY_IF (JERRY_FUNCTION_TYPE_GENERATOR, EVALUATE (async_generator_function), JERRY_FEATURE_SYMBOL, true),
    ENTRY_IF (JERRY_FUNCTION_TYPE_GENERIC, EVALUATE (method_function), JERRY_FEATURE_SYMBOL, false),
    ENTRY (JERRY_FUNCTION_TYPE_GENERIC, EVALUATE (builtin_function)),
    ENTRY (JERRY_FUNCTION_TYPE_GENERIC, EVALUATE (simple_function)),
    ENTRY (JERRY_FUNCTION_TYPE_BOUND, EVALUATE (bound_function)),
    ENTRY (JERRY_FUNCTION_TYPE_GENERIC, jerry_function_external (test_ext_function)),
    ENTRY (JERRY_FUNCTION_TYPE_ACCESSOR, EVALUATE (getter_function)),
    ENTRY (JERRY_FUNCTION_TYPE_ACCESSOR, EVALUATE (setter_function)),
  };

  for (size_t idx = 0; idx < sizeof (entries) / sizeof (entries[0]); idx++)
  {
    jerry_function_type_t type_info = jerry_function_type (entries[idx].value);
    TEST_ASSERT (!entries[idx].active
                 || (type_info == entries[idx].type_info
                     && jerry_value_is_async_function (entries[idx].value) == entries[idx].is_async));
    jerry_value_free (entries[idx].value);
  }

  jerry_cleanup ();

  return 0;
} /* main */

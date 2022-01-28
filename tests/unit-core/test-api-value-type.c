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
  jerry_type_t type_info;
  jerry_value_t value;
} test_entry_t;

#define ENTRY(TYPE, VALUE) \
  {                        \
    TYPE, VALUE            \
  }

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

  const char test_eval_function[] = "function demo(a) { return a + 1; }; demo";

  test_entry_t entries[] = {
    ENTRY (JERRY_TYPE_NUMBER, jerry_number (-33.0)),
    ENTRY (JERRY_TYPE_NUMBER, jerry_number (3)),
    ENTRY (JERRY_TYPE_NUMBER, jerry_nan ()),
    ENTRY (JERRY_TYPE_NUMBER, jerry_infinity (false)),
    ENTRY (JERRY_TYPE_NUMBER, jerry_infinity (true)),

    ENTRY (JERRY_TYPE_BOOLEAN, jerry_boolean (true)),
    ENTRY (JERRY_TYPE_BOOLEAN, jerry_boolean (false)),

    ENTRY (JERRY_TYPE_UNDEFINED, jerry_undefined ()),

    ENTRY (JERRY_TYPE_OBJECT, jerry_object ()),
    ENTRY (JERRY_TYPE_OBJECT, jerry_array (10)),
    ENTRY (JERRY_TYPE_EXCEPTION, jerry_throw_sz (JERRY_ERROR_TYPE, "error")),

    ENTRY (JERRY_TYPE_NULL, jerry_null ()),

    ENTRY (JERRY_TYPE_FUNCTION,
           jerry_eval ((jerry_char_t *) test_eval_function, sizeof (test_eval_function) - 1, JERRY_PARSE_NO_OPTS)),
    ENTRY (JERRY_TYPE_FUNCTION, jerry_function_external (test_ext_function)),

    ENTRY (JERRY_TYPE_STRING, jerry_string_sz (test_eval_function)),
    ENTRY (JERRY_TYPE_STRING, jerry_string_sz ("")),
  };

  for (size_t idx = 0; idx < sizeof (entries) / sizeof (entries[0]); idx++)
  {
    jerry_type_t type_info = jerry_value_type (entries[idx].value);

    TEST_ASSERT (type_info != JERRY_TYPE_NONE);
    TEST_ASSERT (type_info == entries[idx].type_info);

    jerry_value_free (entries[idx].value);
  }

  jerry_value_t symbol_desc_value = jerry_string_sz ("foo");
  jerry_value_t symbol_value = jerry_symbol_with_description (symbol_desc_value);
  jerry_type_t type_info = jerry_value_type (symbol_value);

  TEST_ASSERT (type_info != JERRY_TYPE_NONE);
  TEST_ASSERT (type_info == JERRY_TYPE_SYMBOL);

  jerry_value_free (symbol_value);
  jerry_value_free (symbol_desc_value);

  if (jerry_feature_enabled (JERRY_FEATURE_BIGINT))
  {
    /* Check simple bigint value type */
    uint64_t digits_buffer[2] = { 1, 0 };
    jerry_value_t value_bigint = jerry_bigint (digits_buffer, 2, false);
    jerry_type_t value_type_info = jerry_value_type (value_bigint);

    TEST_ASSERT (value_type_info != JERRY_TYPE_NONE);
    TEST_ASSERT (value_type_info == JERRY_TYPE_BIGINT);

    jerry_value_free (value_bigint);

    /* Check bigint wrapped in object type */
    jerry_char_t object_bigint_src[] = "Object(5n)";
    jerry_value_t object_bigint = jerry_eval (object_bigint_src, sizeof (object_bigint_src) - 1, JERRY_PARSE_NO_OPTS);
    TEST_ASSERT (!jerry_value_is_exception (object_bigint));

    jerry_type_t object_type_info = jerry_value_type (object_bigint);

    TEST_ASSERT (object_type_info != JERRY_TYPE_NONE);
    TEST_ASSERT (object_type_info == JERRY_TYPE_OBJECT);

    jerry_value_free (object_bigint);
  }

  if (jerry_feature_enabled (JERRY_FEATURE_REALM))
  {
    jerry_value_t new_realm = jerry_realm ();
    jerry_value_t old_realm = jerry_set_realm (new_realm);

    jerry_type_t new_realm_type = jerry_value_type (new_realm);
    TEST_ASSERT (new_realm_type == JERRY_TYPE_OBJECT);

    jerry_value_t new_realm_this = jerry_realm_this (new_realm);
    jerry_type_t new_realm_this_type = jerry_value_type (new_realm_this);
    TEST_ASSERT (new_realm_this_type == JERRY_TYPE_OBJECT);
    jerry_value_free (new_realm_this);

    jerry_type_t old_realm_type = jerry_value_type (old_realm);
    TEST_ASSERT (old_realm_type == JERRY_TYPE_OBJECT);

    jerry_value_free (new_realm);

    jerry_value_t old_realm_this = jerry_realm_this (old_realm);
    jerry_type_t old_realm_this_type = jerry_value_type (old_realm_this);
    TEST_ASSERT (old_realm_this_type == JERRY_TYPE_OBJECT);
    jerry_value_free (old_realm_this);

    /* Restore the old realm as per docs */
    jerry_set_realm (old_realm);
  }

  jerry_cleanup ();

  return 0;
} /* main */

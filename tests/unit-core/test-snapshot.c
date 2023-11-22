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

#include "config.h"
#include "test-common.h"

/**
 * Maximum size of snapshots buffer
 */
#define SNAPSHOT_BUFFER_SIZE (256)

/**
 * Maximum size of literal buffer
 */
#define LITERAL_BUFFER_SIZE (256)

/**
 * Magic strings
 */
static const jerry_char_t *magic_strings[] = { (const jerry_char_t *) " ",      (const jerry_char_t *) "a",
                                               (const jerry_char_t *) "b",      (const jerry_char_t *) "c",
                                               (const jerry_char_t *) "from",   (const jerry_char_t *) "func",
                                               (const jerry_char_t *) "string", (const jerry_char_t *) "snapshot" };

/**
 * Magic string lengths
 */
static const jerry_length_t magic_string_lengths[] = { 1, 1, 1, 1, 4, 4, 6, 8 };

static void
test_exec_snapshot (uint32_t *snapshot_p, size_t snapshot_size, uint32_t exec_snapshot_flags)
{
  char string_data[32];

  jerry_init (JERRY_INIT_EMPTY);

  jerry_register_magic_strings (magic_strings,
                                sizeof (magic_string_lengths) / sizeof (jerry_length_t),
                                magic_string_lengths);

  jerry_value_t res = jerry_exec_snapshot (snapshot_p, snapshot_size, 0, exec_snapshot_flags, NULL);

  TEST_ASSERT (!jerry_value_is_exception (res));
  TEST_ASSERT (jerry_value_is_string (res));
  jerry_size_t sz = jerry_string_size (res, JERRY_ENCODING_CESU8);
  TEST_ASSERT (sz == 20);
  sz = jerry_string_to_buffer (res, JERRY_ENCODING_CESU8, (jerry_char_t *) string_data, sz);
  TEST_ASSERT (sz == 20);
  jerry_value_free (res);
  TEST_ASSERT (!strncmp (string_data, "string from snapshot", (size_t) sz));

  jerry_cleanup ();
} /* test_exec_snapshot */

static void
test_static_snapshot (void)
{
  if (jerry_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE) && jerry_feature_enabled (JERRY_FEATURE_SNAPSHOT_EXEC))
  {
    static uint32_t snapshot_buffer[SNAPSHOT_BUFFER_SIZE];
    const jerry_char_t code_to_snapshot[] = TEST_STRING_LITERAL ("function func(a, b, c) {"
                                                                 "  c = 'snapshot';"
                                                                 "  return arguments[0] + ' ' + b + ' ' + arguments[2];"
                                                                 "};"
                                                                 "func('string', 'from');");

    jerry_init (JERRY_INIT_EMPTY);
    jerry_register_magic_strings (magic_strings,
                                  sizeof (magic_string_lengths) / sizeof (jerry_length_t),
                                  magic_string_lengths);

    jerry_value_t parse_result = jerry_parse (code_to_snapshot, sizeof (code_to_snapshot) - 1, NULL);
    TEST_ASSERT (!jerry_value_is_exception (parse_result));

    jerry_value_t generate_result =
      jerry_generate_snapshot (parse_result, JERRY_SNAPSHOT_SAVE_STATIC, snapshot_buffer, SNAPSHOT_BUFFER_SIZE);
    jerry_value_free (parse_result);

    TEST_ASSERT (!jerry_value_is_exception (generate_result) && jerry_value_is_number (generate_result));

    size_t snapshot_size = (size_t) jerry_value_as_number (generate_result);
    jerry_value_free (generate_result);

    /* Static snapshots are not supported by default. */
    jerry_value_t exec_result = jerry_exec_snapshot (snapshot_buffer, snapshot_size, 0, 0, NULL);
    TEST_ASSERT (jerry_value_is_exception (exec_result));
    jerry_value_free (exec_result);

    jerry_cleanup ();

    test_exec_snapshot (snapshot_buffer, snapshot_size, JERRY_SNAPSHOT_EXEC_ALLOW_STATIC);
  }
} /* test_static_snapshot */

static void
test_merge_snapshot (void)
{
  if (jerry_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE) && jerry_feature_enabled (JERRY_FEATURE_SNAPSHOT_EXEC))
  {
    static uint32_t snapshot_buffer_0[SNAPSHOT_BUFFER_SIZE];
    static uint32_t snapshot_buffer_1[SNAPSHOT_BUFFER_SIZE];
    size_t snapshot_sizes[2];
    static uint32_t merged_snapshot_buffer[SNAPSHOT_BUFFER_SIZE];

    const jerry_char_t code_to_snapshot1[] = "var a = 'hello'; 123";

    jerry_init (JERRY_INIT_EMPTY);

    jerry_value_t parse_result = jerry_parse (code_to_snapshot1, sizeof (code_to_snapshot1) - 1, NULL);
    TEST_ASSERT (!jerry_value_is_exception (parse_result));

    jerry_value_t generate_result = jerry_generate_snapshot (parse_result, 0, snapshot_buffer_0, SNAPSHOT_BUFFER_SIZE);
    jerry_value_free (parse_result);

    TEST_ASSERT (!jerry_value_is_exception (generate_result) && jerry_value_is_number (generate_result));

    snapshot_sizes[0] = (size_t) jerry_value_as_number (generate_result);
    jerry_value_free (generate_result);

    jerry_cleanup ();

    const jerry_char_t code_to_snapshot2[] = "var b = 'hello'; 456";

    jerry_init (JERRY_INIT_EMPTY);

    parse_result = jerry_parse (code_to_snapshot2, sizeof (code_to_snapshot2) - 1, NULL);
    TEST_ASSERT (!jerry_value_is_exception (parse_result));

    generate_result = jerry_generate_snapshot (parse_result, 0, snapshot_buffer_1, SNAPSHOT_BUFFER_SIZE);
    jerry_value_free (parse_result);

    TEST_ASSERT (!jerry_value_is_exception (generate_result) && jerry_value_is_number (generate_result));

    snapshot_sizes[1] = (size_t) jerry_value_as_number (generate_result);
    jerry_value_free (generate_result);

    jerry_cleanup ();

    jerry_init (JERRY_INIT_EMPTY);

    const char *error_p;
    const uint32_t *snapshot_buffers[2];

    snapshot_buffers[0] = snapshot_buffer_0;
    snapshot_buffers[1] = snapshot_buffer_1;

    static uint32_t snapshot_buffer_0_bck[SNAPSHOT_BUFFER_SIZE];
    static uint32_t snapshot_buffer_1_bck[SNAPSHOT_BUFFER_SIZE];

    memcpy (snapshot_buffer_0_bck, snapshot_buffer_0, SNAPSHOT_BUFFER_SIZE);
    memcpy (snapshot_buffer_1_bck, snapshot_buffer_1, SNAPSHOT_BUFFER_SIZE);

    size_t merged_size = jerry_merge_snapshots (snapshot_buffers,
                                                snapshot_sizes,
                                                2,
                                                merged_snapshot_buffer,
                                                SNAPSHOT_BUFFER_SIZE,
                                                &error_p);

    jerry_cleanup ();

    TEST_ASSERT (0 == memcmp (snapshot_buffer_0_bck, snapshot_buffer_0, SNAPSHOT_BUFFER_SIZE));
    TEST_ASSERT (0 == memcmp (snapshot_buffer_1_bck, snapshot_buffer_1, SNAPSHOT_BUFFER_SIZE));

    jerry_init (JERRY_INIT_EMPTY);

    jerry_value_t res = jerry_exec_snapshot (merged_snapshot_buffer, merged_size, 0, 0, NULL);
    TEST_ASSERT (!jerry_value_is_exception (res));
    TEST_ASSERT (jerry_value_as_number (res) == 123);
    jerry_value_free (res);

    res = jerry_exec_snapshot (merged_snapshot_buffer, merged_size, 1, 0, NULL);
    TEST_ASSERT (!jerry_value_is_exception (res));
    TEST_ASSERT (jerry_value_as_number (res) == 456);
    jerry_value_free (res);

    jerry_cleanup ();
  }
} /* test_merge_snapshot */

static void
test_save_literals (void)
{
  if (jerry_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE))
  {
    /* C format generation */
    jerry_init (JERRY_INIT_EMPTY);

    static jerry_char_t literal_buffer_c[LITERAL_BUFFER_SIZE];
    static uint32_t literal_snapshot_buffer[SNAPSHOT_BUFFER_SIZE];
    static const jerry_char_t code_for_c_format[] = "var object = { aa:'fo\" o\\n \\\\', Bb:'max', aaa:'xzy0' };";

    jerry_value_t parse_result = jerry_parse (code_for_c_format, sizeof (code_for_c_format) - 1, NULL);
    TEST_ASSERT (!jerry_value_is_exception (parse_result));

    jerry_value_t generate_result =
      jerry_generate_snapshot (parse_result, 0, literal_snapshot_buffer, SNAPSHOT_BUFFER_SIZE);
    jerry_value_free (parse_result);

    TEST_ASSERT (!jerry_value_is_exception (generate_result));
    TEST_ASSERT (jerry_value_is_number (generate_result));

    size_t snapshot_size = (size_t) jerry_value_as_number (generate_result);
    jerry_value_free (generate_result);

    const size_t lit_c_buf_sz = jerry_get_literals_from_snapshot (literal_snapshot_buffer,
                                                                  snapshot_size,
                                                                  literal_buffer_c,
                                                                  LITERAL_BUFFER_SIZE,
                                                                  true);
    TEST_ASSERT (lit_c_buf_sz == 239);

    static const char *expected_c_format = ("jerry_length_t literal_count = 5;\n\n"
                                            "jerry_char_t *literals[5] =\n"
                                            "{\n"
                                            "  \"Bb\",\n"
                                            "  \"aa\",\n"
                                            "  \"aaa\",\n"
                                            "  \"xzy0\",\n"
                                            "  \"fo\\\" o\\x0A \\\\\"\n"
                                            "};\n\n"
                                            "jerry_length_t literal_sizes[5] =\n"
                                            "{\n"
                                            "  2 /* Bb */,\n"
                                            "  2 /* aa */,\n"
                                            "  3 /* aaa */,\n"
                                            "  4 /* xzy0 */,\n"
                                            "  8 /* fo\" o\n \\ */\n"
                                            "};\n");

    TEST_ASSERT (!strncmp ((char *) literal_buffer_c, expected_c_format, lit_c_buf_sz));

    /* List format generation */
    static jerry_char_t literal_buffer_list[LITERAL_BUFFER_SIZE];
    const size_t lit_list_buf_sz = jerry_get_literals_from_snapshot (literal_snapshot_buffer,
                                                                     snapshot_size,
                                                                     literal_buffer_list,
                                                                     LITERAL_BUFFER_SIZE,
                                                                     false);
    TEST_ASSERT (lit_list_buf_sz == 34);
    TEST_ASSERT (
      !strncmp ((char *) literal_buffer_list, "2 Bb\n2 aa\n3 aaa\n4 xzy0\n8 fo\" o\n \\\n", lit_list_buf_sz));

    jerry_cleanup ();
  }
} /* test_save_literals */

static void
test_function_snapshot (void)
{
  if (!jerry_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE) || !jerry_feature_enabled (JERRY_FEATURE_SNAPSHOT_EXEC))
  {
    return;
  }

  const jerry_init_flag_t flags = JERRY_INIT_EMPTY;
  static uint32_t function_snapshot_buffer[SNAPSHOT_BUFFER_SIZE];

  const jerry_char_t code_to_snapshot[] = "return a + b";

  jerry_init (flags);

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_HAS_ARGUMENT_LIST;
  parse_options.argument_list = jerry_string_sz ("a, b");

  jerry_value_t parse_result = jerry_parse (code_to_snapshot, sizeof (code_to_snapshot) - 1, &parse_options);
  TEST_ASSERT (!jerry_value_is_exception (parse_result));

  jerry_value_t generate_result =
    jerry_generate_snapshot (parse_result, 0, function_snapshot_buffer, SNAPSHOT_BUFFER_SIZE);
  jerry_value_free (parse_result);
  jerry_value_free (parse_options.argument_list);

  TEST_ASSERT (!jerry_value_is_exception (generate_result) && jerry_value_is_number (generate_result));

  size_t function_snapshot_size = (size_t) jerry_value_as_number (generate_result);
  jerry_value_free (generate_result);

  jerry_cleanup ();

  jerry_init (flags);

  jerry_value_t function_obj = jerry_exec_snapshot (function_snapshot_buffer,
                                                    function_snapshot_size,
                                                    0,
                                                    JERRY_SNAPSHOT_EXEC_LOAD_AS_FUNCTION,
                                                    NULL);

  TEST_ASSERT (!jerry_value_is_exception (function_obj));
  TEST_ASSERT (jerry_value_is_function (function_obj));

  jerry_value_t this_val = jerry_undefined ();
  jerry_value_t args[2];
  args[0] = jerry_number (1.0);
  args[1] = jerry_number (2.0);

  jerry_value_t res = jerry_call (function_obj, this_val, args, 2);

  TEST_ASSERT (!jerry_value_is_exception (res));
  TEST_ASSERT (jerry_value_is_number (res));
  double num = jerry_value_as_number (res);
  TEST_ASSERT (num == 3);

  jerry_value_free (args[0]);
  jerry_value_free (args[1]);
  jerry_value_free (res);
  jerry_value_free (function_obj);

  jerry_cleanup ();
} /* test_function_snapshot */

static void
arguments_test_exec_snapshot (uint32_t *snapshot_p, size_t snapshot_size, uint32_t exec_snapshot_flags)
{
  jerry_init (JERRY_INIT_EMPTY);
  jerry_value_t res = jerry_exec_snapshot (snapshot_p, snapshot_size, 0, exec_snapshot_flags, NULL);
  TEST_ASSERT (!jerry_value_is_exception (res));
  TEST_ASSERT (jerry_value_is_number (res));
  double raw_value = jerry_value_as_number (res);
  TEST_ASSERT (raw_value == 15);
  jerry_value_free (res);

  jerry_cleanup ();
} /* arguments_test_exec_snapshot */

static void
test_function_arguments_snapshot (void)
{
  if (jerry_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE) && jerry_feature_enabled (JERRY_FEATURE_SNAPSHOT_EXEC))
  {
    static uint32_t arguments_snapshot_buffer[SNAPSHOT_BUFFER_SIZE];

    const jerry_char_t code_to_snapshot[] = TEST_STRING_LITERAL ("function f(a,b,c) {"
                                                                 "  arguments[0]++;"
                                                                 "  arguments[1]++;"
                                                                 "  arguments[2]++;"
                                                                 "  return a + b + c;"
                                                                 "}"
                                                                 "f(3,4,5);");
    jerry_init (JERRY_INIT_EMPTY);

    jerry_value_t parse_result = jerry_parse (code_to_snapshot, sizeof (code_to_snapshot) - 1, NULL);
    TEST_ASSERT (!jerry_value_is_exception (parse_result));

    jerry_value_t generate_result =
      jerry_generate_snapshot (parse_result, 0, arguments_snapshot_buffer, SNAPSHOT_BUFFER_SIZE);
    jerry_value_free (parse_result);

    TEST_ASSERT (!jerry_value_is_exception (generate_result) && jerry_value_is_number (generate_result));

    size_t snapshot_size = (size_t) jerry_value_as_number (generate_result);
    jerry_value_free (generate_result);

    jerry_cleanup ();

    arguments_test_exec_snapshot (arguments_snapshot_buffer, snapshot_size, 0);
    arguments_test_exec_snapshot (arguments_snapshot_buffer, snapshot_size, JERRY_SNAPSHOT_EXEC_COPY_DATA);
  }
} /* test_function_arguments_snapshot */

static void
test_snapshot_with_user (void)
{
  if (jerry_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE) && jerry_feature_enabled (JERRY_FEATURE_SNAPSHOT_EXEC))
  {
    static uint32_t snapshot_buffer[SNAPSHOT_BUFFER_SIZE];

    const jerry_char_t code_to_snapshot[] = TEST_STRING_LITERAL ("function f() {}\n"
                                                                 "f");
    jerry_init (JERRY_INIT_EMPTY);

    jerry_value_t parse_result = jerry_parse (code_to_snapshot, sizeof (code_to_snapshot) - 1, NULL);
    TEST_ASSERT (!jerry_value_is_exception (parse_result));

    jerry_value_t generate_result = jerry_generate_snapshot (parse_result, 0, snapshot_buffer, SNAPSHOT_BUFFER_SIZE);
    jerry_value_free (parse_result);

    TEST_ASSERT (!jerry_value_is_exception (generate_result) && jerry_value_is_number (generate_result));

    size_t snapshot_size = (size_t) jerry_value_as_number (generate_result);
    jerry_value_free (generate_result);

    for (int i = 0; i < 3; i++)
    {
      jerry_exec_snapshot_option_values_t snapshot_exec_options;

      if (i == 0)
      {
        snapshot_exec_options.user_value = jerry_object ();
      }
      else if (i == 1)
      {
        snapshot_exec_options.user_value = jerry_number (-3.5);
      }
      else
      {
        snapshot_exec_options.user_value = jerry_string_sz ("AnyString...");
      }

      jerry_value_t result = jerry_exec_snapshot (snapshot_buffer,
                                                  snapshot_size,
                                                  0,
                                                  JERRY_SNAPSHOT_EXEC_HAS_USER_VALUE,
                                                  &snapshot_exec_options);

      TEST_ASSERT (!jerry_value_is_exception (result) && jerry_value_is_function (result));

      jerry_value_t user_value = jerry_source_user_value (result);
      jerry_value_free (result);

      result = jerry_binary_op (JERRY_BIN_OP_STRICT_EQUAL, user_value, snapshot_exec_options.user_value);

      TEST_ASSERT (jerry_value_is_true (result));

      jerry_value_free (result);
      jerry_value_free (user_value);
      jerry_value_free (snapshot_exec_options.user_value);
    }

    jerry_cleanup ();
  }
} /* test_snapshot_with_user */

int
main (void)
{
  TEST_INIT ();

  test_static_snapshot ();

  test_merge_snapshot ();

  test_save_literals ();

  test_function_snapshot ();

  test_function_arguments_snapshot ();

  test_snapshot_with_user ();

  return 0;
} /* main */

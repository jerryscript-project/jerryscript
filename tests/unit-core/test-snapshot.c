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

#include "config.h"
#include "jerryscript.h"

#include "test-common.h"

/**
 * Maximum size of snapshots buffer
 */
#define SNAPSHOT_BUFFER_SIZE (256)

static void test_function_snapshot (void)
{
  /* function to snapshot */
  if (!jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE)
      || !jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_EXEC))
  {
    return;
  }

  const jerry_init_flag_t flags = JERRY_INIT_EMPTY;
  static uint32_t function_snapshot_buffer[SNAPSHOT_BUFFER_SIZE];

  const char *args_p = "a, b";
  const char *code_to_snapshot_p = "return a + b";

  jerry_init (flags);
  size_t function_snapshot_size = jerry_parse_and_save_function_snapshot ((jerry_char_t *) code_to_snapshot_p,
                                                                          strlen (code_to_snapshot_p),
                                                                          (jerry_char_t *) args_p,
                                                                          strlen (args_p),
                                                                          false,
                                                                          function_snapshot_buffer,
                                                                          SNAPSHOT_BUFFER_SIZE);
  TEST_ASSERT (function_snapshot_size != 0);
  jerry_cleanup ();

  jerry_init (flags);

  jerry_value_t function_obj = jerry_load_function_snapshot_at (function_snapshot_buffer,
                                                                function_snapshot_size,
                                                                0,
                                                                false);

  TEST_ASSERT (!jerry_value_has_error_flag (function_obj));
  TEST_ASSERT (jerry_value_is_function (function_obj));

  jerry_value_t this_val = jerry_create_undefined ();
  jerry_value_t args[2];
  args[0] = jerry_create_number (1.0);
  args[1] = jerry_create_number (2.0);

  jerry_value_t res = jerry_call_function (function_obj, this_val, args, 2);

  TEST_ASSERT (!jerry_value_has_error_flag (res));
  TEST_ASSERT (jerry_value_is_number (res));
  double num = jerry_get_number_value (res);
  TEST_ASSERT (num == 3);

  jerry_release_value (args[0]);
  jerry_release_value (args[1]);
  jerry_release_value (res);
  jerry_release_value (function_obj);

  jerry_cleanup ();
} /* test_function_snapshot */

int
main (void)
{
  TEST_INIT ();

  /* Dump / execute snapshot */
  if (jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE)
      && jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_EXEC))
  {
    static uint32_t global_mode_snapshot_buffer[SNAPSHOT_BUFFER_SIZE];
    static uint32_t eval_mode_snapshot_buffer[SNAPSHOT_BUFFER_SIZE];
    char string_data[32];

    const char *code_to_snapshot_p = "(function () { return 'string from snapshot'; }) ();";

    jerry_init (JERRY_INIT_EMPTY);
    size_t global_mode_snapshot_size = jerry_parse_and_save_snapshot ((jerry_char_t *) code_to_snapshot_p,
                                                                      strlen (code_to_snapshot_p),
                                                                      true,
                                                                      false,
                                                                      global_mode_snapshot_buffer,
                                                                      SNAPSHOT_BUFFER_SIZE);
    TEST_ASSERT (global_mode_snapshot_size != 0);
    jerry_cleanup ();

    jerry_init (JERRY_INIT_EMPTY);
    size_t eval_mode_snapshot_size = jerry_parse_and_save_snapshot ((jerry_char_t *) code_to_snapshot_p,
                                                                    strlen (code_to_snapshot_p),
                                                                    false,
                                                                    false,
                                                                    eval_mode_snapshot_buffer,
                                                                    SNAPSHOT_BUFFER_SIZE);
    TEST_ASSERT (eval_mode_snapshot_size != 0);
    jerry_cleanup ();

    jerry_init (JERRY_INIT_EMPTY);

    jerry_value_t res = jerry_exec_snapshot (global_mode_snapshot_buffer,
                                             global_mode_snapshot_size,
                                             false);

    TEST_ASSERT (!jerry_value_has_error_flag (res));
    TEST_ASSERT (jerry_value_is_string (res));
    jerry_size_t sz = jerry_get_string_size (res);
    TEST_ASSERT (sz == 20);
    sz = jerry_string_to_char_buffer (res, (jerry_char_t *) string_data, sz);
    TEST_ASSERT (sz == 20);
    jerry_release_value (res);
    TEST_ASSERT (!strncmp (string_data, "string from snapshot", (size_t) sz));

    res = jerry_exec_snapshot (eval_mode_snapshot_buffer,
                               eval_mode_snapshot_size,
                               false);

    TEST_ASSERT (!jerry_value_has_error_flag (res));
    TEST_ASSERT (jerry_value_is_string (res));
    sz = jerry_get_string_size (res);
    TEST_ASSERT (sz == 20);
    sz = jerry_string_to_char_buffer (res, (jerry_char_t *) string_data, sz);
    TEST_ASSERT (sz == 20);
    jerry_release_value (res);
    TEST_ASSERT (!strncmp (string_data, "string from snapshot", (size_t) sz));

    jerry_cleanup ();
  }

  /* Merge snapshot */
  if (jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE)
      && jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_EXEC))
  {
    static uint32_t snapshot_buffer_0[SNAPSHOT_BUFFER_SIZE];
    static uint32_t snapshot_buffer_1[SNAPSHOT_BUFFER_SIZE];
    size_t snapshot_sizes[2];
    static uint32_t merged_snapshot_buffer[SNAPSHOT_BUFFER_SIZE];

    const char *code_to_snapshot_p = "123";

    jerry_init (JERRY_INIT_EMPTY);
    snapshot_sizes[0] = jerry_parse_and_save_snapshot ((jerry_char_t *) code_to_snapshot_p,
                                                       strlen (code_to_snapshot_p),
                                                       true,
                                                       false,
                                                       snapshot_buffer_0,
                                                       SNAPSHOT_BUFFER_SIZE);
    TEST_ASSERT (snapshot_sizes[0] != 0);
    jerry_cleanup ();

    code_to_snapshot_p = "456";

    jerry_init (JERRY_INIT_EMPTY);
    snapshot_sizes[1] = jerry_parse_and_save_snapshot ((jerry_char_t *) code_to_snapshot_p,
                                                       strlen (code_to_snapshot_p),
                                                       true,
                                                       false,
                                                       snapshot_buffer_1,
                                                       SNAPSHOT_BUFFER_SIZE);
    TEST_ASSERT (snapshot_sizes[1] != 0);
    jerry_cleanup ();

    jerry_init (JERRY_INIT_EMPTY);

    const char *error_p;
    const uint32_t *snapshot_buffers[2];

    snapshot_buffers[0] = snapshot_buffer_0;
    snapshot_buffers[1] = snapshot_buffer_1;

    size_t merged_size = jerry_merge_snapshots (snapshot_buffers,
                                                snapshot_sizes,
                                                2,
                                                merged_snapshot_buffer,
                                                SNAPSHOT_BUFFER_SIZE,
                                                &error_p);

    jerry_cleanup ();


    jerry_init (JERRY_INIT_EMPTY);

    jerry_value_t res = jerry_exec_snapshot_at (merged_snapshot_buffer, merged_size, 0, false);
    TEST_ASSERT (!jerry_value_has_error_flag (res));
    TEST_ASSERT (jerry_get_number_value (res) == 123);
    jerry_release_value (res);

    res = jerry_exec_snapshot_at (merged_snapshot_buffer, merged_size, 1, false);
    TEST_ASSERT (!jerry_value_has_error_flag (res));
    TEST_ASSERT (jerry_get_number_value (res) == 456);
    jerry_release_value (res);

    jerry_cleanup ();
  }

  /* Save literals */
  if (jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE))
  {
    /* C format generation */
    jerry_init (JERRY_INIT_EMPTY);

    static uint32_t literal_buffer_c[SNAPSHOT_BUFFER_SIZE];
    static const char *code_for_c_format_p = "var object = { aa:'fo o', Bb:'max', aaa:'xzy0' };";

    size_t literal_sizes_c_format = jerry_parse_and_save_literals ((jerry_char_t *) code_for_c_format_p,
                                                                   strlen (code_for_c_format_p),
                                                                   false,
                                                                   literal_buffer_c,
                                                                   SNAPSHOT_BUFFER_SIZE,
                                                                   true);
    TEST_ASSERT (literal_sizes_c_format == 203);

    static const char *expected_c_format = (
                                            "jerry_length_t literal_count = 4;\n\n"
                                            "jerry_char_ptr_t literals[4] =\n"
                                            "{\n"
                                            "  \"Bb\",\n"
                                            "  \"aa\",\n"
                                            "  \"aaa\",\n"
                                            "  \"xzy0\"\n"
                                            "};\n\n"
                                            "jerry_length_t literal_sizes[4] =\n"
                                            "{\n"
                                            "  2 /* Bb */,\n"
                                            "  2 /* aa */,\n"
                                            "  3 /* aaa */,\n"
                                            "  4 /* xzy0 */\n"
                                            "};\n"
                                            );

    TEST_ASSERT (!strncmp ((char *) literal_buffer_c, expected_c_format, literal_sizes_c_format));
    jerry_cleanup ();

    /* List format generation */
    jerry_init (JERRY_INIT_EMPTY);

    static uint32_t literal_buffer_list[SNAPSHOT_BUFFER_SIZE];
    static const char *code_for_list_format_p = "var obj = { a:'aa', bb:'Bb' };";

    size_t literal_sizes_list_format = jerry_parse_and_save_literals ((jerry_char_t *) code_for_list_format_p,
                                                                      strlen (code_for_list_format_p),
                                                                      false,
                                                                      literal_buffer_list,
                                                                      SNAPSHOT_BUFFER_SIZE,
                                                                      false);

    TEST_ASSERT (literal_sizes_list_format == 25);
    TEST_ASSERT (!strncmp ((char *) literal_buffer_list, "1 a\n2 Bb\n2 aa\n2 bb\n3 obj\n", literal_sizes_list_format));

    jerry_cleanup ();
  }

  test_function_snapshot ();

  return 0;
} /* main */

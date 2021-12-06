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

#define T(op, lhs, rhs, res) \
  {                          \
    op, lhs, rhs, res        \
  }

#define T_NAN(op, lhs, rhs) \
  {                         \
    op, lhs, rhs            \
  }

#define T_ERR(op, lsh, rhs) T_NAN (op, lsh, rhs)

#define T_ARI(lhs, rhs)                                                                                       \
  T_NAN (JERRY_BIN_OP_SUB, lhs, rhs), T_NAN (JERRY_BIN_OP_MUL, lhs, rhs), T_NAN (JERRY_BIN_OP_DIV, lhs, rhs), \
    T_NAN (JERRY_BIN_OP_REM, lhs, rhs)

typedef struct
{
  jerry_binary_op_t op;
  jerry_value_t lhs;
  jerry_value_t rhs;
  jerry_value_t expected;
} test_entry_t;

typedef struct
{
  jerry_binary_op_t op;
  jerry_value_t lhs;
  jerry_value_t rhs;
} test_nan_entry_t;

typedef test_nan_entry_t test_error_entry_t;

int
main (void)
{
  TEST_INIT ();

  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t obj1 = jerry_eval ((jerry_char_t *) "o={x:1};o", 9, JERRY_PARSE_NO_OPTS);
  jerry_value_t obj2 = jerry_eval ((jerry_char_t *) "o={x:1};o", 9, JERRY_PARSE_NO_OPTS);
  jerry_value_t err1 = jerry_throw_sz (JERRY_ERROR_SYNTAX, "error");

  test_nan_entry_t test_nans[] = {
    /* Testing addition (+) */
    T_NAN (JERRY_BIN_OP_ADD, jerry_number (3.1), jerry_undefined ()),
    T_NAN (JERRY_BIN_OP_ADD, jerry_undefined (), jerry_undefined ()),
    T_NAN (JERRY_BIN_OP_ADD, jerry_undefined (), jerry_null ()),

    /* Testing subtraction (-), multiplication (*), division (/), remainder (%) */
    T_ARI (jerry_number (3.1), jerry_undefined ()),
    T_ARI (jerry_string_sz ("foo"), jerry_string_sz ("bar")),
    T_ARI (jerry_string_sz ("foo"), jerry_undefined ()),
    T_ARI (jerry_string_sz ("foo"), jerry_null ()),
    T_ARI (jerry_string_sz ("foo"), jerry_number (5.0)),
    T_ARI (jerry_undefined (), jerry_string_sz ("foo")),
    T_ARI (jerry_null (), jerry_string_sz ("foo")),
    T_ARI (jerry_number (5.0), jerry_string_sz ("foo")),
    T_ARI (jerry_undefined (), jerry_undefined ()),
    T_ARI (jerry_undefined (), jerry_null ()),
    T_ARI (jerry_null (), jerry_undefined ()),
    T_ARI (jerry_value_copy (obj1), jerry_value_copy (obj1)),
    T_ARI (jerry_value_copy (obj1), jerry_value_copy (obj2)),
    T_ARI (jerry_value_copy (obj2), jerry_value_copy (obj1)),
    T_ARI (jerry_value_copy (obj2), jerry_undefined ()),
    T_ARI (jerry_value_copy (obj1), jerry_string_sz ("foo")),
    T_ARI (jerry_value_copy (obj1), jerry_null ()),
    T_ARI (jerry_value_copy (obj1), jerry_boolean (true)),
    T_ARI (jerry_value_copy (obj1), jerry_boolean (false)),
    T_ARI (jerry_value_copy (obj1), jerry_number (5.0)),

    /* Testing division (/) */
    T_NAN (JERRY_BIN_OP_DIV, jerry_boolean (false), jerry_boolean (false)),
    T_NAN (JERRY_BIN_OP_DIV, jerry_number (0.0), jerry_number (0.0)),
    T_NAN (JERRY_BIN_OP_DIV, jerry_null (), jerry_null ()),

    /* Testing remainder (%) */
    T_NAN (JERRY_BIN_OP_REM, jerry_boolean (true), jerry_boolean (false)),
    T_NAN (JERRY_BIN_OP_REM, jerry_boolean (false), jerry_boolean (false)),
    T_NAN (JERRY_BIN_OP_REM, jerry_number (0.0), jerry_number (0.0)),
    T_NAN (JERRY_BIN_OP_REM, jerry_null (), jerry_null ()),
  };

  for (uint32_t idx = 0; idx < sizeof (test_nans) / sizeof (test_nan_entry_t); idx++)
  {
    jerry_value_t result = jerry_binary_op (test_nans[idx].op, test_nans[idx].lhs, test_nans[idx].rhs);
    TEST_ASSERT (jerry_value_is_number (result));

    double num = jerry_value_as_number (result);

    TEST_ASSERT (num != num);

    jerry_value_free (test_nans[idx].lhs);
    jerry_value_free (test_nans[idx].rhs);
    jerry_value_free (result);
  }

  test_entry_t tests[] = {
    /* Testing addition (+) */
    T (JERRY_BIN_OP_ADD, jerry_number (5.0), jerry_number (5.0), jerry_number (10.0)),
    T (JERRY_BIN_OP_ADD, jerry_number (3.1), jerry_number (10), jerry_number (13.1)),
    T (JERRY_BIN_OP_ADD, jerry_number (3.1), jerry_boolean (true), jerry_number (4.1)),
    T (JERRY_BIN_OP_ADD, jerry_string_sz ("foo"), jerry_string_sz ("bar"), jerry_string_sz ("foobar")),
    T (JERRY_BIN_OP_ADD, jerry_string_sz ("foo"), jerry_undefined (), jerry_string_sz ("fooundefined")),
    T (JERRY_BIN_OP_ADD, jerry_string_sz ("foo"), jerry_null (), jerry_string_sz ("foonull")),
    T (JERRY_BIN_OP_ADD, jerry_string_sz ("foo"), jerry_number (5.0), jerry_string_sz ("foo5")),

    T (JERRY_BIN_OP_ADD, jerry_null (), jerry_null (), jerry_number (0.0)),
    T (JERRY_BIN_OP_ADD, jerry_boolean (true), jerry_boolean (true), jerry_number (2.0)),
    T (JERRY_BIN_OP_ADD, jerry_boolean (true), jerry_boolean (false), jerry_number (1.0)),
    T (JERRY_BIN_OP_ADD, jerry_boolean (false), jerry_boolean (true), jerry_number (1.0)),
    T (JERRY_BIN_OP_ADD, jerry_boolean (false), jerry_boolean (false), jerry_number (0.0)),
    T (JERRY_BIN_OP_ADD,
       jerry_value_copy (obj1),
       jerry_value_copy (obj1),
       jerry_string_sz ("[object Object][object Object]")),
    T (JERRY_BIN_OP_ADD,
       jerry_value_copy (obj1),
       jerry_value_copy (obj2),
       jerry_string_sz ("[object Object][object Object]")),
    T (JERRY_BIN_OP_ADD,
       jerry_value_copy (obj2),
       jerry_value_copy (obj1),
       jerry_string_sz ("[object Object][object Object]")),
    T (JERRY_BIN_OP_ADD, jerry_value_copy (obj1), jerry_null (), jerry_string_sz ("[object Object]null")),
    T (JERRY_BIN_OP_ADD, jerry_value_copy (obj1), jerry_undefined (), jerry_string_sz ("[object Object]undefined")),
    T (JERRY_BIN_OP_ADD, jerry_value_copy (obj1), jerry_boolean (true), jerry_string_sz ("[object Object]true")),
    T (JERRY_BIN_OP_ADD, jerry_value_copy (obj1), jerry_boolean (false), jerry_string_sz ("[object Object]false")),
    T (JERRY_BIN_OP_ADD, jerry_value_copy (obj1), jerry_number (5.0), jerry_string_sz ("[object Object]5")),
    T (JERRY_BIN_OP_ADD, jerry_value_copy (obj1), jerry_string_sz ("foo"), jerry_string_sz ("[object Object]foo")),

    /* Testing subtraction (-) */
    T (JERRY_BIN_OP_SUB, jerry_number (5.0), jerry_number (5.0), jerry_number (0.0)),
    T (JERRY_BIN_OP_SUB, jerry_number (3.1), jerry_number (10), jerry_number (-6.9)),
    T (JERRY_BIN_OP_SUB, jerry_number (3.1), jerry_boolean (true), jerry_number (2.1)),
    T (JERRY_BIN_OP_SUB, jerry_boolean (true), jerry_boolean (true), jerry_number (0.0)),
    T (JERRY_BIN_OP_SUB, jerry_boolean (true), jerry_boolean (false), jerry_number (1.0)),
    T (JERRY_BIN_OP_SUB, jerry_boolean (false), jerry_boolean (true), jerry_number (-1.0)),
    T (JERRY_BIN_OP_SUB, jerry_boolean (false), jerry_boolean (false), jerry_number (0.0)),
    T (JERRY_BIN_OP_SUB, jerry_null (), jerry_null (), jerry_number (-0.0)),

    /* Testing multiplication (*) */
    T (JERRY_BIN_OP_MUL, jerry_number (5.0), jerry_number (5.0), jerry_number (25.0)),
    T (JERRY_BIN_OP_MUL, jerry_number (3.1), jerry_number (10), jerry_number (31)),
    T (JERRY_BIN_OP_MUL, jerry_number (3.1), jerry_boolean (true), jerry_number (3.1)),
    T (JERRY_BIN_OP_MUL, jerry_boolean (true), jerry_boolean (true), jerry_number (1.0)),
    T (JERRY_BIN_OP_MUL, jerry_boolean (true), jerry_boolean (false), jerry_number (0.0)),
    T (JERRY_BIN_OP_MUL, jerry_boolean (false), jerry_boolean (true), jerry_number (0.0)),
    T (JERRY_BIN_OP_MUL, jerry_boolean (false), jerry_boolean (false), jerry_number (0.0)),
    T (JERRY_BIN_OP_MUL, jerry_null (), jerry_null (), jerry_number (0.0)),

    /* Testing division (/) */
    T (JERRY_BIN_OP_DIV, jerry_number (5.0), jerry_number (5.0), jerry_number (1.0)),
    T (JERRY_BIN_OP_DIV, jerry_number (3.1), jerry_number (10), jerry_number (0.31)),
    T (JERRY_BIN_OP_DIV, jerry_number (3.1), jerry_boolean (true), jerry_number (3.1)),
    T (JERRY_BIN_OP_DIV, jerry_boolean (true), jerry_boolean (true), jerry_number (1.0)),
    T (JERRY_BIN_OP_DIV, jerry_boolean (true), jerry_boolean (false), jerry_infinity (false)),
    T (JERRY_BIN_OP_DIV, jerry_boolean (false), jerry_boolean (true), jerry_number (0.0)),

    /* Testing remainder (%) */
    T (JERRY_BIN_OP_REM, jerry_number (5.0), jerry_number (5.0), jerry_number (0.0)),
    T (JERRY_BIN_OP_REM, jerry_number (5.0), jerry_number (2.0), jerry_number (1.0)),
    T (JERRY_BIN_OP_REM, jerry_number (3.1), jerry_number (10), jerry_number (3.1)),
    T (JERRY_BIN_OP_REM, jerry_number (3.1), jerry_boolean (true), jerry_number (0.10000000000000009)),
    T (JERRY_BIN_OP_REM, jerry_boolean (true), jerry_boolean (true), jerry_number (0.0)),
    T (JERRY_BIN_OP_REM, jerry_boolean (false), jerry_boolean (true), jerry_number (0.0)),

  };

  for (uint32_t idx = 0; idx < sizeof (tests) / sizeof (test_entry_t); idx++)
  {
    jerry_value_t result = jerry_binary_op (tests[idx].op, tests[idx].lhs, tests[idx].rhs);
    TEST_ASSERT (!jerry_value_is_exception (result));

    jerry_value_t equals = jerry_binary_op (JERRY_BIN_OP_STRICT_EQUAL, result, tests[idx].expected);
    TEST_ASSERT (jerry_value_is_boolean (equals) && jerry_value_is_true (equals));
    jerry_value_free (equals);

    jerry_value_free (tests[idx].lhs);
    jerry_value_free (tests[idx].rhs);
    jerry_value_free (tests[idx].expected);
    jerry_value_free (result);
  }

  jerry_value_t obj3 = jerry_eval ((jerry_char_t *) "o={valueOf:function(){throw 5}};o", 33, JERRY_PARSE_NO_OPTS);

  test_error_entry_t error_tests[] = {
    /* Testing addition (+) */
    T_ERR (JERRY_BIN_OP_ADD, jerry_value_copy (err1), jerry_value_copy (err1)),
    T_ERR (JERRY_BIN_OP_ADD, jerry_value_copy (err1), jerry_undefined ()),
    T_ERR (JERRY_BIN_OP_ADD, jerry_undefined (), jerry_value_copy (err1)),

    /* Testing subtraction (-), multiplication (*), division (/), remainder (%) */
    T_ARI (jerry_value_copy (err1), jerry_value_copy (err1)),
    T_ARI (jerry_value_copy (err1), jerry_undefined ()),
    T_ARI (jerry_undefined (), jerry_value_copy (err1)),

    /* Testing addition (+) */
    T_ERR (JERRY_BIN_OP_ADD, jerry_value_copy (obj3), jerry_undefined ()),
    T_ERR (JERRY_BIN_OP_ADD, jerry_value_copy (obj3), jerry_null ()),
    T_ERR (JERRY_BIN_OP_ADD, jerry_value_copy (obj3), jerry_boolean (true)),
    T_ERR (JERRY_BIN_OP_ADD, jerry_value_copy (obj3), jerry_boolean (false)),
    T_ERR (JERRY_BIN_OP_ADD, jerry_value_copy (obj3), jerry_value_copy (obj2)),
    T_ERR (JERRY_BIN_OP_ADD, jerry_value_copy (obj3), jerry_string_sz ("foo")),

    /* Testing subtraction (-), multiplication (*), division (/), remainder (%) */
    T_ARI (jerry_value_copy (obj3), jerry_undefined ()),
    T_ARI (jerry_value_copy (obj3), jerry_null ()),
    T_ARI (jerry_value_copy (obj3), jerry_boolean (true)),
    T_ARI (jerry_value_copy (obj3), jerry_boolean (false)),
    T_ARI (jerry_value_copy (obj3), jerry_value_copy (obj2)),
    T_ARI (jerry_value_copy (obj3), jerry_string_sz ("foo")),
  };

  for (uint32_t idx = 0; idx < sizeof (error_tests) / sizeof (test_error_entry_t); idx++)
  {
    jerry_value_t result = jerry_binary_op (tests[idx].op, error_tests[idx].lhs, error_tests[idx].rhs);
    TEST_ASSERT (jerry_value_is_exception (result));
    jerry_value_free (error_tests[idx].lhs);
    jerry_value_free (error_tests[idx].rhs);
    jerry_value_free (result);
  }

  jerry_value_free (obj1);
  jerry_value_free (obj2);
  jerry_value_free (obj3);
  jerry_value_free (err1);

  jerry_cleanup ();

  return 0;
} /* main */

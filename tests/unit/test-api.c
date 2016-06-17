/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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

#include "jerry.h"
#include "jerry-api.h"
#include "config.h"

#include "test-common.h"

const char *test_source = (
                           "function assert (arg) { "
                           "  if (!arg) { "
                           "    throw Error('Assert failed');"
                           "  } "
                           "} "
                           "this.t = 1; "
                           "function f () { "
                           "return this.t; "
                           "} "
                           "this.foo = f; "
                           "this.bar = function (a) { "
                           "return a + t; "
                           "}; "
                           "function A () { "
                           "this.t = 12; "
                           "} "
                           "this.A = A; "
                           "this.a = new A (); "
                           "function call_external () { "
                           "  return this.external ('1', true); "
                           "} "
                           "function call_throw_test() { "
                           "  var catched = false; "
                           "  try { "
                           "    this.throw_test(); "
                           "  } catch (e) { "
                           "    catched = true; "
                           "    assert(e.name == 'TypeError'); "
                           "    assert(e.message == 'error'); "
                           "  } "
                           "  assert(catched); "
                           "} "
                           "function throw_reference_error() { "
                           " throw new ReferenceError ();"
                           "} "
                           "p = {'alpha':32, 'bravo':false, 'charlie':{}, 'delta':123.45, 'echo':'foobar'};"
                           "np = {}; Object.defineProperty (np, 'foxtrot', { "
                           "get: function() { throw 'error'; }, enumerable: true }) "
                           );

bool test_api_is_free_callback_was_called = false;

static bool
handler (const jerry_object_t *function_obj_p, /**< function object */
         const jerry_value_t this_val, /**< this value */
         const jerry_value_t args_p[], /**< arguments list */
         const jerry_length_t args_cnt, /**< arguments length */
         jerry_value_t *ret_val_p) /**< [out] return value */
{
  char buffer[32];
  jerry_size_t sz;

  printf ("ok %p %p %p %d %p\n", function_obj_p, this_val, args_p, args_cnt, ret_val_p);

  JERRY_ASSERT (args_cnt == 2);

  JERRY_ASSERT (jerry_value_is_string (args_p[0]));
  sz = jerry_get_string_size (jerry_get_string_value (args_p[0]));
  JERRY_ASSERT (sz == 1);
  sz = jerry_string_to_char_buffer (jerry_get_string_value (args_p[0]),
                                    (jerry_char_t *) buffer,
                                    sz);
  JERRY_ASSERT (sz == 1);
  JERRY_ASSERT (!strncmp (buffer, "1", (size_t) sz));

  JERRY_ASSERT (jerry_value_is_boolean (args_p[1]));

  *ret_val_p = jerry_create_string_value (jerry_create_string ((jerry_char_t *) "string from handler"));

  return true;
} /* handler */

static bool
handler_throw_test (const jerry_object_t *function_obj_p, /**< function object */
                    const jerry_value_t this_val, /**< this value */
                    const jerry_value_t args_p[], /**< arguments list */
                    const jerry_length_t args_cnt, /**< arguments length */
                    jerry_value_t *ret_val_p) /**< [out] return value */
{
  printf ("ok %p %p %p %d %p\n", function_obj_p, this_val, args_p, args_cnt, ret_val_p);

  jerry_object_t *error_p = jerry_create_error (JERRY_ERROR_TYPE,
                                                (jerry_char_t *) "error");

  *ret_val_p = jerry_create_object_value (error_p);

  return false;
} /* handler_throw_test */

static void
handler_construct_freecb (uintptr_t native_p)
{
  JERRY_ASSERT (native_p == (uintptr_t) 0x0012345678abcdefull);
  printf ("ok object free callback\n");

  test_api_is_free_callback_was_called = true;
} /* handler_construct_freecb */

static bool
handler_construct (const jerry_object_t *function_obj_p, /**< function object */
                   const jerry_value_t this_val, /**< this value */
                   const jerry_value_t args_p[], /**< arguments list */
                   const jerry_length_t args_cnt, /**< arguments length */
                   jerry_value_t *ret_val_p) /**< [out] return value */
{
  printf ("ok construct %p %p %p %d %p\n", function_obj_p, this_val, args_p, args_cnt, ret_val_p);

  JERRY_ASSERT (jerry_value_is_object (this_val));

  JERRY_ASSERT (args_cnt == 1);
  JERRY_ASSERT (jerry_value_is_boolean (args_p[0]));
  JERRY_ASSERT (jerry_get_boolean_value (args_p[0]) == true);

  jerry_set_object_field_value (jerry_get_object_value (this_val),
                                (jerry_char_t *) "value_field",
                                args_p[0]);

  jerry_set_object_native_handle (jerry_get_object_value (this_val),
                                  (uintptr_t) 0x0000000000000000ull,
                                  handler_construct_freecb);

  uintptr_t ptr;
  bool is_ok = jerry_get_object_native_handle (jerry_get_object_value (this_val), &ptr);
  JERRY_ASSERT (is_ok && ptr == (uintptr_t) 0x0000000000000000ull);

  /* check if setting handle for second time is handled correctly */
  jerry_set_object_native_handle (jerry_get_object_value (this_val),
                                  (uintptr_t) 0x0012345678abcdefull,
                                  handler_construct_freecb);

  return true;
} /* handler_construct */

/**
 * Extended Magic Strings
 */
#define JERRY_MAGIC_STRING_ITEMS \
  JERRY_MAGIC_STRING_DEF (GLOBAL, global) \
  JERRY_MAGIC_STRING_DEF (CONSOLE, console)


#define JERRY_MAGIC_STRING_DEF(NAME, STRING) \
  static const char jerry_magic_string_ex_ ## NAME[] = # STRING;

JERRY_MAGIC_STRING_ITEMS

#undef JERRY_MAGIC_STRING_DEF

const jerry_length_t magic_string_lengths[] =
{
#define JERRY_MAGIC_STRING_DEF(NAME, STRING) \
    (jerry_length_t) (sizeof (jerry_magic_string_ex_ ## NAME) - 1u),

  JERRY_MAGIC_STRING_ITEMS

#undef JERRY_MAGIC_STRING_DEF
};

const jerry_char_ptr_t magic_string_items[] =
{
#define JERRY_MAGIC_STRING_DEF(NAME, STRING) \
    (const jerry_char_ptr_t) jerry_magic_string_ex_ ## NAME,

  JERRY_MAGIC_STRING_ITEMS

#undef JERRY_MAGIC_STRING_DEF
};

static bool
foreach (const jerry_string_t *name, /**< field name */
         const jerry_value_t value, /**< field value */
         void *user_data) /**< user data */
{
  char str_buf_p[128];
  jerry_size_t sz = jerry_string_to_char_buffer (name, (jerry_char_t *) str_buf_p, 128);
  str_buf_p[sz] = '\0';

  if (!strncmp (str_buf_p, "alpha", (size_t) sz))
  {
    JERRY_ASSERT (jerry_value_is_number (value));
    JERRY_ASSERT (jerry_get_number_value (value) == 32.0);
  }
  else if (!strncmp (str_buf_p, "bravo", (size_t) sz))
  {
    JERRY_ASSERT (jerry_value_is_boolean (value));
    JERRY_ASSERT (jerry_get_boolean_value (value) == false);
  }
  else if (!strncmp (str_buf_p, "charlie", (size_t) sz))
  {
    JERRY_ASSERT (jerry_value_is_object (value));
  }
  else if (!strncmp (str_buf_p, "delta", (size_t) sz))
  {
    JERRY_ASSERT (jerry_value_is_number (value));
    JERRY_ASSERT (jerry_get_number_value (value) == 123.45);
  }
  else if (!strncmp (str_buf_p, "echo", (size_t) sz))
  {
    JERRY_ASSERT (jerry_value_is_string (value));
    jerry_size_t echo_sz = jerry_string_to_char_buffer (jerry_get_string_value (value),
                                                        (jerry_char_t *) str_buf_p,
                                                        128);
    str_buf_p[echo_sz] = '\0';
    JERRY_ASSERT (!strncmp (str_buf_p, "foobar", (size_t) echo_sz));
  }
  else
  {
    JERRY_ASSERT (false);
  }

  JERRY_ASSERT (!strncmp ((const char *) user_data, "user_data", 9));
  return true;
} /* foreach */

static bool
foreach_exception (const jerry_string_t *name, /**< field name */
                   const jerry_value_t value, /**< field value */
                   void *user_data) /**< user data */
{
  JERRY_UNUSED (value);
  JERRY_UNUSED (user_data);
  char str_buf_p[128];
  jerry_size_t sz = jerry_string_to_char_buffer (name, (jerry_char_t *) str_buf_p, 128);
  str_buf_p[sz] = '\0';

  if (!strncmp (str_buf_p, "foxtrot", (size_t) sz))
  {
    JERRY_ASSERT (false);
  }
  return true;
} /* foreach_exception */

static bool
foreach_subset (const jerry_string_t *name, /**< field name */
                const jerry_value_t value, /**< field value */
                void *user_data) /**< user data */
{
  JERRY_UNUSED (name);
  JERRY_UNUSED (value);
  int *count_p = (int *) (user_data);

  if (*count_p == 3)
  {
    return false;
  }
  (*count_p)++;
  return true;
} /* foreach_subset */

int
main (void)
{
  TEST_INIT ();

  jerry_init (JERRY_FLAG_EMPTY);

  bool is_ok;
  jerry_size_t sz;
  jerry_value_t val_t, val_foo, val_bar, val_A, val_A_prototype, val_a, val_a_foo, val_value_field, val_p, val_np;
  jerry_value_t val_external, val_external_construct, val_call_external;
  jerry_object_t *global_obj_p, *obj_p;
  jerry_object_t *external_func_p, *external_construct_p;
  jerry_object_t *throw_test_handler_p;
  jerry_object_t *err_obj_p = NULL;
  jerry_value_t res, args[2];
  char buffer[32];

  is_ok = jerry_parse ((jerry_char_t *) test_source, strlen (test_source), &err_obj_p);
  JERRY_ASSERT (is_ok && err_obj_p == NULL);

  is_ok = (jerry_run (&res) == JERRY_COMPLETION_CODE_OK);
  JERRY_ASSERT (is_ok);
  JERRY_ASSERT (jerry_value_is_undefined (res));

  global_obj_p = jerry_get_global ();

  // Test corner case for jerry_string_to_char_buffer
  args[0] = jerry_create_string_value (jerry_create_string ((jerry_char_t *) ""));
  sz = jerry_get_string_size (jerry_get_string_value (args[0]));
  JERRY_ASSERT (sz == 0);
  jerry_release_value (args[0]);

  // Get global.boo (non-existing field)
  val_t = jerry_get_object_field_value (global_obj_p, (jerry_char_t *) "boo");
  JERRY_ASSERT (!jerry_value_is_error (val_t));
  JERRY_ASSERT (jerry_value_is_undefined (val_t));

  // Get global.t
  val_t = jerry_get_object_field_value (global_obj_p, (jerry_char_t *) "t");
  JERRY_ASSERT (!jerry_value_is_error (val_t));
  JERRY_ASSERT (jerry_value_is_number (val_t)
                && jerry_get_number_value (val_t) == 1.0);
  jerry_release_value (val_t);

  // Get global.foo
  val_foo = jerry_get_object_field_value (global_obj_p, (jerry_char_t *) "foo");
  JERRY_ASSERT (!jerry_value_is_error (val_foo));
  JERRY_ASSERT (jerry_value_is_object (val_foo));

  // Call foo (4, 2)
  args[0] = jerry_create_number_value (4);
  args[1] = jerry_create_number_value (2);
  res = jerry_call_function (jerry_get_object_value (val_foo), NULL, args, 2);
  JERRY_ASSERT (!jerry_value_is_error (res));
  JERRY_ASSERT (jerry_value_is_number (res)
                && jerry_get_number_value (res) == 1.0);
  jerry_release_value (res);

  // Get global.bar
  val_bar = jerry_get_object_field_value (global_obj_p, (jerry_char_t *) "bar");
  JERRY_ASSERT (!jerry_value_is_error (val_bar));
  JERRY_ASSERT (jerry_value_is_object (val_bar));

  // Call bar (4, 2)
  res = jerry_call_function (jerry_get_object_value (val_bar), NULL, args, 2);
  JERRY_ASSERT (!jerry_value_is_error (res));
  JERRY_ASSERT (jerry_value_is_number (res)
                && jerry_get_number_value (res) == 5.0);
  jerry_release_value (res);
  jerry_release_value (val_bar);

  // Set global.t = "abcd"
  jerry_release_value (args[0]);
  args[0] = jerry_create_string_value (jerry_create_string ((jerry_char_t *) "abcd"));
  is_ok = jerry_set_object_field_value (global_obj_p, (jerry_char_t *) "t", args[0]);
  JERRY_ASSERT (is_ok);

  // Call foo (4, 2)
  res = jerry_call_function (jerry_get_object_value (val_foo), NULL, args, 2);
  JERRY_ASSERT (!jerry_value_is_error (res));
  JERRY_ASSERT (jerry_value_is_string (res));
  sz = jerry_get_string_size (jerry_get_string_value (res));
  JERRY_ASSERT (sz == 4);
  sz = jerry_string_to_char_buffer (jerry_get_string_value (res), (jerry_char_t *) buffer, sz);
  JERRY_ASSERT (sz == 4);
  jerry_release_value (res);
  JERRY_ASSERT (!strncmp (buffer, "abcd", (size_t) sz));
  jerry_release_value (args[0]);
  jerry_release_value (args[1]);

  // Get global.A
  val_A = jerry_get_object_field_value (global_obj_p, (jerry_char_t *) "A");
  JERRY_ASSERT (!jerry_value_is_error (val_A));
  JERRY_ASSERT (jerry_value_is_object (val_A));

  // Get A.prototype
  is_ok = jerry_is_constructor (jerry_get_object_value (val_A));
  JERRY_ASSERT (is_ok);
  val_A_prototype = jerry_get_object_field_value (jerry_get_object_value (val_A),
                                                  (jerry_char_t *) "prototype");
  JERRY_ASSERT (!jerry_value_is_error (val_A_prototype));
  JERRY_ASSERT (jerry_value_is_object (val_A_prototype));
  jerry_release_value (val_A);

  // Set A.prototype.foo = global.foo
  is_ok = jerry_set_object_field_value (jerry_get_object_value (val_A_prototype),
                                        (jerry_char_t *) "foo",
                                        val_foo);
  JERRY_ASSERT (is_ok);
  jerry_release_value (val_A_prototype);
  jerry_release_value (val_foo);

  // Get global.a
  val_a = jerry_get_object_field_value (global_obj_p, (jerry_char_t *) "a");
  JERRY_ASSERT (!jerry_value_is_error (val_a));
  JERRY_ASSERT (jerry_value_is_object (val_a));

  // Get a.t
  res = jerry_get_object_field_value (jerry_get_object_value (val_a), (jerry_char_t *) "t");
  JERRY_ASSERT (!jerry_value_is_error (res));
  JERRY_ASSERT (jerry_value_is_number (res)
                && jerry_get_number_value (res) == 12.0);
  jerry_release_value (res);

  // foreach properties
  val_p = jerry_get_object_field_value (global_obj_p, (jerry_char_t *) "p");
  is_ok = jerry_foreach_object_field (jerry_get_object_value (val_p), foreach, (void *) "user_data");
  JERRY_ASSERT (is_ok);

  // break foreach at third element
  int count = 0;
  is_ok = jerry_foreach_object_field (jerry_get_object_value (val_p), foreach_subset, &count);
  JERRY_ASSERT (is_ok);
  JERRY_ASSERT (count == 3);
  jerry_release_value (val_p);

  // foreach with throw test
  val_np = jerry_get_object_field_value (global_obj_p, (jerry_char_t *) "np");
  is_ok = !jerry_foreach_object_field (jerry_get_object_value (val_np), foreach_exception, NULL);
  JERRY_ASSERT (is_ok);
  jerry_release_value (val_np);

  // Get a.foo
  val_a_foo = jerry_get_object_field_value (jerry_get_object_value (val_a),
                                            (jerry_char_t *) "foo");
  JERRY_ASSERT (!jerry_value_is_error (val_a_foo));
  JERRY_ASSERT (jerry_value_is_object (val_a_foo));

  // Call a.foo ()
  res = jerry_call_function (jerry_get_object_value (val_a_foo),
                             jerry_get_object_value (val_a),
                             NULL,
                             0);
  JERRY_ASSERT (!jerry_value_is_error (res));
  JERRY_ASSERT (jerry_value_is_number (res)
                && jerry_get_number_value (res) == 12.0);
  jerry_release_value (res);
  jerry_release_value (val_a_foo);

  jerry_release_value (val_a);

  // Create native handler bound function object and set it to 'external' variable
  external_func_p = jerry_create_external_function (handler);
  JERRY_ASSERT (external_func_p != NULL
                && jerry_is_function (external_func_p)
                && jerry_is_constructor (external_func_p));

  val_external = jerry_create_object_value (external_func_p);
  is_ok = jerry_set_object_field_value (global_obj_p,
                                        (jerry_char_t *) "external",
                                        val_external);
  JERRY_ASSERT (is_ok);
  jerry_release_value (val_external);

  // Call 'call_external' function that should call external function created above
  val_call_external = jerry_get_object_field_value (global_obj_p,
                                                    (jerry_char_t *) "call_external");
  JERRY_ASSERT (!jerry_value_is_error (val_call_external));
  JERRY_ASSERT (jerry_value_is_object (val_call_external));
  res = jerry_call_function (jerry_get_object_value (val_call_external),
                             global_obj_p,
                             NULL,
                             0);
  jerry_release_value (val_call_external);
  JERRY_ASSERT (!jerry_value_is_error (res));
  JERRY_ASSERT (jerry_value_is_string (res));
  sz = jerry_get_string_size (jerry_get_string_value (res));
  JERRY_ASSERT (sz == 19);
  sz = jerry_string_to_char_buffer (jerry_get_string_value (res), (jerry_char_t *) buffer, sz);
  JERRY_ASSERT (sz == 19);
  jerry_release_value (res);
  JERRY_ASSERT (!strncmp (buffer, "string from handler", (size_t) sz));

  // Create native handler bound function object and set it to 'external_construct' variable
  external_construct_p = jerry_create_external_function (handler_construct);
  JERRY_ASSERT (external_construct_p != NULL
                && jerry_is_function (external_construct_p)
                && jerry_is_constructor (external_construct_p));

  val_external_construct = jerry_create_object_value (external_construct_p);
  is_ok = jerry_set_object_field_value (global_obj_p,
                                        (jerry_char_t *) "external_construct",
                                        val_external_construct);
  JERRY_ASSERT (is_ok);

  // Call external function created above, as constructor
  args[0] = jerry_create_boolean_value (true);
  res = jerry_construct_object (external_construct_p, args, 1);
  JERRY_ASSERT (!jerry_value_is_error (res));
  JERRY_ASSERT (jerry_value_is_object (res));
  val_value_field = jerry_get_object_field_value (jerry_get_object_value (res),
                                                  (jerry_char_t *) "value_field");

  // Get 'value_field' of constructed object
  JERRY_ASSERT (!jerry_value_is_error (val_value_field));
  JERRY_ASSERT (jerry_value_is_boolean (val_value_field)
                && jerry_get_boolean_value (val_value_field));
  jerry_release_value (val_value_field);
  jerry_release_object (external_construct_p);

  uintptr_t ptr;
  is_ok = jerry_get_object_native_handle (jerry_get_object_value (res), &ptr);
  JERRY_ASSERT (is_ok
                && ptr == (uintptr_t) 0x0012345678abcdefull);

  jerry_release_value (res);


  // Test: Throwing exception from native handler.
  throw_test_handler_p = jerry_create_external_function (handler_throw_test);
  JERRY_ASSERT (throw_test_handler_p != NULL
                && jerry_is_function (throw_test_handler_p));

  val_t = jerry_create_object_value (throw_test_handler_p);
  is_ok = jerry_set_object_field_value (global_obj_p,
                                        (jerry_char_t *) "throw_test",
                                        val_t);
  JERRY_ASSERT (is_ok);
  jerry_release_value (val_t);

  val_t = jerry_get_object_field_value (global_obj_p, (jerry_char_t *) "call_throw_test");
  JERRY_ASSERT (!jerry_value_is_error (val_t));
  JERRY_ASSERT (jerry_value_is_object (val_t));

  res = jerry_call_function (jerry_get_object_value (val_t),
                             global_obj_p,
                             NULL,
                             0);
  JERRY_ASSERT (!jerry_value_is_error (res));
  jerry_release_value (val_t);
  jerry_release_value (res);

  // Test: Unhandled exception in called function
  val_t = jerry_get_object_field_value (global_obj_p,
                                        (jerry_char_t *) "throw_reference_error");
  JERRY_ASSERT (!jerry_value_is_error (val_t));
  JERRY_ASSERT (jerry_value_is_object (val_t));

  res = jerry_call_function (jerry_get_object_value (val_t),
                               global_obj_p,
                               NULL,
                               0);

  JERRY_ASSERT (jerry_value_is_error (res));
  jerry_release_value (val_t);

  // 'res' should contain exception object
  JERRY_ASSERT (jerry_value_is_object (res));
  jerry_release_value (res);

  // Test: Call of non-function
  obj_p = jerry_create_object ();
  res = jerry_call_function (obj_p,
                             global_obj_p,
                             NULL,
                             0);
  JERRY_ASSERT (jerry_value_is_error (res));

  // 'res' should contain exception object
  JERRY_ASSERT (jerry_value_is_object (res));
  jerry_release_value (res);

  jerry_release_object (obj_p);

  // Test: Unhandled exception in function called, as constructor
  val_t = jerry_get_object_field_value (global_obj_p,
                                        (jerry_char_t *) "throw_reference_error");
  JERRY_ASSERT (!jerry_value_is_error (val_t));
  JERRY_ASSERT (jerry_value_is_object (val_t));

  res = jerry_construct_object (jerry_get_object_value (val_t), NULL, 0);
  JERRY_ASSERT (jerry_value_is_error (res));
  jerry_release_value (val_t);

  // 'res' should contain exception object
  JERRY_ASSERT (jerry_value_is_object (res));
  jerry_release_value (res);

  // Test: Call of non-function as constructor
  obj_p = jerry_create_object ();
  res = jerry_construct_object (obj_p, NULL, 0);
  JERRY_ASSERT (jerry_value_is_error (res));

  // 'res' should contain exception object
  JERRY_ASSERT (jerry_value_is_object (res));
  jerry_release_value (res);

  jerry_release_object (obj_p);


  // Test: Array Object API
  jerry_object_t *array_obj_p = jerry_create_array_object (10);
  JERRY_ASSERT (jerry_is_array (array_obj_p));
  JERRY_ASSERT (jerry_get_array_length (array_obj_p) == 10);

  jerry_value_t v_in = jerry_create_number_value (10.5);
  jerry_set_array_index_value (array_obj_p, 5, v_in);
  jerry_value_t v_out;
  jerry_get_array_index_value (array_obj_p, 5, &v_out);

  JERRY_ASSERT (jerry_value_is_number (v_out)
                && jerry_get_number_value (v_out) == 10.5);

  jerry_release_value (v_in);
  jerry_release_value (v_out);
  jerry_release_object (array_obj_p);

  // Test: eval
  const char *eval_code_src_p = "(function () { return 123; })";
  jerry_completion_code_t status = jerry_eval ((jerry_char_t *) eval_code_src_p,
                                               strlen (eval_code_src_p),
                                               false,
                                               true,
                                               &val_t);
  JERRY_ASSERT (status == JERRY_COMPLETION_CODE_OK);
  JERRY_ASSERT (jerry_value_is_object (val_t));
  JERRY_ASSERT (jerry_is_function (jerry_get_object_value (val_t)));

  res = jerry_call_function (jerry_get_object_value (val_t),
                             NULL,
                             NULL,
                             0);
  JERRY_ASSERT (!jerry_value_is_error (res));
  JERRY_ASSERT (jerry_value_is_number (res)
                && jerry_get_number_value (res) == 123.0);
  jerry_release_value (res);

  jerry_release_value (val_t);

  // cleanup.
  jerry_release_object (global_obj_p);

  // TEST: run gc.
  jerry_gc ();

  jerry_cleanup ();

  JERRY_ASSERT (test_api_is_free_callback_was_called);

  // External Magic String
  jerry_init (JERRY_FLAG_SHOW_OPCODES);

  uint32_t num_magic_string_items = (uint32_t) (sizeof (magic_string_items) / sizeof (jerry_char_ptr_t));
  jerry_register_external_magic_strings (magic_string_items,
                                         num_magic_string_items,
                                         magic_string_lengths);

  const char *ms_code_src_p = "var global = {}; var console = [1]; var process = 1;";
  is_ok = jerry_parse ((jerry_char_t *) ms_code_src_p, strlen (ms_code_src_p), &err_obj_p);
  JERRY_ASSERT (is_ok && err_obj_p == NULL);

  is_ok = (jerry_run (&res) == JERRY_COMPLETION_CODE_OK);
  JERRY_ASSERT (is_ok);
  JERRY_ASSERT (jerry_value_is_undefined (res));

  jerry_cleanup ();

  // Dump / execute snapshot
  // FIXME: support save/load snapshot for optimized parser
  if (false)
  {
    static uint8_t global_mode_snapshot_buffer[1024];
    static uint8_t eval_mode_snapshot_buffer[1024];

    const char *code_to_snapshot_p = "(function () { return 'string from snapshot'; }) ();";

    jerry_init (JERRY_FLAG_SHOW_OPCODES);
    size_t global_mode_snapshot_size = jerry_parse_and_save_snapshot ((jerry_char_t *) code_to_snapshot_p,
                                                                      strlen (code_to_snapshot_p),
                                                                      true,
                                                                      global_mode_snapshot_buffer,
                                                                      sizeof (global_mode_snapshot_buffer));
    JERRY_ASSERT (global_mode_snapshot_size != 0);
    jerry_cleanup ();

    jerry_init (JERRY_FLAG_SHOW_OPCODES);
    size_t eval_mode_snapshot_size = jerry_parse_and_save_snapshot ((jerry_char_t *) code_to_snapshot_p,
                                                                    strlen (code_to_snapshot_p),
                                                                    false,
                                                                    eval_mode_snapshot_buffer,
                                                                    sizeof (eval_mode_snapshot_buffer));
    JERRY_ASSERT (eval_mode_snapshot_size != 0);
    jerry_cleanup ();

    jerry_init (JERRY_FLAG_SHOW_OPCODES);

    is_ok = (jerry_exec_snapshot (global_mode_snapshot_buffer,
                                  global_mode_snapshot_size,
                                  false,
                                  &res) == JERRY_COMPLETION_CODE_OK);

    JERRY_ASSERT (is_ok);
    JERRY_ASSERT (jerry_value_is_undefined (res));
    jerry_release_value (res);

    is_ok = (jerry_exec_snapshot (eval_mode_snapshot_buffer,
                                  eval_mode_snapshot_size,
                                  false,
                                  &res) == JERRY_COMPLETION_CODE_OK);

    JERRY_ASSERT (is_ok);
    JERRY_ASSERT (jerry_value_is_string (res));
    sz = jerry_get_string_size (jerry_get_string_value (res));
    JERRY_ASSERT (sz == 20);
    sz = jerry_string_to_char_buffer (jerry_get_string_value (res), (jerry_char_t *) buffer, sz);
    JERRY_ASSERT (sz == 20);
    jerry_release_value (res);
    JERRY_ASSERT (!strncmp (buffer, "string from snapshot", (size_t) sz));

    jerry_cleanup ();
  }

  return 0;
} /* main */

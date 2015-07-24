/* Copyright 2015 Samsung Electronics Co., Ltd.
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
                           "  var catched = false "
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
                           );

bool test_api_is_free_callback_was_called = false;

/**
 * Initialize Jerry API value with specified boolean value
 */
static void
test_api_init_api_value_bool (jerry_api_value_t *out_value_p, /**< out: API value */
                              bool v) /**< boolean value to initialize with */
{
  out_value_p->type = JERRY_API_DATA_TYPE_BOOLEAN;
  out_value_p->v_bool = v;
} /* test_api_init_api_value_bool */

/**
 * Initialize Jerry API value with specified float64 number
 */
static void
test_api_init_api_value_float64 (jerry_api_value_t *out_value_p, /**< out: API value */
                                 double v) /**< float64 value to initialize with */
{
  out_value_p->type = JERRY_API_DATA_TYPE_FLOAT64;
  out_value_p->v_float64 = v;
} /* test_api_init_api_value_float64 */

/**
 * Initialize Jerry API value with specified string
 */
static void
test_api_init_api_value_string (jerry_api_value_t *out_value_p, /**< out: API value */
                                const char* v) /**< string value to initialize with */
{
  out_value_p->type = JERRY_API_DATA_TYPE_STRING;
  out_value_p->v_string = jerry_api_create_string ((jerry_api_char_t *) v);
} /* test_api_init_api_value_string */

/**
 * Initialize Jerry API value with specified object
 */
static void
test_api_init_api_value_object (jerry_api_value_t *out_value_p, /**< out: API value */
                                jerry_api_object_t* v) /**< object value to initialize with */
{
  jerry_api_acquire_object (v);

  out_value_p->type = JERRY_API_DATA_TYPE_OBJECT;
  out_value_p->v_object = v;
} /* test_api_init_api_value_object */

static bool
handler (const jerry_api_object_t *function_obj_p,
         const jerry_api_value_t *this_p,
         jerry_api_value_t *ret_val_p,
         const jerry_api_value_t args_p[],
         const jerry_api_length_t args_cnt)
{
  char buffer[32];
  ssize_t sz;

  printf ("ok %p %p %p %d %p\n", function_obj_p, this_p, args_p, args_cnt, ret_val_p);

  JERRY_ASSERT (args_cnt == 2);

  JERRY_ASSERT (args_p[0].type == JERRY_API_DATA_TYPE_STRING);
  sz = jerry_api_string_to_char_buffer (args_p[0].v_string, NULL, 0);
  JERRY_ASSERT (sz == -1);
  sz = jerry_api_string_to_char_buffer (args_p[0].v_string, (jerry_api_char_t *) buffer, -sz);
  JERRY_ASSERT (sz == 1);
  JERRY_ASSERT (!strncmp (buffer, "1", (size_t) sz));

  JERRY_ASSERT (args_p[1].type == JERRY_API_DATA_TYPE_BOOLEAN);
  JERRY_ASSERT (args_p[1].v_bool == true);

  test_api_init_api_value_string (ret_val_p, "string from handler");

  return true;
} /* handler */

static bool
handler_throw_test (const jerry_api_object_t *function_obj_p,
                    const jerry_api_value_t *this_p,
                    jerry_api_value_t *ret_val_p,
                    const jerry_api_value_t args_p[],
                    const jerry_api_length_t args_cnt)
{
  printf ("ok %p %p %p %d %p\n", function_obj_p, this_p, args_p, args_cnt, ret_val_p);

  jerry_api_object_t* error_p = jerry_api_create_error (JERRY_API_ERROR_TYPE,
                                                        (jerry_api_char_t *) "error");

  test_api_init_api_value_object (ret_val_p, error_p);

  jerry_api_release_object (error_p);

  return false;
}

static void
handler_construct_freecb (uintptr_t native_p)
{
  JERRY_ASSERT (native_p == (uintptr_t) 0x0012345678abcdefull);
  printf ("ok object free callback\n");

  test_api_is_free_callback_was_called = true;
} /* handler_construct_freecb */

static bool
handler_construct (const jerry_api_object_t *function_obj_p,
                   const jerry_api_value_t *this_p,
                   jerry_api_value_t *ret_val_p,
                   const jerry_api_value_t args_p[],
                   const jerry_api_length_t args_cnt)
{
  printf ("ok construct %p %p %p %d %p\n", function_obj_p, this_p, args_p, args_cnt, ret_val_p);

  JERRY_ASSERT (this_p != NULL);
  JERRY_ASSERT (this_p->type == JERRY_API_DATA_TYPE_OBJECT);

  JERRY_ASSERT (args_cnt == 1);
  JERRY_ASSERT (args_p[0].type == JERRY_API_DATA_TYPE_BOOLEAN);
  JERRY_ASSERT (args_p[0].v_bool == true);

  jerry_api_set_object_field_value (this_p->v_object, (jerry_api_char_t *) "value_field", &args_p[0]);

  jerry_api_set_object_native_handle (this_p->v_object,
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

const jerry_api_length_t magic_string_lengths[] =
{
#define JERRY_MAGIC_STRING_DEF(NAME, STRING) \
    (jerry_api_length_t)(sizeof(jerry_magic_string_ex_ ## NAME) - 1u),

  JERRY_MAGIC_STRING_ITEMS

#undef JERRY_MAGIC_STRING_DEF
};

const jerry_api_char_ptr_t magic_string_items[] =
{
#define JERRY_MAGIC_STRING_DEF(NAME, STRING) \
    (const jerry_api_char_ptr_t)jerry_magic_string_ex_ ## NAME,

  JERRY_MAGIC_STRING_ITEMS

#undef JERRY_MAGIC_STRING_DEF
};



int
main (void)
{
  TEST_INIT ();

  jerry_init (JERRY_FLAG_EMPTY);

  bool is_ok, is_exception;
  ssize_t sz;
  jerry_api_value_t val_t, val_foo, val_bar, val_A, val_A_prototype, val_a, val_a_foo, val_value_field;
  jerry_api_value_t val_external, val_external_construct, val_call_external;
  jerry_api_object_t* global_obj_p, *obj_p;
  jerry_api_object_t* external_func_p, *external_construct_p;
  jerry_api_object_t* throw_test_handler_p;
  jerry_api_value_t res, args[2];
  char buffer[32];

  is_ok = jerry_parse ((jerry_api_char_t *)test_source, strlen (test_source));
  JERRY_ASSERT (is_ok);

  is_ok = (jerry_run () == JERRY_COMPLETION_CODE_OK);
  JERRY_ASSERT (is_ok);

  global_obj_p = jerry_api_get_global ();

  // Test corner case for jerry_api_string_to_char_buffer
  test_api_init_api_value_string (&args[0], "");
  sz = jerry_api_string_to_char_buffer (args[0].v_string, NULL, 0);
  JERRY_ASSERT (sz == 0);
  jerry_api_release_value (&args[0]);

  // Get global.t
  is_ok = jerry_api_get_object_field_value (global_obj_p, (jerry_api_char_t *)"t", &val_t);
  JERRY_ASSERT (is_ok
                && val_t.type == JERRY_API_DATA_TYPE_FLOAT64
                && val_t.v_float64 == 1.0);
  jerry_api_release_value (&val_t);

  // Get global.foo
  is_ok = jerry_api_get_object_field_value (global_obj_p, (jerry_api_char_t *)"foo", &val_foo);
  JERRY_ASSERT (is_ok
                && val_foo.type == JERRY_API_DATA_TYPE_OBJECT);

  // Call foo (4, 2)
  test_api_init_api_value_float64 (&args[0], 4);
  test_api_init_api_value_float64 (&args[1], 2);
  is_ok = jerry_api_call_function (val_foo.v_object, NULL, &res, args, 2);
  JERRY_ASSERT (is_ok
                && res.type == JERRY_API_DATA_TYPE_FLOAT64
                && res.v_float64 == 1.0);
  jerry_api_release_value (&res);

  // Get global.bar
  is_ok = jerry_api_get_object_field_value (global_obj_p, (jerry_api_char_t *)"bar", &val_bar);
  JERRY_ASSERT (is_ok
                && val_bar.type == JERRY_API_DATA_TYPE_OBJECT);

  // Call bar (4, 2)
  is_ok = jerry_api_call_function (val_bar.v_object, NULL, &res, args, 2);
  JERRY_ASSERT (is_ok
                && res.type == JERRY_API_DATA_TYPE_FLOAT64
                && res.v_float64 == 5.0);
  jerry_api_release_value (&res);
  jerry_api_release_value (&val_bar);

  // Set global.t = "abcd"
  test_api_init_api_value_string (&args[0], "abcd");
  is_ok = jerry_api_set_object_field_value (global_obj_p,
                                            (jerry_api_char_t *)"t",
                                            &args[0]);
  JERRY_ASSERT (is_ok);
  jerry_api_release_value (&args[0]);

  // Call foo (4, 2)
  is_ok = jerry_api_call_function (val_foo.v_object, NULL, &res, args, 2);
  JERRY_ASSERT (is_ok
                && res.type == JERRY_API_DATA_TYPE_STRING);
  sz = jerry_api_string_to_char_buffer (res.v_string, NULL, 0);
  JERRY_ASSERT (sz == -4);
  sz = jerry_api_string_to_char_buffer (res.v_string, (jerry_api_char_t *) buffer, -sz);
  JERRY_ASSERT (sz == 4);
  jerry_api_release_value (&res);
  JERRY_ASSERT (!strncmp (buffer, "abcd", (size_t) sz));

  // Get global.A
  is_ok = jerry_api_get_object_field_value (global_obj_p, (jerry_api_char_t *)"A", &val_A);
  JERRY_ASSERT (is_ok
                && val_A.type == JERRY_API_DATA_TYPE_OBJECT);

  // Get A.prototype
  is_ok = jerry_api_is_constructor (val_A.v_object);
  JERRY_ASSERT (is_ok);
  is_ok = jerry_api_get_object_field_value (val_A.v_object,
                                            (jerry_api_char_t *) "prototype",
                                            &val_A_prototype);
  JERRY_ASSERT (is_ok
                && val_A_prototype.type == JERRY_API_DATA_TYPE_OBJECT);
  jerry_api_release_value (&val_A);

  // Set A.prototype.foo = global.foo
  is_ok = jerry_api_set_object_field_value (val_A_prototype.v_object,
                                            (jerry_api_char_t *) "foo",
                                            &val_foo);
  JERRY_ASSERT (is_ok);
  jerry_api_release_value (&val_A_prototype);
  jerry_api_release_value (&val_foo);

  // Get global.a
  is_ok = jerry_api_get_object_field_value (global_obj_p, (jerry_api_char_t *) "a", &val_a);
  JERRY_ASSERT (is_ok
                && val_a.type == JERRY_API_DATA_TYPE_OBJECT);

  // Get a.t
  is_ok = jerry_api_get_object_field_value (val_a.v_object, (jerry_api_char_t *) "t", &res);
  JERRY_ASSERT (is_ok
                && res.type == JERRY_API_DATA_TYPE_FLOAT64
                && res.v_float64 == 12.0);
  jerry_api_release_value (&res);

  // Get a.foo
  is_ok = jerry_api_get_object_field_value (val_a.v_object, (jerry_api_char_t *) "foo", &val_a_foo);
  JERRY_ASSERT (is_ok
                && val_a_foo.type == JERRY_API_DATA_TYPE_OBJECT);

  // Call a.foo ()
  is_ok = jerry_api_call_function (val_a_foo.v_object, val_a.v_object, &res, NULL, 0);
  JERRY_ASSERT (is_ok
                && res.type == JERRY_API_DATA_TYPE_FLOAT64
                && res.v_float64 == 12.0);
  jerry_api_release_value (&res);
  jerry_api_release_value (&val_a_foo);

  jerry_api_release_value (&val_a);

  // Create native handler bound function object and set it to 'external' variable
  external_func_p = jerry_api_create_external_function (handler);
  JERRY_ASSERT (external_func_p != NULL
                && jerry_api_is_function (external_func_p)
                && jerry_api_is_constructor (external_func_p));

  test_api_init_api_value_object (&val_external, external_func_p);
  is_ok = jerry_api_set_object_field_value (global_obj_p,
                                            (jerry_api_char_t *) "external",
                                            &val_external);
  JERRY_ASSERT (is_ok);
  jerry_api_release_value (&val_external);
  jerry_api_release_object (external_func_p);

  // Call 'call_external' function that should call external function created above
  is_ok = jerry_api_get_object_field_value (global_obj_p, (jerry_api_char_t *) "call_external", &val_call_external);
  JERRY_ASSERT (is_ok
                && val_call_external.type == JERRY_API_DATA_TYPE_OBJECT);
  is_ok = jerry_api_call_function (val_call_external.v_object,
                                   global_obj_p,
                                   &res,
                                   NULL, 0);
  jerry_api_release_value (&val_call_external);
  JERRY_ASSERT (is_ok
                && res.type == JERRY_API_DATA_TYPE_STRING);
  sz = jerry_api_string_to_char_buffer (res.v_string, NULL, 0);
  JERRY_ASSERT (sz == -19);
  sz = jerry_api_string_to_char_buffer (res.v_string, (jerry_api_char_t *) buffer, -sz);
  JERRY_ASSERT (sz == 19);
  jerry_api_release_value (&res);
  JERRY_ASSERT (!strncmp (buffer, "string from handler", (size_t) sz));

  // Create native handler bound function object and set it to 'external_construct' variable
  external_construct_p = jerry_api_create_external_function (handler_construct);
  JERRY_ASSERT (external_construct_p != NULL
                && jerry_api_is_function (external_construct_p)
                && jerry_api_is_constructor (external_construct_p));

  test_api_init_api_value_object (&val_external_construct, external_construct_p);
  is_ok = jerry_api_set_object_field_value (global_obj_p,
                                            (jerry_api_char_t *) "external_construct",
                                            &val_external_construct);
  JERRY_ASSERT (is_ok);
  jerry_api_release_value (&val_external_construct);
  jerry_api_release_object (external_construct_p);

  // Call external function created above, as constructor
  test_api_init_api_value_bool (&args[0], true);
  is_ok = jerry_api_construct_object (external_construct_p, &res, args, 1);
  JERRY_ASSERT (is_ok
                && res.type == JERRY_API_DATA_TYPE_OBJECT);
  is_ok = jerry_api_get_object_field_value (res.v_object,
                                            (jerry_api_char_t *)"value_field",
                                            &val_value_field);

  // Get 'value_field' of constructed object
  JERRY_ASSERT (is_ok
                && val_value_field.type == JERRY_API_DATA_TYPE_BOOLEAN
                && val_value_field.v_bool == true);
  jerry_api_release_value (&val_value_field);

  uintptr_t ptr;
  is_ok = jerry_api_get_object_native_handle (res.v_object, &ptr);
  JERRY_ASSERT (is_ok
                && ptr == (uintptr_t) 0x0012345678abcdefull);

  jerry_api_release_value (&res);


  // Test: Throwing exception from native handler.
  throw_test_handler_p = jerry_api_create_external_function (handler_throw_test);
  JERRY_ASSERT (throw_test_handler_p != NULL
                && jerry_api_is_function (throw_test_handler_p));

  test_api_init_api_value_object (&val_t, throw_test_handler_p);
  is_ok = jerry_api_set_object_field_value (global_obj_p,
                                            (jerry_api_char_t *) "throw_test",
                                            &val_t);
  JERRY_ASSERT (is_ok);
  jerry_api_release_value (&val_t);
  jerry_api_release_object (throw_test_handler_p);

  is_ok = jerry_api_get_object_field_value (global_obj_p, (jerry_api_char_t *) "call_throw_test", &val_t);
  JERRY_ASSERT (is_ok
                && val_t.type == JERRY_API_DATA_TYPE_OBJECT);

  is_ok = jerry_api_call_function (val_t.v_object,
                                   global_obj_p,
                                   &res,
                                   NULL, 0);
  JERRY_ASSERT (is_ok);
  jerry_api_release_value (&val_t);
  jerry_api_release_value (&res);

  // Test: Unhandled exception in called function
  is_ok = jerry_api_get_object_field_value (global_obj_p, (jerry_api_char_t *) "throw_reference_error", &val_t);
  JERRY_ASSERT (is_ok
                && val_t.type == JERRY_API_DATA_TYPE_OBJECT);

  is_ok = jerry_api_call_function (val_t.v_object,
                                   global_obj_p,
                                   &res,
                                   NULL, 0);
  is_exception = !is_ok;

  JERRY_ASSERT (is_exception);
  jerry_api_release_value (&val_t);

  // 'res' should contain exception object
  JERRY_ASSERT (res.type == JERRY_API_DATA_TYPE_OBJECT);
  jerry_api_release_value (&res);

  // Test: Call of non-function
  obj_p = jerry_api_create_object ();
  is_ok = jerry_api_call_function (obj_p,
                                   global_obj_p,
                                   &res,
                                   NULL, 0);
  is_exception = !is_ok;
  JERRY_ASSERT (is_exception);

  // 'res' should contain exception object
  JERRY_ASSERT (res.type == JERRY_API_DATA_TYPE_OBJECT);
  jerry_api_release_value (&res);

  jerry_api_release_object (obj_p);

  // Test: Unhandled exception in function called, as constructor
  is_ok = jerry_api_get_object_field_value (global_obj_p, (jerry_api_char_t *) "throw_reference_error", &val_t);
  JERRY_ASSERT (is_ok
                && val_t.type == JERRY_API_DATA_TYPE_OBJECT);

  is_ok = jerry_api_construct_object (val_t.v_object,
                                      &res,
                                      NULL, 0);
  is_exception = !is_ok;

  JERRY_ASSERT (is_exception);
  jerry_api_release_value (&val_t);

  // 'res' should contain exception object
  JERRY_ASSERT (res.type == JERRY_API_DATA_TYPE_OBJECT);
  jerry_api_release_value (&res);

  // Test: Call of non-function as constructor
  obj_p = jerry_api_create_object ();
  is_ok = jerry_api_construct_object (obj_p,
                                      &res,
                                      NULL, 0);
  is_exception = !is_ok;
  JERRY_ASSERT (is_exception);

  // 'res' should contain exception object
  JERRY_ASSERT (res.type == JERRY_API_DATA_TYPE_OBJECT);
  jerry_api_release_value (&res);

  jerry_api_release_object (obj_p);

  // Test: eval
  const char *eval_code_src_p = "(function () { return 123; })";
  jerry_completion_code_t status = jerry_api_eval ((jerry_api_char_t *) eval_code_src_p,
                                                   strlen (eval_code_src_p),
                                                   false,
                                                   true,
                                                   &val_t);
  JERRY_ASSERT (status == JERRY_COMPLETION_CODE_OK);
  JERRY_ASSERT (val_t.type == JERRY_API_DATA_TYPE_OBJECT);
  JERRY_ASSERT (jerry_api_is_function (val_t.v_object));

  is_ok = jerry_api_call_function (val_t.v_object,
                                   NULL,
                                   &res,
                                   NULL, 0);
  JERRY_ASSERT (is_ok);
  JERRY_ASSERT (res.type == JERRY_API_DATA_TYPE_FLOAT64
                && res.v_float64 == 123.0);
  jerry_api_release_value (&res);

  jerry_api_release_value (&val_t);

  // cleanup.
  jerry_api_release_object (global_obj_p);

  jerry_cleanup ();

  JERRY_ASSERT (test_api_is_free_callback_was_called);

  // External Magic String
  jerry_init (JERRY_FLAG_SHOW_OPCODES);

  uint32_t num_magic_string_items = (uint32_t) (sizeof (magic_string_items) / sizeof (jerry_api_char_ptr_t));
  jerry_register_external_magic_strings (magic_string_items,
                                         num_magic_string_items,
                                         magic_string_lengths);

  const char *ms_code_src_p = "var global = {}; var console = [1]; var process = 1;";
  is_ok = jerry_parse ((jerry_api_char_t *) ms_code_src_p, strlen (ms_code_src_p));
  JERRY_ASSERT (is_ok);

  is_ok = (jerry_run () == JERRY_COMPLETION_CODE_OK);
  JERRY_ASSERT (is_ok);

  jerry_cleanup ();

  return 0;
}

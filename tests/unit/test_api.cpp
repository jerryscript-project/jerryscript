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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jerry.h"
#include "jerry-api.h"

const char *test_source = (
                           "this.t = 1; "
                           "function f () { "
                           "return this.t; "
                           "} "
                           "this.foo = f; "
                           "this.bar = function (a) { "
                           "return a + t; "
                           "} "
                           "function A () { "
                           "this.t = 12; "
                           "} "
                           "this.A = A; "
                           "this.a = new A (); "
                           "function call_external () { "
                           "  return this.external ('1', true); "
                           "} "
                           "function call_throw_test() { "
                           "  bool catched = false "
                           "  try { "
                           "    this.throw_test(); "
                           "  } catch (e) { "
                           "    catched = true; "
                           "    assert(e.name == 'TypeError'); "
                           "    assert(e.message == 'error'); "
                           "  } "
                           "  assert(catched); "
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
  out_value_p->v_string = jerry_api_create_string (v);
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
         const uint16_t args_cnt)
{
  char buffer[32];
  ssize_t sz;

  printf ("ok %p %p %p %d %p\n", function_obj_p, this_p, args_p, args_cnt, ret_val_p);

  assert (args_cnt == 2);

  assert (args_p[0].type == JERRY_API_DATA_TYPE_STRING);
  sz = jerry_api_string_to_char_buffer (args_p[0].v_string, NULL, 0);
  assert (sz == -2);
  sz = jerry_api_string_to_char_buffer (args_p[0].v_string, buffer, -sz);
  assert (sz == 2);
  assert (!strcmp (buffer, "1"));

  assert (args_p[1].type == JERRY_API_DATA_TYPE_BOOLEAN);
  assert (args_p[1].v_bool == true);

  test_api_init_api_value_string (ret_val_p, "string from handler");

  return true;
} /* handler */

static bool
handler_throw_test (const jerry_api_object_t *function_obj_p,
                    const jerry_api_value_t *this_p,
                    jerry_api_value_t *ret_val_p,
                    const jerry_api_value_t args_p[],
                    const uint16_t args_cnt)
{
  printf ("ok %p %p %p %d %p\n", function_obj_p, this_p, args_p, args_cnt, ret_val_p);

  jerry_api_object_t* error_p = jerry_api_create_error (JERRY_API_ERROR_TYPE, "error");

  test_api_init_api_value_object (ret_val_p, error_p);

  jerry_api_release_object (error_p);

  return false;
}

static void
handler_construct_freecb (uintptr_t native_p)
{
  assert (native_p == (uintptr_t) 0x0012345678abcdefull);
  printf ("ok object free callback\n");

  test_api_is_free_callback_was_called = true;
} /* handler_construct_freecb */

static bool
handler_construct (const jerry_api_object_t *function_obj_p,
                   const jerry_api_value_t *this_p,
                   jerry_api_value_t *ret_val_p,
                   const jerry_api_value_t args_p[],
                   const uint16_t args_cnt)
{
  printf ("ok construct %p %p %p %d %p\n", function_obj_p, this_p, args_p, args_cnt, ret_val_p);

  assert (this_p != NULL);
  assert (this_p->type == JERRY_API_DATA_TYPE_OBJECT);

  assert (args_cnt == 1);
  assert (args_p[0].type == JERRY_API_DATA_TYPE_BOOLEAN);
  assert (args_p[0].v_bool == true);

  jerry_api_set_object_field_value (this_p->v_object, "value_field", &args_p[0]);

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
  jerry_init (JERRY_FLAG_EMPTY);

  bool is_ok;
  ssize_t sz;
  jerry_api_value_t val_t, val_foo, val_bar, val_A, val_A_prototype, val_a, val_a_foo, val_value_field;
  jerry_api_value_t val_external, val_external_construct, val_call_external;
  jerry_api_object_t* global_obj_p;
  jerry_api_object_t* external_func_p, *external_construct_p;
  jerry_api_object_t* throw_test_handler_p;
  jerry_api_value_t res, args[2];
  char buffer[32];

  is_ok = jerry_parse (test_source, strlen (test_source));
  assert (is_ok);

  is_ok = (jerry_run () == JERRY_COMPLETION_CODE_OK);
  assert (is_ok);

  global_obj_p = jerry_api_get_global ();

  // Get global.t
  is_ok = jerry_api_get_object_field_value (global_obj_p, "t", &val_t);
  assert (is_ok
          && val_t.type == JERRY_API_DATA_TYPE_FLOAT64
          && val_t.v_float64 == 1.0);
  jerry_api_release_value (&val_t);

  // Get global.foo
  is_ok = jerry_api_get_object_field_value (global_obj_p, "foo", &val_foo);
  assert (is_ok
          && val_foo.type == JERRY_API_DATA_TYPE_OBJECT);

  // Call foo (4, 2)
  test_api_init_api_value_float64 (&args[0], 4);
  test_api_init_api_value_float64 (&args[1], 2);
  is_ok = jerry_api_call_function (val_foo.v_object, NULL, &res, args, 2);
  assert (is_ok
          && res.type == JERRY_API_DATA_TYPE_FLOAT64
          && res.v_float64 == 1.0);
  jerry_api_release_value (&res);

  // Get global.bar
  is_ok = jerry_api_get_object_field_value (global_obj_p, "bar", &val_bar);
  assert (is_ok
          && val_bar.type == JERRY_API_DATA_TYPE_OBJECT);

  // Call bar (4, 2)
  is_ok = jerry_api_call_function (val_bar.v_object, NULL, &res, args, 2);
  assert (is_ok
          && res.type == JERRY_API_DATA_TYPE_FLOAT64
          && res.v_float64 == 5.0);
  jerry_api_release_value (&res);
  jerry_api_release_value (&val_bar);

  // Set global.t = "abcd"
  test_api_init_api_value_string (&args[0], "abcd");
  is_ok = jerry_api_set_object_field_value (global_obj_p,
                                            "t",
                                            &args[0]);
  assert (is_ok);
  jerry_api_release_value (&args[0]);

  // Call foo (4, 2)
  is_ok = jerry_api_call_function (val_foo.v_object, NULL, &res, args, 2);
  assert (is_ok
          && res.type == JERRY_API_DATA_TYPE_STRING);
  sz = jerry_api_string_to_char_buffer (res.v_string, NULL, 0);
  assert (sz == -5);
  sz = jerry_api_string_to_char_buffer (res.v_string, buffer, -sz);
  assert (sz == 5);
  jerry_api_release_value (&res);
  assert (!strcmp (buffer, "abcd"));

  // Get global.A
  is_ok = jerry_api_get_object_field_value (global_obj_p, "A", &val_A);
  assert (is_ok
          && val_A.type == JERRY_API_DATA_TYPE_OBJECT);

  // Get A.prototype
  is_ok = jerry_api_is_constructor (val_A.v_object);
  assert (is_ok);
  is_ok = jerry_api_get_object_field_value (val_A.v_object,
                                            "prototype",
                                            &val_A_prototype);
  assert (is_ok
          && val_A_prototype.type == JERRY_API_DATA_TYPE_OBJECT);
  jerry_api_release_value (&val_A);

  // Set A.prototype.foo = global.foo
  is_ok = jerry_api_set_object_field_value (val_A_prototype.v_object,
                                            "foo",
                                            &val_foo);
  assert (is_ok);
  jerry_api_release_value (&val_A_prototype);
  jerry_api_release_value (&val_foo);

  // Get global.a
  is_ok = jerry_api_get_object_field_value (global_obj_p, "a", &val_a);
  assert (is_ok
          && val_a.type == JERRY_API_DATA_TYPE_OBJECT);

  // Get a.t
  is_ok = jerry_api_get_object_field_value (val_a.v_object, "t", &res);
  assert (is_ok
          && res.type == JERRY_API_DATA_TYPE_FLOAT64
          && res.v_float64 == 12.0);
  jerry_api_release_value (&res);

  // Get a.foo
  is_ok = jerry_api_get_object_field_value (val_a.v_object, "foo", &val_a_foo);
  assert (is_ok
          && val_a_foo.type == JERRY_API_DATA_TYPE_OBJECT);

  // Call a.foo ()
  is_ok = jerry_api_call_function (val_a_foo.v_object, val_a.v_object, &res, NULL, 0);
  assert (is_ok
          && res.type == JERRY_API_DATA_TYPE_FLOAT64
          && res.v_float64 == 12.0);
  jerry_api_release_value (&res);
  jerry_api_release_value (&val_a_foo);

  jerry_api_release_value (&val_a);

  // Create native handler bound function object and set it to 'external' variable
  external_func_p = jerry_api_create_external_function (handler);
  assert (external_func_p != NULL
          && jerry_api_is_function (external_func_p)
          && jerry_api_is_constructor (external_func_p));

  test_api_init_api_value_object (&val_external, external_func_p);
  is_ok = jerry_api_set_object_field_value (global_obj_p,
                                            "external",
                                            &val_external);
  assert (is_ok);
  jerry_api_release_value (&val_external);
  jerry_api_release_object (external_func_p);

  // Call 'call_external' function that should call external function created above
  is_ok = jerry_api_get_object_field_value (global_obj_p, "call_external", &val_call_external);
  assert (is_ok
          && val_call_external.type == JERRY_API_DATA_TYPE_OBJECT);
  is_ok = jerry_api_call_function (val_call_external.v_object,
                                   global_obj_p,
                                   &res,
                                   NULL, 0);
  jerry_api_release_value (&val_call_external);
  assert (is_ok
          && res.type == JERRY_API_DATA_TYPE_STRING);
  sz = jerry_api_string_to_char_buffer (res.v_string, NULL, 0);
  assert (sz == -20);
  sz = jerry_api_string_to_char_buffer (res.v_string, buffer, -sz);
  assert (sz == 20);
  jerry_api_release_value (&res);
  assert (!strcmp (buffer, "string from handler"));

  // Create native handler bound function object and set it to 'external_construct' variable
  external_construct_p = jerry_api_create_external_function (handler_construct);
  assert (external_construct_p != NULL
          && jerry_api_is_function (external_construct_p)
          && jerry_api_is_constructor (external_construct_p));

  test_api_init_api_value_object (&val_external_construct, external_construct_p);
  is_ok = jerry_api_set_object_field_value (global_obj_p,
                                            "external_construct",
                                            &val_external_construct);
  assert (is_ok);
  jerry_api_release_value (&val_external_construct);
  jerry_api_release_object (external_construct_p);

  // Call external function created above, as constructor
  test_api_init_api_value_bool (&args[0], true);
  is_ok = jerry_api_construct_object (external_construct_p, &res, args, 1);
  assert (is_ok
          && res.type == JERRY_API_DATA_TYPE_OBJECT);
  is_ok = jerry_api_get_object_field_value (res.v_object,
                                            "value_field",
                                            &val_value_field);

  // Get 'value_field' of constructed object
  assert (is_ok
          && val_value_field.type == JERRY_API_DATA_TYPE_BOOLEAN
          && val_value_field.v_bool == true);
  jerry_api_release_value (&val_value_field);

  uintptr_t ptr;
  is_ok = jerry_api_get_object_native_handle (res.v_object, &ptr);
  assert (is_ok
          && ptr == (uintptr_t) 0x0012345678abcdefull);

  jerry_api_release_value (&res);


  // Test: Throwing exception from native handler.
  throw_test_handler_p = jerry_api_create_external_function (handler_throw_test);
  assert (throw_test_handler_p != NULL
          && jerry_api_is_function (throw_test_handler_p));

  test_api_init_api_value_object (&val_t, throw_test_handler_p);
  is_ok = jerry_api_set_object_field_value (global_obj_p,
                                            "throw_test",
                                            &val_t);
  assert (is_ok);
  jerry_api_release_value (&val_t);
  jerry_api_release_object (throw_test_handler_p);

  is_ok = jerry_api_get_object_field_value (global_obj_p, "call_throw_test", &val_t);
  assert (is_ok
          && val_t.type == JERRY_API_DATA_TYPE_OBJECT);

  is_ok = jerry_api_call_function (val_t.v_object,
                                   global_obj_p,
                                   &res,
                                   NULL, 0);
  assert (is_ok);
  jerry_api_release_value (&val_t);
  jerry_api_release_value (&res);


  // cleanup.
  jerry_api_release_object (global_obj_p);

  jerry_cleanup ();

  assert (test_api_is_free_callback_was_called);

  // External Magic String
  jerry_init (JERRY_FLAG_SHOW_OPCODES);

  uint32_t num_magic_string_items = sizeof (magic_string_items) / sizeof (jerry_api_char_ptr_t);
  jerry_register_external_magic_strings (magic_string_items,
                                         num_magic_string_items,
                                         magic_string_lengths);

  const char *ms_code_src_p = "var global = {}; var console = [1]; var process = 1;";
  is_ok = jerry_parse (ms_code_src_p, strlen (ms_code_src_p));
  assert (is_ok);

  is_ok = (jerry_run () == JERRY_COMPLETION_CODE_OK);
  assert (is_ok);

  jerry_cleanup ();

  return 0;
}

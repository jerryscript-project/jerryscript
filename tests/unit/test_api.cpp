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
                           );

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

int
main (void)
{
  jerry_init (JERRY_FLAG_EMPTY);

  bool is_ok;
  ssize_t sz;
  jerry_api_value_t val_t, val_foo, val_bar, val_A, val_A_prototype, val_a, val_a_foo;
  jerry_api_object_t* global_obj_p;
  jerry_api_value_t res, args [2];
  char buffer [16];

  is_ok = jerry_parse (NULL, test_source, strlen (test_source));
  assert (is_ok);

  is_ok = (jerry_run (NULL) == JERRY_COMPLETION_CODE_OK);
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

  jerry_api_release_object (global_obj_p);

  jerry_cleanup ();

  return 0;
}

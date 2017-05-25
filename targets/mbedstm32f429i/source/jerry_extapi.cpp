/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#include <stdlib.h>
#include <stdio.h>

#include "jerry-core/jerry.h"
#include "jerry_extapi.h"
 
#include "native_mbedstm32f429i.h"

#ifndef MIN
#define MIN(A,B) ((A)<(B)?(A):(B))
#endif

//-----------------------------------------------------------------------------

#define __UNSED__ __attribute__((unused))

#define DECLARE_HANDLER(NAME) \
static bool \
NAME ## _handler (const jerry_api_object_t * function_obj_p __UNSED__, \
                  const jerry_api_value_t *  this_p __UNSED__, \
                  jerry_api_value_t *        ret_val_p __UNSED__, \
                  const jerry_api_value_t    args_p[], \
                  const jerry_api_length_t   args_cnt)

#define REGISTER_HANDLER(NAME) \
  register_native_function ( # NAME, NAME ## _handler)

//-----------------------------------------------------------------------------

DECLARE_HANDLER(assert)
{
  if (args_cnt == 1
      && args_p[0].type == JERRY_API_DATA_TYPE_BOOLEAN
      && args_p[0].u.v_bool == true)
  {
    printf (">> Jerry assert true\r\n");
    return true;
  }
  printf ("ERROR: Script assertion failed\n");
  exit (JERRY_STANDALONE_EXIT_CODE_FAIL);
  return false;
}

DECLARE_HANDLER(led)
{
  if (args_cnt < 2)
  {
    return false;
  }

  int port, value;
  port = (int)JS_VALUE_TO_NUMBER (&args_p[0]);
  value = (int)JS_VALUE_TO_NUMBER (&args_p[1]);

  ret_val_p->type = JERRY_API_DATA_TYPE_BOOLEAN;
  if (port >=0 && port <= 3)
  {
    native_led(port, value);
    ret_val_p->u.v_bool = true;
  }
  else
  {
    ret_val_p->u.v_bool = false;
  }
  return true;
}

//-----------------------------------------------------------------------------

static bool
register_native_function (const char* name,
                          jerry_external_handler_t handler)
{
  jerry_api_object_t *global_obj_p;
  jerry_api_object_t *reg_func_p;
  jerry_api_value_t reg_value;
  bool bok;

  global_obj_p = jerry_api_get_global ();
  reg_func_p = jerry_api_create_external_function (handler);

  if (!(reg_func_p != NULL
      && jerry_api_is_function (reg_func_p)
      && jerry_api_is_constructor (reg_func_p)))
  {
    printf ("Error: create_external_function failed !!!\r\n");
    jerry_api_release_object (global_obj_p);
    return false;
  }

  jerry_api_acquire_object (reg_func_p);
  reg_value.type = JERRY_API_DATA_TYPE_OBJECT;
  reg_value.u.v_object = reg_func_p;

  bok = jerry_api_set_object_field_value (global_obj_p,
                                          (jerry_api_char_t *) name,
                                          &reg_value);

  jerry_api_release_value (&reg_value);
  jerry_api_release_object (reg_func_p);
  jerry_api_release_object (global_obj_p);

  if (!bok)
  {
    printf ("Error: register_native_function failed: [%s]\r\n", name);
  }

  return bok;
}

void js_register_functions (void)
{
  REGISTER_HANDLER (assert);
  REGISTER_HANDLER (led);
}

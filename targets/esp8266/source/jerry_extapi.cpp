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

#include <stdlib.h>
#include <stdio.h>

#include "jerry-core/jerry.h"
#include "jerry-core/jrt/jrt.h"  /* for JERRY_ERROR_MSG */
#include "jerry_extapi.h"

#include "native_esp8266.h"



#ifndef MIN
#define MIN(A,B) ((A)<(B)?(A):(B))
#endif


#define __UNSED__ __attribute__((unused))

#define DELCARE_HANDLER(NAME) \
static bool \
NAME ## _handler (const jerry_api_object_t * function_obj_p __UNSED__, \
                  const jerry_api_value_t *  this_p __UNSED__, \
                  jerry_api_value_t *        ret_val_p __UNSED__, \
                  const jerry_api_value_t    args_p[], \
                  const jerry_api_length_t   args_cnt)

#define REGISTER_HANDLER(NAME) \
  register_native_function ( # NAME, NAME ## _handler)

//-----------------------------------------------------------------------------

DELCARE_HANDLER(assert) {
  if (args_cnt == 1
      && args_p[0].type == JERRY_API_DATA_TYPE_BOOLEAN
      && args_p[0].v_bool == true)
  {
    printf (">> Jerry assert true\r\n");
    return true;
  }
  JERRY_ERROR_MSG ("Script assertion failed\n");
  exit (JERRY_STANDALONE_EXIT_CODE_FAIL);
  return false;
}


DELCARE_HANDLER(print) {
  jerry_api_length_t cc;

  if (args_cnt)
  {
    printf(">> print(%d) :", (int)args_cnt);
    for (cc=0; cc<args_cnt; cc++)
    {
      if (args_p[cc].type == JERRY_API_DATA_TYPE_STRING && args_p[cc].v_string)
      {
        static char buffer[128];
        int length, maxlength;
        length = -jerry_api_string_to_char_buffer (args_p[0].v_string, NULL, 0);
        maxlength  = MIN(length, 126);
        jerry_api_string_to_char_buffer (args_p[cc].v_string,
                                         (jerry_api_char_t *) buffer,
                                         maxlength);
        *(buffer + length) = 0;
        printf("[%s] ", buffer);
      }
      else
      {
        printf ("(%d) ", args_p[cc].type);
      }
    }
    printf ("\r\n");
  }
  return true;
}


//-----------------------------------------------------------------------------

DELCARE_HANDLER(gpio_dir) {
  if (args_cnt < 2)
  {
    return false;
  }

  int port, value;
  port = (int)JS_VALUE_TO_NUMBER (&args_p[0]);
  value = (int)JS_VALUE_TO_NUMBER (&args_p[1]);

  native_gpio_dir (port, value);

  return true;
} /* gpio_dir_handler */

DELCARE_HANDLER(gpio_set) {
  if (args_cnt < 2)
  {
    return false;
  }

  int port, value;
  port = (int)JS_VALUE_TO_NUMBER (&args_p[0]);
  value = (int)JS_VALUE_TO_NUMBER (&args_p[1]);

  native_gpio_set (port, value);

  return true;
} /* gpio_dir_handler */


DELCARE_HANDLER(gpio_get) {
  if (args_cnt < 1)
  {
    return false;
  }

  int port, value;
  port = (int)JS_VALUE_TO_NUMBER (&args_p[0]);

  value = native_gpio_get (port) ? 1 : 0;

  ret_val_p->type = JERRY_API_DATA_TYPE_FLOAT64;
  ret_val_p->v_float64 = (double)value;

  return true;
} /* gpio_dir_handler */


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
    printf ("!!! create_external_function failed !!!\r\n");
    jerry_api_release_object (global_obj_p);
    return false;
  }

  jerry_api_acquire_object (reg_func_p);
  reg_value.type = JERRY_API_DATA_TYPE_OBJECT;
  reg_value.v_object = reg_func_p;

  bok = jerry_api_set_object_field_value (global_obj_p,
                                          (jerry_api_char_t *)name,
                                          &reg_value);

  jerry_api_release_value (&reg_value);
  jerry_api_release_object (reg_func_p);
  jerry_api_release_object (global_obj_p);

  if (!bok)
  {
    printf ("!!! register_native_function failed: [%s]\r\n", name);
  }

  return bok;
}


//-----------------------------------------------------------------------------

void js_register_functions (void)
{
  REGISTER_HANDLER(assert);
  REGISTER_HANDLER(print);
  REGISTER_HANDLER(gpio_dir);
  REGISTER_HANDLER(gpio_set);
  REGISTER_HANDLER(gpio_get);
}

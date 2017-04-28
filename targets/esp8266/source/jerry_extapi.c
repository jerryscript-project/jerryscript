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

#include <stdlib.h>
#include <stdio.h>

#include "jerry-core/include/jerryscript.h"
#include "jerry_extapi.h"

#include "native_esp8266.h"



#ifndef MIN
#define MIN(A,B) ((A)<(B)?(A):(B))
#endif


#define __UNSED__ __attribute__((unused))

#define DELCARE_HANDLER(NAME) \
static jerry_value_t \
NAME ## _handler (const jerry_value_t  function_obj_val __UNSED__, \
                  const jerry_value_t  this_val __UNSED__, \
                  const jerry_value_t  args_p[], \
                  const jerry_length_t  args_cnt)

#define REGISTER_HANDLER(NAME) \
  register_native_function ( # NAME, NAME ## _handler)

/*---------------------------------------------------------------------------*/

DELCARE_HANDLER(assert) {
  if (args_cnt == 1
      && jerry_value_is_boolean (args_p[0])
      && jerry_get_boolean_value (args_p[0]))
  {
    printf (">> Jerry assert true\r\n");
    return jerry_create_boolean (true);
  }
  printf ("Script assertion failed\n");
  exit (JERRY_STANDALONE_EXIT_CODE_FAIL);
  return jerry_create_boolean (false);
}


DELCARE_HANDLER(print) {
  jerry_length_t cc;

  if (args_cnt)
  {
    printf(">> print(%d) :", (int) args_cnt);
    for (cc=0; cc<args_cnt; cc++)
    {
      if (jerry_value_is_string (args_p[cc]))
      {
        char *buffer;
        jerry_size_t size = jerry_get_string_size (args_p[0]);
        buffer = (char *) malloc(size + 1);

        if(!buffer)
        {
            // not enough memory for this string.
            printf("[<too-long-string>]");
            continue;
        }

        jerry_string_to_char_buffer (args_p[cc],
                                     (jerry_char_t *) buffer,
                                     size);
        *(buffer + size) = 0;
        printf("[%s] ", buffer);
        free (buffer);
      }
      else
      {
        printf ("(%d) ", args_p[cc]);
      }
    }
    printf ("\r\n");
  }
  return jerry_create_boolean (true);
}


/*---------------------------------------------------------------------------*/

DELCARE_HANDLER(gpio_dir) {
  int port, value;
  if (args_cnt < 2)
  {
    return false;
  }

  port = (int) jerry_get_number_value (args_p[0]);
  value = (int) jerry_get_number_value (args_p[1]);

  native_gpio_dir (port, value);

  return jerry_create_boolean (true);
} /* gpio_dir_handler */

DELCARE_HANDLER(gpio_set) {
  int port, value;
  if (args_cnt < 2)
  {
    return jerry_create_boolean (false);
  }

  port = (int) jerry_get_number_value (args_p[0]);
  value = (int) jerry_get_number_value (args_p[1]);

  native_gpio_set (port, value);

  return jerry_create_boolean (true);
} /* gpio_dir_handler */


DELCARE_HANDLER(gpio_get) {
  int port, value;
  if (args_cnt < 1)
  {
    return false;
  }

  port = (int) jerry_get_number_value (args_p[0]);

  value = native_gpio_get (port) ? 1 : 0;

  return jerry_create_number ((double) value);
} /* gpio_dir_handler */


/*---------------------------------------------------------------------------*/

static bool
register_native_function (const char* name,
                          jerry_external_handler_t handler)
{
  jerry_value_t global_obj_val;
  jerry_value_t reg_func_val;
  jerry_value_t prop_name_val;
  jerry_value_t res;
  bool bok;

  global_obj_val = jerry_get_global_object ();
  reg_func_val = jerry_create_external_function (handler);
  bok = true;

  if (!(jerry_value_is_function (reg_func_val)
        && jerry_value_is_constructor (reg_func_val)))
  {
    printf ("!!! create_external_function failed !!!\r\n");
    jerry_release_value (reg_func_val);
    jerry_release_value (global_obj_val);
    return false;
  }

  prop_name_val = jerry_create_string ((const jerry_char_t *) name);
  res = jerry_set_property (global_obj_val, prop_name_val, reg_func_val);

  if (jerry_value_has_error_flag (res))
  {
    bok = false;
  }

  jerry_release_value (res);
  jerry_release_value (prop_name_val);
  jerry_release_value (reg_func_val);
  jerry_release_value (global_obj_val);

  if (!bok)
  {
    printf ("!!! register_native_function failed: [%s]\r\n", name);
  }

  return bok;
}


/*---------------------------------------------------------------------------*/

void js_register_functions (void)
{
  REGISTER_HANDLER(assert);
  REGISTER_HANDLER(print);
  REGISTER_HANDLER(gpio_dir);
  REGISTER_HANDLER(gpio_set);
  REGISTER_HANDLER(gpio_get);
}

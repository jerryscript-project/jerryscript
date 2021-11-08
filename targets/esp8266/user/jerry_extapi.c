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

#include "jerry_extapi.h"

#include <stdio.h>
#include <stdlib.h>

#include "jerryscript.h"

#include "c_types.h"
#include "gpio.h"

#define __UNUSED__ __attribute__ ((unused))

#define DELCARE_HANDLER(NAME)                                                           \
  static jerry_value_t NAME##_handler (const jerry_call_info_t *call_info_p __UNUSED__, \
                                       const jerry_value_t args_p[],                    \
                                       const jerry_length_t args_cnt)

#define REGISTER_HANDLER(NAME) register_native_function (#NAME, NAME##_handler)

DELCARE_HANDLER (assert)
{
  if (args_cnt == 1 && jerry_value_is_true (args_p[0]))
  {
    printf (">> Jerry assert true\r\n");
    return jerry_boolean (true);
  }
  printf ("Script assertion failed\n");
  exit (JERRY_STANDALONE_EXIT_CODE_FAIL);
  return jerry_boolean (false);
} /* assert */

DELCARE_HANDLER (print)
{
  if (args_cnt)
  {
    for (jerry_length_t cc = 0; cc < args_cnt; cc++)
    {
      if (jerry_value_is_string (args_p[cc]))
      {
        jerry_size_t size = jerry_string_size (args_p[0], JERRY_ENCODING_UTF8);
        char *buffer;
        buffer = (char *) malloc (size + 1);

        if (!buffer)
        {
          // not enough memory for this string.
          printf ("[<too-long-string>]");
          continue;
        }

        jerry_string_to_buffer (args_p[cc], JERRY_ENCODING_UTF8, (jerry_char_t *) buffer, size);
        buffer[size] = '\0';
        printf ("%s ", buffer);
        free (buffer);
      }
      else if (jerry_value_is_number (args_p[cc]))
      {
        double number = jerry_value_as_number (args_p[cc]);
        if ((int) number == number)
        {
          printf ("%d", (int) number);
        }
        else
        {
          char buff[50];
          sprintf (buff, "%.10f", number);
          printf ("%s", buff);
        }
      }
    }
    printf ("\r\n");
  }
  return jerry_boolean (true);
} /* print */

DELCARE_HANDLER (gpio_dir)
{
  if (args_cnt < 2)
  {
    return jerry_boolean (false);
  }

  int port = (int) jerry_value_as_number (args_p[0]);
  int value = (int) jerry_value_as_number (args_p[1]);

  if (value)
  {
    GPIO_AS_OUTPUT (1 << port);
  }
  else
  {
    GPIO_AS_INPUT (1 << port);
  }

  return jerry_boolean (true);
} /* gpio_dir */

DELCARE_HANDLER (gpio_set)
{
  if (args_cnt < 2)
  {
    return jerry_boolean (false);
  }

  int port = (int) jerry_value_as_number (args_p[0]);
  int value = (int) jerry_value_as_number (args_p[1]);

  GPIO_OUTPUT_SET (port, value);

  return jerry_boolean (true);
} /* gpio_set */

DELCARE_HANDLER (gpio_get)
{
  if (args_cnt < 1)
  {
    return jerry_boolean (false);
  }

  int port = (int) jerry_value_as_number (args_p[0]);
  int value = GPIO_INPUT_GET (port) ? 1 : 0;

  return jerry_number ((double) value);
} /* gpio_get */

static bool
register_native_function (const char *name, jerry_external_handler_t handler)
{
  jerry_value_t global_obj_val = jerry_current_realm ();
  jerry_value_t reg_func_val = jerry_function_external (handler);

  if (!(jerry_value_is_function (reg_func_val) && jerry_value_is_constructor (reg_func_val)))
  {
    printf ("!!! create_external_function failed !!!\r\n");
    jerry_value_free (reg_func_val);
    jerry_value_free (global_obj_val);
    return false;
  }

  jerry_value_t prop_name_val = jerry_string_sz (name);
  jerry_value_t res = jerry_object_set (global_obj_val, prop_name_val, reg_func_val);

  jerry_value_free (reg_func_val);
  jerry_value_free (global_obj_val);
  jerry_value_free (prop_name_val);

  if (jerry_value_is_exception (res))
  {
    printf ("!!! register_native_function failed: [%s]\r\n", name);
    jerry_value_free (res);
    return false;
  }

  jerry_value_free (res);

  return true;
} /* register_native_function */

void
js_register_functions (void)
{
  REGISTER_HANDLER (assert);
  REGISTER_HANDLER (print);
  REGISTER_HANDLER (gpio_dir);
  REGISTER_HANDLER (gpio_set);
  REGISTER_HANDLER (gpio_get);
} /* js_register_functions */

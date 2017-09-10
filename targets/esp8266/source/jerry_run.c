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
#include "jerry_run.h"


static const char* fn_sys_loop_name = "sysloop";
jerry_value_t parsed_res;


/*---------------------------------------------------------------------------*/

int js_entry (const char *source_p, const size_t source_size)
{
  const jerry_char_t *jerry_src = (const jerry_char_t *) source_p;
  int ret_code = 0; /* JERRY_COMPLETION_CODE_OK */
  jerry_init_flag_t flags = JERRY_INIT_EMPTY;

  jerry_init (flags);

  js_register_functions ();

  parsed_res = jerry_parse ((jerry_char_t *) jerry_src, source_size, false);

  if (jerry_value_has_error_flag (parsed_res))
  {
    printf ("Error: jerry_parse failed\r\n");
    ret_code = JERRY_ERROR_SYNTAX;
  }

  return ret_code;
}

int js_eval (const char *source_p, const size_t source_size)
{
  int status = 0;
  jerry_value_t res;

  res = jerry_eval ((jerry_char_t *) source_p,
                           source_size,
                           false);
  if (jerry_value_has_error_flag (res)) {
	  status = -1;
  }

  jerry_release_value (res);

  return status;
}

int js_loop (uint32_t ticknow)
{
  jerry_value_t global_obj_val;
  jerry_value_t sysloop_func;
  jerry_value_t val_args[1];
  uint16_t val_argv;
  jerry_value_t res;
  jerry_value_t prop_name_val;
  int ret_code = 0;

  global_obj_val = jerry_get_global_object ();
  prop_name_val = jerry_create_string ((const jerry_char_t *) fn_sys_loop_name);
  sysloop_func = jerry_get_property (global_obj_val, prop_name_val);
  jerry_release_value (prop_name_val);

  if (jerry_value_has_error_flag (sysloop_func)) {
    printf ("Error: '%s' not defined!!!\r\n", fn_sys_loop_name);
    jerry_release_value (sysloop_func);
    jerry_release_value (global_obj_val);
    return -1;
  }

  if (!jerry_value_is_function (sysloop_func)) {
    printf ("Error: '%s' is not a function!!!\r\n", fn_sys_loop_name);
    jerry_release_value (sysloop_func);
    jerry_release_value (global_obj_val);
    return -2;
  }

  val_argv = 1;
  val_args[0] = jerry_create_number (ticknow);

  res = jerry_call_function (sysloop_func,
                             global_obj_val,
                             val_args,
                             val_argv);

  if (jerry_value_has_error_flag (res)) {
    ret_code = -3;
  }

  jerry_release_value (res);
  jerry_release_value (sysloop_func);
  jerry_release_value (global_obj_val);

  return ret_code;
}

void js_exit (void)
{
  jerry_cleanup ();
}

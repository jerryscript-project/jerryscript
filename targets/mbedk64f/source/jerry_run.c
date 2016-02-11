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

#include "jerry-core/jerry.h"

#include <stdlib.h>
#include <stdio.h>

#include "jerry_run.h"
#include "jerry_extapi.h"


static const char* fn_sys_loop_name = "sysloop";


int js_entry (const char *source_p, const size_t source_size)
{
  const jerry_api_char_t *jerry_src = (const jerry_api_char_t *) source_p;
  jerry_completion_code_t ret_code = JERRY_COMPLETION_CODE_OK;
  jerry_flag_t flags = JERRY_FLAG_EMPTY;

  jerry_init (flags);

  js_register_functions ();

  if (!jerry_parse (jerry_src, source_size))
  {
    printf ("Error: jerry_parse failed\r\n");
    ret_code = JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION;
  }
  else
  {
    if ((flags & JERRY_FLAG_PARSE_ONLY) == 0)
    {
      ret_code = jerry_run ();
    }
  }

  return ret_code;
}

int js_eval (const char *source_p, const size_t source_size)
{
  jerry_completion_code_t status;
  jerry_api_value_t res;

  status = jerry_api_eval ((jerry_api_char_t *) source_p,
                           source_size,
                           false,
                           false,
                           &res);

  jerry_api_release_value (&res);

  return status;
}

int js_loop (uint32_t ticknow)
{
  jerry_api_object_t *global_obj_p;
  jerry_api_value_t sysloop_func;
  bool is_ok;

  global_obj_p = jerry_api_get_global ();
  is_ok = jerry_api_get_object_field_value (global_obj_p,
                          (const jerry_api_char_t*)fn_sys_loop_name,
                          &sysloop_func);
  if (!is_ok)
  {
    printf ("Error: '%s' not defined!!!\r\n", fn_sys_loop_name);
    jerry_api_release_object (global_obj_p);
    return -1;
  }

  if (!API_DATA_IS_FUNCTION (&sysloop_func))
  {
    printf ("Error: '%s' is not a function!!!\r\n", fn_sys_loop_name);
    jerry_api_release_value (&sysloop_func);
    jerry_api_release_object (global_obj_p);
    return -2;
  }

  jerry_api_value_t* val_args;
  uint16_t val_argv;

  val_argv = 1;
  val_args = (jerry_api_value_t*)malloc (sizeof (jerry_api_value_t) * val_argv);
  val_args[0].type = JERRY_API_DATA_TYPE_UINT32;
  val_args[0].v_uint32 = ticknow;

  jerry_api_value_t res;
  is_ok = jerry_api_call_function (sysloop_func.v_object,
                                   global_obj_p,
                                   &res,
                                   val_args,
                                   val_argv);
  jerry_api_release_value (&res);
  free (val_args);

  jerry_api_release_value (&sysloop_func);
  jerry_api_release_object (global_obj_p);

  return 0;
}

void js_exit (void)
{
  jerry_cleanup ();
}


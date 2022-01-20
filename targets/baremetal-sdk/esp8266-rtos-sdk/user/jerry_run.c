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

#include "jerry_run.h"

#include <stdio.h>
#include <stdlib.h>

#include "jerryscript-port.h"
#include "jerryscript.h"

#include "jerry_extapi.h"

static const char *fn_sys_loop_name = "sysloop";

void
js_entry ()
{
  union
  {
    double d;
    unsigned u;
  } now = { .d = jerry_port_current_time () };
  srand (now.u);

  jerry_init (JERRY_INIT_EMPTY);
  js_register_functions ();
}

int
js_eval (const char *source_p, const size_t source_size)
{
  jerry_value_t res = jerry_eval ((jerry_char_t *) source_p, source_size, JERRY_PARSE_NO_OPTS);
  if (jerry_value_is_exception (res))
  {
    jerry_value_free (res);
    return -1;
  }

  jerry_value_free (res);

  return 0;
}

int
js_loop (uint32_t ticknow)
{
  jerry_value_t global_obj_val = jerry_current_realm ();
  jerry_value_t prop_name_val = jerry_string_sz (fn_sys_loop_name);
  jerry_value_t sysloop_func = jerry_object_get (global_obj_val, prop_name_val);
  jerry_value_free (prop_name_val);

  if (jerry_value_is_exception (sysloop_func))
  {
    printf ("Error: '%s' not defined!!!\r\n", fn_sys_loop_name);
    jerry_value_free (sysloop_func);
    jerry_value_free (global_obj_val);
    return -1;
  }

  if (!jerry_value_is_function (sysloop_func))
  {
    printf ("Error: '%s' is not a function!!!\r\n", fn_sys_loop_name);
    jerry_value_free (sysloop_func);
    jerry_value_free (global_obj_val);
    return -2;
  }

  jerry_value_t val_args[] = { jerry_number (ticknow) };
  uint16_t val_argv = sizeof (val_args) / sizeof (jerry_value_t);

  jerry_value_t res = jerry_call (sysloop_func, global_obj_val, val_args, val_argv);

  for (uint16_t i = 0; i < val_argv; i++)
  {
    jerry_value_free (val_args[i]);
  }

  jerry_value_free (sysloop_func);
  jerry_value_free (global_obj_val);

  if (jerry_value_is_exception (res))
  {
    jerry_value_free (res);
    return -3;
  }

  jerry_value_free (res);

  return 0;
}

void
js_exit (void)
{
  jerry_cleanup ();
}

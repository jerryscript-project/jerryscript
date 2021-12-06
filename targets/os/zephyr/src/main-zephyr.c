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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>

#include "jerryscript-port.h"
#include "jerryscript.h"

#include "getline-zephyr.h"
#include "jerryscript-ext/handler.h"
#include <sys/printk.h>

static jerry_value_t print_function;

/**
 * Register a JavaScript function in the global object.
 */
static void
register_js_function (const char *name_p, /**< name of the function */
                      jerry_external_handler_t handler_p) /**< function callback */
{
  jerry_value_t result_val = jerryx_handler_register_global (name_p, handler_p);

  if (jerry_value_is_exception (result_val))
  {
    jerry_port_log (JERRY_LOG_LEVEL_WARNING, "Warning: failed to register '%s' method.", name_p);
  }

  jerry_value_free (result_val);
} /* register_js_function */

static int
shell_cmd_handler (char *source_buffer)
{
  jerry_value_t ret_val;

  ret_val = jerry_eval ((jerry_char_t *) source_buffer, strlen (source_buffer), JERRY_PARSE_NO_OPTS);

  if (jerry_value_is_exception (ret_val))
  {
    /* User-friendly error messages require at least "cp" JerryScript
       profile. Include a message prefix in case "cp_minimal" profile
       is used. */
    printf ("Error executing statement: ");
    /* Clear error flag, otherwise print call below won't produce any
       output. */
    ret_val = jerry_exception_value (ret_val, true);
  }

  if (!jerry_value_is_exception (print_function))
  {
    jerry_value_t ret_val_print = jerry_call (print_function, jerry_undefined (), &ret_val, 1);
    jerry_value_free (ret_val_print);
  }

  jerry_value_free (ret_val);

  return 0;
} /* shell_cmd_handler */

void
main (void)
{
  union
  {
    double d;
    unsigned u;
  } now = { .d = jerry_port_get_current_time () };
  srand (now.u);
  uint32_t zephyr_ver = sys_kernel_version_get ();
  printf ("JerryScript build: " __DATE__ " " __TIME__ "\n");
  printf ("JerryScript API %d.%d.%d\n", JERRY_API_MAJOR_VERSION, JERRY_API_MINOR_VERSION, JERRY_API_PATCH_VERSION);
  printf ("Zephyr version %d.%d.%d\n",
          (int) SYS_KERNEL_VER_MAJOR (zephyr_ver),
          (int) SYS_KERNEL_VER_MINOR (zephyr_ver),
          (int) SYS_KERNEL_VER_PATCHLEVEL (zephyr_ver));

  zephyr_getline_init ();
  jerry_init (JERRY_INIT_EMPTY);
  register_js_function ("print", jerryx_handler_print);
  jerry_value_t global_obj_val = jerry_current_realm ();

  jerry_value_t print_func_name_val = jerry_string_sz ("print");
  print_function = jerry_object_get (global_obj_val, print_func_name_val);
  jerry_value_free (print_func_name_val);
  jerry_value_free (global_obj_val);
  if (jerry_value_is_exception (print_function))
  {
    printf ("Error: could not look up print function, expression results won't be printed\n");
  }

  while (1)
  {
    char *s;
    printf ("js> ");
    fflush (stdout);
    s = zephyr_getline ();
    if (*s)
    {
      shell_cmd_handler (s);
    }
  }

  /* As we never retturn from REPL above, don't call jerry_cleanup() here. */
} /* main */

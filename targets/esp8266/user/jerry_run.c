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
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include <espressif/esp_common.h>

#ifdef JERRY_DEBUGGER
#include "esp_sntp.h"
#endif

#include "jerry_extapi.h"
#include "jerry_run.h"
#include "jerry-targetjs.h"

#include "jerryscript.h"
#include "jerryscript-port.h"

static const char *fn_sys_loop_name = "sysloop";

/**
 * Add '.js' suffix to resource name
 */
static void
suffix_resource_name (const char *name, /**< name of the source */
                      jerry_size_t length, /**< length of the source */
                      jerry_char_t *resource_name) /**< destination of the suffixed source name */

{
  strncpy ((char *) resource_name, name, length);
  resource_name[length] = '.';
  resource_name[length + 1] = 'j';
  resource_name[length + 2] = 's';
} /* suffix_resource_name */


/**
 * Print error value
 */
static void
print_unhandled_exception (jerry_value_t error_value) /**< error value */
{
  jerry_char_t err_str_buf[256];

  if (jerry_value_is_object (error_value))
  {
    jerry_value_t stack_str = jerry_create_string ((const jerry_char_t *) "stack");
    jerry_value_t backtrace_val = jerry_get_property (error_value, stack_str);
    jerry_release_value (stack_str);

    if (!jerry_value_is_error (backtrace_val)
        && jerry_value_is_array (backtrace_val))
    {
      printf ("Exception backtrace:\n");

      uint32_t length = jerry_get_array_length (backtrace_val);

      /* This length should be enough. */
      if (length > 32)
      {
        length = 32;
      }

      for (uint32_t i = 0; i < length; i++)
      {
        jerry_value_t item_val = jerry_get_property_by_index (backtrace_val, i);

        if (!jerry_value_is_error (item_val)
            && jerry_value_is_string (item_val))
        {
          jerry_size_t str_size = jerry_get_string_size (item_val);

          if (str_size >= 256)
          {
            printf ("%3d: [Backtrace string too long]\n", i);
          }
          else
          {
            jerry_size_t string_end = jerry_string_to_char_buffer (item_val, err_str_buf, str_size);
            err_str_buf[string_end] = 0;

            printf ("%3d: %s\n", i, err_str_buf);
          }
        }

        jerry_release_value (item_val);
      }
    }
    jerry_release_value (backtrace_val);
  }
} /* print_unhandled_exception */

/**
 * Parse and run the given sources
 * Return true - if parsing and run was succesful
 *        fasle - otherwise
 */
static bool
parse_and_run_resource (const char *name, /**< name of the source */
                        const char *source, /**< content of the source */
                        const int length) /**< length of the source */
{
  jerry_size_t resource_name_length = strlen (name) + 3;
  jerry_char_t resource_name[resource_name_length];

  suffix_resource_name (name, resource_name_length, resource_name);

  jerry_value_t res = jerry_parse (resource_name,
                                   resource_name_length,
                                   (jerry_char_t *) source,
                                   length,
                                   false);

  if (jerry_value_is_error (res))
  {
    jerry_value_clear_error_flag (&res);
    print_unhandled_exception (res);
    return false;
  }

  jerry_value_t func_val = res;
  res = jerry_run (func_val);
  jerry_release_value (func_val);

  if (jerry_value_is_error (res))
  {
    jerry_value_clear_error_flag (&res);
    print_unhandled_exception (res);
    return false;
  }

  jerry_release_value (res);
  return true;
} /* parse_and_run_resource */

/**
 * Initialize the engine and parse the given sources
 * Return true - if all script parsing was succesful
 *        fasle - otherwise
 */
bool
jerry_task_init (void)
{
  srand ((unsigned) jerry_port_get_current_time ());
  DECLARE_JS_CODES;
  jerry_init (JERRY_INIT_EMPTY);

  register_js_entries ();

#ifdef JERRY_DEBUGGER
  while (sdk_wifi_station_get_connect_status () != STATION_GOT_IP)
  {
    vTaskDelay (1000 / portTICK_PERIOD_MS);
  }

  if (!sntp_been_initialized)
  {
    init_esp_sntp ();
  }

  jerry_debugger_init (5001);
#endif /* JERRY_DEBUGGER */

  /* run rest of the js files first */
  for (int idx = 1; js_codes[idx].source != NULL; idx++)
  {
    if (!parse_and_run_resource (js_codes[idx].name, js_codes[idx].source, js_codes[idx].length))
    {
      return false;
    }
  }

  /* run main.js */
  if (!parse_and_run_resource (js_codes[0].name, js_codes[0].source, js_codes[0].length))
  {
    return false;
  }

  return true;
} /* jerry_task_init */

/**
 * Call sysloop function
 * Return true - if the function call was succesfull
 *        false - otherwise
 */
bool
js_loop (void)
{
  jerry_value_t global_obj_val = jerry_get_global_object ();
  jerry_value_t prop_name_val = jerry_create_string ((const jerry_char_t *) fn_sys_loop_name);
  jerry_value_t sysloop_func = jerry_get_property (global_obj_val, prop_name_val);
  jerry_release_value (prop_name_val);

  if (jerry_value_is_error (sysloop_func))
  {
    jerry_value_clear_error_flag (&sysloop_func);
    print_unhandled_exception (sysloop_func);
    jerry_release_value (sysloop_func);
    jerry_release_value (global_obj_val);
    return false;
  }

  if (!jerry_value_is_function (sysloop_func))
  {
    printf ("Error: '%s' is not a function!\n", fn_sys_loop_name);
    jerry_release_value (sysloop_func);
    jerry_release_value (global_obj_val);
    return false;
  }

  jerry_value_t res = jerry_call_function (sysloop_func,
                                           global_obj_val,
                                           NULL,
                                           0);

  jerry_release_value (sysloop_func);
  jerry_release_value (global_obj_val);

  if (jerry_value_is_error (res))
  {
    jerry_release_value (res);
    return false;
  }

  jerry_release_value (res);

  return true;
} /* js_loop */

/**
 * Terminate the engine
 */
void
jerry_task_exit (void)
{
  jerry_cleanup ();
} /* jerry_task_exit */

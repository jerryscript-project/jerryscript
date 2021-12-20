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

#include "jerryscript-port.h"
#include "jerryscript.h"

#include "jerryscript-ext/handlers.h"
#include "jerryscript-ext/properties.h"
#include "mbed.h"

/**
 * Standalone Jerry exit codes
 */
#define JERRY_STANDALONE_EXIT_CODE_OK   (0)
#define JERRY_STANDALONE_EXIT_CODE_FAIL (1)

#if MBED_MAJOR_VERSION == 5
static Serial serial(USBTX, USBRX, 115200);
#elif MBED_MAJOR_VERSION == 6
static BufferedSerial serial(USBTX, USBRX, 115200);
#else
#error Unsupported Mbed OS version.
#endif

int main()
{
  /* Initialize engine */
  jerry_init (JERRY_INIT_EMPTY);

  const jerry_char_t script[] = "print ('Hello, World!');";
  jerry_log (JERRY_LOG_LEVEL_DEBUG, "This test run the following script code: [%s]\n\n", script);

  /* Register the print function in the global object. */
  jerryx_register_global ("print", jerryx_handler_print);

  /* Setup Global scope code */
  jerry_value_t ret_value = jerry_parse (script, sizeof (script) - 1, NULL);

  if (!jerry_value_is_exception (ret_value))
  {
    /* Execute the parsed source code in the Global scope */
    ret_value = jerry_run (ret_value);
  }

  int ret_code = JERRY_STANDALONE_EXIT_CODE_OK;

  if (jerry_value_is_exception (ret_value))
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "[Error] Script Error!");

    ret_code = JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  jerry_value_free (ret_value);

  /* Cleanup engine */
  jerry_cleanup ();

  return ret_code;
}

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
#include <sys/printk.h>
#include <zephyr.h>

#include "jerryscript-port.h"
#include "jerryscript.h"

#include "getline-zephyr.h"
#include "jerryscript-ext/handlers.h"
#include "jerryscript-ext/properties.h"
#include "jerryscript-ext/repl.h"

void
main (void)
{
  union
  {
    double d;
    unsigned u;
  } now = { .d = jerry_port_current_time () };
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
  jerryx_register_global ("print", jerryx_handler_print);

  jerryx_repl (JERRY_ZSTR_ARG ("js> "));

  jerry_cleanup ();
} /* main */

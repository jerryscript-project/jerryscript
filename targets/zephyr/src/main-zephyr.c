/* Copyright 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <zephyr.h>
#include <misc/printk.h>
#include <misc/shell.h>

#include "jerry-api.h"

#if defined (CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT       printf
#else
#include <misc/printk.h>
#define PRINT       printk
#endif

static char *source_buffer = NULL;
static unsigned char flags = 0;

#define VERBOSE 0x01

/**
 * Jerryscript simple test loop
 */
int jerryscript_test ()
{
  jerry_value_t ret_val;

  const char script[] =
  "var test=0; " \
  "for (var t=100; t<1000; t++) test+=t; " \
  "print ('Hi JS World! '+test);";

  printf ("Script [%s]\n", script);
  ret_val = jerry_eval ((jerry_char_t *) script,
      strlen (script),
      false);

  return jerry_value_has_error_flag (ret_val) ? -1 : 0;
} /* jerryscript_test */


static int shell_cmd_verbose (int argc, char *argv[])
{
  printf ("Enable verbose \n");
  flags |= VERBOSE;
  return 0;
} /* shell_cmd_verbose */


static int shell_cmd_syntax_help (int argc, char *argv[])
{
  printf ("version jerryscript & zephyr versions\n");
  return 0;
} /* shell_cmd_syntax_help */


static int shell_cmd_version (int argc, char *argv[])
{
  uint32_t version = sys_kernel_version_get ();

  printf ("Jerryscript API %d.%d\n", JERRY_API_MAJOR_VERSION, JERRY_API_MINOR_VERSION);

  printk ("Zephyr version %d.%d.%d\n", SYS_KERNEL_VER_MAJOR (version),
    SYS_KERNEL_VER_MINOR (version),
    SYS_KERNEL_VER_PATCHLEVEL (version));
  return 0;
} /* shell_cmd_version */


static int shell_cmd_test (int argc, char *argv[])
{
  return jerryscript_test ();
} /* shell_cmd_test */


static int shell_cmd_handler (int argc, char *argv[])
{
  if (argc <= 0)
  {
    return -1;
  }

  unsigned int size = 0;
  for (int t = 0; t < argc; t++)
  {
    size += strlen (argv[t]) + 1;
  }

  source_buffer = (char *) malloc (size);

  char *d = source_buffer;
  unsigned int len;

  for (int t = 0; t < argc; t++)
  {
    len = strlen (argv[t]);
    memcpy (d, argv[t], len);
    d += len;
    *d = ' ';
    d++;
  }

  * (d - 1) = '\0';

  if (flags & VERBOSE)
  {
    printf ("[%s] %lu\n", source_buffer, strlen (source_buffer));
  }

  jerry_value_t ret_val;

  ret_val = jerry_eval ((jerry_char_t *) source_buffer,
    strlen (source_buffer),
    false);

  free (source_buffer);

  if (jerry_value_has_error_flag (ret_val))
  {
    printf ("Failed to run JS\n");
  }

  jerry_release_value (ret_val);

  return 0;
} /* shell_cmd_handler */

#define SHELL_COMMAND(name,cmd) { name, cmd }

const struct shell_cmd commands[] =
{
  SHELL_COMMAND ("syntax", shell_cmd_syntax_help),
  SHELL_COMMAND ("version", shell_cmd_version),
  SHELL_COMMAND ("test", shell_cmd_test),
  SHELL_COMMAND ("verbose", shell_cmd_verbose),
  SHELL_COMMAND (NULL, NULL)
};


void main (void)
{
  printf ("Jerry Compilation " __DATE__ " " __TIME__ "\n");
  jerry_init (JERRY_INIT_EMPTY);
  shell_register_app_cmd_handler (shell_cmd_handler);
  shell_init ("js> ", commands);
  /* Don't call jerry_cleanup() here, as shell_init() returns after setting
     up background task to process shell input, and that task calls
     shell_cmd_handler(), etc. as callbacks. This processing happens in
     the infinite loop, so JerryScript doesn't need to be de-initialized. */
} /* main */


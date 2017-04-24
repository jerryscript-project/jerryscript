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
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>

#include "jerryscript.h"
#include "jerryscript-port.h"
#include "jmem.h"

#include <apps/shell/tash.h> // To register tash command

/**
 * Maximum command line arguments number.
 */
#define JERRY_MAX_COMMAND_LINE_ARGS (16)

/**
 * Standalone Jerry exit codes.
 */
#define JERRY_STANDALONE_EXIT_CODE_OK   (0)
#define JERRY_STANDALONE_EXIT_CODE_FAIL (1)

/**
 * Print usage and available options
 */
static void
print_help (char *name)
{
  printf ("Usage: %s [OPTION]... [FILE]...\n"
          "\n"
          "Options:\n"
          "  --log-level [0-3]\n"
          "  --mem-stats\n"
          "  --mem-stats-separate\n"
          "  --show-opcodes\n"
          "  --start-debug-server\n"
          "\n",
          name);
} /* print_help */

/**
 * Read source code into buffer.
 *
 * Returned value must be freed with jmem_heap_free_block if it's not NULL.
 * @return NULL, if read or allocation has failed
 *         pointer to the allocated memory block, otherwise
 */
static const uint8_t *
read_file (const char *file_name, /**< source code */
           size_t *out_size_p) /**< [out] number of bytes successfully read from source */
{
  FILE *file = fopen (file_name, "r");
  if (file == NULL)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: cannot open file: %s\n", file_name);
    return NULL;
  }

  int fseek_status = fseek (file, 0, SEEK_END);
  if (fseek_status != 0)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Failed to seek (error: %d)\n", fseek_status);
    fclose (file);
    return NULL;
  }

  long script_len = ftell (file);
  if (script_len < 0)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Failed to get the file size(error %ld)\n", script_len);
    fclose (file);
    return NULL;
  }

  rewind (file);

  uint8_t *buffer = jmem_heap_alloc_block_null_on_error (script_len);

  if (buffer == NULL)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Out of memory error\n");
    fclose (file);
    return NULL;
  }

  size_t bytes_read = fread (buffer, 1u, script_len, file);

  if (!bytes_read || bytes_read != script_len)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: failed to read file: %s\n", file_name);
    jmem_heap_free_block ((void*) buffer, script_len);

    fclose (file);
    return NULL;
  }

  fclose (file);

  *out_size_p = bytes_read;
  return (const uint8_t *) buffer;
} /* read_file */

/**
 * Provide the 'assert' implementation for the engine.
 *
 * @return true - if only one argument was passed and the argument is a boolean true.
 */
static jerry_value_t
assert_handler (const jerry_value_t func_obj_val __attribute__((unused)), /**< function object */
                const jerry_value_t this_p __attribute__((unused)), /**< this arg */
                const jerry_value_t args_p[], /**< function arguments */
                const jerry_length_t args_cnt) /**< number of function arguments */
{
  if (args_cnt == 1
      && jerry_value_is_boolean (args_p[0])
      && jerry_get_boolean_value (args_p[0]))
  {
    return jerry_create_boolean (true);
  }
  else
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Script Error: assertion failed\n");
    exit (JERRY_STANDALONE_EXIT_CODE_FAIL);
  }
} /* assert_handler */

/**
 * Provide the 'gc' implementation for the engine.
 *
 * @return undefined.
 */
static jerry_value_t
gc_handler (const jerry_value_t func_obj_val __attribute__((unused)), /**< function object */
            const jerry_value_t this_p __attribute__((unused)), /**< this arg */
            const jerry_value_t args_p[] __attribute__((unused)), /**< function arguments */
            const jerry_length_t args_cnt __attribute__((unused))) /**< number of function arguments */
{
  jerry_gc ();
  return jerry_create_undefined ();
} /* gc_handler */

/**
 * Print error value
 */
static void
print_unhandled_exception (jerry_value_t error_value) /**< error value */
{
  printf ("print_unhandled_exception\n");
  (void)(error_value);
} /* print_unhandled_exception */

/**
 * Register a JavaScript function in the global object.
 */
static void
register_js_function (const char *name_p, /**< name of the function */
                      jerry_external_handler_t handler_p) /**< function callback */
{
  jerry_value_t global_obj_val = jerry_get_global_object ();

  jerry_value_t function_val = jerry_create_external_function (handler_p);
  jerry_value_t function_name_val = jerry_create_string ((const jerry_char_t *) name_p);
  jerry_value_t result_val = jerry_set_property (global_obj_val, function_name_val, function_val);

  jerry_release_value (function_name_val);
  jerry_release_value (function_val);
  jerry_release_value (global_obj_val);

  if (jerry_value_has_error_flag (result_val))
  {
    jerry_port_log (JERRY_LOG_LEVEL_WARNING, "Warning: failed to register '%s' method.", name_p);
    print_unhandled_exception (result_val);
  }

  jerry_release_value (result_val);
} /* register_js_function */



/**
 * Main program.
 *
 * @return 0 if success, error code otherwise
 */
#ifdef CONFIG_BUILD_KERNEL
int main (int argc, FAR char *argv[])
#else
int jerry_main (int argc, char *argv[])
#endif
{
  if (argc > JERRY_MAX_COMMAND_LINE_ARGS)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR,
                    "Too many command line arguments. Current maximum is %d\n",
                    JERRY_MAX_COMMAND_LINE_ARGS);

    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  const char *file_names[JERRY_MAX_COMMAND_LINE_ARGS];
  int i;
  int files_counter = 0;

  jerry_init_flag_t flags = JERRY_INIT_EMPTY;

  for (i = 1; i < argc; i++)
  {
    if (!strcmp ("-h", argv[i]) || !strcmp ("--help", argv[i]))
    {
      print_help (argv[0]);
      return JERRY_STANDALONE_EXIT_CODE_OK;
    }
    else if (!strcmp ("--mem-stats", argv[i]))
    {
      flags |= JERRY_INIT_MEM_STATS;
    }
    else if (!strcmp ("--mem-stats-separate", argv[i]))
    {
      flags |= JERRY_INIT_MEM_STATS_SEPARATE;
    }
    else if (!strcmp ("--show-opcodes", argv[i]))
    {
      flags |= JERRY_INIT_SHOW_OPCODES | JERRY_INIT_SHOW_REGEXP_OPCODES;
    }
    else if (!strcmp ("--start-debug-server", argv[i]))
    {
      flags |= JERRY_INIT_DEBUGGER;
    }
    else
    {
      file_names[files_counter++] = argv[i];
    }
  }

  if (files_counter == 0)
  {
    printf ("No input files, running a hello world demo:\n");
    char *source_p = "var a = 3.5; print('Hell world ' + (a + 1.5) + ' times from JerryScript')";

    jerry_run_simple ((jerry_char_t *) source_p, strlen (source_p), flags);
    return JERRY_STANDALONE_EXIT_CODE_OK;
  }

  jerry_init (flags);
  register_js_function ("assert", assert_handler);
  register_js_function ("gc", gc_handler);
  jerry_value_t ret_value = jerry_create_undefined ();

  for (i = 0; i < files_counter; i++)
  {
    size_t source_size;
    const jerry_char_t *source_p = read_file (file_names[i], &source_size);

    if (source_p == NULL)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Source file load error\n");
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }

    ret_value = jerry_parse_named_resource ((jerry_char_t *) file_names[i],
                                            strlen (file_names[i]),
                                            source_p,
                                            source_size,
                                            false);

    jmem_heap_free_block ((void*) source_p, source_size);

    if (!jerry_value_has_error_flag (ret_value))
    {
      jerry_value_t func_val = ret_value;
      ret_value = jerry_run (func_val);
      jerry_release_value (func_val);
    }
    else
    {
      printf("jerry_parse error\n");
      break;
    }

    if (jerry_value_has_error_flag (ret_value))
    {
      printf("jerry_run error\n");
      break;
    }

    jerry_release_value (ret_value);
    ret_value = jerry_create_undefined ();
  }

  int ret_code = JERRY_STANDALONE_EXIT_CODE_OK;

  if (jerry_value_has_error_flag (ret_value))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Unhandled exception: Script Error!\n");
    ret_code = JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  jerry_release_value (ret_value);
  jerry_cleanup ();

  return ret_code;
} /* main */

int jerry_register_cmd() {
  const tash_cmdlist_t tash_cmds[] 
      = { { "jerry", jerry_main, TASH_EXECMD_SYNC }, { 0, 0, 0 } };
  tash_cmdlist_install(tash_cmds);
  return 0;
}


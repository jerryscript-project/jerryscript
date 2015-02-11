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

#ifdef __TARGET_MCU
#include "common-io.h"
#include "actuators.h"
#include "sensors.h"

#include JERRY_MCU_SCRIPT_HEADER
static const char generated_source [] = JERRY_MCU_SCRIPT;

#endif

#include "jrt.h"
#include "vm.h"
#include "jerry-libc.h"
#include "lexer.h"
#include "parser.h"
#include "serializer.h"
#include "deserializer.h"

#define MAX_STRINGS 100
#define MAX_NUMS 25

static bool
jerry_run (const char *script_source, size_t script_source_size,
           bool is_parse_only, bool is_show_opcodes, bool is_show_mem_stats)
{
  const opcode_t *opcodes;

  mem_init ();
  deserializer_init ();

  parser_init (script_source, script_source_size, is_show_opcodes);
  parser_parse_program ();

  opcodes = (const opcode_t*) deserialize_bytecode ();

#ifdef __TARGET_HOST
  serializer_print_opcodes ();
#endif /* __TARGET_HOST */
  parser_free ();

  if (is_parse_only)
  {
    deserializer_free ();
    mem_finalize (is_show_mem_stats);
    return true;
  }

  init_int (opcodes, is_show_mem_stats);

  bool is_success = run_int ();

  deserializer_free ();
  mem_finalize (is_show_mem_stats);

  return is_success;
} /* jerry_run */

#ifdef __TARGET_HOST
static uint8_t source_buffer[ JERRY_SOURCE_BUFFER_SIZE ];

static const char*
read_sources (const char *script_file_names[],
              size_t files_count,
              size_t *out_source_size_p)
{
  size_t i;
  uint8_t *source_buffer_tail = source_buffer;

  for (i = 0; i < files_count; i++)
  {
    const char *script_file_name = script_file_names[i];

    _FILE *file = __fopen (script_file_name, "r");

    if (file == NULL)
    {
      jerry_exit (ERR_IO);
    }

    int fseek_status = __fseek (file, 0, __SEEK_END);

    if (fseek_status != 0)
    {
      jerry_exit (ERR_IO);
    }

    long script_len = __ftell (file);

    if (script_len < 0)
    {
      jerry_exit (ERR_IO);
    }

    __rewind (file);

    const size_t current_source_size = (size_t)script_len;

    if (source_buffer_tail + current_source_size >= source_buffer + sizeof (source_buffer))
    {
      jerry_exit (ERR_OUT_OF_MEMORY);
    }

    size_t bytes_read = __fread (source_buffer_tail, 1, current_source_size, file);
    if (bytes_read < current_source_size)
    {
      jerry_exit (ERR_IO);
    }

    __fclose (file);

    source_buffer_tail += current_source_size;
  }

  const size_t source_size = (size_t) (source_buffer_tail - source_buffer);
  JERRY_ASSERT(source_size < sizeof (source_buffer));

  *out_source_size_p = source_size;

  return (const char*)source_buffer;
}

int
main (int argc __unused,
      char **argv __unused)
{
  if (argc > CONFIG_JERRY_MAX_COMMAND_LINE_ARGS)
  {
    jerry_exit (ERR_OUT_OF_MEMORY);
  }

  const char *file_names[CONFIG_JERRY_MAX_COMMAND_LINE_ARGS];
  bool parse_only = false, show_opcodes = false;
  bool print_mem_stats = false;
  int i;
  size_t files_counter = 0;

  jrt_set_mem_limits (MEM_HEAP_AREA_SIZE + CONFIG_MEM_DATA_LIMIT_MINUS_HEAP_SIZE,
                      CONFIG_MEM_STACK_LIMIT);

  for (i = 1; i < argc; i++)
  {
    if (!__strcmp ("-v", argv[i]))
    {
      __printf ("Build date: \t%s\n", JERRY_BUILD_DATE);
      __printf ("Commit hash:\t%s\n", JERRY_COMMIT_HASH);
      __printf ("Branch name:\t%s\n", JERRY_BRANCH_NAME);
      __printf ("\n");
    }
    if (!__strcmp ("--mem-stats", argv[i]))
    {
#ifdef MEM_STATS
      print_mem_stats = true;
#else /* MEM_STATS */
      __printf ("Ignoring --mem-stats because of '!MEM_STATS' build configuration.\n");

      print_mem_stats = false;
#endif /* !MEM_STATS */
    }
    else if (!__strcmp ("--parse-only", argv[i]))
    {
      parse_only = true;
    }
    else if (!__strcmp ("--show-opcodes", argv[i]))
    {
#ifdef JERRY_ENABLE_PP
      show_opcodes = true;
#else /* !JERRY_ENABLE_PP */
      __printf ("Ignoring --show-opcodes since target is not x86_64 or debug is not enabled.\n");
      show_opcodes = false;
#endif /* JERRY_ENABLE_PP */
    }
    else
    {
      file_names[files_counter++] = argv[i];
    }
  }

  if (files_counter == 0)
  {
    jerry_exit (ERR_NO_FILES);
  }

  size_t source_size;
  const char *source_p = read_sources (file_names, files_counter, &source_size);

  bool is_success = jerry_run (source_p, source_size, parse_only, show_opcodes, print_mem_stats);

  jerry_exit (is_success ? ERR_OK : ERR_FAILED_ASSERTION_IN_SCRIPT);
}
#endif /* __TARGET_HOST */

#ifdef __TARGET_MCU
static uint32_t start __unused;
static uint32_t finish_native_ms __unused;
static uint32_t finish_parse_ms __unused;
static uint32_t finish_int_ms __unused;

int
main (void)
{
  initialize_sys_tick ();
  initialize_leds ();
  initialize_timer ();

  const char *source_p = generated_source;
  const size_t source_size = sizeof (generated_source);

  set_sys_tick_counter ((uint32_t) - 1);
  start = get_sys_tick_counter ();
  jerry_run (source_p,
             source_size, false, false, false);
  finish_parse_ms = (start - get_sys_tick_counter ()) / 1000;
}
#endif /* __TARGET_MCU */

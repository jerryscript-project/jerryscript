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

#include "config.h"
#include "jerry.h"
#include "jerry-libc.h"

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
  int i;
  size_t files_counter = 0;

  jrt_set_mem_limits (CONFIG_MEM_HEAP_AREA_SIZE + CONFIG_MEM_DATA_LIMIT_MINUS_HEAP_SIZE,
                      CONFIG_MEM_STACK_LIMIT);

  jerry_flag_t flags = JERRY_FLAG_EMPTY;

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
      flags |= JERRY_FLAG_MEM_STATS;
#else /* MEM_STATS */
      __printf ("Ignoring --mem-stats because of '!MEM_STATS' build configuration.\n");
#endif /* !MEM_STATS */
    }
    else if (!__strcmp ("--parse-only", argv[i]))
    {
      flags |= JERRY_FLAG_PARSE_ONLY;
    }
    else if (!__strcmp ("--show-opcodes", argv[i]))
    {
      flags |= JERRY_FLAG_SHOW_OPCODES;
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

  jerry_err_t ret_code = jerry_run_simple (source_p, source_size, flags);

  jerry_exit (ret_code);
}

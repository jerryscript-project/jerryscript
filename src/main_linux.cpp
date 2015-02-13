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

#include <stdio.h>
#include <string.h>

#include "jerry.h"

/**
 * Maximum command line arguments number
 */
#define JERRY_MAX_COMMAND_LINE_ARGS (64)

/**
 * Maximum size of source code buffer
 */
#define JERRY_SOURCE_BUFFER_SIZE (1048576)

/**
 * Standalone Jerry exit codes
 */
#define JERRY_STANDALONE_EXIT_CODE_OK   (0)
#define JERRY_STANDALONE_EXIT_CODE_FAIL (1)

static uint8_t source_buffer[ JERRY_SOURCE_BUFFER_SIZE ];

static const char*
read_sources (const char *script_file_names[],
              int files_count,
              size_t *out_source_size_p)
{
  int i;
  uint8_t *source_buffer_tail = source_buffer;

  for (i = 0; i < files_count; i++)
  {
    const char *script_file_name = script_file_names[i];

    FILE *file = fopen (script_file_name, "r");

    if (file == NULL)
    {
      break;
    }

    int fseek_status = fseek (file, 0, SEEK_END);

    if (fseek_status != 0)
    {
      break;
    }

    long script_len = ftell (file);

    if (script_len < 0)
    {
      break;
    }

    rewind (file);

    const size_t current_source_size = (size_t)script_len;

    if (source_buffer_tail + current_source_size >= source_buffer + sizeof (source_buffer))
    {
      break;
    }

    size_t bytes_read = fread (source_buffer_tail, 1, current_source_size, file);
    if (bytes_read < current_source_size)
    {
      break;
    }

    fclose (file);

    source_buffer_tail += current_source_size;
  }

  if (i < files_count)
  {
    printf ("Failed to read script N%d\n", i + 1);

    return NULL;
  }
  else
  {
    const size_t source_size = (size_t) (source_buffer_tail - source_buffer);

    *out_source_size_p = source_size;

    return (const char*)source_buffer;
  }
}

int
main (int argc,
      char **argv)
{
  if (argc >= JERRY_MAX_COMMAND_LINE_ARGS)
  {
    printf ("Too many command line arguments. Current maximum is %d (JERRY_MAX_COMMAND_LINE_ARGS)\n", argc);

    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  const char *file_names[JERRY_MAX_COMMAND_LINE_ARGS];
  int i;
  int files_counter = 0;

  size_t max_data_bss_size, max_stack_size;
  jerry_get_memory_limits (&max_data_bss_size, &max_stack_size);

  // FIXME:
  //  jrt_set_mem_limits (max_data_bss_size, max_stack_size);

  jerry_flag_t flags = JERRY_FLAG_EMPTY;

  for (i = 1; i < argc; i++)
  {
    if (!strcmp ("-v", argv[i]))
    {
      printf ("Build date: \t%s\n", jerry_build_date);
      printf ("Commit hash:\t%s\n", jerry_commit_hash);
      printf ("Branch name:\t%s\n", jerry_branch_name);
      printf ("\n");
    }
    if (!strcmp ("--mem-stats", argv[i]))
    {
#ifdef MEM_STATS
      flags |= JERRY_FLAG_MEM_STATS;
#else /* MEM_STATS */
      printf ("Ignoring --mem-stats because of '!MEM_STATS' build configuration.\n");
#endif /* !MEM_STATS */
    }
    else if (!strcmp ("--parse-only", argv[i]))
    {
      flags |= JERRY_FLAG_PARSE_ONLY;
    }
    else if (!strcmp ("--show-opcodes", argv[i]))
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
    return JERRY_STANDALONE_EXIT_CODE_OK;
  }
  else
  {
    size_t source_size;
    const char *source_p = read_sources (file_names, files_counter, &source_size);

    if (source_p == NULL)
    {
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }
    else
    {
      jerry_completion_code_t ret_code = jerry_run_simple (source_p, source_size, flags);

      if (ret_code == JERRY_COMPLETION_CODE_OK)
      {
        return JERRY_STANDALONE_EXIT_CODE_OK;
      }
      else
      {
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }
    }
  }
}

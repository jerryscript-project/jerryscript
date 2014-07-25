/* Copyright 2014 Samsung Electronics Co., Ltd.
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
#include "generated.h"
#endif

#include "globals.h"
#include "interpreter.h"
#include "jerry-libc.h"
#include "lexer.h"
#include "parser.h"
#include "serializer.h"
#include "deserializer.h"
#include "optimizer-passes.h"

#define MAX_STRINGS 100
#define MAX_NUMS 25

static void
jerry_run( const char *script_source,
           size_t script_source_size __unused)
{
  const char *strings[MAX_STRINGS];
  int32_t nums[MAX_NUMS];
  uint8_t strings_num, nums_count;
  uint8_t offset;
  const OPCODE *opcodes;

  mem_init();

  TODO( Consider using script_source_size in lexer to check buffer boundaries );

  lexer_init( script_source);

  lexer_run_first_pass();

  strings_num = lexer_get_strings (strings);
  nums_count = lexer_get_nums (nums);
  lexer_adjust_num_ids ();

  offset = serializer_dump_strings (strings, strings_num);
  serializer_dump_nums (nums, nums_count, offset, strings_num);

  parser_init ();
  parser_parse_program ();

  opcodes = deserialize_bytecode ();

  optimizer_run_passes ((OPCODE *) opcodes);

#ifdef __HOST
  serializer_print_opcodes ();
#endif

  init_int (opcodes);
  run_int ();
} /* jerry_run */

#ifdef __HOST
static uint8_t source_buffer[ JERRY_SOURCE_BUFFER_SIZE ];

static const char*
read_source( const char *script_file_name,
             size_t *out_source_size_p)
{
  _FILE *file = __fopen (script_file_name, "r");

  if (file == NULL)
    {
      jerry_exit (ERR_IO);
    }

  int fseek_status = __fseek( file, 0, __SEEK_END);

  if ( fseek_status != 0 )
    {
      jerry_exit (ERR_IO);
    }

  long script_len = __ftell( file);

  if ( script_len < 0 )
    {
      jerry_exit (ERR_IO);
    }

  __rewind( file);

  const size_t source_size = (size_t)script_len;
  size_t bytes_read = 0;
 
  while ( bytes_read < source_size )
    {
      bytes_read += __fread( source_buffer, 1, source_size, file);

      if ( __ferror( file) != 0 )
        {
          jerry_exit (ERR_IO);
        }
    }

  __fclose( file);

  *out_source_size_p = source_size;
  return (const char*)source_buffer;
}

int
main (int argc __unused,
      char **argv __unused)
{
  const char *file_name = NULL;

  if (argc > 2)
    {
      jerry_exit (ERR_SEVERAL_FILES);
    }
  else if (argc == 2)
    {
      file_name = argv[1];
    }
  else
    {
      jerry_exit (ERR_NO_FILES);
    }
 
  size_t source_size;
  const char *source_p = read_source( file_name, &source_size);

  jerry_run( source_p,
             source_size);

  mem_heap_print( false, false, true);

  return 0;
}
#endif

#ifdef __TARGET_MCU
int
main(void)
{
  const char *source_p = generated_source;
  const size_t source_size = sizeof(generated_source);

  jerry_run( source_p,
             source_size);
}
#endif

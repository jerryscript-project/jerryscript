/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#include "deserializer.h"
#include "jerry.h"
#include "jrt.h"
#include "parser.h"
#include "serializer.h"
#include "vm.h"

bool
jerry_run (const char *script_source, size_t script_source_size,
           bool is_parse_only, bool is_show_opcodes, bool is_show_mem_stats)
{
  const opcode_t *opcodes;

  mem_init ();
  deserializer_init ();

  parser_init (script_source, script_source_size, is_show_opcodes);
  parser_parse_program ();

  opcodes = (const opcode_t*) deserialize_bytecode ();

  serializer_print_opcodes ();
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

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

#include "globals.h"
#include "mem-allocator.h"
#include "opcodes.h"
#include "deserializer.h"
#include "common.h"
#include "parser.h"
#include "jerry-libc.h"

/**
 * Unit test's main function.
 */
int
main( int __unused argc,
      char __unused **argv)
{
  char program[] = {'a','=','1',';','v','a','r',' ','a',';','\0'};
  bool is_ok;

  mem_init();
  deserializer_init ();
  parser_init (program, __strlen (program), false);
  parser_parse_program ();
  parser_free ();

  if (!opcodes_equal(deserialize_bytecode (), (opcode_t[]) {
    [0] = getop_reg_var_decl (1, 2),    // var tmp1 .. tmp2;
    [1] =     getop_var_decl (0),       // var a;
    [2] =   getop_assignment (1, 1, 1), // tmp1 = 1: SMALLINT;
    [3] =   getop_assignment (0, 4, 1), // a = tmp1: TYPEOF(tmp1);
    [4] =      getop_exitval (0)        // exit 0;
  }, 5))
  {
    is_ok = false;
  }
  else
  {
    is_ok = true;
  }

  deserializer_free ();
  mem_finalize (false);

  return (is_ok ? 0 : 1);
} /* main */

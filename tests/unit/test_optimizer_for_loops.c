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
#include "serializer.h"
#include "optimizer-passes.h"
#include "jerry-libc.h"
#include "deserializer.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "mem-allocator.h"

#define MAX_STRINGS 100
#define MAX_NUMS 25
/**
 * Unit test's main function.
 */
int
main( int __unused argc,
      char __unused **argv)
{
  const char *strings[MAX_STRINGS];
  int32_t nums[MAX_NUMS];
  uint8_t strings_num, nums_count;
  uint16_t offset;
  const OPCODE *opcodes;
  const char *source = "for (var i = 0; i < 10; i++) {\n"
                       "  var j = 10;\n"
                       "}\n"
                       "for (var i = 0; i < 10; i++) {\n"
                       "  var j = 10;\n"
                       "}";

  mem_init ();
  serializer_init (true);
  lexer_init (source, __strlen (source), true);
  lexer_run_first_pass();

  strings_num = lexer_get_strings (strings);
  nums_count = lexer_get_nums (nums);
  lexer_adjust_num_ids ();

  offset = serializer_dump_strings (strings, strings_num);
  serializer_dump_nums (nums, nums_count, offset, strings_num);
  
  parser_init ();
  parser_parse_program ();

  opcodes = deserialize_bytecode ();
  serializer_print_opcodes ();
  if (!opcodes_equal (opcodes, (OPCODE[]) {
    [0]  = getop_reg_var_decl (5, 8),  // var tmp5 .. tmp8; 
    [1]  = getop_var_decl (0),         // var i; 
    [2]  = getop_var_decl (1),         // var j; 
    [3]  = getop_var_decl (0),         // var i; 
    [4]  = getop_var_decl (1),         // var j; 
    [5]  = getop_assignment (5, 2, 2), // tmp5 = 0; 
    [6]  = getop_assignment (0, 4, 5), // i = tmp5; 
    [7]  = getop_assignment (7, 2, 4), // tmp7 = 10; 
    [8]  = getop_less_than (6, 0, 7),  // tmp6 = i < tmp7; 
    [9]  = getop_is_false_jmp (6, 16), // if (!tmp6) goto 16; 
    [10] = getop_jmp_down (3),         // goto 13; 
    [11] = getop_post_incr (8, 0),     // tmp8 = i ++; 
    [12] = getop_jmp_up (5),           // goto 7; 
    [13] = getop_assignment (5, 2, 4), // tmp5 = 10; 
    [14] = getop_assignment (1, 4, 5), // j = tmp5; 
    [15] = getop_jmp_up (5),           // goto 10; 
    [16] = getop_assignment (5, 2, 2), // tmp5 = 0; 
    [17] = getop_assignment (0, 4, 5), // i = tmp5; 
    [18] = getop_assignment (7, 2, 4), // tmp7 = 10; 
    [19] = getop_less_than (6, 0, 7),  // tmp6 = i < tmp7; 
    [20] = getop_is_false_jmp (6, 27), // if (!tmp6) goto 27; 
    [21] = getop_jmp_down (3),         // goto 24; 
    [22] = getop_post_incr (8, 0),     // tmp8 = i ++; 
    [23] = getop_jmp_up (5),           // goto 18; 
    [24] = getop_assignment (5, 2, 4), // tmp5 = 10; 
    [25] = getop_assignment (1, 4, 5), // j = tmp5; 
    [26] = getop_jmp_up (5),           // goto 21; 
    [27] = getop_exitval (0)           // exit 0; 
  }, 28))
    return 1;

  return 0;
} 

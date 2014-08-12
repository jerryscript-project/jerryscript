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
  ecma_number_t nums[MAX_NUMS];
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
    [0]  = getop_reg_var_decl (2, 5),  // var tmp2 .. tmp5; 
    [1]  = getop_var_decl (0),         // var i; 
    [2]  = getop_var_decl (1),         // var j; 
    [3]  = getop_assignment (2, 1, 0), // tmp2 = 0; 
    [4]  = getop_assignment (0, 4, 2), // i = tmp2; 
    [5]  = getop_assignment (4, 1, 10),// tmp4 = 10; 
    [6]  = getop_less_than (3, 0, 4),  // tmp3 = i < tmp4; 
    [7]  = getop_is_false_jmp (3, 14), // if (!tmp3) goto 14; 
    [8]  = getop_jmp_down (3),         // goto 11; 
    [9]  = getop_post_incr (5, 0),     // tmp5 = i ++; 
    [10] = getop_jmp_up (5),           // goto 5; 
    [11] = getop_assignment (2, 1, 10),// tmp2 = 10; 
    [12] = getop_assignment (1, 4, 2), // j = tmp2; 
    [13] = getop_jmp_up (5),           // goto 8; 
    [14] = getop_nop (),               // ; 
    [15] = getop_assignment (2, 1, 0), // tmp2 = 0; 
    [16] = getop_assignment (0, 4, 2), // i = tmp2; 
    [17] = getop_assignment (4, 1, 10),// tmp7 = 10; 
    [18] = getop_less_than (3, 0, 4),  // tmp3 = i < tmp7; 
    [19] = getop_is_false_jmp (3, 27), // if (!tmp3) goto 27; 
    [20] = getop_jmp_down (3),         // goto 23; 
    [21] = getop_post_incr (5, 0),     // tmp5 = i ++; 
    [22] = getop_jmp_up (5),           // goto 17; 
    [23] = getop_nop (),               // ; 
    [24] = getop_assignment (2, 1, 10),// tmp2 = 10; 
    [25] = getop_assignment (1, 4, 5), // j = tmp2; 
    [26] = getop_jmp_up (5),           // goto 21; 
    [27] = getop_exitval (0)           // exit 0; 
  }, 28))
    return 1;

  return 0;
} 

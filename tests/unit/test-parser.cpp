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

#include "mem-allocator.h"
#include "opcodes.h"
#include "parser.h"
#include "serializer.h"

#include "test-common.h"

static bool
instrs_equal (const vm_instr_t *instrs1, vm_instr_t *instrs2, uint16_t size)
{
  uint16_t i;
  for (i = 0; i < size; i++)
  {
    if (memcmp (&instrs1[i], &instrs2[i], sizeof (vm_instr_t)) != 0)
    {
      return false;
    }
  }

  return true;
}

/**
 * Unit test's main function.
 */
int
main (int __attr_unused___ argc,
      char __attr_unused___ **argv)
{
  TEST_INIT ();

  const vm_instr_t *instrs_p;
  jsp_status_t parse_status;

  mem_init ();

  // #1
  char program1[] = "a=1;var a;";

  serializer_init ();
  parser_set_show_instrs (true);
  parse_status = parser_parse_script ((jerry_api_char_t *) program1, strlen (program1), &instrs_p);

  JERRY_ASSERT (parse_status == JSP_STATUS_OK && instrs_p != NULL);

  vm_instr_t instrs[] =
  {
    getop_meta (OPCODE_META_TYPE_SCOPE_CODE_FLAGS, // [ ]
                OPCODE_SCOPE_CODE_FLAGS_NOT_REF_ARGUMENTS_IDENTIFIER
                | OPCODE_SCOPE_CODE_FLAGS_NOT_REF_EVAL_IDENTIFIER,
                INVALID_VALUE),
    getop_reg_var_decl (OPCODE_REG_FIRST, OPCODE_REG_GENERAL_FIRST, 0),
    getop_var_decl (0),             // var a;
    getop_assignment (130, 1, 1),   // $tmp0 = 1;
    getop_assignment (0, 6, 130),   // a = $tmp0;
    getop_ret ()                    // return;
  };

  JERRY_ASSERT (instrs_equal (instrs_p, instrs, 5));

  serializer_free ();

  // #2
  char program2[] = "var var;";

  serializer_init ();
  parser_set_show_instrs (true);
  parse_status = parser_parse_script ((jerry_api_char_t *) program2, strlen (program2), &instrs_p);

  JERRY_ASSERT (parse_status == JSP_STATUS_SYNTAX_ERROR && instrs_p == NULL);

  serializer_free ();

  mem_finalize (false);

  return 0;
} /* main */

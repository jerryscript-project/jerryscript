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

#include "bytecode-data.h"
#include "mem-allocator.h"
#include "opcodes.h"
#include "parser.h"
#include "test-common.h"

static bool
instrs_equal (const vm_instr_t *instrs1, vm_instr_t *instrs2, uint16_t size)
{
  static const uint8_t instr_fields_num[] =
  {
#define VM_OP_0(opcode_name, opcode_name_uppercase) \
    1,
#define VM_OP_1(opcode_name, opcode_name_uppercase, arg1, arg1_type) \
    2,
#define VM_OP_2(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type) \
    3,
#define VM_OP_3(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type, arg3, arg3_type) \
    4,

#include "vm-opcodes.inc.h"
  };

  uint16_t i;
  for (i = 0; i < size; i++)
  {
    if (memcmp (&instrs1[i], &instrs2[i], instr_fields_num[instrs1[i].op_idx] * sizeof (vm_idx_t)) != 0)
    {
      return false;
    }
  }

  return true;
}

#define VM_OP_0(opcode_name, opcode_name_uppercase) \
        static vm_instr_t __attr_unused___ getop_##opcode_name (void) \
        { \
          vm_instr_t instr; \
          instr.op_idx = VM_OP_##opcode_name_uppercase; \
          instr.data.raw_args[0] = VM_IDX_EMPTY; \
          instr.data.raw_args[1] = VM_IDX_EMPTY; \
          instr.data.raw_args[2] = VM_IDX_EMPTY; \
          return instr; \
        }
#define VM_OP_1(opcode_name, opcode_name_uppercase, arg_1, arg1_type) \
        static vm_instr_t __attr_unused___ getop_##opcode_name (vm_idx_t arg1_v) \
        { \
          vm_instr_t instr; \
          instr.op_idx = VM_OP_##opcode_name_uppercase; \
          instr.data.raw_args[0] = arg1_v; \
          instr.data.raw_args[1] = VM_IDX_EMPTY; \
          instr.data.raw_args[2] = VM_IDX_EMPTY; \
          return instr; \
        }
#define VM_OP_2(opcode_name, opcode_name_uppercase, arg_1, arg1_type, arg_2, arg2_type) \
        static vm_instr_t __attr_unused___ getop_##opcode_name (vm_idx_t arg1_v, vm_idx_t arg2_v) \
        { \
          vm_instr_t instr; \
          instr.op_idx = VM_OP_##opcode_name_uppercase; \
          instr.data.raw_args[0] = arg1_v; \
          instr.data.raw_args[1] = arg2_v; \
          instr.data.raw_args[2] = VM_IDX_EMPTY; \
          return instr; \
        }
#define VM_OP_3(opcode_name, opcode_name_uppercase, arg_1, arg1_type, arg_2, arg2_type, arg3_name, arg3_type) \
        static vm_instr_t __attr_unused___ getop_##opcode_name (vm_idx_t arg1_v, vm_idx_t arg2_v, vm_idx_t arg3_v) \
        { \
          vm_instr_t instr; \
          instr.op_idx = VM_OP_##opcode_name_uppercase; \
          instr.data.raw_args[0] = arg1_v; \
          instr.data.raw_args[1] = arg2_v; \
          instr.data.raw_args[2] = arg3_v; \
          return instr; \
        }

#include "vm-opcodes.inc.h"

/**
 * Unit test's main function.
 */
int
main (int __attr_unused___ argc,
      char __attr_unused___ **argv)
{
  TEST_INIT ();

  const bytecode_data_header_t *bytecode_data_p;
  jsp_status_t parse_status;

  mem_init ();

  // #1
  char program1[] = "a=1;var a;";

  lit_init ();
  parser_set_show_instrs (true);
  parse_status = parser_parse_script ((jerry_api_char_t *) program1, strlen (program1), &bytecode_data_p);

  JERRY_ASSERT (parse_status == JSP_STATUS_OK && bytecode_data_p != NULL);

  vm_instr_t instrs[] =
  {
    getop_reg_var_decl (1u, 0u, 0u),
    getop_assignment (0, 1, 1),                      // a = 1 (SMALLINT);
    getop_ret ()                                     // return;
  };

  JERRY_ASSERT (instrs_equal (bytecode_data_p->instrs_p, instrs, 3));

  lit_finalize ();
  bc_finalize ();

  // #2
  char program2[] = "var var;";

  lit_init ();
  parser_set_show_instrs (true);
  parse_status = parser_parse_script ((jerry_api_char_t *) program2, strlen (program2), &bytecode_data_p);

  JERRY_ASSERT (parse_status == JSP_STATUS_SYNTAX_ERROR && bytecode_data_p == NULL);

  lit_finalize ();
  bc_finalize ();

  mem_finalize (false);

  return 0;
} /* main */

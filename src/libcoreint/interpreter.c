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

#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "pretty-printer.h"
#include "interpreter.h"
#include "opcodes.h"

#include "actuators.h"

void
gen_bytecode (FILE *src_file)
{
  statement *st;
  lexer_set_file (src_file);
  parser_init ();
  st = parser_parse_statement ();
  assert (st);
  while (st->type != STMT_EOF)
  {
    //pp_statement (st);
    st = parser_parse_statement ();
    assert (st);
  }
  //pp_finish ();

//  FILE *file = fopen (FILE_NAME, "w+b");
//
//  OPCODE op0;
//
//  save_op_loop_inf (file, op0, 1);
//  save_op_call_1 (file, op0, 0, LED_GREEN);
//  save_op_call_1 (file, op0, 0, LED_BLUE);
//  save_op_call_1 (file, op0, 0, LED_ORANGE);
//  save_op_call_1 (file, op0, 0, LED_RED);
//  save_op_call_1 (file, op0, 0, LED_GREEN);
//  save_op_call_1 (file, op0, 0, LED_BLUE);
//  save_op_call_1 (file, op0, 0, LED_ORANGE);
//  save_op_call_1 (file, op0, 0, LED_RED);
//  save_op_jmp (file, op0, 0);
//
//  fclose (file);
}

void
run_int ()
{
  FILE *file = fopen (FILE_NAME, "rb");
  OPCODE op_curr;
  __int_data.pos = 0;


  if (file == NULL)
  {
    fputs ("File error", stderr);
    exit (1);
  }

  while (!feof (file))
  {
    if (!fread (&op_curr, sizeof (OPCODE), 1, file))
    {
      break;
    }

    __int_data.pos++;

    op_curr.opfunc_ptr (op_curr);
    fseek (file, __int_data.pos * sizeof (OPCODE), SEEK_SET);
  }

  fclose (file);
}


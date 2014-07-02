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
#include "opcode.h"


void
safe_opcode (FILE *file, opcode_ptr ptr, int arg1, int arg2)
{
  curr_opcode.func = ptr;
  curr_opcode.arg1 = arg1;
  curr_opcode.arg2 = arg2;

  if (file != NULL)
  {
    fwrite (&curr_opcode, sizeof (struct opcode_packed), 1, file);
  }
}

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
    pp_statement (st);
    st = parser_parse_statement ();
    assert (st);
  }
  pp_finish ();

  FILE *file = fopen (FILE_NAME, "w+b");
  int i;

  for (i = 0; i <= 10; i++)
  {
    safe_opcode (file, control_op, i, i);
    safe_opcode (file, decl_op, i, i);
    safe_opcode (file, call_op, i, i);
  }

  fclose (file);
}

void
run_int ()
{
  FILE *file = fopen (FILE_NAME, "rb");

  if (file == NULL)
  {
    fputs ("File error", stderr);
    exit (1);
  }

  while (!feof (file))
  {
    fread (&curr_opcode, sizeof (struct opcode_packed), 1, file);

    //printf ("read %d, %d, %p\n", curr_opcode.arg1, curr_opcode.arg2, curr_opcode.func);
    curr_opcode.func (curr_opcode.arg1, curr_opcode.arg2);
  }

  fclose (file);
}


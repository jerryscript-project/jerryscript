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

#include "interpreter.h"

void
gen_bytecode ()
{
#ifdef __HOST
  FILE *file = fopen (FILE_NAME, "w+b");
#endif

  //TODO REMOVE
  {
    save_op_data (file, get_op_loop_inf (1));
    save_op_data (file, get_op_call_1 (0, 12));
    save_op_data (file, get_op_call_1 (0, 13));
    save_op_data (file, get_op_call_1 (0, 14));
    save_op_data (file, get_op_call_1 (0, 15));
    save_op_data (file, get_op_jmp (0)); // mandatory!
  }
  
#ifdef __HOST
  fclose (file);
#endif
}

void
run_int ()
{
  OPCODE op_curr;
  __int_data.pos = 0;

#ifdef __HOST
  FILE *file = fopen (FILE_NAME, "rb");

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
#endif
}


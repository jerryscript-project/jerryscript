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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"

#include "ctx-manager.h"
#include "mem-allocator.h"

#include "interpreter.h"

#include "generated.h"

void fake_exit ();

void
fake_exit (void)
{
  for (;;);
}

int
main (int argc, char **argv)
{
  bool dump_tokens = false;
  bool dump_ast = true;
  const char *file_name = NULL;
  FILE *file = NULL;

  mem_Init ();
  ctx_Init ();

  if (argc > 0)
    for (int i = 1; i < argc; i++)
    {
      if (!strcmp ("-t", argv[i]))
        dump_tokens = true;
      else if (!strcmp ("-a", argv[i]))
        dump_ast = true;
      else if (file_name == NULL)
        file_name = argv[i];
      else
        fatal (ERR_SEVERAL_FILES);
    }

  if (file_name == NULL)
    fatal (ERR_NO_FILES);

  if (dump_tokens && dump_ast)
    fatal (ERR_SEVERAL_FILES);

  file = fopen (file_name, "r");

  if (file == NULL)
  {
    fatal (ERR_IO);
  }

  //gen_bytecode (generated_source);
  gen_bytecode (file);
  //run_int ();

#ifdef __TARGET_MCU
  fake_exit ();
#endif 

  return 0;
}

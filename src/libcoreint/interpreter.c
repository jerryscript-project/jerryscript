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
init_int (void)
{
#define INIT_OP_FUNC(name) __opfuncs[ __op__idx_##name ] = opfunc_##name ;
  JERRY_STATIC_ASSERT (sizeof (OPCODE) <= 4);

  OP_LIST (INIT_OP_FUNC)
}

void
run_int (void)
{
  init_int ();

  struct __int_data int_data;
  int_data.pos = 0;

  while (true)
  {
    run_int_from_pos (&int_data);
  }
}

void
run_int_from_pos (struct __int_data *int_data)
{
  OPCODE *curr = &__program[int_data->pos];
  __opfuncs[curr->op_idx](*curr, int_data);
}

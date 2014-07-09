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
  /*
   while (true) {
    LEDToggle (LED3);
    LEDToggle (LED6);
    LEDToggle (LED7);
    LEDToggle (LED4);
    LEDToggle (LED10);
    LEDToggle (LED8);
    LEDToggle (LED9);
    LEDToggle (LED5);

    wait(500);
   }
   */
//  save_op_data (0, getop_loop_inf (1));
//  save_op_data (1, getop_call_1 (0, 12));
//  save_op_data (2, getop_call_1 (0, 13));
//  save_op_data (3, getop_call_1 (0, 14));
//  save_op_data (4, getop_call_1 (0, 15));
//  save_op_data (5, getop_jmp (0));

#ifdef __MCU
  // It's mandatory to restart app!
  save_op_data (getop_jmp (0));
#endif
}

void
init_int ()
{
#define INIT_OP_FUNC(name) __opfuncs[ name ] = opfunc_##name ;
  // FIXME
  // JERRY_STATIC_ASSERT (sizeof (OPCODE) <= 4);

  OP_LIST (INIT_OP_FUNC)
}

void
run_int ()
{
  init_int ();

  struct __int_data int_data;
  int_data.pos = 0;

  while (true)
  {
    run_int_from_pos(&int_data);
  }
}

void
run_int_from_pos (struct __int_data *int_data)
{
  OPCODE *curr = &__program[int_data->pos];
  __opfuncs[curr->op_idx](*curr, int_data);
}

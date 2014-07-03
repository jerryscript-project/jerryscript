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

#ifndef OPCODES_H
#define	OPCODES_H

#include "stdio.h"

#define OP_RET_TYPE void
#define OP_INT_TYPE int

union __opcodes;

typedef OP_RET_TYPE (*op_proxy_ptr)(union __opcodes);

typedef OP_RET_TYPE (*opfunc_int_ptr)(union __opcodes);
typedef OP_RET_TYPE (*opfunc_int_int_ptr)(union __opcodes);

union __opcodes 
{
  op_proxy_ptr opfunc_ptr;
  
  struct __op_loop_inf
  {
    opfunc_int_ptr opfunc_ptr;
    int opcode_idx;
  } op_loop_inf;

  /** Call with 1 argument */
  struct __op_call_1
  {
    opfunc_int_int_ptr opfunc_ptr;
    int name_literal_idx;
    int arg_literal_idx;
  } op_call_1;

  struct __op_jmp
  {
    opfunc_int_ptr opfunc_ptr;
    int opcode_idx;
  } op_jmp;
} __packed;

void save_op_jmp(FILE *, union __opcodes, int);
void save_op_call_1(FILE *, union __opcodes, int, int);
void save_op_loop_inf(FILE *, union __opcodes, int);

#endif	/* OPCODES_H */


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

/* 
 * File:   opcode.h
 * Author: egavrin
 *
 * Created on July 2, 2014, 3:12 PM
 */

#ifndef OPCODE_H
#define	OPCODE_H

#define OP_RET_TYPE void
#define OP_ARG_TYPE int

#define OP_ATTR_DECL OP_ARG_TYPE, OP_ARG_TYPE
#define OP_ATTR_DEF OP_ARG_TYPE arg1, OP_ARG_TYPE arg2

#define OP_DECLARATION(opname) OP_RET_TYPE opname (OP_ATTR_DECL)
#define OP_DEFINITION(opname) OP_RET_TYPE opname (OP_ATTR_DEF)

// opcode ptr
typedef OP_RET_TYPE (*opcode_ptr)(OP_ATTR_DECL);

struct
__attribute__ ((__packed__))
opcode_packed
{
  opcode_ptr func;
  OP_ARG_TYPE arg1;
  OP_ARG_TYPE arg2;
}
curr_opcode;

#define INIT_OPCODE_PTR(func) opcode_ptr func_ptr = ptr;

OP_DECLARATION (decl_op);
OP_DECLARATION (call_op);
OP_DECLARATION (control_op);

#endif	/* OPCODE_H */


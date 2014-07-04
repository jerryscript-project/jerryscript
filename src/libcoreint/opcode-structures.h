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

#ifndef OPCODE_STRUCTURES_H
#define	OPCODE_STRUCTURES_H

 // Jerry bytecode ver:07/04/2014 

// 
#define OP_TYPE_IDX uint8_t

OP_DEF (nop) { };

OP_DEF (jmp)
{
  OP_TYPE_IDX opcode_idx;
};

OP_DEF (decl) { };

OP_DEF (decl_func_named)
{
  OP_TYPE_IDX name_literal_idx;  
};

OP_DEF (decl_func_anon) { };

OP_DEF (decl_var_global) { };

OP_DEF (decl_var_local) { };

/** Call with 1 argument */
OP_DEF (call_1)
{
  OP_TYPE_IDX name_literal_idx;
  OP_TYPE_IDX arg1_literal_idx;
};

OP_DEF (call_2)
{
  OP_TYPE_IDX name_literal_idx;
  OP_TYPE_IDX arg1_literal_idx;
  OP_TYPE_IDX arg2_literal_idx;
};

OP_DEF (call_n) { };

OP_DEF (list_header) { };
OP_DEF (list_element) { };
OP_DEF (list_tail) { };

OP_DEF (try_begin) { };

OP_DEF (try_end) { };

OP_DEF (catch_begin) { };

OP_DEF (catch_end) { };

OP_DEF (finally_begin) { };

OP_DEF (finally_end) { };

OP_DEF (with_begin) { };

OP_DEF (with_end) { };

OP_DEF (loop_inf)
{
  OP_TYPE_IDX opcode_idx;
};

OP_DEF (loop_cond_pre) { };

OP_DEF (loop_cond_post) { };

OP_DEF (swtch) { };

#endif	/* OPCODE_STRUCTURES_H */


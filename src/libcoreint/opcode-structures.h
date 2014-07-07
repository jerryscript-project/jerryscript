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
#define OP_DEF(name, list) struct __op_##name { list ;  }

#define OP_CODE_DECL(name, type, ... ) \
        OP_DEF (name, type##_DECL( __VA_ARGS__ ) ); \
        OPCODE getop_##name ( type );

#define T_IDX uint8_t
#define T_IDX_IDX T_IDX, T_IDX
#define T_IDX_IDX_IDX T_IDX, T_IDX, T_IDX

#define T_IDX_DECL(name) uint8_t name
#define T_IDX_IDX_DECL(name1, name2) \
        T_IDX_DECL( name1 ) ; \
        T_IDX_DECL( name2 )
#define T_IDX_IDX_IDX_DECL(name1, name2, name3) \
        T_IDX_DECL( name1 ) ; \
        T_IDX_DECL( name2 ); \
        T_IDX_DECL( name3 )


OP_CODE_DECL(nop, T_IDX, opcode_idx);
OP_CODE_DECL(jmp, T_IDX, opcode_idx);
OP_CODE_DECL(loop_inf, T_IDX, opcode_idx);

OP_CODE_DECL(call_1, T_IDX_IDX,
             name_literal_idx,
             arg1_literal_idx);

OP_CODE_DECL(call_2, T_IDX_IDX_IDX,
             name_literal_idx,
             arg1_literal_idx,
             arg2_literal_idx);

OP_CODE_DECL(call_n, T_IDX_IDX_IDX,
             name_literal_idx,
             arg1_literal_idx,
             arg2_literal_idx);


#endif	/* OPCODE_STRUCTURES_H */


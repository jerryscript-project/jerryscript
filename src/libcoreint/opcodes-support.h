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

#ifndef OPCODES_SUPPORT_H
#define OPCODES_SUPPORT_H

#include "opcodes.h"

#define GETOP_DECL_0(a, name) \
        __opcode getop_##name (void);

#define GETOP_DECL_1(a, name, arg1) \
        __opcode getop_##name (T_IDX);

#define GETOP_DECL_2(a, name, arg1, arg2) \
        __opcode getop_##name (T_IDX, T_IDX);

#define GETOP_DECL_3(a, name, arg1, arg2, arg3) \
        __opcode getop_##name (T_IDX, T_IDX, T_IDX);


OP_ARGS_LIST (GETOP_DECL)

#endif /* OPCODES_SUPPORT_H */


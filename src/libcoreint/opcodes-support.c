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

#include "opcodes-support.h"

#define GETOP_DEF_0(a, name) \
        __opcode getop_##name (void) \
        { \
          __opcode opdata; \
          opdata.op_idx = __op__idx_##name; \
          return opdata; \
        }

#define GETOP_DEF_1(a, name, field1) \
        __opcode getop_##name (T_IDX arg1) \
        { \
          __opcode opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          return opdata; \
        }

#define GETOP_DEF_2(a, name, field1, field2) \
        __opcode getop_##name (T_IDX arg1, T_IDX arg2) \
        { \
          __opcode opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          opdata.data.name.field2 = arg2; \
          return opdata; \
        }

#define GETOP_DEF_3(a, name, field1, field2, field3) \
        __opcode getop_##name (T_IDX arg1, T_IDX arg2, T_IDX arg3) \
        { \
          __opcode opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          opdata.data.name.field2 = arg2; \
          opdata.data.name.field3 = arg3; \
          return opdata; \
        }

OP_ARGS_LIST (GETOP_DEF)

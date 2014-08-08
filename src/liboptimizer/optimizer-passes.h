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

#ifndef OPTIMIZER_PASSES_H
#define OPTIMIZER_PASSES_H

#include "globals.h"
#include "opcodes.h"

void optimizer_move_opcodes (OPCODE *, OPCODE *, uint16_t);
void optimizer_adjust_jumps (OPCODE *, OPCODE *, int16_t);
void optimizer_reorder_scope (uint16_t, uint16_t);
void optimizer_run_passes (OPCODE *);

#endif // OPTIMIZER_PASSES_H

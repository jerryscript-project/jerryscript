/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#ifndef PRETTY_PRINTER
#define PRETTY_PRINTER

#include "jrt.h"
#ifdef JERRY_ENABLE_PRETTY_PRINTER
#include "vm.h"
#include "scopes-tree.h"

void pp_opcode (opcode_counter_t, opcode_t, bool);
void pp_op_meta (opcode_counter_t, op_meta, bool);
#endif // JERRY_ENABLE_PRETTY_PRINTER

#endif // PRETTY_PRINTER

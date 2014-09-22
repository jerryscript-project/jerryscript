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

#include "optimizer-passes.h"
#include "opcodes.h"
#include "deserializer.h"
#include "serializer.h"
#include "jerry-libc.h"

#define NAME_TO_ID(op) (__op__idx_##op)

void
optimizer_run_passes (opcode_t *opcodes __unused)
{
  FIXME (/*Write optimizer when postparser will be ready.  */)
}

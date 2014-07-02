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

#include "opcode.h"

#include <stdio.h>

OP_DEFINITION (control_op)
{
  printf ("control_op %d, %d\n", arg1, arg2);
}

OP_DEFINITION (decl_op)
{
  printf ("decl_op %d, %d\n", arg1, arg2);
}

OP_DEFINITION (call_op)
{
  printf ("call_op %d, %d\n", arg1, arg2);
}
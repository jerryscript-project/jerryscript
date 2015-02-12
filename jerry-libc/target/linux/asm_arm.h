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

#ifndef ASM_ARM_H
#define ASM_ARM_H

/*
 * mov syscall_no (%r0) -> %r7
 * mov arg1 (%r1) -> %r0
 * svc #0
 */
#define SYSCALL_1 \
  push {r4-r12, lr}; \
  \
  mov r7, r0; \
  mov r0, r1; \
  \
  svc #0; \
  \
  pop {r4-r12, pc};

/*
 * mov syscall_no (%r0) -> %r7
 * mov arg1 (%r1) -> %r0
 * mov arg2 (%r2) -> %r1
 * svc #0
 */
#define SYSCALL_2 \
  push {r4-r12, lr}; \
  \
  mov r7, r0; \
  mov r0, r1; \
  mov r1, r2; \
  \
  svc #0; \
  \
  pop {r4-r12, pc};

/*
 * mov syscall_no (%r0) -> %r7
 * mov arg1 (%r1) -> %r0
 * mov arg2 (%r2) -> %r1
 * mov arg3 (%r3) -> %r2
 * svc #0
 */
#define SYSCALL_3 \
  push {r4-r12, lr}; \
  \
  mov r7, r0; \
  mov r0, r1; \
  mov r1, r2; \
  mov r2, r3; \
  \
  svc #0; \
  \
  pop {r4-r12, pc};

#define _START            \
   ldr r0, [sp, #0];      \
   add r1, sp, #4;        \
   bl main;               \
                          \
   bl exit;             \
   1:                     \
   b 1b

#endif /* !ASM_ARM_H */

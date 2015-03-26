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

#ifndef ASM_X86_H
#define ASM_X86_H

/*
 * mov syscall_no -> %eax
 * mov arg1 -> %ebx
 * int $0x80
 * mov %eax -> ret
 */
#define SYSCALL_1 \
  push %edi;               \
  push %esi;               \
  push %ebx;               \
  mov 0x10 (%esp), %eax;   \
  mov 0x14 (%esp), %ebx;   \
  int $0x80;               \
  pop %ebx;                \
  pop %esi;                \
  pop %edi;                \
  ret;

/*
 * mov syscall_no -> %eax
 * mov arg1 -> %ebx
 * mov arg2 -> %ecx
 * int $0x80
 * mov %eax -> ret
 */
#define SYSCALL_2 \
  push %edi;               \
  push %esi;               \
  push %ebx;               \
  mov 0x10 (%esp), %eax;   \
  mov 0x14 (%esp), %ebx;   \
  mov 0x18 (%esp), %ecx;   \
  int $0x80;               \
  pop %ebx;                \
  pop %esi;                \
  pop %edi;                \
  ret;

/*
 * mov syscall_no -> %eax
 * mov arg1 -> %ebx
 * mov arg2 -> %ecx
 * mov arg3 -> %edx
 * int $0x80
 * mov %eax -> ret
 */
#define SYSCALL_3 \
  push %edi;               \
  push %esi;               \
  push %ebx;               \
  mov 0x10 (%esp), %eax;   \
  mov 0x14 (%esp), %ebx;   \
  mov 0x18 (%esp), %ecx;   \
  mov 0x1c (%esp), %edx;   \
  int $0x80;               \
  pop %ebx;                \
  pop %esi;                \
  pop %edi;               \
  ret;

#define _START             \
   mov %esp, %eax;         \
   add $4, %eax;           \
   push %eax;              \
   mov 0x4 (%esp), %eax;   \
   push %eax;              \
                           \
   call main;              \
                           \
   push %eax;              \
   call exit;              \
   1:                      \
   jmp 1b

#endif /* !ASM_X86_H */

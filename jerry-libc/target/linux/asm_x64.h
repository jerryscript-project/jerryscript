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

#ifndef ASM_X64_H
#define ASM_X64_H

/*
 * mov syscall_no (%rdi) -> %rax
 * mov arg1 (%rsi) -> %rdi
 * syscall
 */
#define SYSCALL_1 \
  mov %rdi, %rax; \
  mov %rsi, %rdi; \
  syscall; \
  ret;

/*
 * mov syscall_no (%rdi) -> %rax
 * mov arg1 (%rsi) -> %rdi
 * mov arg2 (%rdx) -> %rsi
 * syscall
 */
#define SYSCALL_2 \
  mov %rdi, %rax; \
  mov %rsi, %rdi; \
  mov %rdx, %rsi; \
  syscall; \
  ret;

/*
 * mov syscall_no (%rdi) -> %rax
 * mov arg1 (%rsi) -> %rdi
 * mov arg2 (%rdx) -> %rsi
 * mov arg3 (%rcx) -> %rdx
 * syscall
 */
#define SYSCALL_3 \
  mov %rdi, %rax; \
  mov %rsi, %rdi; \
  mov %rdx, %rsi; \
  mov %rcx, %rdx; \
  syscall; \
  ret;

#define _START            \
   mov (%rsp), %rdi;      \
   mov %rsp, %rsi;        \
   add $8, %rsi;          \
   callq main;            \
                          \
   mov %rax, %rdi;        \
   callq exit;          \
   1:                     \
   jmp 1b

#endif /* !ASM_X64_H */

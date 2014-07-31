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

#ifndef LINUX_X86_ASM_H
#define LINUX_X86_ASM_H

FIXME( Implement x86 ABI );

/*
 * mov syscall_no -> %rax
 * mov arg1 -> %rdi
 * syscall
 * mov %rax -> ret
 */
#define SYSCALL_1( syscall_no, arg1, ret) \
    __asm ( "syscall" \
          : "=a" ( ret ) \
          : "a" (syscall_no), "D" (arg1) \
          : "rcx", "r11" );

/*
 * mov syscall_no -> %rax
 * mov arg1 -> %rdi
 * mov arg2 -> %rsi
 * syscall
 * mov %rax -> ret
 */
#define SYSCALL_2( syscall_no, arg1, arg2, ret) \
    __asm ( "syscall" \
          : "=a" ( ret ) \
          : "a" (syscall_no), "D" (arg1), "S" (arg2) \
          : "rcx", "r11" );

/*
 * mov syscall_no -> %rax
 * mov arg1 -> %rdi
 * mov arg2 -> %rsi
 * mov arg2 -> %rdx
 * syscall
 * mov %rax -> ret
 */
#define SYSCALL_3( syscall_no, arg1, arg2, arg3, ret) \
    __asm ( "syscall" \
          : "=a" ( ret ) \
          : "a" (syscall_no), "D" (arg1), "S" (arg2), "d" (arg3) \
          : "rcx", "r11" );

#define _START            \
   mov (%rsp), %rdi;      \
   mov %rsp, %rsi;        \
   add $8, %rsi;          \
   callq main;            \
                          \
   mov %rax, %rdi;        \
   callq __exit;

#endif /* !LINUX_X86_ASM_H */

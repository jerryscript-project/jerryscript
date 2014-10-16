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

#ifndef ASM_ARM_H
#define ASM_ARM_H

/*
 * mov syscall_no -> %r7
 * mov arg1 -> %r0
 * svc
 * mov %r0 -> ret
 */
#define SYSCALL_1(syscall_no, arg1, ret) \
  __asm volatile ("mov r0, %[arg1]\n" \
                  "mov r7, %[syscall_no]\n" \
                  "svc #0\n" \
                  "mov %[ret], r0\n" \
                  : [ret] "=r" (ret) \
                  : [syscall_no] "r" (syscall_no), [arg1] "r" (arg1) \
                  : "r0", "r1", "r2", "r3", "r7", "r9", "memory");

/*
 * mov syscall_no -> %r7
 * mov arg1 -> %r0
 * mov arg2 -> %r1
 * syscall
 * mov %r0 -> ret
 */
#define SYSCALL_2(syscall_no, arg1, arg2, ret) \
  __asm volatile ("mov r0, %[arg1]\n" \
                  "mov r1, %[arg2]\n" \
                  "mov r7, %[syscall_no]\n" \
                  "svc #0\n" \
                  "mov %[ret], r0\n" \
                  : [ret] "=r" (ret) \
                  : [syscall_no] "r" (syscall_no), [arg1] "r" (arg1), [arg2] "r" (arg2) \
                  : "r0", "r1", "r2", "r3", "r7", "r9", "memory");

/*
 * mov syscall_no -> %r7
 * mov arg1 -> %r0
 * mov arg2 -> %r1
 * mov arg3 -> %r2
 * syscall
 * mov %r0 -> ret
 */
#define SYSCALL_3(syscall_no, arg1, arg2, arg3, ret) \
  __asm volatile ("mov r0, %[arg1]\n" \
                  "mov r1, %[arg2]\n" \
                  "mov r2, %[arg3]\n" \
                  "mov r7, %[syscall_no]\n" \
                  "svc #0\n" \
                  "mov %[ret], r0\n" \
                  : [ret] "=r" (ret) \
                  : [syscall_no] "r" (syscall_no), [arg1] "r" (arg1), [arg2] "r" (arg2), [arg3] "r" (arg3) \
                  : "r0", "r1", "r2", "r3", "r7", "r9", "memory");

#define _START            \
   bl main;               \
                          \
   bl __exit;             \
   1:                     \
   b 1b

#endif /* !ASM_ARM_H */

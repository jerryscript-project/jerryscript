/* Copyright JS Foundation and other contributors, http://js.foundation
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
 * syscall
 */
#define SYSCALL_0 \
  mov %rdi, %rax; \
  syscall; \
  ret;

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

#ifdef ENABLE_INIT_FINI
/*
 * call libc_init_array
 */
#define _INIT             \
  call libc_init_array;
#else /* !ENABLE_INIT_FINI */
#define _INIT
#endif /* ENABLE_INIT_FINI */

/*
 * call libc_init_array
 *
 * mov argc ([%rsp]) -> %rdi
 * mov argv (%rsp + 0x8) -> %rsi
 *
 * call main
 *
 * mov main_ret (%rax) -> %rdi
 * call exit
 *
 * infinite loop
 */
#define _START            \
  _INIT;                  \
                          \
  mov (%rsp), %rdi;       \
  mov %rsp, %rsi;         \
  add $8, %rsi;           \
  callq main;             \
                          \
  mov %rax, %rdi;         \
  callq exit;             \
  1:                      \
  jmp 1b;

/*
 * setjmp
 *
 * According to x86_64 System V ABI, the following registers are
 * callee-saved, and so need to be stored in context:
 *   - %rbp
 *   - %rbx
 *   - %r12
 *   - %r13
 *   - %r14
 *   - %r15
 *   - x87 control word
 *
 * Also, we should store:
 *   - %rsp (stack pointer)
 *   - return address (to jump to upon longjmp)
 *
 * mov return_address ([%rsp]) -> %rax
 *
 * mov %rsp -> jmp_buf_0  ([%rdi + 0x0])
 * mov %rax -> jmp_buf_8  ([%rdi + 0x8])
 * mov %rbp -> jmp_buf_16 ([%rdi + 0x10])
 * mov %rbx -> jmp_buf_24 ([%rdi + 0x18])
 * mov %r12 -> jmp_buf_32 ([%rdi + 0x20])
 * mov %r13 -> jmp_buf_40 ([%rdi + 0x28])
 * mov %r14 -> jmp_buf_48 ([%rdi + 0x30])
 * mov %r15 -> jmp_buf_56 ([%rdi + 0x38])
 * fnstcw   -> jmp_buf_64 ([%rdi + 0x40])
 *
 * ret
 */
#define _SETJMP \
  mov (%rsp), %rax;      \
                         \
  mov %rsp, 0x00(%rdi);  \
  mov %rax, 0x08(%rdi);  \
  mov %rbp, 0x10(%rdi);  \
  mov %rbx, 0x18(%rdi);  \
  mov %r12, 0x20(%rdi);  \
  mov %r13, 0x28(%rdi);  \
  mov %r14, 0x30(%rdi);  \
  mov %r15, 0x38(%rdi);  \
  fnstcw 0x40(%rdi);     \
                         \
  xor %rax, %rax;        \
                         \
  ret;

/*
 * longjmp
 *
 * See also:
 *          _SETJMP
 *
 * mov jmp_buf_0  ([%rdi + 0x0])  -> %rsp
 * mov jmp_buf_8  ([%rdi + 0x8])  -> %rax
 * mov jmp_buf_16 ([%rdi + 0x10]) -> %rbp
 * mov jmp_buf_24 ([%rdi + 0x18]) -> %rbx
 * mov jmp_buf_32 ([%rdi + 0x20]) -> %r12
 * mov jmp_buf_40 ([%rdi + 0x28]) -> %r13
 * mov jmp_buf_48 ([%rdi + 0x30]) -> %r14
 * mov jmp_buf_56 ([%rdi + 0x38]) -> %r15
 * fldcw jmp_buf_64 ([%rdi + 0x40])
 *
 * mov return_address (%rax) -> ([%rsp])
 *
 * mov val (%rsi) -> %rax
 *
 * test (%rax), (%rax)
 * jnz 1f
 * mov $1, %rax
 * 1:
 *
 * ret
 */
#define _LONGJMP \
  mov 0x00(%rdi), %rsp; \
  mov 0x08(%rdi), %rax; \
  mov 0x10(%rdi), %rbp; \
  mov 0x18(%rdi), %rbx; \
  mov 0x20(%rdi), %r12; \
  mov 0x28(%rdi), %r13; \
  mov 0x30(%rdi), %r14; \
  mov 0x38(%rdi), %r15; \
  fldcw 0x40(%rdi);     \
                        \
  mov %rax, (%rsp);     \
                        \
  mov %rsi, %rax;       \
                        \
  test %rax, %rax;      \
  jnz 1f;               \
  mov $1, %rax;         \
 1:                     \
                        \
  ret



#endif /* !ASM_X64_H */

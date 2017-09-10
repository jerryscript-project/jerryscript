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

#ifndef ASM_X86_H
#define ASM_X86_H

/*
 * mov syscall_no -> %eax
 * int $0x80
 * mov %eax -> ret
 */
#define SYSCALL_0 \
  push %edi;               \
  push %esi;               \
  push %ebx;               \
  mov 0x10 (%esp), %eax;   \
  int $0x80;               \
  pop %ebx;                \
  pop %esi;                \
  pop %edi;                \
  ret;

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
 * push argv (%esp + 4)
 * push argc ([%esp + 0x4])
 *
 * call main
 *
 * push main_ret (%eax)
 * call exit
 *
 * infinite loop
 */
#define _START            \
  _INIT;                  \
                          \
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
                          \
  1:                      \
  jmp 1b;

/*
 * setjmp
 *
 * According to x86_32 System V ABI, the following registers are
 * callee-saved, and so need to be stored in context:
 *   - %ebx
 *   - %esp
 *   - %ebp
 *   - %esi
 *   - %edi
 *   - x87 control word
 *
 * Also, we should store:
 *   - return address (to jump to upon longjmp)
 *
 * mov return_address ([%esp]) -> %eax
 *
 * mov env ([%esp + 0x4]) -> %edx
 *
 * mov %ebx -> jmp_buf_0  ([%edx + 0x0])
 * mov %esp -> jmp_buf_4  ([%edx + 0x4])
 * mov %ebp -> jmp_buf_8  ([%edx + 0x8])
 * mov %esi -> jmp_buf_12 ([%edx + 0xc])
 * mov %edi -> jmp_buf_16 ([%edx + 0x10])
 * mov %eax -> jmp_buf_20 ([%edx + 0x14])
 * fnstcw   -> jmp_buf_24 ([%edx + 0x18])
 *
 * ret
 */
#define _SETJMP \
  mov (%esp), %eax;       \
  mov 0x4 (%esp), %edx;   \
                          \
  mov %ebx, 0x00 (%edx);  \
  mov %esp, 0x04 (%edx);  \
  mov %ebp, 0x08 (%edx);  \
  mov %esi, 0x0c (%edx);  \
  mov %edi, 0x10 (%edx);  \
  mov %eax, 0x14 (%edx);  \
  fnstcw 0x18 (%edx);     \
                          \
  xor %eax, %eax;         \
                          \
  ret

/*
 * longjmp
 *
 * See also:
 *          _SETJMP
 *
 * mov env ([%esp + 0x4]) -> %edx
 * mov val ([%esp + 0x8]) -> %eax
 *
 * mov jmp_buf_0    ([%edx + 0x0])  -> %ebx
 * mov jmp_buf_4    ([%edx + 0x8])  -> %esp
 * mov jmp_buf_8    ([%edx + 0x10]) -> %ebp
 * mov jmp_buf_12   ([%edx + 0x18]) -> %esi
 * mov jmp_buf_16   ([%edx + 0x20]) -> %edi
 * mov jmp_buf_20   ([%edx + 0x28]) -> %ecx
 * fldcw jmp_buf_24 ([%edx + 0x30])
 *
 * mov return_address (%ecx) -> ([%esp])
 *
 * cmp (%eax), 0x0
 * jnz 1f
 * xor %eax, %eax
 * 1:
 *
 * ret
 */
#define _LONGJMP \
  mov 0x4 (%esp), %edx;   \
  mov 0x8 (%esp), %eax;   \
                          \
  mov 0x0 (%edx), %ebx;   \
  mov 0x4 (%edx), %esp;   \
  mov 0x8 (%edx), %ebp;   \
  mov 0xc (%edx), %esi;   \
  mov 0x10 (%edx), %edi;  \
  mov 0x14 (%edx), %ecx;  \
  fldcw 0x18 (%edx);      \
                          \
  mov %ecx, (%esp);       \
                          \
  test %eax, %eax;        \
  jnz 1f;                 \
  xor %eax, %eax;         \
 1:                       \
                          \
  ret

#endif /* !ASM_X86_H */

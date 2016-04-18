/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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

#ifndef JERRY_LIBC_SETJMP_H
#define JERRY_LIBC_SETJMP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 * Storage for context, used for nonlocal goto
 *
 * x86_64 (8 * 8 + 2 bytes):
 *   0x00 - %rsp
 *   0x08 - return address
 *   0x10 - %rbp
 *   0x18 - %rbx
 *   0x20 - %r12
 *   0x28 - %r13
 *   0x30 - %r14
 *   0x38 - %r15
 *   0x40 - x87 control word
 *
 * x86_32 (6 * 4 + 2 bytes):
 *   - %ebx
 *   - %esp
 *   - %ebp
 *   - %esi
 *   - %edi
 *   - return address (to jump to upon longjmp)
 *   - x87 control word
 *
 * ARMv7 (10 * 4 + 16 * 4 bytes):
 *   - r4 - r11, sp, lr
 *   - s16 - s31 (if hardfp enabled)
 *
 * See also:
 *          setjmp, longjmp
 */
typedef uint64_t jmp_buf[14];

int setjmp (jmp_buf env);
void longjmp (jmp_buf env, int val);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !JERRY_LIBC_SETJMP_H */

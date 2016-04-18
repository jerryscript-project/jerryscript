/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged
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

#if defined (__TARGET_HOST_x64)
#include "arch/x86-64.h"
#elif defined (__TARGET_HOST_x86)
#include "arch/x86-32.h"
#elif defined (__TARGET_HOST_ARMv7)
#include "arch/arm-v7.h"
#else /* !__TARGET_HOST_x64 && !__TARGET_HOST_x86 && !__TARGET_HOST_ARMv7 */
#error "Unsupported architecture"
#endif /* __TARGET_HOST_x64 */

#if defined (__linux__)
.macro func _name
.global \_name
.type \_name, %function
\_name:
.endm
.macro endfunc _name
.size \_name, .-\_name
.endm
#elif defined (__APPLE__) && defined (__MACH__)
.macro func _name
.global \_name
\_name:
.endm
.macro endfunc _name
.endm
#else /* !__linux && !(__APPLE__ && __MACH__) */
#error "Unsupported OS"
#endif /* __linux__ */

func _start
  _START
endfunc _start

func syscall_0
  SYSCALL_0
endfunc syscall_0

func syscall_1
  SYSCALL_1
endfunc syscall_1

func syscall_2
  SYSCALL_2
endfunc syscall_2

func syscall_3
  SYSCALL_3
endfunc syscall_3

/**
 * setjmp (jmp_buf env)
 *
 * See also:
 *          longjmp
 *
 * @return 0 - if returns from direct call,
 *         nonzero - if returns after longjmp.
 */
func setjmp
  _SETJMP
endfunc setjmp

/**
 * longjmp (jmp_buf env, int val)
 *
 * Note:
 *      if val is not 0, then it would be returned from setjmp,
 *      otherwise - 0 would be returned.
 *
 * See also:
 *          setjmp
 */
func longjmp
  _LONGJMP
endfunc longjmp

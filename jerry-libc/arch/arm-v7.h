/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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
 * svc #0
 */
#define SYSCALL_0 \
  push {r4-r12, lr}; \
  \
  mov r7, r0; \
  \
  svc #0; \
  \
  pop {r4-r12, pc};

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

/*
 * ldr argc ([sp + 0x0]) -> r0
 * add argv (sp + 0x4) -> r1
 *
 * bl main
 *
 * bl exit
 *
 * infinite loop
 */
#define _START            \
   ldr r0, [sp, #0];      \
   add r1, sp, #4;        \
   bl main;               \
                          \
   bl exit;               \
   1:                     \
   b 1b

/**
 * If hard-float mode:
 *   store s16-s31 vfp registers to buffer, pointed with r0 register,
 *   and increase the register on size of stored data.
 */
#ifdef __TARGET_HOST_ARMv7_HARD_FLOAT
# define _STORE_VFP_S16_S31_IF_HARD_FLOAT \
   vstm r0!, {s16 - s31};
# define _LOAD_VFP_S16_S31_IF_HARD_FLOAT \
   vldm r0!, {s16 - s31};
#else /* !__TARGET_HOST_ARMv7_HARD_FLOAT */
# define _STORE_VFP_S16_S31_IF_HARD_FLOAT
# define _LOAD_VFP_S16_S31_IF_HARD_FLOAT
#endif /* __TARGET_HOST_ARMv7_HARD_FLOAT */

/*
 * setjmp
 *
 * According to procedure call standard for the ARM architecture, the following
 * registers are callee-saved, and so need to be stored in context:
 *   - r4 - r11
 *   - sp
 *   - s16 - s31
 *
 * Also, we should store:
 *   - lr
 *
 * stmia {r4-r11, sp, lr} -> jmp_buf_0 (r0)!
 *
 * If hard-float build
 *   vstm  {s16-s31} -> jmp_buf_32 (r0)!
 *
 * mov r0, #0
 *
 * bx lr
 */
#define _SETJMP \
  stmia r0!, {r4 - r11, sp, lr};    \
                                    \
  _STORE_VFP_S16_S31_IF_HARD_FLOAT  \
                                    \
  mov r0, #0;                       \
                                    \
  bx lr;

/*
 * longjmp
 *
 * See also:
 *          _SETJMP
 *
 * ldmia jmp_buf_0 (r0)! -> {r4-r11, sp, lr}
 *
 * If hard-float build
 *   vldm  jmp_buf_32 (r0)! -> {s16-s31}
 *
 * mov r1 -> r0
 * cmp r0, #0
 * bne 1f
 * mov #1 -> r0
 * 1:
 *
 * bx lr
 */
#define _LONGJMP \
  ldmia r0!, {r4 - r11, sp, lr};    \
                                    \
  _LOAD_VFP_S16_S31_IF_HARD_FLOAT   \
                                    \
  mov r0, r1;                       \
  cmp r0, #0;                       \
  bne 1f;                           \
  mov r0, #1;                       \
  1:                                \
                                    \
  bx lr;

#endif /* !ASM_ARM_H */

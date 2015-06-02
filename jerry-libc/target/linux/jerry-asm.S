#ifdef __TARGET_HOST_x64
# include "asm_x64.h"
#elif defined (__TARGET_HOST_x86)
# include "asm_x86.h"
#elif defined (__TARGET_HOST_ARMv7)
# include "asm_arm.h"
#else /* !__TARGET_HOST_x64 && !__TARGET_HOST_x86 && !__TARGET_HOST_ARMv7 */
# error "!__TARGET_HOST_x64 && !__TARGET_HOST_x86 && !__TARGET_HOST_ARMv7"
#endif /* !__TARGET_HOST_x64 && !__TARGET_HOST_x86 && !__TARGET_HOST_ARMv7 */

.global _start
.type _start, %function
_start:
  _START
.size _start, . - _start

.global syscall_0_asm
.type syscall_0_asm, %function
syscall_0_asm:
  SYSCALL_0
.size syscall_0_asm, . - syscall_0_asm

.global syscall_1_asm
.type syscall_1_asm, %function
syscall_1_asm:
  SYSCALL_1
.size syscall_1_asm, . - syscall_1_asm

.global syscall_2_asm
.type syscall_2_asm, %function
syscall_2_asm:
  SYSCALL_2
.size syscall_2_asm, . - syscall_2_asm

.global syscall_3_asm
.type syscall_3_asm, %function
syscall_3_asm:
  SYSCALL_3
.size syscall_3_asm, . - syscall_3_asm


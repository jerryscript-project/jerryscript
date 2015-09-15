#ifdef __TARGET_HOST_x64
# include "arch/x86-64.h"
#elif defined (__TARGET_HOST_x86)
# include "arch/x86-32.h"
#elif defined (__TARGET_HOST_ARMv7)
# include "arch/arm-v7.h"
#else /* !__TARGET_HOST_x64 && !__TARGET_HOST_x86 && !__TARGET_HOST_ARMv7 */
# error "!__TARGET_HOST_x64 && !__TARGET_HOST_x86 && !__TARGET_HOST_ARMv7"
#endif /* !__TARGET_HOST_x64 && !__TARGET_HOST_x86 && !__TARGET_HOST_ARMv7 */

.global _start
_start:
  _START

.global syscall_0_asm
syscall_0_asm:
  SYSCALL_0

.global syscall_1_asm
syscall_1_asm:
  SYSCALL_1

.global syscall_2_asm
syscall_2_asm:
  SYSCALL_2

.global syscall_3_asm
syscall_3_asm:
  SYSCALL_3

/**
 * setjmp (jmp_buf env)
 *
 * See also:
 *          longjmp
 *
 * @return 0 - if returns from direct call,
 *         nonzero - if returns after longjmp.
 */
.global setjmp
setjmp:
  _SETJMP

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
.global longjmp
longjmp:
  _LONGJMP

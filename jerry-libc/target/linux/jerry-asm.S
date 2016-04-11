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
.type _start, %function
_start:
  _START
.size _start, . - _start

.global syscall_0
.type syscall_0, %function
syscall_0:
  SYSCALL_0
.size syscall_0, . - syscall_0

.global syscall_1
.type syscall_1, %function
syscall_1:
  SYSCALL_1
.size syscall_1, . - syscall_1

.global syscall_2
.type syscall_2, %function
syscall_2:
  SYSCALL_2
.size syscall_2, . - syscall_2

.global syscall_3
.type syscall_3, %function
syscall_3:
  SYSCALL_3
.size syscall_3, . - syscall_3

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
.type setjmp, %function
setjmp:
  _SETJMP
.size setjmp, . - setjmp

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
.type longjmp, %function
longjmp:
  _LONGJMP
.size longjmp, . - setjmp

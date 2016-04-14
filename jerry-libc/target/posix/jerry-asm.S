#if defined (__TARGET_HOST_x64)
#include "arch/x86-64.h"
#elif defined (__TARGET_HOST_x86)
#include "arch/x86-32.h"
#elif defined (__TARGET_HOST_ARMv7)
#include "arch/arm-v7.h"
#else
#error "Unsupported architecture"
#endif /* !__TARGET_HOST_x64 && !__TARGET_HOST_x86 && !__TARGET_HOST_ARMv7 */

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
#else
#error "Unsupported OS"
#endif /* !__linux && !(__APPLE__ && __MACH__) */

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

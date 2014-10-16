#ifdef __TARGET_HOST_x64
# include "asm_x64.h"
#elif defined (__TARGET_HOST_x86)
# include "asm_x86.h"
#elif defined (__TARGET_HOST_ARMv7)
# include "asm_arm.h"
#else /* !__TARGET_HOST_x64 && !__TARGET_HOST_x86 && !__TARGET_HOST_ARMv7 */
# error "!__TARGET_HOST_x64 && !__TARGET_HOST_x86 && !__TARGET_HOST_ARMv7"
#endif /* !__TARGET_HOST_x64 && !__TARGET_HOST_x86 && !__TARGET_HOST_ARMv7 */

#ifdef LIBC_RAW
.global _start
_start:
  _START
.end _start
#endif /* LIBC_RAW */

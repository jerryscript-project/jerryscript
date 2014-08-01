#ifdef __TARGET_HOST_x64
# include "asm_x64.h"
#elif defined(__TARGET_HOST_x86)
# include "asm_x86.h"
#else /* !__HOST && !__TARGET_HOST_x86 */
# error "!__HOST && !__TARGET_HOST_x86"
#endif /* !__HOST && !__TARGET_HOST_x86 */

#ifdef LIBC_RAW
.global _start
_start:
  _START
1:
  jmp 1b
.end _start
#endif /* LIBC_RAW */

/**
 * Runtime ABI for ARM architecture
 */

/**
 * long long __aeabi_llsl(long long {r1, r0}, int {r2})
 */
.thumb_func
.global __aeabi_llsl
__aeabi_llsl:
  // if (r2 >= 64) goto 2f;
  cmp r2, #64
  bhs 2f

  // if (r2 < 32) goto 1f;
  cmp r2, #32
  blo 1f

  // r1 = (r0 << (r2 - 32));
  // r0 = 0;
  sub r2, #32
  mov r1, r0
  lsl r1, r2
  mov r0, #0

  bx lr
  
  1:
  // r1 <<= r2;
  lsl r1, r2

  // r2 = 32 - r2;
  rsb r2, r2, #32

  // r1 |= (r0 >> r2);
  mov r3, r0
  lsr r3, r2
  orr r1, r3

  // r2 = 32 - r2;
  rsb r2, r2, #32

  // r0 <<= r2;
  lsl r0, r2

  bx lr

  2:
  // r1 = 0;
  // r0 = 0;
  mov r1, #0
  mov r0, #0

  bx lr
.end __aeabi_llsl
